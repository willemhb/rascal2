#ifndef globals_h

/* begin globals.h */
#define globals_h
#include "common.h"
#include "rtypes.h"

/* 

   this module includes declarations for important global variables, typedefs for enums mapping those variables, convenience macros accessing those variables, and a few
   other enums used generally.

 */

const val_t   HIGHMASK      = ((val_t)UINT32_MAX) << 32;
const val_t   LOWMASK       = UINT32_MAX;
const uint8_t DVAL_SHIFT    = 32;
const val_t   SHIFTED_1     = LOWMASK + 1;
const val_t   SHIFTED_2     = LOWMASK + 2;
const val_t   SHIFTED_WEOF  = ((int64_t)WEOF) << 32;
const val_t   SHIFTED_NAN   = (0x7FC00000ul << 32);
const val_t   SHIFTED_MNAN  = (0xFFC00000ul << 32);
const val_t   SHIFTED_INF   = (0x7F800000ul << 32);
const val_t   SHIFTED_MINF  = (0xFFC00000ul << 32);
const val_t   LTAG_MASK     = 0x07ul;
const val_t   PTR_MASK      = ~LTAG_MASK;

// global array of type objects, indexable using type key
extern type_t** GLOBAL_TYPES;
extern uint32_t TP_COUNTER;

const val_t R_GLOBAL_VALUES[16] =  {
  0,
  NONE,
  OBJ,
  SHIFTED_1 | BOOL,
  BOOL,
  INT,
  SHIFTED_1      | INT,
  SHIFTED_2      | INT,
  SHIFTED_WEOF   | CHAR,
  SHIFTED_NAN    | FLOAT,
  SHIFTED_MNAN   | FLOAT,
  SHIFTED_INF    | FLOAT,
  SHIFTED_MINF   | FLOAT,
};

// other constants (initialized at startup)
extern val_t R_GLOBAL_CONSTANTS[16];

// macros for accessing the value and constant tables
#define R_NIL       R_GLOBAL_VALUES[0]
#define R_UNBOUND   R_GLOBAL_VALUES[1]
#define R_FPTR      R_GLOBAL_VALUES[2]
#define R_TRUE      R_GLOBAL_VALUES[3]
#define R_FALSE     R_GLOBAL_VALUES[4]
#define R_ZERO      R_GLOBAL_VALUES[7]
#define R_ONE       R_GLOBAL_VALUES[8]
#define R_TWO       R_GLOBAL_VALUES[9]
#define R_EOF       R_GLOBAL_VALUES[10]
#define R_NAN       R_GLOBAL_VALUES[11]
#define R_INF       R_GLOBAL_VALUES[12]
#define R_STDIN     R_GLOBAL_CONSTANTS[0]
#define R_STDOUT    R_GLOBAL_CONSTANTS[1]
#define R_STDERR    R_GLOBAL_CONSTANTS[2]
#define R_MAIN      R_GLOBAL_CONSTANTS[3]
#define R_SYMTAB    R_GLOBAL_CONSTANTS[4]
#define R_READTAB   R_GLOBAL_CONSTANTS[5]
#define F_SETV      R_GLOBAL_CONSTANTS[6]
#define F_DEF       R_GLOBAL_CONSTANTS[7]
#define F_QUOTE     R_GLOBAL_CONSTANTS[8]
#define F_IF        R_GLOBAL_CONSTANTS[9]
#define F_FUN       R_GLOBAL_CONSTANTS[10]
#define F_MACRO     R_GLOBAL_CONSTANTS[11]
#define F_DO        R_GLOBAL_CONSTANTS[12]
#define F_LET       R_GLOBAL_CONSTANTS[13]

// the strings used to create forms, special variables, and special symbols
const chr_t* SPECIAL_FORMS[] = { "setv", "def", "quote", "if", "fun", "macro", "do", "let" };

