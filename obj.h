#ifndef obj_h

/* begin obj.h */
#define obj_h

#include "common.h"

/* 
   this module contains the definitions for all the core builtin types,
   the basic api for working with values and data (accessing common fields,
   getting object size, etc.), the api for working with rtype_t objects,
   and core error checking/reporting utilities.

   Memory handling and bounds checking is in the vm module, and type-specific apis
   are in specialized module for each type (or family of types).

 */
/* forward declarations */
int32_t typecode(rval_t);
rtype_t* type(rval_t);
chr_t* vm_val_typename(rval_t);
uint32_t vm_obj_size(rval_t);

#define type_selfcode(t) ((t)->head.meta)


/* convenience macros */
// unsafe casts
#define _toobj(v)  ((robj_t*)ptr(v))
#define _totype(v) ((rtype_t*)ptr(v))
#define _toint(v)  ((rint_t)val(v))
#define _tocons(v) ((rcons_t*)ptr(v))
#define _tosym(v)  ((rsym_t*)ptr(v))
#define _tosymt(v) ((rsymt_t*)ptr(v))
#define _tostr(v)  ((rstr_t*)ptr(v))
#define _toproc(v) ((rproc_t*)ptr(v))
#define _toprim(v) ((rprim_t*)ptr(v))
#define _toport(v) ((rport_t*)ptr(v))
#define _objhead(v) (_toobj(v)->head)


/* 
   'safe' macros for getting the typecode and casting to a particular type.
   
   Making these macros leads to more precise error reporting.

 */

// fast type checks for builtin types
#define istype(v) (typecode(v) == TYPECODE_TYPE)
#define issym(v)  (typecode(v) == TYPECODE_SYM)
#define iscons(v) (typecode(v) == TYPECODE_CONS)
#define isstr(v)  (typecode(v) == TYPECODE_STR)
#define isproc(v) (typecode(v) == TYPECODE_PROC)
#define isport(v) (typecode(v) == TYPECODE_PORT)
#define isint(v)  (typecode(v) == TYPECODE_INT)
#define isa(v,t)  (typecode(v) == (t))

#define SAFECAST_MACRO(v,t)			\
  ({ rval_t _v_ = v ; \
    if (!is##t(_v_)) \
      {\
	eprintf(stderr,"Expected type %s, got %s.\n", #t, vm_val_typename(_v_)); \
	escape(ERROR_TYPE);\
      }\
    _to##t(_v_) ; })

#define totype(v) SAFECAST_MACRO(v,type)
#define tosym(v)  SAFECAST_MACRO(v,sym)
#define tocons(v) SAFECAST_MACRO(v,cons)
#define tostr(v)  SAFECAST_MACRO(v,str)
#define toproc(v) SAFECAST_MACRO(v,proc)
#define toport(v) SAFECAST_MACRO(v,port)
#define toint(v)  SAFECAST_MACRO(v,int)

/* end obj.h */
#endif
