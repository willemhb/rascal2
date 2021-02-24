#ifndef rstr_h
#define rstr_h

#include "rascal.h"
#include "values.h"
#include "error.h"
#include "mem.h"
#include "cval.h"
#include "hashing.h"
#include "strlib.h"
#include "describe.h"

struct rstr_t
{
  VOBJECT_HEAD;
  chr_t chars[16];
};

struct bytes_t
{
  VOBJECT_HEAD;
  uchr_t bytes[16];
};

bool     isstr(val_t);
bool     isbytes(val_t);
void     rstr_prn(val_t,riostrm_t*);
void     bytes_prn(val_t,riostrm_t*);
size_t   rstr_elcnt(val_t);
size_t   rstr_sizeof(val_t);
rstr_t*  mk_rstr(const chr_t*);
bytes_t* mk_bytes(const uchr_t*,size_t);
val_t    rstr_new(val_t,size_t);
val_t    bytes_new(val_t,size_t);
rchr_t   rstr_assocn(rstr_t*,rint_t);
rint_t   rstr_assocv(rstr_t*,rchr_t);
hash_t   rstr_hash(val_t,uint32_t);
hash_t   bytes_hash(val_t,uint32_t);
int32_t  rstr_ord(val_t,val_t);
int32_t  bytes_ord(val_t,val_t);
rstr_t*  sf_tostr(const chr_t*,int32_t,const chr_t*,val_t*);
bytes_t* sf_tobytes(const chr_t*,int32_t,const chr_t*,val_t*);

extern type_t RSTR_TYPE_OBJ;
extern type_t BYTES_TYPE_OBJ;

#endif
