#ifndef rascal_h

/* begin rascal.h */
#define rascal_h
#include "common.h"
#include "rtypes.h"
#include "globals.h"
#include "capi.h"
#include "opcodes.h"

/* 

   general utilities - util/ 
 
 */

// stack manipulating functions
val_t  push(val_t);         
val_t  pushn(size_t);
val_t  pop();
val_t  popn(size_t);

// numeric utilities
uint32_t cpow2_32(int32_t);
uint64_t cpow2_64(int64_t);
uint64_t clog2(uint64_t);

// string and character utilities
hash32_t hash_string(const chr_t*);
hash32_t hash_bytes(const uchr_t*,size_t);
hash32_t rehash(hash32_t,uint32_t);
int32_t strsz(const chr_t*);
int32_t u8strlen(const chr_t*);
int32_t u8strcmp(const chr_t*,const chr_t*);
int32_t u8len(const chr_t*);
int32_t u8tou32(chr32_t*,const chr_t*);
ichr32_t nextu8(const chr_t*);
ichr32_t nthu8(const chr_t*,size_t);
int32_t iswodigit(ichr32_t);
int32_t iswbdigit(ichr32_t);

// io utilities
int32_t peekwc(iostrm_t*);
int32_t fskip(iostrm_t*);
int32_t fskipto(iostrm_t*,ichr32_t);

// basic error handling
void rsp_savestate(rsp_ectx_t*);
void rsp_raise(rsp_err_t);
const char* rsp_errname(rsp_err_t);
const chr_t* rsp_efmt(rsp_err_t);
void rsp_vperror(const chr_t*, int32_t, const chr_t*, rsp_err_t, ...);
rsp_ectx_t* rsp_restorestate(rsp_ectx_t*);

/* util/rbits.c */
// checking tags, getting type information
uint32_t ltag(val_t);
tpkey_t  v_tpkey(val_t);
tpkey_t  o_tpkey(obj_t*);
tpkey_t  g_tpkey(void*);
uint8_t  vmflags(obj_t*);
type_t*  val_type(val_t);              // get the type object
type_t*  tk_type(tpkey_t);
size_t   val_size(val_t);              // get the size of a value
chr_t*   val_typename(val_t);
chr_t*   tp_typename(type_t*);
chr_t*   tk_typename(tpkey_t);

#define type_name(v)              \
  _Generic((v),                   \
           type_t*:tp_typename,   \
	   tpkey_t:tk_typename,	  \
	   val_t:val_typename)(v)

#define get_type(v)               \
  _Generic((v),                   \
           tpkey_t:tk_type,       \
           val_t:val_type)(v)

#define tpkey(v)                  \
  _Generic((v),                   \
    val_t:v_tpkey,                \
    obj_t:o_tpkey,                \
	   default:g_tpkey)(v)

// handling fptrs
val_t    trace_fp(val_t);   // recursively follow a forward pointer
val_t    update_fp(val_t*); // recursively update a forward pointer

// global value predicates
bool isnil(val_t);
bool istrue(val_t);
bool isfalse(val_t);
bool isfptr(val_t);
bool isunbound(val_t);
bool isreof(val_t);

// single dispatch type predicates
bool     ispair(val_t);
bool     islist(val_t);
bool     isiostrm(val_t);
bool     isbool(val_t);
bool     ischar(val_t);
bool     isint(val_t);
bool     isfloat(val_t);
bool     isobj(val_t);

// multiple dispatch type predicates
bool     v_istuple(val_t);
bool     v_isntuple(val_t);
bool     v_isbtuple(val_t);
bool     v_isstr(val_t);
bool     v_isbstr(val_t);
bool     v_isatom(val_t);
bool     v_istable(val_t);
bool     v_isset(val_t);
bool     v_isdict(val_t);
bool     v_istype(val_t);
bool     o_istuple(obj_t*);
bool     o_isntuple(obj_t*);
bool     o_isbtuple(obj_t*);
bool     o_isstr(obj_t*);
bool     o_isbstr(obj_t*);
bool     o_isatom(obj_t*);
bool     o_istable(obj_t*);
bool     o_isset(obj_t*);
bool     o_isdict(obj_t*);
bool     o_istype(obj_t*);
bool     g_istuple(void*);
bool     g_isntuple(void*);
bool     g_isbtuple(void*);
bool     g_isstr(void*);
bool     g_isbstr(void*);
bool     g_isatom(void*);
bool     g_istable(void*);
bool     g_isset(void*);
bool     g_isdict(void*);
bool     g_istype(void*);

