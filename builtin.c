#include "capi.h"

char* strval(val_t s, const char* fl, int ln, const char* fn)
{
  if (isstr(s)) return tostr_(s);
  else if (issym(s)) return symname_(s);
  else
    {
      _rsp_perror(fl,ln,fn,TYPE_ERR,"Non-string type.");
      rsp_raise(TYPE_ERR);
      return 0;
    }
}

float floatval(val_t x)
{
  if (isint(x))
    {
      return (float)toint_(x);
    }
  else return tofloat(x);
}

int intval(val_t x)
{
  if (isfloat(x))
    {
      return (int)tofloat_(x);
    }
  else return toint(x);
}

#define DECLARE_ARITHMETIC_2(fname,funci,funcf,rtni,rtnf)	\
  val_t bltn_## fname(val_t* a)                                 \
  {                                                             \
    if (isfloat(a[1]) || isfloat(a[0]))                         \
      {                                                         \
	return rtnf(funcf(floatval(a[0]),floatval(a[1])));	\
      }					                        \
    else                                                        \
    {                                                           \
      return rtni(funci(intval(a[0]),intval(a[1])));		\
    }                                                           \
  }


#define DECLARE_ARITHMETIC_1(fname,funci,funcf,rtni,rtnf)       \
  val_t bltn_## fname(val_t* a)					\
    {                                                           \
    if (isfloat(a[0]))                                          \
      {                                                         \
	return rtnf(funcf(tofloat_(a[0])));			\
      }					                        \
    else                                                        \
    {                                                           \
      return rtni(funci(toint(a[0])));				\
    }                                                           \
  }


#define DECLARE_BITWISE_2(fname,fnc)				\
  val_t bltn_## fname(val_t* a)                                 \
  {                                                             \
      return mk_int(fnc(toint(a[1]),toint(a[0])));		\
  }

#define DECLARE_BITWISE_1(fname,fnc)				\
  val_t bltn_## fname(val_t* a)                                 \
  {                                                             \
      return mk_int(fnc(toint(a[0])));		                \
  }


DECLARE_ARITHMETIC_2(add,add_ii,add_ff,mk_int,mk_float)
DECLARE_ARITHMETIC_2(sub,sub_ii,sub_ff,mk_int,mk_float)
DECLARE_ARITHMETIC_2(mul,mul_ii,mul_ff,mk_int,mk_float)
DECLARE_ARITHMETIC_2(div,div_ii,div_ff,mk_int,mk_float)
DECLARE_ARITHMETIC_2(rem,rem_ii,rem_ff,mk_int,mk_float)
DECLARE_ARITHMETIC_2(eql,eql_ii,eql_ff,mk_bool,mk_bool)
DECLARE_ARITHMETIC_2(neql,neql_ii,neql_ff,mk_bool,mk_bool)
DECLARE_ARITHMETIC_2(lt,lt_ii,lt_ff,mk_bool,mk_bool)
DECLARE_ARITHMETIC_2(le,le_ii,le_ff,mk_bool,mk_bool)
DECLARE_ARITHMETIC_2(gt,gt_ii,gt_ff,mk_bool,mk_bool)
DECLARE_ARITHMETIC_2(ge,ge_ii,ge_ff,mk_bool,mk_bool)
DECLARE_ARITHMETIC_1(neg,neg_i,neg_f,mk_int,mk_int)
DECLARE_BITWISE_2(band,and_ii)
DECLARE_BITWISE_2(bor,or_ii)
DECLARE_BITWISE_2(bxor,xor_ii)
DECLARE_BITWISE_2(lsh,lsh_ii)
DECLARE_BITWISE_2(rsh,rsh_ii)
DECLARE_BITWISE_1(bneg,bneg_i)


#define DECLARE_BUILTIN(fname,func,argc) DECLARE_BUILTIN_ ## argc ## _(fname,func)

#define DECLARE_BUILTIN_0_(fname,func)	      \
  val_t bltn_ ## fname(val_t* a)              \
  {					      \
     return func() ;	                      \
  }

#define DECLARE_BUILTIN_1_(fname,func)	      \
  val_t bltn_ ## fname ## _(val_t* a)         \
  {					      \
    return func(a[0]) ;		              \
  }

DECLARE_BUILTIN(intp,isint,1)
DECLARE_BUILTIN(floatp,isfloat,1)
DECLARE_BUILTIN(charp,ischar,1)
DECLARE_BUILTIN(consp,iscons,1)
DECLARE_BUILTIN(listp,islist,1)
DECLARE_BUILTIN(functionp,isfunction,1)
DECLARE_BUILTIN(tablep,istable,1)
DECLARE_BUILTIN(dvecp,isdvec,1)
DECLARE_BUILTIN(fvecp,isfvec,1)
DECLARE_BUILTIN(typep,istype,1)
DECLARE_BUILTIN(boolp,isbool,1)
DECLARE_BUILTIN(nilp,isnil,1)
DECLARE_BUILTIN(nonep,isnone,1)
DECLARE_BUILTIN(truep,istrue,1)
DECLARE_BUILTIN(falsep,isfalse,1)

#define DECLARE_BUILTIN_2_(fname,func)	      \
  val_t bltn_ ## fname ## _(val_t* a)         \
  {					      \
     return fname(a[0],a[1]);                 \
  }

