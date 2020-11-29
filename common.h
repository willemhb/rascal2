#ifndef common_h

/* begin common.h */
#define common_h
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>

#include "lib/strlib.h"

/* core type  and variable definitions go in this module */
/* typedefs for core builtins */
typedef uintptr_t rval_t;
typedef unsigned char uchr_t;
typedef char chr_t;
typedef uchr_t rbvec_t;
typedef int32_t rint_t;
typedef struct _rtype_t rtype_t;
typedef struct _robj_t robj_t;
typedef struct _rcons_t rcons_t;
typedef struct _rsym_t rsym_t;
typedef struct _rproc_t rproc_t;
typedef struct _rprim_t rprim_t;
typedef struct _rstr_t rstr_t;
typedef struct _rport_t rport_t;


#define OBJECT_HEAD     \
  struct {              \
    uint64_t meta : 32; \
    uint64_t type : 29; \
    uint64_t flags : 3; \
  } head

struct _robj_t {
  OBJECT_HEAD;
};

struct _rcons_t {
  rval_t car;
  rval_t cdr;
};

struct _rsym_t {
  OBJECT_HEAD;
  rval_t binding;
  rsym_t* left;
  rsym_t* right;
  chr_t name[1];
};

struct _rtype_t {
    OBJECT_HEAD;          // pointer to the type, also holds flags on the data field
    struct {
      uint64_t base_size : 16;
      uint64_t lowtag : 1;
      uint64_t free : 46;
    } flags;
    rproc_t* tp_constructor;
    void*  (*tp_new)();
    uint32_t (*tp_sizeof)();
    chr_t* (*tp_write)(rval_t, FILE*);
    rsym_t* tp_name;
};

struct _rstr_t {
  OBJECT_HEAD;
  chr_t chars[1];
};


struct _rproc_t {
  OBJECT_HEAD;
  rval_t formals;      // this is a list of argument names for lambdas and a list of argument types for builtins
  rval_t env;          // this is nil for builtins and list of local environments for lambdas
  rval_t body;         // either a builtin function or 
};

struct _rport_t {
  OBJECT_HEAD;
  FILE* stream;
  chr_t chrbuff[8];
};

/* 
   tag values 

   the lowtag comprises the 2 least significant bits of an rval_t. They indicate how the value should be interpreted.
   the lowtags are direct (small integers, floats, etc.), objptr (pointer to an object head with additional type information),
   strptr (a pointer to a null-terminated array of bytes), or a pointer to a cons.

   the widetag is the 3rd least-significant bit, and represents type-specific metadata. The widetag belongs to the type, ie,
   objects are not free to set the widetags of their constituent members. The widetag is used to, for example, indicate whether
   a cons is a valid list, or whether a bytestring represents binary or textual data.
 
*/

enum {
  LOWTAG_DIRECT=0b00,
  LOWTAG_OBJPTR=0b01,
  LOWTAG_BVECPTR=0b10,
  LOWTAG_CONSPTR=0b11,
  DEFAULT_WTAG=0b0,
};

/* 

   for convenience, the full tags for common builtin types and variants are given below
   these are not unambiguous typecodes, but they are to aid in creating tags and for distinguishing
   cons pointers from list pointers. They are NOT a proper type test.
*/

enum {
  TAG_INT=LOWTAG_DIRECT,
  TAG_TYPE=LOWTAG_OBJPTR,
  TAG_CONS=LOWTAG_CONSPTR,
  TAG_SYM=LOWTAG_OBJPTR,
  TAG_SYM_INTERNED=0x4 | LOWTAG_OBJPTR,
  TAG_SYM_UNINTERNED=LOWTAG_OBJPTR,
  TAG_BVEC=LOWTAG_BVECPTR,
  TAG_BVEC_BIN=0x4 | LOWTAG_BVECPTR,
  TAG_BVEC_TXT=LOWTAG_BVECPTR,
  TAG_STR=LOWTAG_OBJPTR,
  TAG_LIST= 0x4 | LOWTAG_CONSPTR,
  TAG_PROC=LOWTAG_OBJPTR,
  TAG_PORT=LOWTAG_OBJPTR,
};

enum {
  TYPECODE_NIL,
  TYPECODE_NONE,
  TYPECODE_TYPE,
  TYPECODE_BVEC,
  TYPECODE_CONS,
  TYPECODE_SYM,
  TYPECODE_STR,
  TYPECODE_PROC,
  TYPECODE_PORT,
  TYPECODE_INT,
  NUM_BUILTIN_TYPES,
};

chr_t* BUILTIN_TYPENAMES[] = { "nil", "none", "type", "bvec", "cons",
                                "sym", "str", "proc", "port", "int"};

#define ltag(v)          ((v) & 0x3)
#define wtag(v)          (((v) & 0x4)>>2)
#define tag(v)           ((v) & 0x7)
#define untag(v)         ((v) & ~0x7ul)
#define val(v)           ((v) >> 32)
#define ptr(v)           ((void*)untag(v))
#define cptr(v)          ((void*)(v))
#define tagptr(p,t)      (((rval_t)(p)) | (t))
#define tagval(v,t)      ((rval_t)(((v) << 32) | (t)))
#define hobj(v)          ((uchr_t*)ptr(v))
#define obj(v)           ((robj_t*)ptr(v))

/* memory */
#define AS_BYTES 8
#define AS_WORDS 1

uchr_t *HEAP, *EXTRA, *FREE, *FROMSPACE, *TOSPACE, *TOFREE;
int64_t HEAPSIZE;
bool GROWHEAP;
uint32_t TYPECOUNTER, NUMTYPES;
rtype_t** TYPES;

#define ALLOCATIONS() (FREE - HEAP)

/* evaluator */
// the stack
rval_t* STACK;
int32_t STACKSIZE, SP;

// the evaluator core to reduce errors and unnecessary checking, these registers are typed
rval_t EXP, VAL, CONTINUE;
rsym_t* NAME;               // special register for holding unevaluated names
rcons_t* ARGL, *UNEV, *ENV;
rproc_t* PROC;

rsym_t* GLOBALS;

// special constants
// The lowtags and typecodes are laid out so that NIL, including correct lowtag and typecode, has
// a value of 0, allowing it to be interpreted as C 'NULL'.
rval_t NIL = 0, NONE = TYPECODE_NONE << 3, OK, T;

#define isnil(v)  ((v) == NIL)
#define isnone(v) ((v) == NONE)
#define isok(v)   ((v) == OK)
#define istrue(v) ((v) == T)

/* basic error handling definitions */

jmp_buf SAFETY;
int32_t ERRORCODE;

enum {
  ERROR_TYPE=1,
  ERROR_ARITY,
  ERROR_VARNAME,
  ERROR_OVERFLOW,
  ERROR_NIL,
  ERROR_NONE,
  ERROR_FUNAPP,
  ERROR_IO,
  ERROR_NULLPTR,
};


const chr_t* ERRNAMES[] = {NULL, "TYPE", "ARITY", "VARNAME", "OVERFLOW",
                           "NIL", "NONE", "FUNAPP", "NULLPTR"};


#define eprintf(file, fmt, args...) fprintf(file, "%s: %d: %s: ", __FILE__, __LINE__, __func__); fprintf(file, fmt, ##args)
#define localerror() int32_t error = setjmp(SAFETY)
#define savepoint() setjmp(SAFETY)
#define escape(e) longjmp(SAFETY, e)


/* end common.h */
#endif
