#ifndef error_h
/* begin error.h */

#define error_h
#include <time.h>
#include "common.h"

/* 
   This module implements the basic error handling and logging system.

   The core API for objects (type checking, casting), types (getting type data and type
   metadata), direct values (because they don't warrant their own module), conses & lists 
   (because they're fundamental), and allocation/memory management can be found in obj.h/obj.c

   The API for symbols, tables, and environments can be found in env.h/env.c. 
   
   The API for functions, procedures, the compiler/evaluator/vm, etc., can be found in
   eval.h/eval.c.

   The API for strings, bytevectors, ports, and the reader can be found in txtio.h/txtio.c.

   The API for builtin functions, bootstrapping, initialization, and cleanup can be found
   in builtins.h/builtins.c.

   The main function can be found in rascal.c!
 */

#define STDLOG_PATH "log/rascal.log"
#define STDHIST_PATH "log/rascal.history.log"

/* 
   Safety should be used to jump directly to the top level in the event of critical errors 
   like memory overflow, stack overflow, potential segfault, etc. Common errors should return
 
   Common errors should be signaled using a few conventional sentinel values based on the
   expected return type.

   NULL  - functions that return pointers should return NULL to signal an error.
   -2    - functions that return integers should return -2 to signal an error (
           -1 is used for interning symbols, so using -1 would lead to a lot of unnecessary
           error checking).
   NONE  - functions that return val_t should return the special rascal value NONE to signal
           an error.

   Additionally, since these sentinels might be normal return values, the functions that return
   them should set the global ERRORCODE variable. An error occured if and only if a return value
   is one of the above, AND the value of ERRORCODE is non-zero.

   This module provides a few macros and functions to simplify raising and handling errors.

   the checks(v) macro checks that both error conditions are true.

   the fail(e,s,fmt,args...) macro sets the error status, writes the supplied message to stdlog,
   and returns the given sentinel.

   ifail, rfail, and cfail fail with the sentinels -2, NONE, and NULL respectively.

   the clear macro resets the value of ERRORCCODE to 0.

   if checks returns true, then it MUST be followed by a call to fail or clear, or future
   error codes will be unreliable.
*/

FILE* stdlog;
jmp_buf SAFETY;

typedef enum r_errc_t {
  OK_ERR,
  TYPE_ERR,
  VALUE_ERR,
  ARITY_ERR,
  UNBOUND_ERR,
  OVERFLOW_ERR,
  IO_ERR,
  NULLPTR_ERR,
  SYNTAX_ERR,
  INDEX_ERR,
} r_errc_t;

r_errc_t ERRORCODE = OK_ERR;

const chr_t* ERRNAMES[] = {
                            "OK", "TYPE", "VALUE", "ARITY", "UNBOUND",
                            "OVERFLOW", "IO", "NULLPTR", "SYNTAX", "INDEX",
                          };

bool _checkerr_null(void*);         // checks dispatch for pointers
bool _checkerr_val(val_t);          // checks dispatch for val
bool _checkerr_int(int_t);          // checks dispatch for int

/* set a given error code and return a sentinel value */
int_t _seterr_int(int_t,int_t);
val_t _seterr_none(int_t,val_t);
void* _seterr_null(int_t,void*);

#define seterr(i,s) _Generic((s),                         \
			     val_t:_seterr_none,          \
			     int_t:_seterr_int,           \
			     default:_seterr_null)(i,s)

#define checkerr(s) _Generic((s),                         \
			     val_t:_checkerr_val,         \
			     int_t:_checkerr_int,         \
			     default:_checkerr_null)(s)

#define clearerr() ERRORCODE = OK_ERR


// fail and exit the function with a sentinel value
#define fail(e,s)                 \
  ERRORCODE = e ;                 \
  return s

#define failv(e,s,fmt,args...)    \
  eprintf(e,stdlog,fmt,##args) ;  \
  ERRORCODE = e ;                 \
  return s                        \

#define eprintf(e, file, fmt, args...)				\
  fprintf(file, "%s: %d: %s: %s ERROR: ",__FILE__,__LINE__,__func__,ERRNAMES[e]);    \
  fprintf(file,fmt,##args);                                                          \
  fprintf(file,".\n")

//  print common error messages to the log file.
#define elogf(e,fmt,args...) eprintf(e,stdlog,fmt,##args)
// shorthand for jumping to the safety point. The safety point prints an informative message
// and initiates program exit.
#define escape(e) longjmp(SAFETY, e)
// since they're called together a lot, escapef composes them
#define escapef(e,file,fmt,args...) eprintf(e,file,fmt,##args); escape(e)


void init_log();                 // initialize the global variable stdlog. Stdlog is 
chr_t* read_log(chr_t*, int_t);  // read the log file into a string buffer.
void dump_log(FILE*);            // dump the contents of the log file into another file.
int_t finalize_log();            // copy the contents of the log to the history
/* end error.h */
#endif
