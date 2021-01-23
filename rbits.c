#include "rascal.h"

/* tag manipulation, type testing, pointer tracing */
// get the type object associated with a value

inline vtag_t vtag(val_t v)
{
  ntag_t n = ntag(v);
  return n == NTAG_DIRECT ? wtag(v) : n;
}

inline otag_t otag(val_t v)
{
  switch (ntag(v))
    {
    case NTAG_VEC ... NTAG_BIGNUM:
      return otag_(v,0xff);
    case NTAG_METHOD ... NTAG_OBJECT:
      return otag_(v,0xf);
    default:
      return OTAG_NONE;
    }
}

inline chr8_t*  val_tpname(val_t v)             { return val_type(v)->name; }
inline table_t* val_fields(val_t v)             { return val_type(v)->tp_fields; }
inline size_t   val_nfields(val_t v)            { return val_type(v)->tp_fields->count; }
inline c_num_t  val_cnum(val_t v)               { return val_type(v)->tp_cnum; }
inline c_num_t  common_cnum(val_t x, val_t y)   { return val_cnum(x) | val_cnum(y) ; }
inline size_t   val_tpbase(val_t x)             { return val_type(x)->tp_base; }

/* predicates */
inline bool isdirect(val_t v) { return !v || ntag(v) == NTAG_DIRECT; }
inline bool isatom(val_t v) { return val_type(v)->tp_atomic_p; }
inline bool iscvalue(val_t v)
{
  ntag_t t = ntag(v);
  return t == NTAG_IOSTRM || NTAG_CPRIM ? true : false;
}

inline bool hasmethflags(val_t v)
{
  ntag_t t = ntag(v);
  return t == NTAG_METHOD || t == NTAG_CPRIM;
}

inline bool ispair(val_t v)
{
  if (v)
    {
        ntag_t t = ntag(v);
	return t == NTAG_LIST || t == NTAG_CONS;
    }

  return false;
}

inline bool tab_nmspc(table_t* t)       { return (t->otag & 0x70) == WTAG_NMSPC    ; }
inline bool vec_envt(vec_t* v)          { return (v->otag & 0x70) == WTAG_ENVT     ; }
inline bool vec_contfrm(vec_t* v)       { return v->otag == OTAG_CONTFRM           ; }
inline bool tab_symtab(table_t* t)      { return t->otag == OTAG_SYMTAB            ; }
inline bool vec_bytecode(vec_t* v)      { return v->otag == OTAG_BYTECODE          ; }
inline bool tab_readtab(table_t* t)     { return t->otag == OTAG_READTAB           ; }
inline bool tab_modnmspc(table_t* t)    { return t->otag == OTAG_MODNMSPC          ; }
inline bool vec_modenvt(vec_t* v)       { return v->otag == OTAG_MODENVT           ; }
inline bool val_nmspc(val_t v)          { return istable(v) && otag_(v,0x70) == WTAG_NMSPC ; }
inline bool val_envt(val_t v)           { return isvec(v) && otag_(v,0x70) == WTAG_ENVT  ; }
inline bool val_contfrm(val_t v)        { return isvec(v) && otag_(v,0xff) == OTAG_CONTFRM ; }
inline bool val_symtab(val_t v)         { return istable(v) && otag_(v,0xff) == OTAG_SYMTAB ; }
inline bool val_bytecode(val_t v)       { return isvec(v)   && otag_(v,0xff) == OTAG_BYTECODE ; }
inline bool val_readtab(val_t v)        { return istable(v) && otag_(v,0xff) == OTAG_READTAB      ; }
inline bool val_modnmspc(val_t v)       { return istable(v) && otag_(v,0xff) == OTAG_MODNMSPC ; }
inline bool val_modenvt(val_t v)        { return isvec(v)   && otag_(v,0xff) == OTAG_MODENVT  ; }


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

type_t* val_type(val_t v)
{
  int t = vtag(v);
  if (t < VTAG_OBJECT)
    return CORE_TYPES[v && true][t];
  else if (t > VTAG_OBJECT)
    return CORE_DIRECT_TYPES[(t>>4)];
  else
    return otype_(v);
}

size_t val_size(val_t v)
{
  if (isdirect(v)) return 8;

  type_t* to = val_type(v);
  if (to->tp_size) return to->tp_size(v);
  else return tp_base(to);
}

size_t val_asize(val_t v)
{
  if (isdirect(v))
    return 8;
  size_t bs = max(val_size(v),16u);
  while (bs % 16) bs++;

  return bs;
}

val_t val_itof(val_t v)
{
  rsp_c64_t cv = { .value = v };
  cv.padded.bits.float32 = (float)cv.padded.bits.integer;
  cv.padded.tag = VTAG_FLOAT;
  return cv.value;
}

val_t val_ftoi(val_t v)
{
  rsp_c64_t cv = { .value = v };
  cv.padded.bits.integer = (int)cv.padded.bits.float32;
  cv.padded.tag = VTAG_INT;
  return cv.value;
}

