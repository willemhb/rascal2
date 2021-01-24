#ifndef globals_h

/* begin globals.h */
#define globals_h
#include "common.h"
#include "rtypes.h"

/* 

   this module includes declarations for important global variables, typedefs for enums mapping those variables, convenience macros accessing those variables, and a few
   other enums used generally.

 */

// arrays to hold builtin type objects; these can be indexed with different parts of the vtag or otag
extern type_t*  CORE_DIRECT_TYPES[16];
extern type_t*  CORE_OBJECT_TYPES[16];
extern type_t*  CORE_BIGNUM_TYPES[16];
extern type_t*  CORE_TYPES[3][16];
extern type_t*  TYPE_METAOBJ;

// special constants - common values, singletons, sentinels
const val_t R_GLOBAL_VALUES[16] =  { 0x0ul, VTAG_SYM, VTAG_CONS, (UINT32_MAX + 1) | VTAG_BOOL, VTAG_BOOL, ((val_t)UINT32_MAX<<32) | VTAG_BOOL, (UINT32_MAX + 2) | VTAG_BOOL,
			             VTAG_INT, (UINT32_MAX + 1) | VTAG_INT, (UINT32_MAX + 2) | VTAG_INT, (((int64_t)WEOF) << 32) | VTAG_CHAR };

// other constants (initialized at startup)
extern val_t R_GLOBAL_CONSTANTS[32];

// macros for accessing the value and constant tables
#define R_NIL       R_GLOBAL_VALUES[0]
#define R_UNBOUND   R_GLOBAL_VALUES[1]
#define R_FPTR      R_GLOBAL_VALUES[2]
#define R_TRUE      R_GLOBAL_VALUES[3]
#define R_FALSE     R_GLOBAL_VALUES[4]
#define R_NONE      R_GLOBAL_VALUES[5]
#define R_OK        R_GLOBAL_VALUES[6]
#define R_ZERO      R_GLOBAL_VALUES[7]
#define R_ONE       R_GLOBAL_VALUES[8]
#define R_TWO       R_GLOBAL_VALUES[9]
#define R_EOF       R_GLOBAL_VALUES[10]
#define R_STDIN     R_GLOBAL_CONSTANTS[0]
#define R_STDOUT    R_GLOBAL_CONSTANTS[1]
#define R_STDERR    R_GLOBAL_CONSTANTS[2]
#define R_SCRIPT    R_GLOBAL_CONSTANTS[3]
#define R_ARGV      R_GLOBAL_CONSTANTS[4]
#define R_ARGC      R_GLOBAL_CONSTANTS[5]
#define R_MAIN      R_GLOBAL_CONSTANTS[6]
#define R_SYMTAB    R_GLOBAL_CONSTANTS[7]
#define R_READTAB   R_GLOBAL_CONSTANTS[8]
#define F_SETV      R_GLOBAL_CONSTANTS[9]
#define F_DEF       R_GLOBAL_CONSTANTS[10]
#define F_QUOTE     R_GLOBAL_CONSTANTS[11]
#define F_IF        R_GLOBAL_CONSTANTS[12]
#define F_FUN       R_GLOBAL_CONSTANTS[13]
#define F_MACRO     R_GLOBAL_CONSTANTS[14]
#define F_DO        R_GLOBAL_CONSTANTS[15]
#define F_LET       R_GLOBAL_CONSTANTS[16]


// the strings used to create forms, special variables, and special symbols
const char* SPECIAL_FORMS[] = { "setv", "def", "quote", "if", "fun", "macro", "do", "let" };
const char* SPECIAL_CONSTANTS[] = { "nil", "none", "t", "f", "ok", "unbound" };
const char* SPECIAL_GLOBALS[] = { "&envt", "&in", "&out", "&err", "&name", "&argv", "&argc", "&main", "&symtab", "&readtab"};
const char* SPECIAL_KEYWORDS[] =  { "r",  "w", "a", "r+", "w+", "a+", "alnum", "alpha", "blank", "cntrl", "digit", "graph", "lower", "print", "space",  "upper",   "xdigit", "&"};

// main memory
 // RAM is the main HEAP; FREE is the address of the next free cons cell; toffset is used to reallocate a block before GC if the reallocation would trigger a collection
extern unsigned char *RAM, *FREE, *EXTRA;
extern val_t HEAPSIZE, STACKSIZE, DUMPSIZE;
const float RAM_LOAD_FACTOR = 0.6;
extern bool GROWHEAP;

