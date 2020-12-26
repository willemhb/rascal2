#include "strutil.h"

// 

hash32_t hash32(const char* s) {
  hash32_t out = FNV_1A_32_OFFSET;
  int limit = strlen(s);

  for (int i = 0; i < limit; i++) {
    out ^= s[i];
    out *= FNV_1A_32_PRIME;
  }

  return out;
}


hash64_t hash64(const char* s) {
  hash64_t out = FNV_1A_64_OFFSET;
  int limit = strlen(s);

  for (int i = 0; i < limit; i++) {
    out ^= s[i];
    out *= FNV_1A_64_PRIME;
  }

  return out;
}
