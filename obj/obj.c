#include "../include/rsp_core.h"

/* module internal declarations */
bool     check_btuple_index(uint32_t,uint32_t);
int32_t  get_btuple_index(uint32_t,uint32_t);


/* object constructors */
list_t* mk_list(size_t argc, val_t* args)
{
  if (argc == 0)
    return NULL;

  else
    {
      pair_t* curr;
      vm_allocc(argc,&curr,0);
      pair_t* out = curr;
      size_t i = 1;
      for (; i < argc - 1; i++, curr++)
    {
      pcar(curr) = args[i];
      pcdr(curr) = (val_t)(curr + 1);
    }

  pcar(curr) = args[i];
  pcdr(curr) = 0;

  return (list_t*)out;
  }
}

pair_t* mk_pair(val_t ca, val_t cd)
{
  pair_t* hd;

  vm_allocc(1,&hd,2,&ca,&cd);

  car_(hd) = ca;
  cdr_(hd) = cd;

return (pair_t*)hd;
}

tuple_t* mk_tuple(size_t n, uint8_t fl, uint8_t t)
{
  tuple_t* new;
  vm_allocw(1,n,&new,0);
  tuple_szdata(new) = 0;
  otag(new) = t;
  tuple_flags(new) = fl;
  // initialize all elements to NIL
  memset((uchr_t*)tuple_data(new),0,n*8);

  return new;
}

/* tuple sizing */
inline size_t _tuple_size(obj_t* t) { return 8 * _tuple_elcnt(t); }
inline size_t _btuple_elcnt(obj_t* t) { return __builtin_popcount(tuple_szdata(t)); }

size_t _tuple_elcnt(obj_t* t)
{
  if (otag(t) == BTUPLE)
    return _btuple_elcnt(t);

  else
    return tuple_szdata(t);
}

/* tuple accessors */
inline val_t* _tuple_data(obj_t* t)
{
  return (val_t*)(&(t->obj_data[0]));
}

bool check_btuple_index(uint32_t bmp, uint32_t ref)
{
  return bmp & (1<<ref);
}

int32_t tuple_check_idx(tuple_t* t, val_t** rslt, uint32_t idx)
{
  if (idx > 31)
    {
      *rslt = NULL;
      return -1;
    }

  val_t* cells = tuple_data(t);

  if (otag(t) == TUPLE)
    *rslt = cells + idx;

  else
    {
      int32_t bidx = get_btuple_index(tuple_szdata(t),idx);
      if (bidx == -1)
	{
	  *rslt = NULL;
	  return 3;
	}
      
      else
	{
	  *rslt = cells + bidx;
	}
    }
  
  if (isnil(**rslt))
    return 0;

  else if (islist(**rslt))
    return 1;

  else
    return 2;
}

int32_t get_btuple_index(uint32_t bmp, uint32_t ref)
{
  if (check_btuple_index(bmp,ref))
    return __builtin_popcount(bmp & ((1<<ref) - 1));

  else
    return -1;
}

val_t* tuple_ref(tuple_t* t, uint32_t idx)
{
  if (otag(t) == TUPLE)
    if (idx < tuple_szdata(t))
      return tuple_data(t) + idx;

    else
      return NULL;

  else
    {
      int32_t ref = get_btuple_index(tuple_szdata(t),idx);

      if (ref == -1)
	return NULL;

      else
	return tuple_data(t) + idx;
    }
}

/* gc functions */
val_t copy_pair(type_t* to, val_t v, uchr_t** tos)
{
  uchr_t* frm = ptr(uchr_t*,v);

  memcpy(*tos,frm,16);

  pair_t* new = (pair_t*)tos;
  *tos += 16;

  pcar(new) = gc_trace(pcar(new));
  pcdr(new) = gc_trace(pcdr(new));

  return vm_tag(to,(val_t)new);
}

