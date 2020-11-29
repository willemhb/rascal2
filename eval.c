#include "eval.h"

static int32_t vm_validate_formals(rval_t formals, uint8_t bodytype) {
  /* 
     this helper function will return the number of arguments if it's a valid list of formals for the given
     bodytype, the negative of that count + 1 it's a valid argument list with rest args, and 0 if it is not
     a valid argument list.

   */
  if (bodytype == BODYTYPE_EXPR) {
    if (vm_list_of(formals, TYPECODE_INT)) {
      return vm_list_len(formals);
    } else {
      return -1;
    }
  } else if (vm_list_of(formals,TYPECODE_SYM)) {
    return vm_list_len(formals);
  } else if (vm_cons_of(formals,TYPECODE_SYM,REQUIRE_CONS)) {
    return -1 + (-1 * vm_cons_len(formals,REQUIRE_CONS));
  } else if (typecode(formals) == TYPECODE_SYM) {
    return -2;
  } else {
    return -1;
  }
}

/* working with procedure types */
rproc_t* vm_new_proc(rval_t formals, rval_t env, rval_t body, uint8_t callmode, uint8_t bodytype) {
  uint8_t vargs = VARGS_FALSE;
  int32_t nargs = vm_validate_formals(formals, bodytype);

  if (nargs == -1) {
    eprintf(stderr,"Invalid argument list of type %s.\n", vm_val_typename(formals));
    escape(ERROR_TYPE);
  } else if (nargs < 1) {
    nargs *= -1;
    nargs -= 1;
    vargs = VARGS_TRUE;
  }
  // this is where the argument list is evaluated
  
  
  uchr_t* obj = vm_allocate(sizeof(rproc_t));
  rproc_t* pobj = (rproc_t*)obj;
  pobj->head.type = TYPECODE_PROC;
  pobj->head.meta = nargs;
  pobj->head.flags = 0;

  setcallmode(pobj,callmode);
  setbodytype(pobj,bodytype);
  setvargs(pobj,vargs);
  pobj->formals = formals;
  pobj->env = env;
  pobj->body = body;

  return pobj;
}

uint32_t vm_proc_objsize() {
  return sizeof(rproc_t);
}

rval_t (*vm_proc_cfnc(rproc_t* p))(rcons_t*) {
  void* cfnc = cptr(p->body);
  return (rval_t(*)(rcons_t*))cfnc;
}

int32_t vm_check_argco(int32_t argc, rval_t v, bool vargs) {
  int32_t nargs = vm_list_len(v);

  if (nargs < 1) {
    return EVALARGS_NONLIST;
  } else if (nargs < argc) {
    return EVALARGS_ARITY_SHORT;
  } else if (nargs > argc  && !vargs) {
    return EVALARGS_ARITY_LONG;
  } else {
    return EVALARGS_OK;
  }
}

int32_t vm_check_argtypes(rval_t formals, rval_t v) {
  while (iscons(v)) {
    uint32_t currt = val(_tocons(formals)->car);
    rval_t curr = _tocons(v)->car;
    if (typecode(curr) != currt) return EVALARGS_TYPE;
    formals = _tocons(formals)->cdr;
    v = _tocons(v)->cdr;
  }

  return EVALARGS_OK;
}

int32_t vm_check_args(rproc_t* p, rval_t v) {
  int32_t argc = argco(p);
  rval_t formals = p->formals;
  bool hasv = vargs(p);
  int32_t argco_check = vm_check_argco(argc,v,hasv);

  if (argco_check) return argco_check;
  else if (bodytype(p) == BODYTYPE_CFNC) {
    return vm_check_argtypes(formals,v);
  } else {
    return EVALARGS_OK;
  }
}


/* stack interface */
void vm_push(rval_t v) {
  if (vm_stack_overflow(1)) {
    eprintf(stderr, "Exiting due to stack overflow.\n");
    escape(ERROR_OVERFLOW);
  }

  SP++;
  STACK[SP] = v;
  return;
}

