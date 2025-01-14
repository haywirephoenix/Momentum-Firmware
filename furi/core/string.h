/** Furi string container
 * 
 * And various method to manipulate strings
 *
 * @file string.h
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <m-core.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Furi string failure constant. */
#define FURI_STRING_FAILURE ((size_t) - 1)

/** Furi string primitive. */
typedef struct FuriString FuriString;

//---------------------------------------------------------------------------
//                               Constructors
//---------------------------------------------------------------------------

/** Allocate new FuriString.
 *
 * @return     pointer to the instance of FuriString
 */
FuriString* furi_string_alloc(void);

/** Allocate new FuriString and set it to string.
 *
 * Allocate & Set the string a to the string.
 *
 * @param      source  The source FuriString instance
 *
 * @return     pointer to the new instance of FuriString
 */
FuriString* furi_string_alloc_set(const FuriString* source);

/** Allocate new FuriString and set it to C string.
 *
 * Allocate & Set the string a to the C string.
 *
 * @param      cstr_source  The C-string instance
 *
 * @return     pointer to the new instance of FuriString
 */
FuriString* furi_string_alloc_set_str(const char cstr_source[]);

/** Allocate new FuriString and printf to it.
 *
 * Initialize and set a string to the given formatted value.
 * 
 * @param      format     The printf format
 * @param[in]  ...        args to format
 *
 * @return     pointer to the new instance of FuriString
 */
FuriString* furi_string_alloc_printf(const char format[], ...)
    _ATTRIBUTE((__format__(__printf__, 1, 2)));

/** Allocate new FuriString and printf to it.
 *
 * Initialize and set a string to the given formatted value.
 *
 * @param      format  The printf format
 * @param      args    The format arguments
 *
 * @return     pointer to the new instance of FuriString
 */
FuriString* furi_string_alloc_vprintf(const char format[], va_list args);

/** Allocate new FuriString and move source string content to it.
 *
 * Allocate the string, set it to the other one, and destroy the other one.
 *
 * @param      source  The source FuriString instance
 *
 * @return     pointer to the new instance of FuriString
 */
FuriString* furi_string_alloc_move(FuriString* source);

//---------------------------------------------------------------------------
//                               Destructors
//---------------------------------------------------------------------------

/** Free FuriString.
 *
 * @param      string  The FuriString instance to free
 */
void furi_string_free(FuriString* string);

//---------------------------------------------------------------------------
//                         String memory management
//---------------------------------------------------------------------------

/** Reserve memory for string.
 *
 * Modify the string capacity to be able to handle at least 'alloc' characters
 * (including final null char).
 *
 * @param      string  The FuriString instance
 * @param      size    The size to reserve
 */
void furi_string_reserve(FuriString* string, size_t size);

/** Reset string.
 *
 * Make the string empty.
 *
 * @param      string  The FuriString instance
 */
void furi_string_reset(FuriString* string);

/** Swap two strings.
 *
 * Swap the two strings string_1 and string_2.
 *
 * @param      string_1  The FuriString instance 1
 * @param      string_2  The FuriString instance 2
 */
void furi_string_swap(FuriString* string_1, FuriString* string_2);

/** Move string_2 content to string_1.
 *
 * Copy data from one string to another and destroy the source.
 *
 * @param      destination  The destination FuriString
 * @param      source  The source FuriString
 */
void furi_string_move(FuriString* destination, FuriString* source);

/** Compute a hash for the string.
 *
 * @param      string  The FuriString instance
 *
 * @return     hash value
 */
size_t furi_string_hash(const FuriString* string);

/** Get string size (usually length, but not for UTF-8)
 *
 * @param      string  The FuriString instance
 *
 * @return     size of the string
 */
size_t furi_string_size(const FuriString* string);

/** Check that string is empty or not
 *
 * @param      string  The FuriString instance
 *
 * @return     true if empty otherwise false
 */
bool furi_string_empty(const FuriString* string);

