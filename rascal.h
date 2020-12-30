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
#include "lib/numutil.h"
#include "lib/ioutil.h"



typedef uintptr_t rsp_val_t;
typedef struct type_t type_t;


/* 
   values, object heads, and tags

   All rascal values are represented by pointer-sized unsized integers that can be interpreted
   as half-word sized C data (stored in the high half of the word), C pointers (special cases),
   or pointers to objects in the lisp heap (most object data). All data accessible through
   rascal must be accessed through tagged values -- either wrapped direct data, a tagged C
   pointer, or a tagged pointer to an object head. object fields that don't fall into
   one of these categories cannot be returned directly as rascal values. The word-sized, tagged,
   rascal-legal value will be canonically referred to as the "value representation". The
   corresponding heap object (if any) will be referred to as the "object representation".

   Type information is stored in type_t structs, which record information about the type,
   including how to interpret the value representation, the memory layout of the object repres-
   entation, the constructor (if any), &c.

   The type of the value is encoded using a CL-inspired tagging system involving a 3- or 4-bit common
   tag (ltag), a 4-bit direct tag for direct data (dtag), and a 4-bit object tag for common
   heap objects. The full tag is 8-bits wide and is composed of (least-to-most significant bits)
   the narrow component and either the data component (if any) or the object component (if any).
   The resulting wide tag (wtag) identifies either a builtin type with a known location or
   the location of a pointer to the type object.

   The lisp heap is a contiguous array of word-sized, double-word aligned cells (the minimum
   allocation size is two machine words to support forwarding pointers). All objects in the
   lisp heap are cons cells, unicode strings, or objects. Cons cells and strings have special
   narrow tags to identify them, but general objects require a 4-bit object tag to distinguish
   them. These 4 bits are stored in the 4 least significant bits of the object head.

   There are two general categories of object head; object heads with explicit pointers to
   the type, and object heads that store flags and metadata. In general, The former are used
   for derived types while the latter are used for builtin types (whose locations are known).

   As such, all legal Rascal values must begin with a word-sized block that reserves its 4
   least-significant bits for the object tag.

   Below are enums defining the complete set of tags, as well as macros defining the various 
   types of object heads, as well as type definitions for casting to different kinds of
   objects. The object tag can always be accessed by casting to an obj_t object.

 */

enum {

  /* 3-bit lowtags */
  
  LTAG_NIL              =0b0000ul,  // a special lowtag for NIL
  LTAG_STR              =0b0010ul,  // a utf-8 C string
  LTAG_BSTR             =0b0100ul,  // a binary byte string (the first 8 bytes contains the length)
  LTAG_CFUNCTION        =0b0110ul,  // a C function (for now assume this is a builtin C function)

  /* 4-bit low tags */
  
  LTAG_CONS             =0b0001ul,  // a non-list cons cell
  LTAG_LIST             =0b0011ul,  // a list-structured chain of cons cells
  LTAG_SYM              =0b0101ul,  // a pointer to a symbol

  /* lowtags for header tagged objects */
  
  LTAG_TABLE            =0b0111ul,  // a pointer to a table
  LTAG_ARRAY            =0b1001ul,  // a pointer to an array
  LTAG_PROCEDURE        =0b1011ul,  // a pointer to a procedure
  LTAG_OBJ              =0b1101ul,  // other object type
  LTAG_DIRECT           =0b1111ul,  // direct data (read the next 4-bits)

  /* direct tags for direct data */
  
  DTAG_CHAR             =0b00001111ul, // a utf-32 character
  DTAG_INT              =0b00011111ul, // a signed integer
  DTAG_BOOL             =0b00101111ul, // a boolean
  DTAG_FLOAT            =0b00111111ul, // a 32-bit floating point number
  DTAG_NONE             =0b11111111ul, // a special data tag for NONE

  /* table otags */
  OTAG_TABLE_SET        =0b0000ul,   // a set
  OTAG_TABLE_DICT       =0b0001ul,   // a dictionary
  OTAG_TABLE_SYM        =0b0010ul,   // a symbol table
  OTAG_TABLE_READ       =0b0011ul,   // a specialized symbol table
 
  /* 
     procedure otags

     at least one additional procedure otag is planned for generic functions
   */