const chr_t* SPECIAL_CONSTANTS[] = { "nil", "none", "t", "f", "ok", "unbound", "nan", "inf" };

const chr_t* BUILTIN_TYPE_NAMES[16] = 
{
  "nil", "bool", "char", "int", "float"
  "atom", "sym", "str", "bstr", "tuple",
  "btuple", "dict", "pair", "list", "iostrm",
  "type",
};

// main memory
 // RAM is the main HEAP; FREE is the address of the next free cons cell; toffset is used to reallocate a block before GC if the reallocation would trigger a collection
extern unsigned char *RAM, *FREE, *EXTRA;
extern val_t HEAPSIZE, STACKSIZE, DUMPSIZE;
const float RAM_LOAD_FACTOR = 0.6;
extern bool GROWHEAP, GREWHEAP;

// the main registers are typed for correctness and faster execution speed
extern val_t *STACK;
extern val_t REGISTERS[16];

// aliases for main registers
#define VAL             REGISTERS[0]
#define ENVT            REGISTERS[1]
#define CODE            REGISTERS[2]
#define CONT            REGISTERS[3]
#define SB              REGISTERS[4]       // stack base
#define SP              REGISTERS[5]       // stack top
#define PC              REGISTERS[6]       // program counter
#define OP              REGISTERS[7]       // current instruction
#define ARG(x)          REGISTERS[8 + (x)] // operands for bytecode instructons
#define TMP(x)          REGISTERS[12 + (x)] // auxilliary registers

// flags for bitmapping the main registers
typedef enum {
  RX_VALUE    =0x0001,
  RX_ENVT     =0x0002,
  RX_CONT     =0x0004,
  RX_FUNC     =0x0008,
  RX_BP       =0x0010,
  RX_SP       =0x0020,
  RX_PC       =0x0040,
  RX_OP       =0x0080,
  RX_ARG_0    =0x0100,
  RX_ARG_1    =0x0200,
  RX_ARG_2    =0x0400,
  RX_ARG_3    =0x0800,
  WRX_0       =0x1000,
  WRX_1       =0x2000,
  WRX_2       =0x4000,
  WRX_3       =0x8000,
} rx_map_t;


/* reader state */
extern chr_t TOKBUFF[];
extern int32_t TOKPTR;
extern int32_t LASTCHR;
extern int32_t TOKTYPE;

/* error handling */
extern rsp_ectx_t* exc_stack;

// error codes (can be used to index the error names array)
typedef enum
{
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
  IO_ERR,
} rsp_err_t;

extern rsp_err_t rsp_errcode; // the current error code

const chr_t* ERROR_NAMES[] =
{
  "type", "null", "none", "unbound",
  "envt", "value", "bounds", "arity",
  "syntax", "name", "io",
};


#define ERRINFO_FMT     "[%s:%i:%s] %s error: "
#define ERR_FMT         "%s"
#define TYPE_ERR_FMT    "expected type %s, got %s"
#define NULL_ERR_FMT    "unexpected null pointer"
#define NONE_ERR_FMT    "unexpected none value"
#define UNBOUND_ERR_FMT "unbound symbol %s"
#define VALUE_ERR_FMT   ERR_FMT
#define BOUNDS_ERR_FMT  ERR_FMT
#define ARITY_ERR_FMT   "expected %i args, got %i"
#define SYNTAX_ERR_FMT  ERR_FMT
#define NAME_ERR_FMT    "attempt to re-bind reserved name %s"
#define IO_ERR_FMT      ERR_FMT


const chr_t* ERROR_FMTS[] =
{
  TYPE_ERR_FMT, NULL_ERR_FMT, NONE_ERR_FMT, UNBOUND_ERR_FMT,
  VALUE_ERR_FMT, BOUNDS_ERR_FMT, ARITY_ERR_FMT, SYNTAX_ERR_FMT,
  NAME_ERR_FMT, IO_ERR_FMT,
};

/* end globals.h */
#endif
