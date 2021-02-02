#ifndef rtypes_h

/* begin rtypes.h */
#define rtypes_h
#include "common.h"

/* typedefs for builtin types, the tagging system, and a few macros */

// typedefs for rascal types
typedef uintptr_t val_t;       // a tagged rascal value
typedef uint32_t tpkey_t;

/* non-object types */
typedef bool     rbool_t;
typedef int32_t  rint_t;
typedef ichr32_t rchr_t;
typedef flt32_t  rflt_t;
typedef FILE     iostrm_t;

/* core object types */
typedef struct obj_t  obj_t;
typedef struct vobj_t vobj_t;
typedef struct pair_t pair_t;
typedef struct list_t list_t;
typedef struct atom_t atom_t;
typedef struct ftuple_t ftuple_t;
typedef struct tuple_t tuple_t;
typedef struct btuple_t btuple_t;
typedef struct table_t table_t;
typedef table_t set_t;
typedef table_t dict_t;

/* metaobjects */
typedef struct type_t type_t;

/* C data representations */
typedef struct str_t   str_t;
typedef struct bstr_t  bstr_t;

// speciaized structures used by the VM
typedef tuple_t envt_t;
typedef tuple_t code_t;
typedef dict_t  nmspc_t;
typedef dict_t  module_t;
typedef tuple_t closure_t;

// valid function signatures for functions callable from within rascal
typedef val_t (*r_cfun_t)(size_t,val_t*);
typedef val_t (*r_zfun_t)();
typedef val_t (*r_ufun_t)(val_t);
typedef val_t (*r_bfun_t)(val_t,val_t);
typedef val_t (*r_mfun_t)(val_t,envt_t*,list_t*);

// signatures for functions used by the C API
typedef void*   (*r_callc_t)(type_t*,size_t,void*);
typedef int32_t (*r_cinit_t)(type_t*,val_t,void*);

// this struct holds vm state for handling exceptions
typedef struct _rsp_ectx_t rsp_ectx_t;

typedef enum
  {
    LTAG_LIST    = 0x00u,
    OBJ          = 0x01u,
    DIRECT       = 0x02u,
    OBJHEAD      = 0x03u,
    LTAG_IOSTRM  = 0x04u,
    LT_BOOL      = 0x02<<3 | DIRECT,
    LT_CHAR      = 0x05<<3 | DIRECT,
    LT_INT       = 0x06<<3 | DIRECT,
    LT_FLOAT     = 0x07<<3 | DIRECT,
    NONE         = 0xff,
  } ltag_t;

typedef enum
  {
    LIST           = LTAG_LIST,
    PAIR           = OBJ,
    BOOL           = DIRECT,
    NIL            = OBJHEAD,
    IOSTRM         = LTAG_IOSTRM,
    CHAR           = 0x05u,
    // numeric types
    INT            = 0x06u,
    FLOAT          = 0x07u,
    UINT           = 0x08u,
    IMAG           = 0x09u,
    CMPLX          = 0x0au,
    BIGINT         = 0x0bu,
    BIGFLT         = 0x0cu,
    BIGIMAG        = 0x0du,
    BIGCMPLX       = 0x0eu,
    BIGRAT         = 0x0fu,
    // end numeric types
    OBJECT         = 0x10u,
    FTUPLE         = 0x11u,
    SLEAF          = 0x12u,
    DLEAF          = 0x13u,
    ILEAF          = 0x14u,
    NTUPLE         = 0x15u,
    BTUPLE         = 0x16u,
    SNODE          = 0x17u,
    DNODE          = 0x18u,
    INODE          = 0x19u,
    STR            = 0x1au,
    BSTR           = 0x1bu,

    /* mapping types */
    TABLE          = 0x1cu,
    SET            = 0x1du,
    DICT           = 0x1eu,
    SYMTB          = 0x1fu,
    READTB         = 0x20u,
    NMSPC          = 0x21u,

    /* vm types */
    MODULE         = 0x22u,
    METHTAB        = 0x23u,
    CODE           = 0x24u,
    CLOSURE        = 0x25u,
    ENVT           = 0x26u,
    BLTNFUNC       = 0x27u,
    CPOINTER       = 0x28u,
    
    /* low-level types */
    CARRAY         = 0x29u,
    CVECTOR        = 0x2au,
    ATOM           = 0x2bu,

    /* metaobjects and base classes */

    TYPE           = 0x2cu,
    GENERIC        = 0x2du,
    PROTOCOL       = 0x2eu,
  } bltn_tpkey_t;

/* 
    canonical object heads
 */

#define RSP_OBJECT_HEAD      \
  uint32_t obj_tpkey;        \
  uint16_t obj_hflags;       \
  uint8_t  obj_lflags;       \
  uint8_t  obj_vflags;       \

// not currently implemented
#define RSP_VOBJECT_HEAD     \
  uint32_t obj_tpkey;        \
  uint16_t obj_hflags;       \
  uint8_t  obj_lflags;       \
  uint8_t  obj_vflags;	     \
  uptr_t   obj_size


typedef enum
  {
    SZKEY_NONE   = 0x0u,
    SZKEY_LENGTH = 0x1u,
    SZKEY_BITMAP = 0x2u,
  } szkey_t;

/* the common numeric type can be found by bitwise OR-ing the codes */

