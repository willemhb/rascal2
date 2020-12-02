#include <ctype.h>
#include "txtio.h"

/* text and io types */
port_t* vm_new_port(chr_t* fname, chr_t* readmode) {
  FILE* f = fopen(fname,readmode);
  uchr_t* obj = vm_allocate(24);
  port_t* pobj = (port_t*)obj;

  pobj->head.type = TYPECODE_PORT;
  pobj->stream = f;

  for (int i = 0; i < 8; i++) {
    pobj->chrbuff[i] = '\0';
  }

  // save a reference to the port to close unclosed files during cleanup 
  sym_t* interned = vm_new_gensym(fname,SYMFLAG_INTERNED);
  interned->binding = tagport(pobj);

  return pobj;
}

uint_t vm_port_sizeof(val_t v) {
  return 24;
}

bstr_t* vm_new_bstr(chr_t* chrs) {
  int_t nb = strlen(chrs);
  uchr_t* obj = vm_allocate(sizeof(bstr_t) + nb);
  bstr_t* bobj = (bstr_t*)obj;
  bobj->head.meta = nb;
  bobj->head.type = TYPECODE_BSTR;
  memcpy(bobj->chars,chrs,nb);

  return bobj;
}

uint_t vm_bstr_len(bstr_t* bs) {
  return bs->head.meta;
}

uint_t vm_bstr_sizeof(val_t bstr) {
  return 8 + vm_bstr_len(_tobstr(bstr));
}

void vm_prn_int(val_t v,port_t* p) {
  int_t i = _toint(v);
  if (!i) {
    vm_putc(p, '0');
    return;
  }

  if (i < 1) {
    vm_putc(p, '-');
    i *= -1;
  }

  chr_t digits[16];
  int_t c = 0;
  while (i) {
    digits[c] = i % 10;
    i /= 10;
    c++;
  }

  for (int_t j = c; j >= 0; j--) {
    vm_putc(p, digits[j]);
  }

  return;
}

void vm_prn_type(val_t t,port_t* p) {
  vm_puts(p, "#type:");
  vm_puts(p, _totype(t)->tp_name->name);
}

void vm_prn_bvec(val_t b, port_t* p) {
  vm_puts(p, "b\"");
  vm_puts(p, (chr_t*)_tobvec(b));
  vm_putc(p, '"');
  return;
}

void vm_prn_bstr(val_t s, port_t* p) {
  vm_putc(p, '"');
  vm_puts(p, _tobstr(s)->chars);
  vm_putc(p, '"');
}

void vm_prn_cons(val_t c, port_t* p) {
  vm_putc(p, '(');

  if (!islist(c)) {
    vm_prn(_car(c), p);
    vm_puts(p, " . ");
    vm_prn(_cdr(c), p);

  } else {
    int_t i;
    for (i = 0; i < MAX_LIST_ELEMENTS; i++) {
      if (isnil(c)) break;
      vm_prn(_car(c), p);
      c = _cdr(c);
      if (isnil(c)) break; // no space between the closing paren and the last element
      vm_putc(p, ' ');
    }

    if (i == MAX_LIST_ELEMENTS) {
      vm_puts(p, "...");
    }
  }

  vm_putc(p, ')');
}


void vm_prn_proc(val_t pr,port_t* p) {
  chr_t* ptype;
  proc_t* proc = _toproc(pr);
  if (callmode(proc) == CALLMODE_MACRO) ptype = "macro";
  else if (bodytype(proc) == BODYTYPE_CFNC) ptype = "builtin";
  else ptype = "function";
  vm_puts(p, "#proc:");
  vm_puts(p, ptype);
  return;
}

void vm_prn_sym(val_t s, port_t* p) {
  vm_puts(p,_tosym(s)->name);
}

void vm_prn(val_t v, port_t* p) {
  switch(typecode(v)) {
  case TYPECODE_NIL:
    vm_puts(p,"nil");
    break;
  case TYPECODE_NONE:
    vm_puts(p,"none");
    break;
  case TYPECODE_PORT:
    vm_puts(p, "#port");
    break;
  default:
    type(v)->tp_prn(v,p);
    break;
  }

  vm_putc(p,'\n');
}

/* low level port api (calls to the underlying C api) */
chr_t vm_getc(port_t* p) {
  return fgetc(p->stream);
}

int_t vm_putc(port_t* p, chr_t c) {
  return fputc(c, p->stream);
}

int_t vm_peekc(port_t* p) {
  int_t next = fgetc(p->stream);

  if (next == EOF) {
    return next;
  } else {
    ungetc(next, p->stream);
    return (chr_t)next;
  }
}

