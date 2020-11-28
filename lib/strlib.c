#include "strlib.h"


hash_t hash_string(char* s) {
  hash_t out = FNV1A_OFFSET_BASIS;
  int limit = strlen(s);

  for (int i = 0; i < limit; i++) {
    out ^= s[i];
    out *= FNV1A_PRIME;
  }

  return out;
}


int32_t ceil_mod_eight(int32_t n, int32_t u) {
  div_t divr = div(n, 8);

  if (divr.rem) return (divr.quot + 1) * u; 

  return divr.quot * u;
}



int32_t get_aligned_size(char* s) {
  int nbytes = strlen(s) + 1;

  return ceil_mod_eight(nbytes, 8);
}