#define GEN_PRED(v,tp) _Generic((v), val_t:v_is##tp, obj_t*:o_is##tp,default:g_is##tp)(v)

#define istuple(v)  GEN_PRED(v,tuple)
#define isntuple(v) GEN_PRED(v,ntuple)
#define isbtuple(v) GEN_PRED(v,btuple)
#define isstr(v)    GEN_PRED(v,str)
#define isbstr(v)   GEN_PRED(v,bstr)
#define isatom(v)   GEN_PRED(v,atom)
#define istable(v)  GEN_PRED(v,table)
#define isset(v)    GEN_PRED(v,set)
#define isdict(v)   GEN_PRED(v,dict)
#define istype(v)   GEN_PRED(v,type)

// general predicates
bool     isa(val_t,type_t*);
bool     isallocated(val_t);
bool     isdirect(val_t);
bool     isempty(val_t);

// top-level api functions
hash32_t rsp_hash(val_t);
hash32_t rsp_rehash(val_t,uint32_t);
int32_t  rsp_ord(val_t,val_t);         
void     rsp_prn(val_t,val_t);
size_t   rsp_size(val_t);
size_t   rsp_asize(val_t);
size_t   rsp_elcnt(val_t);
val_t*   rsp_fields(val_t);
val_t*   rsp_elements(val_t);
bool     cbool(val_t);                 
void     prn_value(val_t,iostrm_t*);
void     vm_prn(val_t,iostrm_t*);
val_t    vm_tag(type_t*,val_t);

// safecasts
#define SAFECAST_ARGS const chr_t*,int32_t,const chr_t*,val_t

list_t*   tolist( SAFECAST_ARGS );
pair_t*   topair( SAFECAST_ARGS );
str_t*    tostr( SAFECAST_ARGS );
atom_t*   toatom( SAFECAST_ARGS );
iostrm_t* toiostrm( SAFECAST_ARGS );
ftuple_t* totuple( SAFECAST_ARGS );
tuple_t*  tontuple( SAFECAST_ARGS );
btuple_t* tobtuple( SAFECAST_ARGS );
table_t*  totable( SAFECAST_ARGS );
set_t*    toset( SAFECAST_ARGS );
dict_t*   todict( SAFECAST_ARGS );
type_t*   totype( SAFECAST_ARGS );
rint_t    toint( SAFECAST_ARGS );
rbool_t   tobool( SAFECAST_ARGS );
rflt_t    tofloat( SAFECAST_ARGS );
rchr_t    tochar( SAFECAST_ARGS );
obj_t*    toobj( SAFECAST_ARGS );
type_t*   totype(SAFECAST_ARGS );

/* object apis - obj/ */
// memory management (mem.c)
void*    vm_cmalloc(uint64_t);
int32_t  vm_cfree(void*);
void*    vm_crealloc(void*,uint64_t,bool);
void*    vm_alloc(size_t,size_t,size_t);
void*    vm_realloc(val_t,size_t,size_t);
size_t   calc_mem_size(size_t);
bool     gc_check(void);
void     gc_resize(void);
void     gc_run(void);
val_t    gc_trace(val_t);
val_t    gc_copy(type_t*,val_t);
bool     p_in_heap(void*v,uchr_t*h,uint64_t sz);
bool     v_in_heap(val_t v, uchr_t* h, uint64_t sz);

#define in_heap(v,u,sz)                            \
  _Generic((v),                                    \
	   val_t:v_in_heap,                        \
	   default:p_in_heap)(v,u,sz)

// lists, pairs, and tuples (obj.c)
list_t*    mk_list(size_t,val_t*);
pair_t*    mk_pair(val_t,val_t);
ftuple_t*  mk_ftuple(size_t,uint8_t,tpkey_t);
tuple_t*   mk_tuple(size_t,uint8_t,tpkey_t);
btuple_t*  mk_btuple(uint32_t,uint8_t,tpkey_t);
btuple_t*  mk_gl_btuple(uint32_t,uint8_t,tpkey_t);

size_t     _tuple_size(obj_t*);
size_t     _tuple_elcnt(obj_t*);
val_t*     _tuple_data(obj_t*);

