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
#include "lib/strutil.h"

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
typedef struct lambda_t lambda_t;    // generic procedure
typedef struct proc_t proc_t;        // uncompiled procedure
typedef struct code_t code_t;        // compiled procedure
typedef struct table_t table_t;      // generic table type
typedef table_t symtab_t;            // symbol table
typedef struct methtab_t methtab_t;  // method table
typedef struct vec_t vec_t;
typedef struct cdata_t cdata_t;
typedef FILE iostrm_t;

#define OBJECT_HEAD     \
  val_t head

struct obj_t {
  OBJECT_HEAD;
};


#define PROC_HEAD       \
  val_t bodysize : 41;  \
  val_t argco    : 16;  \
  val_t macro    :  1;  \
  val_t vargs    :  1;  \
  val_t ptype    :  2;  \
  val_t otag     :  3;  \
  val_t env

struct lambda_t {
  PROC_HEAD;
};

typedef enum rsp_proctypes_t {
  PROCTYPE_EXPRESSION,
  PROCTYPE_COMPILED,
} rsp_proctypes_t;

#define TABLE_HEAD        \
  val_t padding  : 55;    \
  val_t constant :  1;    \
  val_t balance  :  3;    \
  val_t ttype    :  2;    \
  val_t otag     :  3;    \
  val_t key;              \
  val_t binding;	  \
  val_t left;		  \
  val_t right

struct table_t {
  TABLE_HEAD;
};

typedef enum rsp_tabletypes_t {
  TABLETYPE_SYMTAB,
  TABLETYPE_METHTAB,
  TABLETYPE_READTAB,
  TABLETYPE_DICT,
} rsp_tabletypes_t;

/* 
   tag definitions 

   tags are composed of 3-to-8-bit tags stored on all legal rascal values, and 4-bit tags stored on all rascal objects. All tags
   can be translated into an 8-bit tag that allows rascal to unambiguously find the type of a rascal value. The tags are read as
   follows:

   If the first three bits are 0, the first 8 bits are returned (gives the tag for most direct data).

   If the first bit is 0, the first three bits are returned.

   If the first bit is 1, the first 4 bits are examined:
   * If the numeric value of the first 4 bits is less than 13, the first 4 bits are returned
   * If the numeric value of the first 4 bits is equal to 13, the first 4-bit object tag is retrieved, padded by 4, and OR'd to the
     first 4 bits
   * Otherwise, 255 is returned (a special tag indicating the unique value NONE)

   The resulting tag allows the value's type_t object to be retrieved.

   This system has the consequence that it allows NIL to be equal to C NULL and NONE to be equal to C -1. 

 */



typedef enum {
  LTAG_DIRECT           =0b0000ul,   // direct data
  LTAG_CPTR_IOSTREAM    =0b0010ul,   // a pointer to a C FILE object
  LTAG_CPTR_BUILTIN     =0b0100ul,   // a pointer to a builtin rascal function
  LTAG_CPTR_OTHER       =0b0110ul,   // a pointer to arbitrary other C data, no C spec given (only allowed in special instances)
  LTAG_LPTR_CONS        =0b0001ul,   // a pointer to a cons
  LTAG_LPTR_LIST        =0b0011ul,   // a pointer to a list
  LTAG_LPTR_CONSOBJ     =0b0101ul,   // a pointer to a cons proxy object
  LTAG_LPTR_SEQ         =0b0111ul,   // a pointer to a sequence (a special case of a cons object).
  LTAG_LPTR_STR         =0b1001ul,   // a pointer to a rascal string
  LTAG_LPTR_VEC         =0b1011ul,   // a pointer to a rascal vector
  LTAG_LPTR_OBJ         =0b1101ul,   // a pointer to an object (read otag).
  OTAG_NONE             =0b0000ul,   // sentinel indicating not to use the otag
  OTAG_TYPE             =0b0001ul,   // a type object
  OTAG_ARRAY            =0b0010ul,   // an n-dimensional array
  OTAG_PROC             =0b0011ul,   // object is laid out like a procedure
  OTAG_ATOM             =0b0100ul,   // object is an atom (hash of a non-string, non-direct object).
  OTAG_TABLE            =0b0101ul,   // object is laid out like a table
  OTAG_SUBTYPE_NOMINAL  =0b0110ul,   // subtype of a builtin type; after the object head, the layout follows its parent type
  OTAG_SUBTYPE_EXTENDED =0b0111ul,   // subtype of a builtin type, plus additional fields
  OTAG_CVALUE           =0b1000ul,   // a lisp value storing C data with a type-defined C spec (used to implement big floats, etc).
  OTAG_RECORD           =0b1001ul,   // an object with a fixed number of type defined fields
  OTAG_CDATA_64         =0b1010ul,   // 64-bits of C data, stored directly, including a 1-byte C spec
  OTAG_CDATA_128        =0b1011ul,   // 128-bits of C data, stored directly, including a 1-byte C spec
  OTAG_CDATA_256        =0b1100ul,   // 256-bits of C data, stored directly, including a 1-byte C spec
  OTAG_CDATA_OBJECT     =0b1011ul,   // arbitrary C data, stored directly, including a a multi-byte C spec
  OTAG_CDATA_PTR        =0b1100ul,   // a pointer to arbitrary C data, including a multi-byte C spec
  WTAG_NIL              =0,
  WTAG_IOSTREAM         =LTAG_CPTR_IOSTREAM,
  WTAG_CPTR_BUILTIN     =LTAG_CPTR_BUILTIN,
  WTAG_CPTR_OTHER       =LTAG_CPTR_OTHER,
  WTAG_CONS             =LTAG_LPTR_CONS,
  WTAG_LIST             =LTAG_LPTR_LIST,
  WTAG_CONSOBJ          =LTAG_LPTR_CONSOBJ,
  WTAG_SEQ              =LTAG_LPTR_SEQ,
  WTAG_STR              =LTAG_LPTR_STR,
  WTAG_VEC              =LTAG_LPTR_VEC,
  WTAG_INT              =0b00010000ul,   // integers
  WTAG_FLOAT            =0b00100000ul,   // floats
  WTAG_IMAG             =0b00110000ul,   // imaginary number
  WTAG_REAL             =0b01000000ul,   // special numeric values like inf and NaN
  WTAG_BOOL             =0b01010000ul,   // booleans
  WTAG_CHAR             =0b01100000ul,   // utf-32 character
  WTAG_CDATA_32         =0b01110000ul,   // C data up to 32 bits with a stored C spec (next byte).
  WTAG_TYPE             =(OTAG_TYPE << 4) || LTAG_LPTR_OBJ,
  WTAG_ARRAY            =(OTAG_ARRAY << 4) || LTAG_LPTR_OBJ,
  WTAG_PROC             =(OTAG_PROC << 4) || LTAG_LPTR_OBJ,
  WTAG_ATOM             =(OTAG_ARRAY << 4) || LTAG_LPTR_OBJ,
  WTAG_NONE             =255,
} tag_t;
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

  CSPEC_FL_DATAPTR        =0b00,    // data pointer
  CSPEC_FL_FPTR           =0b10,    // function pointer
  CSPEC_FL_PTR_BACKREF    =0b01,    // the pointer refers to data defined earlier in the spec
  
  /* struct flags */

  CSPEC_FL_STRUCT_START   =0b00000,   // start of a struct
  CSPEC_FL_UNION_START    =0b00100,   // start of a union
  CSPEC_FL_STRUCT_FIELD   =0b01000,   // start of a regular field
  CSPEC_FL_STRUCT_BFIELD  =0b01100,   // start of a bitfield
  CSPEC_FL_STRUCT_END     =0b10000,
  CSPEC_FL_UNION_END      =0b10100,
  CSPEC_FL_STRUCT_PACKED  =0b00001,
  CSPEC_FL_STRUCT_ALIGNED =0b00010,
  
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
  val_t size;
  val_t elements[1];
};

