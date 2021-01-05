#ifndef rascal_h

/* begin common.h */
#define rascal_h
#include "common.h"

/* 

   this header holds the declarations of builtin types, global constants, generally useful macros, and all the C functions used 
   internally by the VM. Builtin functions and C bindings are in capi.h 

 */

/* the current object head layouts imply various sizing limits, which are defined here */

const uint64_t MAX_ARRAY_ELEMENTS = (1ul << 60) - 1;
const uint64_t MAX_TABLE_ELEMENTS = (1ul << 52) - 1;
const uint64_t MAX_OBJECT_BASESIZE = (1ul << 46) - 1;
const uint64_t MAX_PROC_ARGCO = (1ul << 28) - 1;

/*  structs to represent builtin object types , along with the accessors used to get specific fields */
// generic object - the meta field is a tagged pointer to the type and fields is a vector (of at least one element)
// containing the object data - all heap objects have the same alignment as obj_t
struct obj_t {
  OBJECT_HEAD;
  val_t fields[1];
};

struct fvec_t {
  val_t allocated;
  val_t elements[1];
};

struct proc_t {
  OBJECT_HEAD;
  val_t argco   : 32;
  val_t kwargco : 32;
  val_t template;
  val_t proc_fields[1];
};

#define proc_closure(v)  ((v)->proc_fields[0])
#define proc_locals(v)   ((v)->proc_fields[1])

struct cons_t {
  val_t car;
  val_t cdr;
};

struct sym_t {
  val_t record;        // this symbol's entry in a symbol table, plus some flags in the low bits, or the hash if it's uninterned
  char name[1];        // the name hangs off the end
};

struct node_t {
  val_t left;
  val_t right;
  val_t hashkey;
  val_t binding;
};

#define left_balance(v)  (((v)->left) & 0xf)
#define right_balance(v) (((v)->right) & 0xf)
#define left_child(v)    (((v)->left) & (~0xful))
#define right_child(v)   (((v)->right) & (~0xful))

struct dvec_t {
  val_t elements;
  val_t curr;
  val_t _extra[2];   // used by environment frames
};

struct table_t {
  OBJECT_HEAD;
  val_t count;       // number of elements
  val_t records;     // pointer to the nodes storing keys and bindings
};

struct type_t {
  OBJECT_HEAD;
  val_t parent;                          // pointer to the parent type

  /* flags encoding size information */
  
  val_t val_base_size     : 47;         // the minimum size for objects of this type
  val_t val_fixed_size    :  1;
  
  /* flags encoding the size and C representation for values of this type */

  val_t val_cnum          :  8;         // the C numeric type of the value representation
  val_t val_cptr          :  3;         // the C pointer type of the value representation
  
  /* misc other data about values */

  val_t tp_atomic         :  1;        // legal for use in a plain dict
  val_t val_ltag          :  4;

  /* layout information */

  table_t* tp_readable;                               // a dict mapping rascal-accessible fields to offsets
  table_t* tp_writeable;                              // a dict mapping rascal-writeable fields to offsets

  /* function pointers */

  int   (*tp_val_size)(val_t);                          // called when the size can't be determined from flags alone
  val_t (*tp_new)(val_t*,int);                          // function looked up when the type is called for its constructor
  val_t (*tp_init)(type_t*,val_t,void*);                // initializer
  val_t (*tp_copy)(val_t);                              // used by the garbage collector to move the value and leave behind an fptr
  void  (*tp_prn)(val_t,FILE*);                         // used to print values
  char name[1];
};

/* constant definitions and variable declarations */
// special constants - common values, singletons, sentinels, and standard streams
const val_t R_NIL = 0x0ul;
const val_t R_NONE = UINT_MAX;
const val_t R_UNBOUND = LTAG_SYM;                  // an unbound symbol
const val_t R_FPTR = LTAG_CONS;                    // a forwarding pointer
const val_t R_TRUE = (UINT32_MAX + 1) | LTAG_BOOL;
const val_t R_FALSE = LTAG_BOOL;
const val_t R_ZERO = LTAG_INT;
const val_t R_ONE = (UINT32_MAX + 1) | LTAG_INT;
const val_t R_TWO = (UINT32_MAX + 2) | LTAG_INT;
const val_t R_EOF = (((int64_t)WEOF) << 32) | LTAG_CHAR;

