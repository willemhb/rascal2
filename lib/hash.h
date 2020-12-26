#ifndef hash_h

/* begin hash.h */
#define hash_h

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <wctype.h>
#include <stdio.h>

typedef uint32_t hash_t;

#define FNV1A_OFFSET_BASIS 0x811c9dc5u
#define FNV1A_PRIME        0x01000193u
#define streq(s1, s2) (strcmp(s1,s2)==0)

// forward declarations
hash_t hash_string(const char*);
int32_t get_aligned_size(char*);
int32_t ceil_mod_eight(int32_t,int32_t);
char* itoa(int32_t,int32_t);
int32_t int_str_len(int64_t,int64_t);

/* helpers for treating utf-8 strings as arrays of fixed-size characters */
wint_t nth_wc(char* s, size_t n);
int find_wc(wchar_t cp, char* s);
int replace_nth_wc(char* old, char* new, size_t i, wchar_t cp, int max);
size_t nchars(char*);
int testuc(unsigned char);

/* 

   utf-8 low level io helpers 

*/
wint_t fgetuc(FILE*);
int fputuc(wchar_t,FILE*);
int fungetuc(wchar_t,FILE*);
wint_t peekuc(FILE*);

/* end hash.h */
#endif
