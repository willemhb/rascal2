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

/*  structs to represent builtin object types */

struct list_t {
  val_t car;
  struct list_t* cdr;
};


struct cons_t {
  val_t car;
  val_t cdr;
};

struct sym_t {
  val_t hash;          // this symbol's hash, plus some flags
  char name[1];        // the name hangs off the end
};

struct bvec32_t {
  val_t meta    : 61;       // combination of the bitmap and additional metadata
  val_t vecsz   :  3;       // the base size of the bitvector as a power of two (max 5; 6 and 7)
  val_t elements[1];        // are special cases for AVL set nodes and AVL dict nodes
};

struct bvec64_t {
  val_t meta    : 61;       // flag bits
  val_t vecsz   :  3;       // the base size of the bitvector as a power of two (max 6)
  val_t bmap;               // bitmap of the vector's fields
  val_t elements[2];        // the elements themselves
};

struct node_t {
  val_t  order    : 59; // the original insertion order
  val_t  balance  :  3; // the balance factor
  val_t  fieldcnt :  2;
  node_t* left;
  node_t* right;
  val_t _node_fields[1];
};


struct fvec_t {
  val_t allocated;
  val_t elements[1];
};

struct dvec_t {
  val_t elements;  // a flagged pointer to the underlying fvec
  val_t curr;      // the offset of the next free cell in the underlying fvec
};

struct code_t {
  val_t codesz;          // size of the instruction sequence in bytes
  unsigned char* instr;  // pointer to the bytecode instruction sequence
  val_t values[2];       // an array of constant values used in the code body
};

/* 
   objects with explicit type pointers - these objects fill out the space that would 
   otherwise be filled by padding with a "_<type>_fields" array, which allows subtypes 
   with additional fields to be represented by these structs.

 */

struct obj_t {
  OBJECT_HEAD;
  val_t _fields[1];
};


struct table_t {
  OBJECT_HEAD;            // a flagged pointer to the object head
  node_t* records;        // a pointer to the elements holding the 
  val_t count;            // number of elements
  val_t _table_fields[1]; // other fields (used by extension types)
};

struct function_t {
  OBJECT_HEAD;
  val_t argco   : 32;
  val_t kwargco : 32;
  val_t template;
  val_t closure;              // the captured environment frame
  val_t envt;                 // the local environment
  val_t _function_fields[1];  // other fields (first is the generic, others are used by
};                            // extension types)

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
  val_t (*tp_new)(val_t*,int);                          // the rascal callable constructor
  int   (*tp_init)(val_t,val_t*,int);                   // the initializer
  val_t (*tp_copy)(val_t);                              // garbage collector
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
extern val_t EXP, VALUE, ENVT, CONT, TEMPLATE;

/* 
   the stacks used by the virtual machine

   EVAL - holds evaluated sub-expressions needed by the current procedure or expression
   STACK - a more general stack used to save values that will be needed later

*/

extern val_t *EVAL;

// offset registers
extern size_t SP, PC;

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
extern char TOKBUFF[];
extern int TOKPTR;
extern int LASTCHR;
extern rsp_tok_t TOKTYPE;

// builtin form names
const char* FORM_NAMES[] = { "setv", "def", "quote", "if", "fun", "macro", "do", "let" };


const char* SPECIAL_VARIABLES[] = { "*env*", "*stdin*", "*stdout*", "*stderr*",
                                    "nil", "none", "t" };

// these are used to interface to C's IO and wctype facilities
const char* IOWC_SYMBOLS[] = { "r",  "w", "a", "r+", "w+", "a+",
                               "alnum", "alpha", "blank", "cntrl",
			       "digit", "graph", "lower", "print",
                               "space",  "upper",   "xdigit", };

// the symbols corresponding to the available special forms
extern val_t F_SETV, F_DEF, F_QUOTE, F_IF, F_FUN, F_MACRO, F_DO, F_LET;

