#ifndef vm_h
#define vm_h

#include "rsp_core.h"
#include "opcodes.h"
#include "objects.h"

typedef struct
{
  tpkey_t type;
  uint32_t cmeta;
  val_t argl;
  val_t body;
  val_t envt;
} closure_t;

// create a captured environment frame ()
val_t  mk_closure(val_t envt, val_t* loc);
val_t  mk_proc(val_t nms, val_t body, val_t envt, val_t* loc, bool do_eval);
size_t mk_let_env(val_t*,size_t,val_t);
val_t  env_put(val_t nm, val_t envt);
size_t env_size(val_t envt);
val_t  env_set(val_t nm, val_t val, val_t envt);
val_t  env_get(val_t nm, val_t envt);
val_t  rsp_eval(val_t expr, val_t envt, val_t nmspc);

#endif
