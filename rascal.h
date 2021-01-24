#ifndef rascal_h

/* begin rascal.h */
#define rascal_h
#include "common.h"
#include "rtypes.h"
#include "globals.h"

/* 
   this is the main header used by different modules - it supplies everything from common, rtypes, and globals, and provides convenience macros and
   function declarations. The actual functions are divided between several files, listed below (above the declarations of the functions to be found in those files)

   
   convenience macros are defined at the bottom

 */

/* util.c */
// stack manipulating functions
val_t  push(val_t**,val_t*,val_t*,val_t);         // stack, sp, stack size, value
val_t  pushn(val_t**,val_t*,val_t*,size_t,...);   // stack, sp, stack size, va size, values
val_t  pop(val_t*,val_t*);                        // stack, sp
val_t  popn(val_t*,val_t*,size_t);                // stack, sp, size
val_t  stk_reserve(val_t**,val_t*,val_t*,size_t);

// numeric utilities
uint32_t cpow2_32(int32_t);
uint64_t cpow2_64(int64_t);
uint64_t clog2(uint64_t);

// string and character utilities
hash32_t hash_str(const chr8_t*);
int32_t strsz(const chr8_t*);
int32_t u8strlen(const chr8_t*);
int32_t u8strcmp(const chr8_t*,const chr8_t*);
int32_t u8len(const chr8_t*);
int32_t u8tou32(chr32_t*,const chr8_t*);
ichr32_t nextu8(const chr8_t*);
ichr32_t nthu8(const chr8_t*,size_t);
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
void rsp_vperror(const chr8_t*, int32_t, const chr8_t*, rsp_err_t, const chr8_t*, ...);
rsp_ectx_t* rsp_restorestate(rsp_ectx_t*);

/* 
   rbits.c 

 */

// checking tags, getting type information
vtag_t   vtag(val_t);
otag_t   otag(val_t);
type_t*  val_type(val_t);              // get the type object associated with the value
size_t   val_size(val_t);              // get the size of an object in bytes
size_t   val_asize(val_t);

// handling fptrs
val_t    trace_fp(val_t);              // recursively follow a forward pointer (generally safe)
val_t    update_fp(val_t*);            // recursively update a forward pointer (generally safe)
uptr_t   safe_cast(val_t);             // traces the forwarding pointer and checks for extension types (returns the pointer as an int)


// comparison
int32_t  ordflt(flt32_t,flt32_t);
int32_t  ordint(int32_t,int32_t);
int32_t  ordhash(hash32_t,hash32_t);
int32_t  ordmem(uchr8_t*,size_t,uchr8_t*,size_t);
int32_t  ordstrval(val_t,val_t);
int32_t  ordbval(val_t,val_t);
int32_t  ordsymval(val_t,val_t);
int32_t  vm_ord(val_t,val_t);          // top level ordering function

// predicates (these functions are just for those tests that can't be safely done with macros; macro definitions for simple tests are at the bottom of the file)
bool iscvalue(val_t);
bool hasmethflags(val_t);
bool ispair(val_t);                // test if this is a list or a cons
bool tab_nmspc(table_t*);
bool vec_envt(vec_t*);   
bool vec_contfrm(vec_t*);
bool tab_symtab(table_t*);
bool vec_bytecode(vec_t*);     
bool tab_readtab(table_t*);    
bool tab_modnmspc(table_t*);   
bool vec_modenvt(vec_t*);     
bool val_nmspc(val_t);         
bool val_envt(val_t);          
bool val_contfrm(val_t);       
bool val_symtab(val_t);        
bool val_bytecode(val_t);
bool val_readtab(val_t);
bool val_modnmspc(val_t);
bool val_modenvt(val_t);


/* 
   safe casts - these check the type before performing the requested cast, and raise an error if the types don't match 
   there are also safecasts for special VM types (not strictly different types) and a genericized safecast for callables
   and conses or lists
   
 */

#define SAFECAST_ARGS const chr8_t*,int32_t,const chr8_t*,val_t

