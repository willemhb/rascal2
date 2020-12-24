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
typedef wint_t ucpi_t;
typedef uint32_t uint_t;
typedef struct type_t type_t;
typedef struct obj_t obj_t;
typedef struct cons_t cons_t;
typedef struct sym_t sym_t;
typedef struct proc_t proc_t;
typedef struct code_t code_t;
typedef struct table_t table_t;
typedef struct vec_t vec_t;
typedef struct cdata_t cdata_t;
typedef FILE iostrm_t;

#define OBJECT_HEAD     \
  struct {              \
  val_t type;           \
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
  LOWTAG_DIRECT    =0b000ul,   // direct data
  LOWTAG_OP        =0b001ul,   // a special lowtag for VM opcodes
  LOWTAG_CONS      =0b010ul,   // a pointer to a cons
  LOWTAG_STR       =0b011ul,   // a pointer to a string
  LOWTAG_SYM       =0b100ul,   // a pointer to a symbol
  LOWTAG_CODE      =0b101ul,   // a pointer to compiled code
  LOWTAG_IOSTRM    =0b110ul,   // a C FILE object
  LOWTAG_OBJ       =0b111ul,   // a pointer to a rascal object
} ltag_t;

typedef enum otag_t {
  OBJTAG_RECORD     =0b000ul,  // standard object type with a type-defined number of named fields
  OBJTAG_VECTOR     =0b001ul,  // a vector of lisp values
  OBJTAG_TABLE      =0b010ul,  // table
  OBJTAG_PROC       =0b011ul,  // object is a procedure object
  OBJTAG_BUILTIN    =0b100ul,  // subtype of a builtin type; first element is pointer to type and remainder is laid out like parent
  OBJTAG_BUILTINPTR =0b101ul,  // like OBJTAG_BUILTIN, but the only element is a pointer to an object of the parent type
  OBJTAG_CDATA      =0b110ul,  // arbitrary C data (the object head includes necessary information for interpreting the data).
  OBJTAG_TYPE       =0b111ul,  // A type object
} otag_t;


/* 
   C specs for interpreting rascal values as C data.

   C specs are arrays of unsigned characters that encode C type information. They're used to 
   streamline the interface between C and rascal. Calling C functions is done by constructing
   a C spec for the function

   C specs are designed to represent common types compactly, with the ability to represent
   any type in the C standard with some degree of compactness.

   All C specs begin with a head byte, whose 3 most-significant bits indicate a type family.
   
 */

typedef enum cspec_head_t {
  CSPEC_HEAD_VOID           =0b000,  // special type specs -- meaning is context dependent
  CSPEC_HEAD_CHRVAL         =0b001,  // character types
  CSPEC_HEAD_INTVAL         =0b010,  // integer types
  CSPEC_HEAD_FLOATVAL       =0b011,  // floating point types
  CSPEC_HEAD_STRUCT         =0b100,  // this spec begins (or ends) a description of a struct type
  CSPEC_HEAD_LIBRARY        =0b101,  // description of special library types (FILE, jmp_buf, div_t, &c)
  CSPEC_HEAD_PTR            =0b110,  // this spec begins a description of a pointer
  CSPEC_HEAD_ARRAY          =0b111,  // this spec indicates a description of an array (can include size information)
} cspec_head_t;

typedef enum cspec_offset_t {
  CSPEC_OFFSET_HEAD      =5,
  CSPEC_OFFSET_SIGN      =3,
  CSPEC_OFFSET_REAL      =3,
  CSPEC_OFFSET_VALSIZE   =0,
  CSPEC_OFFSET_FIELDTYPE =4,
  CSPEC_OFFSET_PTRTYPE   =4,
  CSPEC_OFFSET_PTRDEPTH  =0,
  CSPEC_OFFSET_ARRAYLEN  =4,
} cspec_offset_t;

