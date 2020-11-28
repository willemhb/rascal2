#include "obj.h"

/* type utilities */
int32_t typecode(rval_t v) {
  int32_t lt = lowtag(v);
  if (lt == LOWTAG_DIRECT) {
    return (v & UINT_MAX) >> 3;
  } else {
    return obj(v)->head.type;
  }
}

rtype_t* type(rval_t v) {
  return TYPES[typecode(v)];
}

chr_t* vm_val_typename(rval_t v) {
  int32_t t = typecode(v);

  if (t < NUM_BUILTIN_TYPES) {
    return BUILTIN_TYPENAMES[t];
  }
  
  else {
    return (TYPES[t])->tp_name->name; 
  }
}

uint32_t vm_obj_size (rval_t v) {
  if (lowtag(v) == LOWTAG_DIRECT) {
    return 8;
  } else {
    return type(v)->tp_sizeof(obj(v));
  }
}
