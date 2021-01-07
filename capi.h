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

const char* BUILTIN_FUNCNAMES[NUM_BUILTIN_FUNCTIONS] = {   };

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

/* builtin functions */
// numeric functions
val_t rsp_add(val_t,val_t);
val_t rsp_sub(val_t,val_t);
val_t rsp_mul(val_t,val_t);
val_t rsp_div(val_t,val_t);
val_t rsp_rem(val_t,val_t);
val_t rsp_eql(val_t,val_t);
val_t rsp_neql(val_t,val_t);
val_t rsp_gt(val_t,val_t);
val_t rsp_ge(val_t,val_t);
val_t rsp_lt(val_t,val_t);
val_t rsp_le(val_t,val_t);
val_t rsp_bor(val_t,val_t);
val_t rsp_bxor(val_t,val_t);
val_t rsp_band(val_t,val_t);
val_t rsp_lsh(val_t,val_t);
val_t rsp_rsh(val_t,val_t);
val_t rsp_bneg(val_t);

// comparison
val_t rsp_ord(val_t,val_t);
val_t rsp_id(val_t,val_t);              // true if two objects are identical
val_t rsp_eq(val_t,val_t);              // true if two objects represent the same data

// general predicates/booleans
val_t rsp_nilp(val_t);
val_t rsp_nonep(val_t);
val_t rsp_truep(val_t);
val_t rsp_falsep(val_t);
val_t rsp_eofp(val_t);
val_t rsp_not(val_t);
val_t rsp_isa(val_t,val_t);            // test whether the first argument's type equals the
                                       // second argument

// IO & character handling
val_t rsp_open(val_t,val_t);
val_t rsp_close(val_t,val_t);
val_t rsp_read(val_t);
val_t rsp_load(val_t);
val_t rsp_prn(val_t,val_t);
val_t rsp_getc(val_t);
val_t rsp_putc(val_t,val_t);
val_t rsp_gets(val_t,val_t);
val_t rsp_puts(val_t,val_t);
val_t rsp_isctype(val_t,val_t);
val_t rsp_upper(val_t);
val_t rsp_lower(val_t);

// generic object API - these should be implemented as generic functions once generics
// are implemented
val_t    rsp_len(val_t);                // get the number of elements contained
val_t    rsp_asscf(val_t,val_t);        // return the value of a named field
val_t    rsp_assck(val_t,val_t);        // key-based search
val_t    rsp_asscn(val_t,val_t);        // index-based search
val_t    rsp_asscv(val_t,val_t);        // search for a value
val_t    rsp_rplcf(val_t,val_t,val_t);  // replace the value of a named field
val_t    rsp_rplck(val_t,val_t,val_t);  // change the value associated with a particular key
val_t    rsp_rplcn(val_t,val_t,val_t);  // replace the nth element of a collection
val_t    rsp_rplcv(val_t,val_t,val_t);  // replace the first occurence of a value
val_t    rsp_conj(val_t,val_t);         // add a new element to a sequence
val_t    rsp_join(val_t,val_t);         // merge two sequences
val_t    rsp_seq(val_t);                // return a cons whose first element is 
val_t    rsp_fst(val_t);                // get the first element of a sequence
val_t    rsp_rst(val_t);                // return a sequence whose cdr will yield the next
                                        // element of the sequence

/* end capi.h */
#endif
