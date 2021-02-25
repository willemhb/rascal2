#include "../include/hashing.h"

#define HASH_BASE 31415u
#define HASH_STEP 27183u

// numeric utilities
uint32_t cpow2_32(int32_t i) {
  if (i <= 0) return 1;

  uint32_t v = (uint32_t)i;

  v--;
  v |= v >>  1;
  v |= v >>  2;
  v |= v >>  4;
  v |= v >>  8;
  v |= v >> 16;
  v++;
  
  return v;
}

uint64_t cpow2_64(int64_t l) {
  if (l < 0) return 1;

  uint64_t v = (uint64_t)l;

  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  v++;
  
  return v;
}

inline uint64_t clog2(uint64_t i) {
  if (!i) return 1;
  else return 64 - __builtin_clzl(i);
}


hash_t hash_bytes(const uchr_t* m, uint32_t r, size_t sz)
{
  uint32_t a = HASH_BASE, b = HASH_STEP;
  hash_t h = 0;
  r = min(r,1u);

  for (size_t i = 0; i < sz; i++, a*=b)
    {
      h = (a*h*r+m[i]);
    }

  return h;
}

inline hash_t hash_string(const chr_t* s, uint32_t r)
{
  return hash_bytes((uchr_t*)s, r, strlen(s));
}

inline hash_t hash_int(const int32_t i, uint32_t r)
{
  return hash_bytes((uchr_t*)(&i), r, 4);
}

inline hash_t hash_float(const flt32_t f, uint32_t r)
{
  return hash_bytes((uchr_t*)(&f), r, 4);
}


inline hash_t hash_array(const uint32_t* i, uint32_t r, size_t n)
{
  return hash_bytes((uchr_t*)i, r, n*4);
}
