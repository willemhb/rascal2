#include <ctype.h>
#include "txtio.h"

/* text and io types */

/* basic string type */
str_t* new_str(chr_t* s) {
  str_t* out = (str_t*)vm_allocate(strlen(s) + 1);
  strcpy(out,s);
  return out;
}

/* port interface */
val_t open(val_t fnm, val_t md) {
  str_t* fname = tostr(fnm);
  str_t* mode = tostr(md);

  return tagp(vm_open(fname,mode));
}

val_t close(val_t p) {
  port_t* p_ = toport(p);
  int_t status = vm_close(p_);
  if (status == EOF) {
    escapef(IO_ERR,stderr,"EOF returned from fclose.");
  } else {
    return OK;
  }
}

val_t prn(val_t v, val_t p) {
  port_t* strm = toport(p);

  vm_prn(v,strm);

  return OK;
}

val_t reads(val_t p) {
  static chr_t buffer[512];
  port_t* port = toport(p);
  chr_t* out = fgets(buffer,512,port->stream);

  if (out == NULL) escapef(IO_ERR,stdout,"Failed to read string from port.");
  return tagp(new_str(out));
}

val_t load(val_t fname) {
  str_t* fnames = tostr(fname);
  port_t* p = vm_open(fnames,"r");
  return vm_load(p);
}

port_t* vm_open(str_t* fname, str_t* readmode) {
  FILE* f = fopen(fname,readmode);

  if (f == NULL) {
    escapef(IO_ERR,stderr,"Failed to open %s",fname);
  }
  
  port_t* obj = (port_t*)vm_allocate(24);

  type_(obj) = TYPECODE_PORT;
  stream_(obj) = f;
  fl_iotype(obj) = TEXT_PORT;  // worry about binary later

  if (streq(readmode,"r")) {
    fl_readable(obj) = 1;
    fl_writable(obj) = 0;
  } else if (streq(readmode,"w")) {
    fl_readable(obj) = 0;
    fl_writable(obj) = 1;
  } else {
    fl_readable(obj) = 1;
    fl_writable(obj) = 1;
  }

  for (int i = 0; i < 8; i++) {
    buffer_(obj)[i] = '\0';
  }

  return obj;
}

val_t _std_port(FILE* f) {
  port_t* obj = (port_t*)vm_allocate(24);

  type_(obj) = TYPECODE_PORT;
  stream_(obj) = f;
  fl_iotype(obj) = TEXT_PORT;  // worry about binary later

  if (f == stdin) {
    fl_readable(obj) = 1;
    fl_writable(obj) = 0;
  } else {
    fl_readable(obj) = 0;
    fl_writable(obj) = 1;
  }

  return tagp(obj);
}

int_t vm_close(port_t* p) {
  fl_writable(p) = 0;
  fl_readable(p) = 0;
  fflush(stream_(p));

  return fclose(stream_(p));
}

void prn_int(val_t v,port_t* p) {
  int_t i = toint(v);
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

void prn_type(val_t t,port_t* p) {
  vm_puts(p, "#type:");
  vm_puts(p, totype(t)->tp_name->name);
}

void prn_str(val_t s, port_t* p) {
  str_t* str = tostr_(s);
  vm_putc(p, '"');
  vm_puts(p, str);
  vm_putc(p, '"');
}

void prn_cons(val_t c, port_t* p) {
  vm_putc(p, '(');

  if (!islist(c)) {
    vm_prn(car_(c), p);
    vm_puts(p, " . ");
    vm_prn(cdr_(c), p);

  } else {
    int_t i;
    for (i = 0; i < MAX_LIST_ELEMENTS; i++) {
      if (isnil(c)) break;
      vm_prn(car_(c), p);
      c = cdr_(c);
      if (isnil(c)) break; // no space between the closing paren and the last element
      vm_putc(p, ' ');
    }

    if (i == MAX_LIST_ELEMENTS) {
      vm_puts(p, "...");
    }
  }

  vm_putc(p, ')');
}


void prn_proc(val_t pr,port_t* p) {
  chr_t* ptype;
  proc_t* proc = toproc_(pr);
  if (callmode_(proc) == CALLMODE_MACRO) ptype = "macro";
  else if (bodytype_(proc) == BODYTYPE_CFNC) ptype = "builtin";
  else ptype = "function";
  vm_puts(p, "#proc:");
  vm_puts(p, ptype);
  return;
}

void prn_sym(val_t s, port_t* p) {
  vm_puts(p,name_(s));
}

void vm_prn(val_t v, port_t* p) {
  if (!isopen(p)) {
    escapef(IO_ERR,stderr,"Printing to a closed port.");
  }
  
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
    type_of(v)->tp_prn(v,p);
    break;
  }

  vm_putc(p,'\n');
}