typedef enum cspec_fl_t {

  /* sign flags for characters and integers */

  CSPEC_FL_SIGNED         =0b01,
  CSPEC_FL_UNSIGNED       =0b10,
  CSPEC_FL_NOSIGN         =0b11,

  /* imaginary/complex qualifiers for floats */

  CSPEC_FL_REAL           =0b01,
  CSPEC_FL_IMAG           =0b10,
  CSPEC_FL_CMPLX          =0b11,

  /* field types for structs (bit fields or normal fields)*/

  CSPEC_FL_FIELDS         =0b0, 
  CSPEC_FL_BITS           =0b1,

  /* sizes for value types */

  CSPEC_FL_VALSIZE_0      =0b000,  // for special cases?
  CSPEC_FL_VALSIZE_8      =0b001,
  CSPEC_FL_VALSIZE_16     =0b010,
  CSPEC_FL_VALSIZE_32     =0b011,
  CSPEC_FL_VALSIZE_64     =0b100,
  CSPEC_FL_VALSIZE_128    =0b101,
  CSPEC_FL_VALSIZE_256    =0b110,
  CSPEC_FL_VALSIZE_512    =0b111,

  /* pointer flags */

  CSPEC_FL_DATAPTR        =0b0,    // data pointer
  CSPEC_FL_FPTR           =0b1,   // function pointer

  /* array flags */

  CSPEC_FL_ARRAY_FIXED    =0b0,   // an array with definite length
  CSPEC_FL_ARRAY_FLEXIBLE =0b1,   // an array of variable length

  /* builtin flags */

  CSPEC_FL_LIB_FILE       =0b00000,  // FILE
  CSPEC_FL_LIB_TM         =0b00001,  // tm
  CSPEC_FL_LIB_DIVT       =0b00010,  // div_t
  CSPEC_FL_LIB_LDIVT      =0b00011,  // ldiv_t
  CSPEC_FL_LIB_LLDIVT     =0b00100,  // lldiv_t
  CSPEC_FL_LIB_JMPBUF     =0b00101,  // jmp_buf
  CSPEC_FL_LIB_TIMESPEC   =0b00110,  // timespec

} cspec_fl_t;

// C specs for C builtin and library types
extern const uchr_t* BUILTIN_CSPECS[];

uchr_t* make_cspec(cspec_head_t*,cspec_fl_t*,cspec_offset_t*,int_t);

/* builtin object types */

/* cons */
struct cons_t {
  val_t car;
  val_t cdr;
};

/* symbol */

struct sym_t {
  val_t hash;
  chr_t name[1];
};

/* vector */
struct vec_t {
  struct {
    val_t length : 61;
    val_t otag   :  3;
  } head;
  val_t elements[1];
};

/* C data */
struct cdata_t {
  struct {
    val_t cspec_len : 16; // the number of bytes occupied by the spec portion
    val_t obj_len   : 45; // the number of bytes occupied by the data portion
    val_t otag      :  3; // reserved for otag
  } head;
  /* 
     object representation of the C spec and the C data 
     (C spec is first, followed immediately by the C data).
   */
  uchr_t obj[1];
};

/* table */

struct table_t {
  struct {
    val_t padding      : 53;  // ensure alignment
    val_t method_table :  1;  // method table (key is a type, binding is 2-element array)
    val_t reader_table :  1;  // reader table (key is a char, binding is a reading procedure)
    val_t symbol_table :  1;  // symbol table (key is a symbol)
    val_t constant     :  1;
    val_t global       :  1;  // flags a global table (eg GLOBALS)
    val_t balance      :  3;  // balance factor
    val_t otag         :  3;  // reserved for otag
  } head;
  val_t key;
  val_t left;
  val_t right;
  val_t binding[1];
};

/* types */

