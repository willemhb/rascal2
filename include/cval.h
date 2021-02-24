#ifndef cval_h
#define cval_h

#include "rascal.h"
#include "values.h"
#include "error.h"
#include "mem.h"

/* 
   core of cval, vcval, and direct data implementations.

 */

typedef enum
  {
    INLINED = 0x01u,
  } cvflags_t;


bool       cv_isinlined(val_t);
val_t      cv_size(type_t*,val_t);
val_t      cv_elcnt(val_t);
val_t      fcv_relocate(type_t*,val_t,uchr_t**);
val_t      cv_relocate(type_t*,val_t,uchr_t**);

#endif
