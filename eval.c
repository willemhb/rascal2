#include "rascal.h"

/* working with procedure types */
val_t new_proc(val_t formals, val_t env, val_t body, proc_fl callmode, proc_fl bodytype) {
  int_t argco;
  proc_fl vargs;
  
  if (bodytype == BODYTYPE_CFNC) {
    argco = formals;
    vargs = VARGS_FALSE; // this flag should be explicitly altered during initialization
    formals = NIL;

  } else if (!isenvnames(formals)) {
    escapef(TYPE_ERR, stderr, "Malformed argument list.");
  } else {
    argco = ncells(formals);
    vargs = islist(formals) ? VARGS_FALSE : VARGS_TRUE;
    fprintf(stdout, "%d\n", vargs);
  }

  proc_t* obj = (proc_t*)vm_allocate(sizeof(proc_t));

  // initialize
  type_(obj) = TYPECODE_PROC;
  argco_(obj) = argco;
  callmode_(obj) = callmode;
  bodytype_(obj) = bodytype;
  vargs_(obj) = vargs;
  formals_(obj) = formals;
  env_(obj) = env;
  body_(obj) = body;

  return tagp(obj);
}

inline bool check_argco(int_t argc, val_t argl, bool vargs) {
  int_t nargs = ncells(argl);

  if (nargs < argc) return false;
  else if (nargs > argc) return vargs;
  else return true;
}

inline val_t invoke(val_t (*behavior)(), int_t argco, val_t argl, bool vargs) {
  val_t argarray[argco];

  for (int_t i = 0; i < argco; i++) {
    if (argl == NIL) {
      escapef(ARITY_ERR,stderr,"Expected %d, got %d", argco, i);
    }
    argarray[i] = car_(argl);
    argl = cdr_(argl);
  }

  if (argl != NIL && !vargs) {
    escapef(ARITY_ERR, stderr, "Expected %d, got %d", argco,argco + ncells(argl));
  }

  switch (argco) {
  case 1:
    return behavior(argarray[0]);
  case 2:
    return behavior(argarray[0], argarray[1]);
  case 3:
    return behavior(argarray[0], argarray[1], argarray[2]);
  case 4:
    return behavior(argarray[0], argarray[1], argarray[2], argarray[3]);
  case 5:
    return behavior(argarray[0], argarray[1], argarray[2], argarray[3], argarray[4]);
  case 6:
    return  behavior(argarray[0], argarray[1], argarray[2], argarray[3], argarray[4], argarray[5]);
  default:
    escapef(ARITY_ERR,stderr,"Rascal does not currently support builtin functions of more than 6 fixed arguments.");
  }
}

/* stack interface */
inline void push(val_t v) {
  if (stack_overflow(1)) {
    escapef(OVERFLOW_ERR,stderr,"stack overflow");
  }

  SP++;
  STACK[SP] = v;
  return;
}

inline val_t pop() {
  if (SP == -1) {
    escapef(OVERFLOW_ERR,stderr,"pop on empty stack");
  }

  val_t out = STACK[SP];
  SP--;
  return out;
}

inline val_t peek() {
  if (SP == -1) {
    escapef(OVERFLOW_ERR,stderr,"peek on empty stack");
  }

  return STACK[SP];
}

val_t vm_analyze(val_t x) {
  int_t t = typecode(x);
  int_t i;
  switch (t) {
  case TYPECODE_SYM:
    return EV_VARIABLE;
  case TYPECODE_CONS:
    if (!islist(x)) {
      return EV_LITERAL;
    }
    for (i=0; i < NUM_FORMS; i++) {
      if (car_(x) == BUILTIN_FORMS[i]) {
	return i;
      }
    }
    return EV_APPLY;

  default:
    return EV_LITERAL;
  }
}


