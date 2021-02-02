#include "../include/rsp_core.h"
#include "../include/describe.h"

/* tag manipulation, type testing, pointer tracing */
inline uint32_t ltag(val_t v) { return v & LTAG_MASK ; }

tpkey_t v_tpkey(val_t v)
{
  if (!(v & PTR_MASK))
    return NIL;

  uint32_t n = ltag(v);
  switch (n)
    {
      case DIRECT:
        return n >> 3;

    case LIST: case IOSTRM:
      return n;

    case OBJHEAD:
      return (v & HIGHMASK) >> 32;
	
    case OBJ: default:
      v = ptr(val_t*,v)[0];

      if ((v & 0x7) == OBJHEAD)
	return (v & HIGHMASK) >> 32;

      else
	return PAIR;
    }
}

inline tpkey_t o_tpkey(obj_t* o)
{
  return o->obj_tpkey;
}

inline tpkey_t g_tpkey(void* v)
{
  if (!v)
    return NIL;

  else
    return ((obj_t*)v)->obj_tpkey;
}

inline type_t* val_type(val_t v) { return GLOBAL_TYPES[tpkey(v)]; }
inline chr_t*  val_typename(val_t v) { return val_type(v)->name; }
inline chr_t*  tp_typename(type_t* to) { return to->name; }

/* predicates */
inline bool isobj(val_t v)
{
  if (v & PTR_MASK)
    switch (v & 0x7)
      {
      case OBJ: case LIST:
	return true;
      default:
	return false;
      }

  else
    return false;
}


EQUALITY_PREDICATE(nil,R_NIL)
EQUALITY_PREDICATE(true,R_TRUE)
EQUALITY_PREDICATE(false,R_FALSE)
EQUALITY_PREDICATE(unbound,R_UNBOUND)
EQUALITY_PREDICATE(fptr,R_FPTR)
EQUALITY_PREDICATE(reof,R_EOF)
TPKEY_PREDICATE_SINGLE(pair,PAIR)
TPKEY_PREDICATE_SINGLE(list,LIST)
TPKEY_PREDICATE_SINGLE(iostrm,IOSTRM)
TPKEY_PREDICATE_SINGLE(bool,BOOL)
TPKEY_PREDICATE_SINGLE(char,CHAR)
TPKEY_PREDICATE_SINGLE(int,INT)
TPKEY_PREDICATE_SINGLE(float,FLOAT)
TPKEY_PREDICATE(str,STR)
TPKEY_PREDICATE(bstr,BSTR)
TPKEY_PREDICATE(ntuple,NTUPLE)
TPKEY_PREDICATE(atom,ATOM)
TPKEY_PREDICATE(set,SET)
TPKEY_PREDICATE(dict,DICT)
TPKEY_PREDICATE(type,TYPE)
TPKEY_RANGE_PREDICATE(tuple,FTUPLE,INODE)
TPKEY_RANGE_PREDICATE(btuple,BTUPLE,INODE)
TPKEY_RANGE_PREDICATE(table,TABLE,NMSPC)


inline bool isa(val_t v, type_t* to) { return val_type(v) == to ; }
inline bool isempty(val_t v) { return v & PTR_MASK; }

bool isallocated(val_t v)
{
  switch(ntag(v))
    {
    case IOSTRM:
    case DIRECT:
      return false;

    default:
      if (v & PTR_MASK)
	{
	  return in_heap(trace_fp(v),RAM,HEAPSIZE);
	}

      else
	return false;
    }
}

val_t update_fp(val_t* v)
{
  while (isallocated(*v) && car_(*v) == R_FPTR)
    *v = cdr_(*v);

  return *v;
}

val_t trace_fp(val_t v)
{
  while (isallocated(v) && car_(v) == R_FPTR)
    v = cdr_(v);
  return v;
}

