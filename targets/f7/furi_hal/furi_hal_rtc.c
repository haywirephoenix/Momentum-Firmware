#include <furi_hal_interrupt.h>
#include <furi_hal_rtc.h>
#include <furi_hal_light.h>
#include <furi_hal_debug.h>
#include <furi_hal_serial_control.h>

#include <stm32wbxx_ll_pwr.h>
#include <stm32wbxx_ll_bus.h>
#include <stm32wbxx_ll_rcc.h>
#include <stm32wbxx_ll_rtc.h>
#include <stm32wbxx_ll_utils.h>

#include <furi.h>

#define TAG "FuriHalRtc"

#define FURI_HAL_RTC_LSE_STARTUP_TIME 300

#define FURI_HAL_RTC_CLOCK_IS_READY() (LL_RCC_LSE_IsReady() && LL_RCC_LSI1_IsReady())

#define FURI_HAL_RTC_HEADER_MAGIC   0x10F1
#define FURI_HAL_RTC_HEADER_VERSION 0

typedef struct {
    uint16_t magic;
    uint8_t version;
    uint8_t unused;
} FuriHalRtcHeader;

typedef struct {
    uint8_t log_level    : 4;
    uint8_t log_reserved : 4;
    uint8_t flags;
    FuriHalRtcBootMode boot_mode                 : 4;
    FuriHalRtcHeapTrackMode heap_track_mode      : 2;
    FuriHalRtcLocaleUnits locale_units           : 1;
    FuriHalRtcLocaleTimeFormat locale_timeformat : 1;
    FuriHalRtcLocaleDateFormat locale_dateformat : 2;
    FuriHalRtcLogDevice log_device               : 2;
    FuriHalRtcLogBaudRate log_baud_rate          : 3;
    uint8_t reserved                             : 1;
} SystemReg;

_Static_assert(sizeof(SystemReg) == 4, "SystemReg size mismatch");

typedef struct {
    FuriHalRtcAlarmCallback alarm_callback;
    void* alarm_callback_context;
} FuriHalRtc;

static FuriHalRtc furi_hal_rtc = {};

static const FuriHalSerialId furi_hal_rtc_log_devices[] = {
    [FuriHalRtcLogDeviceUsart] = FuriHalSerialIdUsart,
    [FuriHalRtcLogDeviceLpuart] = FuriHalSerialIdLpuart,
    [FuriHalRtcLogDeviceReserved] = FuriHalSerialIdMax,
    [FuriHalRtcLogDeviceNone] = FuriHalSerialIdMax,
};

static const uint32_t furi_hal_rtc_log_baud_rates[] = {
    [FuriHalRtcLogBaudRate230400] = 230400,
    [FuriHalRtcLogBaudRate9600] = 9600,
    [FuriHalRtcLogBaudRate38400] = 38400,
    [FuriHalRtcLogBaudRate57600] = 57600,
    [FuriHalRtcLogBaudRate115200] = 115200,
    [FuriHalRtcLogBaudRate460800] = 460800,
    [FuriHalRtcLogBaudRate921600] = 921600,
    [FuriHalRtcLogBaudRate1843200] = 1843200,
};

static void furi_hal_rtc_enter_init_mode(void) {
    LL_RTC_EnableInitMode(RTC);
    while(LL_RTC_IsActiveFlag_INIT(RTC) != 1)
        ;
}

static void furi_hal_rtc_exit_init_mode(void) {
    LL_RTC_DisableInitMode(RTC);
    furi_hal_rtc_sync_shadow();
}

static void furi_hal_rtc_reset(void) {
    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();
}

static bool furi_hal_rtc_start_clock_and_switch(void) {
    // Clock operation require access to Backup Domain
    LL_PWR_EnableBkUpAccess();

    // Enable LSI and LSE
    LL_RCC_LSI1_Enable();
    LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_HIGH);
    LL_RCC_LSE_Enable();

    // Wait for LSI and LSE startup
    uint32_t c = 0;
    while(!FURI_HAL_RTC_CLOCK_IS_READY() && c < FURI_HAL_RTC_LSE_STARTUP_TIME) {
        LL_mDelay(1);
        c++;
    }

    if(FURI_HAL_RTC_CLOCK_IS_READY()) {
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
        LL_RCC_EnableRTC();
        return LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSE;
    } else {
        return false;
    }
}