// macros since the actual locations of stdin, stdout, and stderr aren't fixed
#define R_STDIN  (((val_t)stdin)  | LTAG_CFILE)
#define R_STDOUT (((val_t)stdout) | LTAG_CFILE)
#define R_STDERR (((val_t)stderr) | LTAG_CFILE)

// memory, GC, and VM
extern unsigned char *FROMSPACE, *TOSPACE, *FREE;
const float RAM_LOAD_FACTOR = 0.8;
extern val_t HEAPSIZE, STACKSIZE;
extern bool GROWHEAP;

// registers holding rascal values
extern val_t VALUE, ENVT, CONT, TEMPLATE;

/* 
   the stacks used by the virtual machine

   EVAL - holds evaluated sub-expressions needed by the current procedure or expression
   STACK - a more general stack used to save values that will be needed later

*/

extern val_t *EVAL, *STACK;

// offset registers
extern size_t EP, SP, PC;

// working registers
extern val_t WRX[8];

struct _rsp_ectx_t {
  jmp_buf buf;
  struct _rsp_ectx_t* prev;
  
  // all the shared global state that might have changed
  
  unsigned char* FREE_state;
  val_t VALUE_state;
  val_t ENVT_state;
  val_t CONT_state;
  val_t TEMPLATE_state;
  val_t PC_state;
  size_t EP_state;
  size_t SP_state;
};

// reader
extern const int TOKBUFF_SIZE;
extern char TOKBUFF[];
extern int TOKPTR;
extern rsp_tok_t TOKTYPE;

// builtin form names
const char* FORM_NAMES[] = { "setv", "def", "quote", "if", "fn", "macro", "do", "let" };

const char* SPECIAL_VARIABLES[] = { "*env*", "*stdin*", "*stdout*", "*stderr*",
                                    "nil", "none", "t", "f", ":ok" ":r",
				    ":w", ":a", ":r+", ":w+", ":a+", };

// the symbols corresponding to the available special forms
extern val_t F_SETV, F_DEF, F_QUOTE, F_IF, F_FN, F_MACRO, F_DO, F_LET;

// other important global variables; right now, this is just the symbol table and the global environment
extern val_t R_SYMTAB, R_TOPLEVEL;

// error handling
extern rsp_ectx_t* exc_stack;
extern rsp_err_t rsp_errcode;

/* function declarations & apis */
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

#define compare(x,y)                       \
  ({                                       \
    typeof(x) __x__ = x ;                  \
    typeof(y) __y__ = y ;                  \
    int __r__ ;                            \
    if (__x__ == __y__) __r__ = 0 ;        \
    else __r__ = __x__ < __y__ ? -1 : 1 ;  \
    __r__ ;                                \
  })

// manipulating tags and accessing object metadata
int ltag(val_t);

#define ntag(v)              ((v) & 0xf)
#define wtag(v)              ((v) & 0xff)
#define addr(v)              ((v) & (~0xful))
#define bits(d)              (((rsp_direct_ctypes_t)(d)).bits)
#define pad(d)               (((val_t)bits(d)) << 32)
#define unpad(d)             ((d) >> 32)
#define tag_v(v,t)           (pad(v) | (t))
#define tag_p(p,t)           (((val_t)(p)) | (t))
#define obj_(v)              ((obj_t*)addr(v))
#define type_(v)             ((type_t*)((obj_(v)->type) & (~0xful)))
#define flags_(v)            ((obj_(v)->type) & 0xful)
#define ptr_(t,v)            ((t)addr(v))
#define val_(t,v)            ((t)unpad_b(v))
#define aligned(p,n)         (((val_t)(p)) % (n) == 0)

type_t* get_val_type(val_t);             // get the type object associated with the value
int get_val_size(val_t);                 // get the size of an object
char* get_val_type_name(val_t);          // get the object type name
c_num_t get_val_cnum(val_t);
bool isdirect(val_t);
bool isatomic(val_t);
bool iscallable(val_t);
c_num_t get_common_cnum(val_t,val_t);

// fast comparisons and tests
#define isnil(v)      ((v) == R_NIL)
#define isnone(v)     ((v) == R_NONE)
#define istrue(v)     ((v) == R_TRUE)
#define isfalse(v)    ((v) == R_FALSE)
#define isfptr(v)     ((v) == R_FPTR)
#define isunbound(v)  ((v) == R_UNBOUND)