//---------------------------------------------------------------------------
//                               Getters
//---------------------------------------------------------------------------

/** Get the character at the given index.
 *
 * Return the selected character of the string.
 *
 * @param      string  The FuriString instance
 * @param      index   The index
 *
 * @return     character at index
 */
char furi_string_get_char(const FuriString* string, size_t index);

/** Return the string view a classic C string.
 *
 * @param      string  The FuriString instance
 *
 * @return     const C-string, usable till first container change
 */
const char* furi_string_get_cstr(const FuriString* string);

//---------------------------------------------------------------------------
//                               Setters
//---------------------------------------------------------------------------

/** Set the string to the other string.
 *
 * Set the string to the source string.
 *
 * @param      string  The FuriString instance
 * @param      source  The source
 */
void furi_string_set(FuriString* string, FuriString* source);

/** Set the string to the other C string.
 *
 * Set the string to the source C string.
 *
 * @param      string  The FuriString instance
 * @param      source  The source
 */
void furi_string_set_str(FuriString* string, const char source[]);

/** Set the string to the n first characters of the C string.
 *
 * @param      string  The FuriString instance
 * @param      source  The source
 * @param      length  The length
 */
void furi_string_set_strn(FuriString* string, const char source[], size_t length);

/** Set the character at the given index.
 *
 * @param      string  The FuriString instance
 * @param      index   The index
 * @param      c       The character
 */
void furi_string_set_char(FuriString* string, size_t index, const char c);

/** Set the string to the n first characters of other one.
 *
 * @param      string  The FuriString instance
 * @param      source  The source
 * @param      offset  The offset
 * @param      length  The length
 */
void furi_string_set_n(FuriString* string, const FuriString* source, size_t offset, size_t length);

/** Format in the string the given printf format
 *
 * @param      string     The string
 * @param      format     The format
 * @param[in]  ...        The args
 *
 * @return     number of characters printed or negative value on error
 */
int furi_string_printf(FuriString* string, const char format[], ...)
    _ATTRIBUTE((__format__(__printf__, 2, 3)));

/** Format in the string the given printf format
 *
 * @param      string  The FuriString instance
 * @param      format  The format
 * @param      args    The arguments
 *
 * @return     number of characters printed or negative value on error
 */
int furi_string_vprintf(FuriString* string, const char format[], va_list args);

//---------------------------------------------------------------------------
//                               Appending
//---------------------------------------------------------------------------

/** Append a character to the string.
 *
 * @param      string  The FuriString instance
 * @param      c       The character
 */
void furi_string_push_back(FuriString* string, char c);

/** Append a string to the string.
 *
 * Concatenate the string with the other string.
 *
 * @param      string_1  The string 1
 * @param      string_2  The string 2
 */
void furi_string_cat(FuriString* string_1, const FuriString* string_2);

/** Append a C string to the string.
 *
 * Concatenate the string with the C string.
 *
 * @param      string_1   The string 1
 * @param      cstring_2  The cstring 2
 */
void furi_string_cat_str(FuriString* string_1, const char cstring_2[]);

/** Append to the string the formatted string of the given printf format.
 *
 * @param      string     The string
 * @param      format     The format
 * @param[in]  ...        The args
 *
 * @return     number of characters printed or negative value on error
 */
int furi_string_cat_printf(FuriString* string, const char format[], ...)
    _ATTRIBUTE((__format__(__printf__, 2, 3)));

/** Append to the string the formatted string of the given printf format.
 *
 * @param      string  The FuriString instance
 * @param      format  The format
 * @param      args    The arguments
 *
 * @return     number of characters printed or negative value on error
 */
int furi_string_cat_vprintf(FuriString* string, const char format[], va_list args);

//---------------------------------------------------------------------------
//                               Comparators
//---------------------------------------------------------------------------

/** Compare two strings and return the sort order.
 *
 * @param      string_1  The string 1
 * @param      string_2  The string 2
 *
 * @return     zero if equal
 */