// other important global variables; right now, this is just the symbol table and the global environment
extern val_t R_SYMTAB, R_TOPLEVEL, R_VARGSSYM;

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

#define UNSAFE_ACCESSOR(t,v,cnvt,f)  \
   _Generic((v),		     \
           t:v,		             \
           val_t:cnvt((val_t)(v)))->f

#define ntag(v)              ((v) & 0xf)
#define wtag(v)              ((v) & 0xff)
#define addr(v)              ((v) & (~0xful))
#define bits(d)              (((rsp_c32_t)(d)).bits32)
#define pad(d)               (((val_t)bits(d)) << 32)
#define unpad(d)             ((d) >> 32)
#define tag_v(v,t)           (pad(v) | (t))
#define tag_p(p,t)           (((val_t)(p)) | (t))
#define ptr_(t,v)            ((t)addr(v))
#define uval_(v)             ((unsigned)unpad(v))
#define ival_(v)             (((rsp_c32_t)((unsigned)unpad(v))).integer)
#define fval_(v)             (((rsp_c32_t)((unsigned)unpad(v))).float32)
#define aligned(p,n)         (((val_t)(p)) % (n) == 0)

#define type_(v)                             \
  (_Generic((v),		             \
    obj_t*:v,                                \
    type_t*:v,                               \
    table_t*:v,                              \
    sym_t*:v,                                \
    function_t*:v,		             \
    default:toobj_((val_t)(v)))->type)

#define flags_(v)                            \
  ((unsigned)(_Generic((v),		     \
    obj_t*:v,                                \
    type_t*:v,                               \
    table_t*:v,                              \
    sym_t*:v,                                \
    function_t*:v,		             \
    default:toobj_((val_t)(v)))->type & (0xf)))


type_t* get_val_type(val_t);             // get the type object associated with the value
int get_val_size(val_t);                 // get the size of an object
int val_len(val_t);                      // generic length
char* get_val_type_name(val_t);          // get the object type name
c_num_t get_val_cnum(val_t);
bool isdirect(val_t);
bool isatomic(val_t);
bool iscallable(val_t);
bool isnumeric(val_t);
bool hasflags(val_t);

// handling different numeric types
val_t   rsp_itof(val_t);
val_t   rsp_ftoi(val_t);
c_num_t get_common_cnum(val_t,val_t);
c_num_t promote_cnum(val_t*,val_t*);

// fast comparisons and tests
#define isnil(v)      ((v) == R_NIL)
#define isnone(v)     ((v) == R_NONE)
#define istrue(v)     ((v) == R_TRUE)
#define isfalse(v)    ((v) == R_FALSE)
#define isfptr(v)     ((v) == R_FPTR)
#define iseof(v)      ((v) == R_EOF)
#define isunbound(v)  ((v) == R_UNBOUND)

// tag-based tests for builtin types
#define hasltag(v,t)        (ltag(v) == (t))
#define hasoflags(v,lt,ot)  ({ val_t __v__ = v ; hasltag(__v__,lt) && (flags_(__v__) & (ot)); })
#define islist(v)       hasltag(v,LTAG_LIST)
#define iscons(v)       hasltag(v,LTAG_CONS)
// special test for distinguishing objects that have an accessible field called 'car'
#define hascar(v)       ({ val_t __v__ = v ; __v__ && ((__v__ & 0xf) <= LTAG_CONS) ; })
#define isstr(v)        hasltag(v,LTAG_STR)
#define iscfile(v)      hasltag(v,LTAG_CFILE)
#define issym(v)        hasltag(v,LTAG_SYM)
#define isnode(v)       hasltag(v,LTAG_NODE)
#define isfvec(v)       hasltag(v,LTAG_FVEC)
#define isdvec(v)       hasltag(v,LTAG_DVEC)
#define iscode(v)       hasltag(v,LTAG_CODE)
#define isobj(v)        hasltag(v,LTAG_OBJECT)
#define istable(v)      hasltag(v,LTAG_TABLEOBJ)
#define isfunction(v)   hasltag(v,LTAG_FUNCOBJ)
#define istype(v)       hasltag(v,LTAG_METAOBJ)
#define isint(v)        hasltag(v,LTAG_INT)
#define isfloat(v)      hasltag(v,LTAG_FLOAT)
#define ischar(v)       hasltag(v,LTAG_CHAR)
#define isbool(v)       hasltag(v,LTAG_BOOL)
#define isbuiltin(v)    hasltag(v,LTAG_BUILTIN)

