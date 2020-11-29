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
rport_t* vm_new_port(FILE*);
uint32_t vm_port_sizeof(rval_t);
rbvec_t* vm_new_bvec(uchr_t*);
uint32_t vm_bvec_sizeof(rval_t);
rstr_t* vm_new_str(chr_t*);
uint32_t vm_str_len(rstr_t*);
uint32_t vm_str_sizeof();

/* writers for builtin types */

/* tokenizing */
typedef enum _r_tok_t {
  TOK_LPAR,
  TOK_RPAR,
  TOK_DOT,
  TOK_QUOT,
  TOK_NUM,
  TOK_STR,
  TOK_SYM,
} r_tok_t;

/* core reader procedures */
void vm_prn(rval_t,FILE*);
rval_t vm_read(FILE*);
rval_t vm_read_sexpr(FILE*);
rval_t vm_load(FILE*);
void vm_repl(void);
/* end txtio.h */
#endif
