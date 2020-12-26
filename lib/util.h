#ifndef util_h

/* begin util.h */
#define util_h

#include <stddef.h>
#include <ctype.h>
#include "numrepr.h"

/* various library utilities that don't quite fit anywhere else */

char    ntodchr(digit_t);
digit_t dchrton(char);

// get the number of digits required to represent an integral value
int     indigits(long,num_base_t);
int     fndigits(float,num_base_t);

/* array utilities */
int  reverse(void*,size_t,size_t);

/* memory utilities */

// valid alignment sizes
typedef enum {
  ALIGN_1  =  1,
  ALIGN_2  =  2,
  ALIGN_4  =  4,
  ALIGN_8  =  8,
  ALIGN_16 = 16,
} alignment_t;

// given a number of bytes, find the least value <= a minimum aligned to the given align
size_t aligned_size(size_t,size_t,alignment_t);
// determine the alignment of an arbitrary memory location
alignment_t get_alignment(unsigned char*);
// align a block of memory of the given size 
void align_mem(unsigned char**,size_t,size_t,alignment_t);
#endif