list_t*   tolist( SAFECAST_ARGS );
cons_t*   tocons( SAFECAST_ARGS );
rstr_t*   tostr( SAFECAST_ARGS );
bstr_t*   tobstr( SAFECAST_ARGS );
sym_t*    tosym( SAFECAST_ARGS );
iostrm_t* toiostrm( SAFECAST_ARGS );
method_t* tomethod( SAFECAST_ARGS );
cprim_t*  tocprim( SAFECAST_ARGS );
vec_t*    tovec( SAFECAST_ARGS );
node_t*   tonode( SAFECAST_ARGS );
table_t*  totable( SAFECAST_ARGS );
type_t*   totype( SAFECAST_ARGS );
code_t*   tocode( SAFECAST_ARGS );
cfrm_t*   tocfrm( SAFECAST_ARGS );
envt_t*   toenvt( SAFECAST_ARGS );
nmspc_t*  tonmspc( SAFECAST_ARGS );
symtab_t* tosymtab( SAFECAST_ARGS );
rint_t    toint( SAFECAST_ARGS );
rbool_t   tobool( SAFECAST_ARGS );
rflt_t    tofloat( SAFECAST_ARGS );
rchr_t    tochar( SAFECAST_ARGS );
func_t*   tofunc( SAFECAST_ARGS );
cons_t*   topair( SAFECAST_ARGS );

// miscellaneous utilities
val_t    val_itof(val_t);              // floating point32_t conversion 
val_t    val_ftoi(val_t);              // int32_teger conversion
c_num_t  vm_promote(val_t*,val_t*);    // cast to common type and return common type
bool     cbool(rbool_t);               // convert rascal boolean to C boolean
rstr_t*  strval(val_t);                // gets the string representation of a value if it has an obvious one, otherwise throw

/* obj.c */
// memory management
size_t   calc_mem_size(size_t);
bool     gc_check(size_t,bool);
uchr8_t* vm_cmalloc(uint64_t);                     // used for initialization and non-heap alloc
int32_t  vm_cfree(uchr8_t*);                       // used at startup/cleanup
uchr8_t* vm_crealloc(uchr8_t**,uint64_t,bool);     // used to grow stack/heap
uchr8_t* vm_alloc(size_t,size_t);                  // allocate an aligned block - arguments are base size and extra
size_t   vm_allocsz_str(type_t*,size_t,val_t*);
size_t   vm_allocsz_bytes(type_t*,size_t,val_t*);
size_t   vm_allocsz_copies(type_t*,size_t,val_t*);
size_t   vm_allocsz_words(type_t*,size_t,val_t*);
val_t*   vm_vec_realloc(vec_t*,size_t,size_t);     // reallocate a vector's elements
void     gc_run();                                 // run the garbage collector
void     gc_resize();                              // resize the TOSPACE
uchr8_t* gc_reserve(val_t);                        // reserve a block in the TOSPACE
val_t    gc_trace(val_t,vtag_t);                   // trace an arbitrary value (if ltag == LTAG_NONE, the value is tagged; otherwise, it's an untagged value with the specified ltag)
bool     isallocated(val_t);                         // check if a value is heap allocated
bool     in_heap(val_t v, uchr8_t* h, uint64_t sz);  // check if h <= addr(v) <= (h + sz)

/* 
   vm- and rascal constructor functions

   functions prefixed with 'mk_' or 'vm_' are convenience functions and vm-internal functions. functions prefixed with 'rsp_' are rascal-callable. They do not need to validate
   their argument counts, but they shouldn't assume that their arguments are the correct type.
 */