c_num_t vm_promote(val_t* x, val_t* y)
{
  c_num_t xn = common_cnum(*x,*y);
  if (xn == CNUM_FLOAT32)
    {
      *x = val_itof(*x);
      *y = val_itof(*y);
    }
  
  return xn;
}

/* safety functions */
#define SAFECAST_PTR(ctype,rtype,test)	                                       \
  ctype to ## rtype(const chr8_t* fl, int32_t ln, const chr8_t* fnc,val_t v)   \
  {                                                                            \
    if (!test(v))                                                              \
  {                                                                            \
    rsp_vperror(fl,ln,fnc,TYPE_ERR,TYPEERR_FMT,#rtype,val_tpname(v)) ;         \
    rsp_raise(TYPE_ERR) ;						       \
    return (ctype)0;                                                           \
  }                                                                            \
    return ptr(ctype,trace_fp(v));					       \
  }

#define SAFECAST_VAL(ctype,rtype,test,cnvt)				       \
  ctype to ## rtype(const chr8_t* fl, int32_t ln, const chr8_t* fnc,val_t v)   \
  {                                                                            \
    if (!test(v))                                                              \
  {                                                                            \
    rsp_vperror(fl,ln,fnc,TYPE_ERR,TYPEERR_FMT,#rtype,val_tpname(v)) ;         \
    rsp_raise(TYPE_ERR) ;						       \
    return (ctype)0;                                                           \
  }                                                                            \
    return (ctype)cnvt(v);						       \
  }

// fucking all of the safecast macros
SAFECAST_PTR(list_t*,list,islist)
SAFECAST_PTR(cons_t*,cons,iscons)
SAFECAST_PTR(chr8_t*,str,isstr)
SAFECAST_PTR(iostrm_t*,iostrm,isiostrm)
SAFECAST_PTR(sym_t*,sym,issym)
SAFECAST_PTR(bstr_t*,bstr,isbstr)
SAFECAST_PTR(node_t*,node,isnode)
SAFECAST_PTR(vec_t*,vec,isvec)
SAFECAST_PTR(table_t*,table,istable)
SAFECAST_PTR(method_t*,method,ismethod)
SAFECAST_PTR(type_t*,type,istype)
SAFECAST_PTR(cprim_t*,cprim,iscprim)
SAFECAST_PTR(func_t*,func,hasmethflags)
SAFECAST_VAL(rint_t,int,isint,ival)
SAFECAST_VAL(rflt_t,float,isfloat,fval)
SAFECAST_VAL(rchr_t,char,ischar,cval)
SAFECAST_VAL(rbool_t,bool,isbool,bval)
SAFECAST_PTR(code_t*,code,val_bytecode)
SAFECAST_PTR(envt_t*,envt,val_envt)
SAFECAST_PTR(symtab_t*,symtab,val_symtab)
SAFECAST_PTR(nmspc_t*,nmspc,val_nmspc)
SAFECAST_PTR(readtab_t*,readtab,val_readtab)
SAFECAST_PTR(cfrm_t*,contfrm,val_contfrm)
SAFECAST_PTR(cons_t*,pair,ispair)


inline bool cbool(rbool_t b)
{
  if (!b) return false;
  else if (b > 0) return true;
  else return false;
}

inline int32_t ordflt(flt32_t x, flt32_t y)
{
  if (x == y) return 0;
  else if (x < y) return -1;
  else return 1;
}

inline int32_t ordint(int32_t x, int32_t y)
{
  if (x == y) return 0;
  else if (x < y) return -1;
  else return 1;
}

inline int32_t ordhash(hash32_t x, hash32_t y)
{
  if (x == y) return 0;
  else if (x < y) return -1;
  else return 1;
}

inline int32_t ordmem(uchr8_t* bx, size_t xsz, uchr8_t* by, size_t ysz)
{
  size_t stop = min(xsz,ysz);
  int32_t c = memcmp(bx,by,stop);
  if (!c) return c;
  else if (c < 0) return -1;
  else return 1;
}

inline int32_t ordstrval(val_t sx, val_t sy)
{
  return u8strcmp(ptr(chr8_t*,sx),ptr(chr8_t*,sy));
}


inline int32_t ordbval(val_t bx, val_t by)
{
  bstr_t* bsx = ptr(bstr_t*, bx), *bsy = ptr(bstr_t*, by);
  return ordmem(bsx->bytes,bsx->size,bsy->bytes,bsy->size);
}


inline int32_t ordsymval(val_t sx, val_t sy)
{
  sym_t* smx = ptr(sym_t*,sx), *smy = ptr(sym_t*,sy);
  return ordhash(smx->hash,smy->hash) || u8strcmp(smx->name,smy->name);
}


int32_t vm_ord(val_t x, val_t y)
{
  if (!isatom(x) || !isatom(y)) rsp_raise(TYPE_ERR);
  type_t* tx = val_type(x), *ty = val_type(y);

  if (tx != ty) return ordhash(tx->tp_hash,ty->tp_hash);
  else if (tx->tp_ord) return tx->tp_ord(x,y);
  else if (tx->tp_cnum == CNUM_FLOAT32) return ordflt(fval(x),fval(y));
  else return ordint(ival(x),ival(y));
}
