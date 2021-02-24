
#ifndef opcodes_h

#define opcodes_h
#include "common.h"

/* this module contains the exec/eval opcodes */

typedef enum
{
  /* 0-argument opcodes */
  OP_NOP                =0x00u, // do nothing
  OP_HALT               =0x01u, // end execution
  OP_PUSH               =0x02u, // Push the contents of VALUE onto EVAL, incrementing SP
  OP_POP                =0x03u, // Pop the top of the stack and leave the result in VALUE
  OP_APPLY              =0x04u, // apply a callable to a list of arguments
  OP_TAPPLY             =0x05u, // tail apply
  OP_LOADN              =0x06u, // lookup the name on TOS in the namespace
  OP_SAVE               =0x07u, // save a continuation
  OP_RESTORE            =0x08u, // restore a continuation

  /* 1-argument opcodes */
  OP_LOADL              =0x09u, // load a local value into VALUE
  OP_SETL               =0x0au, // store VALUE in the indicated local environment frame
  OP_LOADV              =0x0bu, // load a value from the function's VALUE array
  OP_CALL               =0x0cu, // call VALUE with n args (already on stack)
  OP_TCALL              =0x0du, // tail call VALUE with n args
  OP_BRANCH             =0x0eu,
  OP_JUMP               =0x0fu,

  /* 2-argument opcodes */
  OP_LOADC              =0x10u, // load a value from the closure
  OP_SETC               =0x11u, // set a value in an enclosed environment
} opcode_t;

extern const size_t OPCODE_SIZES[];

typedef enum
{
  TOK_LPAR,
  TOK_RPAR,
  TOK_DOT,
  TOK_QUOT,
  TOK_INT,
  TOK_FLOAT,
  TOK_STROPEN,
  TOK_SYM,
  TOK_CHAR,
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
