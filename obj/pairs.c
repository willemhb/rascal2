#include "../include/pairs.h"

MK_SAFECAST_P(pair_t*,pair,addr)

pair_t* mk_pair(val_t ca, val_t cd)
{
  pair_t* hd = vm_allocc(1);

  car_(hd) = ca;
  cdr_(hd) = cd;

return (pair_t*)hd;
}


/* object constructors */
pair_t* mk_list(val_t* args, size_t argc)
{
  if (argc == 0)
    return NULL;

  else
    {
      pair_t* out = vm_allocc(argc);
      pair_t* curr = out;
      size_t i = 1;
      for (; i < argc - 1; i++, curr++)
    {
      head(curr) = args[i];
      tail(curr) = (val_t)(curr + 1);
    }

  head(curr) = args[i];
  tail(curr) = 0;

  return out;
  }
}

inline val_t rsp_cons(val_t args, size_t argc)
{
  argcount(argc,2u);
  val_t* stk = (val_t*)args;
  return (val_t)mk_pair(stk[0],stk[1]);
}

inline val_t car(val_t ca)
{
  return topair(ca)->car;
}

inline val_t cdr(val_t cd)
{
  return topair(cd)->cdr;
}

/* gc functions */
val_t relocate_pair(type_t* to, val_t v, uchr_t** tos)
{
  uchr_t* frm = ptr(uchr_t*,v);

  memcpy(*tos,frm,16);

  pair_t* new = (pair_t*)(*tos);
  *tos += 16;

  head(new) = gc_trace(head(new));
  tail(new) = gc_trace(tail(new));

  return tag((val_t)new, to);
}


hash_t hash_pair(val_t v, uint32_t r)
{
  val_t ca = ptr(pair_t*,v)->car, cd = ptr(pair_t*,v)->cdr;
  hash_t hashes[2];
  hashes[0] = val_hash(ca);
  hashes[1] = val_hash(cd);
  hash_t final = hash_array(hashes,2,r);
  return final;
}

size_t pair_elcnt(val_t p)
{
  pair_t* px = topair(p);
  size_t out = 0;

  for (;px;px=(pair_t*)cdr_(px))
    {
      assert(ispair(p),TYPE_ERR);
      out++;
    }

  return out;
}

val_t pair_assocn(pair_t* p, int32_t i)
{
  while (i--)
    {
      assert(p, BOUNDS_ERR);
      assert(ispair(tail(p)),TYPE_ERR);
      p = (pair_t*)tail(p);
    }

  return head(p);
}

int32_t pair_assocv(pair_t* p, val_t v)
{
  size_t idx = 0;
  while (1)
    {
      if (!p)
	return -1;

      if (head(p) == v)
	return idx;

      assert(ispair(tail(p)),TYPE_ERR);
      p = (pair_t*)tail(p);
    }
}


pair_t* pair_rplcn(pair_t* p, int32_t i, val_t x)
{
  
  while (i--)
    {
      assert(p,BOUNDS_ERR);
      assert(ispair(tail(p)),TYPE_ERR);
      p = (pair_t*)tail(p);
    }

  head(p) = x;
  return p;
}

pair_t* pair_append(pair_t** ls, val_t v)
{
    pair_t* new = mk_pair(v,R_NIL);

    while (*ls)
      ls = (pair_t**)(&tail(*ls));

    *ls = new;

    return *ls;
}


/* printing */
void prn_pair(val_t p, riostrm_t* f) {
  fputwc('(',f);

   if (p)
     for (;;)
       {
	 val_t pc = cdr_(p);
	 val_prn(car_(p),f);

	 if (!pc)
	   break;

	 fputwc(' ', f);
	 if (!ispair(pc))
	   {
	     fputs(". ", f);
	     val_prn(pc,f);
	     break;
	   }

	 p = pc;
       }

  fputwc(')',f);
  return;
}


cfunc_api_t PAIR_API =
  {
    .prn             = prn_pair,
    .call            = NULL,
    .size            = fobj_sizeof,
    .elcnt           = pair_elcnt,
    .hash            = hash_pair,
    .ord             = NULL,
    .new             = fobj_new,
    .builtin_new     = rsp_cons,
    .init            = NULL,
    .relocate        = relocate_pair,
    .isalloc         = NULL,
  };

const type_t PAIR_TYPE_OBJ =
  {
    .type               = DATATYPE,
    .cmeta              = OBJECT,
    .tp_tpkey           = PAIR,
    .tp_ltag            = PAIR,
    .tp_sizing          = FIXED,
    .tp_init_sz         = 0,
    .tp_base_sz         = 16,
    .tp_nfields         = 2,
    .tp_cvtable         = NULL,
    .tp_capi            = &PAIR_API,
    .name = "pair",
};
