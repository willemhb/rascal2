#ifndef env_h

#define env_h

/* begin env.h */

#include "common.h"
#include "error.h"
#include "obj.h"

/* 

   This module contains the API for symbols, tables, and environments, as well as the generic
   ord function (major component of the API for tables).

   The basic error handling and logging mechanisms can be found in error.h/rascal.h.

   The core API for objects (type checking, casting), types (getting type data and type
   metadata), direct values (because they don't warrant their own module), conses & lists 
   (because they're fundamental), and allocation/memory management can be found in obj.h/obj.c
   
   The API for functions, procedures, the compiler/evaluator/vm, etc., can be found in
   eval.h/eval.c.

   The API for strings, bytevectors, ports, and the reader can be found in txtio.h/txtio.c.

   The API for builtin functions, bootstrapping, initialization, and cleanup can be found
   in builtins.h/builtins.c.

   The main function can be found in rascal.c!

 */


// accessors for symbols

#define hash_(s)  FAST_ACCESSOR_MACRO(s,sym_t*,head.meta)
#define name_(s)  FAST_ACCESSOR_MACRO(s,sym_t*,name)

val_t hash(val_t);
chr_t* name(val_t);

// accessors for tables
#define key_(t)     FAST_ACCESSOR_MACRO(t,tab_t*,key)
#define binding_(t) FAST_ACCESSOR_MACRO(t,tab_t*,binding)
#define left_(t)    FAST_ACCESSOR_MACRO(t,tab_t*,left)
#define right_(t)   FAST_ACCESSOR_MACRO(t,tab_t*,right)


// generic comparison
int_t cmpv(val_t,val_t);
int_t cmps(chr_t*,chr_t*);
int_t cmphs(chr_t*,hash_t,chr_t*,hash_t);

// symbols
val_t sym(chr_t*);

// tables
tab_t* new_tab();
sym_t* intern_builtin(const chr_t*,val_t);
tab_t* tab_search(val_t,tab_t*);
val_t  tab_get(val_t,val_t);
val_t  tab_set(val_t,val_t,val_t);

// environments
bool isenvnames(val_t);  // test whether the given argument is a valid list of argument names
val_t   new_env(val_t,val_t,val_t);
val_t env_assoc(val_t,val_t);
val_t env_put(val_t,val_t);
val_t env_set(val_t,val_t,val_t);
/* end env.h */
#endif
 