  OTAG_PROCEDURE_COMPILED =0b0001ul,   // a bytecode object

  /* 
     array otags 

     once arrays are fully implemented, the array otags will be composable, using
     the first four tags below as bit flags. Some of these are mutually exclusive,
     and the invalid tags are repurposed to denote VM frames. All VM frames are
     either dynamic untyped vectors or static untyped vectors, so only the first flag
     is meaningful. The scheme will be:

     0b1000 = dynamic array
     0b0100 = multidimensional array
     0b0010 = unboxed array
     0b0001 = typed array

     dynamic and multidimensional are mutually exclusive, as are unboxed and typed,
     so tags of the forms:
     
     0b11xx
     0bxx11

     are repurposed to represent frames used by the virtual machine.

     The only array types currently implemented are dynamic vectors of tagged rascal
     values and fixed vectors of tagged rascal values. These arrays are compatible with
     the scheme above.

   */

    OTAG_ARRAY_DVEC           =0b1000ul,  // a simple dynamic array
    OTAG_ARRAY_FVEC           =0b0000ul,  // a simple fixed vector
    OTAG_FRAME_ENV            =0b1011ul,  // an environment frame
    OTAG_FRAME_CONT           =0b0011ul,  // a continuation frame
    OTAG_FRAME_LAMBDA         =0b1100ul,  // an uncompiled lambda or let with a captured closure
    OTAG_FRAME_MACRO          =0b1101ul,  // an uncompiled macro expression with a captured closure

  /* object otags 

     future planned otags:

     OTAG_OBJ_RECORD  // a struct-like type
     OTAG_OBJ_SUBTYPE // subtype of a builtin type
     OTAG_OBJ_CDATA   // tagged C data
     OTAG_OBJ_CPTR,   // indirected C pointer

   */

  OTAG_OBJ_TYPE          =0b0000ul,  // a type object
  
};

// tagging API
#define ltag_width(v)  (((v) & 0x1) ? 4 : 3)
#define get_tag(v,t)   ((v) & (t))
#define mask_tag(v,t)  ((v) & (~(t)))
#define get_val(v)     ((v) >> 32)

rsp_val_t get_addr(rsp_val_t);    // mask out the correct bits to extract the address
int read_tag(rsp_val_t);          // get an 8-bit full tag from a value
int get_tags(type_t*);            // get all of the tags needed to tag a value of this type
int tag_value(rsp_val_t,int,int); // given the raw bits of a rascal value and two tags, apply the appropriate tags and return
                                  // a correct tagged value



/* special values NIL and NONE */

const rsp_val_t NIL = 0x0ul;
const rsp_val_t NONE = UINT_MAX;

/* 
   special sentinels - UNBOUND signals an interned symbol where a binding has not been provided, while FPTR signals a value that has been moved during garbage collection
   (the new location can be found by checking the next word-sized block). Both are equivalent to tagging the NULL pointer with a specific tag.

 */

const rsp_val_t UNBOUND = LTAG_SYM;                     // this is the same as tagging the null pointer with LTAG_SYM
const rsp_val_t FPTR = LTAG_OBJ;                        // this is the same as tagging the null pointer with LTAG_OBJ

/* booleans */

const rsp_val_t R_TRUE = (UINT32_MAX + 1) | 0b0011000ul;
const rsp_val_t R_FALSE = 0b001100ul;

/* common integer values */

const rsp_val_t R_ZERO = 0b00100000ul; 
const rsp_val_t R_ONE = (UINT32_MAX + 1) | 0b00100000ul;

/* important file descriptors */
const rsp_val_t R_STDIN_FNO = R_ZERO;
const rsp_val_t R_STDOUT_FNO = R_ONE;
const rsp_val_t R_STDERR_FNO = (UINT32_MAX + 2) | 0b01100000ul;

// object head macros and generic object types

/* cons - a classic! */
typedef struct {
  rsp_val_t car;
  rsp_val_t cdr;
} cons_t;


#define OBJECT_HEAD          \
  rsp_val_t head

typedef struct {
  OBJECT_HEAD;
} obj_t;


