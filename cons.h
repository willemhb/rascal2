#ifndef cons_h

/* begin cons.h */
#define cons_h
#include "obj.h"
#include "mem.h"

/* 
   this module defines the core api for conses and lists, and the functions that depend 
   on them.
*/

#define tagcons(v) tagptr(v, LOWTAG_OBJPTR)

// conses
// list predicate
bool islist(rval_t);
// Check that the supplied list is a valid argument list and return the VARGS flag (on success)
// or -1 (on failure)
int32_t norm_arglist(rval_t*);
// fix the metadata on a cons that's been turned into a list
rval_t vm_fix_list(rval_t);
// vm api
rcons_t* vm_new_cons(rval_t,rval_t);
rcons_t* vm_new_list(rval_t,rval_t);
rval_t   vm_cons_constructor(rval_t,rval_t);
uint32_t vm_cons_sizeof();
int32_t vm_list_len(rval_t);
rcons_t* vm_cons_lastpair(rcons_t*);
rcons_t* vm_cons_append(rcons_t*,rval_t);
rcons_t* vm_assoc_list(rval_t,rcons_t*);

/* end cons.h */
#endif
