#include "../include/rsp_core.h"
#include "hamt.c"

/* internal definitions */

table_t* mk_table(size_t nk, tpkey_t tp)
{
  table_t* new; tuple_t* lvl_1; pair_t* ord_k;

  lvl_1 = mk_tuple(32,0,TUPLE);
  ord_k = mk_pair(R_NIL,R_NIL);
  tb_levels(new) = 1;
  tb_nkeys(new) = 0;
  tb_mapping(new) = tag_p(lvl_0,OBJ);
  tb_ordkeys(new) = tag_p(ord_k,OBJ);
  tb_flags(new) = flags;
  otag(new) = tp;

  return new;
}

table_t* 

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
