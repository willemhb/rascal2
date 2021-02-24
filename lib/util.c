#include "../include/rascal.h"

/* utility functions */

// valid numeric patterns
#define BIN_NUM_RE "[+-]?0[bB][01]+"
#define OCT_NUM_RE "[+-]?0[oO][0-7]+"
#define DEC_NUM_RE "[+-]?[[:digit:]]+"
#define HEX_NUM_RE "[+-]?0[xX][[:xdigit:]]+"

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

inline const chr_t* rsp_efmt(rsp_err_t eno) { return ERROR_FMTS[eno]; }
inline const chr_t* rsp_errname(rsp_err_t eno) { return ERROR_NAMES[eno]; }

void rsp_vperror(const chr_t* fl, int32_t ln, const chr_t* fnc, rsp_err_t eno, ...)
{
  fprintf(stderr,ERRINFO_FMT,fl,ln,fnc,rsp_errname(eno));
  const chr_t* fmt = rsp_efmt(eno);
  va_list args;

   va_start(args, eno);
   vfprintf(stderr, fmt, args);
   va_end(args);

   fputc('\n',stderr);

   return;
}

void rsp_raise(rsp_err_t errno) {
  if (exc_stack == NULL)
     longjmp(exc_stack->buf, errno);

  fprintf(stderr, "Exiting due to unhandled %s error.\n", rsp_errname(errno));
  exit(EXIT_FAILURE);
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
