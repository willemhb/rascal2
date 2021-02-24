#ifndef pairs_h
#define pairs_h
#include "rascal.h"
#include "values.h"
#include "mem.h"
#include "hashing.h"
#include "describe.h"
#include "obj.h"
#include "error.h"

extern const type_t PAIR_TYPE_OBJ;

// pairs
pair_t*   mk_list(val_t*,size_t);
pair_t*   mk_pair(val_t,val_t);
val_t     rsp_cons(val_t,size_t);
val_t     car(val_t);
val_t     cdr(val_t);
val_t     pair_relocate(type_t*,val_t,uchr_t**);
hash_t    pair_hash(val_t,uint32_t);
void      pair_prn(val_t,riostrm_t*);
size_t    pair_elcnt(val_t);
pair_t*   pair_append(pair_t**,val_t);
val_t     pair_assocn(pair_t*,int32_t);
int32_t   pair_assocv(pair_t*,val_t);
pair_t*   pair_rplcn(pair_t*,int32_t, val_t);
pair_t*   sf_topair(const chr_t*,const int32_t,const chr_t*, val_t*);


#define head(p)    ((p)->car)
#define tail(p)    ((p)->cdr)
#define topair(v)  sf_topair(__FILE__,__LINE__,__func__, &(v))

#endif
