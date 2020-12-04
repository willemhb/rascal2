#ifndef rascal_h

/* begin common.h */
#define rascal_h

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include "lib/strlib.h"

/* core type  and variable definitions go in this module */
/* typedefs for core builtins */
typedef uintptr_t val_t;
typedef intptr_t  ival_t;
typedef unsigned char uchr_t;
typedef char chr_t;
typedef chr_t str_t;                 // refers to memory in the lisp heap allocated for str
typedef int int_t;
typedef float float_t;
typedef wchar_t ucp_t;
typedef uint32_t uint_t;
typedef struct type_t type_t;
typedef struct obj_t obj_t;
typedef struct cons_t cons_t;
typedef struct sym_t sym_t;
typedef struct proc_t proc_t;
typedef struct dict_t dict_t;
typedef struct port_t port_t;

/* 

   builtin object types and their API
   
   the generic object head contains a 32-bit meta-field (different types can use this for 
   different purposes), 3 1-bit boolean flags, and a type field giving the typecode for
   objects of that type. If a type requires non-boolean flags, these can be stored in
   meta.
 
*/

#define OBJECT_HEAD     \
  struct {              \
    val_t meta : 32;    \
    val_t type : 29;    \
    val_t flags_2 : 1;  \
    val_t flags_1 : 1;	\
    val_t flags_0 : 1;  \
  } head

struct obj_t {
  OBJECT_HEAD;
};

/* These macros provide the core api for working with values  and object types */

#define tag(v)           ((v) & 0x7)
#define untag(v)         (((val_t)(v)) & ~0x7ul)
#define val(v)           (((val_t)(v)) >> 32)
#define ptr(v)           ((void*)untag(v))
#define cptr(v)          ((void*)(v))
#define tagptr(p,t)      (((val_t)(p)) | (t))
#define tagval(v,t)      ((((val_t)(v)) << 32) | (t))
#define heapobj(v)       ((uchr_t*)ptr(v))
#define obj(v)           ((obj_t*)ptr(v))

/* object head accessors */
#define head(o)          (((obj_t*)(o))->head)
#define meta_(o)          (((obj_t*)(o))->head.meta)
#define type_(o)         (((obj_t*)(o))->head.type)   
#define flags_(obj,off)    (((obj_t*)(obj))->head.flags_##off)

/* 
   tag values

   The lowtag indicates how to read the type and data on a val_t. Direct objects store their
   type information and data in the word itself. Object pointers point to an object head 
   with a type field. Cons pointers point to a cons cell that may or may not also be a list
   (the list invariant has not been checked). It has a type of cons. A list pointer points to
   a cons whose list invariant has been checked. It has a type of cons. A string pointer
   points to a utf-8 encoded null terminated bytestring. It has a type of str. 

   The lowtag consobj, listobj, and strobj are pointers to objects whose only data member is
   a cons (stored directly), a list pointer (stored as a pointer), or an array of bytes that
   may be binary data or text data (the bytes begin immediately after the object head). These
   are included to allow future subtyping of any builtin type with a consistent API.

   Other C types can be represented, but cannot be represented directly as val_t for memory
   safety (port_t is an example).
   
   The tag and typecode system is set up so that the special value NIL is identical to the
   C value NULL (NIL has a lowtag of LOWTAG_DIRECT and its typecode is 0).
*/

typedef enum ltag_t {
  LOWTAG_DIRECT  =0b000,
  LOWTAG_CONSPTR =0b001,
  LOWTAG_LISTPTR =0b010,
  LOWTAG_STRPTR  =0b011,
  LOWTAG_OBJPTR  =0b100,
  LOWTAG_CONSOBJ =0b101,
  LOWTAG_LISTOBJ =0b110,
  LOWTAG_STROBJ  =0b111,
} ltag_t;