// tag based tests for special VM data structures
#define hastabflags(v,t)   hasoflags(v,LTAG_TABLEOBJ,t)
#define hassymflags(v,t)   hasoflags(v,LTAG_SYM,t)
#define hasfuncflags(v,t)  hasoflags(v,LTAG_FUNCOBJ,t)
#define hasdvecflags(v,t)  hasoflags(v,LTAG_DVEC,t)
#define issymtab(v)        hastabflags(v,TABLEFLAG_SYMTAB)
#define isenvt(v)          hastabflags(v,TABLEFLAG_ENVT)
#define isnmspc(v)         hastabflags(v,TABLEFLAG_NMSPC)
#define isreadtab(v)       hastabflags(v,TABLEFLAG_READTAB)
#define isinterned(v)      hassymflags(v,SYMFLAG_INTERNED)
#define iskeyword(v)       hassymflags(v,SYMFLAG_KEYWORD)
#define isreserved(v)      hassymflags(v,SYMFLAG_RESERVED)
#define isgensym(v)        hassymflags(v,SYMFLAG_GENSYM)
#define ismacro(v)         hasfuncflags(v,FUNCFLAG_MACRO)
#define ismethod(v)        hasfuncflags(v,FUNCFLAG_METHOD)
#define hasvargs(v)        hasfuncflags(v,FUNCFLAG_VARGS)
#define hasvarkw(v)        hasfuncflags(v,FUNCFLAG_VARKW)
#define isenvtframe(v)     hasdvecflags(v,DVECFLAG_ENVTFRAME)

// unsafe casts
#define tolist_(v)      ptr_(list_t*,v)
#define tocons_(v)      ptr_(cons_t*,v)
#define tostr_(v)       ptr_(char*,v)
#define tocfile_(v)     ptr_(FILE*,v)
#define tosym_(v)       ptr_(sym_t*,v)
#define tonode_(v)      ptr_(node_t*,v)
#define tofvec_(v)      ptr_(fvec_t*,v)
#define todvec_(v)      ptr_(dvec_t*,v)
#define tocode_(v)      ptr_(code_t*,v)
#define toobj_(v)       ptr_(obj_t*,v)
#define totable_(v)     ptr_(table_t*,v)
#define tofunction_(v)  ptr_(function_t*,v)
#define totype_(v)      ptr_(type_t*,v)
#define toint_(v)       ival_(v)
#define tofloat_(v)     fval_(v)
#define tochar_(v)      ((wchar_t)ival_(v))
#define tobool_(v)      ((bool)uval_(v))
#define tobuiltin_(v)   BUILTIN_FUNCTIONS[uval_(v)]

// functions that check types before performing the requested access
list_t*      _tolist(val_t,const char*,int,const char*);
cons_t*      _tocons(val_t,const char*,int,const char*);
char*        _tostr(val_t,const char*,int,const char*);
FILE*        _tocfile(val_t,const char*,int,const char*);
sym_t*       _tosym(val_t,const char*,int,const char*);
node_t*      _tonode(val_t,const char*,int,const char*);
fvec_t*      _tofvec(val_t,const char*,int,const char*);
dvec_t*      _todvec(val_t,const char*,int,const char*);
code_t*      _tocode(val_t,const char*,int,const char*);
obj_t*       _toobj(val_t,const char*,int,const char*);
table_t*     _totable(val_t,const char*,int,const char*);
function_t*  _tofunction(val_t,const char*,int,const char*);
type_t*      _totype(val_t,const char*,int,const char*);
int          _torint(val_t,const char*,int,const char*);
float        _torfloat(val_t,const char*,int,const char*);
wchar_t      _torchar(val_t,const char*,int,const char*);
bool         _torbool(val_t,const char*,int,const char*);
rsp_cfunc_t  _tobuiltin(val_t,const char*,int,const char*);