#define tuple_size(t)   _tuple_size((obj_t*)t)
#define tuple_elcnt(t)  _tuple_elcnt((obj_t*)t)
#define tuple_data(t)   _tuple_data((obj_t*)t)

val_t     copy_pair(type_t*,val_t,uchr_t**);
val_t     copy_tuple(type_t*,val_t,uchr_t**);
hash32_t  hash_pair(val_t);
hash32_t  hash_tuple(val_t);
int32_t   ord_pair(val_t,val_t);
int32_t   ord_tuple(val_t,val_t);
void      prn_pair(val_t,iostrm_t*);
void      prn_list(val_t,iostrm_t*);
void      prn_tuple(val_t,iostrm_t*);
void      prn_btuple(val_t,iostrm_t*);
list_t*   list_append(list_t**,val_t);

// strings, bstrings and core atoms api (string.c)
str_t*    mk_str(chr_t*);
bstr_t*   mk_bstr(size_t,uchr_t*);
atom_t*   new_atom(chr_t*,size_t,uint16_t,hash32_t);
val_t     copy_str(type_t*,val_t,uchr_t**);
val_t     copy_bstr(type_t*,val_t,uchr_t**);
size_t    str_size(obj_t*);
size_t    bstr_size(obj_t*);
int32_t   ord_str(val_t,val_t);
int32_t   ord_bstr(val_t,val_t);
hash32_t  hash_str(val_t);
hash32_t  hash_bstr(val_t);
hash32_t  hash_atom(val_t);
void      prn_str(val_t,iostrm_t*);
void      prn_bstr(val_t,iostrm_t*);
void      prn_atom(val_t,iostrm_t*);
chr_t*    strval(val_t);

// tables, sets, and dicts (table.c)
table_t*   mk_table(size_t,tpkey_t);
dict_t*    mk_dict(size_t);
set_t*     mk_set(size_t);
val_t      copy_table(type_t*,val_t);
val_t*     tb_lookup(table_t*,val_t);
val_t*     tb_addkey(table_t*,val_t);
val_t*     tb_setkey(table_t*,val_t,val_t);
val_t*     tb_rmvkey(table_t*,val_t);
void       prn_dict(val_t,iostrm_t*);
void       prn_set(val_t,iostrm_t*);
atom_t*    mk_atom(chr_t*,uint16_t);
atom_t*    intern_string(chr_t*,hash32_t,uint16_t);

// callable types

// direct data (direct.c)
val_t     mk_bool(int32_t);
val_t     mk_char(int32_t);
val_t     mk_int(int32_t);
val_t     mk_float(flt32_t);
hash32_t  hash_small(val_t);
void      prn_int(val_t,iostrm_t*);
void      prn_float(val_t,iostrm_t*);
void      prn_char(val_t,iostrm_t*);
void      prn_bool(val_t,iostrm_t*);
int32_t   ord_ival(val_t,val_t);
int32_t   ord_fval(val_t,val_t);
val_t     val_itof(val_t);              // floating point conversion 
val_t     val_ftoi(val_t);              // integer conversion

/* 
   vm.c - environment API, the VM, and the compiler 
 */
int32_t   env_locate(envt_t*,atom_t*,size_t*,size_t*);
pair_t*   env_extend(envt_t*,atom_t*);
size_t    env_bind(envt_t*,val_t);
val_t     env_lookup(tuple_t*,size_t,size_t);
val_t     env_assign(tuple_t*,size_t,size_t,val_t);

// the virtual machine and compiler
int32_t   vm_check_argco();
bool      vm_check_bltn_argco();
val_t     vm_call_bltn(r_cfun_t*,size_t,val_t*);           
opcode_t  vm_fetch_instr();                             
val_t     vm_exec(tuple_t*,tuple_t*);                         
code_t*   vm_compile(val_t,envt_t*,module_t*); 
val_t     vm_expand(val_t,envt_t*,pair_t*);

// reader/printer support functions
rsp_tok_t vm_get_token(iostrm_t*);
val_t     vm_read_expr(iostrm_t*);
val_t     vm_read_cons(iostrm_t*);                   // read a cons or list
val_t     vm_read_bytes(iostrm_t*);                  // optimized reader for bytestring literals
val_t     vm_read_coll(type_t*,iostrm_t*,rsp_tok_t);
void      vm_print(val_t,iostrm_t*);
val_t     vm_load(iostrm_t*,val_t);

/* bltins.c */

/* rspinit.c */
// memory
void init_heap();
void init_types();
void init_registers();