enum {
  TYPECODE_NIL,
  TYPECODE_CONS,
  TYPECODE_NONE,
  TYPECODE_STR,
  TYPECODE_TYPE,
  TYPECODE_SYM,
  TYPECODE_DICT,
  TYPECODE_PROC,
  TYPECODE_PORT,
  TYPECODE_INT,
  /* the types below this mark have not been implemented yet */
  TYPECODE_FLOAT, // 32-bit floating point number
  TYPECODE_UCP,   // A UTF-32 code point
  NUM_BUILTIN_TYPES = TYPECODE_INT,
};

// this enum supplies the correct lowtags for builtin direct data.

enum {
  INT_LOWTAG = (TYPECODE_INT << 3) || LOWTAG_DIRECT,
  FLOAT_LOWTAG = (TYPECODE_FLOAT << 3) || LOWTAG_DIRECT,
  UCP_LOWTAG = (TYPECODE_UCP << 3) || LOWTAG_DIRECT,
};

// convenience macros for applying the correct tag to an object
#define tagv(v) _Generic((v),                         \
    int_t:tagval(v, INT_LOWTAG),                      \
    float_t:tagval(v, FLOAT_LOWTAG),                  \
			 ucp_t:tagval(v, UCP_LOWTAG))
#define tagp(p) tagptr(p,                               \
		       _Generic((p),                    \
				sym_t*:LOWTAG_STROBJ,   \
				str_t*:LOWTAG_STRPTR,   \
				default:LOWTAG_OBJPTR))


/* convenience macros for casting and testing */
// unsafe casts to builtin types
#define totype_(v) ((type_t*)ptr(v))
#define toint_(v)  ((int_t)val(v))
#define todict_(v)  ((dict_t*)ptr(v))
#define tocons_(v) ((cons_t*)ptr(v))
#define tosym_(v)  ((sym_t*)ptr(v))
#define tostr_(v)  ((str_t*)ptr(v))
#define toproc_(v) ((proc_t*)ptr(v))
#define toport_(v) ((port_t*)ptr(v))


/* 
   convenience macros for safe and fast accessors 

   safe accessors are implemented as functions, and use a safecast to convert their
   argument to the correct type (or escape) before attempting to access its fields. 

   Fast accessors are _Generic macros that can be used with val_t or with pointers to
   the object's C type. They perform a cast if necessary, without performing any checks.

   In general, these should only be used for VM internal tasks, in situations where the
   necessary checks have already been performed.

 */