static void furi_hal_rtc_recover(void) {
    DateTime datetime = {0};

    // Handle fixable LSE failure
    if(LL_RCC_LSE_IsCSSDetected()) {
        furi_hal_light_sequence("rgb B");
        // Shutdown LSE and LSECSS
        LL_RCC_LSE_DisableCSS();
        LL_RCC_LSE_Disable();
    } else {
        furi_hal_light_sequence("rgb R");
    }

    // Temporary switch to LSI
    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
    if(LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSI) {
        // Get datetime before RTC Domain reset
        furi_hal_rtc_get_datetime(&datetime);
    }

    // Reset RTC Domain
    furi_hal_rtc_reset();

    // Start Clock
    if(!furi_hal_rtc_start_clock_and_switch()) {
        // Plan C: reset RTC and restart
        furi_hal_light_sequence("rgb R.r.R.r.R.r");
        furi_hal_rtc_reset();
        NVIC_SystemReset();
    }

    // Set date if it valid
    if(datetime.year != 0) {
        furi_hal_rtc_set_datetime(&datetime);
    }
}

static void furi_hal_rtc_alarm_handler(void* context) {
    UNUSED(context);

    if(LL_RTC_IsActiveFlag_ALRA(RTC) != 0) {
        /* Clear the Alarm interrupt pending bit */
        LL_RTC_ClearFlag_ALRA(RTC);

        /* Alarm callback */
        furi_check(furi_hal_rtc.alarm_callback);
        furi_hal_rtc.alarm_callback(furi_hal_rtc.alarm_callback_context);
    }
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17);
}

static void furi_hal_rtc_set_alarm_out(bool enable) {
    FURI_CRITICAL_ENTER();
    LL_RTC_DisableWriteProtection(RTC);
    if(enable) {
        LL_RTC_SetAlarmOutEvent(RTC, LL_RTC_ALARMOUT_ALMA);
        LL_RTC_SetOutputPolarity(RTC, LL_RTC_OUTPUTPOLARITY_PIN_LOW);
        LL_RTC_SetAlarmOutputType(RTC, LL_RTC_ALARM_OUTPUTTYPE_OPENDRAIN);
    } else {
        LL_RTC_SetAlarmOutEvent(RTC, LL_RTC_ALARMOUT_DISABLE);
        LL_RTC_SetOutputPolarity(RTC, LL_RTC_OUTPUTPOLARITY_PIN_LOW);
        LL_RTC_SetAlarmOutputType(RTC, LL_RTC_ALARM_OUTPUTTYPE_OPENDRAIN);
    }
    LL_RTC_EnableWriteProtection(RTC);
    FURI_CRITICAL_EXIT();
}

void furi_hal_rtc_init_early(void) {
    // Enable RTCAPB clock
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_RTCAPB);

    // Prepare clock
    if(!furi_hal_rtc_start_clock_and_switch()) {
        // Plan B: try to recover
        furi_hal_rtc_recover();
    }

    // Verify header register
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterHeader);
    FuriHalRtcHeader* data = (FuriHalRtcHeader*)&data_reg;
    if(data->magic != FURI_HAL_RTC_HEADER_MAGIC || data->version != FURI_HAL_RTC_HEADER_VERSION) {
        furi_hal_rtc_reset_registers();
    }

    if(furi_hal_rtc_is_flag_set(FuriHalRtcFlagDebug)) {
        furi_hal_debug_enable();
    } else {
        furi_hal_debug_disable();
    }
}

void furi_hal_rtc_deinit_early(void) {
}

void furi_hal_rtc_init(void) {
    LL_RTC_InitTypeDef RTC_InitStruct;
    RTC_InitStruct.HourFormat = LL_RTC_HOURFORMAT_24HOUR;
    RTC_InitStruct.AsynchPrescaler = 127;
    RTC_InitStruct.SynchPrescaler = 255;
    LL_RTC_Init(RTC, &RTC_InitStruct);

    furi_log_set_level(furi_hal_rtc_get_log_level());
    furi_hal_serial_control_set_logging_config(
        furi_hal_rtc_log_devices[furi_hal_rtc_get_log_device()],
        furi_hal_rtc_log_baud_rates[furi_hal_rtc_get_log_baud_rate()]);

    FURI_LOG_I(TAG, "Init OK");
    furi_hal_rtc_set_alarm_out(false);
}

