#include "../include/rsp_core.h"


str_t* mk_str(chr_t* s)
{
  size_t b = strsz(s);
  str_t* new;

  if (in_heap((val_t)s,RAM,HEAPSIZE))
    {
      chr_t buf[b];
      strcpy(buf,s);
      vm_allocb(1,b,&new,0);
      strcpy(new->str_chrs,buf);
    }

  else
    {
      vm_allocb(1,b,&new,0);
      strcpy(new->str_chrs,s);
    }

  otag(new) = STRING;
  return new;
}

bstr_t* mk_bstr(size_t nb, uchr_t* b)
{
  bstr_t* new;

  if (in_heap(b,RAM,HEAPSIZE))
    {
      uchr_t buf[nb];
      memcpy(buf,b,nb);
      vm_allocb(8,nb,&new,0);
      memcpy(new->bytes,buf,nb);
    }

  else
    {
      vm_allocb(8,nb,&new,0);
      memcpy(new->bytes,b,nb);
    }

  bstr_nbytes(new) = nb;
  otag(new)        = BSTRING;

  return new;
}

atom_t* new_atom(chr_t* sn, size_t ssz, uint16_t fl, hash32_t h)
{
  atom_t* out;

  if (!ssz)
    ssz = strsz(sn);

  if (!h)
    h = hash_string(sn);

  chr_t buf[ssz];
  strcpy(buf,sn);
  vm_allocb(1,ssz,&out,0);

  atm_hash(out) = h;
  atm_flags(out) = fl;
  strcpy(atm_name(out),sn);
  otag(out) = ATOM;

  return out;
}

size_t str_size(obj_t* s)
{
  chr_t* schrs = (chr_t*)(&((s)->obj_data[0]));
  return strsz(schrs);
}

size_t bstr_size(obj_t* b)
{
  return ptr(bstr_t*,b)->nbytes;
}

val_t copy_str(type_t* to, val_t s, uchr_t** tos)
{
  uchr_t* frm = ptr(uchr_t*,s);
  size_t tot = to->tp_base_size + str_size((obj_t*)frm);
  memcpy(*tos,frm,tot);
  *tos += calc_mem_size(tot);

  return vm_tag(to,s);
}

val_t copy_bstr(type_t* to, val_t b, uchr_t** tos)
{
  uchr_t* frm = ptr(uchr_t*,b);
  size_t tot = to->tp_base_size + bstr_size((obj_t*)b);
  memcpy(*tos,frm,tot);
  *tos += calc_mem_size(tot);

  return vm_tag(to,b);
}

int32_t ord_str(val_t sx, val_t sy)
{
  chr_t* sxch = ptr(str_t*,sx)->str_chrs;
  chr_t* sxcy = ptr(str_t*,sy)->str_chrs;

  return u8strcmp(sxch,sxcy);
}

int32_t ord_bstr(val_t bx, val_t by)
{
  bstr_t* bxb = ptr(bstr_t*,bx), *byb = ptr(bstr_t*,by);
  size_t bxnb = bstr_nbytes(bxb), bynb = bstr_nbytes(byb);
  size_t maxcmp = min(bxnb,bynb);

  int32_t rslt = memcmp(bstr_bytes(bxb),bstr_bytes(byb),maxcmp);

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

hash32_t hash_str(val_t s)
{
  chr_t* sch = ptr(str_t*,s)->str_chrs;
  return hash_string(sch);
}

hash32_t hash_bstr(val_t b)
{
  uchr_t* bbs = ptr(bstr_t*,b)->bytes;
  size_t  bnb = ptr(bstr_t*,b)->nbytes;

  return hash_bytes(bbs,bnb);
}

hash32_t hash_atom(val_t a)
{
  return atm_hash(ptr(atom_t*,a));
}

void prn_str(val_t s, iostrm_t* f)
{
  str_t* sx = ptr(str_t*,s);
  fputc('"',f);
  fputs(str_chars(sx),f);
  fputc('"',f);
  return;
}

void prn_bstr(val_t b, iostrm_t* f)
{
  bstr_t* bx = ptr(bstr_t*,b);
  uchr_t* bxb = bstr_bytes(bx);
  fputs("b\"",f);

  size_t nb = bstr_nbytes(bx);

  for (size_t i = 0; i < nb; i++)
    {
      
      fprintf(f,"%.3d",bxb[i]);

      if (nb > i - 1)
	fputwc(U' ',f);
    }

  fputwc(U'"',f);
  return;
}

void prn_atom(val_t a, iostrm_t* f)
{
  atom_t* ax = ecall(toatom,a);
  fputs(atm_name(ax),f);
  return;
}

chr_t* strval(val_t v)
{
  if (isatom(v))
    return atm_name(ptr(atom_t*,v));

  else if (isstr(v))
    return str_chars(ptr(str_t*,v));

  else
    {
      rsp_raise(TYPE_ERR);
      return NULL;
    }
}