// these macros interface with the functions above, passing the calling function's
// debugging information
#define tolist(v)      _tolist(v,__FILE__,__LINE__,__func__)
#define tocons(v)      _tocons(v,__FILE__,__LINE__,__func__)
#define tostr(v)       _tostr(v,__FILE__,__LINE__,__func__)
#define tocfile(v)     _tocfile(v,__FILE__,__LINE__,__func__)
#define tosym(v)       _tosym(v,__FILE__,__LINE__,__func__)
#define tonode(v)      _tonode(v,__FILE__,__LINE__,__func__)
#define tofvec(v)      _tofvec(v,__FILE__,__LINE__,__func__)
#define todvec(v)      _todvec(v,__FILE__,__LINE__,__func__)
#define tocode(v)      _tocode(v,__FILE__,__LINE__,__func__)
#define toobj(v)       _toobj(v,__FILE__,__LINE__,__func__)
#define totable(v)     _totable(v,__FILE__,__LINE__,__func__)
#define tofunction(v)  _tofunction(v,__FILE__,__LINE__,__func__)
#define totype(v)      _totype(v,__FILE__,__LINE__,__func__)
#define toint(v)       _toint(v,__FILE__,__LINE__,__func__)
#define tofloat(v)     _tofloat(v,__FILE__,__LINE__,__func__)
#define tochar(v)      _tochar(v,__FILE__,__LINE__,__func__)
#define tobool(v)      _tobool(v,__FILE__,__LINE__,__func__)
#define tobuiltin(v)   _tobuiltin(v,__FILE__,__LINE__,__func__)

// general object convenience functions and predicates
bool cbool(val_t);                // convert rascal value to C boolean
val_t safe_ptr(val_t);            // follow any forwarding pointers to the current location
val_t check_fptr(val_t*);         // check for forwarding pointer on an arbitrary 
val_t update_fptr(val_t*);        // recursively update a forwarding pointer

// memory management and bounds checking
// allocation
unsigned char* rsp_alloc(size_t);    // allocate an aligned block at least n bytes in size
unsigned char* rsp_allocw(size_t);   // allocate n word-sized blocks (maintaining alignment)
val_t          rsp_new(type_t*,val_t*,int);                     // root constructor
unsigned char* rsp_copy(unsigned char*,unsigned char*,size_t);  // generic copier

// gc
void gc();
val_t gc_trace(val_t*);
bool in_heap(val_t, val_t*,val_t*);
bool heap_limit(unsigned long);
bool stack_overflow(val_t*, unsigned long, unsigned long);