void furi_hal_rtc_prepare_for_shutdown(void) {
    furi_hal_rtc_set_alarm_out(true);
}

void furi_hal_rtc_sync_shadow(void) {
    if(!LL_RTC_IsShadowRegBypassEnabled(RTC)) {
        LL_RTC_ClearFlag_RS(RTC);
        while(!LL_RTC_IsActiveFlag_RS(RTC)) {
        };
    }
}

void furi_hal_rtc_reset_registers(void) {
    for(size_t i = 0; i < RTC_BKP_NUMBER; i++) {
        furi_hal_rtc_set_register(i, 0);
    }

    uint32_t data_reg = 0;
    FuriHalRtcHeader* data = (FuriHalRtcHeader*)&data_reg;
    data->magic = FURI_HAL_RTC_HEADER_MAGIC;
    data->version = FURI_HAL_RTC_HEADER_VERSION;
    furi_hal_rtc_set_register(FuriHalRtcRegisterHeader, data_reg);

    // Initialize extended flags register
    furi_hal_rtc_set_register(FuriHalRtcRegisterExtendedFlags, 0);
}

uint32_t furi_hal_rtc_get_register(FuriHalRtcRegister reg) {
    return LL_RTC_BAK_GetRegister(RTC, reg);
}

void furi_hal_rtc_set_register(FuriHalRtcRegister reg, uint32_t value) {
    LL_RTC_BAK_SetRegister(RTC, reg, value);
}

void furi_hal_rtc_set_log_level(uint8_t level) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->log_level = level;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
    furi_log_set_level(level);
}

uint8_t furi_hal_rtc_get_log_level(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->log_level;
}

void furi_hal_rtc_set_log_device(FuriHalRtcLogDevice device) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->log_device = device;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);

    furi_hal_serial_control_set_logging_config(
        furi_hal_rtc_log_devices[furi_hal_rtc_get_log_device()],
        furi_hal_rtc_log_baud_rates[furi_hal_rtc_get_log_baud_rate()]);
}

FuriHalRtcLogDevice furi_hal_rtc_get_log_device(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->log_device;
}

void furi_hal_rtc_set_log_baud_rate(FuriHalRtcLogBaudRate baud_rate) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->log_baud_rate = baud_rate;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);

    furi_hal_serial_control_set_logging_config(
        furi_hal_rtc_log_devices[furi_hal_rtc_get_log_device()],
        furi_hal_rtc_log_baud_rates[furi_hal_rtc_get_log_baud_rate()]);
}

FuriHalRtcLogBaudRate furi_hal_rtc_get_log_baud_rate(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->log_baud_rate;
}

void furi_hal_rtc_set_flag(FuriHalRtcFlag flag) {
    if(flag <= (1 << 7)) { // Original flags
        uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
        SystemReg* data = (SystemReg*)&data_reg;
        data->flags |= flag;
        furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
    } else { // Extended flags
        uint32_t ext_flags = furi_hal_rtc_get_register(FuriHalRtcRegisterExtendedFlags);
        ext_flags |= (flag >> 8); // Shift back to use first bits of new register
        furi_hal_rtc_set_register(FuriHalRtcRegisterExtendedFlags, ext_flags);
    }

    if(flag & FuriHalRtcFlagDebug) {
        furi_hal_debug_enable();
    }
}

void furi_hal_rtc_reset_flag(FuriHalRtcFlag flag) {
    if(flag <= (1 << 7)) { // Original flags
        uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
        SystemReg* data = (SystemReg*)&data_reg;
        data->flags &= ~flag;
        furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
    } else { // Extended flags
        uint32_t ext_flags = furi_hal_rtc_get_register(FuriHalRtcRegisterExtendedFlags);
        ext_flags &= ~(flag >> 8);
        furi_hal_rtc_set_register(FuriHalRtcRegisterExtendedFlags, ext_flags);
    }

    if(flag & FuriHalRtcFlagDebug) {
        furi_hal_debug_disable();
    }
}

