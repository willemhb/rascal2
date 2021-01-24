#ifndef rtypes_h

/* begin rtypes.h */
#define rtypes_h
#include "common.h"

/* typedefs for builtin types, the tagging system, and a few macros */


// typedefs for rascal types
typedef uintptr_t val_t;       // a tagged rascal value
typedef int8_t rbool_t;        // rascal booleans can be signed
typedef int32_t rint_t;
typedef flt32_t rflt_t;
typedef ichr32_t rchr_t;
typedef chr8_t rstr_t;
typedef struct list_t list_t;
typedef struct cons_t cons_t;
typedef struct bstr_t bstr_t;
typedef FILE iostrm_t;
typedef struct sym_t sym_t;
typedef struct method_t method_t;
typedef struct cprim_t cprim_t;
typedef struct vec_t vec_t;
typedef struct node_t node_t;
typedef struct table_t table_t;
typedef struct whead_t whead_t;
typedef struct nhead_t nhead_t;
typedef struct obj_t obj_t;
typedef struct cval_t cval_t;
typedef struct type_t type_t;

// generic callable type
typedef struct func_t func_t;

// aliases for special types used by the VM
typedef vec_t envt_t;
typedef vec_t cfrm_t;
typedef vec_t code_t;
typedef table_t nmspc_t;
typedef table_t symtab_t;
typedef table_t readtab_t;

/* 
   valid function signatures for functions callable from within rascal 

   functions of up to 3 direct arguments are supported for speed, but generally functions take their
   arguments from the stack. If a builtin function accepts variable arguments, then it should take exactly two parameters;
   the number of arguments and the top of the stack. 

   macro functions implemented in C take the expansion environment as their first argument.

 */

typedef union
{
  val_t (*z_arg_fun)();
  val_t (*u_arg_fun)(val_t);
  val_t (*b_arg_fun)(val_t,val_t);
  val_t (*n_arg_fun)(val_t*);
  val_t (*v_arg_fun)(size_t,val_t*);
  val_t (*z_arg_mac)(envt_t*);
  val_t (*u_arg_mac)(envt_t*,val_t);
  val_t (*b_arg_mac)(envt_t*,val_t,val_t);
  val_t (*n_arg_mac)(envt_t*,val_t*);
  val_t (*v_arg_mac)(envt_t*,size_t,val_t*);
  val_t (*z_arg_meta)(type_t*);
  val_t (*u_arg_meta)(type_t*,val_t);
  val_t (*b_arg_meta)(type_t*,val_t,val_t);
  val_t (*n_arg_meta)(type_t*,val_t*);                 
  val_t (*v_arg_meta)(type_t*,size_t,val_t*);
  val_t (*z_arg_meta_mac)(type_t*,envt_t*);
  val_t (*u_arg_meta_mac)(type_t*,envt_t*,val_t);
  val_t (*b_arg_meta_mac)(type_t*,envt_t*,val_t,val_t);        
  val_t (*n_arg_meta_mac)(type_t*,envt_t*,val_t*);
  val_t (*v_arg_meta_mac)(type_t*,envt_t*,size_t,val_t*);
} rcfun_t;

// this struct holds vm state for handling exceptions
typedef struct _rsp_ectx_t rsp_ectx_t;


// a union of all the C value structures used to represent builtin rascal values
typedef union
{
  uptr_t    rsp_ptr;
  uint32_t  rsp_bits;
  rchr_t    rsp_char;
  rint_t    rsp_int;
  rbool_t   rsp_bool;
  rflt_t    rsp_float;
  iostrm_t* rsp_iostrm;
  list_t*   rsp_list;
  cons_t*   rsp_cons;
  rstr_t*   rsp_str;
  bstr_t*   rsp_bstr;
  node_t*   rsp_node;
  vec_t*    rsp_vec;
  table_t*  rsp_table;
  method_t* rsp_method;
  cprim_t*  rsp_cprim;
  type_t*   rsp_type;
  obj_t*    rsp_obj;
} rsp_unboxed_t;

