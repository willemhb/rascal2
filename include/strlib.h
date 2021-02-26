#ifndef strlib_h
#define strlib_h
#include "common.h"

// numeric utilities & hashing utilities
int32_t  incu8(wchar_t*, const chr_t*);
size_t   strsz(const chr_t*);
int32_t  u8strlen(const chr_t*);
int32_t  u8strcmp(const chr_t*,const chr_t*);
int32_t  u8len(const chr_t*);
int32_t  u8tou32(chr32_t*,const chr_t*);
cint32_t nextu8(const chr_t*);
cint32_t nthu8(const chr_t*,size_t);
int32_t  iswodigit(cint32_t);
int32_t  iswbdigit(cint32_t);

#endif
