#include "strlib.h"


hash_t hash_string(const char* s) {
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


int32_t int_str_len(int64_t i, int64_t base) {
  if (!i) {
    return 1;
  }

  int32_t out = 1;
  
  if (i < 1) {
    out += 1;  // add 1 if the sign is negative
    i *= -1;
  }

  while (i) {
    i /= base;
    out += 1;
  }

  return out;
}

char dtoc(int32_t i, int32_t b) {
  switch (b) {
  case 2:
    switch(i) {
    case 0 ... 1:
      return 48 + i;
    default: return '\0';
    }
  case 8:
    switch(i) {
    case 0 ... 7: return 48 + i;
    default: return '\0';
    }
  case 10:
    switch (i) {
    case 0 ... 9: return 48 + i;
    default: return '\0';
    }
  case 16:
    switch (i) {
    case 0 ... 9: return 48 + i;
    case 10 ... 15: return 55 + i;
    default: return '\0';
    }
  default: return '\0';
  }
}

char* itoa(int32_t i, int32_t b) {
  char buffer[35];
  char reverse_buffer[35];
  int b_start = 0, r_idx = 0;
  
  if (i == 2) {
    buffer[0] = '0';
    buffer[1] = 'b';
    b_start = 2;
  } else if (b == 8) {
    buffer[0] = '0';
    buffer[1] = 'o';
    b_start = 2;
  } else if (b == 16) {
    buffer[0] = '0';
    buffer[1] = 'x';
    b_start = 2;
  }

  while (i) {
    reverse_buffer[r_idx] = dtoc(i % b, b);
    r_idx++;
    i /= b;
  }

  int j = b_start;
  for (; j < r_idx + b_start; j++) {
    buffer[j] = reverse_buffer[j-b_start];
  }

  buffer[j] = '\0';

  char* out = malloc(strlen(buffer) + 1);
  strcpy(out,buffer);

  return out;
}
