#ifndef atypes_h

/* begin atypes.h */
#define atypes_h

/* 

   rationalized definitions for arithmetic types 

*/


/*
  stdint.h provides:
  
  int8_t
  int16_t
  int32_t
  int64_t
  uint8_t
  uint16_t
  uint32_t
  uint64_t
  intptr_t
  uintptr_t

  stddef.h provides:

  ptrdiff_t
  size_t       (we just use uint32_t)

 */

#include <stdint.h>
#include <stddef.h>


typedef int8_t     i8_t;
typedef uint8_t    u8_t;
typedef int16_t   i16_t;
typedef uint16_t  u16_t;
typedef int32_t   i32_t;
typedef uint32_t  u32_t;
typedef int64_t   i64_t;
typedef uint64_t  u64_t;
typedef intptr_t  iptr_t;
typedef uintptr_t uptr_t;
typedef ptrdiff_t pdiff_t;

/*

  character type definitions

  uchar.h provides
  char16_t
  char32_t

 */

#include <uchar.h>
#include <wctype.h>

typedef char               c8_t;
typedef signed char       sc8_t;
typedef unsigned char     uc8_t;
typedef char16_t          c16_t;
typedef char32_t          c32_t;
typedef wchar_t           ucp_t;
typedef wint_t           ucpi_t;

/* floating point type definitions are pretty simple */

typedef float  f32_t;
typedef double f64_t;

/* end atypes.h */
#endif
