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
typedef struct cons_t cons_t;
typedef struct sym_t sym_t;
typedef struct fvec_t fvec_t;
typedef struct node_t node_t;
typedef struct type_t type_t;
typedef struct obj_t obj_t;
typedef struct proc_t proc_t;
typedef struct dvec_t dvec_t;
typedef struct table_t table_t;

// type signature for builtin C functions - the first argument is an array of arguments and the second is the size of the array
typedef val_t (*rsp_cfunc_t)(val_t*,int);

// this struct holds vm state for handling exceptions
typedef struct _rsp_ectx_t rsp_ectx_t;

// value tags for determining type - the current system is forward compatible with planned extensions
typedef enum {
  LTAG_LIST             =0x00,  // a list pointer (may be nil)
  LTAG_CFILE            =0x01,  // a C file pointer
  LTAG_STR              =0x03,  // a C string
  LTAG_CONS             =0x04,  // a non-list cons
  LTAG_SYM              =0x05,  // a pointer to a symbol
  LTAG_NODE             =0x06,  // an AVL node (low level type used to implement other types)
  LTAG_FVEC             =0x07,  // a fixed-length vector or bitmapped vector
  LTAG_DVEC             =0x08,  // a dynamic vector
  LTAG_TABLE            =0x09,  // a table object
  /* the following types have an explicit type pointer, with optional flags stored in the low bits prefix */
  LTAG_FUNCTION         =0x0a,  // a closure object
  LTAG_META             =0x0b,  // a metaobject (implicitly a type)
  LTAG_OBJECT           =0x0c,  // a common object
  LTAG_BIGNUM           =0x0d,  // a heap-allocated numeric type (not implemented)
  LTAG_DIRECT           =0x0f,  // special ltag indicating a wide direct tag
  LTAG_INT              =0x1f,  
  LTAG_CHAR             =0x2f,
  LTAG_BOOL             =0x3f,
  LTAG_BUILTIN          =0x4f,  // a builtin C function; the numeric value is it index in the builtin functions array, and it stores
  LTAG_NONE             =0xff,  // flags in the 2nd byte and its argument count in bytes 3-4
} ltag_t;

#define MAX_CORE_OBJECT_TYPES 16
#define MAX_CORE_DIRECT_TYPES 16
#define NUM_BUILTIN_FUNCTIONS 256
extern type_t* CORE_OBJECT_TYPES[MAX_CORE_OBJECT_TYPES];
extern type_t* CORE_DIRECT_TYPES[MAX_CORE_DIRECT_TYPES];
extern rsp_cfunc_t BUILTIN_FUNCTIONS[NUM_BUILTIN_FUNCTIONS];
extern char* BUILTIN_FUNCTION_NAMES[NUM_BUILTIN_FUNCTIONS];

// these flags represent a mix of metadata, disambiguating tags, and object flags; these flags can always be found in
// the 4 least-significant bits of the first element of the object
typedef enum {
  // symbol flags
  SYMFLAG_INTERNED   =0b0001ul,
  SYMFLAG_KEYWORD    =0b0010ul,
  SYMFLAG_RESERVED   =0b0100ul,
  SYMFLAG_GENSYM     =0b1000ul,

  // procedure flags
  PROCFLAG_MACRO     =0b0001ul,  // this closure should be executed as a macro
  PROCFLAG_VARGS     =0b0010ul,  // this closure accepts variable arguments
  PROCFLAGS_VARKW    =0b0100ul,  // this closure accepts variable keyword arguments
  PROCFLAGS_GENERIC  =0b1000ul,  // this closure has multiple implementations

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

// macros for describng object heads
#define OBJECT_HEAD	              \
  val_t type

// union type to represent C version of all builtin direct types and different object heads
typedef union {
  int integer;
  float float32;
  wchar_t unicode;
  bool boolean;
  unsigned bits;
} rsp_direct_ctypes_t;

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
} c_ptr_t;

/* opcodes */

typedef enum {
  OP_HALT,
  OP_LOAD_CONSTANT,
} opcode_t;

/* builtin reader tokens */

typedef enum {
  TOK_LPAR,
  TOK_RPAR,
  TOK_DOT,
  TOK_QUOT,
  TOK_NUM,
  TOK_STR,
  TOK_SYM,
  TOK_SLASH,
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
  VALUE_ERR,   // unexpected value
  BOUNDS_ERR,  // bad array access
  ARITY_ERR,   // wrong argument count
} rsp_err_t;