int furi_string_cmp(const FuriString* string_1, const FuriString* string_2);

/** Compare string with C string and return the sort order.
 *
 * @param      string_1   The string 1
 * @param      cstring_2  The cstring 2
 *
 * @return     zero if equal
 */
int furi_string_cmp_str(const FuriString* string_1, const char cstring_2[]);

/** Compare two strings (case insensitive according to the current locale) and
 * return the sort order.
 *
 * Note: doesn't work with UTF-8 strings.
 *
 * @param      string_1  The string 1
 * @param      string_2  The string 2
 *
 * @return     zero if equal
 */
int furi_string_cmpi(const FuriString* string_1, const FuriString* string_2);

/** Compare string with C string (case insensitive according to the current
 * locale) and return the sort order.
 *
 * Note: doesn't work with UTF-8 strings.
 *
 * @param      string_1   The string 1
 * @param      cstring_2  The cstring 2
 *
 * @return     zero if equal
 */
int furi_string_cmpi_str(const FuriString* string_1, const char cstring_2[]);

//---------------------------------------------------------------------------
//                                 Search
//---------------------------------------------------------------------------

/** Search the first occurrence of the needle in the string from the position
 * start.
 *
 * @param      string  The FuriString instance
 * @param      needle  The needle
 * @param      start   The start (By default, start is zero)
 *
 * @return     position or FURI_STRING_FAILURE if not found
 */
size_t furi_string_search(const FuriString* string, const FuriString* needle, size_t start);

/** Search the first occurrence of the needle in the string from the position
 * start.
 *
 * @param      string  The FuriString instance
 * @param      needle  The needle
 * @param      start   The start (By default, start is zero)
 *
 * @return     position or FURI_STRING_FAILURE if not found
 */
size_t furi_string_search_str(const FuriString* string, const char needle[], size_t start);

/** Search for the position of the character c from the position start (include)
 * in the string.
 *
 * @param      string  The FuriString instance
 * @param      c       The character
 * @param      start   The start (By default, start is zero)
 *
 * @return     position or FURI_STRING_FAILURE if not found
 */
size_t furi_string_search_char(const FuriString* string, char c, size_t start);

/** Reverse search for the position of the character c from the position start
 * (include) in the string.
 *
 * @param      string  The FuriString instance
 * @param      c       The character
 * @param      start   The start (By default, start is zero)
 *
 * @return     position or FURI_STRING_FAILURE if not found
 */
size_t furi_string_search_rchar(const FuriString* string, char c, size_t start);

//---------------------------------------------------------------------------
//                                Equality
//---------------------------------------------------------------------------

/** Test if two strings are equal.
 *
 * @param      string_1  The string 1
 * @param      string_2  The string 2
 *
 * @return     true if equal false otherwise
 */
bool furi_string_equal(const FuriString* string_1, const FuriString* string_2);

/** Test if the string is equal to the C string.
 *
 * @param      string_1   The string 1
 * @param      cstring_2  The cstring 2
 *
 * @return     true if equal false otherwise
 */
bool furi_string_equal_str(const FuriString* string_1, const char cstring_2[]);

//---------------------------------------------------------------------------
//                                Replace
//---------------------------------------------------------------------------

/** Replace in the string the sub-string at position 'pos' for 'len' bytes into
 * the C string 'replace'.
 *
 * @param      string   The string
 * @param      pos      The position
 * @param      len      The length
 * @param      replace  The replace
 */
void furi_string_replace_at(FuriString* string, size_t pos, size_t len, const char replace[]);

/** Replace a string 'needle' to string 'replace' in a string from 'start'
 * position.
 *
 * @param      string   The string
 * @param      needle   The needle
 * @param      replace  The replace
 * @param      start    The start (By default, start is zero)
 *
 * @return     Return FURI_STRING_FAILURE if 'needle' not found or replace position.
 */