bool furi_hal_rtc_is_flag_set(FuriHalRtcFlag flag) {
    if(flag <= (1 << 7)) { // Original flags
        uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
        SystemReg* data = (SystemReg*)&data_reg;
        return data->flags & flag;
    } else { // Extended flags
        uint32_t ext_flags = furi_hal_rtc_get_register(FuriHalRtcRegisterExtendedFlags);
        return ext_flags & (flag >> 8);
    }
}

void furi_hal_rtc_set_boot_mode(FuriHalRtcBootMode mode) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->boot_mode = mode;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
}

FuriHalRtcBootMode furi_hal_rtc_get_boot_mode(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->boot_mode;
}

void furi_hal_rtc_set_heap_track_mode(FuriHalRtcHeapTrackMode mode) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->heap_track_mode = mode;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
}

FuriHalRtcHeapTrackMode furi_hal_rtc_get_heap_track_mode(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->heap_track_mode;
}

void furi_hal_rtc_set_locale_units(FuriHalRtcLocaleUnits value) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->locale_units = value;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
}

FuriHalRtcLocaleUnits furi_hal_rtc_get_locale_units(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->locale_units;
}

void furi_hal_rtc_set_locale_timeformat(FuriHalRtcLocaleTimeFormat value) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->locale_timeformat = value;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
}

FuriHalRtcLocaleTimeFormat furi_hal_rtc_get_locale_timeformat(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->locale_timeformat;
}

void furi_hal_rtc_set_locale_dateformat(FuriHalRtcLocaleDateFormat value) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->locale_dateformat = value;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
}

FuriHalRtcLocaleDateFormat furi_hal_rtc_get_locale_dateformat(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->locale_dateformat;
}

void furi_hal_rtc_set_datetime(DateTime* datetime) {
    furi_check(!FURI_IS_IRQ_MODE());
    furi_check(datetime);

    FURI_CRITICAL_ENTER();
    /* Disable write protection */
    LL_RTC_DisableWriteProtection(RTC);

    /* Enter Initialization mode and wait for INIT flag to be set */
    furi_hal_rtc_enter_init_mode();

    /* Set time */
    LL_RTC_TIME_Config(
        RTC,
        LL_RTC_TIME_FORMAT_AM_OR_24,
        __LL_RTC_CONVERT_BIN2BCD(datetime->hour),
        __LL_RTC_CONVERT_BIN2BCD(datetime->minute),
        __LL_RTC_CONVERT_BIN2BCD(datetime->second));

    /* Set date */
    LL_RTC_DATE_Config(
        RTC,
        datetime->weekday,
        __LL_RTC_CONVERT_BIN2BCD(datetime->day),
        __LL_RTC_CONVERT_BIN2BCD(datetime->month),
        __LL_RTC_CONVERT_BIN2BCD(datetime->year - 2000));

    /* Exit Initialization mode */
    furi_hal_rtc_exit_init_mode();

    /* Enable write protection */
    LL_RTC_EnableWriteProtection(RTC);
    FURI_CRITICAL_EXIT();
}

void furi_hal_rtc_get_datetime(DateTime* datetime) {
    furi_check(!FURI_IS_IRQ_MODE());
    furi_check(datetime);

    FURI_CRITICAL_ENTER();
    uint32_t time = LL_RTC_TIME_Get(RTC); // 0x00HHMMSS
    uint32_t date = LL_RTC_DATE_Get(RTC); // 0xWWDDMMYY
    FURI_CRITICAL_EXIT();

    datetime->second = __LL_RTC_CONVERT_BCD2BIN((time >> 0) & 0xFF);
    datetime->minute = __LL_RTC_CONVERT_BCD2BIN((time >> 8) & 0xFF);
    datetime->hour = __LL_RTC_CONVERT_BCD2BIN((time >> 16) & 0xFF);
    datetime->year = __LL_RTC_CONVERT_BCD2BIN((date >> 0) & 0xFF) + 2000;
    datetime->month = __LL_RTC_CONVERT_BCD2BIN((date >> 8) & 0xFF);
    datetime->day = __LL_RTC_CONVERT_BCD2BIN((date >> 16) & 0xFF);
    datetime->weekday = __LL_RTC_CONVERT_BCD2BIN((date >> 24) & 0xFF);
}

