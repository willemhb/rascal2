#include "../include/rsp_core.h"

typedef enum
  {
    INDEX_OUT_OF_RANGE = -1,
    INDEX_FREE         =  0,
    INDEX_BOUND_KEYS   =  1,
    INDEX_BOUND_TABLE  =  2,
    INDEX_FREE_BTUPLE  =  3,
  } tb_lookup_rslt_t;

val_t*   tbnode_split(val_t*,list_t*,val_t,hash32_t,val_t,hash32_t,uint8_t);
val_t*   tbnode_addkey(table_t*,tuple_t**,val_t,hash32_t);
val_t*   tbnode_realloc(tuple_t**,uint32_t);

hash32_t extended_hash(hash32_t h, uint8_t lvl)
{
  if (lvl >= 6)
      for (uint8_t tmp = 6; tmp <= lvl; tmp += 6)
	h = rehash(h,tmp);

  return h;
}

uint32_t local_idx(hash32_t h, uint8_t lvl)
{
  if (lvl > 6)
    lvl &= 6;
  return h >> (lvl * 5) & 0x1f;
}

val_t    set_getkey(val_t ks)
{
  return pcar(ptr(list_t*,ks));
}

val_t    dict_getkey(val_t ks)
{
  return pcar(ptr(pair_t*,pcar(ptr(list_t*,ks))));
}

/* internal definitions */

table_t* mk_table(uint16_t flags, uint8_t tp, bool global)
{
  table_t* new; tuple_t* lvl_0; pair_t* ord_k;

  if (global)
    {
      new = vm_cmalloc(sizeof(table_t));
    }

  else
    {
      // run the gc if there's not enough space to allocate the dict and one 32-node level
      vm_allocw(1,39,NULL,1,0);
      vm_allocw(1,3,&new,1,0);
    }

  lvl_0 = mk_tuple(32,0,TUPLE);
  ord_k = mk_pair(R_NIL,R_NIL);
  tb_levels(new) = 1;
  tb_nkeys(new) = 0;
  tb_mapping(new) = tag_p(lvl_0,OBJ);
  tb_ordkeys(new) = tag_p(ord_k,OBJ);
  tb_flags(new) = flags;
  otag(new) = tp;

  return new;
}

tuple_t* mk_tbnode(size_t nk, uint8_t lvl)
{
  if (nk < 16)
    return mk_tuple(nk,lvl,BTUPLE);

  else
    return mk_tuple(32,lvl,TUPLE);
}


val_t* tbnode_split(val_t* cell, list_t* orig, val_t ok, hash32_t ok_h, val_t nk, hash32_t nkh, uint8_t lvl)
{
  if (!orig)
    { orig = ptr(list_t*,*cell);
      ok = ispair(pcar(orig)) ? ptr(pair_t*,pcar(orig))->car : pcar(orig);
      ok_h = extended_hash(rsp_hash(ok),lvl);
    }

  hash32_t enkh = extended_hash(nkh,lvl);

  uint32_t oh_idx = local_idx(ok_h,lvl);
  uint32_t nh_idx = local_idx(enkh,lvl);
  tuple_t* new; val_t* out;

  if (oh_idx == nh_idx)
    {
      new = mk_tbnode(1,lvl);
      out = tbnode_split(tuple_data(new),orig,ok,ok_h,nk,nkh,lvl+1);
      tuple_szdata(new) = 1 << oh_idx;
      *cell = tag_p(new,OBJ);
    }

  else
    {
      new = mk_tbnode(2,lvl);
      tuple_szdata(new) = (1 << oh_idx) | (1 << nh_idx);
      *cell = tag_p(new,OBJ);

      if (oh_idx < nh_idx)
	{
	  tuple_data(new)[0] = tag_p(orig,LIST);
	  out = tuple_data(new) + 1;
	}

      else
	{
	  tuple_data(new)[1] = tag_p(orig,LIST);
	  out = tuple_data(new);
	}
    }

  return out;
}

