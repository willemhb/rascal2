#include "../include/values.h"
#include "../include/describe.h"

/* tag manipulation, type testing, pointer tracing */
inline uint32_t ltag(val_t v)
{
  return v & LTAG_MASK ;
}

tpkey_t tpkey(val_t v)
{
  if (!(v & PTR_MASK))
    return NIL;

  tpkey_t n = ltag(v);
  switch (n)
    {
      case PAIR ... IOSTRM:
        return n;

    case OBJECT ... FUNCTION:
      return objhead(v)->type;
	
    case DIRECT: default:
      return v & DTYPE_MASK;
    }
}

inline val_t addr(val_t v)
{
  return v & PTR_MASK;
}

inline void* vptr(val_t v)
{
  return ptr(void*,trace_fptr(v));
}

inline rsp_c32_t value(val_t v)
{
  return (rsp_c32_t)unpad(v);
}

val_t pad(rsp_c32_t b)
{
  return ((val_t)b.bits32) << 32;
}

uint32_t unpad(val_t v)
{
  return v >> 32;
}

bool p_aligned(const void* p)
{
  return (val_t)p % 16;
}

bool v_aligned(val_t v)
{
  return (v & PTR_MASK) % 16;
}


inline obj_t* objhead(val_t v)
{
  return ptr(obj_t*,trace_fptr(v));
}


inline type_t* val_type(val_t v) { return GLOBAL_TYPES[tpkey(v)]; }
inline type_t* tk_type(tpkey_t t) { return GLOBAL_TYPES[t]; }
inline chr_t*  val_typename(val_t v) { return val_type(v)->name; }
inline chr_t*  tk_typename(tpkey_t t) { return tk_type(t)->name; }

val_t tag_from_type(rsp_c64_t d, type_t* to)
{
  return d.bits64 | to->tp_ltag;
}

val_t tag_from_tpkey(rsp_c64_t d, tpkey_t t)
{
  return d.bits64 | t;
}

// top-level functions for hashing and sizing
inline size_t val_sizeof(val_t v, type_t* to)
{
  if (isdirect(v))
    return 8;

  if (!to)
    to = val_type(v);

  if (to->tp_capi->size)
    return to->tp_capi->size(to, v);

  return to->tp_base_sz;
}

inline size_t val_nwords(val_t v, type_t* to)
{
  return val_sizeof(v,to) / 8;
}

hash_t val_hash(val_t v)
{
  type_t* to = val_type(v);
  return to->tp_capi->hash(v,to->tp_tpkey+1);
}

hash_t   val_rehash(val_t v, uint32_t r)
{
  type_t* to = val_type(v);
  return to->tp_capi->hash(v,(to->tp_tpkey+1)*r);
}

/* predicates */
  MK_EQUALITY_PREDICATE(R_NIL,nil)
  MK_EQUALITY_PREDICATE(R_TRUE,true)
  MK_EQUALITY_PREDICATE(R_FALSE,false)
  MK_EQUALITY_PREDICATE(R_FPTR,fptr)
  MK_EQUALITY_PREDICATE(R_UNBOUND,unbound)
  MK_EQUALITY_PREDICATE(R_EOF,eof)
    MK_LTAG_PREDICATE(PAIR,pair)
    MK_LTAG_PREDICATE(IOSTRM,iostrm)
    MK_LTAG_PREDICATE(SYMBOL,symbol)
    MK_LTAG_PREDICATE(OBJECT,obj)
    MK_LTAG_PREDICATE(CVALUE,cval)

inline bool isdirect(val_t v) { return !v || ltag(v) == DIRECT ; }
inline bool hastype(val_t v, tpkey_t tk) { return tpkey(v) == tk ; }
inline bool isempty(val_t v) { return (v & PTR_MASK) ? false : true ; }

inline bool isallocated(val_t v, type_t* to)
{
  if (!to)
    to = val_type(v);

  if (to->tp_isalloc)
    return to->tp_capi->isalloc(to,v);

  return false;
}

val_t trace_fptr(val_t v)
{
  if (isallocated(v, NULL))
    while (isfptr(car_(v)))
      v = cdr_(v);

  return v;
}


val_t _update_fptr(val_t* v)
{
  if (isallocated(*v,NULL))
    while (isfptr(car_(*v)))
      *v = cdr_(*v);

  return *v;
}


void val_prn(val_t v, riostrm_t* f)
{
  type_t* to = val_type(v);
  
  if (to->tp_capi->prn)
      to->tp_capi->prn(v,f);

  else if (isdirect(v))
    fprintf(f,"<#%s-%u>",to->name,unpad(v));

  else
    fprintf(f,"<#%s-%p>",to->name,ptr(void*,v));

  return;
}


bool val_eqv(val_t x, val_t y)
{
  tpkey_t tx = tpkey(x), ty = tpkey(y);
  
  if (tx == ty)
    {
      return GLOBAL_TYPES[tx]->tp_capi->ord(x,y) == 0;
    }

  return false;
}

int32_t val_finalize(type_t* to, val_t x)
{
  if (isdirect(x))
    return 0;

  else if (to->tp_capi->finalize)
    if (to->tp_capi->finalize(to,x))
      {
	fprintf(stderr,"Exiting due to failure during finalization.");
	exit(EXIT_FAILURE);
      }

  void* mem = ptr(void*, x);
  vm_cfree(mem);
  return 0;
}

void* val_init_head(type_t* to, size_t sz, void* dest)
{
  obj_t* no = (obj_t*)dest;
  no->type = to->tp_tpkey;
  no->cmeta = 0;

  switch (to->tp_sizing)
    {
      default:
        break;

      case SMALL_LEN:
        no->cmeta = sz;
	break;

     case WIDE_LEN:
       no->data[0] = sz;
       break;
    }

  return dest + to->tp_init_sz;
}


val_t datatype_call(type_t* to, val_t* args, size_t argc)
{
  val_t new;
  capi_t* capi = to->tp_capi;

  if (to->tp_ltag == DIRECT)
    {
      if (to->tp_tpkey <= INTEGER)
	new = capi->builtin_new((val_t)args, argc);

      else
	new = capi->new(to,(val_t)args, argc);
    }

  else if (to->tp_tpkey <= SYMTAB)
	new = capi->builtin_new((val_t)args,argc);

  else
    new = capi->new(to,(val_t)args,argc);

  return new;
}

capi_t DATATYPE_CAPI =
  {
    .prn = NULL,
    .call = datatype_call,
    .size = NULL,
    .elcnt = NULL,
    .hash = NULL,
    .ord = NULL,
    .new = NULL,
    .init = NULL,
    .relocate = NULL,
    .isalloc = NULL,
  };

type_t DATATYPE_TYPE_OBJECT =
  {
    .type        = NIL,
    .cmeta       = DATATYPE,
    .tp_sizing   = FIXED,
    .tp_init_sz  = 8,
    .tp_base_sz  = sizeof(type_t),
    .tp_nfields  = 0,
    .tp_cvtable  = NULL,
    .tp_capi     = &DATATYPE_CAPI,
    .name        = "type",
  };
