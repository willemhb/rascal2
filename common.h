#ifndef common_h

/* begin common.h */
#define common_h

/* 
   this file includes common headers, core typedefs, declarations for global constants, and all the enums/flags used in the VM.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>

// unicode constants
#define MIN_CODEPOINT 0
#define MAX_CODEPOINT 0x10FFFF
#define UEOF WEOF

enum {
  /* masks for testing the first byte in a UTF-8 sequence */
  U8_BMASK_1  = 0b10000000,
  U8_BMASK_2  = 0b11100000,
  U8_BMASK_3  = 0b11110000,
  U8_BMASK_4  = 0b11111000,
  /* bytemarks for testing the length of a UTF-8 sequence */
  U8_BMARK_1  = 0b00000000,
  U8_BMARK_2  = 0b11000000,
  U8_BMARK_3  = 0b11100000,
  U8_BMARK_4  = 0b11110000,
  /* limits for 1, 2, 3, and 4-byte code points (the value is 1-greater than the maximum value) */
  UCP_LIMIT_1 = 0x00000080,
  UCP_LIMIT_2 = 0x00000800,
  UCP_LIMIT_3 = 0x00010000,
  UCP_LIMIT_4 = 0x00110000,
};

typedef uintptr_t uptr_t;
typedef uintptr_t val_t;   // a rascal value; all functions that expect rascal values or pointers to rascal values should use this type
typedef uint64_t hash64_t; // a 64-bit hash

// C representation of different object types
typedef struct list_t list_t;
typedef struct cons_t cons_t;
typedef struct sym_t sym_t;
typedef struct node_t node_t;
typedef struct fvec_t fvec_t;
typedef struct dvec_t dvec_t;
typedef struct code_t code_t;
typedef struct obj_t obj_t;
typedef struct table_t table_t;
typedef struct function_t function_t;
typedef struct type_t type_t;

// type signature for builtin C functions that take their arguments from the stack - the VM checks the argument count
// before passing it to the function, so that's not needed
typedef val_t (*rsp_cfunc_t)(val_t*);

// this struct holds vm state for handling exceptions
typedef struct _rsp_ectx_t rsp_ectx_t;

// value tags for determining type - the current system is forward compatible with planned extensions
typedef enum {
  LTAG_LIST             =0x00,  // a list pointer (may be nil)
  LTAG_CONS             =0x01,  // a non-list cons
  LTAG_STR              =0x02,  // a C string
  LTAG_CVALUE           =0x03,  // a tagged C value (not implemented)
  LTAG_CFILE            =0x04,  // a C file pointer
  LTAG_SYM              =0x05,  // a pointer to a symbol

  // special objects used by the VM or as building blocks for user types
  LTAG_NODE             =0x06,  // an AVL node (low level type used to implement other types)
  LTAG_FVEC             =0x07,  // a fixed-length vector
  LTAG_DVEC             =0x08,  // a dynamic vector
  LTAG_CODE             =0x09,  // a compiled bytecode object

  /* types with explicit (possibly flagged) type pointers */

  LTAG_OBJECT           =0x0a,  // a common data object
  LTAG_TABLEOBJ         =0x0b,  // a table object
  LTAG_FUNCOBJ          =0x0c,  // a common function object with a captured closure
  LTAG_METAOBJ          =0x0d,  // a metaobject (implicitly a type)
  LTAG_BIGNUM           =0x0f,  // a heap-allocated numeric type (not implemented)
  LTAG_INT              =0x1f,
  LTAG_FLOAT            =0x2f,
  LTAG_CHAR             =0x3f,
  LTAG_BOOL             =0x4f,
  LTAG_BUILTIN          =0x5f,  // a builtin C function; the numeric value is an array index
  LTAG_NONE             =0xff,  // flags in the 2nd byte and its argument count in bytes 3-4
} ltag_t;

