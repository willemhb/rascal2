#ifndef txtio_h

/* begin txtio.h */
#define txtio_h

#include "common.h"
#include "obj.h"
#include "cons.h"
#include "env.h"
#include "mem.h"
#include "eval.h"

/* 
   this module contains the API for builtin text types (str, bytv) and,
   semi-relatedly, the core of the reader.

 */

/* interface to builtin text and io types */
port_t* vm_new_port(chr_t*,chr_t*);
port_t* _vm_new_port(FILE*);               // private function for initializing stdin, stdout
uint_t vm_port_sizeof(val_t);              // etc
bstr_t* vm_new_bstr(chr_t*);
uint_t vm_bstr_len(bstr_t*);
uint_t vm_bstr_sizeof(val_t);

/* printers for builtin types */

#define MAX_LIST_ELEMENTS 100

void vm_prn_int(val_t,port_t*);
void vm_pn_type(val_t,port_t*);
void vm_prn_bstr(val_t,port_t*);
void vm_prn_cons(val_t,port_t*);
void vm_prn_sym(val_t,port_t*);
void vm_prn_proc(val_t,port_t*);

/* low level port api */
chr_t vm_getc(port_t*);
int_t vm_putc(port_t*, chr_t);
int_t vm_peekc(port_t*);
int_t vm_puts(port_t*, chr_t*);
chr_t* vm_gets(port_t*,int_t);
port_t* vm_open(chr_t*, chr_t*);
int vm_close(port_t*);
bool vm_eof(port_t*);


/* tokenizing */
typedef enum _r_tok_t {
  TOK_LPAR,
  TOK_RPAR,
  TOK_DOT,
  TOK_QUOT,
  TOK_NUM,
  TOK_STR,
  TOK_SYM,
  TOK_EOF,
  TOK_DONE,
  TOK_STXERR,
} r_tok_t;

#define TOKBUFF_SIZE 1024
chr_t TOKBUFF[TOKBUFF_SIZE];
chr_t STXERR[512];
int_t TOKPTR = 0;
r_tok_t TOKTYPE;
r_tok_t vm_get_toktype(port_t*);
r_tok_t vm_get_token(port_t*);

val_t vm_read_expr(port_t*);
cons_t* vm_read_sexpr(port_t*);
val_t vm_read_str(port_t*);
val_t vm_read_literal(port_t*);

/* core reader procedures */
void vm_prn(val_t,port_t*);
val_t vm_read(port_t*);
void vm_load(port_t*);
void vm_repl(void);
/* end txtio.h */
#endif
