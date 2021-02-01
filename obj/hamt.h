#ifndef hamt_h

#define hamt_h
#include "../include/rsp_core.h"

/*

  internals for the HAMT implementation underlying sets and dicts

 */

typedef tuple_t node_t;

// forward declarations
bool       _has_bindings(obj_t*);

#define    has_bindings(h) _has_bindings((obj_t*)(h))

uint8_t    get_local_index(node_t*,hash32_t);
hash_t*    mk_hash(hash32_t,size_t,uint8_t);
node_t*    mk_hamt_node(size_t,uint8_t);
int32_t    copy_hamt_node(node_t*,node_t*);
val_t*     hamt_node_search(node_t*,val_t,hash32_t);
val_t*     hamt_hash_search(hash_t*,val_t);
val_t*     hamt_ihash_search(hash_t*,chr_t*,hash32_t);
val_t*     hamt_hash_addkey(hash_t*,val_t);
val_t*     hamt_node_addkey(node_t*,val_t,hash32_t);
val_t*     hamt_node_split(node_t*,uint8_t);

#endif