size_t
    furi_string_replace(FuriString* string, FuriString* needle, FuriString* replace, size_t start);

/** Replace a C string 'needle' to C string 'replace' in a string from 'start'
 * position.
 *
 * @param      string   The string
 * @param      needle   The needle
 * @param      replace  The replace
 * @param      start    The start (By default, start is zero)
 *
 * @return     Return FURI_STRING_FAILURE if 'needle' not found or replace position.
 */
size_t furi_string_replace_str(
    FuriString* string,
    const char needle[],
    const char replace[],
    size_t start);

/** Replace all occurrences of 'needle' string into 'replace' string.
 *
 * @param      string   The string
 * @param      needle   The needle
 * @param      replace  The replace
 */
void furi_string_replace_all(
    FuriString* string,
    const FuriString* needle,
    const FuriString* replace);

/** Replace all occurrences of 'needle' C string into 'replace' C string.
 *
 * @param      string   The string
 * @param      needle   The needle
 * @param      replace  The replace
 */
void furi_string_replace_all_str(FuriString* string, const char needle[], const char replace[]);

//---------------------------------------------------------------------------
//                            Start / End tests
//---------------------------------------------------------------------------

/** Test if the string starts with the given string.
 *
 * @param      string  The FuriString instance
 * @param      start   The FuriString instance 
 *
 * @return     true if string starts with
 */
bool furi_string_start_with(const FuriString* string, const FuriString* start);

/** Test if the string starts with the given C string.
 *
 * @param      string  The FuriString instance
 * @param      start   The start
 *
 * @return     true if string starts with
 */
bool furi_string_start_with_str(const FuriString* string, const char start[]);

/** Test if the string ends with the given string.
 *
 * @param      string  The FuriString instance
 * @param      end     The end
 *
 * @return     true if string ends with
 */
bool furi_string_end_with(const FuriString* string, const FuriString* end);

/** Test if the string ends with the given string (case insensitive according to the current locale).
 *
 * @param      string  The FuriString instance
 * @param      end     The end
 *
 * @return     true if string ends with
 */
bool furi_string_end_withi(const FuriString* string, const FuriString* end);

/** Test if the string ends with the given C string.
 *
 * @param      string  The FuriString instance
 * @param      end     The end
 *
 * @return     true if string ends with
 */
bool furi_string_end_with_str(const FuriString* string, const char end[]);

/** Test if the string ends with the given C string (case insensitive according to the current locale).
 *
 * @param      string  The FuriString instance
 * @param      end     The end
 *
 * @return     true if string ends with
 */
bool furi_string_end_withi_str(const FuriString* string, const char end[]);

//---------------------------------------------------------------------------
//                                Trim
//---------------------------------------------------------------------------

/** Trim the string left to the first 'index' bytes.
 *
 * @param      string  The FuriString instance
 * @param      index   The index
 */
void furi_string_left(FuriString* string, size_t index);

/** Trim the string right from the 'index' position to the last position.
 *
 * @param      string  The FuriString instance
 * @param      index   The index
 */
void furi_string_right(FuriString* string, size_t index);

/** Trim the string from position index to size bytes.
 *
 * See also furi_string_set_n.
 *
 * @param      string  The FuriString instance
 * @param      index   The index
 * @param      size    The size
 */
void furi_string_mid(FuriString* string, size_t index, size_t size);

/** Trim a string from the given set of characters (default is " \n\r\t").
 *
 * @param      string  The FuriString instance
 * @param      chars   The characters
 */
void furi_string_trim(FuriString* string, const char chars[]);

//---------------------------------------------------------------------------
//                                UTF8
//---------------------------------------------------------------------------

/** An unicode value */
typedef unsigned int FuriStringUnicodeValue;

/** Compute the length in UTF8 characters in the string.
 *
 * @param      string  The FuriString instance
 *
 * @return     strings size
 */
