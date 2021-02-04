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
#define nd_prefix(b)      (b)->obj_hflags

typedef enum
  {
    INDEX_FREE     = 0x00,
    NODE_BOUND     = 0x01,
    LEAF_BOUND     = 0x02,
  } node_idx_tp_t;

typedef enum
  {
    L1_MASK        = 0x0000001fu,
    L2_MASK        = L1_MASK << 5,
    L3_MASK        = L2_MASK << 5,
    L4_MASK        = L3_MASK << 5,
    L5_MASK        = L4_MASK << 5,
    L6_MASK        = L5_MASK << 5,
  } hamt_mask_t;

typedef enum
  {
    HLVL_ONE = 1,
    HLVL_TWO,
    HLVL_THREE,
    HLVL_FOUR,
    HLVL_FIVE,
    HLVL_SIX,
  } hamt_lvl_t;

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
uint8_t        node_hash_chunk(node_t*,hash32_t);
uint8_t        lvl_hash_chunk(hamt_lvl_t,hash32_t);


#define        get_hash_chunk(x,h)					\
  _Generic((x),node_t*:node_hash_chunk,hamt_lvl_t:lvl_hash_chunk)(x,h)



uint8_t        get_local_index(node_t*,uint8_t);
uint8_t        get_prefix(hamt_lvl_t,hash32_t);
bool           check_prefix(node_t*,hash32_t);
uint8_t        find_split(hash32_t,hash32_t,hamt_lvl_t);
node_idx_tp_t  check_index(node_t*,hash32_t);
val_t          get_nd_index(node_t*,hash32_t);
leaf_t*        mk_leaf(hash32_t,tpkey_t);
node_t*        mk_node(size_t,uint8_t,hash32_t,tpkey_t);
node_t*        node_realloc(node_t*);
leaf_t*        leaf_realloc(leaf_t*);
val_t*         node_split(val_t*,node_t*,tpkey_t,hamt_lvl_t,val_t,hash32_t);
val_t*         leaf_split(val_t*,leaf_t*,tpkey_t,hamt_lvl_t,val_t,hash32_t);
val_t*         hamt_insert(val_t*,val_t,hash32_t,tpkey_t);
val_t*         node_insert(node_t*,val_t*,val_t,hash32_t);
val_t*         leaf_insert(leaf_t*,val_t*,val_t);
val_t*         hamt_search(hamt_t*,val_t,hash32_t);
val_t*         leaf_search(leaf_t*,val_t,hash32_t);
val_t*         ileaf_search(leaf_t*,chr_t*,hash32_t);
uint32_t       hamt_prn_traverse(val_t,iostrm_t*);


#endif
