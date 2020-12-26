#ifndef numutil_h

/* begin numutil.h */
#define numutil_h


#include <stdint.h>
#include <stddef.h>
#include <tgmath.h>

/*

  Store floating point numbers as integer bit patterns and extract the parts of the floating point number.

 */

typedef union {
  float f;
  uint32_t i;
  struct {
    uint32_t sign     :  1;
    uint32_t exponent :  8;
    uint32_t mantissa : 23;
  } b;
} fl32_bits_t;

typedef union {
  double f;
  uint64_t i;
  struct {
    uint64_t sign     :  1;
    uint64_t exponent : 11;
    uint64_t mantissa : 52;
  } b;
} dbl64_bits_t;

fl32_bits_t fl_bits_u32(uint32_t);
fl32_bits_t fl_bits_f32(float);
dbl64_bits_t fl_bits_u64(uint64_t);
dbl64_bits_t fl_bits_f64(double);

#define fl_bits(x)             \
  _Generic((x),                \
    uint32_t:fl_bits_u32,      \
    float:fl_bits_f32,         \
    uint64_t:fl_bits_u64,      \
    double:fl_bits_f64)(x)
    

// type generic max and min macros
#define max(x,y)             \
  ({                         \
    typeof(x) _X_ = x     ;  \
    typeof(y) _Y_ = y     ;  \
    _X_ < _Y_ ? _Y_ : _X_ ;  \
   })

#define min(x,y)             \
  ({                         \
    typeof(x) _X_ = x     ;  \
    typeof(y) _Y_ = y     ;  \
    _X_ > _Y_ ? _Y_ : _X_ ;  \
   })

/*

  helpers for numeric representation

  The helpers below determine the minimum buffer size required to represent
  a numeric value. Used to help with numeric IO.
 */

uint32_t frepr_sz(double,int32_t,int32_t);
uint32_t irepr_sz(int64_t,int32_t);
uint32_t urepr_sz(uint64_t,int32_t);

#define nrepr_sz(n)            \
  _Generic((n),                \
    uint64_t  :urepr_sz,       \
    uint32_t  :urepr_sz,       \
    uint16_t  :urepr_sz,       \
    uint8_t   :urepr_sz,       \
    default:irepr_sz)(n)

/* end numutil.h */
#endif
