#ifndef capi_h

/* begin capi.h */
#define capi_h

#include "rascal.h"


/* 
   builtin.c and capi.c contain functions that wrap VM functions and C bindings
   as rascal accessible functions (functions that take a pointer to an array of arguments
   and a count of the array size).

   I would like to replace both of these with a proper C interface, but this is much easier to implement at the moment.

 */


const char* BUILTIN_FUNCNAMES[NUM_BUILTIN_FUNCTIONS] =
  {
  "alpha?", "alnum?",  "lower?", "upper?",
  "digit?", "xdigit?", "cntrl?", "graph?",
  "space?", "blank?", "print?", "punct?",
  "upper", "lower", "osgetenv", "exit",
  "open", "close", "eof?", "getc", "putc",
  "peekc", "gets", "puts",
  };

/* inlined functional bindings for C arithmetic */
// arithmetic

#define DESCRIBE_API(fname) val_t bltn_## fname(val_t*)
#define DESCRIBE_VOID_API(fname) void bltn_## fname(val_t*)

// character handling
DESCRIBE_API(alphap);
DESCRIBE_API(alnump);
DESCRIBE_API(lowerp);
DESCRIBE_API(upperp);
DESCRIBE_API(digitp);
DESCRIBE_API(xdigitp);
DESCRIBE_API(cntrlp);
DESCRIBE_API(graphp);
DESCRIBE_API(spacep);
DESCRIBE_API(blankp);
DESCRIBE_API(printp);
DESCRIBE_API(punctp);
DESCRIBE_API(upper);
DESCRIBE_API(lower);
DESCRIBE_API(u8len);

// system calls
DESCRIBE_VOID_API(exit);
DESCRIBE_API(osgetenv);

// IO utilities
DESCRIBE_API(peekc);
DESCRIBE_API(putc);
DESCRIBE_API(getc);
DESCRIBE_API(gets);
DESCRIBE_API(puts);
DESCRIBE_API(open);
DESCRIBE_API(close);
DESCRIBE_API(eofp);


/* builtin functions */
// numeric functions


/* end capi.h */
#endif
