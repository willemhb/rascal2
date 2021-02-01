#include "../include/rsp_core.h"

/* tag manipulation, type testing, pointer tracing */
inline uint32_t ltag(val_t v) { return v & LTAG_MASK ; }

uint32_t tpkey(val_t v)
{
  if (!(v & PTR_MASK))
    return NIL_TP;

  uint32_t n = ltag(v);
  switch (n)
    {
      case DIRECT:
        return n >> 3;

      case LIST:
	return LIST_TP;

      case IOSTRM:
	return IOSTRM_TP;

      case OBJHEAD:
	return (v & 0xf8u) + NUM_DIRECT_TYPES;
	
    case OBJ: default:
      v = ptr(val_t*,v)[0];

      if ((v & 0x7) == OBJHEAD)
	return (v & 0xf8u) + NUM_DIRECT_TYPES;

      else
	return PAIR_TP;
    }
}

inline type_t* val_type(val_t v) { return GLOBAL_TYPES[tpkey(v)]; }
inline chr_t*  val_typename(val_t v) { return val_type(v)->name; }
inline chr_t*  tp_typename(type_t* to) { return to->name; }

/* predicates */

#define EQUALITY_PREDICATE(nm,value) inline bool is ##nm(val_t v) { return v == value ; }

EQUALITY_PREDICATE(nil,R_NIL)
EQUALITY_PREDICATE(true,R_TRUE)
EQUALITY_PREDICATE(false,R_FALSE)
EQUALITY_PREDICATE(unbound,R_UNBOUND)
EQUALITY_PREDICATE(fptr,R_FPTR)
EQUALITY_PREDICATE(reof,R_EOF)

#define TPKEY_PREDICATE(nm,tv) inline bool is##nm(val_t v) { return tpkey(v) == tv; }

TPKEY_PREDICATE(pair,PAIR_TP)
TPKEY_PREDICATE(list,LIST_TP)
TPKEY_PREDICATE(str,STR_TP)
TPKEY_PREDICATE(bstr,BSTR_TP)
TPKEY_PREDICATE(tuple,TUPLE_TP)
TPKEY_PREDICATE(atom,ATOM_TP)
TPKEY_PREDICATE(set,SET_TP)
TPKEY_PREDICATE(dict,DICT_TP)
TPKEY_PREDICATE(iostrm,IOSTRM_TP)
TPKEY_PREDICATE(bool,BOOL_TP)
TPKEY_PREDICATE(char,CHAR_TP)
TPKEY_PREDICATE(int,INT_TP)
TPKEY_PREDICATE(float,FLOAT_TP)
TPKEY_PREDICATE(type,TYPE_TP)


inline bool isa(val_t v, type_t* to) { return val_type(v) == to ; }
inline bool isobj(val_t v) { return isallocated(v); }
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

/* safety functions */
#define SAFECAST_PTR(ctype,rtype,test)	                                       \
  ctype to ## rtype(const chr_t* fl, int32_t ln, const chr_t* fnc,val_t v)     \
  {                                                                            \
    if (!test(v))                                                              \
  {                                                                            \
    rsp_vperror(fl,ln,fnc,TYPE_ERR,rsp_efmt(TYPE_ERR),#rtype,type_name(v)) ;   \
    rsp_raise(TYPE_ERR) ;						       \
    return (ctype)0;                                                           \
  }                                                                            \
    return ptr(ctype,trace_fp(v));					       \
  }

#define SAFECAST_VAL(ctype,rtype,test,cnvt)                                    \
  ctype to ## rtype(const chr_t* fl, int32_t ln, const chr_t* fnc,val_t v)     \
  {                                                                            \
    if (!test(v))                                                              \
  {                                                                            \
    rsp_vperror(fl,ln,fnc,TYPE_ERR,rsp_efmt(TYPE_ERR),#rtype,type_name(v)) ;   \
    rsp_raise(TYPE_ERR) ;						       \
    return (ctype)0;                                                           \
  }                                                                            \
    return cnvt(v);							       \
  }

// fucking all of the safecast macros
SAFECAST_PTR(list_t*,list,islist)
SAFECAST_PTR(pair_t*,pair,ispair)
SAFECAST_PTR(str_t*,str,isstr)
SAFECAST_PTR(bstr_t*,bstr,isbstr)
SAFECAST_PTR(iostrm_t*,iostrm,isiostrm)
SAFECAST_PTR(atom_t*,atom,isatom)
SAFECAST_PTR(tuple_t*,tuple,istuple)
SAFECAST_PTR(set_t*,set,isset)
SAFECAST_PTR(dict_t*,dict,isdict)
SAFECAST_PTR(type_t*,type,istype)
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
