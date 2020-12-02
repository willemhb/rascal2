#include "eval.h"

/* working with procedure types */
proc_t* new_proc(val_t formals, val_t env, val_t body, proc_fl callmode, proc_fl bodytype) {
  if (!isenvnames(formals)) {
    failv(TYPE_ERR,NULL,"Bad list of variable names");
  }
  
  int_t argco = ncells(formals);
  proc_fl vargs = islist(formals) ? VARGS_FALSE : VARGS_TRUE;

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

  return obj;
}

uint_t vm_proc_objsize() {
  return sizeof(proc_t);
}

inline bool check_argco(int_t argc, val_t argl, bool vargs) {
  int_t nargs = ncells(argl);

  if (nargs < argc) return false;
  else if (nargs > argc) return vargs;
  else return true;
}


/* stack interface */
inline void _push(val_t v) {
  if (stack_overflow(1)) {
    eprintf(OVERFLOW_ERR,stderr,"stack overflow");
    escape(OVERFLOW_ERR);
  }

  SP++;
  STACK[SP] = v;
  return;
}

inline void _pushs(sym_t* v) {
  _push(tagptr(v,LOWTAG_STROBJ));
  return;
}


inline void _pushc(cons_t* c) {
  _push(tagc(c));
  return;
}

inline void _pusho(obj_t* o) {
  _push(tagptr(o,LOWTAG_OBJPTR));
  return;
}

inline val_t pop() {
  if (SP == -1) {
    eprintf(OVERFLOW_ERR,stderr,"pop on empty stack");
    escape(OVERFLOW_ERR);
  }

  return STACK[--SP];
}

inline val_t peek() {
  if (SP == -1) {
    eprintf(OVERFLOW_ERR,stderr,"peek on empty stack");
    escape(OVERFLOW_ERR);
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
    for (i=0; i < NUM_FORMS; i++) {
      if (car_(x) == BUILTIN_FORMS[i]) return i;

      else x = cdr_(x);
      
      if (!iscons(x)) break;
    }

    if (i < NUM_FORMS) {
      return EV_LITERAL;  // read dotted lists as literals
    }

    return EV_APPLY;

  default:
    return EV_LITERAL;
  }
}