typedef struct {
  rsp_val_t allocated : 60;
  rsp_val_t otag      :  4;
  rsp_val_t elcnt;
  rsp_val_t elements;        // pointer to the elements, in case the vector needs to grow
} dvec_t;

typedef struct {
  rsp_val_t allocated : 60;
  rsp_val_t otag      :  4;
  rsp_val_t elements[1];     // elements stored directly; cannot add more elements than allocatd
} fvec_t;


#define PROC_OBJ_HEAD                  \
  rsp_val_t pargs           : 28;      \
  rsp_val_t kwargs          : 28;      \
  rsp_val_t method          :  1;      \
  rsp_val_t vargs           :  1;      \
  rsp_val_t varkw           :  1;      \
  rsp_val_t macro           :  1;      \
  rsp_val_t otag            :  4


/*

  the common fields are:

  formals : a dict mapping names to offsets
  

 */

#define PROC_COMMON_FIELDS             \
  rsp_val_t formals;                   \
  rsp_val_t closure


/* signature for valid builtin C functions - the first argument is a stack of rascal values and the second is the number of arguments the function was called with */
typedef rsp_val_t (*rsp_builtin_t)(rsp_val_t*,int);

typedef struct {
  PROC_OBJ_HEAD;
  PROC_COMMON_FIELDS;
} proc_t;

typedef struct {
  PROC_OBJ_HEAD;
  PROC_COMMON_FIELDS;
  rsp_val_t code;     // a pointer to the bytecode sequence
  rsp_val_t vals[1];  // an array of values used in the procedure body
} bytecode_t;

/* 
   symbols, dicts, and sets

   symbols are interned strings, as in traditional lisp. Tables are implemented as balanced binary trees that remember their
   insertion order. Reader tables are similar to symbol tables, with different conventions for their keys and bindings.

   the fields in the TABLE_OBJ_HEAD have the following meanings:

   ord_key        : the interpretation of this field depends on the value of is_root.

   is_root        : a boolean flag, true if this table has no parent. If true, ord_key is the number of keys in the table. otherwise, ord_key is the insertion order of the current node (
                    this allows both tables and sets to remember the insertion order of the keys without additional overhead).

   is_constant    : indicates whether or not the binding can be changed.

   flags          : dependent on the table type. If it's a symbol table, the flags hold information about the symbol. If it's
                    a reader table, the flags hold information about the token type.

   balance_factor : used to implement AVL tree balancing algorithm

   otag           : reserved for the otag.
 */

#define TABLE_OBJ_HEAD               \
  rsp_val_t ord_key         : 52;    \
  rsp_val_t is_root         :  1;    \
  rsp_val_t constant        :  1;    \
  rsp_val_t flags	    :  3;    \
  rsp_val_t balance_factor  :  3;    \
  rsp_val_t otag            :  4

enum {
  TOKTYPE_CHR,
  TOKTYPE_STR,
  TOKTYPE_CHRSET,
  TOKTYPE_DLM_START,
  TOKTYPE_DLM_END,
};

// if the symbol is uninterned, these flags are stored on the symbol directly
enum {
  SYMFLAG_GENSYM   =0b100,
  SYMFLAG_RESERVED =0b010,
  SYMFLAG_KEYWORD  =0b001,
};


typedef struct table_t table_t;

#define TABLE_COMMON_FIELDS           \
  rsp_val_t left;                     \
  rsp_val_t right;                    \
  rsp_val_t hashkey


typedef struct {
  rsp_val_t record;   // this symbol's entry in a symbol table
  char name[1];       // the name hangs off the end
} sym_t;


struct table_t {
  TABLE_OBJ_HEAD;
  TABLE_COMMON_FIELDS;
};

typedef struct {
  TABLE_OBJ_HEAD;
  TABLE_COMMON_FIELDS;
} set_t;

typedef struct {
  TABLE_OBJ_HEAD;
  TABLE_COMMON_FIELDS;
  rsp_val_t binding;
} dict_t;


rsp_val_t   get_key(table_t*);
rsp_val_t*  search_key(table_t*,rsp_val_t);
rsp_val_t   lookup_key(table_t*,rsp_val_t);
rsp_val_t   put_key(table_t*,rsp_val_t);
rsp_val_t   set_key(table_t*,rsp_val_t,rsp_val_t);

