#ifndef builtin_h

/* begin builtin.h */
#define builtin_h

#include "common.h"
#include "error.h"
#include "obj.h"
#include "env.h"
#include "txtio.h"

/* 
   macros for defining some of these fucking builtins 

 */

#define DESCRIBE_ARITHMETIC(name,op,rtn)	       \
  inline val_t name(val_t x, val_t y) {                \
  int_t xn = toint(x) ;                                \
  int_t yn = toint(y) ;                                \
  return rtn(op(xn,yn)) ;			       \
}


#define DESCRIBE_PREDICATE(name,test)	               \
  inline val_t name(val_t x) {                         \
    if (test(x)) return T ;			       \
    else return NIL ;				       \
}

#define DECLARE(name,argco) DECLARE_##argco(name)
#define DECLARE_1(name) val_t name(val_t)
#define DECLARE_2(name) val_t name(val_t,val_t)

DECLARE(r_add,2);
DECLARE(r_sub,2);
DECLARE(r_mul,2);
DECLARE(r_div,2);
DECLARE(r_rem,2);
DECLARE(r_eqnum,2);
DECLARE(r_lt,2);
DECLARE(r_nilp,1);
DECLARE(r_nonep,1);


/* somewhat more involved functions declared here */

val_t r_isa(val_t,val_t);
val_t r_eqp(val_t,val_t);
val_t r_size(val_t);
val_t r_cmp(val_t,val_t);
val_t r_typeof(val_t);
val_t r_rplca(val_t,val_t);
val_t r_rplcd(val_t,val_t);
val_t r_eval(val_t,val_t);                 
val_t r_apply(val_t,val_t);                              
val_t r_sym(val_t);
val_t r_str(val_t);
val_t r_int(val_t);

#define NUM_BUILTINS 30

/* bootstrapping builtin functions */
void _new_builtin_function(const chr_t*,int_t,const void*);  
val_t  _new_form_name(chr_t*);                 
val_t  _new_self_evaluating(chr_t*);

/* memory initialization */
void init_heap();
void init_types();
void init_registers();

/* initialization of core language */
void init_builtin_types();
void init_forms();
void init_special_variables();
void init_builtin_functions();

/* toplevel bootstrapping function */
void bootstrap_rascal();

/* end builtin.h */
#endif
