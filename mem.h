#ifndef mem_h

/* begin mem.h */
#define mem_h
#include "common.h"
#include "obj.h"

/*

  gc allocator, and initializer

  The allocator takes an integer (the base size), an optional pointer to an arbitrary
  object, and a pointer to a function that calculates the size of that object in bytes, aligned
  to a multiple of 8.

  The generic initializer takes a pointer to the head of the new object, a pointer to some data,
  and the number of items in the array to be initialized. If the last argument is negative, then
  the the last element of the array is treated as an array of bytes.

*/

#define MAXSTACK 4096          // arbitrary
#define MAXTYPES UINT_MAX >> 3
#define MAXBASE USHRT_MAX
#define INIT_HEAPSIZE 8 * 2048 // in bytes
#define INIT_NUMTYPES 256      // number of records
#define RAM_LOAD_FACTOR 0.8    // resize ram if, directly after garbage collection, (FREE - HEAP) > more than RAM_LOAD_FACTOR * (HEAPSIZE)
#define CHECK_RESIZE() (FREE - HEAP) > (RAM_LOAD_FACTOR * HEAPSIZE) ? true : false

/* general memory helpers */
int32_t vm_align_memory(uchr_t**,uchr_t*);
/* memory initialization */
void vm_init_heap();
void vm_init_types();

void vm_gc();
rval_t vm_gc_trace(rval_t*);
rsym_t* vm_gc_trace_sym(rsym_t*);
uchr_t* vm_gc_copy(uchr_t*,uchr_t*,int32_t);
uchr_t* vm_allocate(int32_t);
rtype_t* vm_new_type();

/* memory bounds checking and overflow checking */
bool vm_heap_limit(uint32_t);
bool vm_type_limit(uint32_t);
bool vm_type_overflow(uint32_t);
bool vm_stack_overflow(uint32_t);
bool vm_type_overflow(uint32_t);

/* end mem.h */
#endif