/* object apis */
// genricized unsafe accessors
#define car_(v)             UNSAFE_ACCESSOR(cons_t*,v,tocons_,car)
#define cdr_(v)             UNSAFE_ACCESSOR(cons_t*,v,tocons_,cdr)
#define first_(v)           UNSAFE_ACCESSOR(list_t*,v,tolist_,car)
#define rest_(v)            UNSAFE_ACCESSOR(list_t*,v,tolist_,cdr)
#define symname_(v)         UNSAFE_ACCESSOR(sym_t*,v,tosym_,name)
#define symhash_(v)         UNSAFE_ACCESSOR(sym_t*,v,tosym_,hash)
#define order_(v)           UNSAFE_ACCESSOR(node_t*,v,tonode_,order)
#define balance_(v)         UNSAFE_ACCESSOR(node_t*,v,tonode_,balance)
#define fieldcnt_(v)        UNSAFE_ACCESSOR(node_t*,v,tonode_,fieldcnt)
#define left_(v)            UNSAFE_ACCESSOR(node_t*,v,tonode_,left)
#define right_(v)           UNSAFE_ACCESSOR(node_t*,v,tonode_,right)
#define hashkey_(v)         UNSAFE_ACCESSOR(node_t*,v,tonode_,_node_fields)[0]
#define bindings_(v)         ((val_t*)(&(UNSAFE_ACCESSOR(node_t*,v,tonode_,_node_fields)[1])))
#define fvec_elements_(v)   UNSAFE_ACCESSOR(fvec_t*,v,tofvec_,elements)
#define fvec_allocated_(v)  UNSAFE_ACCESSOR(fvec_t*,v,tofvec_,allocated)
#define dvec_elements_(v)   (((fvec_t*)(UNSAFE_ACCESSOR(dvec_t*,v,todvec_,elements) & (~0xful)))->elements)
#define dvec_allocated_(v)  (((fvec_t*)(UNSAFE_ACCESSOR(dvec_t*,v,todvec_,elements) & (~0xful)))->allocated)
#define dvec_curr_(v)       UNSAFE_ACCESSOR(dvec_t*,v,todvec_,curr)
#define records_(v)         UNSAFE_ACCESSOR(table_t*,v,totable_,records)
#define count_(v)           UNSAFE_ACCESSOR(table_t*,v,totable_,count)
#define fnargco_(v)         UNSAFE_ACCESSOR(function_t*,v,tofunction_,argco)
#define template_(v)        UNSAFE_ACCESSOR(function_t*,v,tofunction_,template)
#define closure_(v)         UNSAFE_ACCESSOR(function_t*,v,tofunction_,closure)
#define envt_(v)            UNSAFE_ACCESSOR(function_t*,v,tofunction_,envt)

/* basic object apis */
// lists & conses
val_t mk_list(int);                     
int   init_list(val_t,val_t*,int);
void  prn_list(val_t,FILE*);
val_t mk_cons();
val_t vm_cons(val_t,val_t);
int   init_cons(val_t,val_t*,int);
void  prn_cons(val_t,FILE*);

val_t _car(val_t,const char*,int,const char*);
val_t _cdr(val_t,const char*,int,const char*);

#define car(v) _car(v,__FILE__,__LINE__,__func__)
#define cdr(v) _cdr(v,__FILE__,__LINE__,__func__)

// cfiles
val_t mk_cfile(FILE*);

// strings
val_t mk_str(const char*);
void  prn_str(val_t,FILE*);

// symbols
val_t mk_sym(char*,int);                                    // name, flags
val_t new_sym(char*,size_t,hash64_t,int,unsigned char*);
void  prn_sym(val_t,FILE*);
hash64_t symhash(val_t);
char*    symname(val_t);

// AVL nodes API
node_t* new_node(int,int,unsigned char*);
val_t mk_node(int,int,unsigned char*);                        // number of extra fields
node_t* node_search(node_t*,val_t);
int node_insert(node_t**,node_t**,val_t,int,int,unsigned char*);
int node_intern(node_t**,val_t*,char*,hash64_t,int,int);
int balance_factor(node_t*);

#define right_heavy(n) (balance_(n) > 3)
#define left_heavy(n)  (balance_(n) < 3)
#define unbalanced(n)  ({ int __b__ = balance_(n) ; __b__ > 4 || __b__ < 2 ; })

int node_delete(node_t**,val_t);
void balance_node(node_t**);
node_t* rotate_ll(node_t*,node_t*);
node_t* rotate_rr(node_t*,node_t*);
node_t* rotate_lr(node_t*,node_t*);
node_t* rotate_rl(node_t*,node_t*);

// fixed vectors and dynamic vectors
val_t mk_fvec(int);                       // element count
int   init_fvec(val_t,val_t*,int);
void  prn_fvec(val_t,FILE*);
val_t mk_dvec(int);                       // element count
int   init_dvec(val_t,val_t*,int);
void prn_dvec(val_t,FILE*);