val_t eval_expr(val_t x,  val_t e) {

  static void* labels[] = {
                            &&ev_setv,
			    &&ev_quote,
			    &&ev_let,
			    &&ev_do,
                            &&ev_fn,
			    &&ev_macro,
			    &&ev_if,
			    &&ev_literal,
			    &&ev_variable,
			    &&ev_assign,
			    &&ev_assign_end,
			    &&ev_apply,
			    &&ev_apply_op_done,
			    &&ev_apply_argloop,
			    &&ev_apply_accumarg,
			    &&ev_apply_dispatch,
			    &&ev_return,
			    &&ev_macro_return,
			    &&ev_sequence_start,
			    &&ev_sequence_loop,
			    &&ev_setv_assign,
			    &&ev_let_argloop,
			    &&ev_if_test,
			    &&ev_if_alternative,
			    &&ev_if_next,
			    &&ev_apply_macro,
			    &&ev_apply_builtin,
			    &&ev_halt,
  };

  static void* errors[] = { &&everr_unknown, &&everr_type, &&everr_value, &&everr_arity,
                            &&everr_unbound, &&everr_overflow, &&everr_io, &&everr_nullptr,
			    &&everr_syntax, &&everr_index, &&everr_application, };

  r_errc_t error = setjmp(SAFETY);

  if (error) {
    goto *errors[error];
  } else {
    goto ev_start;
  }

 ev_start:
  /* save the current values of all registers */
  save(EXP);
  save(VAL);
  save(CONTINUE);
  save(NAME);
  save(ENV);
  save(UNEV);
  save(ARGL);
  save(PROC);

  EXP = x;
  
  failf(!(islist(e) || isnone(e)),TYPE_ERR,"Invalid environment type.");
  ENV = isnone(e) ? ENV : e;
  CONTINUE = EV_HALT;

  dispatch(EXP);

 ev_halt:
  WRX = VAL;
  restore(PROC);
  restore(ARGL);
  restore(UNEV);
  restore(ENV);
  restore(NAME);
  restore(CONTINUE);
  restore(VAL);
  restore(EXP);

  return WRX;
  
 everr_type: everr_value: everr_arity:
 everr_unbound: everr_syntax: everr_index:
 everr_application:
  eprintf(error,stderr,"The error caused eval to fail, but not fatally.");
  return NONE;

 everr_nullptr:
  eprintf(error,stderr,"Unexpected null pointer. Exiting.");
  exit(EXIT_FAILURE);
 
 everr_io:
  eprintf(error,stderr,"Exiting to avoid system issues.");
  exit(EXIT_FAILURE);
  
 everr_overflow:
  eprintf(error,stderr,"Memory overflow caused the program to fail. Exiting.");
  exit(EXIT_FAILURE);
  
 everr_unknown:
  eprintf(error,stderr,"Suspicious error caused eval to fail. Exiting.");
  exit(EXIT_FAILURE);

 ev_return:
  restore(CONTINUE);
  restore(ENV);

  jump(CONTINUE);

 ev_macro_return:
  restore(CONTINUE);
  restore(ENV);
  EXP = VAL;

  dispatch(EXP);

 ev_literal:  // save the current values of every register and install e in the environment
  VAL = EXP;
  jump(CONTINUE);

 ev_variable:
  VAL = vm_gete(EXP,ENV);
  failf(VAL == NONE, UNBOUND_ERR, "Unbound symbol error.");

  jump(CONTINUE);

 ev_quote:
  VAL = cxr(EXP,AD);

  jump(CONTINUE);

 ev_setv:
  save(CONTINUE);
  CONTINUE = EV_SETV_ASSIGN;
  UNEV = cxr(EXP,ADD);
  save(UNEV);
  EXP = cxr(EXP,AD);

  dispatch(EXP);

 ev_setv_assign:
  failf(!issym(VAL),TYPE_ERR, "Attempt to assign to non-symbol");
  failf(fl_const_(tosym_(VAL)), VALUE_ERR, "Attempt to reassign constant symbol %s", name_(VAL));
  NAME = VAL;
  restore(UNEV);
  EXP = UNEV;
  restore(CONTINUE);

  jump(EV_ASSIGN);

 ev_assign:
  vm_pute(NAME,ENV);
  save(NAME);
  save(ENV);
  save(CONTINUE);

  CONTINUE = EV_ASSIGN_END;

  dispatch(EXP);

 ev_assign_end:
  restore(CONTINUE);
  restore(ENV);
  restore(NAME);

  vm_sete(NAME, VAL, ENV);

  jump(CONTINUE);

 ev_macro:
  WRX = cxr(EXP,AD); // formals
  WRY = cxr(EXP,DD); // body
  VAL = new_proc(WRX, ENV, WRY, CALLMODE_MACRO, BODYTYPE_EXPR);

  jump(CONTINUE);
  
 ev_fn:
  WRX = cxr(EXP,AD);
  WRY = cxr(EXP,DD);
  VAL = new_proc(WRX, ENV, WRY, CALLMODE_FUNC, BODYTYPE_EXPR);

  jump(CONTINUE);

 ev_apply:
  save(CONTINUE);
  CONTINUE = EV_APPLY_OP_DONE;

  UNEV =  cdr_(EXP); // list of applied arguments
  save(UNEV);
  EXP = car_(EXP);

  dispatch(EXP);

 ev_apply_op_done:
  restore(UNEV);
  restore(CONTINUE);
  PROC = istype(VAL) ? totype_(VAL)->tp_new : VAL;
  failf(!isproc(PROC),APPLICATION_ERR,"Attempt to apply non-function with type %s", typename(PROC));

  // skip evalarg loop for macros
  branch(callmode_(toproc_(PROC)) == CALLMODE_MACRO,EV_APPLY_MACRO);

  ARGL = NIL;
  save(PROC);
  save(CONTINUE);
  
  CONTINUE = EV_APPLY_ACCUMARG;
  
  goto ev_apply_argloop;

 ev_apply_argloop:
  branch(UNEV == NIL, EV_APPLY_DISPATCH);

  EXP = car(UNEV);

  save(ENV);
  save(ARGL);
  save(UNEV);

  dispatch(EXP);

 ev_apply_accumarg:
  restore(UNEV);
  restore(ARGL);
  restore(ENV);

  append(&ARGL, VAL);
  UNEV = cdr_(UNEV);

  goto ev_apply_argloop;

 ev_apply_dispatch:
  restore(CONTINUE);  // saved at apply_op_done
  restore(PROC);      // saved at apply_op_done

  branch(bodytype_(PROC) == BODYTYPE_CFNC, EV_APPLY_BUILTIN);

  failf(!check_argco(argco_(PROC),ARGL,vargs_(PROC)), ARITY_ERR, "expected %d, got %d.",argco_(PROC),ncells(ARGL));

  save(ENV);          // save for after procedure application
  save(CONTINUE);     // restore after procedure application

  UNEV = body_(PROC);
  ENV = new_env(formals_(PROC), ARGL, env_(PROC));
  CONTINUE = EV_RETURN;

  goto ev_sequence_start;

 ev_apply_builtin:
  VAL = invoke((val_t (*)())(body_(PROC)),argco_(PROC),ARGL,vargs_(PROC));
  jump(CONTINUE);

 ev_apply_macro:
  ARGL = UNEV;

  failf(!check_argco(argco_(PROC),ARGL,vargs_(PROC)), ARITY_ERR, "Incorrect arity.");

  save(ENV);
  save(CONTINUE);
  
  UNEV = body_(PROC);
  ENV = new_env(formals_(PROC),ARGL,env_(PROC));
  CONTINUE = EV_MACRO_RETURN;

  goto ev_sequence_start;
  
 ev_sequence_start: // assumes that unev is a list of 
  VAL = NIL;
  save(CONTINUE);
  save(UNEV);

  goto ev_sequence_loop;

 ev_sequence_loop:
  restore(UNEV);
  restore(CONTINUE);

  branch(UNEV == NIL, CONTINUE);

  save(CONTINUE);

  EXP = car_(UNEV);
  UNEV = cdr_(UNEV);
  save(UNEV);

  CONTINUE = EV_SEQUENCE_LOOP;

  dispatch(EXP);

 ev_if:
  save(CONTINUE);
  CONTINUE = EV_IF_TEST;
  
  UNEV = cxr(EXP,DD);

  failf(UNEV==NIL,ARITY_ERR,"No branches supplied to if");
  
  save(UNEV);

  EXP = cxr(EXP,AD);

  dispatch(EXP);

 ev_do:
  UNEV = cdr_(EXP);

  goto ev_sequence_start;

 ev_let:
  save(ENV);                     
  save(CONTINUE);
  CONTINUE = EV_LET_ARGLOOP;
  
  ENV = new_env(NIL,NIL,ENV);      // set up new frame
  UNEV = cxr(EXP,DD);              // the let body arguments
  ARGL = cxr(EXP,AD);              // The let argument list
  save(UNEV);
  save(ARGL);

  goto ev_let_argloop;

 ev_let_argloop:
  restore(ARGL);

  if (ARGL == NIL) {
    restore(UNEV);
    CONTINUE = EV_RETURN;
    goto ev_sequence_start;

  } else {
    NAME = car_(ARGL);
    failf(!issym(NAME),TYPE_ERR,"Let bindings must be symbols, name has type %s.",typename(NAME));
    ARGL = cdr_(ARGL);
    failf(ARGL == NIL, ARITY_ERR, "Unpaired let binding.");
    EXP = car_(ARGL);
    ARGL = cdr_(ARGL);
    vm_pute(NAME,ENV);
    save(ARGL);

    goto ev_assign;
  }

 ev_if_test:
  restore(UNEV);
  restore(CONTINUE);

  branch(!rbooltocbool(VAL), EV_IF_ALTERNATIVE);
  
  EXP = car_(UNEV);
  dispatch(EXP);
  
 ev_if_alternative:
  UNEV = cdr_(UNEV);               // drop the consequent
  branch(UNEV == NIL, CONTINUE);   // the value of the if form is NIL if no alternative given 
  EXP = car_(UNEV);                // get the next expression to be tried
  UNEV = cdr_(UNEV);
  branch(UNEV != NIL, EV_IF_NEXT); // if UNEV is not NIL, treat this as another predicate

  dispatch(EXP);

  ev_if_next:
    save(CONTINUE);
    save(UNEV);

    CONTINUE = EV_IF_TEST;

    dispatch(EXP);
}