void vm_save_rcons(rcons_t* r) {
  vm_push(tagcons(r));
  return;
}

void vm_save_rsym(rsym_t* r) {
  vm_push(tagsym(r));
  return;
}

rval_t vm_pop() {
  if (SP == -1) {
    eprintf(stderr, "Exiting due to stack underflow.\n");
    escape(ERROR_OVERFLOW);
  }

  return STACK[--SP];
}


rval_t vm_peek() {
  if (SP == -1) {
    eprintf(stderr, "Exiting due to illegal memory access on empty stack underflow.\n");
    escape(ERROR_OVERFLOW);
  }

  return STACK[SP];
}

void vm_restore_rval(rval_t* r) {
  *r = vm_pop();
  return;
}

void vm_restore_rcons(rcons_t** r) {
  *r = _tocons(vm_pop());
  return;
}

void vm_restore_rsym(rsym_t** r) {
  *r = _tosym(vm_pop());
  return;
}

rval_t vm_analyze(rval_t x) {
  int32_t t = typecode(x);
  int32_t i;
  switch (t) {
  case TYPECODE_SYM:
    return EV_VARIABLE;
  case TYPECODE_CONS:
    for (i=0;i < NUM_FORMS;i++) {
      if (_tosym(_tocons(x)->car) == BUILTIN_FORMS[i]) return i;
    }

    return EV_APPLY;

  default:
    return EV_LITERAL;
  }
}


void vm_eval() {
  static void* labels[] = { &&ev_setv, &&ev_quote, &&ev_let, &&ev_do,
                            &&ev_label, &&ev_fn, &&ev_macro, &&ev_if,
                            &&ev_literal, &&ev_variable, &&ev_assign,
			    &&ev_assign_end, &&ev_apply,
			    &&ev_apply_op_done, &&ev_apply_argloop,
			    &&ev_apply_accumarg, &&ev_apply_dispatch,
			    &&ev_return, &&ev_macro_return, &&ev_sequence_start,
			    &&ev_sequence_loop, &&ev_setv_assign, &&ev_let_argloop,
			    &&ev_label_apply, &&ev_if_branch };

  int32_t argcheck;

  goto *labels[vm_analyze(EXP)];

 ev_return:
  restore(CONTINUE);
  restore(ENV);

  goto *labels[CONTINUE];

 ev_macro_return:
  restore(CONTINUE);
  restore(ENV);
  EXP = VAL;

  goto *labels[vm_analyze(EXP)];
  
 ev_literal:  // save the current values of every register and install e in the environment
  VAL = EXP;
  goto *labels[CONTINUE];

 ev_variable:
  VAL = vm_get_sym_env(_tosym(EXP),ENV);

  goto *labels[CONTINUE];

 ev_quote:
  VAL = _car(_cdr(EXP));

  goto *labels[CONTINUE];

 ev_setv:
  save(CONTINUE);
  CONTINUE = EV_SETV_ASSIGN;
  EXP = _car(_cdr(_cdr(EXP)));
  save(EXP);
  EXP = _car(_cdr(EXP));

  goto *labels[vm_analyze(EXP)];

 ev_setv_assign:
  NAME = _tosym(VAL);
  vm_put_sym_env(NAME,ENV);  // add the name to the environment if it doesn't exist already
  restore(EXP);
  restore(CONTINUE);

  goto ev_assign;

 ev_assign:
  save(NAME);
  save(ENV);
  save(CONTINUE);

  CONTINUE = EV_ASSIGN_END;

  goto *labels[vm_analyze(EXP)];

 ev_assign_end:
  restore(CONTINUE);
  restore(NAME);
  restore(ENV);
  vm_set_sym_env(NAME, VAL, ENV);

  goto *labels[CONTINUE];

 ev_macro:
  UNEV = _tocons(_car(_cdr(_cdr(EXP))));
  EXP = _car(_cdr(EXP));
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