val_t* tbnode_realloc(tuple_t** nd, uint32_t lidx)
{
  tuple_t* orig = *nd;
  uint8_t obmp = tuple_szdata(orig), osz = tuple_size((obj_t*)orig);
  tuple_t* new = mk_tbnode(osz+1, tbnd_level(orig));
  val_t* old_els = tuple_data(orig);
  val_t* new_els = tuple_data(new);
  val_t* out;
  uint8_t bmidx = (1 << lidx);

  if (otag(new) == BTUPLE)
    {
      tuple_szdata(new) = obmp | bmidx;
      for (uint8_t curr = 1, i = 0;i < osz + 1; curr <<= 1)
	{
	  if (obmp & curr)
	    {
	      new_els[i] = old_els[i];
	      i++;
	    }

	  else if (bmidx & curr)
	    {
	      out = new_els + i;
	      i++;
	    }
	}
    }

  else
    {
      for (uint8_t curr = 1, i = 0; i < 32; i++, curr <<= 1)
	{
	  if (obmp & curr)
	    new_els[i] = old_els[i];

	  else if (bmidx & curr)
	    out = new_els + i;

	  else
	    continue;
	}
    }

  *nd = new;
  car_(orig) = R_FPTR;
  cdr_(orig) = tag_p(new,OBJ);

  return out;
}

list_t* tb_bindings(table_t* d)
{
  pair_t* d_ok = ptr(pair_t*,tb_ordkeys(d));
  return ptr(list_t*,pcar(d_ok));
}

list_t* tb_bindings_tail(dict_t* d)
{
  pair_t* d_ok = ptr(pair_t*,tb_ordkeys(d));
  return ptr(list_t*,pcar(d_ok));
}

list_t* tb_addkey_seq(table_t* tb, val_t k)
{
  pair_t* bindings = ptr(pair_t*,tb_ordkeys(tb));
  pair_t* new_b = mk_pair(k,R_NIL);
  list_t* out;
  
  if (tb_nkeys(tb))
    {
      list_t* curr = ptr(list_t*,pcdr(bindings));
      out = list_append(&curr,tag_p(new_b,OBJ));
    }

  else
    {
      out = (list_t*)mk_pair(tag_p(new_b,OBJ),R_NIL);
      pcar(bindings) = tag_p(out,LIST);
      pcdr(bindings) = tag_p(out,LIST);
    }

  tb_nkeys(tb)++;
  return out;
}

val_t* tbnode_addkey(table_t* tb, tuple_t** nd, val_t k, hash32_t h)
{
  uint8_t lvl = tbnd_level(*nd); hash32_t sh = extended_hash(h,lvl);
  uint32_t loc_idx = local_idx(sh,lvl);
  val_t* rslt; tuple_t* nb;

  switch (tuple_check_idx(*nd,&rslt,loc_idx))
    {
    case INDEX_OUT_OF_RANGE:
      rsp_raise(BOUNDS_ERR);
      return NULL;

    case INDEX_FREE:
      return rslt;

    case INDEX_BOUND_KEYS:
      return tbnode_split(rslt,NULL,R_NIL,0,tbnd_level(*nd)+1,k,h);

    case INDEX_BOUND_TABLE:
      nb = ptr(tuple_t*,*rslt);
      rslt = tbnode_addkey(tb,&nb,k,h);
      return rslt;

    case INDEX_FREE_BTUPLE:
      rslt = tbnode_realloc(nd,loc_idx);
      return rslt;

    default:
      return rslt;
    }
}

list_t* tb_addkey(table_t* tb, val_t k)
{
  hash32_t h = rsp_hash(k);
  tuple_t* dmp = ptr(tuple_t*,tb_mapping(tb));
  val_t* new = tbnode_addkey(tb,&dmp,k,h);

  if (new == NULL)
    return NULL;

  else if (*new == R_NIL)
    {
      list_t* out = tb_addkey_seq(tb,k);
      *new = tag_p(out,LIST);
      tb_mapping(tb) = tag_p(dmp,OBJ);
      return out;
    }

  else
    return ptr(list_t*,*new);
}