/*
  tagging system

  tags are used by the VM for two purposes:

  1) determining the type of an object
  2) determining if the VM has reserved an object for special use cases

  there are two kinds of tags:

  1) tags stored in the low bits of the value ('vtag')
  2) tags stored in the low bits of the object head ('otag')

  tags are divided into two parts:

  1) a 4-bit narrow tag ('ntag') stored in the 4 least-significant bits of every rascal value and every tagged object head
  2) a 4-bit wide tag ('wtag') stored in the remainder of the least-significant byte or tagged object head
  
  All rascal values have vtag. The vtag is either:
  1) the 4-least significant bits of the value, and the rest of the value is a pointer to the object
  2) the 8-least significant bits of the value, and the 32 most-significant bits store the data

  Some rascal values also have an otag. An otag is either:
  1) the 4-least significant bits of the object head, and the rest of the object head is a pointer to the object's type
  2) the 8-least significant bits of the object head, and the rest of the object head is object metadata

  If a value does not store its type object directly, then the type object can be retrieved by using the vtag to index an array of builtin type pointers

  The following vtags indicate object heads with a 4-bit object tag

  1) VTAG_METHOD
  2) VTAG_CPRIM
  3) VTAG_GENERIC
  3) VTAG_OBJECT

  The following vtags indicate object heads with an 8-bit object tag

  1) VTAG_TABLE
  2) VTAG_ARR
  3) VTAG_BIGNUM

*/

typedef enum
{
  NTAG_LIST             = 0x00,  
  NTAG_CONS             = 0x01,  
  NTAG_IOSTRM           = 0x02,
  NTAG_STR              = 0x03,  
  NTAG_BSTR             = 0x04,  
  NTAG_SYM              = 0x05,  
  NTAG_NODE             = 0x06,  
  NTAG_VEC              = 0x07, 
  NTAG_TABLE            = 0x08,  
  NTAG_BIGNUM           = 0x09,
  NTAG_METHOD           = 0x0a,    
  NTAG_CPRIM            = 0x0b,
  NTAG_GENERIC          = 0x0c,
  NTAG_OBJECT           = 0x0d,
  NTAG_DIRECT           = 0x0f,  // 32-bit data, rest of the tag is in the next 4 bits
} ntag_t;

/*
  wtags are used for two purposes:

  1) indicating numeric types

  2) flagging builtin types as being reserved by the VM for special purposes

 */

typedef enum
  {
    WTAG_INT            = 0x10,
    WTAG_UINT           = 0x20,
    WTAG_FLOAT          = 0x30,
    WTAG_IMAG           = 0x40,
    WTAG_CMPLX          = 0x50,
    WTAG_RATNL          = 0x60,
    WTAG_BOOL           = 0x60,
    WTAG_CHAR           = 0x70,
    WTAG_CONTFRM        = 0x10,
    WTAG_BYTECODE       = 0x20,
    WTAG_ENVT           = 0x30,
    WTAG_NMSPC          = 0x40,
    WTAG_SYMTAB         = 0x50,
    WTAG_READTAB        = 0x60,
    WTAG_MODULE         = 0x80,
    WTAG_MODULE_NMSPC   = WTAG_MODULE | WTAG_NMSPC,
    WTAG_MODULE_ENVT    = WTAG_MODULE | WTAG_ENVT,
    WTAG_EXTENSION      = 0xf0,                           // indicates a user extension of builtin types with tagged object heads
    WTAG_NONE           = 0xff,                           // an invalid widetag - indicates the specified narrow tag doesn't have a wide component
  } wtag_t;

typedef enum
{
  VTAG_LIST             = NTAG_LIST,  
  VTAG_CONS             = NTAG_CONS,  
  VTAG_IOSTRM           = NTAG_IOSTRM,
  VTAG_STR              = NTAG_STR,  
  VTAG_BSTR             = NTAG_BSTR,  
  VTAG_SYM              = NTAG_SYM,  
  VTAG_NODE             = NTAG_NODE,  
  VTAG_VEC              = NTAG_VEC, 
  VTAG_TABLE            = NTAG_TABLE,  
  VTAG_BIGNUM           = NTAG_BIGNUM,
  VTAG_METHOD           = NTAG_METHOD,    
  VTAG_CPRIM            = NTAG_CPRIM,
  VTAG_GENERIC          = NTAG_GENERIC,
  VTAG_OBJECT           = NTAG_OBJECT,
  VTAG_INT              = WTAG_INT   | NTAG_DIRECT,
  VTAG_UINT             = WTAG_UINT  | NTAG_DIRECT,
  VTAG_FLOAT            = WTAG_FLOAT | NTAG_DIRECT,
  VTAG_IMAG             = WTAG_IMAG  | NTAG_DIRECT,
  VTAG_BOOL             = WTAG_BOOL  | NTAG_DIRECT,
  VTAG_CHAR             = WTAG_CHAR  | NTAG_DIRECT,
  VTAG_NONE             = WTAG_NONE,                   // sentinel
} vtag_t;