#define MAX_CORE_OBJECT_TYPES 16
#define MAX_CORE_DIRECT_TYPES 16
#define NUM_BUILTIN_FUNCTIONS 59
extern type_t* CORE_OBJECT_TYPES[MAX_CORE_OBJECT_TYPES];
extern type_t* CORE_DIRECT_TYPES[MAX_CORE_DIRECT_TYPES];
// these arrays store the callables and metadata for builtin functions
extern rsp_cfunc_t BUILTIN_FUNCTIONS[NUM_BUILTIN_FUNCTIONS];
extern int         BUILTIN_FUNCTION_ARGCOS[NUM_BUILTIN_FUNCTIONS];
extern bool        BUILTIN_FUNCTION_VARGS[NUM_BUILTIN_FUNCTIONS];
extern char*       BUILTIN_FUNCTION_NAMES[NUM_BUILTIN_FUNCTIONS];

// these flags represent a mix of metadata and special vm flags; they can always be found in
// the 4 least-significant bits of the first element of the object
typedef enum {
  // symbol flags
  SYMFLAG_INTERNED   =0b0001ul,
  SYMFLAG_KEYWORD    =0b0010ul,
  SYMFLAG_RESERVED   =0b0100ul,
  SYMFLAG_GENSYM     =0b1000ul,

  // procedure flags
  FUNCFLAG_MACRO    =0b0001ul,  // this closure should be executed as a macro
  FUNCFLAG_VARGS    =0b0010ul,  // this closure accepts variable arguments
  FUNCFLAG_VARKW    =0b0100ul,  // this closure accepts variable keyword arguments
  FUNCFLAG_METHOD   =0b1000ul,  // this closure is one of multiple implementations

  // dvec flags
  DVECFLAG_ENVTFRAME =0b0001ul,

  // table flags
  TABLEFLAG_SYMTAB   =0b0001ul,
  TABLEFLAG_ENVT     =0b0010ul,
  TABLEFLAG_NMSPC    =0b0011ul,
  TABLEFLAG_READTAB  =0b0100ul,

  // sentinel flag - 8-bits wide, can't clash with other flags
  FLAG_NONE          =LTAG_NONE,
} flags_t;

// flags for indicating different registers; used eg when constructing continuation frames
typedef enum {
  RX_VALUE    =0x0001,
  RX_ENVT     =0x0002,
  RX_CONT     =0x0004,
  RX_TEMPLATE =0x0008,
  RX_EP       =0x0010,
  RX_SP       =0x0020,
  RX_PC       =0x0040,
  WRX_0       =0x0080,
  WRX_1       =0x0100,
  WRX_2       =0x0200,
  WRX_3       =0x0400,
  WRX_4       =0x0800,
  WRX_5       =0x1000,
  WRX_6       =0x2000,
  WRX_7       =0x4000,
} rx_map_t;

typedef enum {
  LBL_LITERAL,
  LBL_SYMBOL,
  LBL_QUOTE,
  LBL_DEF,
  LBL_SETV,
  LBL_FUN,
  LBL_MACRO,
  LBL_LET,
  LBL_DO,
  LBL_IF,
  LBL_FUNAPP,
} rsp_label_t;

// macros for describng object heads
#define OBJECT_HEAD	              \
  val_t type

// union type to represent different direct types, plus a struct for accessing the parts
// of an iee754 float
typedef union {
  int integer;
  float float32;
  wchar_t unicode;
  bool boolean;
  struct {
    unsigned fl_sign     :  1;
    unsigned fl_expt     :  8;
    unsigned fl_mant     : 23;
  } flbits;
  unsigned bits32;
} rsp_c32_t;

// union type to represent and manipulate different direct types without padding/unpadding
typedef union {
  struct {
    rsp_c32_t data;
    unsigned padding : 24;
    unsigned lowtag  :  8;
  } direct;
  uptr_t value;
} rsp_c64_t;

// 
/* 
   C api and C descriptions of rascal values

   The numeric types are laid out such that the common can be found using bitwise or

 */

typedef enum {
  C_UINT8   =0b000010,
  C_INT8    =0b000001,
  C_UINT16  =0b000110,
  C_INT16   =0b000011,
  C_UINT32  =0b001110,
  C_INT32   =0b000111,
  C_INT64   =0b001111,
  C_FLOAT32 =0b100111,
  C_FLOAT64 =0b101111,
} c_num_t;

