
#ifndef opcodes_h

#define opcodes_h
#include "common.h"

/* this module contains vm opcodes, compiler labels, and reader tokens */

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
  OP_CALL               =0x07, // call VALUE
  OP_CALL_NEW           =0x08, // call the constructor of VALUE
  OP_CALL_CPRIM         =0x09, // call a builtin C function
  OP_CALL_METHOD        =0x0a, // call a compiled code object
  OP_CALL_GENERIC       =0x0b, // call a generic function
  OP_BINDV              =0x0c, // store VALUE in the next unbound environment location

  /* 1-argument opcodes */

  OP_LOAD_VALUE         =0x0f,
  OP_LOAD_GLOBAL        =0x10,
  OP_JUMP               =0x11,
  OP_LOAD_METHOD        =0x12, 
  OP_BRANCH_T           =0x13, 
  OP_BRANCH_F           =0x14,  
  OP_BINDN              =0x15,  

  /* 2-argument opcodes */

  OP_LOAD_VAR           =0x16,
  OP_STORE_VAR          =0x17,
} opcode_t;

extern const size_t OPCODE_SIZES[];

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

typedef enum
  {
    LBL_LITERAL,
    LBL_SYMBOL,
    LBL_SETV,
    LBL_DEF,
    LBL_QUOTE,
    LBL_IF,
    LBL_FUN,
    LBL_MACRO,
    LBL_DO,
    LBL_LET,
    LBL_FUNAPP,
  } flabel_t;

/* end opcodes.h */
#endif
