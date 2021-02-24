#ifndef hamt_h
#define hamt_h

#include "rascal.h"
#include "obj.h"
#include "values.h"
#include "mem.h"
#include "error.h"
#include "bvec.h"


typedef enum
  {
    L1_MASK        = 0x0000001fu,
    L2_MASK        = L1_MASK << 5,
    L3_MASK        = L2_MASK << 5,
    L4_MASK        = L3_MASK << 5,
    L5_MASK        = L4_MASK << 5,
    L6_MASK        = L5_MASK << 5,
    L7_MASK        = 0xc0000000u,
  } hamt_mask_t;

typedef enum
  {
    HLVL_ONE = 1,
    HLVL_TWO,
    HLVL_THREE,
    HLVL_FOUR,
    HLVL_FIVE,
    HLVL_SIX,
    HLVL_SEVEN,
  } hamt_lvl_t;

leaf_t* hamt_insert(bvec_t*,hash_t,val_t);
obj_t*  hamt_search(bvec_t*,hash_t,val_t);
uint8_t hash_to_index(bvec_t*,hash_t);

#endif