typedef enum {
  C_PTR_NONE     =0b000,   // value is the type indicated by c_num_t
  C_PTR_VOID     =0b001,   // value is a pointer of unknown type
  C_PTR_TO       =0b010,   // value is a pointer to the indicated type
  C_FILE_PTR     =0b011,   // special case - value is a C file pointer
  C_BLTN_PTR     =0b100,   // special case - value is a C function pointer to a builtin function
  C_STR_PTR      =0b101,   // special case - value is a C string pointer
} c_ptr_t;

/* opcodes */

typedef enum {
  OP_HALT,
  
  /* LOAD instructions leave their arguments in VALUE */

  OP_LOAD_RX,              // put the value of a register into VALUE
  OP_LOAD_VALUES,          // an index in the values store
  OP_LOAD_NAME,            // lookup a a namespace binding
  OP_LOAD_VAR,             // a variable reference (offset . local)

  /* 

     STORE instructions take a reference (either opcode args or contents of VALUE)
     and put the value on top of EVAL in the associated location.

   */

  OP_STORE_NAME,           // assign the value of a namespace name
  OP_STORE_VAR,            // assign the value of a lexical variable
  OP_BIND,                 // bind the top N values from EVAL into corresponding ENVT locations
  OP_BRANCH,               // unconditional branch to a label
  OP_BRANCH_T,             // jump to a label if VALUE is non-false
  OP_BRANCH_F,             // jump to a label if VALUE is false
  OP_PUSH,                 // takes one argument, a register ID
  OP_POP,                  // takes one argument, a register ID
  OP_SAVE,                 // save a continuation
  OP_RESTORE,              // restore the indicated saved continuation
  OP_APPLY,                // apply the procedure in VALUE with the values on EVAL
  OP_APPLY_BUILTIN,        // fast application of a builtin procedure (no ENVIRONMENT set up)
} opcode_t;

/* builtin reader tokens */

typedef enum {
  TOK_LPAR,
  TOK_RPAR,
  TOK_DOT,
  TOK_QUOT,
  TOK_INT,
  TOK_FLOAT,
  TOK_STR,
  TOK_SYM,
  TOK_CHAR,
  TOK_UNICODE,
  TOK_EOF,
  TOK_NONE,
  TOK_STXERR,
} rsp_tok_t;

// error handling
typedef enum {
  TYPE_ERR,    // invalid type
  NULL_ERR,    // a null pointer was encountered
  NONE_ERR,    // unexpected none
  UNBOUND_ERR, // unbound symbol
  ENVT_ERR,    // an invalid argument to an environment API function
  VALUE_ERR,   // unexpected value
  BOUNDS_ERR,  // bad array access
  ARITY_ERR,   // wrong argument count
  SYNTAX_ERR,  // bad syntax
  NAME_ERR,    // attempt to rebind a constant
} rsp_err_t;

const char* ERROR_NAMES[] = {
  "type", "null", "none", "unbound",
  "envt", "value", "bounds", "arity",
  "syntax", "name",
};