typedef enum
  {
    OTAG_VEC           = NTAG_VEC,
    OTAG_TABLE         = NTAG_TABLE,
    OTAG_METHOD        = NTAG_METHOD,
    OTAG_CPRIM         = NTAG_CPRIM,
    OTAG_GENERIC       = NTAG_GENERIC,
    OTAG_OBJECT        = NTAG_OBJECT,
    OTAG_CONTFRM       = WTAG_CONTFRM     | NTAG_VEC,
    OTAG_BYTECODE      = WTAG_BYTECODE    | NTAG_VEC,
    OTAG_LOCALENVT     = WTAG_ENVT        | NTAG_VEC,
    OTAG_MODENVT       = WTAG_MODULE_ENVT | NTAG_VEC,
    OTAG_LOCALNMSPC    = WTAG_NMSPC       | NTAG_TABLE,
    OTAG_MODNMSPC      = WTAG_NMSPC       | NTAG_TABLE,
    OTAG_SYMTAB        = WTAG_SYMTAB      | NTAG_TABLE,
    OTAG_READTAB       = WTAG_READTAB     | NTAG_TABLE,
    OTAG_NONE          = WTAG_NONE,
  } otag_t;

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


/* object head macros */
// the generic 
#define OBJECT_HEAD                   \
  val_t metadata


#define NARROW_OBJECT_HEAD	      \
  val_t metadata     : 60;	      \
  val_t otag         :  4

#define WIDE_OBJECT_HEAD              \
  val_t metadata     : 56;            \
  val_t otag         :  8


/* initsz is the number of initial vector slots set aside for metadata (not including the
   extra slot set aside for the current index by dynamic vectors). common vectors don't use 
   this field, but VM vectors use this space to extend the vector */

#define VEC_HEAD                      \
  val_t datasz       : 32;            \
  val_t initsz       : 25;            \
  val_t localvals    :  1;	      \
  val_t dynamic      :  1;	      \
  val_t otag         :  8;            \

typedef enum
  {
    VFLAG_DYNAMIC   = 0x0100,
    VFLAG_LOCALVALS = 0x0200,
  } vecfl_t;

/* global indicates this table isn't allocated in the heap */
#define TABLE_HEAD                    \
  val_t   reserved : 32;              \
  val_t   flags    : 19;              \
  val_t   global   :  1;              \
  val_t   bcnt     :  4;              \
  val_t   otag     :  8

#define METHOD_HEAD                     \
  val_t argco     : 32;                 \
  val_t ndefaults : 16;                 \
  val_t pure      :  1;                 \
  val_t kwargs    :  1;                 \
  val_t opts      :  1;                 \
  val_t varopts   :  1;                 \
  val_t varkw     :  1;			\
  val_t vargs     :  1;                 \
  val_t macro     :  1;                 \
  val_t mopfun    :  1;		        \
  val_t otag      :  4

typedef enum
  {
    FUNFL_PURE    = 0x8000,
    FUNFL_KWARGS  = 0x4000,
    FUNFL_OPTS    = 0x2000,
    FUNFL_VAROPTS = 0x1000,
    FUNFL_VARKW   = 0x0800,
    FUNFL_VARGS   = 0x0400,
    FUNFL_MACRO   = 0x0200,
    FUNFL_MOPFUN  = 0x0100,
  } funfl_t;

/* structs to represent builtin object types */
// untagged objects
struct list_t
{
  val_t   car;
  list_t* cdr;
};

struct cons_t {
  val_t car;
  val_t cdr;
};

struct bstr_t {
  val_t  size;
  uchr8_t bytes[1];
};

