#ifndef env_h

#define env_h

/* begin env.h */

#include "obj.h"
#include "cons.h"

/*

  This module contains the full api for symbols and symt types,
  as well as the env api. These types are very common and commonly used
  together, so including them in a single module is convenient.

 */

// fast accessors
#define symhash(s)    ((hash_t)(obj(s)->head.meta))
#define symflags(s)   (obj(s)->head.flags)
#define isinterned(s) (symflags(s) == SYMFLAG_INTERNED)
#define uninterned(s) (symflags(s) == SYMFLAG_UNINTERNED)
#define symname(s)    (_tosym(s)->name)
#define tagsym(s)     (tagptr(s,LOWTAG_OBJPTR))

// symbols and symbol tables
rsym_t* vm_new_sym(chr_t*,hash_t,uint32_t);
rval_t vm_sym_constructor(rval_t);
rsym_t* vm_intern_str(chr_t*, rsym_t**);
rsym_t** vm_sym_lookup(chr_t*,hash_t,rsym_t**);
uint32_t vm_sym_sizeof(rsym_t*);

// environments
// return the environment location associated with a particular symbol
rcons_t* vm_new_env(rcons_t*,rcons_t*,rval_t);
rval_t   vm_assoc_env(rsym_t*,rval_t);
rval_t   vm_get_sym_env(rsym_t*,rval_t);
void     vm_put_sym_env(rsym_t*,rval_t);
void     vm_set_sym_env(rsym_t*,rval_t,rval_t);
/* end env.h */
#endif
