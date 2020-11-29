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


enum {
  CALLMODE_FUNC,
  CALLMODE_MACRO,
  BODYTYPE_EXPR=0,
  BODYTYPE_CFNC,
  VARGS_TRUE=0,
  VARGS_FALSE,
};


#define tagproc(p)        tagptr(p,LOWTAG_OBJPTR)
#define argco(p)          ((p)->head.meta)
#define callmode(p)       ((p)->head.flags & 0b001)
#define bodytype(p)       (((p)->head.flags & 0b010) >> 1)
#define vargs(p)          (((p)->head.flags & 0b100) >> 2)
#define setcallmode(p,f)  p->head.flags &= 0b110; p->head.flags |= f
#define setbodytype(p,f)  p->head.flags &= 0b101; p->head.flags |= (f << 1)
#define setvargs(p,f)     p->head.flags &= 0b011; p->head.flags |= (f << 2)

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
rproc_t* vm_new_proc(rval_t,rval_t,rval_t,uint8_t,uint8_t);
uint32_t vm_proc_objsize();
rval_t (*vm_proc_cfnc(rproc_t*))(rcons_t*);
int32_t vm_check_argco(int32_t,rval_t,bool);
int32_t vm_check_argtypes(rval_t,rval_t);
int32_t vm_check_args(rproc_t*,rval_t);

// working with the stack
void vm_push(rval_t);
void vm_save_rcons(rcons_t*);
void vm_save_rsym(rsym_t*);
rval_t vm_pop();
rval_t vm_peek();
void vm_restore_rval(rval_t*);
void vm_restore_rcons(rcons_t**);
void vm_restore_rsym(rsym_t**);

/* generic vm_save macro */
#define save(r)             \
  _Generic((r),             \
    rcons_t*:vm_save_rcons, \
    rsym_t*:vm_save_rsym,   \
	   default:vm_push)(r)

/* generic vm_restore macros */
#define _restore(r)              \
  _Generic((r),                  \
    rcons_t**:vm_restore_rcons,  \
    rsym_t**:vm_restore_rsym,    \
	   default:vm_restore_rval)(r)

#define restore(r) _restore(&(r))

// core evaluator
rval_t vm_analyze_expr(rval_t);
void vm_eval();

// builtin forms
enum {
  EV_SETV,
  EV_QUOTE,
  EV_LET,
  EV_DO,
  EV_LABEL,
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
  EV_LABEL_APPLY,
  EV_IF_BRANCH,
};


// save the symbols bound to builtin forms for faster evaluation
rsym_t* BUILTIN_FORMS[NUM_FORMS];

/* end eval.h */
#endif
