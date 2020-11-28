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
  OBJECT_HEAD;
  rval_t car;
  rval_t cdr;
};

enum {
  CONSFLAG_NOLIST,
  CONSFLAG_LIST,
};

struct _rsym_t {
  OBJECT_HEAD;
  rval_t binding;
  rsym_t* left;
  rsym_t* right;
  chr_t name[1];
};

enum {
  SYMFLAG_UNINTERNED,
  SYMFLAG_INTERNED,
};

struct _rtype_t {
    OBJECT_HEAD;          // pointer to the type, also holds flags on the data field
    struct {
      uint64_t base_size : 16;
      uint64_t lowtag : 1;
      uint64_t free : 46;
    } flags;
    rval_t (*tp_constructor)();
    void*  (*tp_new)();
    uint32_t (*tp_sizeof)();
    rsym_t* tp_name;
};

struct _rstr_t {
  OBJECT_HEAD;
  chr_t chars[1];
};


struct _rproc_t {
  OBJECT_HEAD;
  rval_t argspec;      // this is a list of argument names for lambdas and a list of argument types for builtins
  rval_t env;          // this is nil for builtins and list of local environments for lambdas
  rval_t body;         // either a builtin function or 
};

enum {
  PROCFLAGS_CALLMODE_FUNC=0b000,
  PROCFLAGS_CALLMODE_MACRO=0b001,
  PROCFLAGS_BODYTYPE_EXPR=0b000,
  PROCFLAGS_BODYTYPE_CFNC=0b010,
  PROCFLAGS_VARGS_TRUE=0b100,
  PROCFLAGS_VARGS_FALSE=0b000,
};

struct _rport_t {
  OBJECT_HEAD;
  FILE* stream;
};

/* tag values */

enum {
  LOWTAG_DIRECT,
  LOWTAG_OBJPTR,
};

enum {
  TYPECODE_NIL,
  TYPECODE_NONE,
  TYPECODE_TYPE,
  TYPECODE_SYM,
  TYPECODE_CONS,
  TYPECODE_STR,
  TYPECODE_PROC,
  TYPECODE_PORT,
  TYPECODE_INT,
  NUM_BUILTIN_TYPES,
};

chr_t* BUILTIN_TYPENAMES[] = { "nil", "none", "type", "sym", "cons", "str",
                               "proc", "port", "int"};

#define lowtag(v)        ((v) & 0b1)
#define widetag(v)       (((v) & 0b110) >> 1)
#define setwidetag(v,t)  v = v & (~0b110ul) | t
#define untagv(v)        ((v) & (~0b111ul))
#define val(v)           ((v) >> 32)
#define ptr(v)           ((void*)((v) & ~0x3))
#define cptr(v)          ((void*)(v))
#define tagptr(p,t)      (((rval_t)(p)) | (t))
#define tagval(v,t)      ((rval_t)(((v) << 32) | (t)))
#define heapobj(v)       ((uchr_t*)ptr(v))
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

// the evaluator core
rval_t EXP, ARGL, VAL, UNEV, PROC, ENV, CONTINUE;
rsym_t* GLOBALS;

// special constants
rval_t NIL = TYPECODE_NIL, NONE = TYPECODE_NONE, OK, T;

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
