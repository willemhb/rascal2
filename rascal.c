#include "rascal.h"


/* tag manipulation */
// extract the tag and the address
inline int ltag(val_t v) { int nt = ntag(v); return nt <= 0xf ? nt : (int)wtag(v); }
inline bool isdirect(val_t v)
{
  int t = ltag(v);
  return t > 0xf ? t != LTAG_BUILTIN : !v;
}

inline bool iscallable(val_t v)
{
  int t = ltag(v);
  return t == LTAG_FUNCOBJ || t == LTAG_BUILTIN || t == LTAG_METAOBJ;
}

inline bool isatomic(val_t v) { return get_val_type(v)->tp_atomic; }

inline bool isnumeric(val_t v)
{
  int t = ltag(v);
  return t == LTAG_BIGNUM || t == LTAG_INT || t == LTAG_FLOAT;
}

inline bool hasflags(val_t v) {
  switch(ltag(v)) {
  case LTAG_SYM:
  case LTAG_DVEC:
  case LTAG_TABLEOBJ:
  case LTAG_OBJECT:
  case LTAG_FUNCOBJ:
    return true;
  default:
    return false;
  }
}

inline int oflags(val_t v) { return hasflags(v) ? flags_(v) : FLAG_NONE; }

// get the type object associated with a value
type_t* get_val_type(val_t v) {
  int t = ltag(v);

  if (t == LTAG_LIST) return (t ? CORE_OBJECT_TYPES : CORE_DIRECT_TYPES)[t];
  else if (t < LTAG_OBJECT) return CORE_OBJECT_TYPES[t];
  else if (t > LTAG_METAOBJ) return CORE_DIRECT_TYPES[t>>4];
  else return (type_t*)(type_(v) & (~0xful));
}

int get_val_size(val_t v) {
  if (isdirect(v)) return 8;

  type_t* to = get_val_type(v);
  
  if (to->val_fixed_size) return to->val_base_size;

  else return to->tp_val_size(v); 
}

int val_len(val_t v)
{
  switch (ltag(v))
    {
    case LTAG_CONS: return 2;
    case LTAG_LIST:
      {
	list_t* lv = tolist_(v);
	size_t l = 0;
	while (lv)
	  {
	    l++; lv = rest_(lv);
	  }

	return l;
      }
    case LTAG_FVEC: return fvec_allocated_(v);
    case LTAG_DVEC: return dvec_curr_(v);
    case LTAG_STR: return u8strlen(tostr_(v));
    case LTAG_TABLEOBJ: return count_(v);
    default:
      {
	rsp_raise(TYPE_ERR);
	return 0;
      }
    }
}

val_t rsp_itof(val_t v) {
  rsp_c64_t cv = { .value = v };
  cv.direct.data.float32 = (float)cv.direct.data.integer;
  cv.direct.lowtag = LTAG_FLOAT;
  return cv.value;
}

val_t rsp_ftoi(val_t v) {
  rsp_c64_t cv = { .value = v };
  cv.direct.data.integer = (int)cv.direct.data.float32;
  cv.direct.lowtag = LTAG_INT;
  return cv.value;
}

inline char* get_val_type_name(val_t v) { return get_val_type(v)->name; }
inline c_num_t get_val_cnum(val_t v) { return get_val_type(v)->val_cnum; }
inline c_num_t get_common_cnum(val_t x, val_t y) { return get_val_cnum(x) | get_val_cnum(y) ; }

/* safety functions */
#define SAFECAST(ctype,rtype,test,cnvt)	                                       \
  ctype _to ## rtype(val_t v,const char* fl, int ln, const char* fnc)          \
  {                                                                            \
    if (!test(v))                                                              \
  {                                                                            \
    _rsp_perror(fl,ln,fnc,TYPE_ERR,TYPEERR_FMT,#rtype,get_val_type_name(v)) ;  \
    rsp_raise(TYPE_ERR) ;						       \
    return (ctype)0;                                                           \
  }                                                                            \
   return cnvt(safe_ptr(v));						       \
  }

SAFECAST(list_t*,list,islist,tolist_)
SAFECAST(cons_t*,cons,iscons,tocons_)
SAFECAST(char*,str,isstr,tostr_)
SAFECAST(FILE*,cfile,iscfile,tocfile_)
SAFECAST(sym_t*,sym,issym,tosym_)
SAFECAST(node_t*,node,isnode,tonode_)
SAFECAST(fvec_t*,fvec,isfvec,tofvec_)
SAFECAST(dvec_t*,dvec,isdvec,todvec_)
SAFECAST(code_t*,code,iscode,tocode_)
SAFECAST(obj_t*,obj,isobj,toobj_)
SAFECAST(table_t*,table,istable,totable_)
SAFECAST(function_t*,function,isfunction,tofunction_)
SAFECAST(type_t*,type,istype,totype_)
SAFECAST(int,rint,isint,toint_)
SAFECAST(float,rfloat,isfloat,tofloat_)
SAFECAST(wchar_t,rchar,ischar,tochar_)
SAFECAST(rsp_cfunc_t,builtin,isbuiltin,tobuiltin_)

/* safecasts and checking forwarding pointers */
// follow a forwarding pointer without updating it
val_t safe_ptr(val_t v) {
  if (!isobj(v)) return v;
  while (tocons_(v)->car == R_FPTR) v = tocons_(v)->cdr;
  return v;
}

val_t check_fptr(val_t* v) {
  if (!isobj(*v)) return *v;
  return update_fptr(v);
}

val_t update_fptr(val_t* v) {
  // follow the forwarding pointer possibly more than once
  while (tocons_(*v)->car == R_FPTR) *v = tocons_(*v)->cdr;

  return *v;
}

inline bool cbool(val_t v) {
  if (v == R_NIL) return false;
  if (v == R_NONE) return false;
  if (v == R_FALSE) return false;
  return true;
}

int vm_ord(val_t x, val_t y)
{
  if (!isatomic(x) || !isatomic(y)) rsp_raise(TYPE_ERR);
  type_t* tx = get_val_type(x), *ty = get_val_type(y);

  if (tx != ty) return compare((val_t)tx,(val_t)ty);

  switch (tx->val_ltag) {
  case LTAG_STR:
    return u8strcmp(tostr_(x),tostr_(y));
  case LTAG_SYM:
    return compare(symhash_(x),symhash_(y)) ?: u8strcmp(symname_(x),symname_(y));
  case LTAG_INT:
    return compare(toint_(x),toint_(y));
  case LTAG_CHAR:
    return compare(tochar_(x),tochar_(y));
  case LTAG_FLOAT:
    return compare(tofloat_(x),tofloat_(y));
  default:
    return compare(x,y);
  }
}