rsp_val_t intern_string(dict_t*,const char*);

// lookup a name in 
rsp_val_t lookup_name(rsp_val_t,rsp_val_t);


/* 

   C api and C descriptions of rascal values 

   The current goal is to implement a C api that covers rascal values which can be represented as numeric types or strings.

   Ideally, the C api should be able to represent arbitrary C data, but only a small subset is necessary to create a basic
   interface to eg system calls.

 */

typedef enum {
  C_UCHR8   =0b0000,
  C_CHR8    =0b0010,
  C_SCHR8   =0b0011,
  C_UINT8   =0b0100,
  C_INT8    =0b0101,
  C_UINT16  =0b0110,
  C_INT16   =0b0111,
  C_UINT32  =0b1000,
  C_INT32   =0b1001,
  C_FLOAT32 =0b1010,
  C_UINT64  =0b1100,
  C_INT64   =0b1101,
  C_FLOAT64 =0b1110,
} c_num_t;

typedef enum {
  C_PTR_NONE =0b00,   // value is the type indicated by c_num_t
  C_PTR_VOID =0b01,   // value is a pointer of unknown type
  C_PTR_TO   =0b10,   // value is a pointer to the indicated type
  C_FPTR     =0b11,   // special case - value is a C file pointer
} c_ptr_t;

/* 
   core C api function - takes a rascal value, and an encoded request
   

 */
int unwrap(rsp_val_t,void*,c_num_t,c_ptr_t);

/* type objects */

struct type_t {

  /* flags encoding size information */
  rsp_val_t val_size_base   : 46;      // the minimum size for objects of this type (minimum 8 for direct data and 16 for heap data)
  rsp_val_t val_size_static :  1;      // whether all values of this type have the same size
  rsp_val_t val_size_fixed  :  1;      // whether the size of a value of this type can change

  /* flags encoding the size and C representation for values of this type */
  
  rsp_val_t val_cnum        :  4;      // the numeric type of the value representation as C data
  rsp_val_t val_cptr        :  2;      // the pointer type ofthe value representation of the C data
  rsp_val_t val_direct      :  1;      // whether this is direct data or a pointer
  rsp_val_t val_rheap       :  1;      // whether values live in the rascal heap

  /* flags holding the appropriate tags for values of this type */
  
  rsp_val_t val_otag        :  4;
  rsp_val_t val_dtag        :  4;
  rsp_val_t val_ltag        :  4;
  
  /* misc other data about values */
  
  rsp_val_t tp_atomic         :  1;     // legal for use in a plain dict
  rsp_val_t tp_callable       :  1;     // legal as first element in an application
  rsp_val_t tp_enumerated     :  1;
  rsp_val_t tp_singleton      :  1;     

  /* reserved for the otag */
  rsp_val_t otag            :  4;

  /* layout information */

  rsp_val_t tp_name;                    // a pointer to the symbol this type is bound to
  rsp_val_t tp_fields;                  // a dict mapping rascal-accessible fields to offsets
  rsp_val_t tp_get_index;               // called to get a value at a specified index
  rsp_val_t tp_set_index;               // called to assign a value at a specified index

  /* function pointers */
  rsp_val_t tp_new;                            // the function dispatched when the type is called
  rsp_val_t tp_cmp;                            // used to compare two values of the same type
  rsp_val_t tp_allocate;                       // used by the memory manager to allocate new values
  rsp_val_t tp_init;                           // used by the memory manager to initialize an allocated object
  rsp_val_t tp_move;                           // used by the garbage collector to relocate values of this type
  rsp_val_t tp_print;                          // used to print values
  rsp_val_t tp_print_traverse;                 // used to print recursive structures
};

/* variable declarations for builtin types */

extern type_t* ANY_TYPE_OBJ, NIL_TYPE_OBJ, NONE_TYPE_OBJ, INT_TYPE_OBJ,
  FLOAT_TYPE_OBJ, CHAR_TYPE_OBJ, BOOL_TYPE_OBJ, STR_TYPE_OBJ, BYTVEC_TYPE_OBJ,
  TYPE_TYPE_OBJ, CONS_TYPE_OBJ, LIST_TYPE_OBJ, SYM_TYPE_OBJ;

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
char TOKBUFF[TOKBUFF_SIZE];
char STXERR[TOKBUFF_SIZE];
int TOKPTR;
r_tok_t TOKTYPE;