/*

  Future usage of 4-bit lowtag space (types ending with 'OBJ' have explicit type prefixes):

  LTAG_LIST      = 0x00,
  LTAG_CONS      = 0x01,
  LTAG_STR       = 0x02,  // a utf-8 string
  LTAG_CVALUE    = 0x03,  // a tagged C value or pointer
  LTAG_CFUNC     = 0x04,  // a C function pointer
  LTAG_SYM       = 0x05,  // a symbol

  // untagged objects either used by the VM or as efficient building blocks for user types
  LTAG_BV32      = 0x06,  // a bitmapped vector of up to 32 element and 27 to 59 bits for flags
  LTAG_BV64      = 0x07,  // a bitmapped vector of up to 64 elements, and 61 bits for flags
  LTAG_ARRAY     = 0x08,  // a generalized array with type and size information
  LTAG_CODE      = 0x09,  // a compiled bytecode object

  // these types have explicit type pointer
  LTAG_TABLEOBJ  = 0x0a,  // a hash-table object
  LTAG_RECORDOBJ = 0x0b,  // a common object with type-defined fields
  LTAG_FUNCOBJ   = 0x0c,  // a common function object
  LTAG_METAOBJ   = 0x0d,  // a meta-object (considered a function, but with different layout)
  LTAG_BIGNUM    = 0x0f,  // a large numeric datatype stored in the heap

  // direct data with an 8-bit ltag
  LTAG_INT       = 0x1f,  // a 56-bit signed integer (common integer type)
  LTAG_FLOAT     = 0x2f,  // a 32-bit iee754 float
  LTAG_IMAG      = 0x3f,  // a 32-bit imaginary float
  LTAG_CMPLX     = 0x4f,  // a 32-bit complex number

  // lower level integer types
  LTAG_INT32     = 0x5f,  // a 32-bit signed integer
  LTAG_UINT32    = 0x6f,  // a 32-bit unsigned integer
  LTAG_INT16     = 0x7f,  // a 16-bit signed integer
  LTAG_UINT16    = 0x8f,  // a 16-bit unsigned integer
  LTAG_INT8      = 0x9f,  // an 8-bit signed integer
  LTAG_UINT8     = 0xaf,  // an 8-bit unsigned integer

  // other direct types
  LTAG_BOOL      = 0xbf,  // boolean type
  LTAG_CHAR      = 0xcf,  // a utf-32 character (a unicode codepoint)
  LTAG_BUILTIN   = 0xdf,  // a builtin function stored in a special array
  LTAG_NONE      = 0xff,  // a special ltag for NONE

  Under this system the BV32, BV64, and CARRAY types are intended as workhorse data structures 
  that can be used to efficiently and programatically implement a variety of user-defined data
  structures. My goal is to be able to infer an appropriate bitvector implementation from a 
  well-formed type definition. The resulting type will then be a record object with the desired
  metadata and a pointer to the beginning of the linked structure.

  proposed control flow primitives (based on delimited continuations; t is a prompt tag; 
  if not supplied, defaults to :toplevel)

      * (label & t)     - install a capture/escape point marked with the tags t

      * (cc &t)         - capture a continuation up to the nearest enclosing tag t
      
      * (go k val)      - call the continuation associated with k, supplying val as
                          the result of the computation; k can be a continuation or a
                          any type that's a valid argument to label

      * (hndl t h)      - preempt calls to go with a matching tag, executing h with
                          the arguments supplied to go

  These primitives aren't intended to be used in everyday code, but to provide a highly 
  extensible basis for efficiently implementing other control flow operators. The standard
  library should provide:

     * try/catch/throw/raise (continuable throw)
     * coro/yield
     * async/await
     * with (installs io handlers; by default, ensures that streams are cleaned up on
       exit, but can be called with additional handlers to deal with synchronization
       and network IO)

  proposed extension primitives:

  "type extension primitives" are really just metaobjects, which are considered functions
  and therefore callable. The proposed metatypes in this system are:

     * type - concrete datatype metatype; calling should return a new type object describing
       the storage type, fields, and field constraints specified by the arguments.

     * class - a class metaboject, aka an interface or typeclass. The arguments are a name
       and a set of function signatures, and the return value is a new class object describing
       the set of functions member types must implement. Class objects can be called with
       a type object and a set of function objects, creating an implementation of that class
       for the type and adding the type to the class's set of members.

   proposed module system:

   The module system should be as simple as possible while maintaining clean namespaces. The
   current proposal is a system built around a single special form. As in Python, the code
   in a file is treated as a module without needing to be specified as such (a module special
   form should probably be provided anyway).

      * (import fname-or-optlist ...) - load each file specified in the list of arguments
        and bind it to a namespace in the current environment. By default each binding in
	a module 'fname.rsp' is exported as <fname>/<binding>, but these can be changed by
	supplying a list of options whose first element is the filename.

 */

/* end common.h */
#endif
