#ifndef mem_h
#define mem_h
#include "rsp_core.h"
#include "values.h"

// stack manipulation
void  grow_stack();
void  grow_dump();
val_t pop();
void  save(uint32_t);
void  restore(uint32_t);
void  frestore(uint32_t); // restore register values without relinquishing stack space
void  fsave(uint32_t);    // save values without consuming stack space (used with frestore)
val_t push(val_t);
val_t pushn(size_t);
void  popn(size_t);


// memory management
void*    vm_cmalloc(uint64_t);
int32_t  vm_cfree(void*);
void*    vm_crealloc(void*,uint64_t,bool);
void*    vm_alloc(size_t,size_t,size_t);
void*    vm_realloc(val_t,size_t,size_t);
size_t   calc_mem_size(size_t);
size_t   val_asizeof(val_t,type_t*);
bool     gc_check(void);
void     gc_resize(void);
void     gc_run(void);
val_t    gc_trace(val_t);
val_t    gc_copy(type_t*,val_t);

bool     v_in_heap(val_t,void*,uint64_t);
bool     p_in_heap(void*,void*,uint64_t);

#define in_heap(v,u,sz)                            \
  _Generic((v),                                    \
	   val_t:v_in_heap,                        \
	   default:p_in_heap)(v,u,sz)


/* allocation macros */
#define vm_allocw(bs,wds)          vm_alloc(bs,wds,8)
#define vm_allocb(bs,nb)           vm_alloc(bs,nb,1)
#define vm_allocc(nc)              vm_alloc(0,16,nc)
#define vm_reallocw(v,wds)         vm_realloc(v,wds,8)
#define vm_reallocb(v,b)           vm_realloc(v,b,1)

#endif
