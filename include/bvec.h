#ifndef rvec_h
#define rvec_h

#include "rascal.h"
#include "values.h"
#include "error.h"
#include "obj.h"
#include "describe.h"


struct bvec_t
{
  tpkey_t  type;
  uint32_t bv_bmap;
  val_t    bv_elements[1];
};


typedef struct leaf_t
{
  tpkey_t type;
  hash_t hash;
  val_t  key;
  val_t  value;
  struct leaf_t*  next;   // collisions are resolved by separate chaining (hashes only collide if all 32-bits match)
} leaf_t;


// leaf type for maps that don't store bindings
typedef struct sleaf_t
{
  tpkey_t type;
  hash_t hash;
  val_t key;              // key is either a single key or a linked list of keys
} sleaf_t;


int32_t  popcnt(uint32_t);
bool     isbvec(val_t);
bvec_t*  sf_tobvec(const chr_t*,int32_t,const chr_t*,val_t*);
bvec_t*  mk_bvec(val_t,size_t);
bvec_t*  cp_bvec(bvec_t*,size_t);
size_t   bvec_elcnt(val_t);
size_t   bvec_sizeof(type_t*,val_t);

#define tobvec(v) sf_tobvec(__FILE__,__LINE__,__func__,&(v))

#endif