size_t furi_string_utf8_length(FuriString* string);

/** Push unicode into string, encoding it in UTF8.
 *
 * @param      string   The string
 * @param      unicode  The unicode
 */
void furi_string_utf8_push(FuriString* string, FuriStringUnicodeValue unicode);

/** State of the UTF8 decoding machine state */
typedef enum {
    FuriStringUTF8StateStarting,
    FuriStringUTF8StateDecoding1,
    FuriStringUTF8StateDecoding2,
    FuriStringUTF8StateDecoding3,
    FuriStringUTF8StateError
} FuriStringUTF8State;

/** Main generic UTF8 decoder
 *
 * It takes a character, and the previous state and the previous value of the
 * unicode value. It updates the state and the decoded unicode value. A decoded
 * unicode encoded value is valid only when the state is
 * FuriStringUTF8StateStarting.
 *
 * @param      c        The character
 * @param      state    The state
 * @param      unicode  The unicode
 */
void furi_string_utf8_decode(char c, FuriStringUTF8State* state, FuriStringUnicodeValue* unicode);

//---------------------------------------------------------------------------
//                Lasciate ogne speranza, voi ch’entrate
//---------------------------------------------------------------------------

/**
 *
 * Select either the string function or the str function depending on
 * the b operand to the function.
 * func1 is the string function / func2 is the str function.
 */

/** Select for 1 argument */
#define FURI_STRING_SELECT1(func1, func2, a)                                                       \
    _Generic((a), char*: func2, const char*: func2, FuriString*: func1, const FuriString*: func1)( \
        a)

/** Select for 2 arguments */
#define FURI_STRING_SELECT2(func1, func2, a, b)                                                    \
    _Generic((b), char*: func2, const char*: func2, FuriString*: func1, const FuriString*: func1)( \
        a, b)

/** Select for 3 arguments */
#define FURI_STRING_SELECT3(func1, func2, a, b, c)                                                 \
    _Generic((b), char*: func2, const char*: func2, FuriString*: func1, const FuriString*: func1)( \
        a, b, c)

/** Select for 4 arguments */
#define FURI_STRING_SELECT4(func1, func2, a, b, c, d)                                              \
    _Generic((b), char*: func2, const char*: func2, FuriString*: func1, const FuriString*: func1)( \
        a, b, c, d)

/** Allocate new FuriString and set it content to string (or C string).
 *
 * ([c]string)
 */
#define furi_string_alloc_set(a) \
    FURI_STRING_SELECT1(furi_string_alloc_set, furi_string_alloc_set_str, a)

/** Set the string content to string (or C string).
 *
 * (string, [c]string)
 */
#define furi_string_set(a, b) FURI_STRING_SELECT2(furi_string_set, furi_string_set_str, a, b)

/** Compare string with string (or C string) and return the sort order.
 *
 * Note: doesn't work with UTF-8 strings.
 * (string, [c]string)
 */
#define furi_string_cmp(a, b) FURI_STRING_SELECT2(furi_string_cmp, furi_string_cmp_str, a, b)

/** Compare string with string (or C string) (case insensitive according to the current locale) and return the sort order.
 *
 * Note: doesn't work with UTF-8 strings.
 * (string, [c]string)
 */
#define furi_string_cmpi(a, b) FURI_STRING_SELECT2(furi_string_cmpi, furi_string_cmpi_str, a, b)

/** Test if the string is equal to the string (or C string).
 *
 * (string, [c]string)
 */
#define furi_string_equal(a, b) FURI_STRING_SELECT2(furi_string_equal, furi_string_equal_str, a, b)

/** Replace all occurrences of string into string (or C string to another C string) in a string.
 *
 * (string, [c]string, [c]string)
 */
#define furi_string_replace_all(a, b, c) \
    FURI_STRING_SELECT3(furi_string_replace_all, furi_string_replace_all_str, a, b, c)

/** Search for a string (or C string) in a string
 *
 * (string, [c]string[, start=0])
 */
