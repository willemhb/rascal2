#include "../include/bvec.h"


MK_TYPE_PREDICATE(BVECTOR,OBJECT,bvec)
MK_SAFECAST_P(bvec_t*,bvec,addr)

inline int32_t popcnt(uint32_t b)
{
  return __builtin_popcount(b);
}

bvec_t*  mk_bvec(val_t p, size_t sz)
{
  bvec_t* out = vm_allocw(16,sz);
  out->type = BVECTOR;
  out->bv_bmap = 0;
  out->bv_parent = p;
  return out;
}

bvec_t*  cp_bvec(bvec_t* frm, size_t grow)
{
  int32_t frmsz = popcnt(frm->bv_bmap);
  bvec_t* out = vm_allocw(16, frmsz + grow);
  memcpy((void*)out,(void*)frm,16+frmsz);
  return out;
}

size_t   bvec_elcnt(val_t b)
{
  bvec_t* bv = tobvec(b);
  return popcnt(bv->bv_bmap);
}

size_t   bvec_sizeof(type_t* to, val_t b)
{
  return to->tp_base_sz + bvec_elcnt(b) * 8;
}
