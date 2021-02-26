#ifndef table_h
#define table_h

#include "rascal.h"
#include "values.h"
#include "obj.h"
#include "mem.h"
#include "hamt.h"
#include "error.h"
#include "describe.h"

typedef struct table_t
{
  tpkey_t  type;
  uint32_t cmeta;
  uint64_t nkeys;
  val_t    keys;
} table_t;

typedef enum
  {
    SM_INTERNED = 0x01,
    SM_GENSYM   = 0x02,
    SM_RESERVED = 0x04,
    SM_KEYWORD  = 0x08,
  } sm_flags_t;

table_t*  mk_table(size_t);
table_t*  tb_prn(table_t*);
val_t     tb_relocate(table_t*);
val_t     tb_putkey(table_t*,val_t,val_t);
val_t     tb_getkey(table_t*,val_t);
val_t     tb_rmvkey(table_t*,val_t);
table_t*  mk_symtab(size_t);
table_t*  symtb_intern(table_t*,chr_t*);
symbol_t* mk_symbol(chr_t*,uint32_t);

#endif
