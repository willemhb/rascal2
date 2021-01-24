#include "rascal.h"

/* utility functions */

// hashing constants
#define FNV_1A_32_PRIME   0x01000193u
#define FNV_1A_32_OFFSET  0x811c9dc5u
#define FNV_1A_64_PRIME   0x00000100000001b3ul
#define FNV_1A_64_OFFSET  0xcbf29ce484222325ul

// valid numeric patterns
#define BIN_NUM_RE "[+-]?0[bB][01]+"
#define OCT_NUM_RE "[+-]?0[oO][0-7]+"
#define DEC_NUM_RE "[+-]?[[:digit:]]+"
#define HEX_NUM_RE "[+-]?0[xX][[:xdigit:]]+"

// stack manipulation functions
inline val_t push(val_t** stk,val_t* sp, val_t* stksz, val_t v)
{
  *stk[(*sp)++] = v;
  if (unlikely(*sp == *stksz))
    {
      *stksz *= 2;
      vm_crealloc((uchr8_t**)stk,(*stksz)*8,false);
    }

  return *sp;
}

// reserve a portion of the stack in advance
val_t stk_reserve(val_t** stk, val_t* sp, val_t* stksz, size_t n)
{
  if (*stksz <= (*sp + n))
    {
      *stksz *= 2;
      vm_crealloc((uchr8_t**)stk,(*stksz)*8,false);
    }

  val_t out = *sp + 1;
  *sp += n;
  return out;
}

val_t  pushn(val_t** stk, val_t* sp, val_t* stksz, size_t n, ...)
{
  if (*stksz <= (*sp + n))
    {
      *stksz *= 2;
      vm_crealloc((uchr8_t**)stk,(*stksz)*8,false);
    }

  val_t base = *sp + 1;
  val_t top = *sp + n;
  va_list args;
  va_start(args,n);

  for (val_t i = base; i <= top; i++) *stk[i] = va_arg(args,val_t);

  *sp = top;
  va_end(args);

  return base;
}

inline val_t pop(val_t* stk, val_t* sp)
{
  if (unlikely(!(*sp))) rsp_raise(BOUNDS_ERR);
  return stk[--(*sp)];
}

val_t  popn(val_t* stk, val_t* sp, size_t n)
{
  if (unlikely(n > (*sp))) rsp_raise(BOUNDS_ERR);
  *sp -= n;
  return stk[*sp + 1];
}

// numeric utilities
unsigned cpow2_32(int i) {
  if (i < 0) return 1;

  unsigned v = (unsigned)i;

  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  
  return v;
}

