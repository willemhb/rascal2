#include "../include/eval.h"


static val_t vm_form_type(val_t exp)
{
  switch (ltag(exp))
    {
    default: return LBL_LITERAL;
    case SYMBOL: return LBL_SYMBOL;
    case LIST:
      {
	if (!exp) return LBL_LITERAL;
	val_t f = car_(exp);

	if (f == F_SETV) return LBL_SETV;
	else if (f == F_DEF) return LBL_DEF;
	else if (f == F_QUOTE) return LBL_QUOTE;
	else if (f == F_IF) return LBL_IF;
	else if (f == F_FUN) return LBL_FUN;
	else if (f == F_MACRO) return LBL_MACRO;
	else if (f == F_DO) return LBL_DO;
	else if (f == F_LET) return LBL_LET;
	else return LBL_FUNAPP;
      }
    }
}


val_t rsp_eval(val_t expr, val_t envt, val_t nmspc)
{
  static void* labels[] = { &&lbl_literal, &&lbl_symbol, &&lbl_quote,
                            &&lbl_setv, &&lbl_def, &&lbl_if,
			    &&lbl_fun, &&lbl_macro, &&lbl_do, &&lbl_let,
			    &&lbl_funapp, };
  
  if (isnil(nmspc))
    nmspc = MAIN;

  if (isnil(envt))
    envt = (val_t)mk_listn(1,nmspc);

  else
    envt = (val_t)mk_listn(2,envt,nmspc);

  val_t* BASE = STACK + SP + 1;
  
  
 lbl_start:
  EXP  = expr;
  BASE = SP;
  ENVT = envt;
  CONT = R_NIL;
  BODY = R_NIL;
  NEXT = LBL_HALT;
  goto lbl_dispatch;

 lbl_halt:
  return VAL;

 lbl_next:
  goto *labels[NEXT >> 32];

 lbl_dispatch:
  goto *labels[vm_form_type(EXP) >> 32];

 lbl_literal:
  VAL = EXP;
  goto lbl_next;

 lbl_symbol:
  VAL = EXP;
  sym = ptr(symbol_t*,VAL);
  if (!(sym->flags & SM_KEYWORD))
    VAL = env_get(VAL,ENVT);

  goto lbl_next;

 lbl_quote:
  VAL = pop_list(1,&EXP);

  goto lbl_next;

 lbl_setv:
  TMP[0] = pop_list(1,&EXP);             // name
  EXP    = pop_list(1,&EXP);             // binding
  save(RX_NEXT | RX_TMP0 | RX_ENVT);
  NEXT   = LBL_ASSIGN;

  goto lbl_dispatch;

 lbl_def:
  TMP[0] = pop_list(1,&EXP);       // name
  EXP    = pop_list(1,&EXP);       // binding
  env_put(VAL, ENVT);
  save(RX_NEXT | RX_TMP0 | RX_ENVT);
  NEXT   = LBL_ASSIGN;

  goto lbl_dispatch;

 lbl_if:
  save(RX_NEXT);
  TMP[0] = pop_list(1, &EXP);
  TMP[1] = pop_list(1, &EXP);
  TMP[2] = pop_list(1, &EXP);
  EXP    = TMP[0];
  NEXT   = LBL_DECIDE;
  save(RX_NEXT | RX_TMP1 | RX_TMP2);

  goto lbl_dispatch;

 lbl_mk_closure:
  TMP[0] = pop_list(1,&EXP);
  TMP[1] = pop_list(1,&EXP);
  VAL = mk_proc(TMP[1], TMP[2], ENVT, LOCALS, do_eval);

  goto lbl_next;

 lbl_fun:
  do_eval = true;

  goto lbl_mk_closure;

  
 lbl_macro:
  do_eval = false;

  goto lbl_mk_closure;

 lbl_do:
  save(RX_BODY | RX_NEXT);
  BODY = cdr_(EXP);

  goto lbl_seq_start;

 lbl_let:
  save(RX_BODY | RX_ENVT | RX_NEXT | RX_SP);
  // pop the let keyword
  TMP[0] = env_size(ENVT);
  ENVT   = mk_closure(ENVT,LOCALS);
  TMP[1] = pop_list(1,&EXP);
  TMP[2] = rvec_elcnt(TMP[1]);

  if (TMP[2] % 2)
    TMP[2] += 1;

  TMP[3] = TMP[2] / 2;

  SP     = BASE + TMP[3];
  exprs  = torvec(TMP[1])->rv_elements;

  // unpack the let bindings (use the eventual slots for the let locals as temporary storage
  // for the names)
  for (size_t i = 0; i < TMP[3]; i++)
    {
      LOCALS[i] = exprs[i*2];
      push(exprs[i*2+1]);
    }

  ENVT   = mk_let_env(LOCALS, TMP[3], ENVT);
  BODY   = cdr(EXP);
  NEXT   = LBL_SEQ_START;
  save(RX_BODY | RX_ENVT | RX_NEXT);

  goto lbl_let_args_loop;

 lbl_funapp:
  save(CONTINUATION);
  TMP[0] = car_(EXP);
  TMP[1] = cdr_(EXP);
  EXP    = TMP[0];
  NEXT   = LBL_OP_DONE;
  save(RX_TMP1 | NEXT);

  goto lbl_dispatch;

 lbl_op_done:
  restore(RX_TMP1 | NEXT);

  if 
}
