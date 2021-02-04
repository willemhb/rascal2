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

inline uint32_t get_mask(hamt_lvl_t lvl)
{
  static const hamt_mask_t masks[7] =
    {
      0,
      L1_MASK, L2_MASK, L3_MASK,
      L4_MASK, L5_MASK, L6_MASK,
    };

  return masks[lvl];
}

inline uint8_t node_hash_chunk(node_t* nd, hash32_t h)
{
  return (h >> ((nd_level(nd) - 1) * 5)) & 0x1f;
}

inline uint8_t lvl_hash_chunk(hamt_lvl_t l, hash32_t h)
{
  return (h >> (l - 1) * 5) & 0x1f;
}

inline uint8_t get_local_index(node_t* nd, uint8_t lcl)
{
  if (bt_allcsz(nd) < 32)
    return popcount(bt_btmp(nd) & ((1 << lcl) - 1));

  else
    return lcl;
}

inline uint8_t get_prefix(hamt_lvl_t lvl, hash32_t h)
{
  if (lvl == 1)
    return (h >> 30);

  else
    return (h >> (lvl - 2) * 5) & 0x1f;
}

inline bool check_prefix(node_t* n, hash32_t h)
{
  return get_hash_chunk(n,h) == nd_prefix(n);
}

inline uint8_t find_split(hash32_t hx, hash32_t hy, hamt_lvl_t lvl)
{

  for (hamt_mask_t m = get_mask(lvl);lvl < 7;lvl++,m=get_mask(lvl))
    if ((hx & m) == (hy & m))
      break;

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
  tpkey_t ltk;
  uint8_t nkeys = 2, ncells;
  leaf_t* out;

  if (tp == SNODE)
    {
      ltk = SLEAF;
      ncells = 2;
      out = vm_allocw(16,ncells);
    }
  
  else if (tp == INODE)
    {
      ltk = ILEAF;
      ncells = 2;
      out = vm_cmalloc(32);
    }

  else
    {
      ltk = DLEAF;
      ncells = 4;
      out = vm_allocw(16,ncells);
    }

  memset(tuple_data(out),0,ncells*8);
  leaf_allocated(out) = nkeys;
  leaf_free(out) = 2;
  leaf_hash(out) = h;
  otpkey(out) = ltk;
  nd_level(out) = 7;
  
  return out;
}

