#ifndef vm_h

#define vm_h
#include "rascal.h"

// specialized accessors for VM reserved structures
uchr8_t*  code_instr(code_t*);
val_t*    code_values(code_t*);
table_t*  envt_names(envt_t*);
size_t    code_add_value(code_t*,val_t);

envt_t*  envt_next(envt_t*);
envt_t*  evnt_nmspc(envt_t*);
val_t*   envt_bindings(envt_t*);
size_t   envt_nbindings(envt_t*);

#endif
