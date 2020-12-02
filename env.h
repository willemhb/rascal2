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

ival_t _vhash(val_t);
ival_t _shash(sym_t*);
chr_t* _vname(val_t);
chr_t* _sname(sym_t*);

#define hash(s) SAFE_ACCESSOR_MACRO(s,sym_t*,_shash,_vhash)
#define name(s) SAFE_ACCESSOR_MACRO(s,sym_t*,_sname,_vname)

// accessors for tables
#define key_(t)     FAST_ACCESSOR_MACRO(t,tab_t*,key)
#define binding_(t) FAST_ACCESSOR_MACRO(t,tab_t*,binding)
#define left_(t)    FAST_ACCESSOR_MACRO(t,tab_t*,left)
#define right_(t)   FAST_ACCESSOR_MACRO(t,tab_t*,right)

val_t* _vkey(val_t);
val_t* _tkey(tab_t*);
val_t* _vbinding(val_t);
val_t* _tbinding(tab_t*);
tab_t** _vleft(val_t);
tab_t** _tleft(tab_t*);
tab_t** _vright(val_t);
tab_t** _tright(tab_t*);

#define key(t)      SAFE_ACCESSOR_MACRO(t,tab_t*,_vkey,_tkey)
#define binding(t)  SAFE_ACCESSOR_MACRO(t,tab_t*,_vbinding,_tbinding)
#define left(t)     SAFE_ACCESSOR_MACRO(t,tab_t*,_vleft,_tleft)
#define right(t)    SAFE_ACCESSOR_MACRO(t,tab_t*,_vright,_tright)

// generic comparison
int_t cmpv(val_t,val_t);
int_t cmps(chr_t*,chr_t*);
int_t cmph(hash_t,hash_t);
int_t cmphs(chr_t*,hash_t,chr_t*,hash_t);
int_t cmpq(sym_t*,sym_t*);

// symbols
sym_t* new_sym(chr_t*);
val_t sym(chr_t*);

// tables
tab_t* intern_string(chr_t*,tab_t**);
tab_t* new_tab();
tab_t* tab_get(val_t,val_t);
tab_t* tab_set(val_t,val_t,val_t);

// environments
// return the environment location associated with a particular symbol
bool isenvnames(val_t);

cons_t*  new_env(val_t,val_t,val_t);
val_t*  env_get(val_t,val_t);
val_t*  env_put(val_t,val_t);
val_t*  env_set(val_t,val_t,val_t);
/* end env.h */
#endif
