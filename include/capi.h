#ifndef capi_h

/* begin capi.h */
#define capi_h

#include "common.h"
#include "rtypes.h"
#include "globals.h"


/* 

   This header includes the function declarations for:

   1) inlined bindings for C arithmetic operators

   2) builtins to wrap C library functions

   These are included in a separate header mostly because they're long and tedious

 */

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

#define API_0_0_(fname) val_t rsp_ ## fname(void)
#define API_1_0_(fname) val_t rsp_ ## fname(val_t)
#define API_2_0_(fname) val_t rsp_ ## fname(val_t,val_t)
#define API_3_0_(fname) val_t rsp_ ## fname(val_t*)
#define API_0_1_(fname) val_t rsp_ ## fname(size_t,val_t*)
#define DESCRIBE_API(fname,argc,varg) API_ ## argc ## _ ## varg ## _(fname)


// character handling, string handling, and basic os interface
DESCRIBE_API(exit,0,0);
DESCRIBE_API(alphap,1,0);
DESCRIBE_API(alnump,1,0);
DESCRIBE_API(lowerp,1,0);
DESCRIBE_API(upperp,1,0);
DESCRIBE_API(digitp,1,0);
DESCRIBE_API(xdigitp,1,0);
DESCRIBE_API(cntrlp,1,0);
DESCRIBE_API(graphp,1,0);
DESCRIBE_API(spacep,1,0);
DESCRIBE_API(blankp,1,0);
DESCRIBE_API(printp,1,0);
DESCRIBE_API(punctp,1,0);
DESCRIBE_API(upper,1,0);
DESCRIBE_API(lower,1,0);
DESCRIBE_API(u8len,1,0);
DESCRIBE_API(osgetenv,1,0);
DESCRIBE_API(systime,0,0);
DESCRIBE_API(rsptime,0,0);
DESCRIBE_API(peekc,1,0);
DESCRIBE_API(putc,2,0);
DESCRIBE_API(getc,1,0);
DESCRIBE_API(gets,2,0);
DESCRIBE_API(puts,2,0);
DESCRIBE_API(open,2,0);
DESCRIBE_API(close,1,0);
DESCRIBE_API(eofp,1,0);


/* end capi.h */
#endif