typedef enum
{
  CNUM_UINT8    =0b00000010u,
  CNUM_INT8     =0b00000100u,
  CNUM_UINT16   =0b00001010u,
  CNUM_INT16    =0b00000110u,
  CNUM_UINT32   =0b00011010u,
  CNUM_INT32    =0b00001110u,
  CNUM_UINT64   =0b00111010u,
  CNUM_INT64    =0b00011110u,
  CNUM_INT128   =0b00111110u,
  CNUM_FLOAT32  =0b00001111u,
  CNUM_FLOAT64  =0b00011111u,
  CNUM_FLOAT128 =0b00111111u,
  CNUM_IMAG32   =0b10001111u,
  CNUM_IMAG64   =0b10011111u,
  CNUM_IMAG128  =0b10111111u,
  CNUM_CMPLX32  =0b11001111u,
  CNUM_CMPLX64  =0b11011111u,
  CNUM_CMPLX128 =0b11111111u,
} c_num_t;

// these pointer codes are cruder than the numeric codes, but they should be able to
// express enough information about rascal fields to call basic C library functions
// (if the depth of a pointer is unknown, then it should be categorized as a void pointer)
typedef enum {
  PTR_NONE   = 0x00u,
  PTR_VOID   = 0x01u,
  PTR_CHR    = 0x02u,
  PTR_UCHR   = 0x04u,
  PTR_FILE   = 0x08u,
  PTR_RFUNC  = 0x10u,
  PTR_PTR    = 0x20u,
  PTR_PPTR   = 0x40u,
  PTR_TAGGED = 0x80u,
  PTR_RPTR   = PTR_TAGGED | PTR_VOID,
  PTR_RDATA  = PTR_TAGGED | PTR_NONE,
} c_ptr_t;

struct obj_t
{
  RSP_OBJECT_HEAD;
  uchr_t obj_data[8];
};

struct vobj_t
{
  RSP_VOBJECT_HEAD;
  uchr_t vobj_data[16];
};

#define vflags(o)  (o)->obj_vflags
#define otpkey(o)  (o)->obj_tpkey
#define hflags(o)  (o)->obj_hflags
#define lflags(o)  (o)->obj_lflags
#define osize(o)   (o)->obj_size

/* structs to represent builtin object types */
// untagged objects

struct pair_t
{
  val_t car;
  val_t cdr;
};

struct list_t
{
  val_t   car;
  list_t* cdr;
};

#define pcar(p) (p)->car
#define pcdr(p) (p)->cdr

// the generic tuple and ftuple are the building blocks for various extension types
struct ftuple_t
{
  RSP_OBJECT_HEAD;
  val_t elements[1];
};

struct tuple_t
{
  RSP_VOBJECT_HEAD;
  val_t elements[2];
};

struct btuple_t
{
  RSP_OBJECT_HEAD;
  uint32_t bt_size;
  uint32_t bt_map;
  val_t    elements[2];
};

#define bt_allcsz(b)   (b)->bt_size
#define bt_btmp(b)     (b)->bt_map

struct table_t
{
  RSP_OBJECT_HEAD;
  val_t mapping;
  val_t nkeys;
  val_t free;            // a linked list of free table nodes (unused)
};

#define tb_mapping(d)   (d)->mapping
#define tb_nkeys(d)     (d)->nkeys
#define tb_free(d)      (d)->free

const uint8_t MAX_TB_ND_LEVELS = 6;

struct atom_t
{
  RSP_OBJECT_HEAD;
  hash32_t hash;
  chr_t atm_name[4];
};

#define atom_hash(a)  (a)->hash
#define atom_flags(a) (a)->obj_lflags
#define atom_name(a)  &((a)->atm_name[0])

typedef enum
  {
    RESERVED = 0x008u,
    KEYWORD  = 0x004u,
    INTERNED = 0x002u,
    GENSYM   = 0x001u,
  } smflags_t;

struct str_t
{
  RSP_OBJECT_HEAD;
  chr_t str_chrs[8];
};

struct bstr_t
{
  RSP_VOBJECT_HEAD;
  uchr_t bytes[8];
};

#define str_chars(s)   &((s)->str_chrs[0])
#define bstr_nbytes(b) (b)->size
#define bstr_bytes(b)  &((b)->bytes[0])


/* core metaobject */
struct type_t {
  RSP_OBJECT_HEAD;
  tpkey_t   tp_tp_key;     // the appropriate type key for objects of this type
  hash32_t  tp_hash;       // the hash for this type
  /* general predicates */

  bool       tp_direct_p;
  bool       tp_heap_p;
  bool       tp_atomic_p;
  bool       tp_store_p;
  bool       tp_final_p;
  bool       tp_varsz_p;
  bool       tp_cdata_p;   // indicates whether the type holds an C data

  /* size and layout information */
  uint8_t     tp_vtag;
  uint8_t     tp_otag;
  uint8_t     tp_c_num;
  uint8_t     tp_c_ptr;
  uint8_t     tp_elsz;                   // for variably sized types
  size_t      tp_base_size;              // the base size in bytes
  size_t      tp_nfields;
  r_cfun_t*   tp_rsp_new;                // the rascal callable constructor

  /* C api */
  void        (*tp_prn)(val_t,iostrm_t*);
  int32_t     (*tp_ord)(val_t,val_t);
  hash32_t    (*tp_hash_val)(val_t);
  size_t      (*tp_size)(obj_t*);
  size_t      (*tp_elcnt)(obj_t*);
  val_t*      (*tp_get_data)(obj_t*);
  val_t       (*tp_copy)(type_t*,val_t,uchr_t**);

  /* Just the dang type name! */
  chr_t name[1];
};

/* union types for handling different representations of rascal values */
typedef union
{
  rint_t  integer;
  rflt_t  float32;
  rchr_t  unicode;
  rbool_t boolean;
  uint32_t bits32;
} rsp_c32_t;

typedef union
{
  val_t value;
  struct
  {
    rsp_c32_t bits;
    uint32_t pad : 24;
    uint32_t tag :  8;
  } padded;
} rsp_c64_t;

/* end rtypes.h */
#endif
