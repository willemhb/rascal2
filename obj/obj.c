#include "../include/obj.h"

val_t fobj_new(type_t* to, val_t args, size_t argc)
{  
  argcount(to->tp_nfields, argc);
  void* dest = vm_allocw(to->tp_base_sz,to->tp_nfields);
  val_t* stk = (val_t*)args;
  val_t* data = (val_t*)fval_init_head(to,dest);

  for (size_t i = 0; i < to->tp_nfields; i++)
    data[i] = stk[i];

  if (to->tp_init)
    to->tp_init(to,args,argc,dest);

  return tag(dest,to);
}


val_t vobj_new(type_t* to, val_t args, size_t argc)
{
  vargcount(to->tp_nfields,argc);
  size_t var_data = argc - to->tp_nfields;
  void* dest = vm_allocw(to->tp_base_sz,var_data);
  val_t* stk = (val_t*)args;
  val_t* data = (val_t*)vval_init_head(to,argc,dest);

  for (size_t i = 0; i < argc; i++)
    data[i] = stk[i];

  if (to->tp_init)
    to->tp_init(to,args,argc,dest);

  return tag(dest,to);
}

inline val_t vobj_sizeof(type_t* to, val_t x)
{
  return (ptr(vobj_t*,x)->size * 8) + to->tp_base_sz;
}

inline val_t sobj_sizeof(type_t* to, val_t x)
{
  return (ptr(obj_t*,x)->cmeta * 8) + to->tp_base_sz;
}

inline val_t* obj_data(type_t* to, val_t x)
{
  return (val_t*)(ptr(void*,x) + to->tp_data_offset);
}

inline val_t* obj_elements(type_t* to, val_t x)
{
    return (val_t*)(ptr(void*,x) + to->tp_base_sz);
}

inline val_t fobj_relocate(type_t* to, val_t x, uchr_t** dest)
{
  size_t osz = val_sizeof(x,to);
  size_t asz = calc_mem_size(osz);
  uchr_t* out = *dest;
  memcpy(ptr(uchr_t*,x),dest,osz);
  *out += asz;

  val_t* frm_d = obj_data(to,x);
  val_t* to_d  = (val_t*)(out + to->tp_data_offset);

  for (size_t i = 0; i < to->tp_nfields; i++)
      to_d[i] = gc_trace(frm_d[i]);

  val_t rtn = tag(out,to);
  car_(x) = R_FPTR;
  cdr_(x) = rtn;

  return rtn;
}


inline val_t vobj_relocate(type_t* to, val_t x, uchr_t** dest)
{
  size_t len = to->tp_elcnt(x);
  size_t osz = val_sizeof(x,to);
  size_t asz = calc_mem_size(osz);
  uchr_t* out = *dest;
  memcpy(ptr(uchr_t*,x),dest,osz);
  *out += asz;

  val_t* frm_d = obj_data(to,x);
  val_t* to_d  = (val_t*)(out + to->tp_data_offset);

  for (size_t i = 0; i < to->tp_nfields + len; i++)
      to_d[i] = gc_trace(frm_d[i]);

  val_t rtn = tag(out,to);
  car_(x) = R_FPTR;
  cdr_(x) = rtn;

  return rtn;
}
