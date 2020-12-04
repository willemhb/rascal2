 #ifndef obj_h

/* begin obj.h */
#define obj_h

#include "common.h"
#include "error.h"

/* 

   This module contains the core API for objects (type checking, casting), types
   (getting type data and type metadata), direct values (because they don't warrant their own
   module), conses & lists (because they're fundamental), and allocation/memory management.

   The basic error handling and logging mechanisms can be found in error.h/rascal.h.
   
   The core API for objects (type checking, casting), types (getting type data and type
   metadata), direct values (because they don't warrant their own module), conses & lists 
   (because they're fundamental), and allocation/memory management can be found in obj.h/obj.c
   
   The API for symbols, tables, and environments can be found in env.h/env.c. 
   
   The API for functions, procedures, the compiler/evaluator/vm, etc., can be found in
   eval.h/eval.c.

   The API for strings, bytevectors, ports, and the reader can be found in txtio.h/txtio.c.

   The API for builtin functions, bootstrapping, initialization, and cleanup can be found
   in builtins.h/builtins.c.

   The main function can be found in rascal.c!

 */

/* forward declarations */
/* type data */
uint_t typecode(val_t);
type_t* type_of(val_t);
chr_t* typename(val_t);
bool callable(val_t);
bool atomic(val_t);
int_t vm_size(val_t);

/* convenience macros for casting and testing */
// unsafe casts to builtin types
#define totype_(v) ((type_t*)ptr(v))
#define toint_(v)  ((int_t)val(v))
#define totab_(v)  ((tab_t*)ptr(v))
#define tocons_(v) ((cons_t*)ptr(v))
#define tosym_(v)  ((sym_t*)ptr(v))
#define tostr_(v)  ((str_t*)ptr(v))
#define toproc_(v) ((proc_t*)ptr(v))
#define toport_(v) ((port_t*)ptr(v))

/* 
  
   Basic object API accessor API.

   The object API uses _Generic extensively, in order to enable usage of the same set
   of accessors for functions that work with val_t and functions that work with C types.

   Public fields have safe and fast accessors. 

*/

// fast type checks for builtin types
#define istype(v) (typecode(v) == TYPECODE_TYPE)
#define issym(v)  (typecode(v) == TYPECODE_SYM)
#define iscons(v) (typecode(v) == TYPECODE_CONS)
#define istab(v)  (typecode(v) == TYPECODE_TAB)
#define isstr(v)  (typecode(v) == TYPECODE_STR)
#define isproc(v) (typecode(v) == TYPECODE_PROC)
#define isport(v) (typecode(v) == TYPECODE_PORT)
#define isint(v)  (typecode(v) == TYPECODE_INT)
#define isa(v,t)  (typecode(v) == (t))


#define SAFECAST_MACRO(v,tp)			                                                 \
  ({ val_t _v_ = v ;                                                                             \
    if (!is##tp(_v_)) { escapef(TYPE_ERR,stdout,"Expected type %s, got %s",#tp,typename(v)); } ; \
    to##tp##_(_v_) ;								                 \
  })

#define FAST_ACCESSOR_MACRO(v,ctype,f)                 \
  (_Generic((v),				       \
	    val_t:(ctype)ptr(v),		       \
	    ctype:v)->f)

#define totype(v)   SAFECAST_MACRO(v,type)
#define tosym(v)    SAFECAST_MACRO(v,sym)
#define totab(v)    SAFECAST_MACRO(v,tab)
#define tocons(v)   SAFECAST_MACRO(v,cons)
#define tostr(v)    SAFECAST_MACRO(v,str)
#define toproc(v)   SAFECAST_MACRO(v,proc)
#define toport(v)   SAFECAST_MACRO(v,port)
#define toint(v)    SAFECAST_MACRO(v,int)

/* memory management */

/*

  gc allocator, and initializer

  The allocator takes an integer (the base size), an optional pointer to an arbitrary
  object, and a pointer to a function that calculates the size of that object in bytes, aligned
  to a multiple of 8.

  The allocator will align a minimum of 16 bytes, to ensure there is space to replace the object
  with a forwarding pointer during garbage collection. However, only an alignment of 8 can be
  guaranteed, so right now I'm not bothering to try aligning on a 16-byte boundary (if I can
  figure out how to do this I'll switch).
*/

#define MAXSTACK 4096          // arbitrary
#define MAXTYPES UINT_MAX >> 3 
#define MAXBASE USHRT_MAX
#define INIT_HEAPSIZE 8 * 2048 // in bytes
#define INIT_NUMTYPES 256      // number of records
#define RAM_LOAD_FACTOR 0.8    // resize ram if, directly after garbage collection, (FREE - HEAP) > more than RAM_LOAD_FACTOR * (HEAPSIZE)
#define HEAP_ALIGNSIZE 8       // the alignment of the heap
#define CHECK_RESIZE() (FREE - HEAP) > (RAM_LOAD_FACTOR * HEAPSIZE) ? true : false


int_t align_heap(uchr_t**,uchr_t*);
void gc();
val_t gc_trace(val_t*);
uchr_t* gc_copy(uchr_t*,int_t);
uchr_t* vm_allocate(int_t);
type_t* new_type();                         

/* memory bounds checking and overflow checking */
bool heap_limit(uint_t);
bool type_limit(uint_t);
bool type_overflow(uint_t);
bool stack_overflow(uint_t);
bool type_overflow(uint_t);

/* conses & lists */

typedef enum cons_sel {
  TAIL=-1,
  A   =0b00010,
  D   =0b00011,
  AA  =0b00100,
  AD  =0b00101,
  DA  =0b00110,
  DD  =0b00111,
  AAA =0b01000,
  AAD =0b01001,
  ADA =0b01010,
  DAA =0b01100,
  ADD =0b01011,
  DAD =0b01101,
  DDA =0b01110,
  DDD =0b01111,
  AAAA=0b10000,
  AAAD=0b10001,
  AADA=0b10010,
  ADAA=0b10100,
  DAAA=0b11000,
  AADD=0b10011,
  ADAD=0b10101,
  DAAD=0b11001,
  DADA=0b11010,
  DDAA=0b11100,
  ADDD=0b10111,
  DADD=0b11011,
  DDAD=0b11101,
  DDDA=0b11110,
  DDDD=0b11111,
} cons_sel;

#define car_(v) FAST_ACCESSOR_MACRO(v,cons_t*,car)
#define cdr_(v) FAST_ACCESSOR_MACRO(v,cons_t*,cdr)

/* the vm api for conses */
val_t tagc(cons_t*);                // because conses are represented by two lowtags, they
val_t car(val_t);                // have a special tagging function
val_t cdr(val_t);
bool  islist(val_t);
val_t cxr(val_t,cons_sel);
int_t ncells(val_t);
val_t cons(val_t,val_t);
val_t append(val_t*,val_t);
val_t extend(val_t*,val_t);
val_t append_env(val_t*,val_t);

#define isc iscons
#define toc tocons
#define toc_ tocons_

/* end obj.h */
#endif
