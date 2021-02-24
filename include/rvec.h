#ifndef rvec_h
#define rvec_h

#include "rascal.h"
#include "values.h"
#include "error.h"
#include "obj.h"

struct vec32_t
{
  OBJECT_HEAD;
  val_t elements[1];
};

struct bvec_t
{
  VOBJECT_HEAD;
  val_t parent;
  val_t elements[1];
};


svec_t*  mk_vec(val_t*,size_t);
svec_t*  mk_nvec(size_t,...);
bvec_t*  mk_bvec(val_t*,hash_t*,uint8_t,size_t);
bvec_t*  mk_nbvec(uint8_t,size_t,...);
void     vec_prn(val_t,riostrm_t*);
void     bvec_prn(val_t,riostrm_t*);
size_t   vec_elcnt(val_t);
size_t   bvec_elcnt(val_t);
size_t   bvec_sizeof(type_t*,val_t);
hash_t   hash_vec(val_t,uint32_t);
int32_t  init_vec(type_t*,val_t,void*);
val_t    vec_relocate(type_t*,val_t,uchr_t**);


extern type_t SVEC_TYPE_OBJ;
extern type_t BVEC_TYPE_OBJ;

#endif