list_t* tb_lookup(table_t* d, val_t v)
{
  tuple_t* dmp = ptr(tuple_t*,tb_mapping(d));
  hash32_t h = rsp_hash(v);
  uint32_t idx = h, loc_idx;
  val_t* rslt;

  while (dmp)
    {
      if (!idx) // if our hashkey runs out, we extend the hash using the current level of the
	        // tree
	idx = rehash(h,tbnd_level(dmp));

      loc_idx = local_idx(h,tbnd_level(dmp));
      rslt = tuple_ref(dmp,loc_idx);

      if (!rslt)
	return NULL;

      else if (islist(*rslt))
	return ptr(list_t*,rslt);

      else
	dmp = ptr(tuple_t*,*rslt);
    }

  return NULL;
}


val_t copy_table(type_t* to, val_t v)
{
  uchr_t* frm = ptr(uchr_t*,v); dict_t* old_d = ptr(table_t*,v);
  dict_t* new_d = ptr(table_t*,FREE);
  size_t dsz = to->tp_base_size;
  memcpy(FREE,frm,dsz);
  FREE += calc_mem_size(dsz);

  tb_mapping(new_d) = gc_trace(tb_mapping(old_d));
  tb_ordkeys(new_d) = gc_trace(tb_ordkeys(old_d));
  return vm_tag(to,(val_t)new_d);
}

void prn_table(val_t v, iostrm_t* f, const chr_t* dlm)
{
  table_t* t = ptr(table_t*,v);
  fputs(dlm,f);

  pair_t* d_ok = ecall(topair,tb_ordkeys(t));
  list_t* d_ok_start = ptr(list_t*,pcar(d_ok));

  while (d_ok_start)
    {
      vm_print(pcar(d_ok_start),f);
      d_ok_start = pcdr(d_ok_start);

      if (d_ok_start)
	fputwc(' ',f);
    }

  fputs("}",f);
}

void prn_dict(val_t v, iostrm_t* f)
{
  prn_table(v,f,"#d{");
  return;
}



void prn_set(val_t v, iostrm_t* f)
{
  prn_table(v,f,"#{");
  return;
}

atom_t* mk_atom(chr_t* sn, uint16_t fl)
{
  static uint32_t GENSYM_COUNTER = 0;
  size_t ssz = strsz(sn);

  if (fl & GENSYM)
    {
      chr_t gs_buf[20+ssz];
      sprintf(gs_buf,"__gs__%u__%s",GENSYM_COUNTER,sn);
      GENSYM_COUNTER++;
      hash32_t h = hash_string(gs_buf);
      if (fl & INTERNED)
	return intern_string(gs_buf,h,fl);

      else
	return new_atom(gs_buf,ssz,h,fl);
    }

  else
    {
      hash32_t h = hash_string(sn);

      if (fl & INTERNED)
	return intern_string(sn,h,fl);

      else
	return new_atom(sn,ssz,h,fl);
    }
}

atom_t* intern_string(chr_t* sn, hash32_t h, uint16_t fl)
{
  
  set_t* st = ecall(toset,R_SYMTAB);
  size_t nk = tb_nkeys(st);
  size_t ssz = strsz(sn);
  
  atom_t* tmp = vm_cmalloc(8 + ssz);
  atm_hash(tmp) = h;
  strcpy(atm_name(tmp),sn);
  atm_flags(tmp) = fl;
  otag(tmp) = ATOM;
  list_t* new = tb_addkey(st,tag_p(tmp,OBJ));

  if (tb_nkeys(st) == nk)
    {
      vm_cfree(tmp);
      return ptr(atom_t*,pcar(new));
    }

  else
    return tmp;
}
