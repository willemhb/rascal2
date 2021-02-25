#include "../include/hamt.h"


MK_TYPE_PREDICATE(BVECTOR,OBJECT,bvec)
MK_SAFECAST_P(bvec_t*,bvec,addr)


inline uint32_t popcnt(uint32_t b)
{
  return __builtin_popcount(b);
}


inline uint32_t get_mask(hamt_lvl_t lvl)
{
  static const hamt_mask_t masks[7] =
    {
      L1_MASK, L2_MASK, L3_MASK,
      L4_MASK, L5_MASK, L6_MASK,
      L7_MASK,
    };

  return masks[lvl];
}

inline uint8_t get_bm_index(uint32_t bmp, uint8_t idx)
{
  return popcnt(bmp & ((1 << idx) - 1));
}

bvec_t*  mk_bvec(uint8_t lvl, size_t sz, uint16_t flags)
{
  assert(sz <= 32, BOUNDS_ERR);
  bvec_t* out;

  switch (lvl)
    {
    case 1:
      out = (flags & GLOBAL) ? vm_cmalloc((1 + 32)*8) : vm_allocw(8,32);
      break;

    case 2 ... 6:
      out = (flags & GLOBAL) ? vm_cmalloc((1 + sz) * 8) : vm_allocw(8,sz);
      break;

    case 7: default:
      out = (flags & GLOBAL) ? vm_cmalloc(40) : vm_allocw(8,4);
      break;
    }

  out->type       = BVECTOR;
  out->bv_bmap    = 0;
  return out;
}

leaf_t* mk_leaf(hash_t h, val_t key, uint16_t flags)
{
  leaf_t* out;

  if (flags & BINDINGS)
    {
      dleaf_t* nw_dlf  = flags & GLOBAL ? vm_cmalloc(32) : vm_allocw(8,3);
      nw_dlf->type   = TBDLEAF;
      nw_dlf->value  = R_UNBOUND;
      nw_dlf->next   = NULL;
      out = (leaf_t*)nw_dlf;
    }

  else
    {
      sleaf_t* nw_slf  = flags & GLOBAL ? vm_cmalloc(32): vm_allocw(8,3);
      list_t* slf_keys = (list_t*)((void*)nw_slf + 16);
      slf_keys->car    = key;
      slf_keys->cdr    = NULL;
      nw_slf->type     = TBSLEAF;
      nw_slf->keys     = slf_keys;
      out              = (leaf_t*)nw_slf;
    }

  return out;
}


bvec_t*  cp_bvec(bvec_t* frm, int32_t grow)
{
  int32_t frmsz = popcnt(frm->bv_bmap);
  bvec_t* out = vm_allocw(8, frmsz + grow);
  memcpy((void*)out,(void*)frm,16+frmsz);
  return out;
}

size_t bvec_elcnt(val_t b)
{
  bvec_t* bv = tobvec(b);
  return popcnt(bv->bv_bmap);
}

size_t bvec_sizeof(type_t* to, val_t b)
{
  return to->tp_base_sz + bvec_elcnt(b) * 8;
}

val_t* bvec_ref(bvec_t* bv, uint8_t idx)
{
  assert(idx <= 32, BOUNDS_ERR);

  if (bv->bv_bmap & (1 << idx))
    return bv->bv_elements + get_bm_index(bv->bv_bmap,idx);

  else
    return NULL;
}

static obj_t* leaf_search(leaf_t* lf, val_t k)
{
  if (lf->type == TBSLEAF)
    {
      list_t* skeys = ((sleaf_t*)lf)->keys;
      
      while (skeys)
	{
	  if (val_eql(skeys->car,k))
	    return (obj_t*)skeys;

	  skeys = skeys->cdr;
	}
    }

  else
    {
      dleaf_t* dlf = (dleaf_t*)lf;
      while (dlf)
	{
	  if (val_eql(k,(dlf)->key))
	    return (obj_t*)dlf;

	  dlf = (dlf)->next;
	}
    }

  return NULL;
}