val_t     rsp_new(type_t*,size_t,val_t*);                 // entry-point for non-builtin constructor calls
list_t*   mk_list(bool,size_t,...);
val_t     rsp_new_list(size_t,val_t*);
cons_t*   mk_cons(val_t,val_t);
val_t     rsp_new_cons(val_t,val_t);
rstr_t*   mk_str(chr8_t*);
val_t     rsp_new_str(val_t);
bstr_t*   mk_bstr(size_t,uchr8_t*);
val_t     rsp_new_bstr(size_t,val_t*);
iostrm_t* mk_iostrm(chr8_t*,chr8_t*);
val_t     rsp_new_iostrm(size_t,val_t*);
sym_t*    mk_sym(chr8_t*,size_t,hash32_t,int32_t,bool);   // the third argument is a flag indicating the the authority to create
val_t     rsp_new_sym(size_t,val_t*);                     // a new symbol
val_t     rsp_cnvt_sym(size_t,val_t*);
method_t* mk_meth(table_t*,vec_t*,vec_t*,uint64_t);       // local names, bytecode, closure, flags
cprim_t*  mk_cprim(rcfun_t,size_t,uint64_t);              // callable, argcount, flags
vec_t*    mk_vec(otag_t,uint64_t,bool,size_t,...);        // otag, flags, whether the arguments are already stacked, and argcount
val_t     rsp_new_vec(size_t,val_t*);
node_t*   mk_node(size_t,val_t,bool);                     // order, hashkey, and whether to allocate space for bindings
table_t*  mk_table(bool,otag_t,size_t,...);
val_t     rsp_new_table(val_t*,int32_t);
val_t     mk_bool(int32_t);
val_t     rsp_new_bool(val_t*);
val_t     mk_char(int32_t);
val_t     rsp_new_char(val_t*);
val_t     mk_int(int32_t);
val_t     rsp_new_int(val_t*);
val_t     mk_float(flt32_t);
val_t     rsp_new_float(val_t*);
type_t*   mk_type(type_t*,size_t,...);              // mk_type is only called for extension types, so they require a parent (the default is 'object')            
val_t     rsp_new_type(type_t*,size_t,val_t*);

// specialized accessors for nodes and vectors
val_t get_nd_key(node_t*);
val_t get_nd_binding(node_t*);
val_t* get_vec_values(vec_t*);
val_t* get_vec_elements(vec_t*);

/* object APIS (mostly this is the AVL implementation) */
// AVL implementation (for insert, int32_tern, and delete, the extra node_t** argument is the address to leave the node matching the hashkey, and the return value
// indicates whether a new element was created and whether it was inserted in the left or right subtree)
node_t*  node_search(node_t*,val_t);
int32_t  node_insert(node_t**,node_t**,val_t,size_t,uchr8_t*);
int32_t  node_inttern(node_t**,node_t**,chr8_t*,hash32_t,int32_t,size_t);
int32_t  node_delete(node_t*,val_t);
node_t*  node_balance(node_t*);
node_t*  rotate_ll(node_t*,node_t*);
node_t*  rotate_rr(node_t*,node_t*);
node_t*  rotate_lr(node_t*,node_t*);
node_t*  rotate_rl(node_t*,node_t*);

node_t* table_insert(table_t*,val_t);
node_t* table_lookup(table_t*,val_t);
sym_t*  table_intern(symtab_t*,chr8_t*,hash32_t,int32_t);
int32_t table_delete(table_t*,val_t);

val_t vec_append(vec_t*,val_t);
val_t vec_pop(vec_t*);
val_t vec_appendn(vec_t*,bool,size_t,...);
val_t list_append(list_t*,val_t);
val_t list_appendn(list_t*,bool,size_t,...);

/* 
   vm.c - environment API, the VM, and the compiler 
 */

int32_t   env_locate(envt_t*,sym_t*,val_t*,val_t*);           // find the nearest enclosing environment where this name is bound; store location in arg3 and current value in arg4; return a flag
int32_t   env_extend(envt_t*,sym_t*,val_t*);                  // add supplied symbol to the environment
size_t    lenv_bind(envt_t*,val_t);                           // bind the supplied value in the next free slot in the local environment and return the next free location
val_t     local_lookup(envt_t*,size_t,size_t);
val_t     local_assign(envt_t*,size_t,size_t,val_t);
val_t     module_lookup(envt_t*,sym_t*);                      // lookup the supplied symbol in the environment
val_t     module_assign(nmspc_t*,sym_t*,val_t);

