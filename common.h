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
typedef uintptr_t val_t;
typedef intptr_t  ival_t;
typedef unsigned char uchr_t;
typedef char chr_t;
typedef chr_t str_t;                 // refers to memory in the lisp heap allocated for str
typedef int int_t;
typedef uint32_t uint_t;
typedef struct type_t type_t;
typedef struct obj_t obj_t;
typedef struct cons_t cons_t;
typedef cons_t list_t;               // conses and lists are structurally identical, but
typedef struct sym_t sym_t;          // functions that require a list_t* should be verified
typedef struct proc_t proc_t;        // as lists before being passed to those functions.
typedef struct tab_t tab_t;          // a generic table type
typedef struct port_t port_t;

/* 
   the generic object head contains a 32-bit meta-field (different types can use this for 
   different purposes), 3 1-bit boolean flags, and a type field giving the typecode for
   objects of that type. If a type requires non-boolean flags, these can be stored in
   meta.
 
*/

#define OBJECT_HEAD     \
  struct {              \
    val_t meta : 32;    \
    val_t type : 29;    \
    val_t flags_2 : 1;  \
    val_t flags_1 : 1;	\
    val_t flags_0 : 1;  \
  } head

struct obj_t {
  OBJECT_HEAD;
};

/* These macros provide the core api for working with values  and object types */

#define tag(v)           ((v) & 0x7)
#define untag(v)         (((val_t)(v)) & ~0x7ul)
#define val(v)           (((val_t)(v)) >> 32)
#define ptr(v)           ((void*)untag(v))
#define cptr(v)          ((void*)(v))
#define tagptr(p,t)      (((val_t)(p)) | (t))
#define tagval(v,t)      ((((val_t)(v)) << 32) | (t))
#define heapobj(v)       ((uchr_t*)ptr(v))
#define obj(v)           ((obj_t*)ptr(v))

/* object head accessors */
#define head(o)          (((obj_t*)(o))->head)
#define meta_(o)          (((obj_t*)(o))->head.meta)
#define type_(o)         (((obj_t*)(o))->head.type)   
#define flags_(obj,off)    (((obj_t*)(obj))->head.flags_##off)


/* 
   
   Below are the definitions of the core builtin object types, any special flags or enums they
   use, and alias macros for their flag and meta fields (If they have an object head). Flag
   accessors are prefixed with fl_.

 */

struct cons_t {
  val_t car;
  val_t cdr;
};



struct sym_t {
  OBJECT_HEAD;
  val_t binding;
  chr_t name[1];
};


typedef enum sym_fl {
  VARIABLE=0,
  CONSTANT,
} sym_fl;


#define fl_const(s)       ((s)->head.flags_0)
#define isconst(s)        (fl_const(s) == CONSTANT)


/*

  The long term goal is to implement tables as AVL trees, since the metadata required for
  this can be stored in the existing object head with no additional space requirements,
  but this is not an immediate priority. 

  Right now tables are just symbol tables, but they're written to allow arbitrary types
  to be used. Future plans include coming up with a proper generic interface to allow tab_t
  to be used as a proper dictionary type.
  
 */

struct tab_t {
  OBJECT_HEAD;
  val_t key;
  val_t binding;
  tab_t* left;
  tab_t* right;
};


struct type_t {
    OBJECT_HEAD;          // pointer to the type, also holds flags on the data field
    struct {
      val_t base_size : 16;  // the minimum size (in bytes) for objects of this type.
      val_t val_lowtag : 3;  // the lowtag that should be used for objects of this type.
      val_t atomic : 1;      // can values be used as a table key?
      val_t callable : 1;    // can values be used as a function?
      val_t free : 44;
    } flags;
  /* the rascal-callable constructor */
    val_t tp_new;
    // the vm-callable C function to allocate and initialize a new value (NULL for direct data)
    // the vm-callable C function to get a value's size in bytes (can be NULL)
    uint_t (*tp_sizeof)();
    // the vm-callable C function to print or write a value of this type
    void (*tp_prn)(val_t, port_t*);
    // the authoritative type name
    sym_t* tp_name;
};

#define typecode_self(t)  meta_(t)
#define fl_base_size(t)   ((t)->flags.base_size)
#define fl_val_lowtag(t)  ((t)->flags.val_lowtag)
#define fl_atomic(t)      ((t)->flags.atomic)
#define fl_callable(t)    ((t)->flags.callable)


struct proc_t {
  OBJECT_HEAD;
  val_t formals;
  val_t env;
  val_t body;
};


typedef enum proc_fl {
  CALLMODE_FUNC,
  CALLMODE_MACRO,
  BODYTYPE_EXPR=0,
  BODYTYPE_CFNC,
  VARGS_TRUE=0,
  VARGS_FALSE,
} proc_fl;

