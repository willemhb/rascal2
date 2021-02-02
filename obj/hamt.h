#ifndef hamt_h

#define hamt_h
#include "../include/rsp_core.h"

/*

  internals for the HAMT implementation underlying sets and dicts

 */

typedef struct
{
  RSP_VOBJECT_HEAD;
  uptr_t hash;
  val_t keys[1];
} leaf_t;

typedef tuple_t node_t;
typedef vobj_t hamt_t;

typedef enum
  {
    NODE  = TBNODE,
    SLEAF = TBSLEAF,
    DLEAF = TBDLEAF,
    ILEAF = TBILEAF,
  } node_tp_t;

typedef enum
  {
    OUT_OF_RANGE = 0x00,
    
    NODE_BOUND   = 0x01,
    LEAF_BOUND   = 0x02,
  } node_idx_tp_t;

// forward declarations
uint32_t   node_size(node_t*);
uint32_t   node_bmap(node_t*);
node_tp_t  hamt_classify(hamt_t*);
uint8_t    get_local_index(node_t*,hash32_t);
leaf_t*    mk_hamt_leaf(hash32_t,size_t,uint8_t);
node_t*    mk_hamt_node(size_t,uint8_t);
int32_t    copy_hamt_node(node_t*,node_t*);
val_t*     hamt_node_search(node_t*,val_t,hash32_t);
val_t*     hamt_node_addkey(node_t*,val_t,hash32_t);
val_t*     hamt_node_rmvkey(node_t*,val_t,hash32_t);

#endif
