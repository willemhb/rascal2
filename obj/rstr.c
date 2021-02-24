#include "../include/rstr.h"


MK_TYPE_PREDICATE(CVALUE,STRING,str)
MK_TYPE_PREDICATE(CVALUE,BYTES,bytes)
MK_SAFECAST_P(bytes_t*,bytes,addr)
MK_SAFECAST_P(rstr_t*,str,addr)


rstr_t* mk_str(const chr_t* s)
{
  size_t b = strsz(s);
  rstr_t* new;
  chr_t buf[b];
  strcpy(buf,s);
  new = vm_allocb(16,b);
  strcpy(new->chars,buf);
  new->type = STRING;
  new->cmeta = INLINED;
  new->size = b;
  return new;
}

bytes_t* mk_bstr(const uchr_t* b, size_t nb)
{
  bytes_t* new = vm_allocb(16,nb);
  new->type = BYTES;
  new->cmeta = INLINED;
  new->size = nb;
  memcpy(new->bytes,b,nb);

  return new;
}


inline size_t rstr_elcnt(val_t x)
{
  return u8strlen(ptr(rstr_t*,x)->chars);
}


rchr_t rstr_assocn(rstr_t* s, rint_t n)
{
  return nthu8(s->chars,n);
}


rint_t rstr_assocv(rstr_t* s, rchr_t c)
{
  chr_t* sc = s->chars;
  chr32_t curr; size_t inc; size_t cnt = 0;
  chr32_t cc = (chr32_t)c;

  while (*sc)
    {
      inc = incu8(&curr,sc);
      if (curr == cc)
	  return cnt;

      else
	  sc += inc;
    }

  return -1;
}


int32_t rstr_ord(val_t sx, val_t sy)
{
  chr_t* sxch = ptr(rstr_t*,sx)->chars;
  chr_t* sxcy = ptr(rstr_t*,sy)->chars;

  return u8strcmp(sxch,sxcy);
}

int32_t bstr_ord(val_t bx, val_t by)
{
  bytes_t* bxb = ptr(bytes_t*,bx), *byb = ptr(bytes_t*,by);
  size_t bxnb = bxb->size, bynb = byb->size;
  size_t maxcmp = min(bxnb,bynb);

  int32_t rslt = memcmp(bxb->bytes,byb->bytes,maxcmp);

  if (rslt < 0)
    return -1;

  else if (rslt > 0)
    return 1;

  else if (bxnb < bynb)
    return -1;

  else if (bxnb > bynb)
    return 1;

  else
    return 0;
}

hash_t rstr_hash(val_t s, uint32_t sd)
{
  chr_t* sch = ptr(rstr_t*,s)->chars;
  return hash_string(sch, sd);
}

hash_t bytes_hash(val_t b, uint32_t s)
{
  uchr_t* bbs = ptr(bytes_t*,b)->bytes;
  size_t  bnb = ptr(bytes_t*,b)->size;

  return hash_bytes(bbs,s,bnb);
}

void rstr_prn(val_t s, riostrm_t* f)
{
  rstr_t* sx = ptr(rstr_t*,s);
  fputc('"',f);
  fputs(sx->chars,f);
  fputc('"',f);
  return;
}

void bytes_prn(val_t b, riostrm_t* f)
{
  bytes_t* bx = ptr(bytes_t*,b);
  uchr_t* bxb = bx->bytes;
  fputs("b\"",f);

  size_t nb = bx->size;

  for (size_t i = 0; i < nb; i++)
    {
      
      fprintf(f,"%.3d",bxb[i]);

      if (nb > i - 1)
	fputwc(U' ',f);
    }

  fputwc(U'"',f);
  return;
}


cvspec_t BYTES_CVSPEC =
  {
    .el_cnum = CNUM_UINT8,
    .el_cptr = PTR_NONE,
    .el_sz   = 1,
  };


capi_t BYTES_CAPI =
  {
    .prn         = bytes_prn,
    .call        = NULL,
    .size        = cv_size,
    .elcnt       = cv_elcnt,
    .hash        = bytes_hash,
    .ord         = bytes_ord,
    .new         = NULL,
    .builtin_new = bytes_new,
    .init        = NULL,
    .relocate    = cv_relocate,
    .isalloc     = NULL,
  };


cvspec_t RSTR_CVSPEC =
  {
    .el_cnum = CNUM_UINT8,
    .el_cptr = PTR_NONE,
    .el_sz   = 1,
  };

capi_t RSTR_CAPI =
  {
    .prn         = rstr_prn,
    .call        = NULL,
    .size        = cv_size,
    .elcnt       = rstr_elcnt,
    .hash        = rstr_hash,
    .ord         = rstr_ord,
    .new         = NULL,
    .builtin_new = rstr_new,
    .init        = NULL,
    .relocate    = cv_relocate,
    .isalloc     = NULL,
  };

type_t RSTR_TYPE_OBJ =
  {
    .type              = DATATYPE,
    .cmeta             = STRING,
    .tp_tpkey          = STRING,
    .tp_ltag           = CVALUE,
    .tp_isalloc        = true,
    .tp_sizing         = WIDE_LEN,
    .tp_init_sz        = 16,
    .tp_base_sz        = 16,
    .tp_nfields        = 0,
    .tp_cvtable        = &RSTR_CVSPEC,
    .tp_capi           = &RSTR_CAPI,
    .name              = "str",
  };


type_t BYTES_TYPE_OBJ =
  {
    .type              = DATATYPE,
    .cmeta             = BYTES,
    .tp_tpkey          = BYTES,
    .tp_ltag           = CVALUE,
    .tp_isalloc        = true,
    .tp_sizing         = WIDE_LEN,
    .tp_init_sz        = 16,
    .tp_base_sz        = 16,
    .tp_nfields        = 0,
    .tp_cvtable        = &BYTES_CVSPEC,
    .tp_capi           = &BYTES_CAPI,
    .name              = "bytes",
  };