val_t vm_eval(val_t x, cons_t* e) {
  static void* labels[] = { &&ev_setv, &&ev_quote, &&ev_let, &&ev_do,
                            &&ev_label, &&ev_fn, &&ev_macro, &&ev_if,
                            &&ev_literal, &&ev_variable, &&ev_assign,
			    &&ev_assign_end, &&ev_apply,
			    &&ev_apply_op_done, &&ev_apply_argloop,
			    &&ev_apply_accumarg, &&ev_apply_dispatch,
			    &&ev_return, &&ev_macro_return, &&ev_sequence_start,
			    &&ev_sequence_loop, &&ev_setv_assign, &&ev_let_argloop,
			    &&ev_label_apply, &&ev_if_branch };

  static void* errors[] = { &&everr_unknown, &&everr_type, &&everr_value, &&everr_arity,
                            &&everr_unbound, &&everr_overflow, &&everr_io, &&everr_nullptr,
			    &&everr_syntax, &&everr_index };

  int_t argcheck;

  dispatch(EXP);

 everr_unknown: everr_type: everr_value: everr_arity:
 everr_unbound: everr_overflow: everr_io: everr_nullptr:
 everr_syntax: everr_index:
  clearerr();
  failv(ERRORCODE,NONE,"The error caused evaluation to fail.");

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
  VAL = env_get(EXP,ENV);
  errbranch(VAL == NONE,UNBOUND_ERR);
  jump(CONTINUE);

 ev_quote:
  VAL = *cxr(EXP,AD);
  errbranch(checkerr(VAL),ERRORCODE);
  jump(CONTINUE);

 ev_setv:
  save(CONTINUE);
  CONTINUE = EV_SETV_ASSIGN;
  EXP = *cxr(EXP,ADD);
  errbranch(checkerr(EXP),ERRORCODE);
  save(EXP);
  EXP = *cxr(EXP,AD);
  errbranch(checkerr(EXP),ERRORCODE);

  dispatch(EXP);

 ev_setv_assign:
  errbranch(!issym(VAL), TYPE_ERR);
  NAME = VAL;
  env_put(NAME,ENV);                // add the name to the environment if it doesn't exist already
  restore(EXP);
  restore(CONTINUE);

  jump(EV_ASSIGN);

 ev_assign:
  save(NAME);
  save(ENV);
  save(CONTINUE);

  CONTINUE = EV_ASSIGN_END;

  dispatch(EXP);

 ev_assign_end:
  restore(CONTINUE);
  restore(NAME);
  restore(ENV);

  env_set(NAME, VAL, toc_(ENV));
  
  jump(CONTINUE);

 ev_macro:
  RX = cxr(EXP,ADD);
  errbranch(checkerr(RX),ERRORCODE);
  EXP = 
  RY = cxr(EXP,AD);
  VAL = tagproc(vm_new_proc(EXP, tagcons(ENV), tagcons(UNEV), CALLMODE_MACRO, BODYTYPE_EXPR));
  
  goto *labels[CONTINUE];
  
 ev_fn:
  UNEV = _tocons(_car(_cdr(_cdr(EXP))));
  EXP = _car(_cdr(EXP));
  VAL = tagproc(vm_new_proc(EXP, tagcons(ENV), tagcons(UNEV), CALLMODE_FUNC, BODYTYPE_EXPR));
  
  goto *labels[CONTINUE];

 ev_apply:
  save(CONTINUE);  
  UNEV = _tocons(_cdr(EXP));  // list of applied arguments
  save(UNEV);
  EXP = _car(EXP);
  CONTINUE = EV_APPLY_OP_DONE;

  goto *labels[vm_analyze_expr(EXP)];

 ev_apply_op_done:
  restore(UNEV);
  restore(CONTINUE);

  if (istype(VAL)) {
    PROC = _totype(VAL)->tp_constructor;
  } else {
    PROC = _toproc(VAL);
  }
  
  if (callmode(PROC) == CALLMODE_MACRO) {
    ARGL = UNEV;
    goto ev_apply_dispatch;
  } else {
    ARGL = NULL;
    goto ev_apply_argloop;
  }

 ev_apply_argloop:
  if (UNEV == NULL) {
    goto ev_apply_dispatch;
  }

  EXP = UNEV->car;
  save(ARGL);
  save(UNEV);

  goto *labels[vm_analyze(EXP)];

 ev_apply_accumarg:
  restore(UNEV);
  restore(ARGL);
  vm_list_append(&ARGL, VAL);
  UNEV = _lstail(UNEV);

  goto ev_apply_argloop;

 ev_apply_dispatch:
  argcheck = vm_check_args(PROC,tagcons(ARGL));
  if (argcheck) {
    eprintf(stderr, "Bad argument list with code %d.\n", argcheck);
    escape(ERROR_TYPE);
  }

  if (bodytype(PROC) == BODYTYPE_CFNC) {
    VAL = vm_proc_cfnc(PROC)(ARGL);
    goto *labels[CONTINUE];
  }

  save(ENV);
  save(CONTINUE);

  UNEV = _tocons(PROC->body);
  ENV = vm_new_env(PROC->formals, ARGL, ENV);
  CONTINUE = callmode(PROC) == CALLMODE_MACRO ? EV_MACRO_RETURN : EV_RETURN;

  goto ev_sequence_start;

 ev_sequence_start: // assumes that unev is a list of 
  VAL = NIL;
  save(CONTINUE);
  save(UNEV);

  goto ev_sequence_loop;

 ev_sequence_loop:
  restore(UNEV);
  restore(CONTINUE);

  if (UNEV == NULL) {
    goto *labels[CONTINUE];
  }
  save(CONTINUE);

  EXP = UNEV->car;
  UNEV = _tocons(UNEV->cdr);
  save(UNEV);
  CONTINUE = EV_SEQUENCE_LOOP;

  goto *labels[vm_analyze(EXP)];

 ev_if:
  UNEV = _lstail(_lstail(_tocons(EXP)));
  save(UNEV);
  EXP = _car(_cdr(EXP));
  save(CONTINUE);
  CONTINUE = EV_IF_BRANCH;

  goto *labels[vm_analyze(EXP)];

 ev_do:
  UNEV = _tocons(_cdr(EXP));

  goto ev_sequence_start;

 ev_let:
  save(ENV);                     
  save(CONTINUE);
  CONTINUE = EV_LET_ARGLOOP;
  
  ENV = vm_new_env(NIL,NULL,ENV); // set up new frame
  UNEV = _tocons(_cdr(_cdr(EXP)));
  ARGL = _tocons(_car(_cdr(EXP)));
  save(UNEV);
  save(ARGL);

  goto ev_let_argloop;

 ev_let_argloop:
  restore(ARGL);

  if (ARGL == NULL) {
    restore(UNEV);
    CONTINUE = EV_RETURN;
    goto ev_sequence_start;

  } else {
    NAME = _tosym(ARGL->car);
    EXP = _car(_cdr(ARGL->cdr));
    ARGL = _lstail(_lstail(ARGL));
    vm_put_sym_env(NAME,ENV);
    save(ARGL);

    goto ev_assign;
  }
  
 ev_label:
  save(CONTINUE);
  CONTINUE = EV_LABEL_APPLY;
  save(ENV);
  ENV = vm_new_env(NIL,NULL,ENV);
  
  NAME = _tosym(_car(_cdr(EXP)));
  UNEV = _tocons(_cdr(_cdr(_cdr(EXP))));
  EXP = _car(_cdr(_cdr(EXP)));
  save(UNEV);  
  vm_put_sym_env(NAME,ENV);

  goto ev_assign;

 ev_label_apply:
  restore(UNEV);
  CONTINUE = EV_RETURN;

  goto ev_sequence_start;

 ev_if_branch:
  restore(CONTINUE);
  restore(UNEV);
  EXP = VAL == NIL || VAL == NONE ? _car(UNEV->cdr) : UNEV->car;

  goto *labels[vm_analyze(EXP)];
}