struct type_t {
  struct {
    val_t padding      : 22;               // ensure alignment
    val_t tp_cspec_len : 16;
    val_t tp_base      : 16;               // base size for values of this type
    val_t tp_fixed     :  1;               // indicates whether values can exceed the base size
    val_t tp_ltag      :  3;               // appropriate ltag for values of this type
    val_t tp_otag      :  3;               // appropriate otag for values of this type
    val_t otag         :  3;               // reserved for otag
  } head;
  type_t* tp_parent;                   // pointer to the parent type (if any)
  table_t* tp_fields;                  // map from rascal-accessible fields to integer offsets within the object
  val_t (*tp_new)();                   // constructor for new values of this type
  chr_t* tp_name;                      // the name for this type
  uchr_t tp_cspec[1];                  // C-spec for this type (hangs off the end for objects with more than one field)
};

/* procedure */
struct proc_t {
  struct {
    val_t padding  : 42;  // length of the body in bytes (only used for compiled procedures)
    val_t compiled :  1;  // boolean flag for compiled code
    val_t macro    :  1;  // boolean flag for macros
    val_t vargs    :  1;  // bolean flag for whether the procedure accepts vargs
    val_t argco    : 16;  // minimum argcount
    val_t otag     :  3;  // reserved for the otag
  } head;
  val_t env;              // the environment where the procedure was defined
  val_t formals;          // the list of formal parameters
  val_t body;             // pointer to the procedure body (list or code object)
};

/* code object */
struct code_t {
  val_t codesize;        // the size of the instruction sequence (in bytes)
  uchr_t code[1];        // the instruction sequence
};

/* tokenizer */

typedef enum _r_tok_t {
  TOK_LPAR,
  TOK_RPAR,
  TOK_DOT,
  TOK_QUOT,
  TOK_NUM,
  TOK_STR,
  TOK_SYM,
  TOK_UCP,
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

/* memory, gc, & allocator */

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

// special constants & registers
val_t NIL, NONE, OK, T, FPTR, R_STDIN, R_STDOUT, R_STDERR, GLOBALS;

// opcodes, vm, & vm registers
#define STACKSIZE 8192               // arbitrary
val_t* STACK, *SP;
val_t ENV, CONTINUE, VAL, CODE, IP, INSTR, WRX, WRY, WRZ;

// register ids
typedef enum rxid_t {
  RXID_ENV,
  RXID_CONTINUE,
  RXID_VAL,
  RXID_CODE,
  RXID_IP,
  RXID_INSTR,
  RXID_WRX,
  RXID_WRY,
  RXID_WRZ,
} rxid_t;

// C operations (mostly arithmetic, comparison, and bitwise operators; used to simplify)
typedef enum cop_t {
  COP_ADD,
  COP_SUB,
  COP_MUL,
  COP_DIV,
  COP_REM,
  COP_EQ,
  COP_NEQ,
  COP_LT,
  COP_GT,
  COP_LE,
  COP_GE,
  COP_LSH,
  COP_RSH,
  COP_BAND,
  COP_BOR,
  COP_XOR,
  COP_BNEG,
} cop_t;

void fetch();
void decode();
void execute();
void push(val_t);
void pushrx(rxid_t);
void pop(rxid_t);
void peek(int_t,rxid_t);


enum {

  /* 0 argument opcodes */

  OP_HALT,
  OP_APPLY,
  OP_RETURN,
  ZEROARG_OPS = OP_RETURN,
  
  /* 1 argument opcodes */

  OP_PUSH,                  // register id
  OP_POP,                   // register id
  OP_TBRANCH,               // code offset
  OP_NBRANCH,               // code offset
  OP_JUMP,                  // code offset
  UNARG_OPS = OP_JUMP,    

  /* 2 argument opcodes */
  OP_COP,                   // op id, argcount
  OP_PEEK,                  // stack offset, register id
  OP_LOADV,                 // frame offset, local offset
  BINARG_OPS = OP_LOADV,
  
  /* 3 argument opcodes */

  OP_CCALL,                 // function id, expected return type, number of arguments
  TERNARG_OPS = OP_CCALL,

  /* special opcodes */
  OP_LOADC,                 // interpret the next 8 bytes as constant direct data
  OP_LOADS,                 // interpret the next word-aligned set of bytes as a string literal and load a reference to it
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

/* end rascal.h */
#endif