// the virtual machine and compiler
opcode_t  vm_fetch_instr();                                  // load the next instruction int32_to INSTR[0] arguments in INSTR[1], INSTR[2], and INSTR[3]; update PC, return opcode
cfrm_t*   vm_save(cfrm_t*);                                  // arg0 is the current continuation frame
cfrm_t*   vm_restore(cfrm_t*);                               // restore the register state to that of the saved frame and return the old frame
val_t     vm_exec(code_t*,envt_t*);                          // bytecode, envt, argument flags, argument count, arguments
code_t*   vm_compile(val_t,envt_t*);                         // entry point for the compiler
val_t     vm_expand(func_t* f,envt_t* e,list_t* l);          // evaluate the macro f in the compile-time environment e and the list of supplied arguments l
val_t     vm_backquote(envt_t*,val_t);                       // A C function to implement the backquote macro

/* rspio.c - printing, reading, and writing */
// printing dispatch functions for builtin types
void  prn_list(val_t,iostrm_t*);
void  prn_cons(val_t,iostrm_t*);
void  prn_str(val_t,iostrm_t*);
void  prn_bstr(val_t,iostrm_t*);
void  prn_sym(val_t,iostrm_t*);
void  prn_vec(val_t,iostrm_t*);
void  prn_int(val_t,iostrm_t*);
void  prn_table(val_t,iostrm_t*);
void  prn_float(val_t,iostrm_t*);
void  prn_char(val_t,iostrm_t*);
void  prn_bool(val_t,iostrm_t*);
void  prn_value(val_t,iostrm_t*); // default printer
void  vm_prn(val_t,iostrm_t*);    // print dispatcher

// reader/printer support functions
rsp_tok_t vm_get_token(iostrm_t*);
val_t     vm_read_expr(iostrm_t*);
val_t     vm_read_cons(iostrm_t*);                   // read a cons or list
val_t     vm_read_bytes(iostrm_t*);                  // optimized reader for bytestring literals
val_t     vm_read_coll(type_t*,iostrm_t*,rsp_tok_t); // read a sequence of space-separated expressions up to a delimiter, then pass the result to the type's constructor
void      vm_print(val_t,iostrm_t*);             
val_t     vm_load(iostrm_t*,val_t);

/* bltins.c */

/* rspinit.c */
// memory
void init_heap();
void init_types();
void init_registers();

// builtins
void init_builtin_types();
void init_special_forms();
void init_global_variables();
void init_builtin_functions();

/* toplevel bootstrapping and cleanup functions */
void bootstrap_rascal();
int  teardown_rascal();

/* rspmain.c */
int rsp_repl();
int rsp_main(int32_t,chr8_t**);

/*
   convenience macros 

   in general, macros affixed with '_' are unsafe; they don't perform necessary safety checks
   before performing the requested operation. Those safety checks may be superfluous, but in general
   the safe version is preferred.

 */

// compiler hint macros
#define unlikely(x) __builtin_expect((x), 0)
#define likely(x)   __builtin_expect((x), 1)

// stack manipulation macros
#define EPUSH(v)           push(&EVAL,&SP,&STACKSIZE,v)
#define EPUSHN(n,...)      pushn(&EVAL,&SP,&STACKSIZE,n, ## __VA_ARGS__)  
#define EPOP()             pop(EVAL,&SP)
#define EPOPN(n)           popn(EVAL,&SP,n)
#define DPUSH(v)           push(&DUMP,&DP,&DUMPSIZE,v)
#define DPUSHN(n,...)      pushn(&DUMP,&DP,&DUMPSIZE,n, ## __VA_ARGS__)  
#define DPOP()             pop(DUMP,&DP)
#define DPOPN(n)           popn(DUMP,&DP,n)

// these macros can be used to avoid the function call and bounds checking overhead of popn
#define SAVESP     val_t __OSP__ = SP
#define RESTORESP  SP = __OSP__
#define SAVEDP     val_t __ODP__ = DP
#define RESTOREDP  val_t DP = __ODP__

// get the
#define BASE(stk,argc) (stk - (argc))

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

/* bit manipulation macros for tagging, untagging, and testing */