chr_t* vm_gets(port_t* p, int_t i) {
  uchr_t* buffer = vm_allocate(i + 1);
  return fgets((chr_t*)buffer, i + 1, p->stream);
}

int_t vm_puts(port_t* p, chr_t* s) {
  return fputs(s, p->stream);
}

int_t vm_close(port_t* p) {
  return fclose(p->stream);
}

bool vm_eof(port_t* p) {
  return feof(p->stream);
}


/* tokenizer */
static void accumtok(chr_t c) {
  if (TOKPTR == TOKBUFF_SIZE - 1) {
    eprintf(stderr, "Token too long.\n");
    escape(ERROR_OVERFLOW);
  } else {
    TOKBUFF[TOKPTR++] = c;
  }
}

static val_t vm_new_form(form_enum_t form, val_t body) {
  val_t f = tagsym(BUILTIN_FORMS[form]);
  cons_t* c = vm_new_list(f, NIL);
  
  if (isnil(body)) {
    // catch empty forms and raise errors
    eprintf(stderr, "Void form %s\n.", BUILTIN_FORMS[f]->name);
    escape(ERROR_SYNTAX);
  }

  vm_list_append(&c,body);

  return tagcons(c);
}

r_tok_t vm_get_token(port_t* p) {
  int_t ch;
  while ((ch = vm_getc(p)) != EOF) {
    if (isspace(ch)) continue;
    
    else if (ch == ';') {
      while ((ch = vm_getc(p)) != EOF && ch != '\n') continue;
      if (ch == EOF) return TOK_EOF;
    }

    break;
  }

  if (ch == EOF) {
    TOKTYPE = TOK_EOF;
    return TOKTYPE;
  } else switch (ch) {
    case '(':
      TOKTYPE = TOK_LPAR;
      break;
    case ')':
      TOKTYPE = TOK_RPAR;
      break;
    case '\'':
      TOKTYPE = TOK_QUOT;
      break;
    case '"':
      while ((ch = vm_getc(p)) != EOF && (ch != '"' || TOKBUFF[TOKPTR] == '\\')) {
	accumtok(ch);
      }
      if (ch == EOF) {
	TOKTYPE = TOK_STXERR;
	sprintf(STXERR, "Unexpected end of string");
      } else {
	accumtok('\0');
	TOKTYPE = TOK_STR;
      }
      break;
    case '.':
      TOKTYPE = TOK_DOT;
      break;
    default:
      while ((ch = vm_getc(p)) != EOF && ch != ';' && ch != ')' && ch != '(') {
	accumtok(ch);
      }
      if (ch == ';' || ch == ')' || ch == '(') {
	TOKTYPE = TOK_STXERR;
	sprintf(STXERR, "Illegal literal character.");
      } else {
	int_t i;
	for (i = 0; i < TOKPTR; i++) {
	  if (!isdigit(TOKBUFF[i])) {
	    if ((TOKBUFF[i] == '+' || TOKBUFF[i] == '-') && i == 0 && strlen(TOKBUFF) > 1) {
	      continue;
	    } else {
	      break;
	    }
	  }
	}

	TOKTYPE = i == TOKPTR ? TOK_NUM : TOK_SYM;
      }
      break;
   }
    return TOKTYPE;
  }

val_t vm_read_expr(port_t* p) {
  r_tok_t t = vm_get_token(p);
  int_t numval;
  
  switch (t) {
  case TOK_NUM:
    numval = atoi(TOKBUFF);
    return tagval(numval, (TYPECODE_INT << 3) | LOWTAG_DIRECT);

  case TOK_STR:
    return tagstr(vm_new_bstr(TOKBUFF));

  case TOK_SYM:
    return tagsym(vm_intern_str(TOKBUFF,&GLOBALS));

  case TOK_LPAR:
    return tagcons(vm_read_sexpr(p));

  case TOK_QUOT:
    // transform 'x to (quote x)
    return vm_new_form(QUOTE_F, vm_read_expr());
  case TOK_STXERR:
    eprintf(stderr, "Exiting due to syntax error: %s.\n", STXERR);
    escape(ERROR_SYNTAX);

  case TOK_EOF:
    return TOK_DONE;

  }
}

/* core reader procedures */
val_t vm_read(port_t*);
val_t vm_read_literal(port_t*);
val_t vm_read_sexpr(port_t*);

/* 
   for simplicity, right now load just reads a file wrapped in a 'do' expression and 
   evalutes it.
 
*/
void vm_load(port_t* p) {
  cons_t* out = vm_new_list(tagsym(BUILTIN_FORMS[EV_DO]), NIL);
  while (!vm_eof(p)) {
    val_t exp = vm_read_expr(p);
    vm_list_append(&out, exp);
  }

  // call vm_eval here
}

void vm_repl(void);
