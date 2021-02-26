#include "../include/bvec.h"



inline uint32_t popcnt(uint32_t b)
{
  return __builtin_popcount(b);
}


bvec_t* mk_bvec(size_t sz, bool gl)
{
  assert(sz <= 32, BOUNDS_ERR);
  bvec_t* out = gl ? vm_cmalloc((1+sz)*8) : vm_allocw(8,sz);
  out->type = BVECTOR;
  out->bv_bmap = 0;

  return out;
}


size_t bvec_elcnt(val_t b)
{
  bvec_t* bv = tobvec(b);
  return popcnt(bv->bv_bmap);
}

size_t bvec_sizeof(type_t* to, val_t b)
{
  return to->tp_base_sz + bvec_elcnt(b) * 8;
}

inline uint32_t bmtoidx(uint32_t bmp)
{
  assert(bmp > 0, BOUNDS_ERR);
  return __builtin_ctz(bmp);
}

inline uint8_t idxtobm(uint32_t bmp, uint8_t idx)
{
  return popcnt(bmp & ((1 << idx) - 1));
}


val_t* bvec_ref(bvec_t* bv, uint8_t idx)
{
  assert(idx <= 32, BOUNDS_ERR);
  if (bv->bv_bmap & (1 << idx))
    return bv->bv_elements + idxtobm(bv->bv_bmap,idx);

  else
    return NULL;
}
