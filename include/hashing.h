#ifndef hashing_h
#define hashing_h
#include "common.h"
#include "strlib.h"


uint32_t cpow2_32(int32_t);
uint64_t cpow2_64(int64_t);
uint64_t clog2(uint64_t);
hash_t   hash_string(const chr_t*,uint32_t);
hash_t   hash_bytes(const uchr_t*,uint32_t,size_t);
hash_t   hash_int(const int32_t, uint32_t);
hash_t   hash_float(const flt32_t,uint32_t);
hash_t   hash_array(const uint32_t*,uint32_t,size_t);

#endif