#define ntag(v)              ((v) & 0xf)
#define wtag(v)              ((v) & 0xff)
#define otag_(v,m)           (*ptr(val_t*,v) & (m))
#define otype_(v)            ((type_t*)(*ptr(val_t*,v) & (~0xful)))
#define oflags_(v)           (*ptr(val_t*,v) & (~0xfful))
#define addr(v)              (((val_t)(v)) & (~0xful))
#define bits(d)              (((rsp_c32_t)(d)).bits32)
#define pad(d)               (((val_t)bits(d)) << 32)
#define unpad(d)             ((d) >> 32)
#define tag_v(v,t)           (pad(v) | (t))
#define tag_p(p,t)           (((val_t)(p)) | (t))
#define tagi(v)              (pad(v) | VTAG_INT)
#define tagc(v)              (pad(v) | VTAG_CHAR)
#define tagb(v)              (pad(v) | VTAG_BOOL)
#define tagf(v)              (pad(v) | VTAG_FLOAT)

// automatic pointer tagging macro
#define tagp(p)                                  \
  _Generic((p),                                  \
	   list_t*:((val_t)(p)) | VTAG_LIST,     \
           cons_t*:((val_t)(p)) | VTAG_CONS,     \
           rstr_t*:((val_t)(p)) | VTAG_STR,      \
	   sym_t*:((val_t)(p))  | VTAG_SYM,	 \
           bstr_t*:((val_t)(p)) | VTAG_BSTR,     \
           vec_t*:((val_t)(p))  | VTAG_VEC,      \
           node_t*:((val_t)(p)) | VTAG_NODE,     \
           iostrm_t*:((val_t)(p)) | VTAG_IOSTRM, \
           table_t*:((val_t)(p)) | VTAG_TABLE,   \
	   method_t*:((val_t)(p)) | VTAG_METHOD, \
	   cprim_t*:((val_t)(p)) | VTAG_CPRIM,   \
	   default:((val_t)(p)) | VTAG_OBJECT)

#define ptr(t,v)             ((t)addr(v))
#define uval(v)              (((rsp_c64_t)(v)).padded.bits.bits32)
#define ival(v)              (((rsp_c64_t)(v)).padded.bits.integer)
#define fval(v)              (((rsp_c64_t)(v)).padded.bits.float32)
#define bval(v)              (((rsp_c64_t)(v)).padded.bits.boolean)
#define cval(v)              (((rsp_c64_t)(v)).padded.bits.unicode)
#define aligned(p,n)         (((val_t)(p)) % (n) == 0)

/* 
   generic macros

   these macros are used to create a single interface for working with rascal values that handles data safely
   or quickly if it's an untagged pointer to the native C type.
*/

#define GENERIC_TYPE_ACCESSOR(v)	          \
  _Generic((v),                                   \
	   type_t*:v,				  \
           val_t:val_type((val_t)(v)))

#define type_meta(v)          (type_t*)(GENERIC_TYPE_ACCESSOR(v)->tp_meta & (~0xful))
#define type_parent(v)        GENERIC_TYPE_ACCESSOR(v)->tp_parent
#define type_origin(v)        GENERIC_TYPE_ACCESSOR(v)->tp_origin
#define type_order(v)         GENERIC_TYPE_ACCESSOR(v)->tp_order
#define type_hash(v)          GENERIC_TYPE_ACCESSOR(v)->tp_hash
#define type_builtin_p(v)     GENERIC_TYPE_ACCESSOR(v)->tp_builtin_p
#define type_direct_p(v)      GENERIC_TYPE_ACCESSOR(v)->tp_direct_p
#define type_atomic_p(v)      GENERIC_TYPE_ACCESSOR(v)->tp_atomic_p
#define type_heap_p(v)        GENERIC_TYPE_ACCESSOR(v)->tp_heap_p
#define type_store_p(v)       GENERIC_TYPE_ACCESSOR(v)->tp_store_p
#define type_final_p(v)       GENERIC_TYPE_ACCESSOR(v)->tp_final_p
#define type_extensible_p(v)  GENERIC_TYPE_ACCESSOR(v)->tp_extensible_p
#define type_vtag(v)          GENERIC_TYPE_ACCESSOR(v)->tp_vtag
#define type_otag(v)          GENERIC_TYPE_ACCESSOR(v)->tp_otag
#define type_c_num(v)         GENERIC_TYPE_ACCESSOR(v)->tp_c_num
#define type_c_ptr(v)         GENERIC_TYPE_ACCESSOR(v)->tp_c_ptr
#define type_base(v)          GENERIC_TYPE_ACCESSOR(v)->tp_base
#define type_fields(v)        GENERIC_TYPE_ACCESSOR(v)->tp_fields
#define type_new(v)           GENERIC_TYPE_ACCESSOR(v)->tp_new
#define type_alloc_sz(v)      GENERIC_TYPE_ACCESSOR(v)->tp_alloc_sz
#define type_init(v)          GENERIC_TYPE_ACCESSOR(v)->tp_init
#define type_prn(v)           GENERIC_TYPE_ACCESSOR(v)->tp_prn
#define type_ord(v)           GENERIC_TYPE_ACCESSOR(v)->tp_ord
#define type_size(v)          GENERIC_TYPE_ACCESSOR(v)->tp_size
#define type_nfields(v)       GENERIC_TYPE_ACCESSOR(v)->tp_nfields
#define type_name(v)          GENERIC_TYPE_ACCESSOR(v)->name


