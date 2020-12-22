#ifndef rascal_h

/* begin common.h */
#define rascal_h

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <wctype.h>
#include <locale.h>
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
typedef wint_t uchri_t;
typedef uint32_t uint_t;
typedef struct type_t type_t;
typedef struct obj_t obj_t;
typedef struct cons_t cons_t;
typedef struct sym_t sym_t;
typedef struct proc_t proc_t;
typedef struct table_t table_t;
typedef struct cdata_t cdata_t;

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

#define tag(v)           ((v) & 0x7ul)
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
   safety (cdata_t is an example).
   
   The tag and typecode system is set up so that the special value NIL is identical to the
   C value NULL (NIL has a lowtag of LOWTAG_DIRECT and its typecode is 0).
*/

typedef enum ltag_t {
  LOWTAG_DIRECT  =0b000ul,
  LOWTAG_CONSPTR =0b001ul,
  LOWTAG_LISTPTR =0b010ul,
  LOWTAG_STRPTR  =0b011ul,
  LOWTAG_OBJPTR  =0b100ul,
  LOWTAG_CONSOBJ =0b101ul,
  LOWTAG_LISTOBJ =0b110ul,
  LOWTAG_STROBJ  =0b111ul,
} ltag_t;

enum {
  TYPECODE_NIL,
  TYPECODE_CONS,
  TYPECODE_NONE,
  TYPECODE_STR,
  TYPECODE_TYPE,
  TYPECODE_SYM,
  TYPECODE_TABLE,
  TYPECODE_PROC,
  TYPECODE_CDATA,      // A box holding A C value along with type information       
  TYPECODE_INT,
  TYPECODE_UCP,        // A UTF-32 codepoint
  NUM_BUILTIN_TYPES, 
  /* the types below this mark have not been implemented yet */
  TYPECODE_FLOAT, // 32-bit floating point number
  TYPECODE_ANY,
};

// this enum supplies the correct lowtags for builtin direct data.

enum {
  INT_LOWTAG = TYPECODE_INT << 3,
  FLOAT_LOWTAG = TYPECODE_FLOAT << 3,
  UCHR_LOWTAG = TYPECODE_UCP << 3,
};

// convenience macros for applying the correct tag to an object
#define tagv(v) _Generic((v),                         \
    int_t:tagval(v, INT_LOWTAG),                      \
			 float_t:tagval(v, FLOAT_LOWTAG))

#define tagp(p) tagptr(p,                               \
		       _Generic((p),                    \
				sym_t*:LOWTAG_STROBJ,   \
				str_t*:LOWTAG_STRPTR,   \
				default:LOWTAG_OBJPTR))


/* convenience macros for casting and testing */
// unsafe casts to builtin types
#define totype_(v) ((type_t*)ptr(v))
#define touchr_(v)  ((uchr_t)val(v))
#define toint_(v)  ((int_t)val(v))
#define totable_(v)  ((table_t*)ptr(v))
#define tocons_(v) ((cons_t*)ptr(v))
#define tosym_(v)  ((sym_t*)ptr(v))
#define tostr_(v)  ((str_t*)ptr(v))
#define toproc_(v) ((proc_t*)ptr(v))
#define toport_(v) ((cdata_t*)ptr(v))


/* 
   convenience macros for safe and fast accessors 

   safe accessors are implemented as functions, and use a safecast to convert their
   argument to the correct type (or escape) before attempting to access its fields. 

   Fast accessors are _Generic macros that can be used with val_t or with pointers to
   the object's C type. They perform a cast if necessary, without performing any checks.

   In general, these should only be used for VM internal tasks, in situations where the
   necessary checks have already been performed.

   The safecast macro also checks for forwarding pointers. 

 */

