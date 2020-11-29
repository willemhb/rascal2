#ifndef cons_h

/* begin cons.h */
#define cons_h
#include "obj.h"
#include "mem.h"

/* 
   this module defines the core api for conses and lists, and the functions that depend 
   on them.
*/

enum {
  WTAG_LIST=0b1,
  WTAG_NOLIST=0b0,
};

typedef enum _ls_enum_t {
  REQUIRE_LIST,
  REQUIRE_CONS,
  ALLOW_ANY,
} ls_enum_t;

// unsafe cons accessors
#define _car(c)      (_tocons(c)->car)
#define _cdr(c)      (_tocons(c)->cdr)
#define _lstail(ls)  (_tocons(ls->cdr))
#define _isnull(ls)  (ls == NULL)
#define empty(v)       \
  _Generic((v),        \
    rval_t:isnil,      \
    rcons_t*:_isnull)(v)

// conses
// list predicate
bool islist(rval_t);
// return a proper cons tag that respects the list invariant
rval_t tagcons(rcons_t*);
// helpers for testing lists
int32_t vm_cons_len(rval_t, ls_enum_t);
int32_t vm_cons_of(rval_t, uint32_t, ls_enum_t);
// vm api
rcons_t* vm_new_cons(rval_t,rval_t,ls_enum_t);
rval_t   vm_cons_constructor(rval_t,rval_t);
uint32_t vm_cons_sizeof(rval_t);
rcons_t* vm_cons_lastpair(rcons_t*);
rcons_t* vm_cons_append(rcons_t**,rval_t,ls_enum_t);
rcons_t* vm_assoc_cons(rval_t,rcons_t*, ls_enum_t);
// since many applications require lists, these macros simplify calling cons vm functions

#define vm_new_list(ca,cd)   vm_new_cons(ca,cd,REQUIRE_LIST)
#define vm_list_len(c)       vm_cons_len(c,REQUIRE_LIST)
#define vm_list_of(c,t)      vm_cons_of(c,t,REQUIRE_LIST)
#define vm_list_append(c,v)  vm_cons_append(c,v,REQUIRE_LIST)
#define vm_assoc_list(v,c)   vm_assoc_cons(v,c,REQUIRE_LIST)

/* end cons.h */
#endif
