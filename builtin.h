#ifndef builtin_h
/* begin builtin.h */
#define builtin_h
#include "common.h"
#include "obj.h"
#include "env.h"
#include "txtio.h"

/* names for various builtins */
chr_t* builtin_forms[] = {"setv", "quote", "let",
                          "do", "fn", "macro", "if" };

chr_t* special_variables[] = {"nil", "none", "t", "stdio", "stdin",
                              "stderr", "*globals*", "*readtable*", };

/* 
   macros for defining some of these fucking builtins 

 */

#define DESCRIBE_ARITHMETIC(op,name,rtn)               \
  inline val_t name(val_t x, val_t y) {                \
  int_t xn = toint(x);                                 \
  int_t yn = toint(y);                                 \
  return rtn(xn (op) yn);                              \
}

#define DESCRIBE_PREDICATE(name,test)	               \
  inline val_t name(val_t x) {                         \
  if test(x) return T ;                                \
  else return NIL ;                                    \
  }

#define DECLARE(name,argco) DECLARE_##argco(name)
#define DECLARE_1(name) val_t name(val_t)
#define DECLARE_2(name) val_t name(val_t,val_t)

DECLARE(add,2);
DECLARE(sub,2);
DECLARE(mul,2);
DECLARE(r_div,2);
DECLARE(rem,2);
DECLARE(r_eqnum,2);
DECLARE(r_lt,2);
DECLARE(nilp,1);
DECLARE(nonep,1);
DECLARE(r_isa,2);
DECLARE(eqp,2);

/* somewhat more involved functions declared here */

val_t r_size(val_t);
val_t r_cmp(val_t,val_t);
val_t r_typeof(val_t);
val_t rplca(val_t,val_t);
val_t rplcd(val_t,val_t);
val_t r_eval(val_t,val_t);                // r_eval and r_apply both work by saving the current contents of global registers,
val_t r_apply(val_t,val_t);               // setting a start point for eval_expr, restoring the registers after eval_expr returns,
                                          // and returning the contents of val.
val_t r_sym(val_t);
val_t r_str(val_t);
val_t r_int(val_t);

#define NUM_BUILTINS 30

static const chr_t* builtin_functions[] = {
  "+", "-", "*", "/", "rem",                  // arithmetic
  "<", "=", "eq?", "nil?","none?",            // predicates and comparison
  "cmp", "type", "isa?", "size", "cons",
  "sym", "str", "int", "car", "cdr",
  "rplca", "rplcd", "open", "close", "read",
  "prn", "reads", "load", "eval", "apply",         
};

static const int_t builtin_function_argcos[] = {
  2, 2, 2, 2, 2,
  2, 2, 2, 1, 1,
  2, 1, 2, 1, 2,
  1, 1, 1, 1, 1,
  2, 2, 2, 1, 1,
  2, 1, 1, 2, 2,
};

static const void* builtin_function_callables[] = {
  add, sub, mul, r_div, rem,
  r_lt, r_eqnum, eqp,				       
  nilp, nonep, r_cmp, r_typeof,
  r_isa, r_size, cons, r_sym,
  r_str, r_int, car, cdr, rplca, rplcd,
  open, close, read, prn, reads,
  load, r_eval, r_apply,
};

/* bootstrapping builtin functions */
proc_t* _new_builtin_function(chr_t*,int_t,void*);  // creates a proc_t* and adds its name to the global symbol table with a constant
sym_t*  _new_form_name(chr_t*);                     // binding
sym_t*  _new_special_variable(chr_t*);
/* memory initialization */
void init_heap();
void init_types();
void init_builtin_types();
void init_global_symbols();
void init_builtin_functions();

/* end builtin.h */
#endif
