#ifndef hamt_h

#define hamt_h
#include "../include/rsp_core.h"
#include "../include/describe.h"

/*

  internals for the HAMT implementation underlying sets and dicts

 */

typedef btuple_t node_t;
typedef btuple_t leaf_t;
typedef btuple_t hamt_t;

#define leaf_allocated(b) (b)->bt_size
#define leaf_free(b)      (b)->obj_hflags
#define leaf_hash(b)      (b)->bt_map
#define nd_level(b)       (b)->obj_lflags
#define nd_dict(b)        (b)->obj_hflags & 0x1
#define nd_global(b)      (b)->obj_hflags & 0x2

typedef enum
  {
    INDEX_FREE     = 0x00,
    NODE_BOUND     = 0x01,
    LEAF_BOUND     = 0x02,
  } node_idx_tp_t;

// type testing macros
#define isnode(v)  GEN_PRED(v,node)
#define isleaf(v)  GEN_PRED(v,leaf)
#define isileaf(v) GEN_PRED(v,ileaf)
#define issleaf(v) GEN_PRED(v,sleaf)
#define isdleaf(v) GEN_PRED(v,dleaf)
#define isinode(v) GEN_PRED(v,inode)
#define issnode(v) GEN_PRED(v,snode)
#define isdnode(v) GEN_PRED(v,dnode)

#define isglobal(v)                     \
  ({                                    \
     tpkey_t __t__ = tpkey(v)         ; \
     __t__ == INODE || __t__ == ILEAF ; \
  })



// forward declarations



uint32_t       leaf_nkeys(leaf_t*);
uint32_t       node_nkeys(node_t*);
uint32_t       node_free(node_t*);
uint8_t        get_hash_chunk(node_t*,hash32_t);
uint8_t        get_local_index(node_t*,uint8_t);
uint8_t        find_split(hash32_t,hash32_t);
node_idx_tp_t  check_index(node_t*,hash32_t);
val_t          get_nd_index(node_t*,hash32_t);
leaf_t*        mk_leaf(hash32_t,tpkey_t);
node_t*        mk_node(size_t,uint8_t,tpkey_t);
node_t*        node_realloc(node_t*);
node_t*        leaf_realloc(leaf_t*);
val_t*         node_insert(node_t*,val_t,hash32_t);
val_t*         leaf_insert(leaf_t*,val_t,hash32_t);
val_t*         hamt_search(hamt_t*,val_t,hash32_t);
val_t*         leaf_search(leaf_t*,val_t,hash32_t);
val_t*         ileaf_search(leaf_t*,chr_t*,hash32_t);
val_t*         hamt_addkey(hamt_t**,val_t,hash32_t);
val_t*         leaf_addkey(leaf_t**,val_t,hash32_t);
val_t*         ileaf_addkey(leaf_t**,val_t,hash32_t);
val_t*         hamt_rmvkey(node_t*,val_t,hash32_t);
val_t*         hamt_setkey(node_t*,val_t,val_t,hash32_t);
val_t*         hamt_split(node_t*,val_t,hash32_t,uint8_t);
uint32_t       hamt_prn_traverse(val_t,iostrm_t*);


#endif
