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

   Every object has a safe and fast accessor for each of its non writeable non-head fields.

   The safe accessor dispatches to a function that, for val_t, checks that the value is of
   that type, setting the global ERRORCODE if it doesn't and returning a sentinel, and for
   C types checks that the value is non-null (setting the global ERRORCODE if it is).  Safe
   accessors return a pointer to the field on success and a NULL pointer on failure.

   The fast accessor expands to a C structure access, with or without a cast depending on
   whether the value is a val_t. The fast accessor can be used safely if, eg, it's known that
   the same value recently passed a typecheck, or is known to be non-null. In general, it
   shouldn't be used unless the appropriate validation has already been performed.

   Each C type also has safe and unsafe casting macros. The safe caster checks the typecode
   first and returns NULL if the typecode is incorrect.

   In general, object API functions should accept val_t arguments, perform any checks that are necessary,
   fail if any of those checks fail (setting the value of ERRORCODE), and return a pointer to the desired
   field if they succeed. Functions that take non val_t arguments may be used internally (symbol interning
   is a good example), but shouldn't be accessed externally.

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


#define SAFECAST_MACRO(v,test,cast,sentinel)			\
  ({ val_t _v_ = v ;                                            \
    test(_v_) ? cast(_v_) : seterr(TYPE_ERR,sentinel);		\
  })

#define FAST_ACCESSOR_MACRO(v,ctype,f)                 \
  (_Generic((v),				       \
	    val_t:(ctype)ptr(v),		       \
	    ctype:v)->f)

#define SAFE_ACCESSOR_MACRO(v,ctype,ctype_acc,val_acc) \
  _Generic((v),                                        \
	   ctype:ctype_acc,			       \
	   val_t:val_acc)(v)


#define totype(v)   SAFECAST_MACRO(v,istype,totype_,NULL)
#define tosym(v)    SAFECAST_MACRO(v,issym,tosym_,NULL)
#define totab(v)    SAFECAST_MACRO(v,istab,totab_,NULL)
#define tocons(v)   SAFECAST_MACRO(v,iscons,tocons_,NULL)
#define tostr(v)    SAFECAST_MACRO(v,isstr,tostr_,NULL)
#define toproc(v)   SAFECAST_MACRO(v,isproc,toproc_,NULL)
#define toport(v)   SAFECAST_MACRO(v,isport,toport_,NULL)
#define toint(v)    SAFECAST_MACRO(v,isint,toint_,-2)

/* direct data */
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

/* general memory helpers */
int_t align_heap(uchr_t**,uchr_t*);

/* memory initialization */
void init_heap();
void init_types();

void gc();
val_t gc_trace(val_t*);
uchr_t* gc_copy(uchr_t*,int_t);
uchr_t* vm_allocate(int_t);
type_t* new_type();                    // location

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

/* the vm api for conses */
val_t tagc(cons_t*);                // because conses are represented by two lowtags, they
val_t* _vcar(val_t);                // have a special tagging function
val_t* _ccar(cons_t*);
val_t* _vcdr(val_t);
val_t* _ccdr(cons_t*);
bool _vislist(val_t);
bool _cislist(cons_t*);
val_t* _vcxr(val_t,cons_sel);
val_t* _ccxr(cons_t*,cons_sel);
int_t _vncells(val_t);
int_t _cncells(cons_t*);
cons_t* new_cons(val_t,val_t);
val_t cons(val_t,val_t);

#define isc iscons
#define toc tocons
#define toc_ tocons_

#define islist(v) _Generic((v),val_t:_vislist,cons_t*:_cislist)(v)
#define car(v)     SAFE_ACCESSOR_MACRO(v,cons_t*,_ccar,_vcar)
#define cdr(v)     SAFE_ACCESSOR_MACRO(v,cons_t*,_ccdr,_vcdr)
#define car_(v)    FAST_ACCESSOR_MACRO(v,cons_t*,car)
#define cdr_(v)    FAST_ACCESSOR_MACRO(v,cons_t*,cdr)
#define cxr(c,s)   _Generic((c),val_t:_vcxr,cons_t*:_ccxr)(c,s)
#define ncells(v)  _Generic((v),val_t:_vncells,cons_t*:_cncells)(v)
#define hastail(c) isc(cdr_(c))


/* end obj.h */
#endif
