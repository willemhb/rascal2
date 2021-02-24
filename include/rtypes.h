#ifndef rtypes_h

/* begin rtypes.h */
#define rtypes_h
#include "common.h"

/* typedefs for builtin types, the tagging system, and a few macros */

// typedefs for rascal types
typedef uintptr_t val_t;       // a tagged rascal value
typedef uint32_t  tpkey_t;

/* core direct types */
typedef bool     rbool_t;
typedef int32_t  rint_t;
typedef cint32_t rchr_t;
typedef flt32_t  rflt_t;

/* C types with a direct rascal representation */
typedef FILE     riostrm_t;

/* core object types */
typedef struct pair_t    pair_t;
typedef struct symbol_t  symbol_t;
typedef struct obj_t     obj_t;
typedef struct vobj_t    vobj_t;
typedef struct cval_t    cval_t;
typedef struct vcval_t   vcval_t;

/* builtin object types */
typedef struct rstr_t     rstr_t;
typedef struct bytes_t    bytes_t;
typedef struct svec_t     svec_t;
typedef struct bvec_t     bvec_t;
typedef struct function_t function_t;
typedef struct builtin_t  builtin_t;

/* metaobjects */
typedef struct type_t type_t;

// valid function signatures for functions callable from within rascal
typedef val_t (*rbuiltin_t)(val_t*,size_t);

// builtin typecodes - direct types must be multiples of 8
enum
  {
    LIST     = 0x00u,
    PAIR     = 0x01u,
    SYMBOL   = 0x02u,
    IOSTRM   = 0x03u,
    OBJECT   = 0x04u,
    CVALUE   = 0x05u,
    FUNCTION = 0x06u,
    DIRECT   = 0x07u,
    NIL      = 0x08u,
    STRING   = 0x09u,
    BYTES    = 0x0au,
    RSVECTOR = 0x0bu,
    RBVECTOR = 0x0cu,
    TABLE    = 0x0du,
    SYMTAB   = 0x0eu,
    DATATYPE = 0x0fu,
    CHAR     = 0x10u,
    BOOL     = 0x18u,
    INTEGER  = 0x20u,
  };


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
  PTR_RVALUE = 0xffu,                  // the pointer can't be characterized in general
} c_ptr_t;

#define OBJECT_HEAD   \
  uint32_t type;      \
  uint32_t cmeta

# define VOBJECT_HEAD \
  uint32_t type;      \
  uint32_t cmeta;     \
  val_t    size

struct pair_t
{
  val_t car;
  val_t cdr;
};

struct symbol_t
{
  uint32_t flags;
  hash_t hash;
  chr_t name[8];
};

struct obj_t
{
  OBJECT_HEAD;
  val_t    data[1];
};

struct vobj_t
{
  VOBJECT_HEAD;
  val_t data[2];
};

struct cval_t
{
  OBJECT_HEAD;
  uchr_t c_data[8];
};

struct vcval_t
{
  VOBJECT_HEAD;
  uchr_t c_data[16];
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
  val_t    value;
  void*    ptr;
  uchr_t*  mem;
  uint64_t bits64;
  rint_t   integer;
  rflt_t   float32;
  rchr_t   unicode;
  rbool_t  boolean;
  uint32_t bits32;
} rsp_c64_t;

typedef enum
  {
    FIXED           = 0x00u,
    IMPLICIT        = 0x01u, // ie null-terminated byte string
    VARIABLE        = 0x02u,
    BITMAPPED       = 0x04u,
    WIDE            = 0x08u,
    SMALL_LEN       = VARIABLE,
    SMALL_BITMAPPED = BITMAPPED | VARIABLE,
    WIDE_LEN        = WIDE | VARIABLE,
    WIDE_BITMAPPED  = WIDE | BITMAPPED | VARIABLE,
  } sizing_t;

typedef struct
{
  c_num_t el_cnum;
  c_ptr_t el_cptr;
  uint16_t el_sz;
} cvspec_t;

typedef struct
{
  void        (*prn)(val_t,riostrm_t*);
  val_t       (*call)(type_t*,val_t*,size_t);
  size_t      (*size)(type_t*,val_t);
  size_t      (*elcnt)(val_t);
  hash_t      (*hash)(val_t,uint32_t);
  int32_t     (*ord)(val_t,val_t);
  val_t       (*new)(type_t*,val_t,size_t);
  val_t       (*builtin_new)(val_t,size_t);
  void*       (*init)(type_t*,val_t,size_t,void*);
  val_t       (*relocate)(type_t*,val_t,uchr_t**);
  uint8_t     (*isalloc)(type_t*,val_t);
} capi_t;

struct type_t {
  OBJECT_HEAD;
  /* type flags and field data */
  tpkey_t tp_tpkey;
  tpkey_t tp_ltag;
  /* general sizing data */
  bool     tp_isalloc;
  sizing_t tp_sizing;
  size_t   tp_init_sz;   // the size of the object's meta-information (in bytes)
  size_t   tp_base_sz;   // the size of the object's fixed-size portion
  size_t   tp_nfields;   // the number of fixed fields on the object
  /* element sizing data */
  cvspec_t* tp_cvtable;
  /* C api */
  capi_t* tp_capi;
  /* Just the dang type name! */
  chr_t name[];
};

/* end rtypes.h */
#endif