static obj_t* sleaf_insert(sleaf_t* lf, val_t k, uint16_t flags, rcmp_t cmpf)
{
  list_t** skeys = &(lf->keys);

  if (!cmpf)
    cmpf = val_eql;

  while (*skeys)
    {
      if (cmpf(k,(*skeys)->car))
	  return (obj_t*)(*skeys);

      skeys = &(*skeys)->cdr;
    }

  list_t* new = flags & GLOBAL ? vm_cmalloc(16) : vm_allocw(0,2);
  new->car = k;
  new->cdr = NULL;
  *skeys = new;

  return (obj_t*)new;
}


static obj_t* dleaf_insert(dleaf_t* lf, hash_t h, val_t k, uint16_t flags, rcmp_t cmpf)
{
  dleaf_t** curr = &lf;

  if (!cmpf)
    cmpf = val_eql;

  while (*curr)
    {
      if (cmpf(k,(*curr)->key))
	{
	  return (obj_t*)(*curr);
	}

      curr = &((*curr)->next);
    }

  leaf_t* new = mk_leaf(h,k,flags);
  *curr = (dleaf_t*)new;
  return (obj_t*)new;
}

static inline val_t leaf_insert(leaf_t* lf, hash_t h, val_t k, uint16_t fl, rcmp_t cmpf)
{
  
  if (lf->type == TBSLEAF)
    return ((val_t)sleaf_insert((sleaf_t*)lf,k,fl,cmpf)) | LIST;

  else
    return ((val_t)dleaf_insert((dleaf_t*)lf,h,k,fl,cmpf)) | OBJECT;
}


static obj_t* sleaf_delete(sleaf_t* lf, val_t k, uint16_t fl)
{
  list_t** curr = &(lf->keys);

  while (*curr)
    {
      if (val_eql(k,(*curr)->car))
	{
	  list_t* old = *curr;
	  *curr = old->cdr;
	  if (fl & GLOBAL)
	    vm_cfree(old);

	  break;
	}

      curr = &(*curr)->cdr;
    }

  if (lf->keys)
    return (obj_t*)lf;

  else
    vm_cfree(lf);

  return NULL;
}

static obj_t* dleaf_delete(dleaf_t* lf, val_t k, uint16_t fl)
{
  dleaf_t** curr = &lf;

  while (*curr)
    {
      if (val_eql((*curr)->key,k))
	{
	  dleaf_t* old = *curr;
	  *curr = old->next;

	  if (fl & GLOBAL)
	      vm_cfree(old);
	}

      else
	curr = &(*curr)->next;
    }

  if (lf->next)
    return (obj_t*)lf;

  else
    vm_cfree(lf);

  return NULL;
}

static inline obj_t* leaf_delete(leaf_t* lf, val_t k, uint16_t fl)
{
  if (lf->type == TBDLEAF)
    return dleaf_delete((dleaf_t*)lf,k,fl);

  else
    return sleaf_delete((sleaf_t*)lf,k,fl);
}

obj_t* hamt_search(val_t n, val_t k)
{
  bvec_t* nb = tobvec(n);
  hash_t h = val_hash(k);

  for (uint8_t l = 0; l < 7 && n; l++)
    {
      uint8_t lcl_idx = (h & get_mask(l+1)) >> (l*5);
      val_t* rslt = bvec_ref(nb,lcl_idx);

      if (!rslt)
	return (obj_t*)n;

      else if (isleaf(*rslt))
	return (obj_t*)leaf_search(ptr(leaf_t*,(*rslt)), k);

      else
	nb = ptr(bvec_t*,*rslt);
    }

  return NULL;
}