#define furi_string_search(...) \
    M_APPLY(                    \
        FURI_STRING_SELECT3,    \
        furi_string_search,     \
        furi_string_search_str, \
        M_DEFAULT_ARGS(3, (0), __VA_ARGS__))
/** Search for a C string in a string
 *
 * (string, cstring[, start=0])
 */
#define furi_string_search_str(...) furi_string_search_str(M_DEFAULT_ARGS(3, (0), __VA_ARGS__))

/** Test if the string starts with the given string (or C string).
 *
 * (string, [c]string)
 */
#define furi_string_start_with(a, b) \
    FURI_STRING_SELECT2(furi_string_start_with, furi_string_start_with_str, a, b)

/** Test if the string ends with the given string (or C string).
 *
 * (string, [c]string)
 */
#define furi_string_end_with(a, b) \
    FURI_STRING_SELECT2(furi_string_end_with, furi_string_end_with_str, a, b)

/** Test if the string ends with the given string (or C string) (case insensitive according to the current locale).
 *
 * (string, [c]string)
 */
#define furi_string_end_withi(a, b) \
    FURI_STRING_SELECT2(furi_string_end_withi, furi_string_end_withi_str, a, b)

/** Append a string (or C string) to the string.
 *
 * (string, [c]string)
 */
#define furi_string_cat(a, b) FURI_STRING_SELECT2(furi_string_cat, furi_string_cat_str, a, b)

/** Trim a string from the given set of characters (default is " \n\r\t").
 *
 * (string[, set=" \n\r\t"])
 */
#define furi_string_trim(...) furi_string_trim(M_DEFAULT_ARGS(2, ("  \n\r\t"), __VA_ARGS__))

/** Search for a character in a string.
 *
 * (string, character[, start=0])
 */
#define furi_string_search_char(...) furi_string_search_char(M_DEFAULT_ARGS(3, (0), __VA_ARGS__))

/** Reverse Search for a character in a string.
 *
 * (string, character[, start=0])
 */
#define furi_string_search_rchar(...) furi_string_search_rchar(M_DEFAULT_ARGS(3, (0), __VA_ARGS__))

/** Replace a string to another string (or C string to another C string) in a string.
 *
 * (string, [c]string, [c]string[, start=0])
 */
#define furi_string_replace(...) \
    M_APPLY(                     \
        FURI_STRING_SELECT4,     \
        furi_string_replace,     \
        furi_string_replace_str, \
        M_DEFAULT_ARGS(4, (0), __VA_ARGS__))

/** Replace a C string to another C string in a string.
 *
 * (string, cstring, cstring[, start=0])
 */
#define furi_string_replace_str(...) furi_string_replace_str(M_DEFAULT_ARGS(4, (0), __VA_ARGS__))

/** INIT OPLIST for FuriString */
#define F_STR_INIT(a) ((a) = furi_string_alloc())

/** INIT SET OPLIST for FuriString */
#define F_STR_INIT_SET(a, b) ((a) = furi_string_alloc_set(b))

/** INIT MOVE OPLIST for FuriString */
#define F_STR_INIT_MOVE(a, b) ((a) = furi_string_alloc_move(b))

/** OPLIST for FuriString */
#define FURI_STRING_OPLIST       \
    (INIT(F_STR_INIT),           \
     INIT_SET(F_STR_INIT_SET),   \
     SET(furi_string_set),       \
     INIT_MOVE(F_STR_INIT_MOVE), \
     MOVE(furi_string_move),     \
     SWAP(furi_string_swap),     \
     RESET(furi_string_reset),   \
     EMPTY_P(furi_string_empty), \
     CLEAR(furi_string_free),    \
     HASH(furi_string_hash),     \
     EQUAL(furi_string_equal),   \
     CMP(furi_string_cmp),       \
     TYPE(FuriString*))

#ifdef __cplusplus
}
#endif