#define UNSAFE_GENERICIZE(t,v)		   \
  _Generic((v),                            \
	   uchr8_t*:(t)v,                  \
	   t:v,                            \
           val_t:ptr(t,(val_t)v))

#define UNSAFE_GENERICIZE2(t1,t2,v)        \
  _Generic((v),                            \
           uchr8_t*:(t1)v,                 \
	   t1:v,                           \
	   t2:v,                           \
           val_t:ptr(t1,(val_t)v))

// genericize by safe casting
#define SAFE_GENERICIZE_ECAST(t,v,cnvt)	                    \
  _Generic((v),                                             \
	   uchr8_t*:(t)v,                                   \
	   t:v,                                             \
           val_t:cnvt(__FILE__,__LINE__,__func__,(val_t)v))

#define SAFE_GENERICIZE_ECAST2(t1,t2,t3,v,cnvt)		    \
  _Generic((v),                                             \
	   uchr8_t*(t3)v,                                   \
	   t1:v,                                            \
	   t2:v,                                            \
           val_t:cnvt(__FILE__,__LINE__,__func__,(val_t)v))

// genericize by selecting different versions of a function
#define SAFE_GENERICZE_SELECT(t,v,unsafe,safe)          \
  _Generic((v),                                         \
	   t:unsafe,                                    \
	   val_t:safe)

/* comparison and predicate macros */
// fast comparisons and tests for special constants
#define isnil(v)      ((v) == R_NIL)
#define istrue(v)     ((v) == R_TRUE)
#define isfalse(v)    ((v) == R_FALSE)
#define isok(v)       ((v) == R_OK)
#define isnone(v)     ((v) == R_NONE)
#define isfptr(v)     ((v) == R_FPTR)
#define isunbound(v)  ((v) == R_UNBOUND)
#define iseof(v)      ((v) == R_EOF)

// type and tag testing
#define isa(v,t)       (val_type(v) == (t))
#define islist(v)      (ntag(v) == NTAG_LIST)
#define iscons(v)      (ntag(v) == NTAG_CONS)
#define isiostrm(v)    (ntag(v) == NTAG_IOSTRM)
#define isstr(v)       (ntag(v) == NTAG_STR)
#define isbstr(v)      (ntag(v) == NTAG_BSTR)
#define issym(v)       (ntag(v) == NTAG_SYM)
#define ismethod(v)    (ntag(v) == NTAG_METHOD)
#define iscprim(v)     (ntag(v) == NTAG_CPRIM)
#define isnode(v)      (ntag(v) == NTAG_NODE)
#define isvec(v)       (ntag(v) == NTAG_VEC)
#define istable(v)     (ntag(v) == NTAG_TABLE)
#define isdirect(v)    (ntag(v) == NTAG_DIRECT)
#define isobj(v)       (ntag(v) == NTAG_OBJECT)
#define isbool(v)      (vtag(v) == VTAG_BOOL)
#define ischar(v)      (vtag(v) == VTAG_CHAR)
#define isint(v)       (vtag(v) == VTAG_INT)
#define isfloat(v)     (vtag(v) == VTAG_FLOAT)
#define istype(v)      (val_type(v) == TYPE_METAOBJ)

