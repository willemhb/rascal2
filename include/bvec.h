#ifndef bvec_h
#define bvec_h
#include "rsp_core.h"
#include "values.h"
#include "obj.h"
#include "describe.h"



struct bvec_t
{
  tpkey_t  type;
  uint32_t bv_bmap;
  val_t    bv_elements[1];
};


uint32_t popcnt(uint32_t);
uint32_t bmtoidx(uint32_t);
uint8_t  idxtobm(uint32_t,uint8_t);
bool     isbvec(val_t);
bvec_t*  sf_tobvec(const chr_t*,int32_t,const chr_t*,val_t*);
bvec_t*  mk_bvec(size_t,bool);
size_t   bvec_elcnt(val_t);
size_t   bvec_sizeof(type_t*,val_t);
uint8_t  get_bm_index(uint32_t,uint8_t);
val_t*   bvec_ref(bvec_t*,uint8_t);
bvec_t*  cp_bvec(bvec_t*,int32_t);

#define tobvec(v)  sf_tobvec(__FILE__,__LINE__,__func__,&(v))

#endif
