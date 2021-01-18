#include "rascal.h"

/*
  virtual machine, evaluator, and function call APIs.
*/

// vm builtins and API
static rsp_label_t vm_analyze(val_t exp)
{
  switch (ltag(exp))
    {
    default: return LBL_LITERAL;
    case LTAG_SYM: return LBL_SYMBOL;
    case LTAG_LIST:
      {
	if (!exp) return LBL_LITERAL;
	val_t f = first_(exp);

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

val_t rsp_eval(val_t exp, val_t env)
{
  static void* ev_labels[] = {
    &&ev_literal, &&ev_symbol, &&ev_quote,
    &&ev_def, &&ev_setv, &&ev_fun, &&ev_macro,
    &&ev_let, &&ev_do, &&ev_if, &&ev_funapp,
  };

  val_t* a, *b, *c, *n, *names, *exprs, *fun, *args, *appenv, x, test, localenv, rslt;
  bool evargs;
  size_t bltnargco;
  val_t OSP = SP;            // save the original stack pointer (restored on return)
  val_t* envp = SAVE(env);

  goto *ev_labels[vm_analyze(exp)];

 ev_return:
  SP = OSP;     // restore stack state
  return rslt;

 ev_literal:
  rslt = exp;

  goto ev_return;

 ev_symbol:
  rslt = iskeyword(exp) ? exp : env_lookup(*envp,exp);

  goto ev_return;

 ev_quote:
  rslt = car(cdr(exp));

  goto ev_return;

 ev_def:
  n = SAVE(car(cdr(exp)));
  b = SAVE(car(cdr(cdr(exp))));
  env_extend(*envp,*n);
  c = SAVE(rsp_eval(*b,*envp));
  rslt = env_assign(*envp,*n,*c);

  goto ev_return;

 ev_setv:
  n = SAVE(car(cdr(exp)));
  b = SAVE(car(cdr(cdr(exp))));
  c = SAVE(rsp_eval(*b,*envp));
  rslt = env_assign(*envp,*n,*c);

  goto ev_return;

 ev_fun:
  n = SAVE(car(cdr(exp)));
  exprs = SAVE(cdr(cdr(exp)));
  rslt = mk_function(*exprs,*n,*envp,0);

  goto ev_return;

 ev_macro:
  n = SAVE(car(cdr(exp)));
  exprs = SAVE(cdr(cdr(exp)));
  rslt = mk_function(*exprs,*n,*envp,FUNCFLAG_MACRO);

  goto ev_return;

 ev_let:
  names = SAVE(car(cdr(exp)));
  exprs = SAVE(cdr(cdr(exp)));
  localenv = mk_envt(R_NIL,NULL);
  *envp = mk_envframe(localenv,*envp,0);

  // bind the names and arguments in the environment
  while (*names)
    {
      env_extend(*envp,car(*names));
      *names = cdr(*names);
      x = *names ? rsp_eval(car(*names),*envp) : R_UNBOUND;
      vec_append(x,*envp);
      if (*names) *names = cdr(*names);
    }

  rslt = rsp_evlis(*exprs,*envp);

  goto ev_return;

 ev_do:
  rslt = rsp_evlis(cdr(exp),*envp);

  goto ev_return;

 ev_if:
  exprs = SAVE(cdr(exp));

  while (*exprs)
    {
      
      if (isnil(cdr(*exprs))) break;

      else
	{
	  x = car(*exprs);
	  *exprs = cdr(*exprs);
	  test = rsp_eval(x,*envp);

	  if (cbool(test)) break;
	  else *exprs = cdr(*exprs);
	}
    }

  rslt = *exprs ? rsp_eval(car(*exprs),*envp) : R_NIL;

  goto ev_return;

 ev_funapp:
  fun = SAVE(rsp_eval(car(exp),*envp));
  args = SAVE(cdr(exp));
  if (isbuiltin(*fun)) goto ev_builtin;
  appenv = SAVE(mk_envframe(envt_(*fun),*envp,fnargco_(*fun)));
  evargs = !ismacro(*fun);

  for (;*args;*args=cdr(*args))
    {
      if (evargs) vec_append(*appenv,rsp_eval(car(*args),*envp));
      else vec_append(*appenv,car(*args));
    }

  rslt = rsp_evlis(template_(*fun),*appenv);
  if (ismacro(*fun)) rslt = rsp_eval(rslt,*envp);

  goto ev_return;

 ev_builtin:
  for (bltnargco = 0;*args;*args=cdr(*args),bltnargco++)
    {
      x = rsp_eval(car(*args),*envp);
      PUSH(x);
    }
  rsp_cfunc_t bltn = tobuiltin_(*fun);
  rslt = bltn(EVAL,bltnargco);
  
  goto ev_return;
}

val_t rsp_evlis(val_t exprs, val_t env)
{
  val_t curr;
  val_t* envp = SAVE(env);
  val_t* seq  = SAVE(exprs);

  while (*seq)
    {
      curr = rsp_eval(car(*seq),*envp);
      *seq = cdr(*seq);
    }

  POPN(2);
  return curr;
}

// a mediocre implementation apply
val_t rsp_apply(val_t fun, val_t argls)
{
  val_t OSP = SP;
  val_t* sfun = SAVE(fun);
  val_t* sargls = SAVE(argls);
  val_t* cls, *frm, x, rslt;
  size_t bltnargco; bool evargs;

  if (isbuiltin(*sfun)) goto ev_builtin;
  else goto ev_funapp;

 
  
 ev_funapp:
  cls = SAVE(closure_(*sfun));
  frm = SAVE(mk_envtframe(envt_(*sfun),*cls,fnargco_(*sfun)));
  evargs = !ismacro(*sfun);

  for (;*sargls;*sargls=cdr(*sargls))
    {
      x = car(*sargls);
      if (evargs) x = rsp_eval(x,*cls);
      vec_append(*frm,x);
    }

  rslt = rsp_evlis(template_(*sfun),*frm);
  if (evargs) rslt = rsp_eval(rslt,*cls);

  goto ev_return;

 ev_builtin:
  for (bltnargco = 0;*sargls;*sargls=cdr(*sargls),bltnargco++)
    {
      x = rsp_eval(car(*sargls),R_TOPLEVEL);
      PUSH(x);
    }

  rsp_cfunc_t bltn = tobuiltin_(*sfun);
  rslt = bltn(EVAL,bltnargco);

  goto ev_return;

 ev_return:
  SP = OSP;
  return rslt;
}

// reader support functions
#define TOKBUFF_SIZE 4096
char TOKBUFF[TOKBUFF_SIZE];
int TOKPTR = 0;
int LASTCHR;                  // to keep track of escapes
rsp_tok_t TOKTYPE = TOK_NONE;

static int accumtok(wint_t c)
{
  if (TOKBUFF_SIZE < (TOKPTR + 4))
    {
      rsp_perror(BOUNDS_ERR,"Token too long.");
      rsp_raise(BOUNDS_ERR);
      return -1;
    }

  else
    {
      int csz = wctomb(TOKBUFF+TOKPTR,c);
      TOKPTR += csz;
      return csz;
    }
}

// reset reader state
static inline void take() { TOKTYPE = TOK_NONE ; TOKPTR = 0; return; }

static inline rsp_tok_t finalize_void_tok(rsp_tok_t tt)
{
  TOKTYPE = tt;
  return tt;
}

static inline rsp_tok_t finalize_tok(wint_t c, rsp_tok_t tt)
{
  TOKTYPE = tt;
  accumtok(c);
  if (c != U'\0') TOKBUFF[++TOKPTR] = U'\0';
  return tt;
}

static rsp_tok_t get_char_tok(FILE* f)
{
  wint_t wc = peekwc(f);
  if (wc == WEOF)
    {
      TOKTYPE = TOK_STXERR;
      rsp_perror(SYNTAX_ERR,"Unexpected EOF reading character.");
      rsp_raise(SYNTAX_ERR);
    }

  if (wc != U'u' && wc != U'U') return finalize_tok(wc,TOK_CHAR);

  else while ((wc = peekwc(f)) != WEOF && !iswspace(wc))
	 {
	   if (!iswdigit(wc))
	     {
	       TOKTYPE = TOK_STXERR;
	       rsp_perror(SYNTAX_ERR,"Non-numeric character in codepoint literal.");
	       rsp_raise(SYNTAX_ERR);
	     }
	   accumtok(wc);
	 }

  if (wc == WEOF && TOKPTR == 0)
    {
	  TOKTYPE = TOK_STXERR;
	  rsp_perror(SYNTAX_ERR,"Unexpected EOF reading character.");
	  rsp_raise(SYNTAX_ERR);
    }

      return finalize_tok(U'\0',TOK_UNICODE);
    }

static rsp_tok_t get_str_tok(FILE* f)
{
  wint_t wc; LASTCHR = '"';

  while ((wc = peekwc(f)) != WEOF && !(wc == '"' && LASTCHR != '\\'))
    {
      accumtok(wc);
    }

  if (wc == WEOF)
    {
	  TOKTYPE = TOK_STXERR;
	  rsp_perror(SYNTAX_ERR,"Unexpected EOF reading character.");
	  rsp_raise(SYNTAX_ERR);
    }

  return finalize_tok(U'\0',TOK_STR);
}

static rsp_tok_t get_symbolic_tok(FILE* f, wint_t fc)
{
  bool hasdot = fc == U'.';
  bool hasint = iswdigit(fc) || fc == U'+' || fc == U'-';
  bool hasfloat = iswdigit(fc) || fc == U'+' || fc == U'-' || hasdot;
  int wc;

  while ((wc = peekwc(f)) != EOF && !iswspace(wc) && wc != U')')
    {
      if (!iswdigit(wc))
	{
	  hasint = false;
	  hasfloat = wc == U'.' && !hasdot;
	  hasdot = true;
	}

      accumtok(wc);
    }

  if (hasint) return finalize_tok(U'\0',TOK_INT);
  else if (hasfloat) return finalize_tok(U'\0',TOK_FLOAT);
  else return finalize_tok(U'\0',TOK_SYM);
}

rsp_tok_t vm_get_token(FILE* f)
{
  if (TOKTYPE != TOK_NONE) return TOKTYPE;

  wint_t wc = fskip(f); // skip initial whitespace
  if (wc == U';')       // skip comments as well
    {
      wc = fskipto(f, U'\n');
    }

  if (wc == WEOF)
    {
      TOKTYPE = TOK_EOF;
      return TOKTYPE;
    }

  switch (wc)
    {
    case U'(' : return finalize_void_tok(TOK_LPAR);
    case U')' : return finalize_void_tok(TOK_RPAR);
    case U'.' : return finalize_void_tok(TOK_DOT);
    case U'\'': return finalize_void_tok(TOK_QUOT);
    case U'\\': return get_char_tok(f);
    case U'\"' : return get_str_tok(f);
    default:
      {
	accumtok(wc);
	return get_symbolic_tok(f,wc);
      }
    }
}

val_t vm_read_expr(FILE* f)
{
  rsp_tok_t tt = vm_get_token(f);
  val_t expr, tl; int iexpr; float fexpr;

  switch (tt)
    {
    case TOK_LPAR:
      {
	take();
	return vm_read_cons(f);
      }
    case TOK_QUOT:
      {
	take();
	tl = vm_read_cons(f);
	return vm_cons(F_QUOTE,tl);
      }
    case TOK_INT:
      {
	iexpr = atoi(TOKBUFF);
	expr = mk_int(iexpr);
	take();
	return expr;
      }
    case TOK_FLOAT:
      {
	fexpr = atof(TOKBUFF);
	expr = mk_float(fexpr);
	take();
	return expr;
      }
    case TOK_CHAR:
      {
	iexpr = nextu8(TOKBUFF);
	expr = mk_char(iexpr);
	take();
	return expr;
      }
    case TOK_UNICODE:
      {
	iexpr = atoi(TOKBUFF);
	expr = mk_char(iexpr);
	take();
	return expr;
      }
    case TOK_STR:
      {
	expr = mk_str(TOKBUFF);
	take();
	return expr;
      }
    case TOK_SYM:
      {
	expr = mk_sym(TOKBUFF,0);
	take();
	return expr;
      }
    default:
      {
	take();
	rsp_raise(SYNTAX_ERR);
	return R_NONE;
      }
    }
}

val_t vm_read_cons(FILE* f)
{
  size_t cnt = 0; rsp_tok_t c;
  val_t expr, OSP = SP;

  while ((c = vm_get_token(f)) != TOK_RPAR && c != TOK_DOT)
    {
      if (c == TOK_EOF)
	{
	  take();
	  rsp_perror(SYNTAX_ERR,"Unexpected EOF reading cons");
	  rsp_raise(SYNTAX_ERR);
	}

      expr = vm_read_expr(f);
      PUSH(expr);
      cnt++;
    }

  if (c == TOK_RPAR)
    {
      take();
      val_t ls = mk_list(cnt);
      init_list(ls,EVAL,cnt);
      SP = OSP;
      return ls;
    }

  else // read the last expression, then begin consing up the dotted list
    {
      take();
      expr = vm_read_expr(f);
      c = vm_get_token(f);
      take();

      if (c != TOK_RPAR) rsp_raise(SYNTAX_ERR);

      val_t ca, cd = expr;

      while (SP != OSP)
	{
	  ca = POP();
	  cd = vm_cons(ca,cd);
	}

      return cd;
    }
}

void vm_print(val_t v, FILE* f)
{
  if (v == R_NIL)
    {
      fputs("nil",f);
      return;
    }
  
  else if (v == R_NONE)
    {
      fputs("none",f);
      return;
    }
  
  else if (isdirect(v))
    {
      fprintf(f,"#<builtin:%s>",BUILTIN_FUNCTION_NAMES[unpad(v)]);
      return;
    }

  else
    {
      type_t* to = get_val_type(v);
      if (to->tp_prn)
	{
	  to->tp_prn(v,f);
	  return;
	}
      else
	{
	  vm_print_val(v,f);
	  return;
	}
    }
}

void vm_print_val(val_t v, FILE* f) // fallback printer
{
  char* tn = get_val_type_name(v);
  if (isdirect(v)) fprintf(f,"#<%s:%u>",tn,(unsigned)unpad(v));
  else fprintf(f,"#<%s:%p>",tn,toobj_(v));
  return;
}

val_t vm_load(FILE* f,val_t e)
{
  val_t OSP = SP;
  val_t *envp = SAVE(e);
  val_t *base = (&EVAL[SP+1]);
  size_t cnt = 0;

  while (!feof(f))
    {
      val_t expr = vm_read_expr(f);
      PUSH(expr);
      cnt++;
    }

  // evaluate the saved expressions in the order they were read
  for (size_t i = 0; i < cnt; i++)
    {
      rsp_eval(base[i],*envp);
    }

  // restore the stack pointer and return
  SP = OSP;
  return R_NIL;
}