// tag-based tests for builtin types
#define hasltag(v,w)    (ltag(v) == (w))
#define hasoflags(v,w)  (oflags(v) == (w))
#define isstr(v)        hasltag(v,LTAG_STR)
#define iscfile(v)      hasltag(v,LTAG_CFILE)
#define iscons(v)       hasltag(v,LTAG_CONS)
#define islist(v)       hasltag(v,LTAG_LIST)
#define issym(v)        hasltag(v,LTAG_SYM)
#define isint(v)        hasltag(v,LTAG_INT)
#define isbool(v)       hasltag(v,LTAG_BOOL)
#define ischar(v)       hasltag(v,LTAG_CHAR)
#define isfvec(v)       hasltag(v,LTAG_FVEC)
#define isdvec(v)       hasltag(v,LTAG_DVEC)
#define istable(v)      hasltag(v,LTAG_TABLE)
#define isnode(v)       hasltag(v,LTAG_NODE)
#define istype(v)       hasltag(v,LTAG_META)
#define isfunction(v)   hasltag(v,LTAG_FUNCTION)
#define isobj(v)        hasltag(v,LTAG_OBJECT)
// somewhat slower tests for other builtins
#define isa(v,t)        (get_val_type(v) == (t))
#define isbuiltin(v,t)  (isa(v,BUILTIN_TYPE_OBJ))
#define islambda(v,t)   (isa(v,LAMBDA_TYPE_OBJ))

// safecast macro; updates forwarding pointers and checks the type before performing
// the requested cast

#define SAFECAST(ctype,test,cnvt,name,v)				                                         \
  ({                                                                                                             \
     val_t __v__  = check_fptr(&(v))                                                                           ; \
     if (!test(__v__)) { rsp_perror(TYPE_ERR,"expected type %s, got %s.",name,__v__) ; rsp_raise(TYPE_ERR) ; } ; \
     cnvt(ctype,__v__)                                                                                         ; \
   })

#define tostr(v)       SAFECAST(char*,isstr,ptr_,"str",v)
#define tocfile(v)     SAFECAST(FILE*,iscfile,ptr_,"cfile",v)
#define tobuiltin(v)   SAFECAST(proc_t*,isbuiltin,ptr_,"builtin",v)
#define tolambda(v)    SAFECAST(proc_t*,islambda,ptr_,"lambda",v)
#define tocons(v)      SAFECAST(cons_t*,iscons,ptr_,"cons",v)
#define tolist(v)      SAFECAST(cons_t*,islist,ptr_,"list",v)
#define tosym(v)       SAFECAST(sym_t*,issym,ptr_,"sym",v)
#define toint(v)       SAFECAST(int,isint,val_,"int",v)
#define tobool(v)      SAFECAST(bool,isbool,val_,"bool",v)
#define tochar(v)      SAFECAST(wchar_t,ischar,val_,"char",v)
#define tofvec(v)      SAFECAST(fvec_t*,isfvec,ptr_,"fvec",v)
#define totable(v)     SAFECAST(table_t*,istable,ptr_,"table",v)
#define totype(v)      SAFECAST(type_t*,istype,ptr_,"type",v)
#define todvec(v)      SAFECAST(dvec_t*,isdvec,ptr_,"dvec",v)

// general object convenience functions and predicates
bool cbool(val_t);                // convert rascal value to C boolean
val_t check_fptr(val_t*);         // check for forwarding pointer on an arbitrary 
val_t update_fptr(val_t*);        // recursively update a forwarding pointer

// memory management and bounds checking
// allocation
unsigned char* rsp_alloc(size_t);                                // allocate an aligned block large enough to accommodate n bytes
unsigned char* rsp_allocw(size_t);                               // allocate n word-sized blocks
int            rsp_init(type_t*,val_t,void*);                    // generic allocator
unsigned char* rsp_copy(unsigned char*,unsigned char*,size_t);   // generic copier
// gc
void gc();
val_t gc_trace(val_t*);
bool in_heap(val_t, val_t*,val_t*);
bool heap_limit(unsigned long);
bool stack_overflow(val_t*, unsigned long, unsigned long);

