#include "rascal.h"

/* text and io types */

/* port accessors */

FILE* stream(val_t p) {
  port_t* pp = toport(p);
  if (isopen(pp)) return pp->stream;
  else escapef(IO_ERR,stderr,"Tried to access a closed io stream.");
}


/* port interface */


/* 
   core IO functions 

   open  -  open a file and create a port
   close -  close a port's underlying file
   read  -  read an S-expression from the file (reads to eof)
   readc -  read the next UTF-8 character from the file
   reads -  read the characters from the stream as a UTF-8 stream (creates a new string).
   peekc -  check the next character in the stream
   prn   -  write an S-expression to the stream
   prnc  -  write a UTF-8 character to the file
   prns  -  write the argument as a plain character string
   load  -  read a file as rascal code and execute it

   other IO functions
   
   eof?  - checks the end of file indicator

*/

val_t r_open(val_t fnm, val_t md) {
  str_t* fname = tostr(fnm);
  str_t* mode = tostr(md);

  return _vm_open(fname,mode);
}

// the functions below are used in bootstrapping to create important ports passing
// strings directly
val_t _vm_open(str_t* fname, str_t* mode) {
    FILE* f = fopen(fname,mode);

  if (f == NULL) {
    escapef(IO_ERR,stderr,"Failed to open %s",fname);
  }

  port_t* obj = (port_t*)vm_allocate(sizeof(port_t));

  type_(obj) = TYPECODE_PORT;
  stream_(obj) = f;
  fl_iotype_(obj) = TEXT_PORT;  // worry about binary later

  if (streq(mode,"r")) {
    fl_readable_(obj) = 1;
    fl_writable_(obj) = 0;
  } else if (streq(mode,"w")) {
    fl_readable_(obj) = 0;
    fl_writable_(obj) = 1;
  } else {
    fl_readable_(obj) = 1;
    fl_writable_(obj) = 1;
  }

  return tagp(obj);
}

val_t _vm_std_port(FILE* f) {
    port_t* obj = (port_t*)vm_allocate(24);

  type_(obj) = TYPECODE_PORT;
  stream_(obj) = f;
  fl_iotype_(obj) = TEXT_PORT;  // worry about binary later

  if (f == stdin) {
    fl_readable_(obj) = 1;
    fl_writable_(obj) = 0;
  } else {
    fl_readable_(obj) = 0;
    fl_writable_(obj) = 1;
  }

  return tagp(obj);
}

val_t r_close(val_t p) {
  FILE* f = stream(p);
  int_t status = fclose(f);
  if (status == EOF) {
    escapef(IO_ERR,stderr,"EOF returned from fclose.");
  } else {
    fl_writable_(p) = 0;
    fl_readable_(p) = 0;
    
    return OK;
  }
}


val_t r_wrt(val_t v, val_t p) {
  FILE* strm = stream(p);
  chr_t buffer[5];
  chr_t* ptype;

  switch(typecode(v)) {
  case TYPECODE_NIL:
    fputs("nil",strm);
    break;

  case TYPECODE_NONE:
    fputs("none",strm);
    break;
 
  case TYPECODE_PORT:
    fputs("#port",strm);
    break;
 
  case TYPECODE_SYM:
    fputs(name(v),strm);
    break;

  case TYPECODE_STR:
    fputc('"', strm);
    fputs(tostr(v), strm);
    fputc('"', strm);
    break;
 
  case TYPECODE_CONS:
      fputc('(',strm);
      int_t i;

      for (i = 0; i < MAX_LIST_ELEMENTS; i++) {

	if (isnil(v)) break;

	r_wrt(car(v), p);
	v = cdr(v);
  
	if (isnil(v)) { // no space between the closing paren and the last element.
	  break;
	} else if (!iscons(v)) { // catch dotted lists
	  fputs(" . ",strm);
	  r_wrt(v,p);
	  break;
	} else {
	  fputc(' ',strm);
	}
      }

      if (i == MAX_LIST_ELEMENTS) {
	fputs("...",strm);
      }

      fputc(')',strm);
      break;

  case TYPECODE_DICT:
    fputs("#d(",strm);
    r_wrt(key_(v),p);
    fputs(" => ",strm);
    r_wrt(binding_(v),p);

    if (left_(v) != NIL) {
      r_wrt(left_(v),p);
    }

    if (right_(v) != NIL) {
      r_wrt(right_(v),p);
    }

    fputs(")",strm);
    break;
 
  case TYPECODE_PROC:
    if (callmode_(v) == CALLMODE_MACRO) ptype = "macro";
    else if (bodytype_(v) == BODYTYPE_CFNC) ptype = "builtin";
    else ptype = "function";
    fputs("#proc:",strm);
    fputs(ptype,strm);
    break;
 
  case TYPECODE_UCP:
    wctomb(buffer,val(v));
    fputc('\\',strm);
    fputs(buffer,strm);
    break;

  case TYPECODE_INT:
    fprintf(stream(p),"%d",toint(v));
    break;

  case TYPECODE_TYPE:
    fputs("#type:",strm);
    fputs(totype(v)->tp_name->name, strm);
    break;

  default:
    type_of(v)->tp_prn(v,p);
    break;
  }

  return OK;
}