// main registers and stacks
extern val_t REGISTERS[16];
// typed registers for holding intermediate untagged values
extern uint64_t UINT64RX[4]; extern int64_t  INT64RX[4]; extern flt64_t  FLT64RX[4];
extern uint32_t UINT32RX[4]; extern int32_t  INT32RX[4]; extern flt32_t  FLT32RX[4];
extern val_t *STACK;

// aliases for main registers
#define VAL             REGISTERS[0]
#define ENVT            REGISTERS[1]
#define TMPL            REGISTERS[2]
#define CONT            REGISTERS[3]
#define SB              REGISTERS[4]
#define SP              REGISTERS[5]
#define PC              REGISTERS[6]
#define OP              REGISTERS[7]
#define ARG(x)          REGISTERS[8 + (x)]
#define TMP(x)          REGISTERS[12 + (x)]
#define NUMREG(t,sz,x)  t ## sz ## RX[(x)] 

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


/* vm opcodes

   opcodes are ordered by argument count (vm_decode checks the value of the opcode to 
   determine how many arguments to load). If opcodes manipulate values, the first argument is
   assumed to be VALUE and remaining arguments are on EVAL. The arguments on EVAL should be 
   popped off by the instruction. */

typedef enum
{
  /* 0-argument opcodes */
  OP_HALT               =0x00, // end execution and return VALUE
  OP_PUSH               =0x01, // Push the contents of VALUE onto EVAL, incrementing SP
  OP_POP                =0x02, // Pop the top of the stack and leave the result in VALUE
  OP_SAVE               =0x03, // Save the current VM state into a new frame; store in CONT
  OP_RESTORE            =0x04, // Restore the VM state from CONT
  OP_LOAD_NAME          =0x05, // Lookup the symbol on top of EVAL in the current namespace
  OP_STORE_NAME         =0x06, // assign EVAL[SP] to VALUE in the current namespace
  OP_CALL               =0x07, // call VALUE (most general kind of dispatch; used when a caller's type can't be known at compile time)
  OP_CALL_NEW           =0x08, // call the constructor of VALUE
  OP_CALL_CPRIM         =0x09, // call a builtin C function
  OP_CALL_METHOD        =0x0a, // call a compiled code object
  OP_CALL_GENERIC       =0x0b, // call a generic function
  OP_BINDV              =0x0c, // store VALUE in the next unbound environment location

  /* 1-argument opcodes */

  OP_LOAD_FIELD         =0x0d,  // load the field from VALUE whose offset is given by the first argument
  OP_STORE_FIELD        =0x0e,  // store TOE in the field whose offset is given by the first argument
  OP_LOAD_VALUE         =0x09,  // load a constant from TEMPLATE->values[N+1]
  OP_LOAD_GLOBAL        =0x0a,
  OP_JUMP               =0x0b,
  OP_LOAD_METHOD        =0x0c,  // create a closure from the arguments on top of the stack and leave the result in VALUE
  OP_BRANCH_T           =0x0d, 
  OP_BRANCH_F           =0x0f,  
  OP_BINDN              =0x11,  

  /* 2-argument opcodes */

  OP_LOAD_VAR           =0x13,
  OP_STORE_VAR          =0x17,
} opcode_t;



/* 
   builtin reader tokens and state
*/

extern chr8_t TOKBUFF[];
extern int32_t TOKPTR;
extern int32_t LASTCHR;

typedef enum
{
  TOK_LPAR,
  TOK_RPAR,
  TOK_LBRACE,
  TOK_RBRACE,
  TOK_LBRACKET,
  TOK_RBRACKET,
  TOK_DOT,
  TOK_QUOT,
  TOK_BACKQUOT,
  TOK_UNQUOT,
  TOK_SPLICE,
  TOK_INT,
  TOK_FLOAT,
  TOK_STROPEN,
  TOK_BSTROPEN,
  TOK_SYM,
  TOK_CHAR,
  TOK_UNICODE,
  TOK_EOF,
  TOK_NONE,
  TOK_STXERR,
} rsp_tok_t;

extern rsp_tok_t TOKTYPE;

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

const char* ERROR_NAMES[] =
{
  "type", "null", "none", "unbound",
  "envt", "value", "bounds", "arity",
  "syntax", "name","io",
};


/* end globals.h */
#endif
