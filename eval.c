#include "eval.h"


/* working with procedure types */
rproc_t* vm_new_proc(rval_t argl, rval_t env, rval_t body, uint32_t flags) {
  int32_t normed_arglist = norm_arglist(&argl);
  
  switch (normed_arglist) {
  case -1:
    eprintf(stderr,"Invalid argument list.\n");
    escape(ERROR_TYPE);
  default:
    flags |= normed_arglist;
    break;
  }

  uchr_t* obj = vm_allocate(sizeof(rproc_t));
  rproc_t* pobj = (rproc_t*)obj;
  pobj->head.type = TYPECODE_PROC;
  pobj->head.flags = flags;
  pobj->argspec = argl;
  pobj->body = body;

  return pobj;
}

uint32_t vm_proc_objsize() {
  return sizeof(rproc_t);
}

int32_t vm_proc_argco(rproc_t* p) {
  return vm_list_len(p->argspec);
}

int32_t vm_check_argco(rproc_t* p, rval_t v) {
  if (!islist(v)) {
    return EVALARGS_NONLIST;
  } else if (vm_list_len(v) < vm_proc_argco(p)) {
    return EVALARGS_ARITY_SHORT;
  } else if (vm_list_len(v) > vm_proc_argco(p) && !hasvargs(p)) {
    return EVALARGS_ARITY_LONG;
  } else {
    return EVALARGS_OK;
  }
}

void vm_push(rval_t v) {
  if (vm_stack_overflow(1)) {
    eprintf(stderr, "Exiting due to stack overflow.\n");
    escape(ERROR_OVERFLOW);
  }

  SP++;
  STACK[SP] = v;
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

rval_t vm_analyze(rval_t x) {
  int32_t t = typecode(x);
  int32_t i;
  switch (t) {
  case TYPECODE_INT:
  case TYPECODE_NONE:
  case TYPECODE_NIL:
  case TYPECODE_STR:
  case TYPECODE_TYPE:
  case TYPECODE_PORT:
  case TYPECODE_PROC:
    return EV_LITERAL;
  case TYPECODE_SYM:
    return EV_VARIABLE;
  case TYPECODE_CONS:
    for (i=0;i < EV_NUM_FORMS;i++) {
      if (_tosym(_tocons(x)->car) == BUILTIN_FORMS[i]) return i;
    }

    return EV_APPLICATION;

  default:
    return EV_UNKNOWN;
  }
}

void vm_eval() {
  static void* labels[] = { &&ev_def, &&ev_setv, &&ev_quote, &&ev_let,
                            &&ev_do, &&ev_label, &&ev_fn, &&ev_macro, &&ev_if,
                            &&ev_illegal, &&ev_literal, &&ev_variable, &&ev_application,
                            &&ev_unknown, &&ev_assign, &&ev_assign_end,
			    &&ev_application_op_done, };

  goto *labels[vm_analyze_expr(EXP)];
  
 ev_literal:  // save the current values of every register and install e in the environment
  VAL = EXP;
  goto *labels[CONTINUE];

 ev_variable:
  VAL = vm_assoc_env(_tosym(EXP),ENV);
  goto *labels[CONTINUE];

 ev_quote:
  VAL = _tocons(_tocons(EXP)->cdr)->car;
  goto *labels[CONTINUE];

 ev_def:
  UNEV = _tocons(_tocons(EXP)->cdr)->car;
  EXP = _tocons(_tocons(_tocons(EXP)->cdr)->cdr)->car;
  vm_put_sym_env(_tosym(UNEV), ENV);

  goto ev_assign;

 ev_setv:
  UNEV = _tocons(_tocons(EXP)->cdr)->car;
  EXP = _tocons(_tocons(_tocons(EXP)->cdr)->cdr)->car;

  goto ev_assign;

 ev_assign:
  save(UNEV);
  save(ENV);
  save(CONTINUE);

  CONTINUE = EV_ASSIGN_END;
  goto *labels[vm_analyze_expr(EXP)];

 ev_assign_end:
  restore(UNEV);
  restore(ENV);
  restore(CONTINUE);
  vm_set_sym_env(_tosym(UNEV), VAL, ENV);

  goto *labels[CONTINUE];

 ev_macro:
  UNEV = _tocons(_tocons(EXP)->cdr)->car;
  EXP = _tocons(_tocons(_tocons(EXP)->cdr)->cdr)->car;
  VAL = tagproc(vm_new_proc(UNEV, ENV, EXP, PROCFLAGS_CALLMODE_MACRO | PROCFLAGS_BODYTYPE_EXPR));
  goto *labels[CONTINUE];
  
 ev_fn:
  UNEV = _tocons(_tocons(EXP)->cdr)->car;
  EXP = _tocons(_tocons(_tocons(EXP)->cdr)->cdr)->car;
  VAL = tagproc(vm_new_proc(UNEV, ENV, EXP, PROCFLAGS_CALLMODE_FUNC | PROCFLAGS_BODYTYPE_EXPR));

  goto *labels[CONTINUE];

 ev_application:
  save(CONTINUE);
  save(ENV);
  UNEV = _tocons(_tocons(EXP)->cdr)->cdr;  // lambda body
  save(UNEV);
  EXP = _tocons(EXP)->car;
  CONTINUE = EV_APPLICATION_OP_DONE;

  goto *labels[vm_analyze_expr(EXP)];

 ev_application_op_done:
  restore(UNEV);
  restore(ENV);
  ARGL = NIL;
  PROC = VAL;

  if (ismacro(_toproc(PROC))) {
    goto ev_macro_args;
  } else if (isprim(_toproc(PROC))) {
    goto ev_prim_args;    
  } else {
    goto ev_fn_args;
  }

 ev_macro_args:
  
}