/* object apis */
// low level constructors - make rascal objects from existing C data or allocate appropriate space for an object of the desired type
val_t mk_int(int);
val_t mk_char(int);
val_t mk_bool(int);
val_t mk_str(const char*);
val_t mk_cfile(FILE*);
val_t mk_sym(char*,int,int,int,int);         // interned, gensym, keyword, reserved flags
val_t mk_builtin(val_t(*)(val_t*,int),int);  // C function, argument count
val_t mk_lambda(val_t,val_t,val_t,int);      // argument list, body, captured environment, macro flag
val_t mk_list(int);                          // element count
val_t mk_dvec(int);                          // element count
val_t mk_fvec(int);                          // element count
val_t mk_cons();
val_t mk_node();                             // binding size
val_t mk_table();                            // element count

// vm constructors for special types
val_t mk_envtframe(val_t,val_t,int);
val_t mk_contframe(int);

// working with AVL nodes
int avl_insert(node_t*,val_t);
int rotate_ll(node_t*);
int rotate_lr(node_t*);
int rotate_rl(node_t*);
int rotate_rr(node_t*);

// object comparison
int      ord(val_t,val_t);  // order two comparable values

// specific object apis for builtin objects
hash64_t symhash(sym_t*);
table_t* left(table_t*);
table_t* right(table_t*);

// field-, index-, and key-based object access (generic)
val_t    rsp_getf(val_t,val_t);         // return the value of a named field
val_t    rsp_setf(val_t,val_t);         // set the value of a named field
val_t    rsp_assck(val_t,val_t);        // key-based search
val_t    rsp_asscn(val_t,val_t);        // index-based search
val_t    rsp_rplck(val_t,val_t,val_t);  // change the value associated with a particular key
val_t    rsp_rplcn(val_t,val_t,val_t);  // replace the nth element of a collection
val_t    rsp_conj(val_t,val_t);         // add a new element to a collection in a way dispatched by the type

// low-level table searching
val_t intern_string(val_t,char*,hash64_t,int,int,int,int);

// vm builtins and API
val_t vm_eval(val_t,val_t);
val_t vm_apply(val_t,val_t);
val_t vm_evlis(val_t,val_t);

// reader support functions
rsp_tok_t vm_get_token(FILE*);
val_t     vm_read_expr(FILE*);
val_t     vm_read_list(FILE*);

/* initialization */

// memory
void init_heap();
void init_types();
void init_registers();

// builtins
void init_builtin_types();
void init_special_forms();
void init_special_variables();
void init_builtin_functions();

/* toplevel bootstrapping function */
void bootstrap_rascal();

// numeric utilities
unsigned cpow2_32(int);
unsigned long cpow2_64(long);

// string and character utilities
hash64_t hash_str(const char*);
int strsz(const char*);
int u8strlen(const char*);
int u8strcmp(const char*,const char*);
int u8len(const char*);
int u8tou32(wchar_t*,const char*);
wint_t nextu8(const char*);
wint_t nthu8(const char*,size_t);
int iswodigit(wint_t);
int iswbdigit(wint_t);

// io utilities
int peekwc(FILE*);
void fskip(FILE*);

// stack interface - 
#define PUSH(v)     (EVAL[EP++] = (v))
#define POP()       (EVAL[--EP])
#define POPN(n)     (EP -= (n))
#define SAVE(v)     (STACK[SP++] = (v))
#define RESTORE(v)  (STACK[--SP])
#define RESTOREN(n) (SP -= (n))

// basic error handling
void rsp_savestate(rsp_ectx_t*);
void rsp_raise(rsp_err_t);
const char* rsp_errname(rsp_err_t);
void _rsp_print_error(const char*, int, const char*, rsp_err_t, const char*, ...);

#define ERRINFO_FMT  "[%s:%i:%s] %s error: "
#define TYPEERR_FMT  "expected type %s, got %s"
#define ARITYERR_FMT "expected %i args, got %i"

#define rsp_perror(eno, fmt, ...) _rsp_print_error(__FILE__,__LINE__,__func__,eno,fmt, ##__VA_ARGS__)

rsp_ectx_t* rsp_restorestate(rsp_ectx_t*);

// this try catch implementation looks like it should work pretty well, but it requires that all TRY blocks be closed
// with an ETRY (which seems fine)

#define TRY                                                                       \
        rsp_ectx_t ectx__; int eno__, __tr;	                                  \
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