size_t rsp_size(val_t v)
{
  if (isallocated(v))
    {
      type_t* to = val_type(v);
      size_t bs = to->tp_base_size;
      if (to->tp_size)
	bs += to->tp_size(ptr(obj_t*,v));

      return bs;
    }

  else
    return 8;
}

size_t rsp_elcnt(val_t v)
{
  if (isallocated(v))
    {
      type_t* to = val_type(v);

      if (to->tp_elcnt)
	return to->tp_elcnt(ptr(obj_t*,v));

      else
	return 0;
    }

  else
    return 0;
}

val_t* rsp_fields(val_t v)
{
  if (isdirect(v))
    return NULL;

  else
    {
      type_t* to = val_type(v);
      if (to->tp_nfields)
	return to->tp_get_data(ptr(obj_t*,v));

      else
	return NULL;
    }
}

val_t* rsp_elements(val_t v)
{
  if (isdirect(v))
    return NULL;

  else
    {
      type_t* to = val_type(v);
      if (to->tp_nfields)
	return to->tp_nfields + to->tp_get_data(ptr(obj_t*,v));

      else
	return NULL;
    }
}

size_t rsp_asize(val_t v)
{
  if (isdirect(v))
    return 8;

  size_t bs = max(rsp_size(v),16u);
  while (bs % 16) bs++;

  return bs;
}

// fucking all of the safecast macros
SAFECAST_PTR(list_t*,list,islist)
SAFECAST_PTR(pair_t*,pair,ispair)
SAFECAST_PTR(str_t*,str,v_isstr)
SAFECAST_PTR(bstr_t*,bstr,v_isbstr)
SAFECAST_PTR(iostrm_t*,iostrm,isiostrm)
SAFECAST_PTR(atom_t*,atom,v_isatom)
SAFECAST_PTR(ftuple_t*,tuple,v_istuple)
SAFECAST_PTR(tuple_t*,ntuple,v_isntuple)
SAFECAST_PTR(btuple_t*,btuple,v_isbtuple)
SAFECAST_PTR(table_t*,table,v_istable)
SAFECAST_PTR(set_t*,set,v_isset)
SAFECAST_PTR(dict_t*,dict,v_isdict)
SAFECAST_PTR(type_t*,type,v_istype)
SAFECAST_VAL(rint_t,int,isint,ival)
SAFECAST_VAL(rflt_t,float,isfloat,fval)
SAFECAST_VAL(rchr_t,char,ischar,cval)
SAFECAST_PTR(obj_t*,obj,isobj)


inline bool cbool(val_t b)
{
  if (!b)
    return false;

  else if (isbool(b))
    return bval(b);

  else
    return true;
}

hash32_t rsp_hash(val_t v)
{
  type_t* to = val_type(v);
  hash32_t h = to->tp_hash_val(v);
  return rehash(h,to->tp_hash);
}

int32_t rsp_ord(val_t x, val_t y)
{
  uint32_t tkx = tpkey(x), tky = tpkey(y);

  int32_t rslt;
  
  if (tkx != tky)
    {
      rslt = compare(tkx,tky);
    }

  else
    rslt = GLOBAL_TYPES[tkx]->tp_ord(x,y);

  return rslt;
}


void rsp_prn(val_t v, val_t strm)
{
  iostrm_t* io = ecall(toiostrm,strm);
  vm_prn(v,io);
  return;
}


void vm_prn(val_t v, iostrm_t* s)
{
  type_t* to = val_type(v);

  if (to->tp_prn)
    to->tp_prn(v,s);

  else
    prn_value(v,s);
}

val_t vm_tag(type_t* to, val_t v)
{
  return tag_p(v,to->tp_vtag);
}

void prn_value(val_t v, iostrm_t* f) // fallback printer
{
  char* tn =type_name(v);
  if (isdirect(v))
    fprintf(f,"#<%s:direct:%u>",tn,(unsigned)unpad(v));

  else
    fprintf(f,"#<%s:object:%p>",tn,ptr(void*,v));

  return;
}