val_t r_prn(val_t v, val_t p) {
  FILE* strm = stream(p);
  chr_t buffer[5];

  switch (typecode(v)) {
  case TYPECODE_UCP:
    wctomb(buffer,val(v));
    fputs(buffer,strm);
    break;

  case TYPECODE_STR:
    fputs(tostr(v),strm);
    break;

  default:
    r_wrt(v,p);
    break;
  }

  return OK;
}

val_t r_eof(val_t p) {
  return feof(stream(p));
}

val_t r_read(val_t p) {
  FILE* f = stream(p);
  val_t out = read_expr(f);
  return out;
}

val_t r_readc(val_t p) {
  FILE* f = stream(p);
  int_t ch = fgetuc(f);

  return ch == EOF ? NIL : tagv(((ucp_t)ch));
}

val_t r_peekc(val_t p) {
  FILE* f = stream(p);
  int_t ch = peekuc(f);

  return ch == EOF ? NIL : tagv(((ucp_t)ch));
}

val_t r_reads(val_t p) {
  static chr_t buffer[512];
  chr_t* out = fgets(buffer,512,stream(p));

  if (out == NULL) escapef(IO_ERR,stdout,"Failed to read string from port.");
  return tagp(vm_str(out));
}

val_t r_load(val_t fname) {
  str_t* fns = tostr(fname);
  FILE* f = fopen(fns,"r");

  if (f == NULL) {
    escapef(IO_ERR,stderr,"Couldn't open file %s", fns);
  }

  push(NIL);
  val_t* module = &STACK[SP - 1];

  while (true) {
    val_t exp = read_expr(f);
    if (feof(f)) break;     // avoid appending 'None'
    append(module,exp);
  }
  
  fclose(f);

  val_t exprs = pop();
  
  return eval_expr(cons(sym("do"),exprs),ROOT_ENV);
}


static void take() {
  // reset the token
  TOKTYPE = TOK_NONE;
}

chr_t* toktype_name(r_tok_t toktype) {
  static chr_t* toktype_names[] = {
    "LPAR", "RPAR", "DOT", "QUOT",
    "NUM", "STR", "SYM", "UCP",
    "EOF", "NONE", "STXERR",
  };

  return toktype_names[toktype];
}

/* tokenizer */
static void accumtok(ucp_t c) {
  static chr_t ucpb[5] = {0, 0, 0, 0, 0};
  wctomb(ucpb,c);
  if (TOKPTR + mblen(ucpb,4) >= (TOKPTR - 1)) {
    escapef(IO_ERR,stderr, "Token too long.\n");
  } else {
    strncpy(TOKBUFF + TOKPTR,ucpb,mblen(ucpb,4));
    TOKPTR += mblen(ucpb,4);
  }
}