val_t copy_tuple(type_t* to, val_t v, uchr_t** tos)
{
  uchr_t* frm = ptr(uchr_t*,v); tuple_t* tu_old = ptr(tuple_t*,v);
  tuple_t* tu_new = (tuple_t*)(*tos);
  size_t elcnt = tuple_elcnt(tu_old);
  size_t tsz = to->tp_base_size + (elcnt * 8);

  memcpy(*tos,frm,tsz);

  tsz = calc_mem_size(tsz);
  *tos += tsz;

  val_t* tuo_elm = tuple_data(tu_old);
  val_t* tun_elm = tuple_data(tu_new);

  for (size_t i = 0; i < elcnt;i++)
    tun_elm[i] = gc_trace(tuo_elm[i]);

  return vm_tag(to,(val_t)tu_new);
}

/* hashing functions */
hash32_t hash_pair(val_t v)
{
  val_t ca = ptr(pair_t*,v)->car, cd = ptr(pair_t*,v)->cdr;
  hash32_t chash = rsp_hash(ca);
  hash32_t dhash = rsp_hash(cd);
  hash32_t final = rehash(chash,dhash);
  return final;
}

hash32_t hash_tuple(val_t v)
{
  tuple_t* tx = ptr(tuple_t*,v);
  size_t telcnt = tuple_elcnt((obj_t*)tx);
  hash32_t acch = 0, ch;
  val_t* tel = tuple_data(tx);

  for (size_t i = 0; i < telcnt; i++)
    {
      ch = rsp_hash(tel[i]);
      acch = rehash(ch,acch);
    }

  return acch;
}

/* ordering functions */
int32_t ord_pair(val_t px, val_t py)
{
  pair_t* ppx = ptr(pair_t*,px), *ppy = ptr(pair_t*,py);
  
  if (!ppx)
    return ppy ? -1 : 0;

  else if (!ppy)
    return 1;

  else
    return rsp_ord(pcar(ppx),pcar(ppy)) ? : rsp_ord(pcdr(ppx),pcdr(ppy));
}

int32_t ord_tuple(val_t tx, val_t ty)
{
  tuple_t* ttx = ptr(tuple_t*,tx), *tty = ptr(tuple_t*,ty);
  val_t* xels = tuple_data(ttx), *yels = tuple_data(tty);
  size_t xlen = tuple_elcnt(tx), ylen = tuple_elcnt(ty);
  size_t maxcompares = min(xlen,ylen);

  for (size_t i = 0; i < maxcompares;i++)
    {
      int32_t result = rsp_ord(xels[i],yels[i]);

      if (result)
	return result;
    }

  if (xlen < ylen)
    return -1;

  else if (ylen < xlen)
    return 1;

  else
    return 0;
}

list_t* list_append(list_t** ls, val_t v)
{
  list_t* lsv = *ls;
  val_t lsbuf = (val_t)(lsv);
  pair_t* new;

  if (vm_allocc(1,&new,2,&lsbuf,&v))
    {
      lsv = ptr(list_t*,lsbuf);
      *ls = lsv;
    }

    while (*ls)
      ls = &pcdr(*ls);

    *ls = mk_list(1,&v);

    return (*ls);
}

/* printing */

void prn_list(val_t ls, iostrm_t* f) {
  fputwc('(',f);
  list_t* lsx = ecall(tolist,ls);

  while (lsx)
    {
      vm_print(pcar(lsx),f);

      if (pcdr(lsx))
	fputwc(' ',f);

      lsx = pcdr(lsx);
    }

  fputwc(')',f);
  return;
}

void prn_pair(val_t c, iostrm_t* f)
{
  pair_t* co = ecall(topair,c);
  fputwc('(',f);
  vm_print(pcar(co),f);
  fputs(" . ",f);
  vm_print(pcdr(co),f);
  fputwc(')',f);
  return;
}

void prn_tuple(val_t v, iostrm_t* f)
{
  tuple_t* tu = ecall(totuple,v);
  fputs("#(",f);
  val_t* el = tuple_data((obj_t*)tu);
  size_t elcnt = tuple_elcnt(v);

  for (size_t i = 0; i < elcnt; i++)
    {
      vm_print(el[i],f);
      
      if (elcnt - i > 1)
	fputwc(' ', f);
    }

  fputwc(')',f);
  return;
}