/* procedure accessors */

#define argco_(p)          meta_(p)
#define callmode_(p)       flags_(p,0)
#define bodytype_(p)       flags_(p,1)
#define vargs_(p)          flags_(p,2)

struct port_t {
  OBJECT_HEAD;
  FILE* stream;
  chr_t chrbuff[8];
};

typedef enum port_fl {
  BINARY_PORT,
  TEXT_PORT,
  READ=1,
  WRITE=1,
} port_fl;

#define fl_iotype(p)      ((p)->head.flags_0)
#define fl_readable(p)    ((p)->head.flags_1)
#define fl_writable(p)    ((p)->head.flags_2)
#define isopen(p)         (fl_writable(p) || fl_readable(p))

/* 
   tag values

   The lowtag indicates how to read the type and data on a val_t. Direct objects store their
   type information and data in the word itself. Object pointers point to an object head 
   with a type field. Cons pointers point to a cons cell that may or may not also be a list
   (the list invariant has not been checked). It has a type of cons. A list pointer points to
   a cons whose list invariant has been checked. It has a type of cons. A string pointer
   points to a utf-8 encoded null terminated bytestring. It has a type of str. 

   The lowtag consobj, listobj, and strobj are pointers to objects whose only data member is
   a cons (stored directly), a list pointer (stored as a pointer), or an array of bytes that
   may be binary data or text data (the bytes begin immediately after the object head). These
   are included to allow future subtyping of any builtin type with a consistent API.

   Other C types can be represented, but cannot be represented directly as val_t for memory
   safety (port_t is an example).
   
   The tag and typecode system is set up so that the special value NIL is identical to the
   C value NULL (NIL has a lowtag of LOWTAG_DIRECT and its typecode is 0).
*/

typedef enum ltag_t {
  LOWTAG_DIRECT  =0b000,
  LOWTAG_CONSPTR =0b001,
  LOWTAG_LISTPTR =0b010,
  LOWTAG_STRPTR  =0b011,
  LOWTAG_OBJPTR  =0b100,
  LOWTAG_CONSOBJ =0b101,
  LOWTAG_LISTOBJ =0b110,
  LOWTAG_STROBJ  =0b111,
} ltag_t;

enum {
  TYPECODE_NIL,
  TYPECODE_CONS,
  TYPECODE_NONE,
  TYPECODE_STR,
  TYPECODE_TYPE,
  TYPECODE_SYM,
  TYPECODE_TAB,
  TYPECODE_PROC,
  TYPECODE_PORT,
  TYPECODE_INT,
  NUM_BUILTIN_TYPES,
};

const chr_t* BUILTIN_TYPENAMES[] = { "nil-type", "cons", "none-type", "str",
                                      "type", "sym", "tab", "proc", "port", "int", };




// these generic macros are intended to simplify tagging values
#define tagi(v) tagval((v), (TYPECODE_INT << 3) || LOWTAG_DIRECT)
#define tagv(v) _Generic((v), \
			 int_t:tagval(v, (TYPECODE_INT << 3) || LOWTAG_DIRECT))

#define tagp(p) tagptr(p,\
		       _Generic((p),\
				sym_t*:LOWTAG_STROBJ,\
				str_t*:LOWTAG_STRPTR,\
				default:LOWTAG_OBJPTR))

/* memory */
#define AS_BYTES 8
#define AS_WORDS 1

uchr_t *HEAP, *EXTRA, *FREE, *FROMSPACE, *TOSPACE;
ival_t HEAPSIZE;
bool GROWHEAP;
uint_t TYPECOUNTER, NUMTYPES;
type_t** TYPES;

#define ALLOCATIONS() (FREE - HEAP)

/* evaluator */
// the stack
val_t* STACK;
int_t STACKSIZE, SP;

// The core evaluator registers
val_t EXP, VAL, CONTINUE, NAME, ENV, UNEV, ARGL, PROC;
// working registers (never saved, always free)
val_t WRX, WRY, WRZ; 
tab_t* GLOBALS;
// special constants
// The lowtags and typecodes are laid out so that NIL, including correct lowtag and typecode,
// is equal to C 'NULL'
// FPTR is a special value used to indicate that this cell has been moved to the new heap
// during garbage collection. 
val_t NIL, NONE, OK, T, FPTR;
val_t R_STDIN, R_STDOUT, R_STDERR, R_PROMPT;

#define isnil(v)         ((v) == NIL)
#define isnone(v)        ((v) == NONE)
#define isok(v)          ((v) == OK)
#define istrue(v)        ((v) == T)
#define isfptr(v)        ((v) == FPTR)
#define cbooltorbool(v)  ((v) ? T : NIL)
#define rbooltocbool(v)  ((v) == NIL || (v) == NONE ? false : true)

/* end common.h */
#endif
