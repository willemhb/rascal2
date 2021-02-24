#include "../include/rvec.h"


const type_t SVEC_TYPE_OBJ =
  {
    .type               = DATATYPE,
    .cmeta              = RSVECTOR,
    .tp_tpkey           = RSVECTOR,
    .tp_ltag            = OBJECT,
    .tp_elsz            = 8u,
    .tp_cnum            = CNUM_UINT64,
    .tp_cptr            = PTR_RPTR,
    .tp_el_cnum         = CNUM_UINT64,
    .tp_el_cptr         = PTR_RVALUE,
    .tp_init_sz         =  8,
    .tp_base_sz         = 16,
    .tp_data_offset     =  8,
    .tp_nfields         =  0,
    .tp_isallocated_fl  =  1,
    .tp_prn             = vec_prn,
    .tp_call            = NULL,
    .tp_sizeof          = bvec_sizeof,
    .tp_elcnt           = bvec_elcnt,
    .tp_hash            = hash_vec,
    .tp_ord             = NULL,
    .tp_bltn_new        = NULL,
    .tp_alloc           = vobj_alloc,
    .tp_init            = vobj_init,
    .tp_relocate        = vec_relocate,
    .tp_from_cdata      = NULL,
    .tp_isallocated_fun = NULL,
    .name = "vector",
  };
