#ifndef common_h

/* begin common.h */
#define common_h

/* 
   this file includes common headers, and a few redefinitions of builtin types.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include <errno.h>

// unicode constants
#define MIN_CODEPOINT 0
#define MAX_CODEPOINT 0x10FFFF
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

// rationalized, explicitly sized types
typedef uintptr_t uptr_t;
typedef uint64_t hash64_t;    // a 64-bit hash
typedef uint32_t hash32_t;    // a 32-bit hash
typedef char chr8_t;
typedef unsigned char uchr8_t;
typedef wchar_t chr32_t;
typedef wint_t ichr32_t;
typedef float flt32_t;
typedef double flt64_t;


typedef struct _rsp_ectx_t {
  jmp_buf buf;
  struct _rsp_ectx_t* prev;
  // all the shared global state that might have changed
  uchr8_t* free_state;
  uptr_t   dp_state;
  uptr_t rx_state[16];
} rsp_ectx_t;


/* end common.h */
#endif