unsigned long cpow2_64(long l) {
  if (l < 0) return 1;

  unsigned long v = (unsigned long)l;

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

inline unsigned long clog2(unsigned long i) {
  if (!i) return 1;
  else return 64 - __builtin_clzl(i);
}

hash32_t hash_str(const char* s) {
  hash32_t out = FNV_1A_32_OFFSET;
  int limit = strlen(s);

  for (int i = 0; i < limit; i++) {
    out ^= s[i];
    out *= FNV_1A_32_PRIME;
  }

  return out;
}

inline int strsz(const char* s) {
  if (!s)
    return 0;

  else
    return strlen(s) + 1;
}

inline int u8len(const char* s) {
  return mblen(s,4);
}

inline wint_t nextu8(const char* s) {
  wchar_t b;
  if (mbtowc(&b,s,4) == -1) return WEOF;
  return b;
}


inline int incu8(wchar_t*  b, const char* s) {
  return mbtowc(b,s,4);
}


inline wint_t nthu8(const char* s, size_t n) {
  int inc;
  for (size_t i = 0; i < n; i++) {
    if ((inc = u8len(s)) == -1) return WEOF;
    s += inc;
  }
  return nextu8(s);
}

int u8strlen(const char* s) {
  int next_cp;
  int count = 0;
  while ((next_cp = u8len(s)) != 0) {
    if (next_cp == -1) return -1;
    count++;
    s++;
  }

  return count;
}

int u8strcmp(const char* sx, const char* sy) {
  wchar_t wcx, wcy;
  int32_t xul, yul;

  while ((*sx != '\0') && (*sy != '\0')) {
    xul = incu8(&wcx,sx);
    yul = incu8(&wcy,sy);
    if (xul == -1) return -2;
    if (yul == -1) return 2;
    if (xul < yul) return -1;
    if (xul > yul) return 1;
    sx += xul;
    sy += yul;
  }

  if (*sx == '\0') return -1;
  else if (*sy == '\0') return 1;
  else return 0;
}

int peekwc(FILE* f) {
  wint_t wc = fgetwc(f);
  if (wc != WEOF) ungetwc(wc,f);
  return wc;
}

int fskip(FILE* f) {
  wint_t wc;
  while ((wc = fgetwc(f)) != WEOF && iswspace(wc)) continue;
  if (wc == WEOF) return WEOF;

  ungetwc(wc,f);
  return wc;
}

int fskipto(FILE* f, wint_t s)
{
  wint_t wc;
  while ((wc = fgetwc(f)) != WEOF && wc != s) continue;
  if (wc == WEOF) return WEOF;
  else return peekwc(f); // peek the next character after the sentinel
}

inline int iswbdigit(wint_t c) {
  return c == U'0' || c == U'1';
}

inline int iswodigit(wint_t c) {
  switch (c) {
  case U'0' ... U'7':
    return true;
  default:
    return false;
  }
}

/* error handling */

inline const char* rsp_errname(rsp_err_t eno)
{
  return ERROR_NAMES[eno];
}

void rsp_vperror(const chr8_t* fl, int32_t ln, const chr8_t* fnc, rsp_err_t eno, const chr8_t* fmt, ...)
{
  fprintf(stderr,ERRINFO_FMT, fl, ln, fnc, rsp_errname(eno));
  va_list args;

   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
   va_end(args);

   fputc('\n',stderr);

   return;
}

void rsp_raise(rsp_err_t errno) {
  if (exc_stack == NULL) {
    fprintf(stderr, "Exiting due to unhandled %s error.\n", rsp_errname(errno));
    exit(EXIT_FAILURE);
  } else {
    longjmp(exc_stack->buf, errno);
  }
}

void rsp_type_err(const char* fl, int ln, const char* fnc, const char* exp, val_t got) {
  fprintf(stderr, "[%s:%i:%s:] type error: expected type %s, got %s", fl,ln,fnc,exp,val_tpname(got));
  rsp_raise(TYPE_ERR);
}



/* inlined bindings for C arithmetic, bitwise, and comparison functions */

// arithmetic
inline int    add_ii  (int x,   int y)      { return x + y; }
inline int    sub_ii  (int x,   int y)      { return x - y; }
inline int    mul_ii  (int x,   int y)      { return x * y; }
inline int    div_ii  (int x,   int y)      { return x / y; }
inline int    rem_ii  (int x,   int y)      { return x % y; }
inline int    neg_i   (int x)               { return -x ; }
inline float  add_ff  (float x, float y)    { return x + y; }
inline float  sub_ff  (float x, float y)    { return x - y; }
inline float  mul_ff  (float x, float y)    { return x * y; }
inline float  div_ff  (float x, float y)    { return x / y; }
inline float  neg_f   (float x)             { return -x ; }
inline long   add_ll  (long x,  long y)     { return x + y; }
inline long   sub_ll  (long x,  long y)     { return x - y; }
inline long   mul_ll  (long x,  long y)     { return x * y; }
inline long   div_ll  (long x,  long y)     { return x / y; }
inline long   rem_ll  (long x,  long y)     { return x % y; }
inline long   neg_l   (long x)              { return -x ; }
inline double add_dd  (double x, double y)  { return x + y; }
inline double sub_dd  (double x, double y)  { return x - y; }
inline double mul_dd  (double x, double y)  { return x * y; }
inline double div_dd  (double x, double y)  { return x / y; }
inline double neg_d   (double x)            { return -x ; }

// comparison
inline bool eql_ii    (int x, int y)         { return x == y; }
inline bool neql_ii   (int x, int y)         { return x != y; }
inline bool gt_ii     (int x, int y)         { return x > y; }
inline bool ge_ii     (int x, int y)         { return x >= y; }
inline bool lt_ii     (int x, int y)         { return x < y; }
inline bool le_ii     (int x, int y)         { return x <= y; }
inline bool eql_ff    (float x, float y)     { return x == y; }
inline bool neql_ff   (float x, float y)     { return x != y; }
inline bool gt_ff     (float x, float y)     { return x > y; }
inline bool ge_ff     (float x, float y)     { return x >= y; }
inline bool lt_ff     (float x, float y)     { return x < y; }
inline bool le_ff     (float x, float y)     { return x <= y; }
inline bool eql_ll    (long x, long y)       { return x == y; }
inline bool neql_ll   (long x, long y)       { return x != y; }
inline bool gt_ll     (long x, long y)       { return x > y; }
inline bool ge_ll     (long x, long y)       { return x >= y; }
inline bool lt_ll     (long x, long y)       { return x < y; }
inline bool le_ll     (long x, long y)       { return x <= y; }
inline bool eql_dd    (double x, double y)   { return x == y; }
inline bool neql_dd   (double x, double y)   { return x != y; }
inline bool gt_dd     (double x, double y)   { return x > y; }
inline bool ge_dd     (double x, double y)   { return x >= y; }
inline bool lt_dd     (double x, double y)   { return x < y; }
inline bool le_dd     (double x, double y)   { return x <= y; }

// bitwise
inline int or_ii (int x, int y)              { return x | y; }
inline int xor_ii(int x, int y)              { return x ^ y; }
inline int and_ii(int x, int y)              { return x & y; }
inline int lsh_ii(int x, int y)              { return x >> y; }
inline int rsh_ii(int x, int y)              { return x << y;}
inline int bneg_i(int x)                     { return ~x; }
inline long or_ll (long x, long y)           { return x | y; }
inline long xor_ll(long x, long y)           { return x ^ y; }
inline long and_ll(long x, long y)           { return x & y; }
inline long lsh_ll(long x, long y)           { return x >> y; }
inline long rsh_ll(long x, long y)           { return x << y;}
inline long bneg_l(long x)                   { return ~x; }
