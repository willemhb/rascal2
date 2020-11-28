#ifndef strlib_h

/* begin strlib.h */
#define strlib_h

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint32_t hash_t;

#define FNV1A_OFFSET_BASIS 0x811c9dc5u
#define FNV1A_PRIME        0x01000193u
#define streq(s1, s2) (strcmp(s1,s2)==0)

// forward declarations
hash_t hash_string(char*);
int32_t get_aligned_size(char*);
int32_t ceil_mod_eight(int32_t,int32_t);
/* end strlib.h */
#endif