void furi_hal_rtc_set_alarm(const DateTime* datetime, bool enabled) {
    furi_check(!FURI_IS_IRQ_MODE());

    FURI_CRITICAL_ENTER();
    LL_RTC_DisableWriteProtection(RTC);

    if(datetime) {
        LL_RTC_ALMA_ConfigTime(
            RTC,
            LL_RTC_ALMA_TIME_FORMAT_AM,
            __LL_RTC_CONVERT_BIN2BCD(datetime->hour),
            __LL_RTC_CONVERT_BIN2BCD(datetime->minute),
            __LL_RTC_CONVERT_BIN2BCD(datetime->second));
        LL_RTC_ALMA_SetMask(RTC, LL_RTC_ALMA_MASK_DATEWEEKDAY);
    }

    if(enabled) {
        LL_RTC_ClearFlag_ALRA(RTC);
        LL_RTC_ALMA_Enable(RTC);
    } else {
        LL_RTC_ALMA_Disable(RTC);
        LL_RTC_ClearFlag_ALRA(RTC);
    }

    LL_RTC_EnableWriteProtection(RTC);
    FURI_CRITICAL_EXIT();
}

bool furi_hal_rtc_get_alarm(DateTime* datetime) {
    furi_check(datetime);

    memset(datetime, 0, sizeof(DateTime));

    datetime->hour = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_ALMA_GetHour(RTC));
    datetime->minute = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_ALMA_GetMinute(RTC));
    datetime->second = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_ALMA_GetSecond(RTC));

    return READ_BIT(RTC->CR, RTC_CR_ALRAE);
}

void furi_hal_rtc_set_alarm_callback(FuriHalRtcAlarmCallback callback, void* context) {
    FURI_CRITICAL_ENTER();
    LL_RTC_DisableWriteProtection(RTC);
    if(callback) {
        furi_check(!furi_hal_rtc.alarm_callback);
        // Set our callbacks
        furi_hal_rtc.alarm_callback = callback;
        furi_hal_rtc.alarm_callback_context = context;
        // Enable RTC ISR
        furi_hal_interrupt_set_isr(FuriHalInterruptIdRtcAlarm, furi_hal_rtc_alarm_handler, NULL);
        // Hello EXTI my old friend
        // Chain: RTC->LINE-17->EXTI->NVIC->FuriHalInterruptIdRtcAlarm
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_17);
        LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_17);
        // Enable alarm interrupt
        LL_RTC_EnableIT_ALRA(RTC);
        // Force trigger
        furi_hal_rtc_alarm_handler(NULL);
    } else {
        furi_check(furi_hal_rtc.alarm_callback);
        // Cleanup EXTI flags and config
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_17);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17);
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_17);
        // Cleanup NVIC flags and config
        furi_hal_interrupt_set_isr(FuriHalInterruptIdRtcAlarm, NULL, NULL);
        // Disable alarm interrupt
        LL_RTC_DisableIT_ALRA(RTC);

        furi_hal_rtc.alarm_callback = NULL;
        furi_hal_rtc.alarm_callback_context = NULL;
    }
    LL_RTC_EnableWriteProtection(RTC);
    FURI_CRITICAL_EXIT();
}

void furi_hal_rtc_set_fault_data(uint32_t value) {
    furi_hal_rtc_set_register(FuriHalRtcRegisterFaultData, value);
}

uint32_t furi_hal_rtc_get_fault_data(void) {
    return furi_hal_rtc_get_register(FuriHalRtcRegisterFaultData);
}

void furi_hal_rtc_set_pin_fails(uint32_t value) {
    furi_hal_rtc_set_register(FuriHalRtcRegisterPinFails, value);
}

uint32_t furi_hal_rtc_get_pin_fails(void) {
    return furi_hal_rtc_get_register(FuriHalRtcRegisterPinFails);
}

void furi_hal_rtc_set_pin_value(uint32_t value) {
    furi_hal_rtc_set_register(FuriHalRtcRegisterPinValue, value);
}

uint32_t furi_hal_rtc_get_pin_value(void) {
    return furi_hal_rtc_get_register(FuriHalRtcRegisterPinValue);
}

uint32_t furi_hal_rtc_get_timestamp(void) {
    DateTime datetime = {0};
    furi_hal_rtc_get_datetime(&datetime);
    return datetime_datetime_to_timestamp(&datetime);
}
