#ifndef eval_h

/* begin eval.h */
#define eval_h

#include "common.h"
#include "obj.h"
#include "mem.h"
#include "env.h"


/* 
   This module handles the functions that implement the evaluator and types representing builtin functions,
   as well as the api for calling C functions.

   C functions that can be called within rascal should take two argumnets, an array of void pointers (evaluated arguments)
   and an argument count, and should return an rval_t (a value that can be represented within the language). They
   assume that the argument array has already been validated for type and arity.

   rascal-compatible C functions are called within rascal through the prim_t type. When a prim_t is called, it uses its argument spec
   and argument count to validate arguments, leaving depositing evaluated arguments in the argarray. 

*/
#define tagproc(p)      tagptr(p,LOWTAG_OBJPTR)
#define callmode(p)     ((p)->head.flags & 0b010)
#define bodytype(p)     ((p)->head.flags & 0b001)
#define vargs(p)        ((p)->head.flags & 0b100)
#define hasvargs(p)     (vargs(p) == PROCFLAGS_VARGS_TRUE)
#define ismacro(p)      (callmode(p) == PROCFLAGS_CALLMODE_MACRO)
#define isfunc(p)       (callmode(p) == PROCFLAGS_CALLMODE_FUNC)
#define isprim(p)       (bodytype(p) == PROCFLAGS_BODYTYPE_CFNC)
#define isexpr(p)       (bodytype(p) == PROCFLAGS_BODYTYPE_EXPR)

/* 
   forward declarations 
   the argument evaluation functions return an integer (0 for success, negative numbers for)
   errors and leave the evaluated arguments in a register passed in to the evaluation function 

*/

enum {
  EVALARGS_OK=0,
  EVALARGS_NONLIST=1,
  EVALARGS_ARITY_SHORT=2,
  EVALARGS_ARITY_LONG=3,
  EVALARGS_TYPE=4,
};

// working with procedure types
/* arglist, body, environment, flags  */
rproc_t* vm_new_proc(rval_t,rval_t,rval_t,uint32_t);
uint32_t vm_proc_objsize();
int32_t vm_proc_argco(rproc_t*);
int32_t vm_check_argco(rproc_t*,rval_t);
int32_t vm_check_argtypes(rproc_t*,rval_t);

// working with the stack
void vm_push(rval_t);
rval_t vm_pop();
rval_t vm_peek();

// convenience macros for the stack
#define save vm_push
#define restore(reg) reg = vm_pop()

// core evaluator
rval_t vm_analyze_expr(rval_t);
void vm_eval();

// builtin forms
enum {
  EV_DEF,
  EV_SETV,
  EV_QUOTE,
  EV_LET,
  EV_DO,
  EV_LABEL,
  EV_FN,
  EV_MACRO,
  EV_IF,
  EV_NUM_FORMS,
  EV_DISPATCH,
  EV_LITERAL,
  EV_VARIABLE,
  EV_APPLICATION,
  EV_UNKNOWN,
  EV_ASSIGN,
  EV_ASSIGN_END,
  EV_APPLICATION_OP_DONE,
  EV_ARG_LOOP,
  EV_ACCUMULATE_ARG,
  EV_APPLICATION_DISPATCH,
  EV_MACRO_DONE,
};


// save the symbols bound to builtin forms for faster evaluation
rsym_t* BUILTIN_FORMS[EV_NUM_FORMS];

/* end eval.h */
#endif
