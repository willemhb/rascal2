#ifndef globals_h

/* begin globals.h */
#define globals_h
#include "common.h"
#include "rtypes.h"

const val_t   HIGHMASK      = ((val_t)UINT32_MAX) << 32;
const val_t   LOWMASK       = UINT32_MAX;
const uint8_t DVAL_SHIFT    = 32;
const val_t   SHIFTED_1     = LOWMASK + 1;
const val_t   SHIFTED_2     = LOWMASK + 2;
const val_t   LTAG_MASK     = 0x07ul;
const val_t   PTR_MASK      = ~LTAG_MASK;
const val_t   DTYPE_MASK    = 0xfffffff8ul;
const val_t   SHIFTED_WEOF  = ((int64_t)WEOF) << 32;
const val_t   SHIFTED_NAN   = (0x7fc00000ul << 32);
const val_t   SHIFTED_MNAN  = (0xffc00000ul << 32);
const val_t   SHIFTED_INF   = (0x7f800000ul << 32);
const val_t   SHIFTED_MINF  = (0xffc00000ul << 32);

// global array of type object pointers, indexable using type key
extern type_t** GLOBAL_TYPES;
// these counters ensure that types 
uint32_t OTYPE_COUNTER = 0x1au;
uint32_t DTYPE_COUNTER = 0x40u;

const val_t R_GLOBAL_VALUES[16] =  {
  0,
  SYMBOL,
  OBJECT,
  SHIFTED_1      | BOOL | DIRECT,
  BOOL           | DIRECT,
  INTEGER        | DIRECT,
  SHIFTED_1      | INTEGER | DIRECT,
  SHIFTED_2      | INTEGER | DIRECT,
  SHIFTED_WEOF   | CHAR    | DIRECT,
};

// other constants (initialized at startup)
extern val_t R_GLOBAL_CONSTANTS[16];

// macros for accessing the value and constant tables
#define R_NIL       R_GLOBAL_VALUES[0]
#define R_UNBOUND   R_GLOBAL_VALUES[1]
#define R_FPTR      R_GLOBAL_VALUES[2]
#define R_TRUE      R_GLOBAL_VALUES[3]
#define R_FALSE     R_GLOBAL_VALUES[4]
#define R_ZERO      R_GLOBAL_VALUES[5]
#define R_ONE       R_GLOBAL_VALUES[6]
#define R_TWO       R_GLOBAL_VALUES[7]
#define R_EOF       R_GLOBAL_VALUES[10]
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

const chr_t* SPECIAL_CONSTANTS[] = { "nil", "none", "t", "f", "ok", "unbound", };
extern const chr_t* BUILTIN_TYPE_NAMES[16];

// main memory
extern uchr_t *RAM, *FREE, *EXTRA;
extern val_t HEAPSIZE, STACKSIZE, DUMPSIZE, HEAPCRITICAL;
const  float RAM_LOAD_FACTOR = 0.8;
extern bool GROWHEAP, GREWHEAP;

// stack, registers, and top-level namespace
extern val_t *STACK, *DUMP, DP;
extern val_t MAIN;
const size_t NUM_REG = 4;
const size_t SP_MIN  = NUM_REG;

// aliases for main registers (they live in reserved space at the base of the stack)
#define REGISTERS       STACK
#define SP              STACK[0]
#define TOS             STACK[SP]

// flags for bitmapping the main registers (EXP and VAL are never saved)

/* reader state */
extern chr_t TOKBUFF[];
extern int32_t TOKPTR;
extern int32_t LASTCHR;
extern int32_t TOKTYPE;

/* error handling */
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