#define DECLARE_BUILTIN_3_(fname)	      \
  val_t bltn_ ## fname ## _(val_t* a)         \
  {					      \
    return fname(a[0],a[1],a[2]);	      \
}

#define DECLARE_BUILTIN_4_(fname)	      \
  val_t bltn_ ## fname ## _(val_t* a)         \
  {					      \
    return fname(a[0],a[1],a[2],a[3]);	      \
  }

/* 

   inlined functional bindings for C arithmetic

 */

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

// the following macros provide boilerplate definitions for C api functions of up to 4 arguments
// future plans are to implement a more robust C api, but this should make it simpler to write basic C library bindings

#define DECLARE_API_0(fname,cfnc,crtp,rrtp,wrp)	\
  rrtp bltn_ ## fname ## _(val_t* a, int c)     \
  {					        \
     crtp _result = fname();		        \
     return wrp(_result);		        \
  }


#define DECLARE_VOID_API_1(fname,cfnc,xtp,cstx,rtn)	   \
  val_t bltn_ ## fname ## _(val_t* a)                      \
  {					                   \
    xtp _x = cstx(a[0],__FILE__,__LINE__,__func__) ;	   \
    cfnc(_x)                                       ;       \
    return rtn;						   \
  }

// a few simple system calls
DECLARE_VOID_API_1(exit,exit,int,_torint,R_NIL)
DECLARE_VOID_API_1(clferr,clearerr,FILE*,_tocfile,R_TRUE)


#define DECLARE_API_1(fname,cfnc,xtp,cstx,crtp,rrtp,wrp)   \
  rrtp bltn_ ## fname ## _(val_t* a)                       \
  {					                   \
    xtp _x = cstx(a[0],__FILE__,__LINE__,__func__) ;	   \
    crtp _result = cfnc(_x)                        ;       \
     return wrp(_result)                           ;	   \
  }

// character manipulation C api functions, IO, getenv
DECLARE_API_1(alphap,iswalpha,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(alnump,iswalnum,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(lowerp,iswlower,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(upperp,iswlower,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(digitp,iswdigit,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(xdigitp,iswxdigit,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(cntrlp,iswcntrl,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(graphp,iswgraph,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(spacep,iswspace,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(blankp,iswblank,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(printp,iswprint,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(punctp,iswpunct,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(upper,towupper,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(lower,towlower,wchar_t,_torchar,int,val_t,mk_bool)
DECLARE_API_1(osgetenv,getenv,char*,_tostr,char*,val_t,mk_str)
DECLARE_API_1(eofp,feof,FILE*,_tocfile,int,val_t,mk_bool)
DECLARE_API_1(peekc,peekwc,FILE*,_tocfile,int,val_t,mk_char)
DECLARE_API_1(getc,fgetwc,FILE*,_tocfile,int,val_t,mk_char)
DECLARE_API_1(close,fclose,FILE*,_tocfile,int,val_t,mk_int)
DECLARE_API_1(tell,ftell,FILE*,_tocfile,long,val_t,mk_int)
DECLARE_API_1(ferrorp,ferror,FILE*,_tocfile,int,val_t,mk_int)
DECLARE_API_1(fremv,remove,char*,_tostr,int,val_t,mk_int)


#define DECLARE_API_2(fname,cfnc,xtp,ytp,cstx,csty,crtp,rrtp,wrp)      \
  rrtp bltn_ ## fname ## _(val_t* a)                                   \
  {					                               \
    xtp _x = cstx(a[0],__FILE__,__LINE__,__func__) ;		       \
    ytp _y = csty(a[1],__FILE__,__LINE__,__func__) ;		       \
    crtp _result = cfnc(_x,_y)                     ;		       \
     return wrp(_result)                           ; 		       \
  }

DECLARE_API_2(open,fopen,char*,char*,_tostr,strval,FILE*,val_t,mk_cfile)
DECLARE_API_2(putc,fputwc,int,FILE*,_torchar,_tocfile,int,val_t,mk_int)
DECLARE_API_2(puts,fputs,char*,FILE*,_tostr,_tocfile,int,val_t,mk_int)
DECLARE_API_2(frename,rename,char*,char*,_tostr,_tostr,int,val_t,mk_int)

#define DECLARE_API_3(fname,cfnc,xtp,ytp,ztp,cstx,csty,cstz,crtp,rrtp,wrp) \
  rrtp bltn_ ## fname ## _(val_t* a)                                       \
  {					                                   \
    xtp _x = cstx(a[0],__FILE__,__LINE__,__func__) ;		           \
    ytp _y = csty(a[1],__FILE__,__LINE__,__func__) ;		           \
    ztp _z = cstz(a[2],__FILE__,__LINE__,__func__) ;			   \
    crtp _result = cfnc(_x,_y,_z)                  ;			   \
     return wrp(_result)                           ; 		           \
  }

DECLARE_API_3(gets,fgets,char*,int,FILE*,_tostr,_torint,_tocfile,char*,val_t,mk_str)
DECLARE_API_3(seek,fseek,FILE*,int,int,_tocfile,_torint,_torint,long,val_t,mk_int)