/* reader suport functions */

void builtin_print(rsp_val_t,FILE*); // fallback print function
rsp_val_t builtin_load(FILE*);
char* toktype_name(r_tok_t);
r_tok_t get_token(FILE*);
rsp_val_t read_expr(FILE*);
rsp_val_t read_cons(FILE*);

/* memory, gc, & allocator */

#define AS_BYTES 8
#define AS_WORDS 1
#define MAXTYPES UINT_MAX >> 3 
#define MAXBASE USHRT_MAX
#define INIT_HEAPSIZE 8 * 4096  // in bytes
#define INIT_NUMTYPES 256       // number of records
#define RAM_LOAD_FACTOR 0.8
#define HEAP_ALIGNSIZE 8        // the alignment of the heap

#define CHECK_RESIZE() (FREE - HEAP) > (RAM_LOAD_FACTOR * HEAPSIZE) ? true : false

int align_heap(unsigned char**,unsigned char*);
void gc();
void gc_trace(rsp_val_t*);
unsigned char* gc_copy(unsigned char*,int,int);
unsigned char* vm_allocate(int);

bool heap_limit(unsigned);
bool type_limit(unsigned);
bool type_overflow(unsigned);
bool stack_overflow(unsigned);
bool type_overflow(unsigned);

/* special registers holding important memory locations and parameters */

unsigned char *HEAP, *EXTRA, *FREE, *FROMSPACE, *TOSPACE;
rsp_val_t HEAPSIZE, ALLOCATIONS;
bool GROWHEAP;

// opcodes, vm, & vm registers
/* 
   register architecture based on 
   https://www.cs.utexas.edu/ftp/garbage/cs345/schintro-v14/schintro_142.html#SEC275

   VALUE - the result of the last expression
   ENVT  - the environment the code is executing in

*/

rsp_val_t VALUE, ENVT, CONT, TEMPLATE, PC;

// the stacks used by the virtual machine
rsp_val_t *EVAL, *SP;

// working registers
rsp_val_t WRX[16];

typedef enum {
  /* these opcodes are common to evaluated and compiled code */
  OP_HALT,
} rsp_opcode_t;

typedef enum {
  EV_HALT,
  EV_LITERAL,
  EV_VARIABLE,
  /* special forms */
  EV_QUOTE,
  EV_BACKQUOTE,
  EV_UNQUOTE,
  EV_SPLICE,
  EV_SETN,
  EV_DEF,
  EV_IF,
  EV_DO,
  EV_MACRO,            // capture the closing environment and create a new macro
  EV_FUN,              // capture the closing environment and create a new function
  EV_LET,

  /* application and argument evaluation */

  EV_APPLY,            // starting point for the evaluation process
  EV_APPLY_OP_DONE,    // set up the frame for the call without updating the frame pointer
  EV_MACRO_ARGLOOP,    // load the arguments into VARS without evaluating them; place the procedure body in EXP, set the return label to EV_MACRO_RETURN, update the FP, and dispatch
  EV_BUILTIN_ARGLOOP,  // 
  EV_BUILTIN_ACCUMARG, // 
  EV_FUNCTION_ARGLOOP, // 
  EV_ACCUMARG,         // accumulate an argument
  EV_ACCUMARG_LAST,    // accumulate the final argument
  EV_RETURN,           
  EV_MACRO_RETURN,     
} rsp_evlabel_t;

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
extern rsp_val_t BUILTIN_FORMS[];

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
   NONE  - functions that return rsp_val_t should return the special rascal value NONE to signal
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
char* get_errname(int_t);
void init_log();                 // initialize the global variable stdlog. Stdlog is 
char* read_log(char*, int_t);  // read the log file into a string buffer.
void dump_log(FILE*);            // dump the contents of the log file into another file.
int_t finalize_log();            // copy the contents of the log to the history


/* initialization */
/* bootstrapping builtin functions */
void _new_builtin_function(const char*, int_t, bool, const void*);  
rsp_val_t  _new_form_name(char*);                 
rsp_val_t  _new_self_evaluating(char*);

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