// otag tests are a little more complicated; these dispatch to a safe and unsafe version depending on whether we're dealing with a tagged or untagged value
#define isnmspc(v)    SAFE_GENERICIZE_SELECT(table_t*,v,tab_nmspc,val_nmspc)(v)
#define isenvt(v)     SAFE_GENERICIZE_SELECT(vec_t*,v,vec_envt,val_envt)(v)
#define iscontfrm(v)  SAFE_GENERICIZE_SELECT(vec_t*,v,vec_contfrm,val_contfrm)(v)
#define issymtab(v)   SAFE_GENERICIZE_SELECT(table_t*,v,tab_symtab,val_symtab)(v)
#define isbytecode(v) SAFE_GENERICIZE_SELECT(vec_t*,v,vec_bytecode,val_bytecode)(v)
#define isreadtab(v)  SAFE_GENERICIZE_SELECT(table_t*,v,tab_readtab,val_readtab)(v)
#define ismodnmspc(v) SAFE_GENERICIZE_SELECT(table_t*,v,tab_modnmspc,val_modnmspc)(v)
#define ismodenvt(v)  SAFE_GENERICIZE_SELECT(vec_t*,vec_modenvt,val_modenvt)(v)

/* 
   genericized object accessors - unsafe accessors should only be used in cases where the
   the necessary checks have already been performed.
 */