#define DESCRIBE_PTR_SAFECAST(ctp,rtp)	                                                      \
  ctp safecast_##rtp##_(val_t* v, chr_t* f, int_t l, const chr_t* p)                          \
  {                                                                                           \
  if (!is##rtp(*v)) escapef_p(TYPE_ERR,stdout,f,l,p,"expected %s, got %s",#rtp,typename(*v)); \
  if (check_fptr(*v)) { update_ptr(v); }				                      \
  return to##rtp##_(*v);                                                                      \
  }

#define DESCRIBE_VAL_SAFECAST(ctp,rtp)                                                        \
  ctp safecast_##rtp##_(val_t v, chr_t* f, int_t l, const chr_t* p)                           \
  {                                                                                           \
    if (!is##rtp(v)) escapef_p(TYPE_ERR,stdout,f,l,p,"expected %s, got %s",#rtp,typename(v)); \
    return to##rtp##_(v);                                                                     \
  }

#define DECLARE_PTR_SAFECAST(ctp,rtp) ctp safecast_##rtp##_(val_t*,chr_t*,int_t,const chr_t*)
#define DECLARE_VAL_SAFECAST(ctp,rtp) ctp safecast_##rtp##_(val_t,chr_t*,int_t,const chr_t*)

DECLARE_VAL_SAFECAST(int_t,int);
DECLARE_PTR_SAFECAST(cons_t*,cons);
DECLARE_PTR_SAFECAST(sym_t*,sym);
DECLARE_PTR_SAFECAST(table_t*,table);
DECLARE_PTR_SAFECAST(proc_t*,proc);
DECLARE_PTR_SAFECAST(cdata_t*,port);
DECLARE_PTR_SAFECAST(type_t*,type);
DECLARE_PTR_SAFECAST(str_t*,str);

#define FAST_ACCESSOR_MACRO(v,ctype,f)                 \
  (_Generic((v),				       \
	    val_t:(ctype)ptr(v),		       \
	    ctype:v)->f)

/* General object APIs; getting type information, metadata, and low-level object information */
bool check_fptr(val_t);
val_t get_fptr(val_t);
val_t update_ptr(val_t*);
val_t trace_fptr(val_t);
uint_t typecode(val_t);
type_t* type_of(val_t);
chr_t* typename(val_t);
int_t vm_size(val_t);


// fast type checks for builtin types
#define isucp(v)  (typecode(v) == TYPECODE_UCP)
#define istype(v) (typecode(v) == TYPECODE_TYPE)
#define issym(v)  (typecode(v) == TYPECODE_SYM)
#define iscons(v) (typecode(v) == TYPECODE_CONS)
#define istable(v)  (typecode(v) == TYPECODE_TABLE)
#define isstr(v)  (typecode(v) == TYPECODE_STR)
#define isproc(v) (typecode(v) == TYPECODE_PROC)
#define isport(v) (typecode(v) == TYPECODE_PORT)
#define isint(v)  (typecode(v) == TYPECODE_INT)
#define isa(v,t)  (typecode(v) == (t))

// safe casts for builtin types 
#define totype(v)   safecast_type_(&v,__FILE__,__LINE__,__func__)
#define tosym(v)    safecast_sym_(&v,__FILE__,__LINE__,__func__)
#define totable(v)   safecast_table_(&v,__FILE__,__LINE__,__func__)
#define tocons(v)   safecast_cons_(&v,__FILE__,__LINE__,__func__)
#define tostr(v)    safecast_str_(&v,__FILE__,__LINE__,__func__)
#define toint(v)    safecast_int_(v,__FILE__,__LINE__,__func__)
#define toport(v)   safecast_port_(&v,__FILE__,__LINE__,__func__)
#define toproc(v)   safecast_proc_(&v,__FILE__,__LINE__,__func__)
#define toucp(v)    safecast_ucp_(v,__FILE__,__LINE__,__func__)

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
uint_t ncells(val_t);
val_t cons(val_t,val_t);
val_t append_i(val_t*,val_t);
val_t append(val_t*,val_t);

/*  
    the rascal api for conses includes car, cdr, and cons (defined above), as well as
    the following.
 */

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
chr_t* vm_str(chr_t*);
// the functions below are intended to help compare uninterned strings to symbols
int_t cmpv(val_t,val_t);

/* 
   rascal api for symbols and strings
 */

/*
  tables

  tables are intended to be a core part of the language, giving low level access to a powerful.
  They provide the functionality provided in most lists by 'tables', but my intent is for them
  to be a useful and virtual interface along the lines of Python tables, so that's what they're
  named. 

  tables are unbalanced trees whose keys must be atoms (direct data, symbols, or types). In
  the future I plan to implement them as some type of balanced trees whose keys can be
  any type that implements a generic comparison function.

  The global symbol table is implemented as a table, and the global readtable (once implemented
  will also be implemented as a table).

 */

struct table_t {
  OBJECT_HEAD;
  val_t key;
  val_t binding;
  val_t left;
  val_t right;
};

typedef enum table_fl {
  GENERAL_KEYS,
  SYMBOL_KEYS,
} table_fl;


#define key_(d)     FAST_ACCESSOR_MACRO(d,table_t*,key)
#define binding_(d) FAST_ACCESSOR_MACRO(d,table_t*,binding)
#define left_(d)    FAST_ACCESSOR_MACRO(d,table_t*,left)
#define right_(d)   FAST_ACCESSOR_MACRO(d,table_t*,right)
#define keytype_(d) FAST_ACCESSOR_MACRO(d,table_t*,head.flags_0)


/* vm api for tables */
table_t* table();
val_t key(val_t);
val_t binding(val_t);
int_t keytype(val_t);
val_t left(val_t);
val_t right(val_t);


sym_t* intern_builtin(chr_t*,val_t);
val_t* table_searchk(val_t,val_t*);
val_t  table_setk(val_t,val_t,val_t*);
val_t*  table_searchv(val_t,val_t*);

/* 
   rascal api for tables

   The table constructor takes an arbitrary number of arguments and inserts them into
   a table, returning the root of the resulting table.

 */
val_t r_table(val_t);

/* 
   rascal API for structured objects 

   This api involves four functions - assr, assv, setr, rplcv.

   (assr some-table 'key) returns a context-appropriate object corresponding to the first 
   location matching the key, or NIL if the key can't be found.

   For tables, this is a cons of the key/value pair.

   For lists, assr returns the cons whose car is the nth element of the list.

   For strings, assr returns the nth character.

   For vectors, assr returns a pair of ('ok . <value>)

   assv looks up a collection's values.

   For tables, assv is a cons of the first key/value pair whose value is equal to the
   second argument.

   For ordered collections, assv returns the index of the first element equal to v.

   setr and rplcv are setters; on success, they return a copy of the original object;
   on failure, they return an error.

   For tables, setr replaces the value associated with the given key with the value of the
   third argument. If the key is not an element in the tableionary, it is added.

   For lists, setr inserts a new cell at the given index and sets its value to the third
   argument. If the index is out of range, setr raises an error.

   For indexed collections, setr replaces the nth value with the given value. If the index
   is out of range, setr raises an error.

   tables do not implement rplcv.

   For lists and indexed collections, rplcv replaces the first occurence of the second 
   argument with the third argument. If no occurences are found, rplcv returns NONE.

   Using either setr or rplcv on a string returns a new string (since it may not
   be possible to replace the given index or character without resizing the array).

*/


bool  isenvnames(val_t);
val_t new_env(val_t,val_t,val_t);
val_t vm_gete(val_t,val_t);
val_t vm_pute(val_t,val_t);
val_t vm_sete(val_t,val_t,val_t);


/* type objects */

struct type_t {
    OBJECT_HEAD;          // pointer to the type, also holds flags on the data field
    struct {
      val_t base_size : 16;  // the minimum size (in bytes) for objects of this type.
      val_t val_lowtag : 3;  // the lowtag that should be used for objects of this type.
      val_t cmpable : 1;     // implements cmp
      val_t atomic : 1;      // can values be used as a table key?
      val_t callable : 1;    // can values be used as a function?
      val_t free : 43;
    } flags;
  /* the rascal-callable constructor */
    sym_t* tp_name;
};

#define typecode_self_(t)  meta_(t)
#define fl_base_size_(t)    ((t)->flags.base_size)
#define fl_val_lowtag_(t)   ((t)->flags.val_lowtag)
#define fl_cmpable_(t)      ((t)->flags.cmpable)
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
#define vargs_(p)          FAST_ACCESSOR_MACRO(p,proc_t*,head.flags_2)
#define formals_(p)        FAST_ACCESSOR_MACRO(p,proc_t*,formals)
#define env_(p)            FAST_ACCESSOR_MACRO(p,proc_t*,env)
#define body_(p)           FAST_ACCESSOR_MACRO(p,proc_t*,body)


typedef enum proc_fl {
  CALLMODE_FUNC,
  CALLMODE_MACRO,
  BODYTYPE_EXPR=0,
  BODYTYPE_BYTECODE,
  VARGS_FALSE=0,
  VARGS_TRUE,
} proc_fl;

/* VM api for procedures */
val_t new_proc(val_t,val_t,val_t,proc_fl,proc_fl);
bool  check_argco(int_t,val_t,bool);

struct cdata_t {
  OBJECT_HEAD;
  void* data;
};

void* get_data(val_t);

typedef enum port_fl {
  BINARY_PORT,
  TEXT_PORT,
  READ=1,
  WRITE=1,
} port_fl;

#define stream_(p) FAST_ACCESSOR_MACRO(p,cdata_t*,stream)

typedef enum _r_tok_t {
  TOK_LPAR,
  TOK_RPAR,
  TOK_DOT,
  TOK_QUOT,
  TOK_NUM,
  TOK_STR,
  TOK_SYM,
  TOK_UCHR,
  TOK_EOF,
  TOK_NONE,
  TOK_STXERR,
} r_tok_t;

#define TOKBUFF_SIZE 1024
chr_t TOKBUFF[TOKBUFF_SIZE];
chr_t STXERR[TOKBUFF_SIZE];
int_t TOKPTR;
r_tok_t TOKTYPE;

/* reader suport functions */
void builtin_print(val_t,FILE*); // fallback print function
val_t builtin_load(FILE*);
chr_t* toktype_name(r_tok_t);
r_tok_t get_token(FILE*);
val_t read_expr(FILE*);
val_t read_cons(FILE*);

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
#define INIT_HEAPSIZE 8 * 4096 // in bytes
#define INIT_NUMTYPES 256      // number of records
#define RAM_LOAD_FACTOR 0.8    // resize ram if, directly after garbage collection, (FREE - HEAP) > more than RAM_LOAD_FACTOR * (HEAPSIZE)
#define HEAP_ALIGNSIZE 8       // the alignment of the heap

#define CHECK_RESIZE() (FREE - HEAP) > (RAM_LOAD_FACTOR * HEAPSIZE) ? true : false

int_t align_heap(uchr_t**,uchr_t*);
void gc_if(uint_t);
void pre_gc(uint_t,val_t**,uint_t);
void gc(val_t**,uint_t);
void gc_trace(val_t*);
table_t* gc_trace_table(table_t**);
uchr_t* gc_copy(uchr_t*,int_t,int_t);
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
ival_t HEAPSIZE, ALLOCATIONS;
bool GROWHEAP;
uint_t TYPECOUNTER, NUMTYPES;
type_t** TYPES;

/* evaluator and the stack */
// the stack
#define MAXSTACK 4096          // arbitrary
val_t* STACK;
int_t STACKSIZE, SP;

// The core evaluator registers
val_t EXP, VAL, CONTINUE, NAME, ENV, UNEV, ARGL, PROC;
// working registers (never saved, always free)
val_t WRX, WRY, WRZ; 
val_t GLOBALS;
val_t ROOT_ENV;
// special constants
// The lowtags and typecodes are laid out so that NIL, including correct lowtag and typecode,
// is equal to C 'NULL'
// FPTR is a special value used to indicate that this cell has been moved to the new heap
// during garbage collection. 
val_t NIL, NONE, OK, T, FPTR;
val_t R_STDIN, R_STDOUT, R_STDERR, R_PROMPT;


// working with the stack
val_t*  push(val_t);
val_t pop();
val_t peek();

// convenience macros for working with registers
#define save push
#define savel(l) push((l << 32) | INT_LOWTAG)
#define restore(r) r = pop()
#define restorel(r) r = val(pop())
#define failf(c,e,fmt,args...) if (c) do { escapef(e,stderr,fmt,##args); } while (0)
#define fail(e,fmt,args...) do { escpapef(e,stderr,fmt,##args); } while (0)
#define branch(c,l) if (c) goto *labels[l]
#define jump(l) goto *labels[l]
#define dispatch(r) goto *labels[vm_analyze(r)]

// evaluator core
val_t analyze_expr(val_t);
val_t eval_expr(val_t,val_t);
val_t apply_fnc(val_t,val_t);

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
  EV_APPLY_MACRO,
  EV_APPLY_BUILTIN,
  EV_HALT,
};

// opcodes, vm, & vm registers
void fetch(uchr_t*);                 // get the next instruction and update the instruction pointer
void decode(uchr_t*);                // interpret the current instruction, load any arguments, and update the instruction pointer
void getargs(uchr_t*,uint_t,uint_t); // load any arguments to the current instruction into the appropriate registers
void execute();                      // execute the current instruction

uchr_t INSTR;     // the current instruction
uchr_t ARGA[8];   // the first argument to the current instruction
uchr_t ARGB[8];   // the second argument to the current instruction
uchr_t ARGC[8];   // the third argument to the current instruction
val_t RESULT;     // holds immediate results (usually to be pushed onto the stack)
uchr_t* IP;       // the instruction pointer

enum {
  OP_HALT,   // signals the end of execution
  OP_POP,    // remove an item from the stack and leave it in a register (takes one argument, the register to leave the result in)
  OP_PEEK,   // load an item from the stack into a register without updating the stack pointer (takes two arguments, the offset from the top of the stack and the register to leave the result in)
  OP_PUSHC,  // push a constant value onto the stack (takes one argument, the value to be pushed)
  OP_PUSHRX, // push the value of a register onto the stack (takes one argument, an ID for the register)
  OP_PUSHS,  // push a reference to a string constant onto the stack (interprets the next byte in the instruction sequence as the head of a string constant)
  OP_LOADV,  // lookup the value of a variable (takes one argument, a variable reference)
  OP_PUTV,   // extend the current environment with a new variable name (takes one argument, a reference to the variable name)
  OP_STOREV, // store a value in the given environment location (takes two arguments, a variable reference and the value to assign it)
  OP_CCALL,  // call a C function (takes three arguments, an ID for the C function, the number of arguments to expect on the stack, and a code for the expected return type)
  OP_APPLY,  // apply the closure on top of the stack
  OP_RETURN, // return from an application
  OP_BRANCH, // conditional branch
  OP_JUMP,   // unconditional branch
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
  EVERR_APPLICATION,
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

#define escapef_p(e,file,cfile,line,cfnc,fmt,args...)			             \
  do                                                                                 \
    {                                                                                \
     fprintf(file,"%s: %d: %s: %s ERROR: ",cfile,line,cfnc,get_errname(e));          \
     fprintf(file,fmt,##args);                                                       \
     fprintf(file, ".\n");                                                           \
     longjmp(SAFETY,e);							             \
    } while (0)

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
chr_t* get_errname(int_t);
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

val_t vm_int(int_t);
#define NUM_BUILTINS 35

/* initialization */
/* bootstrapping builtin functions */
void _new_builtin_function(const chr_t*, int_t, bool, const void*);  
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

/* 
   C call API 

   Since creating new builtins rapidly becomes unmanagable, but there's also a need to call 
   many C functions (character testing functions, for example), the following C call API will
   be implemented to allow access to those functions. 

*/


// The intent is that pointer, ppointer, etc, can be combined with other types to describe
// array and pointer types

typedef enum C_types {
  CHR     =0,
  UCHR    =1,
  UINT8   =2,
  INT8    =3,
  UINT16  =4,
  INT16   =5,
  WCHAR   =6,
  WINT    =7,
  UINT32  =8,
  INT32   =9,
  UINT64  =10,
  INT64   =11,
  FLOAT32 =12,
  FLOAT64 =13,
  CFILE   =14,
  VALUE   =0,
  POINTER =16,
  PPOINTER=32,
} C_types;

// lookup C function based on a numeric code
void* get_cfunc(int_t);
// dispatch on a C_types code and perform an appropriate safecast on a lisp value,
// leaving the result in an array of unsigned characters
void cast_argument(val_t,uint8_t,void**);
// call the C callable and interpret the result as a rascal value
val_t c_call(void*,val_t*,int_t,uint8_t*,int_t);

void** C_FUNCTIONS;
uint8_t** TYPESPECS;

/*
  The functions that need to be implemented:

  constructors:
  
  cons
  table
  ucp
  sym
  int
  str
  proc
  type
  cdata

  accessors (for simplicity implement a general getf and setf):
  getf
  setf

  type accessor information:
  type_of

  a general comparison function:

  cmp

  helpers for interning strings and looking up data in tables and lists:

  intern_string
  table_lookup
  table_setkey
  new_env
  get_env
  put_env
  set_env

  arithmetic assistants (simple functions that supply access to basic arithmetic operations,
  with the rest implemented in time):
  
  add
  sub
  mul
  div
  rem
  lt
  eql

  reader:
  get_token
  load_file
  read_expr
  read_list

  evaluator:

  eval_expr
  apply_fnc

  ccall api:
  lookup_function
  cast_arg
  call_cfnc

*/

/* end rascal.h */
#endif