r_tok_t get_token(FILE* f) {
  if (TOKTYPE != TOK_NONE) {
    return TOKTYPE;
  }

  TOKPTR = 0;
  ucp_t ch;
  int_t c;
  while ((c = peekuc(f)) != EOF) {
    ch = fgetuc(f);
    if (isspace(ch)) {
      continue;
    } else if (ch == ';') {
      while ((c = fgetuc(f)) != EOF && c != '\n') {
	continue;
      }

      if (c == EOF) return TOK_EOF;
      else continue;
    } else {
      break; 
    }
  }
  ch = (ucp_t)c;
  if (ch == EOF) {
    TOKTYPE = TOK_EOF;
    return TOKTYPE;
  } else switch (ch) {
    case '(':
      TOKTYPE = TOK_LPAR;
      return TOKTYPE;
    case ')':
      TOKTYPE = TOK_RPAR;
      return TOKTYPE;
    case '\'':
      TOKTYPE = TOK_QUOT;
      return TOKTYPE;
    
    case '"':
      while ((c = fgetuc(f)) != EOF && (c != '"' || TOKBUFF[TOKPTR] == '\\')) {
	ch = (ucp_t)c;
	accumtok(ch);
      }
      if (ch == EOF) {
	TOKTYPE = TOK_STXERR;
	sprintf(STXERR, "Unexpected end of string");
      } else {
	TOKTYPE = TOK_STR;
      }
      accumtok('\0');
      return TOKTYPE;

    case '.':
      TOKTYPE = TOK_DOT;
      return TOKTYPE;
    case '\\':
      c = fgetuc(f);
      if (c == EOF) {
	TOKTYPE = TOK_STXERR;
	sprintf(STXERR,"Character literal at end of file.");
      } else {
	TOKTYPE = TOK_UCP;
	ch = (ucp_t)c;
	accumtok(ch);
	accumtok('\0');
      }
      break;
    default:
      accumtok((ucp_t)c);
      while ((c = peekuc(f)) != EOF && c != ')' && !isspace(ch)) {
	ch = fgetuc(f);
	accumtok(ch);
	}

      accumtok('\0');

	TOKTYPE = TOK_NUM;
	for (int_t i = 0; TOKBUFF[i] != '\0'; i++) {
	  if (!isdigit(TOKBUFF[i])) {
	    if ((TOKBUFF[i] == '+' || TOKBUFF[i] == '-') && i == 0 && strlen(TOKBUFF) > 1) {
	      continue;
	    } else {
	      TOKTYPE = TOK_SYM;
	      break;
	    }
	  }
	}
	break;
    }

  return TOKTYPE;
}


val_t read_expr(FILE* f) {
  r_tok_t t = get_token(f);
  int_t numval; ucp_t cval;

  switch (t) {
  case TOK_UCP:
    take();
    mbtowc(&cval,TOKBUFF,4);
    return tagv(cval);

  case TOK_NUM:
    numval = atoi(TOKBUFF);
    take();
    return tagv(numval);

  case TOK_STR:
    take();
    return tagp(vm_str(TOKBUFF));

  case TOK_SYM:
    take();
    return sym(TOKBUFF);

  case TOK_LPAR:
    take();
    return read_cons(f);

  case TOK_QUOT:
    take();
    // transform 'x to (quote x)
    return cons(sym("quote"), cons(read_expr(f),NIL));

  case TOK_DOT:
  case TOK_RPAR:
  case TOK_STXERR:
  default:
    sprintf(STXERR, "%s", TOKBUFF);
    escapef(SYNTAX_ERR,stderr,"%s : TOKTYPE: %s",STXERR,toktype_name(TOKTYPE));

  case TOK_EOF:
    return NIL;
  }
}

val_t read_cons(FILE* f) {
  push(NIL); // save the result on the stack to ensure it's garbage collected
  val_t* out = &STACK[SP-1];
  val_t x;                     
  r_tok_t t;

  while ((t = get_token(f)) != TOK_RPAR && t != TOK_DOT && t != TOK_STXERR) {  
    x = read_expr(f);    
    append(out,x);
  }

  if (TOKTYPE == TOK_STXERR) escapef(SYNTAX_ERR,stderr,"%s",STXERR);
  if (TOKTYPE == TOK_DOT) {
    
    take();  // clear the dot token
    x = read_expr(f);
    append_i(out,x);
    }

  if (get_token(f) != TOK_RPAR) escapef(SYNTAX_ERR,stderr,"bad dotted list.");

  take(); // clear the right paren tokne
  return pop(); // return the head of the list
}
