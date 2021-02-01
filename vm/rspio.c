#include "rascal.h"



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



void prn_str(val_t s, FILE* f)
{
  fputwc('"',f);
  fputs(tostr_(s),f);
  fputwc('"',f);
}


void prn_sym(val_t s, FILE* f) {
  fputs(symname_(s),f);
  return;
}


static void tbl_prn_traverse(node_t* n, FILE* f)
{
  if (!n) return;

  vm_print(hashkey_(n),f);
  if (fieldcnt_(n))
    {
      fputs(" => ",f);
      vm_print(bindings_(n)[0],f);
    }

  if (left_(n))
    {
      fputwc(' ',f);
      tbl_prn_traverse(left_(n),f);
    }

  if (n->right)
    {
      fputwc(' ',f);
      tbl_prn_traverse(left_(n),f);
    }

  return;
}

void prn_table(val_t t, FILE* f)
{
  fputwc('{',f);
  tbl_prn_traverse(records_(t),f);
  fputwc('}',f);
}

void prn_bool(val_t b, FILE* f)
{
  if (b == R_TRUE) fputwc('t',f);
  else fputwc('f',f);

  return;
}

void prn_char(val_t c, FILE* f)
{
  fputwc('\\',f);
  fputwc(tochar_(c),f);
  return;
}


void prn_dvec(val_t v, FILE* f)
{
  fputs("#d[",f);
  val_t* el = dvec_elements_(v);
  int elcnt = dvec_allocated_(v);

  for (int i = 0; i < elcnt; i++)
    {
      vm_print(el[i],f);
      if (elcnt - i > 1) fputwc(' ', f);
    }

  fputwc(']',f);
  return;
}

void prn_int(val_t i, FILE* f)
{
  fprintf(f,"%d",toint_(i));
  return;
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
	return vm_mk_cons(F_QUOTE,tl);
      }
    case TOK_INT:
      {
	iexpr = atoi(TOKBUFF);
	expr = vm_mk_int(iexpr);
	take();
	return expr;
      }
    case TOK_FLOAT:
      {
	fexpr = atof(TOKBUFF);
	expr = vm_mk_float(fexpr);
	take();
	return expr;
      }
    case TOK_CHAR:
      {
	iexpr = nextu8(TOKBUFF);
	expr = vm_mk_char(iexpr);
	take();
	return expr;
      }
    case TOK_UNICODE:
      {
	iexpr = atoi(TOKBUFF);
	expr = vm_mk_char(iexpr);
	take();
	return expr;
      }
    case TOK_STR:
      {
	expr = vm_mk_str(TOKBUFF);
	take();
	return expr;
      }
    case TOK_SYM:
      {
	expr = vm_mk_sym(TOKBUFF,0);
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
  val_t expr;
  SAVESP;                      // save the current stack pointer

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
      val_t ls = vm_mk_list(cnt);
      init_list(ls,EVAL,cnt);
      RESTORESP;               // restore the saved stack pointer
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

      while (SP != __OSP__)
	{
	  ca = POP();
	  cd = vm_mk_cons(ca,cd);
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
      type_t* to = val_type(v);
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