#define SAFECAST_MACRO(v,tp)			                                       \
  ({ val_t _v_ = v ;                                                                   \
    if (!is##tp(_v_)) escapef(TYPE_ERR,stdout,"Expected %s, got %s",#tp,typename(v)) ; \
    to##tp##_(_v_) ;								       \
  })

#define FAST_ACCESSOR_MACRO(v,ctype,f)                 \
  (_Generic((v),				       \
	    val_t:(ctype)ptr(v),		       \
	    ctype:v)->f)


/* General object APIs; getting type information, metadata, and low-level object information */

uint_t typecode(val_t);
type_t* type_of(val_t);
const chr_t* typename(val_t);
bool callable(val_t);
bool atomic(val_t);
int_t vm_size(val_t);
// generic comparison
int_t cmpv(val_t,val_t);


// fast type checks for builtin types
#define istype(v) (typecode(v) == TYPECODE_TYPE)
#define issym(v)  (typecode(v) == TYPECODE_SYM)
#define iscons(v) (typecode(v) == TYPECODE_CONS)
#define istab(v)  (typecode(v) == TYPECODE_DICT)
#define isstr(v)  (typecode(v) == TYPECODE_STR)
#define isproc(v) (typecode(v) == TYPECODE_PROC)
#define isport(v) (typecode(v) == TYPECODE_PORT)
#define isint(v)  (typecode(v) == TYPECODE_INT)
#define isa(v,t)  (typecode(v) == (t))

// safe casts for builtin types 
#define totype(v)   SAFECAST_MACRO(v,type)
#define tosym(v)    SAFECAST_MACRO(v,sym)
#define todict(v)    SAFECAST_MACRO(v,dict)
#define tocons(v)   SAFECAST_MACRO(v,cons)
#define tostr(v)    SAFECAST_MACRO(v,str)
#define toproc(v)   SAFECAST_MACRO(v,proc)
#define toport(v)   SAFECAST_MACRO(v,port)
#define toint(v)    SAFECAST_MACRO(v,int)


/* rascal general object API */
val_t r_eqp(val_t,val_t);
val_t r_cmp(val_t,val_t);
val_t r_size(val_t,val_t);
val_t r_typeof(val_t);
val_t r_isa(val_t,val_t);
val_t r_nilp(val_t);
val_t r_nonep(val_t);

/* 
   
   Below are the definitions of the core builtin object types, any special flags or enums they
   use, and alias macros for their flag and meta fields (If they have an object head). Flag
   accessors are prefixed with fl_. Functions that will be bound to rascal builtin functions 
   are prefixed with r_.

 */

struct cons_t {
  val_t car;
  val_t cdr;
};

typedef enum cons_sel {
  TAIL=-1,
  A   =0b00010,
  D   =0b00011,
  AA  =0b00100,
  AD  =0b00101,
  DA  =0b00110,
  DD  =0b00111,
  AAA =0b01000,
  AAD =0b01001,
  ADA =0b01010,
  DAA =0b01100,
  ADD =0b01011,
  DAD =0b01101,
  DDA =0b01110,
  DDD =0b01111,
  AAAA=0b10000,
  AAAD=0b10001,
  AADA=0b10010,
  ADAA=0b10100,
  DAAA=0b11000,
  AADD=0b10011,
  ADAD=0b10101,
  DAAD=0b11001,
  DADA=0b11010,
  DDAA=0b11100,
  ADDD=0b10111,
  DADD=0b11011,
  DDAD=0b11101,
  DDDA=0b11110,
  DDDD=0b11111,
} cons_sel;

#define car_(v) FAST_ACCESSOR_MACRO(v,cons_t*,car)
#define cdr_(v) FAST_ACCESSOR_MACRO(v,cons_t*,cdr)

/* the vm api for conses */
val_t tagc(cons_t*);                // because conses are represented by two lowtags, they
val_t car(val_t);                // have a special tagging function
val_t cdr(val_t);
bool  islist(val_t);
val_t cxr(val_t,cons_sel);
int_t ncells(val_t);
val_t cons(val_t,val_t);
val_t append(val_t*,val_t);
val_t extend(val_t*,val_t);

/*  
    the rascal api for conses includes car, cdr, and cons (defined above), as well as
    the following.
 */

val_t r_consp(val_t);
val_t r_rplca(val_t,val_t);
val_t r_rplcd(val_t,val_t);

#define r_cons cons
#define r_car car
#define r_cdr cdr


#define isc iscons
#define toc tocons
#define toc_ tocons_

/* symbols */

struct sym_t {
  OBJECT_HEAD;
  val_t binding;
  chr_t name[1];
};

typedef enum sym_fl {
  VARIABLE=0,
  CONSTANT,
} sym_fl;


#define hash_(s)     FAST_ACCESSOR_MACRO(s,sym_t*,head.meta)
#define name_(s)     FAST_ACCESSOR_MACRO(s,sym_t*,name)
#define fl_const_(s) FAST_ACCESSOR_MACRO(s,sym_t*,head.flags_0)

/* vm api for symbols and strings */

val_t hash(val_t);
chr_t* name(val_t);
val_t sym(chr_t*);
val_t str(chr_t*);
// the functions below are intended to help compare uninterned strings to symbols
int_t cmps(chr_t*,chr_t*);
int_t cmphs(chr_t*,hash_t,chr_t*,hash_t);
/* 
   rascal api for symbols and strings
 */

val_t r_sym(val_t);
val_t r_str(val_t);

/*
  dicts

  dicts are intended to be a core part of the language, giving low level access to a powerful.
  They provide the functionality provided in most lists by 'tables', but my intent is for them
  to be a useful and virtual interface along the lines of Python dicts, so that's what they're
  named. 

  dicts are unbalanced trees whose keys must be atoms (direct data, symbols, or types). In
  the future I plan to implement them as some type of balanced trees whose keys can be
  any type that implements a generic comparison function.

  The global symbol table is implemented as a dict, and the global readtable (once implemented
  will also be implemented as a dict).

 */

struct dict_t {
  OBJECT_HEAD;
  val_t key;
  val_t binding;
  dict_t* left;
  dict_t* right;
};

#define key_(t)     FAST_ACCESSOR_MACRO(t,dict_t*,key)
#define binding_(t) FAST_ACCESSOR_MACRO(t,dict_t*,binding)
#define left_(t)    FAST_ACCESSOR_MACRO(t,dict_t*,left)
#define right_(t)   FAST_ACCESSOR_MACRO(t,dict_t*,right)

/* vm api for dicts */
dict_t* dict();
sym_t* intern_builtin(const chr_t*,val_t);
dict_t* dict_searchk(val_t,dict_t*);
dict_t* dict_searchv(val_t,dict_t*);

/* 
   rascal api for dicts

   The dict constructor takes an arbitrary number of arguments and inserts them into
   a dict, returning the root of the resulting dict.

 */
val_t r_dict(val_t);

/* 
   rascal API for structured objects 

   This api involves four functions - assr, assv, rplcr, rplcv.

   (assr some-dict 'key) returns a context-appropriate object corresponding to the first 
   location matching the key, or NIL if the key can't be found.

   For dicts, this is a cons of the key/value pair.

   For lists, assr returns the cons whose car is the nth element of the list.

   For strings, assr returns the nth character.

   For vectors, assr returns a pair of ('ok . <value>)

   assv looks up a collection's values.

   For dicts, assv is a cons of the first key/value pair whose value is equal to the
   second argument.

   For ordered collections, assv returns the index of the first element equal to v.

   rplcr and rplcv are setters; on success, they return a copy of the original object;
   on failure, they return an error.

   For dicts, rplcr replaces the value associated with the given key with the value of the
   third argument. If the key is not an element in the dictionary, it is added.

   For lists, rplcr inserts a new cell at the given index and sets its value to the third
   argument. If the index is out of range, rplcr raises an error.

   For indexed collections, rplcr replaces the nth value with the given value. If the index
   is out of range, rplcr raises an error.

   dicts do not implement rplcv.

   For lists and indexed collections, rplcv replaces the first occurence of the second 
   argument with the third argument. If no occurences are found, rplcv returns NONE.

   Using either rplcr or rplcv on a string returns a new string (since it may not
   be possible to replace the given index or character without resizing the array).

*/

val_t r_assr(val_t,val_t);
val_t r_assv(val_t,val_t);
val_t r_rplcr(val_t,val_t,val_t);
val_t r_rplcv(val_t,val_t,val_t);

/* 
   environment API 

 */

// environments
bool  isenvnames(val_t);  // test whether the given argument is a valid list of argument names
val_t new_env(val_t,val_t,val_t);
val_t env_assoc(val_t,val_t);
val_t env_put(val_t,val_t);
val_t env_set(val_t,val_t,val_t);

/* 
   rascal environment API 
   
 */

val_t r_lookup(val_t);



/* type objects */

struct type_t {
    OBJECT_HEAD;          // pointer to the type, also holds flags on the data field
    struct {
      val_t base_size : 16;  // the minimum size (in bytes) for objects of this type.
      val_t val_lowtag : 3;  // the lowtag that should be used for objects of this type.
      val_t atomic : 1;      // can values be used as a table key?
      val_t callable : 1;    // can values be used as a function?
      val_t free : 44;
    } flags;
  /* the rascal-callable constructor */
    val_t tp_new;
    // the vm-callable C function to allocate and initialize a new value (NULL for direct data)
    // the vm-callable C function to get a value's size in bytes (can be NULL)
    uint_t (*tp_sizeof)();
    // the vm-callable C function to print or write a value of this type
    void (*tp_prn)(val_t, port_t*);
    // the authoritative type name
    sym_t* tp_name;
};

#define typecode_self_(t)  meta_(t)
#define fl_base_size_(t)    ((t)->flags.base_size)
#define fl_val_lowtag_(t)   ((t)->flags.val_lowtag)
#define fl_atomic_(t)       ((t)->flags.atomic)
#define fl_callable_(t)     ((t)->flags.callable)

/* procedures types */
struct proc_t {
  OBJECT_HEAD;
  val_t formals;
  val_t env;
  val_t body;
};


#define argco_(p)          FAST_ACCESSOR_MACRO(p,proc_t*,head.meta)
#define callmode_(p)       FAST_ACCESSOR_MACRO(p,proc_t*,head.flags_0)
#define bodytype_(p)       FAST_ACCESSOR_MACRO(p,proc_t*,head.flags_1)
#define vargs_(p)          FAST_ACCESSOR_MACRO(p,proc_t*,head.flags_2)
#define formals_(p)        FAST_ACCESSOR_MACRO(p,proc_t*,formals)
#define env_(p)            FAST_ACCESSOR_MACRO(p,proc_t*,env)
#define body_(p)           FAST_ACCESSOR_MACRO(p,proc_t*,body)
#define xpbody_(p)         toc_(body_(p))
#define cfncbody_(p)       ((val_t (*)())body_(p))


typedef enum proc_fl {
  CALLMODE_FUNC,
  CALLMODE_MACRO,
  BODYTYPE_EXPR=0,
  BODYTYPE_CFNC,
  VARGS_TRUE=0,
  VARGS_FALSE,
} proc_fl;

/* VM api for procedures */
val_t new_proc(val_t,val_t,val_t,proc_fl,proc_fl);
bool  check_argco(int_t,val_t,bool);
val_t invoke(val_t(*)(),int_t,val_t);

/* IO ports and reader/tokenizer */

struct port_t {
  OBJECT_HEAD;
  FILE* stream;
  chr_t chrbuff[8];
};

#define fl_iotype_(p)      FAST_ACCESSOR_MACRO(p,port_t*,head.flags_0)
#define fl_readable_(p)    FAST_ACCESSOR_MACRO(p,port_t*,head.flags_1)
#define fl_writable_(p)    FAST_ACCESSOR_MACRO(p,port_t*,head.flags_2)

typedef enum port_fl {
  BINARY_PORT,
  TEXT_PORT,
  READ=1,
  WRITE=1,
} port_fl;

#define stream_(p) FAST_ACCESSOR_MACRO(p,port_t*,stream)
#define buffer_(p) FAST_ACCESSOR_MACRO(p,port_t*,chrbuff)

typedef enum _r_tok_t {
  TOK_LPAR,
  TOK_RPAR,
  TOK_DOT,
  TOK_QUOT,
  TOK_NUM,
  TOK_STR,
  TOK_SYM,
  TOK_EOF,
  TOK_NONE,
  TOK_STXERR,
} r_tok_t;

#define TOKBUFF_SIZE 1024
chr_t TOKBUFF[TOKBUFF_SIZE];
chr_t STXERR[512];
int_t TOKPTR;
r_tok_t TOKTYPE;

#define take() TOKTYPE = TOK_NONE

/* 
   PORTs, IO interface, and the builtin reader
 */

#define MAX_LIST_ELEMENTS 100

val_t  _std_port(FILE*);         // for initializing stdin, stdout, etc.
port_t* vm_open(chr_t*,chr_t*);
int_t vm_close(port_t*);
void vm_prn(val_t,port_t*);
chr_t vm_getc(port_t*);
int_t vm_putc(port_t*, chr_t);
int_t vm_peekc(port_t*);
int_t vm_puts(port_t*, chr_t*);
str_t* vm_gets(port_t*,int_t);
/* reader internal */
r_tok_t get_token(port_t*);
val_t read_expr(port_t*);
val_t read_cons(port_t*);
val_t vm_read(port_t*);
val_t vm_load(port_t*);
// printers for builtin types
void prn_int(val_t,port_t*);
void prn_type(val_t,port_t*);
void prn_dict(val_t,port_t*);
void prn_str(val_t,port_t*);
void prn_cons(val_t,port_t*);
void prn_sym(val_t,port_t*);
void prn_proc(val_t,port_t*);
void prn_port(val_t,port_t*);
/* 
 rascal port and IO interface
 
*/
val_t  r_open(val_t,val_t);
val_t  r_close(val_t);
val_t  r_read(val_t);
val_t  r_reads(val_t);
val_t  r_prn(val_t,val_t);
val_t r_load(val_t);


/*

  main memory, gc, and allocator

  The allocator takes an integer (the base size), an optional pointer to an arbitrary
  object, and a pointer to a function that calculates the size of that object in bytes, aligned
  to a multiple of 8.

  The allocator will align a minimum of 16 bytes, to ensure there is space to replace the object
  with a forwarding pointer during garbage collection. However, only an alignment of 8 can be
  guaranteed, so right now I'm not bothering to try aligning on a 16-byte boundary (if I can
  figure out how to do this I'll switch).
*/

#define AS_BYTES 8
#define AS_WORDS 1
#define MAXTYPES UINT_MAX >> 3 
#define MAXBASE USHRT_MAX
#define INIT_HEAPSIZE 8 * 2048 // in bytes
#define INIT_NUMTYPES 256      // number of records
#define RAM_LOAD_FACTOR 0.8    // resize ram if, directly after garbage collection, (FREE - HEAP) > more than RAM_LOAD_FACTOR * (HEAPSIZE)
#define HEAP_ALIGNSIZE 8       // the alignment of the heap

#define CHECK_RESIZE() (FREE - HEAP) > (RAM_LOAD_FACTOR * HEAPSIZE) ? true : false

int_t align_heap(uchr_t**,uchr_t*);
void gc();
val_t gc_trace(val_t*);
uchr_t* gc_copy(uchr_t*,int_t);
uchr_t* vm_allocate(int_t);
type_t* new_type();                         

/* memory bounds checking and overflow checking */
bool heap_limit(uint_t);
bool type_limit(uint_t);
bool type_overflow(uint_t);
bool stack_overflow(uint_t);
bool type_overflow(uint_t);

/* special registers holding important memory locations and parameters */
uchr_t *HEAP, *EXTRA, *FREE, *FROMSPACE, *TOSPACE;
ival_t HEAPSIZE;
bool GROWHEAP;
uint_t TYPECOUNTER, NUMTYPES;
type_t** TYPES;

#define ALLOCATIONS() (FREE - HEAP)

/* evaluator and the stack */
// the stack
#define MAXSTACK 4096          // arbitrary
val_t* STACK;
int_t STACKSIZE, SP;

// The core evaluator registers
val_t EXP, VAL, CONTINUE, NAME, ENV, UNEV, ARGL, PROC;
// working registers (never saved, always free)
val_t WRX, WRY, WRZ; 
dict_t* GLOBALS;
// special constants
// The lowtags and typecodes are laid out so that NIL, including correct lowtag and typecode,
// is equal to C 'NULL'
// FPTR is a special value used to indicate that this cell has been moved to the new heap
// during garbage collection. 
val_t NIL, NONE, OK, T, FPTR;
val_t R_STDIN, R_STDOUT, R_STDERR, R_PROMPT;


// working with the stack
void  push(val_t);
val_t pop();
val_t peek();

// convenience macros for working with registers
#define save push
#define restore(r) r = pop()
#define failf(c,e,fmt,args...) if (c) do { escapef(e,stderr,fmt,##args); } while (0)
#define fail(e,fmt,args...) do { escpapef(e,stderr,fmt,##args); } while (0)
#define branch(c,l) if (c) goto *labels[l]
#define jump(l) goto *labels[l]
#define dispatch(r) goto *labels[vm_analyze(r)]

// evaluator core
val_t analyze_expr(val_t);
val_t eval_expr(int_t,val_t,val_t);

/* rascal api for the evaluator */
val_t r_eval(val_t,val_t);
val_t r_apply(val_t,val_t);

// builtin forms and VM evaluator labels
enum {
  EV_NORMAL_START=-1,
  EV_SETV,
  EV_QUOTE,
  EV_LET,
  EV_DO,
  EV_FN,
  EV_MACRO,
  EV_IF,
  NUM_FORMS,
  EV_LITERAL=NUM_FORMS,
  EV_VARIABLE,
  EV_ASSIGN,
  EV_ASSIGN_END,
  EV_APPLY,
  EV_APPLY_OP_DONE,
  EV_APPLY_ARGLOOP,
  EV_APPLY_ACCUMARG,
  EV_APPLY_DISPATCH,
  EV_RETURN,
  EV_MACRO_RETURN,
  EV_SEQUENCE_START,
  EV_SEQUENCE_LOOP,
  EV_SETV_ASSIGN,
  EV_LET_ARGLOOP,
  EV_IF_TEST,
  EV_IF_ALTERNATIVE,
  EV_IF_NEXT,
  EV_APPLY_TYPE,
  EV_APPLY_MACRO,
  EV_APPLY_BUILTIN,
  EV_APPLY_FUNCTION,
  EV_HALT,
};

// a separate set of labels to jump to when ERRORCODE is nonzero
enum {
  EVERR_TYPE=1,
  EVERR_VALUE,
  EVERR_ARITY,
  EVERR_UNBOUND,
  EVERR_OVERFLOW,
  EVERR_IO,
  EVERR_NULLPTR,
  EVERR_SYNTAX,
  EVERR_INDEX,
};

// save the symbols bound to builtin forms for faster evaluation
val_t BUILTIN_FORMS[NUM_FORMS];

#define isnil(v)         ((v) == NIL)
#define isnone(v)        ((v) == NONE)
#define isok(v)          ((v) == OK)
#define istrue(v)        ((v) == T)
#define isfptr(v)        ((v) == FPTR)
#define cbooltorbool(v)  ((v) ? T : NIL)
#define rbooltocbool(v)  ((v) == NIL || (v) == NONE ? false : true)

/* errors and logging */

#define STDLOG_PATH "log/rascal.log"
#define STDHIST_PATH "log/rascal.history.log"

/* 
   Safety should be used to jump directly to the top level in the event of critical errors 
   like memory overflow, stack overflow, potential segfault, etc. Common errors should return
 
   Common errors should be signaled using a few conventional sentinel values based on the
   expected return type.

   NULL  - functions that return pointers should return NULL to signal an error.
   -2    - functions that return integers should return -2 to signal an error (
           -1 is used for interning symbols, so using -1 would lead to a lot of unnecessary
           error checking).
   NONE  - functions that return val_t should return the special rascal value NONE to signal
           an error.

   Additionally, since these sentinels might be normal return values, the functions that return
   them should set the global ERRORCODE variable. An error occured if and only if a return value
   is one of the above, AND the value of ERRORCODE is non-zero.

   This module provides a few macros and functions to simplify raising and handling errors.

   the checks(v) macro checks that both error conditions are true.

   the fail(e,s,fmt,args...) macro sets the error status, writes the supplied message to stdlog,
   and returns the given sentinel.

   ifail, rfail, and cfail fail with the sentinels -2, NONE, and NULL respectively.

   the clear macro resets the value of ERRORCCODE to 0.

   if checks returns true, then it MUST be followed by a call to fail or clear, or future
   error codes will be unreliable.
*/

FILE* stdlog;
jmp_buf SAFETY;

typedef enum r_errc_t {
  OK_ERR,
  TYPE_ERR,
  VALUE_ERR,
  ARITY_ERR,
  UNBOUND_ERR,
  OVERFLOW_ERR,
  IO_ERR,
  NULLPTR_ERR,
  SYNTAX_ERR,
  INDEX_ERR,
  APPLICATION_ERR,
} r_errc_t;

#define eprintf(e, file, fmt, args...)				                     \
  do                                                                                 \
  {									             \
  fprintf(file, "%s: %d: %s: %s ERROR: ",__FILE__,__LINE__,__func__,get_errname(e)); \
  fprintf(file,fmt,##args);                                                          \
  fprintf(file,".\n");                                                               \
  } while (0)								             \

//  print common error messages to the log file.
#define elogf(e,fmt,args...) eprintf(e,stdlog,fmt,##args)
// shorthand for jumping to the safety point. The safety point prints an informative message
// and initiates program exit.

#define escape(e) longjmp(SAFETY, e)

// since they're called together a lot, escapef composes them 
#define escapef(e, file, fmt, args...)				                     \
  do                                                                                 \
  {									             \
  fprintf(file, "%s: %d: %s: %s ERROR: ",__FILE__,__LINE__,__func__,get_errname(e)); \
  fprintf(file,fmt,##args);                                                          \
  fprintf(file,".\n");                                                               \
  longjmp(SAFETY,e);							             \
  } while (0)								             \

// error reporting functions
const chr_t* get_errname(int_t);
void init_log();                 // initialize the global variable stdlog. Stdlog is 
chr_t* read_log(chr_t*, int_t);  // read the log file into a string buffer.
void dump_log(FILE*);            // dump the contents of the log file into another file.
int_t finalize_log();            // copy the contents of the log to the history


#define DESCRIBE_ARITHMETIC(name,op,rtn)	       \
  inline val_t name(val_t x, val_t y) {                \
  int_t xn = toint(x) ;                                \
  int_t yn = toint(y) ;                                \
  return rtn(op(xn,yn)) ;			       \
}


#define DESCRIBE_PREDICATE(name,test)	               \
  inline val_t name(val_t x) {                         \
    if (test(x)) return T ;			       \
    else return NIL ;				       \
}

#define DECLARE(name,argco) DECLARE_##argco(name)
#define DECLARE_1(name) val_t name(val_t)
#define DECLARE_2(name) val_t name(val_t,val_t)

DECLARE(r_add,2);
DECLARE(r_sub,2);
DECLARE(r_mul,2);
DECLARE(r_div,2);
DECLARE(r_rem,2);
DECLARE(r_eqnum,2);
DECLARE(r_lt,2);
DECLARE(r_nilp,1);
DECLARE(r_nonep,1);

/* int builtin */
val_t r_int(val_t);

#define NUM_BUILTINS 30

/* initialization */
/* bootstrapping builtin functions */
void _new_builtin_function(const chr_t*,int_t,const void*);  
val_t  _new_form_name(chr_t*);                 
val_t  _new_self_evaluating(chr_t*);

/* memory initialization */
void init_heap();
void init_types();
void init_registers();

/* initialization of core language */
void init_builtin_types();
void init_forms();
void init_special_variables();
void init_builtin_functions();

/* toplevel bootstrapping function */
void bootstrap_rascal();

/* end rascal.h */
#endif
