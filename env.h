#ifndef env_h

#define env_h

/* begin env.h */

#include "obj.h"
#include "cons.h"


enum {
  SYMFLAG_UNINTERNED,
  SYMFLAG_INTERNED,
  SYMFLAG_VARIABLE=0,
  SYMFLAG_CONSTANT,    // symbols marked constant are for important global variables like form-names, etc.
};


/*

  This module contains the full api for symbols and symt types,
  as well as the env api. These types are very common and commonly used
  together, so including them in a single module is convenient.

 */

// fast accessors
#define symhash(s)        ((hash_t)((s)->head.meta))
#define internedfl(s)     ((s)->head.flags & 0b001)
#define constfl(s)        ((s)->head.flags & 0b010) >> 1)
#define setinterned(s,f)  (s)->head.flags &= 0b110; (s)->head.flags |= f
#define setconstfl(s,f)   (s)->head.flags &= 0b101; (s)->head.flags |= (f << 1)
#define tagsym(s)         tagptr(s,TAG_SYM)


// symbols and symbol tables
rsym_t* vm_new_sym(chr_t*,hash_t,uint32_t);
rval_t vm_sym_constructor(rval_t);
rsym_t* vm_intern_str(chr_t*, rsym_t**);
rsym_t** vm_sym_lookup(chr_t*,hash_t,rsym_t**);
uint32_t vm_sym_sizeof(rsym_t*);

// environments
// return the environment location associated with a particular symbol
rcons_t* vm_new_env(rval_t,rcons_t*,rcons_t*);
rcons_t* vm_assoc_env(rsym_t*,rcons_t*);
rval_t   vm_get_sym_env(rsym_t*,rcons_t*);
void     vm_put_sym_env(rsym_t*,rcons_t*);
void     vm_set_sym_env(rsym_t*,rval_t,rcons_t*);
/* end env.h */
#endif
