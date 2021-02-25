#ifndef value_h
#define value_h
#include "rascal.h"
#include "error.h"

// checking tags, getting type information
uint32_t   ltag(val_t);
tpkey_t    tpkey(val_t);
val_t      addr(val_t);
void*      vptr(val_t);
rsp_c32_t  value(val_t);
val_t      pad(rsp_c32_t);
uint32_t   unpad(val_t);
bool       p_aligned(const void*);
bool       v_aligned(val_t);

#define aligned(p) _Generic((p), val_t:v_aligned, default:p_aligned)(p)

obj_t*     objhead(val_t);

#define ptr(t,v)             ((t)addr(v))
#define uval(v)              (((rsp_c64_t)(v)).padded.bits.bits32)
#define ival(v)              (((rsp_c64_t)(v)).padded.bits.integer)
#define fval(v)              (((rsp_c64_t)(v)).padded.bits.float32)
#define bval(v)              (((rsp_c64_t)(v)).padded.bits.boolean)
#define cval(v)              (((rsp_c64_t)(v)).padded.bits.unicode)

// general unsafe accessors
#define car_(v)              (ptr(val_t*,(val_t)(v))[0])
#define cdr_(v)              (ptr(val_t*,(val_t)(v))[1])

val_t     tag_from_type(rsp_c64_t,type_t*);
val_t     tag_from_tpkey(rsp_c64_t, tpkey_t);

#define tag(v,k)                                                  \
  _Generic((k), type_t*:tag_from_type,                            \
	        int32_t:tag_from_tpkey)((rsp_c64_t)(v).bits64,k)

type_t*  val_type(val_t);
type_t*  tk_type(tpkey_t);
chr_t*   val_typename(val_t);
chr_t*   tk_typename(tpkey_t);

#define get_type(v)   _Generic((v),val_t:val_type,tpkey_t:tk_type)(v)
#define get_tpname(v) _Generic((v),val_t:val_typename,tpkey_t:tk_typename)(v)

// top-level functions for hashing and sizing
size_t   val_sizeof(val_t,type_t*);
size_t   val_nwords(val_t,type_t*);
hash_t   val_hash(val_t);
hash_t   val_rehash(val_t,uint32_t);
int32_t  val_eql(val_t,val_t);
void     val_prn(val_t,riostrm_t*);
int32_t  val_finalize(type_t*,val_t);

// global value predicates
bool isnil(val_t);
bool istrue(val_t);
bool isfalse(val_t);
bool isfptr(val_t);
bool isunbound(val_t);
bool isreof(val_t);

// tag predicates
bool ispair(val_t);
bool islist(val_t);
bool isiostrm(val_t);
bool issymbol(val_t);
bool isfunction(val_t);
bool isobj(val_t);
bool iscval(val_t);
bool isdirect(val_t);

// other predicates
bool hastype(val_t,tpkey_t);
bool isempty(val_t);
bool isallocated(val_t,type_t*);

// global initializer functions
void* fval_init_head(type_t*,void*);
void* vval_init_head(type_t*,size_t,void*);

// handling forwarding pointers and checking location
val_t trace_fptr(val_t);
val_t _update_fptr(val_t*);

#define update_fptr(v) _update_fptr(&(v))

// the api for type objects also goes here
extern type_t DATATYPE_TYPE_OBJECT;

val_t datatype_call(type_t*,val_t*,size_t);

#endif
