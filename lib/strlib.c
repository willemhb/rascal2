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



/* helpers for working with utf-8 strings */

// get the nth unicode code from a multibyte character string
// return -1 if the character string doesn't have n characters
wint_t nth_wc(char* s, size_t n) {
  wchar_t ch;
  size_t nb = 0; // the current index in s, measured in bytes  

  for (size_t i = 0; i < n; i++) {
    if (s[nb] == '\0') return -1;

    nb += mbtowc(&ch,s + nb, 4);
  }

  return ch;
}

int find_wc(wchar_t cp, char* s) {
  size_t idx = 0;
  wchar_t cpb;

  while (*s != '\0') {
    s += mbtowc(&cpb,s,4);
    if (cpb == cp) {
      return idx;
    }

    idx++;
  }

  return -1;
}

// replace the nth character in the multibyte string old with new,
// writing the result to new
int init_setcp_cpy(char* old, char* new, size_t i, wchar_t cp, int max) {
  int total = 0;

  for (size_t j = 0; j < i; j++) {
    if (total >= max) break;
    int nbytes = mblen(old,4);
    memcpy(new,old,nbytes);
    old += nbytes;
    new += nbytes;
    total += nbytes;
  }

  if (total + mblen(new,4) >= max || total + mblen(old,4) >= max) {
    return -1;
  }
  
  new += wctomb(new,cp);
  old += mblen(old,4);
  strncpy(old,new,max-total+mblen(old,4));
  return 0;
}

size_t nchars(char* s) {
  size_t count = 0;
  while (*s != '\0') {
    s += mblen(s,4);
    count++;
  }

  return count;
}

// determine the length of a multibyte utf-8 sequence given the value of the first byte
int testuc(unsigned char ch) {
  if ((ch & 0x80) == 0) return 1;
  else if ((ch & 0xE0) == 0xC0) return 2;
  else if ((ch & 0xF0) == 0xE0) return 3;
  else if ((ch & 0xF8) == 0xF0) return 4;
  else return -1;
}

// fetch the next unicode code point from the given file stream
wint_t fgetuc(FILE* f) {
  wchar_t cpb;
  char buffer[5] = { 0, 0, 0, 0, 0 };

  if (feof(f)) {
    return EOF;
  }

  buffer[0] = fgetc(f);
  int nc = testuc(buffer[0]);

  if (nc == -1) {
    return -1;
  }

  for (int i = 1; i < nc; i++) {
    if (feof(f)) return EOF;
    buffer[i] = fgetc(f);
  }

  mbtowc(&cpb,buffer,4);

  return cpb;
}

int fputuc(wchar_t ch, FILE* f) {
  char buffer[5] = {0, 0, 0, 0, 0};
  wctomb(buffer,ch);
  return fputs(buffer,f);
}

int fungetuc(wchar_t ch, FILE* f) {
  char buffer[5] = {0, 0, 0, 0, 0};
  int nbytes = wctomb(buffer,ch);
  long pos = ftell(f);

  if (nbytes > pos) return -1;
  else fseek(f,pos - nbytes,SEEK_SET);
  return nbytes;
}

wint_t peekuc(FILE* f) {
  wchar_t cpb;
  char buffer[5] = { 0, 0, 0, 0, 0 };
  long pos = ftell(f);

  if (feof(f)) {
    return EOF;
  }

  buffer[0] = fgetc(f);
  int nb = testuc(buffer[0]);

  if (nb == -1) {
    return -1;
  }

  for (int i = 1; i < nb; i++) {
    if (feof(f)) return EOF;
    buffer[i] = fgetc(f);
  }

  mbtowc(&cpb,buffer,4);

  fseek(f, pos, SEEK_SET);

  return cpb;
}