// builtins
void init_symbol_table();
void init_builtin_types();
void init_special_forms();
void init_global_variables();
void init_builtin_functions();

/* toplevel bootstrapping and cleanup functions */
void bootstrap_rascal();
int  teardown_rascal();

/* rspmain.c */
int rsp_repl();
int rsp_main(int32_t,chr_t**);

/*
   convenience macros 

   in general, macros affixed with '_' are unsafe; they don't perform necessary safety checks
   before performing the requested operation. Those safety checks may be superfluous, but in general
   the safe version is preferred.

 */

// compiler hint macros
#define unlikely(x) __builtin_expect((x), 0)
#define likely(x)   __builtin_expect((x), 1)
#define popcount(x) __builtin_popcount(x)

// type-generic min, max, and compare macros
#define min(x,y)                     \
  ({                                 \
    typeof(x) __x__ = x ;            \
    typeof(y) __y__ = y ;            \
    __x__ > __y__ ? __y__ : __x__ ;  \
  })

#define max(x,y)                     \
  ({                                 \
     typeof(x) __x__ = x ;            \
     typeof(y) __y__ = y ;            \
     __x__ < __y__ ? __y__ : __x__ ;  \
  })

#define compare(x,y)                                     \
  ({                                                     \
     typeof(x) __x__ = x;                                \
     typeof(y) __y__ = y;                                \
     (__x__ < __y__ ? -1 : 0) || (__x__ > y ? 1 : 0);    \
  })

/* bit manipulation macros for tagging, untagging, and testing */

#define ntag(v)              ((v) & 0x7)
#define wtag(v)              ((v) & 0xff)
#define addr(v)              (((val_t)(v)) & PTR_MASK)
#define bits(d)              (((rsp_c32_t)(d)).bits32)
#define pad(d)               (((val_t)bits(d)) << 32)
#define unpad(d)             ((d) >> 32)
#define tag_v(v,t)           (pad(v) | (t))
#define tag_p(p,t)           (((val_t)(p)) | (t))
#define ptr(t,v)             ((t)addr(v))
#define uval(v)              (((rsp_c64_t)(v)).padded.bits.bits32)
#define ival(v)              (((rsp_c64_t)(v)).padded.bits.integer)
#define fval(v)              (((rsp_c64_t)(v)).padded.bits.float32)
#define bval(v)              (((rsp_c64_t)(v)).padded.bits.boolean)
#define cval(v)              (((rsp_c64_t)(v)).padded.bits.unicode)
#define aligned(p,n)         (((val_t)(p)) % (n) == 0)

// general unsafe accessor
#define car_(v)              (ptr(val_t*,v)[0])
#define cdr_(v)              (ptr(val_t*,v)[1])

/* allocation macros */
#define vm_allocw(bs,wds)          vm_alloc(bs,wds,8)
#define vm_allocb(bs,nb)           vm_alloc(bs,nb,1)
#define vm_allocc(nc)              vm_alloc(0,16,nc)
#define vm_reallocw(v,wds)         vm_realloc(v,wds,8)
#define vm_reallocb(v,b)           vm_realloc(v,b,1)

/* error handling macros */
#define ecall(f, ...) f(__FILE__,__LINE__,__func__, ##__VA_ARGS__)

#define rsp_perror(eno, fmt, ...) rsp_vperror(__FILE__,__LINE__,__func__,eno, ##__VA_ARGS__)

// this try catch implementation looks like it should work pretty well, but it requires that all TRY blocks be closed
// with an ETRY (which seems fine)

#define TRY                                                                       \
        rsp_ectx_t ectx__; int32_t eno__, __tr;	                                  \
	rsp_savestate(&ectx__);                                                   \
	err_stack = &ectx__;		                                          \
	eno__ = setjmp(ectx__.buf);                                               \
	if (eno__ == 0) {						          \
	  for (__tr=1; __tr; __tr=0, (void)(err_stack=err_stack->prev))

#define CATCH(x)                                                                  \
          } else if (eno__ == x) {						  \
          for (__tr=1; __tr; __tr=0, (void)(err_stack=rsp_restorestate(&ectx__))

#define ETRY                                                                      \
          } else								  \
	  {                                                                       \
	err_stack = rsp_restorestate(&ectx__) ;				          \
	rsp_raise(eno__) ;                                                        \
	  }

/* end rascal.h */
#endif