/* low level port api (calls to the underlying C api) */
chr_t vm_getc(port_t* p) {
  chr_t out = fgetc(stream_(p));
  return out;
}

int_t vm_putc(port_t* p, chr_t c) {
  int_t out = fputc(c,stream_(p));
  if (out == EOF) {
    escapef(IO_ERR,stderr,"EOF");
  }
  return out;
}

int_t vm_peekc(port_t* p) {
 chr_t next = fgetc(stream_(p));

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

bool vm_eof(port_t* p) {
  return feof(p->stream);
}



/* tokenizer */
static void accumtok(chr_t c) {
  if (TOKPTR == TOKBUFF_SIZE - 1) {
    escapef(IO_ERR,stderr, "Token too long.\n");
  } else {
    TOKBUFF[TOKPTR++] = c;
  }
}

r_tok_t get_token(port_t* p) {
  if (TOKTYPE != TOK_NONE) {
    return TOKTYPE;
  }
  
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

    accumtok('\0');
    return TOKTYPE;
  }


val_t read_expr(port_t* p) {
  r_tok_t t = get_token(p);
  int_t numval;

  switch (t) {
  case TOK_NUM:
    numval = atoi(TOKBUFF);
    take();
    return tagval(numval, (TYPECODE_INT << 3) | LOWTAG_DIRECT);

  case TOK_STR:
    take();
    return tagp(new_str(TOKBUFF));

  case TOK_SYM:
    take();
    return sym(TOKBUFF);

  case TOK_LPAR:
    take();
    return read_cons(p);

  case TOK_QUOT:
    take();
    // transform 'x to (quote x)
    return cons(sym("quote"), cons(read_expr(p),NIL));

  case TOK_DOT:
  case TOK_RPAR:
  case TOK_STXERR:
  default:
    escapef(SYNTAX_ERR,stderr, "%s", STXERR);

  case TOK_EOF:
    return NIL;
  }
}

val_t read_cons(port_t* p) {
  val_t out = NIL, curr;
  get_token(p);

  while (TOKTYPE != TOK_RPAR && TOKTYPE != TOK_DOT && TOKTYPE != TOK_STXERR) {
    curr = read_expr(p);
    append(&out,curr);
  }
  
  if (TOKTYPE == TOK_STXERR) escapef(SYNTAX_ERR,stderr,"%s",STXERR);
  if (TOKTYPE == TOK_DOT) {
    val_t* tail = &out;
    while (iscons(*tail)) tail = &cdr_(*tail);

    *tail = read_expr(p);
    if (get_token(p) != TOK_RPAR) escapef(SYNTAX_ERR,stderr,"Malformed dotted list.");
  }

  take(); // clear the right paren token
  return out;
}


/* core reader procedures */
val_t vm_read(port_t* p) {
  if (!(fl_readable(p) || fl_writable(p))) {
    return NONE;
  } else if (vm_eof(p)) {
    return NONE;
  } else {
    return read_expr(p);
  }
}

/* 
   for simplicity, right now load just reads a file wrapped in a 'do' expression and 
   evalutes it.
 
*/

val_t vm_load(port_t* p) {
  val_t out = cons(sym("do"),NIL);

  while (true) {
    val_t exp = vm_read(p);
    if (vm_eof(p)) break;     // avoid appending the 'None' returned when vm_read encounters EOF
    append(&out, exp);        //this can be written be better
  }

  return eval_expr(-1,out,NIL);
}
