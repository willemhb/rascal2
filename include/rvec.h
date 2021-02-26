#ifndef fvec_h
#define fvec_h

#include "rsp_core.h"
#include "values.h"
#include "mem.h"
#include "obj.h"


typedef enum
  {
    RV_INLINED  =0x01,
    RV_GLOBAL   =0x02,
    RV_DYNAMIC  =0x04,
    RV_IMMUTABLE=0x08,
  } rv_flags_t;


struct rvec_t
{
  tpkey_t type;
  uint32_t cmeta;
  uint64_t size;
  uint64_t elcnt;
  val_t rv_elements[1];
};


bool    isrvec(val_t);
rvec_t* sf_torvec(const chr_t*,int32_t,const chr_t*,val_t*);
rvec_t* mk_rvec(val_t*,size_t,uint32_t);
size_t  rvec_sizeof(val_t);
size_t  rvec_elcnt(val_t);
size_t  rvec_spc(val_t);
rvec_t* resize_rvec(rvec_t*,int64_t);
val_t   rvec_relocate(type_t*,val_t,uchr_t**);
void    rvec_prn(val_t,riostrm_t*);
val_t*  rvec_elements(val_t);
val_t   rvec_assocn(val_t,uint64_t);
val_t   rvec_append(val_t,val_t);
val_t   rvec_rplcn(val_t,uint64_t,val_t);

#define torvec(v) sf_torvec(__FILE__,__LINE__,__func__,&(v))

extern type_t RVEC_TYPE_OBJECT;

#endif
