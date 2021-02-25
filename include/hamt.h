#ifndef hamt_h
#define hamt_h

#include "rascal.h"
#include "values.h"
#include "error.h"
#include "obj.h"
#include "describe.h"


struct bvec_t
{
  tpkey_t  type;
  uint32_t bv_bmap;
  val_t    bv_elements[1];
};


// base leaf type
typedef struct leaf_t
{
  tpkey_t type;
  hash_t  hash;
  val_t   _keys;
} leaf_t;

// leaf type for maps that store bindings
typedef struct dleaf_t
{
  tpkey_t type;
  hash_t  hash;
  val_t   key;
  val_t   value;
  struct dleaf_t* next;
} dleaf_t;

// leaf type for maps that don't store bindings
typedef struct sleaf_t
{
  tpkey_t type;
  hash_t  hash;
  list_t* keys;
} sleaf_t;


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
    GLOBAL     = 0x01u, // the table is globally allocated
    CSTRKEY    = 0x02u, // values inserted into the table are raw C strings
    BINDINGS   = 0x04u, // the table stores key/value pairs
  } tb_flags_t;

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



uint32_t popcnt(uint32_t);
uint32_t get_mask(hamt_lvl_t);
bool     isbvec(val_t);
bool     isleaf(val_t);
bool     issleaf(val_t);
bool     isdleaf(val_t);
leaf_t*  sf_toleaf(const chr_t*,int32_t,const chr_t*,val_t*);
bvec_t*  sf_tobvec(const chr_t*,int32_t,const chr_t*,val_t*);
bvec_t*  mk_bvec(uint8_t,size_t,uint16_t);
leaf_t*  mk_leaf(hash_t,val_t,uint16_t);
bvec_t*  cp_bvec(bvec_t*,int32_t);
size_t   bvec_elcnt(val_t);
size_t   bvec_sizeof(type_t*,val_t);
val_t*   bvec_ref(bvec_t*,uint8_t);
obj_t*   hamt_search(val_t,val_t);
val_t    hamt_insert(val_t*,val_t,uint16_t,rcmp_t);
val_t    hamt_put(val_t*,val_t,val_t,uint16_t,rcmp_t);
int32_t  hamt_remove(val_t*,val_t,uint16_t);

#define tobvec(v) sf_tobvec(__FILE__,__LINE__,__func__,&(v))
#define toleaf(v) sf_toleaf(__FILE__,__LINE__,__func__,&(v))

#endif
