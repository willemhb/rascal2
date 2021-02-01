#ifndef rascal_h

/* begin rascal.h */
#define rascal_h
#include "common.h"
#include "rtypes.h"
#include "globals.h"
#include "capi.h"
#include "opcodes.h"

/* 
   this is the main header used by different modules - it supplies everything from common, rtypes, and globals, and provides convenience macros and
   function declarations. The actual functions are divided between several files, listed below (above the declarations of the functions to be found in those files)

   convenience macros are defined at the bottom

 */

/* util.c */
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

/* 
   rbits.c

 */

// checking tags, getting type information
uint32_t ltag(val_t);
uint32_t tpkey(val_t);
type_t*  val_type(val_t);              // get the type object associated with the value
size_t   val_size(val_t);              // get the size of an object in bytes
chr_t*   val_typename(val_t);
chr_t*   tp_typename(type_t*);

#define type_name(v)              \
  _Generic((v),                   \
    type_t*:tp_typename,          \
	   val_t:val_typename)(v)


// handling fptrs
val_t    trace_fp(val_t);              // recursively follow a forward pointer (generally safe)
val_t    update_fp(val_t*);            // recursively update a forward pointer (generally safe)


/* predicates */
// global value predicates
bool isnil(val_t);
bool istrue(val_t);
bool isfalse(val_t);
bool isfptr(val_t);
bool isunbound(val_t);
bool isreof(val_t);

// type predicates
bool     ispair(val_t);
bool     islist(val_t);
bool     istuple(val_t);
bool     isbtuple(val_t);
bool     isstr(val_t);
bool     isbstr(val_t);
bool     isatom(val_t);
bool     issym(val_t);
bool     isdict(val_t);
bool     isiostrm(val_t);
bool     isbool(val_t);
bool     ischar(val_t);
bool     isint(val_t);
bool     isfloat(val_t);
bool     istype(val_t);

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
tuple_t*  totuple( SAFECAST_ARGS );
set_t*    toset( SAFECAST_ARGS );
dict_t*   todict( SAFECAST_ARGS );
type_t*   totype( SAFECAST_ARGS );
rint_t    toint( SAFECAST_ARGS );
rbool_t   tobool( SAFECAST_ARGS );
rflt_t    tofloat( SAFECAST_ARGS );
rchr_t    tochar( SAFECAST_ARGS );
obj_t*    toobj( SAFECAST_ARGS );
type_t*   totype(SAFECAST_ARGS );

/* mem.c */
// memory management
void*    vm_cmalloc(uint64_t);       
int32_t  vm_cfree(void*); 
uchr_t*  vm_crealloc(void**,uint64_t,bool); 
bool     vm_alloc(size_t,size_t,size_t,void*,size_t,...);
bool     vm_realloc(size_t,size_t,size_t,val_t*,size_t,...);
size_t   calc_mem_size(size_t);
bool     gc_check(size_t,bool);
void     gc_resize(void);
void     gc_run(void);                                // run the garbage collector
val_t    gc_trace(val_t);
val_t    gc_copy(type_t*,val_t);                      // the generic copier
bool     p_in_heap(void* v, uchr_t* h, uint64_t sz);  // check if h <= addr(v) <= (h + sz)
bool     v_in_heap(val_t v, uchr_t* h, uint64_t sz);

#define in_heap(v,u,sz)                            \
  _Generic((v),                                    \
	   val_t:v_in_heap,                        \
	   default:p_in_heap)(v,u,sz)

/* individual object apis */
// lists, pairs, and tuples
list_t*   mk_list(size_t,val_t*);
pair_t*   mk_pair(val_t,val_t);
tuple_t*  mk_tuple(size_t,uint8_t,uint8_t);
size_t    _tuple_size(obj_t*);
size_t    _tuple_elcnt(obj_t*);
size_t    _btuple_elcnt(obj_t*);
val_t*    _tuple_data(obj_t*);

#define tuple_size(t)   _tuple_size((obj_t*)t)
#define tuple_elcnt(t)  _tuple_elcnt((obj_t*)t)
#define btuple_elcnt(t) _btuple_elcnt((obj_t*)t)
#define tuple_data(t)   _tuple_data((obj_t*)t)

val_t*    tuple_ref(tuple_t*,uint32_t);
int32_t   tuple_check_idx(tuple_t*,val_t**,uint32_t);
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

// strings, bstrings and core atoms api
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

// tables, sets, and dicts
table_t*   mk_table(uint16_t,uint8_t,bool);
tuple_t*   mk_tbnode(size_t,uint8_t);
val_t      copy_table(type_t*,val_t);
list_t*    tb_bindings(table_t*);
list_t*    tb_bindings_tail(table_t*);
list_t*    tb_lookup(table_t*,val_t);
list_t*    tb_addkey(table_t*,val_t);
void       prn_dict(val_t,iostrm_t*);
void       prn_set(val_t,iostrm_t*);
atom_t*    mk_atom(chr_t*,uint16_t);
atom_t*    intern_string(chr_t*,hash32_t,uint16_t);

// direct data
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
#define vm_allocw(bs,wds,p,n, ...) vm_alloc(bs,wds,8,p,n, ##__VA_ARGS__)
#define vm_allocb(bs,nb,p,n, ...)  vm_alloc(bs,nb,1,p,n, ##__VA_ARGS__)
#define vm_allocc(nc,p,n, ...)     vm_alloc(0,16,nc,p,n, ##__VA_ARGS__)

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
