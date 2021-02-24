#ifndef obj_h
#define obj_h

#include "rascal.h"
#include "values.h"
#include "error.h"
#include "mem.h"

/* 

   core of obj and vobj implementations.

 */

val_t      fobj_new(type_t*,val_t,size_t);
val_t      vobj_new(type_t*,val_t,size_t);
val_t      fobj_sizeof(type_t*,val_t);
val_t      vobj_sizeof(type_t*,val_t);
val_t*     obj_data(type_t*,val_t);
val_t*     obj_elements(type_t*,val_t);
val_t      obj_relocate(type_t*,val_t,uchr_t**);

#endif
