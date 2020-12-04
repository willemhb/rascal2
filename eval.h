#ifndef eval_h

/* begin eval.h */
#define eval_h

#include "common.h"
#include "error.h"
#include "obj.h"
#include "env.h"


/* 
   This module handles the functions that implement the evaluator and types representing builtin functions,
   as well as the api for calling C functions.

   C functions that can be called within rascal should take two argumnets, an array of void pointers (evaluated arguments)
   and an argument count, and should return an val_t (a value that can be represented within the language). They
   assume that the argument array has already been validated for type and arity.

   rascal-compatible C functions are called within rascal through the pim_t type. When a pim_t is called, it uses its argument spec
   and argument count to validate arguments, leaving depositing evaluated arguments in the argarray. 

*/


/* 
   forward declarations 
   the argument evaluation functions return an integer (0 for success, negative numbers for)
   errors and leave the evaluated arguments in a register passed in to the evaluation function 

*/

// working with procedure types
/* arglist, body, environment, flags  */
val_t new_proc(val_t,val_t,val_t,proc_fl,proc_fl);
bool  check_argco(int_t,val_t,bool);
val_t invoke(val_t(*)(),int_t,val_t);

// unsafe accessors (not worrying about safe accessors right now)
#define formals_(p)  FAST_ACCESSOR_MACRO(p,proc_t*,formals)
#define env_(p)      FAST_ACCESSOR_MACRO(p,proc_t*,env)
#define body_(p)     FAST_ACCESSOR_MACRO(p,proc_t*,body)
#define xpbody_(p)   (toc_((p)->body))
#define cfncbody_(p) ((val_t (*)())((p)->body))

// working with the stack
void  push(val_t);
val_t pop();
val_t peek();


#define save push
#define restore(r) r = pop()
#define failf(c,e,fmt,args...) if (c) do { escapef(e,stderr,fmt,##args); } while (0)
#define fail(e,fmt,args...) do { escpapef(e,stderr,fmt,##args); } while (0)
#define branch(c,l) if (c) goto *labels[l]
#define jump(l) goto *labels[l]
#define dispatch(r) goto *labels[vm_analyze(r)]

// core evaluator
val_t analyze_expr(val_t);

// the first argument is a label to start at, or -1 (start at the normal label). The second value is an expression to be evaluated
// eval_expr assumes that any important values in the global registers have been saved. The third value is an environment. All three
// must explicitly be passed to eval_expr.
val_t eval_expr(int_t,val_t,val_t);

// builtin forms and VM evaluator labels
enum {
  EV_NORMAL_START=-1,
  EV_SETV,
  EV_QUOTE,
  EV_LET,
  EV_DO,
  EV_FN,
  EV_MACRO,
  EV_IF,
  NUM_FORMS,
  EV_LITERAL=NUM_FORMS,
  EV_VARIABLE,
  EV_ASSIGN,
  EV_ASSIGN_END,
  EV_APPLY,
  EV_APPLY_OP_DONE,
  EV_APPLY_ARGLOOP,
  EV_APPLY_ACCUMARG,
  EV_APPLY_DISPATCH,
  EV_RETURN,
  EV_MACRO_RETURN,
  EV_SEQUENCE_START,
  EV_SEQUENCE_LOOP,
  EV_SETV_ASSIGN,
  EV_LET_ARGLOOP,
  EV_IF_TEST,
  EV_IF_ALTERNATIVE,
  EV_IF_NEXT,
  EV_APPLY_TYPE,
  EV_APPLY_MACRO,
  EV_APPLY_BUILTIN,
  EV_APPLY_FUNCTION,
  EV_HALT,
};

// a separate set of labels to jump to when ERRORCODE is nonzero
enum {
  EVERR_TYPE=1,
  EVERR_VALUE,
  EVERR_ARITY,
  EVERR_UNBOUND,
  EVERR_OVERFLOW,
  EVERR_IO,
  EVERR_NULLPTR,
  EVERR_SYNTAX,
  EVERR_INDEX,
};

// save the symbols bound to builtin forms for faster evaluation
val_t BUILTIN_FORMS[NUM_FORMS];

/* end eval.h */
#endif
