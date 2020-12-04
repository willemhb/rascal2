#ifndef txtio_h

/* begin txtio.h */
#define txtio_h

#include "common.h"
#include "obj.h"
#include "env.h"
#include "eval.h"

/* 
   this module contains the API for builtin text types (str, bytv) and,
   semi-relatedly, the core of the reader.

 */

#define stream_(p) FAST_ACCESSOR_MACRO(p,port_t*,stream)
#define buffer_(p) FAST_ACCESSOR_MACRO(p,port_t*,chrbuff)

/* 
   interface to builtin text and io types 

   the functions below represent the high level io interface. Open and close
   create and clean up ports, read gets s-expressions from a port, and prn writes
   s-expressions to ports. 

   The low level interface makes calls to the underlying C FILE* API, mostly using
   putc, getc, puts, and gets.

 */

val_t  open(val_t,val_t);
val_t  close(val_t);
val_t  read(val_t);
val_t  reads(val_t);
val_t  prn(val_t,val_t);
val_t load(val_t);

val_t  _std_port(FILE*);  // for initializing stdin, stdout, etc.
port_t* vm_open(chr_t*,chr_t*);
int_t vm_close(port_t*);
void vm_prn(val_t,port_t*);
chr_t vm_getc(port_t*);
int_t vm_putc(port_t*, chr_t);
int_t vm_peekc(port_t*);
int_t vm_puts(port_t*, chr_t*);
str_t* vm_gets(port_t*,int_t);
str_t* new_str(chr_t*);

/* printers for builtin types */

#define MAX_LIST_ELEMENTS 100 // to avoid 

void prn_int(val_t,port_t*);
void prn_type(val_t,port_t*);
void prn_str(val_t,port_t*);
void prn_cons(val_t,port_t*);
void prn_sym(val_t,port_t*);
void prn_proc(val_t,port_t*);
void prn_port(val_t,port_t*);

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
  TOK_NONE,
  TOK_STXERR,
} r_tok_t;

#define TOKBUFF_SIZE 1024
chr_t TOKBUFF[TOKBUFF_SIZE];
chr_t STXERR[512];
int_t TOKPTR;
r_tok_t TOKTYPE;

#define take() TOKTYPE = TOK_NONE

/* reader internal */
r_tok_t get_token(port_t*);
val_t read_expr(port_t*);
val_t read_cons(port_t*);
val_t vm_read(port_t*);
val_t vm_load(port_t*);

/* end txtio.h */
#endif
