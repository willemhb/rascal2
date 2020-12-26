#ifndef strutil_h

/* begin strutil.h */
#define strutil_h

/* 

   This library provides:

   * general string utilities
   * hashing utilities
   * a complete set of utf-8 utilities

 */

#include <stdlib.h>
#include <string.h>
#include <uchar.h>
#include <wctype.h>
#include <locale.h>

typedef char32_t        utf32_t;
typedef unsigned       hash32_t;     // a 32-bit hash
typedef unsigned long  hash64_t;     // a 64-bit hash

typedef char utf8_t[4]; // a utf-8 encoded byte sequence

// hashing constants
#define FNV_1A_32_PRIME   0x01000193
#define FNV_1A_32_OFFSET  0x811c9dc5
#define FNV_1A_64_PRIME   0x00000100000001b3
#define FNV_1A_64_OFFSET  0xcbf29ce484222325
// putting 128-bit constants here for possible future use
#define FNV_1A_128_PRIME  0x0000000001000000000000000000013b
#define FNV_1A_128_OFFSET 0x6c62272e07bb014262b821756295c58d

// unicode constants
#define UNICODE_STANDARD 13.0
#define MAX_CODEPOINT 0x10fffff
#define MIN_CODEPOINT 0
#define NUM_BLOCKS 308
#define UEOF WEOF

enum {
  /* masks for testing the first byte in a UTF-8 sequence */
  U8_BMASK_1  = 0b10000000,
  U8_BMASK_2  = 0b11100000,
  U8_BMASK_3  = 0b11110000,
  U8_BMASK_4  = 0b11111000,
  /* bytemarks for testing the length of a UTF-8 sequence */
  U8_BMARK_1  = 0b00000000,
  U8_BMARK_2  = 0b11000000,
  U8_BMARK_3  = 0b11100000,
  U8_BMARK_4  = 0b11110000,
  /* limits for 1, 2, 3, and 4-byte code points (the value is 1-greater than the maximum value) */
  UCP_LIMIT_1 = 0x00000080,
  UCP_LIMIT_2 = 0x00000800,
  UCP_LIMIT_3 = 0x00010000,
  UCP_LIMIT_4 = 0x00110000,
};

int ensure_utf8();          // set the locale to UTF-8
int strsz(const char*);     // simple helper that gives the number of bytes in a string (avoids forgetting about the terminal \0
hash32_t hash32(const char*);
hash64_t hash64(const char*);

int ulen_cp(utf32_t);
int ulen_s(const char*);

#define ulen(c)               \
  _Generic((c),               \
	   utf32_t:ulen_cp,   \
           char*:ulen_s,      \
           utf8_t:ulen_s)(c)

int u32tou8(char*,utf32_t);
int u8tou32(const char*);

// utilities for working with utf8 strings
int nextu32(char**,utf32_t*);
int ustrref(const char*,size_t);  // get the nth character
int umemref(const char*,size_t);  // get the nth byte

// utf-8 string utilities
int   ustrlen(const char*);  // get the number of characters
int   ustrsz(const char*);   // get the number of bytes
int   ustrcmp(const char*, const char*);
int   ustrncmp(const char*, const char*, size_t);
char* ustrcat(char*, const char*);
char* ustrncat(char*, const char*, size_t);
char* ustrmcat(char*, const char*, size_t);
char* ustrdup(const char*);
char* ustrndup(const char*);
int   ustrchr(const char*, utf32_t);
int   ustrrchr(const char*, utf32_t);
int   ustrspn(const char*, const char*);
int   ustrcspn(const char*, const char*);
int   ustrpbrk(const char*, const char*);
int   ustrstr(const char*, const char*);

// numeric conversions
int   ustrtoi(const char*);
float ustrtof(const char*);

// character tests based on the functions in wctype.h
int isualpha(utf32_t);
int isualnum(utf32_t);
int isublank(utf32_t);
int isucntrl(utf32_t);
int isudigit(utf32_t);
int isugraph(utf32_t);
int isulower(utf32_t);
int isuprint(utf32_t);
int isupunct(utf32_t);
int isuspace(utf32_t);
int isuupper(utf32_t);
int isuxdigit(utf32_t);

// character translations
int toulower(utf32_t);
int touupper(utf32_t);

/* end strutil.h */
#endif