/* C data */
struct cdata_t {
  val_t cspec_len : 16; // the number of bytes occupied by the spec portion
  val_t obj_len   : 45; // the number of bytes occupied by the data portion
  val_t otag      :  3; // reserved for otag
  /* 
     object representation of the C spec and the C data 
     (C spec is first, followed immediately by the C data).
   */
  uchr_t obj[1];
};

/* table */

struct methtab_t {
  TABLE_HEAD;
  val_t next;       // next method in the chain
};

/* types */

struct type_t {
  val_t cspec_len    : 37;               // ensure alignment
  val_t tp_builtin   :  1;
  val_t tp_fixed     :  1;
  val_t tp_base      : 16;                 // base size for values of this type
  val_t tp_ltag      :  3;                 // appropriate ltag for values of this type
  val_t tp_otag      :  3;                 // appropriate otag for values of this type
  type_t* tp_parent;                       // pointer to the parent type (if any)
  symtab_t* tp_fields;                     // map from rascal-accessible fields to integer offsets within the object
  val_t (*tp_new)(val_t*,int_t);           // constructor for new values
  val_t (*tp_getf)(val_t,val_t);           // field accessor
  val_t (*tp_setf)(val_t,val_t,val_t);     // field setter
  val_t (*tp_getk)(val_t,val_t);           // index accessor
  val_t (*tp_setk)(val_t,val_t,val_t);     // index setter
  void  (*tp_unwrap)(uchr_t*,int_t,void*); // 
  chr_t* tp_name;                      // the name for this type
  uchr_t tp_cspec[1];                  // C-spec for this type (hangs off the end for objects with more than one field)
};

/* procedure */
struct proc_t {
  PROC_HEAD;
  val_t formals;          // the list of formal parameters
  val_t body;             // pointer to a list of expressions
};

/* code object */
struct code_t {
  PROC_HEAD;
  val_t values;           // constants store
  uchr_t code[1];         // instructions
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

void fetch();
void decode();
void execute();
void push(val_t);
void pushrx(rxid_t);
void pop(rxid_t);
void peek(int_t,rxid_t);


enum {

  /* 0 argument opcodes (mostly arithmetic) */

  OP_HALT,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_REM,
  OP_NEQ,
  OP_CEQ,
  OP_LT,
  OP_LEQ,
  OP_GT,
  OP_GEQ,
  OP_RSH,
  OP_LSH,
  OP_BAND,
  OP_BOR,
  OP_BXOR,
  OP_BNEG,
  OP_APPLY,
  OP_RETURN,
  ZEROARG_OPS = OP_RETURN,
  
  /* 1 argument opcodes */
  OP_PUSH,                  // register id
  OP_POP,                   // register id
  OP_PUTE,                  // register id
  OP_LOADV,                 // value offset
  OP_BRANCH_T,              // code offset
  OP_BRANCH_NIL,            // code offset
  OP_JUMP,                  // code offset
  UNARG_OPS = OP_JUMP,    

  /* 2 argument opcodes */
  OP_PEEK,                  // stack offset, register id
  OP_LOADE,                 // frame offset, local offset
  OP_STORE,                 // frame offset, local offset
  BINARG_OPS = OP_LOADE,
  
  /* 3 argument opcodes */

  OP_CCALL,                 // function id, expected return type, number of arguments
  TERNARG_OPS = OP_CCALL,
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