const char* ERROR_NAMES[] = { "type", "null", "none", "unbound", "value", "bounds", "arity" };

/*

  Future usage of 4-bit lowtag space (types ending with 'OBJ' have explicit type prefixes):

  LTAG_LIST      = 0x00,
  LTAG_CONS      = 0x01,
  LTAG_STR       = 0x02,  // a utf-8 string
  LTAG_CVALUE    = 0x03,  // a tagged C value or pointer
  LTAG_CFUNC     = 0x04,  // a C function pointer
  LTAG_SYM       = 0x05,  
  LTAG_BIGNUM    = 0x06,  // an annotated numeric type 64-bits or greater

  // untagged objects either used by the VM or included to be used as efficient building blocks for user types
  LTAG_BV32      = 0x07,  // a bitmapped vector of up to 32 elements, with 27 to 59 bits available for flags
  LTAG_BV64      = 0x08,  // a bitmapped vector of up to 64 elements, with 61 bits available for flags
  LTAG_CARRAY    = 0x09,  // a general representation of a C array with type and sizing information
  LTAG_CODE      = 0x0a,  // a compiled bytecode object

  // these object types comprise core user-extensible types and include explicit type pointers in their object heads
  LTAG_RECORDOBJ = 0x0b,  // a common object with type-defined fields
  LTAG_FUNCOBJ   = 0x0c,  // a common function object
  LTAG_METAOBJ   = 0x0d,  // a meta-object (considered a function, but the memory layout is different)

  // direct data with an 8-bit ltag
  LTAG_INT       = 0x0f,  // a 56-bit signed integer (common integer type)
  LTAG_BOOL      = 0x1f,  // boolean type
  LTAG_CHAR      = 0x2f,  // a utf-32 character (a unicode codepoint)
  LTAG_FLOAT     = 0x3f,  // a 32-bit iee754 float
  LTAG_IMAG      = 0x4f,  // a 32-bit imaginary float
  LTAG_CMPLX     = 0x5f,  // a 32-bit complex number

  // lower level integer types
  LTAG_INT32     = 0x6f,  // a 32-bit signed integer
  LTAG_UINT32    = 0x7f,  // a 32-bit unsigned integer
  LTAG_INT16     = 0x8f,  // a 16-bit signed integer
  LTAG_UINT16    = 0x9f,  // a 16-bit unsigned integer
  LTAG_INT8      = 0xaf,  // an 8-bit signed integer
  LTAG_UINT8     = 0xbf,  // an 8-bit unsigned integer

  // arbitrary 32-bit C data with additional type information stored in the next byte
  LTAG_C32       = 0xcf,  // 32-bits of arbitrary C data with an explicit tag
  LTAG_NONE      = 0xff,  // a special ltag for NONE

  Under this system the BV32, BV64, and CARRAY types are intended as workhorse data structures that can be used to efficiently and
  programatically implement a variety of user-defined data structures. My goal is to be able to infer an appropriate bitvector
  implementation from a well-formed type definition. The resulting type will then be a record object with the desired metadata
  and a pointer to the beginning of the linked structure.

  proposed control flow primitives (based on delimited continuations; t is a prompt tag; if not supplied, defaults to :toplevel)

      * (cntl & t)     - install a capture/escape point marked with the tags t

      * (cc &t)        - capture a continuation up to the nearest enclosing tag t
      
      * (go &t &val)   - jump to the nearest matching capture point with tag t, supplying val as the result of the procedure at
                         the save point.

      * (hndl &t)      - preempt discharges of the matching tag t

  These primitives aren't intended to be used in everyday code, but to provide a highly extensible basis for efficiently
  implementing other control flow operators. The standard library should provide:

     * try/catch/throw/raise (continuable throw)
     * coro/yield
     * async/await
     * with (io context-handler/concurrency manager)

     an example of try/catch/raise implemented using these operators:

     (defmac try   [& exprs]     `(cntl :exception ~@exprs (hndl :exception (fn [e &msg] (raise e &msg)))))
     (defmac catch [e & exprs]   `(hndl ~e ~@exprs))
     (defmac raise [e &msg]      `(cc k (go ~e &msg)))
     

 */

/* end common.h */
#endif
