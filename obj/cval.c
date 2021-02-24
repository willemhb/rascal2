#include "../include/cval.h"


size_t cv_size(type_t* to, val_t x)
{
  return (to->tp_base_sz + (ptr(vobj_t*,x)->size * to->tp_cvtable->el_sz));
}

inline val_t cv_elcnt(val_t x)
{
  return ptr(vobj_t*,x)->size;
}

val_t fcv_relocate(type_t* to, val_t x, uchr_t** dest)
{
  size_t osz = to->tp_base_sz;
  size_t asz = calc_mem_size(osz);

  uchr_t* old = ptr(uchr_t*, x);
  uchr_t* new = *dest;
  memcpy(new,old,osz);
  *dest += asz;

  val_t out = tag(x,to);

  car_(old) = R_FPTR;
  cdr_(old) = out;

  return out;
}

val_t cv_relocate(type_t* to, val_t x, uchr_t** dest)
{
  size_t osz = val_sizeof(x,to);
  size_t asz = calc_mem_size(osz);

  uchr_t* old = ptr(uchr_t*, x);
  uchr_t* new = *dest;
  memcpy(new,old,osz);
  *dest += asz;

  val_t out = tag(x,to);

  car_(old) = R_FPTR;
  cdr_(old) = out;

  return out;
}