#define car_(v)             UNSAFE_GENERICIZE2(cons_t*,list_t*,v)->car
#define cdr_(v)             UNSAFE_GENERICIZE2(cons_t*,list_t*,v)->cdr
#define car(v)              SAFE_GENERICIZE_ECAST2(cons_t*,list_t*,cons_t*,v,topair)->car
#define cdr(v)              SAFE_GENERICIZE_ECAST2(cons_t*,list_t*,cons_t*,v,topair)->cdr
#define first(v)            SAFE_GENERICIZE_ECAST(list_t*,v,tolist)->car
#define rest(v)             SAFE_GENERICIZE_ECAST(list_t*,v,tolist)->cdr
#define bs_sz(v)            SAFE_GENERICIZE_ECAST(bstr_t*,v,tobstr)->size
#define bs_bytes(v)         SAFE_GENERICIZE_ECAST(bstr_t*,v,tobstr)->bytes
#define sm_name(v)          SAFE_GENERICIZE_ECAST(sym_t*,v,tosym)->name
#define sm_hash(v)          SAFE_GENERICIZE_ECAST(sym_t*,v,tosym)->hash
#define sm_keyword(v)       SAFE_GENERICIZE_ECAST(sym_t*,v,tosym)->keyword
#define sm_reserved(v)      SAFE_GENERICIZE_ECAST(sym_t*,v,tosym)->reserved
#define sm_interned(v)      SAFE_GENERICIZE_ECAST(sym_t*,v,tosym)->interned
#define sm_gensym(v)        SAFE_GENERICIZE_ECAST(sym_t*,v,tosym)->gensym
#define nd_order(v)         SAFE_GENERICIZE_ECAST(node_t*,v,tonode)->order
#define nd_balance(v)       SAFE_GENERICIZE_ECAST(node_t*,v,tonode)->balance
#define nd_left_(v)         UNSAFE_GENERICIZE(node_t*,v)->left
#define nd_right_(v)        UNSAFE_GENERICIZE(node_t*,v)->right
#define nd_data_(v)         UNSAFE_GENERICIZE(node_t*,v)->nd_data
#define nd_left(v)          SAFE_GENERICIZE_ECAST(node_t*,v,tonode)->left
#define nd_right(v)         SAFE_GENERICIZE_ECAST(node_t*,v,tonode)->right
#define nd_data(v)          SAFE_GENERICIZE_ECAST(node_t*,v,tonode)->nd_data
#define nd_key(v)           get_nd_key(SAFE_GENERICIZE_ECAST(node_t*,v,tonode))
#define nd_binding(v)       get_nd_binding(SAFE_GENERICIZE_ECAST(node_t*,v,tonode))
#define tb_records_(v)      UNSAFE_GENERICIZE(table_t*,v)->records
#define tb_records(v)       SAFE_GENERICIZE_ECAST(table_t*,v,totable)->records
#define tb_count(v)         SAFE_GENERICIZE_ECAST(table_t*,v,totable)->count
#define tb_order(v)         SAFE_GENERICIZE_ECAST(table_t*,v,totable)->order
#define tb_global(v)        SAFE_GENERICIZE_ECAST(table_t*,v,totable)->global
#define tb_reserved(v)      SAFE_GENERICIZE_ECAST(table_t*,v,totable)->reserved
#define vec_datasz_(v)      UNSAFE_GENERICIZE(vec_t*,v)->datasz
#define vec_values_(v)      get_vec_values(UNSAFE_GENERICIZE(vec_t*,v))
#define vec_datasz(v)       SAFE_GENERICIZE_ECAST(vec_t*,v,tovec)->datasz
#define vec_initsz(v)       SAFE_GENERICIZE_ECAST(vec_t*,v,tovec)->initsz
#define vec_localvals(v)    SAFE_GENERICIZE_ECAST(vec_t*,v,tovec)->localvals
#define vec_dynamic(v)      SAFE_GENERICIZE_ECAST(vec_t*,v,tovec)->dynamic
#define vec_data(v)         SAFE_GENERICIZE_ECAST(vec_t*,v,tovec)->data
#define vec_values(v)       get_vec_values(SAFE_GENERICIZE_ECAST(vec_t*,v,tovec))
#define vec_elements(v)     get_vec_elements(SAFE_GENERICIZE_ECAST(vec_t*,v,tovec))
#define fn_method(v)        SAFE_GENERICIZE_ECAST2(method_t*,cprim_t*,func_t*,v,tofunc)->method
#define fn_cprim(v)         SAFE_GENERICIZE_ECAST2(method_t*,cprim_t*,func_t*,v,tofunc)->cprim
#define fn_argc(v)          SAFE_GENERICIZE_ECAST2(method_t*,cprim_t*,func_t*,v,tofunc)->argco
#define fn_pure(v)          SAFE_GENERICIZE_ECAST2(method_t*,cprim_t*,func_t*,v,tofunc)->pure
#define fn_kwargs(v)        SAFE_GENERICIZE_ECAST2(method_t*,cprim_t*,func_t*,v,tofunc)->kwargs
#define fn_varkw(v)         SAFE_GENERICIZE_ECAST2(method_t*,cprim_t*,func_t*,v,tofunc)->varkw
#define fn_macro(v)         SAFE_GENERICIZE_ECAST2(method_t*,cprim_t*,func_t*,v,tofunc)->macro
#define meth_code_(v)       UNSAFE_GENERICIZE(method_t*,v)->code
#define meth_names_(v)      UNSAFE_GENERICIZE(method_t*,v)->names
#define meth_envt_(v)       UNSAFE_GENERICIZE(method_t*,v)->envt
#define meth_code(v)        SAFE_GENERICIZE_ECAST(method_t*,v,tomethod)->code
#define meth_names(v)       SAFE_GENERICIZE_ECAST(method_t*,v,tomethod)->names
#define meth_envt(v)        SAFE_GENERICIZE_ECAST(method_t*,v,tomethod)->envt
#define cprim_callable(v)   SAFE_GENERICIZE_ECAST(cprim_t*,v,tomethod)->callable

/* error handling macros */
#define ERRINFO_FMT  "[%s:%i:%s] %s error: "
#define TYPEERR_FMT  "expected type %s, got %s"
#define ARITYERR_FMT "expected %i args, got %i"

#define ecall(f, ...) f(__FILE__,__LINE__,__func__, ##__VA_ARGS__)

#define rsp_perror(eno, fmt, ...) rsp_vperror(__FILE__,__LINE__,__func__,eno,fmt, ##__VA_ARGS__)

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
