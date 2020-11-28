#ifndef fapi_h

/* begin fapi.h */
#define fapi_h

#include "common.h"
#include "obj.h"

typedef struct _cspec_t cspec_t;

/* 
   the fapi module defines an api for calling C functions from rascal.

   the argspec doesn't live in rascal memory, so the alignment doesn't matter.

   Right now the API is extremely conservative; there must be a direct mapping between
   the rascal value and the C type for the test to pass. This means that only int, flt,
   ucp, str, bvec, and port types can pass (more sophisticated behavior will maybe be
   implemented earlier).
*/


struct _cspec_t { 
  u32_t argco;
  u16_t rtype;
  u16_t atypes[16];   // arbitrarily large 
};

enum {
  CS_VOID=0x00,
  CS_I8=0x01,
  CS_I16=0x02,
  CS_I32=0x03,
  CS_I64=0x04,
  CS_U8=0x05,
  CS_U16=0x06,
  CS_U32=0x07,
  CS_U64=0x09,
  CS_F32=0x0A,
  CS_F64=0x0B,
  CS_CHR=0x0C,
  CS_CHR16=0x0D,
  CS_CHR32=0x0E,
  CS_STRUCT=0x0F,
  CS_DAT=0x00,
  CS_PTR=0x20,
  CS_PPTR=0x30,
  CS_PPPTR=0x40,
  CS_TXTIO=0x50,
  CS_BINIO=0x60,
};

/*
   example usage:  
        chr_t* example_func(chr_t*,i16_t) {
           ... function body
        }

	chr_t* api_func(rval_t) {
	    static const cspec_t argspec = \
               {
	           2,
                   CS_CHR | CS_PTR,
                   { 
                    CS_CHR | CS_PTR,  
                    CS_I16,
                   },
               };

	    
        }
*/

#define specstorage(s) ((s) >> 4)
#define specdtype(s)   ((s) & 0x0F)
#define isio(s)        (specstorage(s) > 4)

enum {
  ERR_OK,
  ERR_ARGCO_EXCESS,
  ERR_ARGCO_DEFICIT,
  ERR_ARGLIST_TYPE,
  ERR_ARGTYPE,
};

// forward declarations


i32_t check_argco(i32_t,rval_t,rval_t*);
i32_t check_arg(u16_t,rval_t);
i32_t check_args(cspec_t, rval_t,rval_t*);

/* end fapi.h */
#endif
