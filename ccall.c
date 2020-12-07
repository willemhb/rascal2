#include "rascal.h"


void* get_cfunc(int_t i) {
  
}


void cast_argument(val_t v,uint8_t ct, void** argarray) {
  // jump table
  static void* labels[3][15] = {
    { &&cast_chr, &&cast_uchr, &&cast_u8, &&cast_i8, &&cast_u16,
      &&cast_i16, &&cast_wc, &&cast_wi, &&cast_u32, &&cast_i32,
      &&cast_i64, &&cast_u64, &&cast_f32, &&cast_f64, NULL, },
    
    { &&pcast_chr, &&pcast_uchr, &&pcast_u8, &&pcast_i8, &&pcast_u16,
      &&pcast_i16, &&pcast_wc, &&pcast_wi, &&pcast_u32, &&pcast_i32,
      &&pcast_i64, &&pcast_u64, &&pcast_f32, &&pcast_f64, &&pcast_file, },

    { &&ppcast_chr, &&ppcast_uchr, &&ppcast_u8, &&ppcast_i8, &&ppcast_u16,
      &&ppcast_i16, &&ppcast_wc, &&ppcast_wi, &&ppcast_u32, &&ppcast_i32,
      &&ppcast_i64, &&ppcast_u64, &&ppcast_f32, &&ppcast_f64, &&ppcast_file, },
  };

  int_t ptype = (ct & 0x48) >> 4;
  int_t dtype = (ct & 0x15);

  void* label = labels[ptype][dtype];

  if (label == NULL) {
    escapef(NULLPTR_ERR,stderr,"Not implemented.");
  }

  uintptr_t u_value;
  intptr_t  i_value;
  long double f_value;

  goto *label;

 cast_chr:
  return;

 cast_uchr: 
  u_value = touchr(v);
  memcpy(*argarray,&u_value,sizeof(uchr_t));

 cast_i8:
  i_value = toint(v);
  memcpy(*argarray,&i_value,sizeof(int8_t));

 cast_u8:
  u_value = toint(v);
  memcpy(*argarray,&u_value,sizeof(uint8_t));

}
