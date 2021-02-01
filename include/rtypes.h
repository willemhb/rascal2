#ifndef rtypes_h

/* begin rtypes.h */
#define rtypes_h
#include "common.h"

/* typedefs for builtin types, the tagging system, and a few macros */


// typedefs for rascal types
typedef uintptr_t val_t;       // a tagged rascal value

/* non-object types */
typedef bool     rbool_t;
typedef int32_t  rint_t;
typedef ichr32_t rchr_t;
typedef flt32_t  rflt_t;
typedef FILE     iostrm_t;

/* core object types */
typedef struct obj_t obj_t;
typedef struct pair_t pair_t;
typedef struct list_t list_t;
typedef struct sym_t  sym_t;
typedef struct atom_t atom_t;
typedef struct tuple_t tuple_t;
typedef tuple_t btuple_t;
typedef tuple_t tbnode_t;
typedef tuple_t anode_t;
typedef btuple_t bnode_t;
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

// signatures for functions used by the C API
typedef void*   (*r_callc_t)(type_t*,size_t,void*);  // the void argument is for any C data that needs to be accounted for in allocation
typedef int32_t (*r_cinit_t)(type_t*,val_t,void*);   // the void argument is for any C data that needs to be initialized

// this struct holds vm state for handling exceptions
typedef struct _rsp_ectx_t rsp_ectx_t;

typedef enum
  {
    NIL_TP         = 0x00u,
    BOOL_TP        = 0x01u,
    CHAR_TP        = 0x02u,
    INT_TP         = 0x03u,
    FLOAT_TP       = 0x04u,
    /* reserved but not implemented */
    UINT_TP        = 0x05u,
    IMAG_TP        = 0x06u,
  } bltn_direct_tp_t;

const uint64_t NUM_DIRECT_TYPES = IMAG_TP + 1;

typedef enum
  {
    /* string and symbol types */
    ATOM_TP        = 0x00u,
    SYM_TP         = 0x01u,
    STR_TP         = 0x02u,
    BSTR_TP        = 0x03u,

    /* tuple types */
    TUPLE_TP       = 0x04u,
    BTUPLE_TP      = 0x05u,
    // reserved but not used
    
 /* TBBNODE_TP     = 0x06u, 
    TBANODE_TP     = 0x07u, */
    
    CODE_TP        = 0x08u,
    CLOSURE_TP     = 0x09u,
    ENVT_TP        = 0x0au,

    /* mapping types */
    TABLE_TP       = 0x0bu,
    SET_TP         = 0x0cu,
    DICT_TP        = 0x0du,
    SYMTB_TP       = 0x0eu,
    READTB_TP      = 0x0fu,
    NMSPC_TP       = 0x10u,
    MODULE_TP      = 0x11u,
    METHTAB_TP     = 0x12u,

    /* low-level types */
    CARR_TP        = 0x13u,
    CVEC_TP        = 0x14u,
    BIGINT_TP      = 0x15u,
    BIGFLT_TP      = 0x16u,
    BIGIMAG_TP     = 0x17u,
    BIGCMPLX_TP    = 0x18u,
    BIGRAT_TP      = 0x19u,

    /* function types */
    BLTNFUNC_TP    = 0x1au,
    METHOD_TP      = 0x1bu,
    CPOINTER_TP    = 0x1cu,
    
    /* types that don't need to fit into the narrowed type tag space */
    OBJECT_TP      = 0x1du,
    PAIR_TP        = 0x1eu,
    LIST_TP        = 0x1fu,
    IOSTRM_TP      = 0x20u,
    TYPE_TP        = 0x21u,
    GENERIC_TP     = 0x22u,
  } bltn_obj_tp_t;

typedef enum
  {
    LIST        = 0x00u,     // a pointer to a list object (implicitly a pair)
    OBJ         = 0x01u,     // a pointer to an arbitrary object (implicitly a pair)
    DIRECT      = 0x02u,     // direct data
    OBJHEAD     = 0x03u,     // marks the start of an object head
    IOSTRM      = 0x04u,

 /* BLTNHEAD    = 0x07u, */

    BOOL        = (BOOL_TP << 3)    | DIRECT,
    CHAR        = (CHAR_TP << 3)    | DIRECT,
    INT         = (INT_TP << 3)     | DIRECT,
    FLOAT       = (FLOAT_TP << 3)   | DIRECT,
    ATOM        = (ATOM_TP << 3)    | OBJHEAD,
    STRING      = (STR_TP << 3)     | OBJHEAD,
    BSTRING     = (BSTR_TP << 3)    | OBJHEAD,
    TUPLE       = (TUPLE_TP << 3)   | OBJHEAD,
    BTUPLE      = (BTUPLE_TP << 3)  | OBJHEAD,
    DICT        = (DICT_TP << 3)    | OBJHEAD,
    TYPE        = (TYPE_TP << 3)    | OBJHEAD,
    NONE        = 0xffu,     // an invalid lowtag
    
  } ltag_t;

/* 
    canonical object heads
 */

#define RSP_OBJECT_HEAD    \
  uint32_t obj_wfield;     \
  uint16_t obj_hflags;     \
  uint8_t  obj_lflags;     \
  uint8_t  obj_tag

// not currently implemented
#define RSP_VOBJECT_HEAD   \
  uint32_t obj_tp_key;     \
  uint16_t obj_hflags;     \
  uint8_t  obj_lflags;     \
  uint8_t  obj_tag;        \
  uint64_t obj_size

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

#define otag(o) (o)->obj_tag

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

// the generic tuple is the building block for most extension types
struct tuple_t
{
  RSP_OBJECT_HEAD;
  val_t elements[1];
};

#define tuple_szdata(t) (t)->obj_wfield
#define tuple_flags(t)  (t)->obj_lflags
#define tbnd_level(t)   (t)->obj_lflags

struct table_t
{
  RSP_OBJECT_HEAD;
  val_t mapping;
  val_t nkeys;
  val_t ordkeys;              // a double-ended, ordered list of the dictionary's keys and
};                            // bindings

#define tb_flags(d)     (d)->obj_hflags
#define tb_levels(d)    (d)->obj_lflags
#define tb_mapping(d)   (d)->mapping
#define tb_nkeys(d)   (d)->nkeys
#define tb_ordkeys(d) (d)->ordkeys

struct atom_t
{
  RSP_OBJECT_HEAD;
  chr_t atm_chrs[8];
};

#define atm_hash(a)  (a)->obj_wfield
#define atm_flags(a) (a)->obj_lflags
#define atm_name(a)  &((a)->atm_chrs[0])

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
  RSP_OBJECT_HEAD;
  uptr_t nbytes;
  uchr_t bytes[8];
};

#define str_chars(s)   (s)->str_chrs
#define bstr_nbytes(b) (b)->nbytes
#define bstr_bytes(b)  (b)->bytes


/* core metaobject */
struct type_t {
  RSP_OBJECT_HEAD;
  uint32_t  tp_tp_key;     // the appropriate type key for objects of this type
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
