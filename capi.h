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

/* inlined functional bindings for C arithmetic */
// arithmetic
int add_ii(int,int);
int sub_ii(int,int);
int mul_ii(int,int);
int div_ii(int,int);
int rem_ii(int,int);
int neg_i(int);
int add_ff(float,float);
int sub_ff(float,float);
int mul_ff(float,float);
int div_ff(float,float);
int rem_ff(float,float);
int neg_f(float);

// comparison
bool eql_ii(int,int);
bool neql_ii(int,int);
bool gt_ii(int,int);
bool ge_ii(int,int);
bool lt_ii(int,int);
bool le_ii(int,int);
bool eql_ff(float,float);
bool neql_ff(float,float);
bool gt_ff(float,float);
bool ge_ff(float,float);
bool lt_ff(float,float);
bool le_ff(float,float);

// bitwise
int or_ii(int,int);
int xor_ii(int,int);
int and_ii(int,int);
int lsh_ii(int,int);
int rsh_ii(int,int);
int bneg_i(int);
/* end capi.h */
#endif