val_t*    _vec_elements(val_t,const char*,int,const char*);
unsigned  _vec_allocated(val_t,const char*,int,const char*);
unsigned  _vec_curr(val_t,const char*,int,const char*);
val_t     vec_append(val_t,val_t);

#define vec_elements(v)  _vec_elements(v,__FILE__,__LINE__,__func__)
#define vec_allocated(v) _vec_allocated(v,__FILE__,__LINE__,__func__)
#define vec_curr(v)      _vec_curr(v,__FILE__,__LINE__,__func__)

// val_t mk_code(unsigned char*,int,int);    // code, code length, number of constants
// int   init_code(val_t*,int);
val_t mk_obj(type_t*);                       // create an object of the specified type

// table objects
val_t mk_table(int,int);                     // number of entries, flags
void prn_table(val_t,FILE*);
int  nodesz(table_t*);
int   init_table(val_t,val_t*,int);
val_t table_insert(val_t,val_t);
val_t table_delete(val_t,val_t);
val_t table_lookup(val_t,val_t);
val_t table_update(val_t,val_t,val_t);
val_t intern_string(val_t,char*,hash64_t,int); // return an interned symbol

// function objects
val_t mk_function(val_t,val_t,val_t,int); // template, formals, captured envt, flags

// direct types
// integers
val_t mk_int(int);
void prn_int(val_t,FILE*);

// floating point numbers
val_t mk_float(float);
void prn_float(val_t,FILE*);

// unicode characters
val_t mk_char(int);
void prn_char(val_t,FILE*);

// booleans
val_t mk_bool(int);
void prn_bool(val_t,FILE*);

// builtin functions
val_t mk_builtin(val_t(*)(val_t*,int),int,bool,int); // C func, argco, vargs, index

// vm constructors for special types
val_t mk_envtframe(val_t,val_t,int);       // envt, closure, # of bindings
val_t mk_envt(val_t,int*);                 // list of names, address to leave vargs flag
val_t save_cnt(val_t,val_t*,int);
val_t restore_cnt(val_t);

// environment API
int env_bind(val_t,val_t);                 // bind a value in the next free location in a frame
int env_bindn(val_t,size_t);               // bind the first N arguments on EVAL
val_t env_extend(val_t,val_t);
val_t env_assign(val_t,val_t,val_t);
val_t env_lookup(val_t,val_t);

// object comparison
int vm_ord(val_t,val_t);  // order two comparable values

// reader/printer support functions
rsp_tok_t vm_get_token(FILE*);
val_t     vm_read_expr(FILE*);
val_t     vm_read_cons(FILE*);
void      vm_print(val_t,FILE*);
void      vm_print_val(val_t,FILE*);  // fallback printer
val_t     vm_load(FILE*,val_t);

// evaluator core (these are also builtin functions)
val_t rsp_eval(val_t,val_t);
val_t rsp_apply(val_t,val_t);
val_t rsp_evlis(val_t,val_t);

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
unsigned long clog2(unsigned long);

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
int fskip(FILE*);
int fskipto(FILE*,wint_t);

// simple stack interface
#define PUSH(v)     (EVAL[SP++] = (v))
#define POP()       (EVAL[--SP])
#define POPN(n)     (SP -= (n))
#define SAVE(v)     ({ EVAL[SP++] = (v) ; &(EVAL[0]) ; })

// basic error handling
void rsp_savestate(rsp_ectx_t*);
void rsp_raise(rsp_err_t);
const char* rsp_errname(rsp_err_t);
void _rsp_perror(const char*, int, const char*, rsp_err_t, const char*, ...);

#define ERRINFO_FMT  "[%s:%i:%s] %s error: "
#define TYPEERR_FMT  "expected type %s, got %s"
#define ARITYERR_FMT "expected %i args, got %i"

#define rsp_perror(eno, fmt, ...) _rsp_perror(__FILE__,__LINE__,__func__,eno,fmt, ##__VA_ARGS__)

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
