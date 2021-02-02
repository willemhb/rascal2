#include "hamt.h"

TPKEY_RANGE_PREDICATE(node,SNODE,INODE)
TPKEY_RANGE_PREDICATE(leaf,SLEAF,ILEAF)
TPKEY_PREDICATE(snode,SNODE)
TPKEY_PREDICATE(inode,INODE)
TPKEY_PREDICATE(dnode,DNODE)
TPKEY_PREDICATE(sleaf,SLEAF)
TPKEY_PREDICATE(ileaf,ILEAF)
TPKEY_PREDICATE(dleaf,DLEAF)
  
inline uint32_t leaf_nkeys(leaf_t* lf)
{
  return leaf_allocated(lf) - leaf_free(lf);
}

inline uint32_t node_nkeys(node_t* nd)
{
  return popcount(bt_btmp(nd));
}

inline uint32_t node_free(node_t* nd)
{
  return bt_allcsz(nd) - popcount(bt_btmp(nd));
}

inline uint8_t get_hash_chunk(node_t* nd, hash32_t h)
{
  return (h >> ((nd_level(nd) - 1) * 5)) & 0x1f;
}

inline uint8_t get_local_index(node_t* nd, uint8_t lcl)
{
  if (bt_allcsz(nd) < 32)
    return popcount(bt_btmp(nd) & ((1 << lcl) - 1));

  else
    return lcl;
}

inline uint8_t find_split(hash32_t hx, hash32_t hy)
{
  uint8_t lvl = 1; uint64_t mask = 0x1ful;

  for (;lvl < 8 && (hx & mask) != (hy & mask);mask <<= (lvl * 5), lvl++)
    continue;

  return lvl;
}

inline node_idx_tp_t check_index(node_t* n, hash32_t h)
{
  uint8_t chnk = get_hash_chunk(n,h);

  if (bt_btmp(n) & chnk)
    {
      val_t occ = tuple_data(n)[get_local_index(n,chnk)];

      if (isleaf(occ))
	return LEAF_BOUND;

      else
	return NODE_BOUND;
    }

  else
    return INDEX_FREE;
}

val_t get_nd_index(node_t* n, hash32_t h)
{
  uint8_t lcl_idx = get_local_index(n,h);
  return tuple_data(n)[lcl_idx];
}

leaf_t* mk_leaf(hash32_t h, tpkey_t tp)
{
  uint8_t nkeys = 2;
  uint8_t ncells = tp == DLEAF ? 4 : 2;
  leaf_t* out = tp == ILEAF ? vm_cmalloc(16 + nkeys * 8) : vm_allocw(16,ncells);
  memset(tuple_data(out),0,ncells*8);
  leaf_allocated(out) = 2;
  leaf_free(out) = 2;
  leaf_hash(out) = h;
  otpkey(out) = ILEAF;
  nd_level(out) = 8;
  
  return out;
}

node_t* mk_node(size_t nk, uint8_t lvl, tpkey_t tp)
{
  node_t* out;

  if (lvl == 7)
    nk = min(nk,4u);
  
  if (tp == INODE)
    out = mk_gl_btuple(nk,lvl,tp);

  else
    out = mk_btuple(nk,lvl,tp);
  
  nd_level(out) = lvl;

  return out;
}

node_t* node_realloc(node_t* orig)
{
  uint8_t old_els = bt_allcsz(orig);
  uint8_t new_els = min(old_els * 2,32);
  uint64_t new_sz = 16 + new_els * 8;
  node_t* new_nd;

  if (isinode(orig))
    new_nd = vm_crealloc(orig,new_sz,false);

  else
    new_nd = vm_reallocw(tag_p(orig,OBJ),new_els);

  return new_nd;
}

leaf_t* leaf_realloc(leaf_t* orig)
{
  uint32_t old_allc = leaf_allocated(orig);
  uint32_t new_allc = old_allc < 8 ? old_allc * 2 : old_allc + (old_allc / 4);
  tpkey_t tk = otpkey(orig);
  leaf_t* new;

  if (tk == ILEAF)
    new = vm_crealloc(orig,16 + new_allc * 8,false);

  else if (tk == SLEAF)
    new = vm_reallocw(tag_p(orig,OBJ),new_allc);

  else
    new = vm_reallocw(tag_p(orig,OBJ),new_allc*2);

  return new;
}

val_t* node_insert(node_t* n, val_t k, hash32_t h)
{
  
  uint8_t chnk = get_hash_chunk(n,h);
  uint8_t lcl_idx = get_local_index(n,chnk);
  val_t*  nd_els = tuple_data(n); val_t occ;

  if (bt_btmp(n) & chnk)
    {
      occ = nd_els[lcl_idx];

      if (isleaf(occ))
	{
	  if (leaf_hash(ptr(leaf_t*,occ)) == h)
	    return leaf_insert(ptr(leaf_t*,occ),k,h);

	  else
	    {
	      
	    }
	}
    }
  
  else if (!node_free(n))
    n = node_realloc(n);

}

val_t* hamt_search(hamt_t* nd, val_t k, hash32_t h)
{
  for (;nd;)
    {
      switch (otpkey(nd))
	{
	case NODE:
	  if (check_index((node_t*)nd,h))
	    nd = ptr(hamt_t*,get_nd_index((node_t*)nd,h));

	  else
	    nd = NULL;

	  break;

	case SLEAF: case DLEAF:
	  return leaf_search((leaf_t*)nd,k,h);

	case ILEAF:
	  return ileaf_search((leaf_t*)nd,(chr_t*)k,h);

	default:
	  nd = NULL;
	  break;
	}
    }

  return NULL;
}

val_t* leaf_search(leaf_t* lf, val_t k, hash32_t h)
{
  if (leaf_hash(lf) == h)
    {
      val_t* keys = tuple_data(lf);
      uint8_t skip = otpkey(lf) == DLEAF ? 2 : 1;
      for (size_t i = 0; i < leaf_nkeys(lf);i += skip)
	{
	  if (!rsp_ord(k,keys[i]))
	    return keys + i;
	}

      if (leaf_free(lf))
	return keys + leaf_nkeys(lf);
    }
    
  return NULL;
}

val_t* ileaf_search(leaf_t* lf, chr_t* s, hash32_t h)
{
  if (leaf_hash(lf) == h)
    {
      val_t* keys = tuple_data(lf);
      uint32_t nk = leaf_nkeys(lf);
      for (uint32_t i = 0; i < nk;i++)
	{
	  atom_t* curr = ptr(atom_t*,keys[i]);
	  if (!strcmp(s,atom_name(curr)))
	    return keys + i;
	}

      if (leaf_free(lf))
	return keys + nk;
    }

  return NULL;
}

val_t* hamt_addkey(hamt_t** nd, val_t k, hash32_t h)
{
  for (;*nd;)
      {
      switch (otpkey(*nd))
	{
	case NODE:
	  switch (check_index(*nd,h))
	    {
	    case INDEX_FREE:
	      
	    }

	case SLEAF: case DLEAF:
	  return leaf_search((leaf_t*)nd,k,h);

	case ILEAF:
	  return ileaf_search((leaf_t*)nd,(chr_t*)k,h);

	default:
	  nd = NULL;
	  break;
	}
    }

  return NULL;
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