node_t* mk_node(size_t nk, uint8_t lvl, hash32_t h, tpkey_t tp)
{
  node_t* out;
  lvl = min(lvl,6u);
  
  if (tp == INODE)
    out = mk_gl_btuple(nk,lvl,tp);

  else
    out = mk_btuple(nk,lvl,tp);

  nd_level(out) = lvl;
  nd_prefix(out) = get_prefix(lvl,h);

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

val_t* leaf_split(val_t* sl, leaf_t* lf, tpkey_t tp, hamt_lvl_t l, val_t k, hash32_t h)
{
  node_t* new_nd = mk_node(2,l,h,tp);
  leaf_t* new_lf = mk_leaf(h,tp);

  tuple_data(new_lf)[0] = k;
  leaf_free(new_lf) = 1;

  if (h < leaf_hash(lf))
    {
      tuple_data(new_nd)[0] = tag_p(new_lf,OBJ);
      tuple_data(new_nd)[1] = tag_p(lf,OBJ);
    }

  else
    {
      tuple_data(new_nd)[0] = tag_p(lf,OBJ);
      tuple_data(new_nd)[1] = tag_p(new_lf,OBJ);
    }

  bt_btmp(new_nd) = get_hash_chunk(l,h) | get_hash_chunk(leaf_hash(lf),h);

  *sl = tag_p(new_nd,OBJ);

  return tuple_data(new_lf);
}

val_t* node_split(val_t* sl, node_t* n, tpkey_t tp, hamt_lvl_t l, val_t k, hash32_t h)
{
  // TODO: check that this produces the correct prefix
  node_t* new_nd = mk_node(2,l,h,tp);
  leaf_t* new_lf = mk_leaf(h,tp);

  tuple_data(new_lf)[0] = k;
  leaf_free(new_lf) = 1;

  if (nd_prefix(new_nd) < nd_prefix(n))
    {
      tuple_data(new_nd)[0] = tag_p(new_nd,OBJ);
      tuple_data(new_nd)[1] = tag_p(n,OBJ);
    }

  else
    {
      tuple_data(new_nd)[0] = tag_p(n,OBJ);
      tuple_data(new_nd)[1] = tag_p(new_nd,OBJ);
    }

  bt_btmp(new_nd) = nd_prefix(new_nd) | nd_prefix(n);

  *sl = tag_p(new_nd,OBJ);

  return tuple_data(new_lf);
}

val_t* hamt_insert(val_t* n, val_t k, hash32_t h, tpkey_t tk)
{

  leaf_t* lf;
  node_t* n_nd, *nd = ptr(node_t*,*n);
  

  if (!nd)
    {
      n_nd = mk_node(2,1,h,tk);
      lf = mk_leaf(h,tk);

      if (tk != INODE)
	tuple_data(lf)[0] = k;
	
      return tuple_data(lf);
    }

  else
    {
      hash32_t l_hsh;
      uint8_t curr_lvl, new_lvl, lcl_idx, chnk;
      val_t occ, *nd_els;

      while (true)
	{
	  curr_lvl = nd_level(nd);
	  chnk = get_hash_chunk(nd,h);
	  lcl_idx = get_local_index(nd,chnk);
	  nd_els = tuple_data(n);

	  if (bt_btmp(nd) & chnk)
	    {
	      occ = nd_els[lcl_idx];

	      if (isleaf(occ))
		{
		  lf = ptr(leaf_t*,occ);
		  l_hsh = leaf_hash(lf);
		  new_lvl = find_split(h,l_hsh,curr_lvl);

		  if (new_lvl == 7)
		    {
		      return leaf_insert(lf,nd_els+lcl_idx,k);
		    }

		  else
		    return leaf_split(nd_els+lcl_idx,lf,otpkey(nd),new_lvl,k,h);
		}

	      else
		{
		  n_nd = ptr(node_t*,occ);

		  if (check_prefix(n_nd,h))
		    {
		      nd = n_nd;
		      n = nd_els + lcl_idx;
		      continue;
		    }

		  else
		    return node_split(nd_els+lcl_idx,n_nd,otpkey(nd),nd_level(nd)+1,k,h);
		}
	    }

	  else
	    return node_insert(nd,n,k,h);
	}
    }
}

val_t* node_insert(node_t* n, val_t* v, val_t k, hash32_t h)
{
  if (!node_free(n))
    {
      n = node_realloc(n);
      *v = tag_p(n,OBJ);
    }

  uint8_t chnk = get_hash_chunk(n,h);
  uint8_t idx = (1 << chnk);
  leaf_t* nl = mk_leaf(tpkey(n),h);

  if (tpkey(n) != INODE)
    tuple_data(nl)[0] = k;

  val_t* nels = tuple_data(n);

  if (__builtin_ctz(idx) < __builtin_ctz(bt_btmp(n)))
    {
      nels[node_nkeys(n)] = tag_p(nl,OBJ);
      bt_btmp(n) |= idx;
      return tuple_data(nl);
    }

  bt_btmp(n) |= idx;
  node_t* cn;
  leaf_t* cl;
  val_t cv;

  for (size_t i = node_nkeys(n);; i--)
    {
      cv = nels[i-1];
      nels[i] = cv;

      if (isnode(cv))
	{
	  cn = ptr(node_t*,cv);
	  if (nd_prefix(cn) > chnk)
	    {
	      nels[i-1] = tag_p(nl,OBJ);
	      break;
	    }
	}

      else if (isleaf(cv))
	{
	  cl = ptr(leaf_t*,cv);
	  if (leaf_hash(cl) > h)
	    {
	      nels[i-1] = tag_p(nl,OBJ);
	      break;
	    }
	}
    }

  return tuple_data(nl);
}


val_t* leaf_insert(leaf_t* l, val_t* v, val_t k)
{

  size_t i;
  if (isileaf(l))
    {
      val_t* atms = tuple_data(l);
      chr_t* s = ptr(chr_t*,k);

      for (i = 0; i < leaf_nkeys(l);i++)
	{
	  atom_t* curr = ptr(atom_t*,atms[i]);
	  if (!strcmp(s,atom_name(curr)))
	    return atms + i;
	}

      if (!leaf_free(l))
	{
	  l = leaf_realloc(l);
	  *v = tag_p(l,OBJ);
	  atms = tuple_data(l);
	}
      return atms + i;
    }

  else if (isdleaf(l))
    {
      val_t* keys = tuple_data(l);

      for (i = 0; i < leaf_nkeys(l) * 2; i+= 2)
	{
	  val_t curr = keys[i];
	  if (!compare(k,curr))
	    return keys + i;
	}

      if (!leaf_free(l))
	{
	  l = leaf_realloc(l);
	  *v = tag_p(l,OBJ);
	  keys = tuple_data(l);
	}

      return keys + i;
    }

  else
    {
      val_t* keys = tuple_data(l);

      for (i = 0; i < leaf_nkeys(l); i++)
	{
	  val_t curr = keys[i];
	  if (!compare(k,curr))
	    return keys + i;
	}

      if (!leaf_free(l))
	{
	  l = leaf_realloc(l);
	  *v = tag_p(l,OBJ);
	  keys = tuple_data(l);
	}

      return keys + i;
    }
}


val_t* hamt_search(node_t* nd, val_t k, hash32_t h)
{
  for (;nd;)
    {
      switch (otpkey(nd))
	{
	case INODE: case DNODE: case SNODE:
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