struct sym_t {
  // 32-bit hash
  val_t hash      : 32;
  val_t flags     : 27;
  val_t keyword   :  1;
  val_t gensym    :  1;
  val_t reserved  :  1;
  val_t global    :  1;
  val_t interned  :  1;
  char name[1];
};

typedef enum
  {
    
    SMFL_INTERNED = 0x01,
    SMFL_GLOBAL   = 0x02,
    SMFL_RESERVED = 0x04,
    SMFL_GENSYM   = 0x08,
    SMFL_KEYWORD  = 0x10,
  } smfl_t;

struct node_t {
  signed long  order   : 33; // the original insertion order
  signed long  flags   : 30; // custom flags
  signed long  balance :  2; // the balance factor (can be -1, 0, or 1)
  node_t* left;
  node_t* right;
  val_t   nd_data;           // this is either the hashkey, or a cons point32_ter whose first element is the hashkey and whose second element is a fixed vector of bindings
};

typedef enum
  {
    ARGFL_VARKWPM = 0x0020,  // this parameter
    ARGFL_VARPM   = 0x0010,  // this paramater is a list of vargs
    ARGFL_OPTION  = 0x0004,  // this parameter is optional; if present, its value is t, otherwise, its value is f
    ARGL_DEFAULT  = 0x0008,  // this parameter has a default value
  } argfl_t;

struct type_t {
  /*meta information */
  OBJECT_HEAD;           // pointer to the metaobject that created the type
  type_t* tp_parent;     // a pointer to the parent type
  type_t* tp_origin;     // a pointer to the builtin type that this type subtyped (if any)
  hash32_t tp_order;     // the order in which this type was created
  hash32_t tp_hash;

  /* general predicates */
  bool       tp_builtin_p;
  bool       tp_direct_p;
  bool       tp_atomic_p;
  bool       tp_heap_p;
  bool       tp_store_p;
  bool       tp_final_p;
  bool       tp_extensible_p;

  /* size and layout information */
  vtag_t     tp_vtag;
  otag_t     tp_otag;
  uint16_t   tp_c_num;
  uint16_t   tp_c_ptr;

  size_t    tp_base;                  // the type's base size in bytes (this can be 0)
  size_t    tp_nfields;               // the number of rascal accessible fields
  size_t    tp_elsz;                  // the size of individual elements of a collection
  table_t*  tp_fields;                // the type's rascal accessible fields
  cprim_t*  tp_new;                   // called to create a new rascal value 

  size_t      (*tp_alloc_sz)(type_t*,size_t,val_t*);  // called by extensions of builtin types to determine the total allocation size
  val_t       (*tp_init)(uchr8_t*,size_t,val_t*);     // called by extensions of builtin types to initialize builtin part
  type_t*     (*tp_subtype)(type_t*,size_t,val_t*);   // called to create a new subtype
  void        (*tp_prn)(val_t,iostrm_t*);
  int32_t     (*tp_ord)(val_t,val_t);
  size_t      (*tp_size)(val_t);
  /* Just the dang type name! */
  chr8_t name[1];
};

struct obj_t
{
  OBJECT_HEAD;               // (possibly tagged) generic object head
  val_t fields[1];           // Object fields; used by extension types
};

struct nhead_t
{
  NARROW_OBJECT_HEAD;
  val_t fields[1];
};

struct whead_t
{
  WIDE_OBJECT_HEAD;
  val_t fields[1];
};

struct vec_t
{
  VEC_HEAD;
  val_t data;                // for fixed vectors, this is the first element in the collection; 
};

struct table_t {
  TABLE_HEAD;
  val_t count;
  val_t order;
  node_t* records;            // a point32_ter to the root of the underlying AVL tree
};

struct func_t                 // a genericized method head
{
  METHOD_HEAD;
  val_t generic;
};

struct cprim_t
{
  METHOD_HEAD;
  val_t generic;
  table_t* names;             // cprims don't lookup values but they might accept keyword or optional arguments; if they do, the first argument is always a bitmap of all the
  rcfun_t callable;
};

struct method_t
{
  METHOD_HEAD;
  val_t    generic;
  table_t* names;             // the local arguments
  vec_t*   code;              // the bytecode and constant values
  vec_t*   envt;              // the captured environment
  val_t fields[1];
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
