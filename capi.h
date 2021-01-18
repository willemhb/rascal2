#ifndef capi_h

/* begin capi.h */
#define capi_h

#include "rascal.h"


/* 
   builtin.c and capi.c contain functions that wrap VM functions and C bindings
   as rascal accessible functions (functions that take a pointer to an array of arguments
   and a count of the array size).

   I would like to replace both of these with a proper C interface, but this is much easier to implement at the moment.

 */


const char* BUILTIN_FUNCNAMES[NUM_BUILTIN_FUNCTIONS] =
  {
  "alpha?", "alnum?",  "lower?", "upper?",
  "digit?", "xdigit?", "cntrl?", "graph?",
  "space?", "blank?", "print?", "punct?",
  "upper", "lower", "osgetenv", "exit",
  "open", "close", "eof?", "getc", "putc",
  "peekc", "gets", "puts",
  };

/* a few C api helpers */
char* strval(val_t,const char*,int,const char*);        // try to get a string representation of a rascal value

/* inlined functional bindings for C arithmetic */
// arithmetic
int add_ii(int,int);
int sub_ii(int,int);
int mul_ii(int,int);
int div_ii(int,int);
int rem_ii(int,int);
int neg_i(int);
long add_ll(long,long);
long sub_ll(long,long);
long mul_ll(long,long);
long div_ll(long,long);
long rem_ll(long,long);
long neg_l(long);
float add_ff(float,float);
float sub_ff(float,float);
float mul_ff(float,float);
float div_ff(float,float);
float rem_ff(float,float);
float neg_f(float);
double add_dd(double,double);
double sub_dd(double,double);
double mul_dd(double,double);
double div_dd(double,double);
double rem_dd(double,double);
double neg_d(double);

// comparison
bool eql_ii(int,int);
bool neql_ii(int,int);
bool gt_ii(int,int);
bool ge_ii(int,int);
bool lt_ii(int,int);
bool le_ii(int,int);
bool eql_ll(long,long);
bool neql_ll(long,long);
bool gt_ll(long,long);
bool ge_ll(long,long);
bool lt_ll(long,long);
bool le_ll(long,long);
bool eql_ff(float,float);
bool neql_ff(float,float);
bool gt_ff(float,float);
bool ge_ff(float,float);
bool lt_ff(float,float);
bool le_ff(float,float);
bool eql_dd(double,double);
bool neql_dd(double,double);
bool gt_dd(double,double);
bool ge_dd(double,double);
bool lt_dd(double,double);
bool le_dd(double,double);

// bitwise
int  or_ii(int,int);
int  xor_ii(int,int);
int  and_ii(int,int);
int  lsh_ii(int,int);
int  rsh_ii(int,int);
int  bneg_i(int);
long or_ll(long,long);
long xor_ll(long,long);
long and_ll(long,long);
long lsh_ll(long,long);
long rsh_ll(long,long);
long bneg_l(long);

#define DESCRIBE_API(fname) val_t bltn_## fname(val_t*)
#define DESCRIBE_VOID_API(fname) void bltn_## fname(val_t*)

// character handling
DESCRIBE_API(alphap);
DESCRIBE_API(alnump);
DESCRIBE_API(lowerp);
DESCRIBE_API(upperp);
DESCRIBE_API(digitp);
DESCRIBE_API(xdigitp);
DESCRIBE_API(cntrlp);
DESCRIBE_API(graphp);
DESCRIBE_API(spacep);
DESCRIBE_API(blankp);
DESCRIBE_API(printp);
DESCRIBE_API(punctp);
DESCRIBE_API(upper);
DESCRIBE_API(lower);
DESCRIBE_API(u8len);

// system calls
DESCRIBE_VOID_API(exit);
DESCRIBE_API(osgetenv);

// IO utilities
DESCRIBE_API(peekc);
DESCRIBE_API(putc);
DESCRIBE_API(getc);
DESCRIBE_API(gets);
DESCRIBE_API(puts);
DESCRIBE_API(open);
DESCRIBE_API(close);
DESCRIBE_API(eofp);


/* builtin functions */
// numeric functions


/* end capi.h */
#endif