val_t hamt_insert(val_t* n, val_t k, uint16_t flags, rcmp_t cmpf)
{
  hash_t h = val_hash(k);
  leaf_t* new_lf;
  val_t  out;
  bvec_t* nb = tobvec(*n);
  val_t* bv_idx = n;

  for (uint8_t l = 0; l < 7; l++, nb=ptr(bvec_t*,(*bv_idx)))
    {
      uint8_t lcl_idx = (h & get_mask(l+1)) >> (l*5);
      val_t* rslt = bvec_ref(nb,lcl_idx);

      if (!rslt)
	{
	  bvec_t* new = cp_bvec(nb,1);
	  new_lf = mk_leaf(h,k,flags);
	  *(bvec_ref(new,lcl_idx)) = ((val_t)new_lf) | OBJECT;

	  if (flags & GLOBAL)
	    {
	      bvec_t* old = ptr(bvec_t*,*bv_idx);
	      vm_cfree(old);
	    }

	  *bv_idx = ((val_t)new) | OBJECT;
	  out = ((val_t)new_lf) | OBJECT;
	  goto end;
	}

      else if (isleaf(*rslt))
	{
	  leaf_t* lf = ptr(leaf_t*,*rslt);
	  if (lf->hash == h)
	    {
	      out = leaf_insert(lf,h,k,flags,cmpf);
	      goto end;
	    }

	  else // create a split
	    {
	      new_lf = mk_leaf(h,k,flags);
	      hash_t lh = lf->hash;
	      bvec_t* new;
	      val_t* root = rslt;
	      for (uint8_t sl = l + 1;sl < 7; sl++)
		{
		  lcl_idx = (h & get_mask(sl + 1)) >> (sl*5);
		  uint8_t h_lcl_idx = (lh & get_mask(sl+1)) >> (sl*5);

		  if (lcl_idx == h_lcl_idx)
		    {
		      new = mk_bvec(sl,1,flags);
		      *root = ((val_t)new) | OBJECT;
		      new->bv_bmap = lcl_idx;
		      root = new->bv_elements;
		    }

		  else
		    {
		      new = mk_bvec(sl,2,flags);
		      *root = ((val_t)new) | OBJECT;
		      if (lcl_idx > h_lcl_idx)
			{
			  new->bv_elements[0] = (val_t)lf     | OBJECT;
			  new->bv_elements[1] = (val_t)new_lf | OBJECT;
			}

		      else
			{
			  new->bv_elements[0] = (val_t)new_lf | OBJECT;
			  new->bv_elements[1] = (val_t)lf     | OBJECT;
			}

		      new->bv_bmap = (1 << lcl_idx) | (1 << h_lcl_idx);
		      out = (val_t)new_lf | OBJECT;
		      goto end;
		    }
		}
	    }
	}

      else
	bv_idx = rslt;
    }

    end:
      return out;
}


val_t hamt_put(val_t* bv, val_t k, val_t b, uint16_t flags, rcmp_t cmpf)
{
  val_t loc = hamt_insert(bv,k,flags,cmpf);
  assert(isdleaf(loc),TYPE_ERR);

  ptr(dleaf_t*,loc)->value = b;
  return loc;
}


int32_t hamt_remove(val_t* bv, val_t key, uint16_t flags)
{
  bvec_t* nb = tobvec(*bv);
  hash_t h = val_hash(key);

  for (uint8_t l = 0; l < 7 && nb; l++)
    {
      uint8_t lcl_idx = (h & get_mask(l+1)) >> (l*5);
      val_t* rslt = bvec_ref(nb,lcl_idx);

      if (!rslt)
	return 0;

      else if (isleaf(*rslt))
	{
	  obj_t* rm = leaf_delete(ptr(leaf_t*,*rslt),key,flags);

	  if (rm == NULL && l) // don't resize the root table
	    {
	      val_t* prev = rslt, *next;
	      uint32_t curridx = prev - nb->bv_elements;

	      while (curridx < popcnt(nb->bv_bmap))
		{
		  next = prev + 1;
		  *prev = *next;
		  prev = next;
		}

	      nb->bv_bmap &= ~(1 << lcl_idx);
	      bvec_t* old = nb;
	      nb = cp_bvec(nb,-1);
	      *bv = (val_t)nb | OBJECT;

	      if (flags & GLOBAL)
		vm_cfree(old);
	    }

	  return 1;
	}

      else
	bv = rslt;
	nb = ptr(bvec_t*, *bv);
    }

  return 0;
}
