#include "rascal.h"

/* memory management and bounds checking */
static size_t calc_mem_size(size_t nbytes) {
  size_t basesz = max(nbytes, 16u);
  while (basesz % 16)
    basesz++;

  return basesz;
}

bool isallocated(val_t v)
{
  ntag_t t = ntag(v);
  switch (t)
    {
    case NTAG_DIRECT: case NTAG_CPRIM: case NTAG_IOSTRM:
      return false;
    case NTAG_SYM:
      return !sm_interned(v);
    case NTAG_TABLE:
      return !tb_global(v);
    case NTAG_OBJECT:
      return otype_(v) != TYPE_METAOBJ;
    default:
      return true;
    }
}

inline bool gc_check(uint64_t n)
{
  return (FREE + n) >= (FROMSPACE + (HEAPSIZE * 8));
}

uchr8_t* vm_cmalloc(uint64_t nbytes)
{
  uchr8_t* out = malloc(nbytes);
  if (!out)
    {
      fprintf(stdout,"[%s:%d:%s] Exiting due to machine allocation failure.\n",__FILE__,__LINE__,__func__);
      exit(EXIT_FAILURE);
    }

  memset(out,0,nbytes);
  return out;
}

int32_t vm_cfree(uchr8_t* m)
{
  free(m);
  return 1;
}

uchr8_t* vm_crealloc(uchr8_t** m, uint64_t size, bool scrub)
{
  uchr8_t* new = realloc(*m,size);
  if (!new)
    {
      fprintf(stdout,"[%s:%d:%s]: Exiting due to machine allocation failure.\n", __FILE__, __LINE__, __func__ ); 
      exit(EXIT_FAILURE);
    }

  if (scrub) memset(new,0,size);
  *m = new;
  return new;
}

uchr8_t* vm_alloc(size_t bs, size_t extra)
{
  size_t alloc_size = calc_mem_size(bs + extra);

  if (gc_check(alloc_size))
    gc_run();
  
  uchr8_t* out = FREE;
  FREE += alloc_size;
  return out;
}

size_t vm_allocsz_str(type_t* to, size_t argc, val_t* args)
{
  val_t* base = BASE(args,argc);
  chr8_t* s = ecall(tostr,*base);
  return strsz(s);
}

size_t vm_allocsz_bytes(type_t* to, size_t argc, val_t* args)
{
    val_t* base = BASE(args,argc);
    uint64_t n = ecall(toint,*base);
    return n;
}

size_t vm_allocsz_copies(type_t* to, size_t argc, val_t* args)
{
  
}

// entry point for non-builtin constructors and constructor calls that couldn't be resolved at compile time
val_t rsp_new(type_t* to, size_t argc, val_t* args)
{
  if (to->tp_builtin_p)
    {
      cprim_t* tnw = to->tp_new;
      if (tnw->vargs)
	return tnw->callable.varg_fun(argc,args);
      else
	return tnw->callable.farg_fun(args);
    }

  else
    {
      size_t allcsz = to->tp_alloc_sz(to,argc,args);
      uchr8_t* new = vm_alloc(to->tp_base,allcsz);

      if (to->tp_vtag == VTAG_OBJECT)
	  ((val_t*)new)[0] = ((val_t)(to)) | OTAG_OBJECT;

      val_t* bs = BASE(args,argc);
      memcpy(new + 8,(uchr8_t*)bs,(to->tp_fields->count)*8);

      if (to->tp_init)
	to->tp_init(new,argc,args);

      return (val_t)new | to->tp_vtag;
    }
}

  uchr8_t* vm_allocs(type_t* to, size_t n, val_t* args)
{
  chr8_t* s = tostr(args[0],__FILE__,__LINE__,__func__);
  return vm_alloc(tp_base(to) + strsz(s));
}

val_t  vm_inits(type_t* to, val_t new, val_t* args)
{
  chr8_t* strf = (chr8_t*)(new + tp_base(to));
  chr8_t* stra = ptr(chr8_t*,args[0]);
  strcpy(strf,stra);
  return new;
}

uchr8_t* vm_allocb(type_t* to, size_t n, val_t* args)
{
  size_t s = uval(args[1]);
  return vm_alloc(tp_base(to) + s);
}

val_t  vm_initb(type_t* to, val_t new, val_t* args)
{
  uint64_t hdsz = tp_base(to);
  val_t* sz_field = (val_t*)(new + hdsz);
  uchr8_t* bts_field = (uchr8_t*)(new + hdsz + 8);
  *sz_field = uval(args[1]);
  uchr8_t* bts = ptr(uchr8_t*,args[0]);
  memcpy(bts_field,bts,*sz_field);
  return new;
}

uchr8_t* vm_allocv(type_t* to, size_t n, val_t* args)
{
  size_t extra = uval(args[0]);
  return vm_alloc(tp_base(to) + ((n + extra) * 8));
}


val_t vm_initv(type_t* to,val_t new,size_t n, val_t* args)
{
  vm_inito(to,new,args);
  size_t hdsz = to->tp_ndsz + to->tp_tagged;
  args += hdsz;
  n -= hdsz;
  vec_t* v = (vec_t*)new;
  val_t* varr = vec_values(v);
  size_t i;
  for (i=0;i < n;i++)
    {
      varr[i] = args[i];
    }
  return new;
}


uchr8_t* vm_allocn(type_t* to, size_t n, val_t* args)
{
  return vm_alloc(tp_base(to) * n);
}

uchr8_t* vm_allocp(type_t* to, size_t n, val_t* args)
{
  size_t aligned_base = calc_mem_size(tp_base(to));
  return vm_alloc(aligned_base + (to->tp_ndsz * n));
}

// reallocate 
val_t* vm_realloc(val_t* sz, size_t curr, size_t new)
{
  uint64_t oldbts = curr * 8;
  uint64_t newbts = calc_mem_size(new * 8);
  val_t* out;

  if (gc_check(new))
    {
      if (GROWHEAP)
	{
	  gc_resize();
	  GROWHEAP = false;
	}
      
      out = (val_t*)TOSPACE;
      TOFFSET = TOSPACE + newbts;
      memset(TOSPACE,0,newbts);
      memcpy(TOSPACE,(uchr8_t*)sz,oldbts);
      *sz = R_FPTR;
      sz[1] = (val_t)out;
      ((val_t*)TOSPACE)[curr+1] = R_FPTR; // mark the end of the new elements
      gc_run();
    }

  else
    {
      out = (val_t*)FREE;
      FREE += newbts;
      memcpy(out,(uchr8_t*)sz,oldbts);
      *sz = R_FPTR;
      sz[1] = (val_t)out;
    }

  return out;
}

void gc_resize()
{
  HEAPSIZE *= 2;
  vm_crealloc(&TOSPACE,HEAPSIZE,true);
  return;
}

void gc_run() {
  if (GROWHEAP) gc_resize();

  // reassign the head of the FREE area to point to the tospace
  FREE = TOFFSET;

  if (TOFFSET != TOSPACE)
    {
      // if a block was preallocated, trace its bindings
      val_t* TARR = (val_t*)TOSPACE;
      size_t numwords = (val_t*)TOFFSET - TARR;
      for (size_t i = 0; i < numwords && TARR[i] != R_FPTR; i++)
	{
	  TARR[i] = gc_trace(TARR[i],LTAG_NONE);
	}
    }

  // trace the top level environment
  R_MAIN = gc_trace(R_MAIN,LTAG_NONE);

  // trace EVAL, DUMP, and the main registers
  for (size_t i = 0; i < SP; i++) EVAL[i] = gc_trace(EVAL[i],LTAG_NONE);
  for (size_t i = 0; i < DP; i++) DUMP[i] = gc_trace(DUMP[i],LTAG_NONE);
  for (size_t i = 0; i < 4; i++) REGISTERS[i] = gc_trace(REGISTERS[i],LTAG_NONE);

  // swap the fromspace & the tospace
  uchr8_t* TMPHEAP = FROMSPACE;
  FROMSPACE = TOSPACE;
  TOSPACE = TMPHEAP;
  TOFFSET = TOSPACE;
  GROWHEAP = (FREE - FROMSPACE) > (HEAPSIZE * RAM_LOAD_FACTOR);

  return;
}

uchr8_t* gc_reserve(val_t v)
{
  uchr8_t* out = FREE;
  size_t osz = calc_mem_size(val_size(v));
  FREE += osz;
  return out;
}


val_t gc_copy(type_t* tp, val_t v, uchr8_t* to)
{
  size_t sz = tp->tp_size ? tp->tp_size(v) : tp_base(tp);
  size_t asz = calc_mem_size(sz);
  uchr8_t* frm = ptr(uchr8_t*,v);
  memcpy(frm,to,asz);
  val_t out = (val_t)frm;
  // install the forwarding pointer
  car_(frm) = R_FPTR;
  cdr_(frm) = tag_p(out,tp->tp_ltag);
  return out;
}

val_t gc_trace(val_t v, ltag_t t)
{
  if (!isallocated(v) || in_heap(v,(val_t*)TOSPACE,HEAPSIZE)) return v;
  else if (isfptr(car_(v)))
    {
      return trace_fp(v);
    }

  else
    {
      bool return_tagged;
      if (t == LTAG_NONE)
	{
	  return_tagged = true;
	}
      else
	{
	  return_tagged = false;
	  t = ntag(v);
	  v = v | t;
	}

      uchr8_t* new_spc = gc_reserve(v);
      val_t new_val, * varr; size_t elcnt;
      type_t* to = val_type(v); uint64_t* offsets;

      switch (t)
	{
	 case LTAG_OBJECT:
	   varr = ptr(val_t*,v); offsets = to->tp_foffsets;
	   for (size_t i = 0; i < to->tp_nfields; i++)
	     {
	       varr[offsets[i]] = gc_trace(varr[offsets[i]],LTAG_NONE);
	     }
	   break;
	 case LTAG_NODE:	   
	   nd_left_(v) = (node_t*)gc_trace((val_t)nd_left_(v),LTAG_NODE);
	   nd_right_(v) = (node_t*)gc_trace((val_t)nd_right_(v),LTAG_NODE);
	   nd_data_(v) = gc_trace(nd_data_(v),LTAG_NONE);
	   break;
	 case LTAG_CONS:
	 case LTAG_LIST:
	   car_(v) = gc_trace(car_(v),LTAG_NONE);
	   cdr_(v) = gc_trace(cdr_(v),LTAG_NONE);
	   break;
	 case LTAG_TABLE:
	   tb_records_(v) = (node_t*)gc_trace((val_t)tb_records_(v),LTAG_NODE);
	   break;
	 case LTAG_VEC:
	   varr = vec_values_(v); elcnt = vec_datasz_(v);
	   for (size_t i = 0;i < elcnt; i++)
	     {
	       varr[i] = gc_trace(varr[i],LTAG_NONE);
	     }
	   break;
	 case LTAG_METHOD:
	   meth_code_(v) = (vec_t*)gc_trace((val_t)meth_code_(v),LTAG_VEC);
	   meth_names_(v) = (table_t*)gc_trace((val_t)meth_names_(v),LTAG_TABLE);
	   meth_envt_(v) = (vec_t*)gc_trace((val_t)meth_envt_(v),LTAG_VEC);
	   break;
	default:
	  break;
	}

      new_val = gc_copy(to,v,new_spc);
      return new_val | (t * return_tagged);
    }
}


inline val_t* get_vec_values(vec_t* v)   { return vec_localvals(v) ? &(vec_data(v)) : (val_t*)(vec_data(v)) ; }
inline val_t* get_vec_elements(vec_t* v) { return get_vec_values(v) + vec_dynamic(v) + vec_initsz(v) ; }


/*
// simple limit checks
bool in_heap(val_t v, val_t* base, uint64_t heapsz)
{
  uptr_t a = addr(v), b = (uptr_t)base, c = (uptr_t)(base + heapsz);
  return a >= b && a <= c;
}

// specialized accessors
inline val_t get_nd_key(node_t* n)
{
  val_t d = nd_data(n);
  return iscons(d) ? ptr(val_t*,d)[0] : d;
}

inline val_t get_nd_binding(node_t* n)
{
  val_t d = nd_data(n);
  return iscons(d) ? ptr(val_t*,d)[1] : d;
}

inline list_t* new_list(int cnt) { return cnt == 0 ? NULL : (list_t*)rsp_allocw(cnt*2) ; }
inline cons_t* new_cons() { return (cons_t*)rsp_allocw(2); }
inline char* new_str(int ssz) { return (char*)rsp_alloc(ssz) ; }

sym_t* new_sym(int ssz, uchr8_t** mem)
{
  
  uchr8_t* m;
  if (mem)
    {
      m = *mem;
      *mem += calc_mem_size(8+ssz);
    }
  else
    {
      m = rsp_alloc(8+ssz);
    }
  
  return (sym_t*)m;
}


inline node_t* new_node(int nb, uchr8_t** mem)
{
  uchr8_t* m;
  if (mem)
    {
      m = rsp_allocw(4+nb);
    }

  else
    {
      m = *mem;
      *mem += calc_mem_size(8*(4+nb));
    }

  return (node_t*)m;
}

inline fvec_t* new_fvec(int elcnt) { return (fvec_t*)rsp_allocw(2 + elcnt); }
inline dvec_t* new_dvec(int elcnt)
{
   elcnt = elcnt < 32 ? 32 : cpow2_32(elcnt);
  val_t store = mk_fvec(elcnt);
  PUSH(store); // in case allocating the dynamic part initiates GC, save the store
  dvec_t* out = (dvec_t*)rsp_allocw(4);
  out->elements = POP();

  return out;
}

inline table_t* new_table(int ndsz, int ndcnt)
{
  ndsz = min(ndsz,2);
  table_t* out = (table_t*)rsp_allocw(6 + ((4 + ndsz) * ndcnt));
  type_(out) = (val_t)CORE_OBJECT_TYPES[LTAG_TABLEOBJ];
  out->_table_fields[0] = ((val_t)out) + 48;

  return out;
}

inline function_t* new_function()
{
  function_t* out = (function_t*)rsp_allocw(6);
  type_(out) = (val_t)CORE_OBJECT_TYPES[LTAG_FUNCOBJ];
  return out;
}

int init_table(val_t t, val_t* args, int argc)
{
  table_t* tab = totable_(t);

  uchr8_t* free = (uchr8_t*)tab + 32; // reserved space
  node_t* r = records_(tab), *new_n;
  int o = 0, rsz = tab->type & 0xf;
  tab->type = tab->type & (~0xful);

  if (rsz == 2) for (int i = argc - 1; i >= 0; o++, free += 48)
		  {
		    val_t new_hk = args[i];
		    if (!isatomic(new_hk)) rsp_raise(TYPE_ERR);
		    i--;
		    val_t new_b = i < 0 ? R_UNBOUND : args[i];
		    i--;
		    node_insert(&r,&new_n,new_hk,o,2,free);
		    bindings_(new_n)[0] = new_b;
		  }

  else for (int i = argc - 1; i >= 0; i--, o++, free += 32)
	 {
	   val_t new_hk = args[i];
	   if (!isatomic(new_hk)) rsp_raise(TYPE_ERR);
	   node_insert(&r,&new_n,new_hk,o,0,free);
	 }

  count_(tab) = o;
  return 0;
}


int init_list(val_t new, val_t* stk, int argc) {
  if (isnil(new)) return 0;

  val_t* curr = (val_t*)addr(new); 
  for (int i = argc - 1; i >= 0; i--) {
    *curr = *(stk + i);

    *(curr + 1) = i ? (val_t)(curr + 2) : R_NIL; // if we're at the last argument, set the tail to nil
  }

  return 0;
}

int init_sym(sym_t* s, const char* sn, hash64_t h, int fl)
{
  symhash_(s) = h | fl;
  memcpy(symname_(s),sn,strsz(sn));
  return 0;
}

inline int init_str(char* sv, const char* sn)
{
  memcpy(sv,sn,strsz(sn));
  return 0;
}

inline val_t mk_list(int cnt) { return cnt == 0 ? R_NIL : tag_p(rsp_allocw(cnt * 2), LTAG_LIST); }

inline val_t mk_cons(val_t ca, val_t cd) {
  PUSH(ca);
  PUSH(cd);
  cons_t* new = new_cons();
  cdr_(new) = POP();
  car_(new) = POP();
  return islist(cd) ? tag_p(new,LTAG_LIST) : tag_p(new,LTAG_CONS);
}


inline val_t mk_cfile(FILE* f) { return tag_p(f,LTAG_CFILE); }


inline val_t mk_table(int cnt, int rsz)
{ table_t* newtb = new_table(cnt,rsz);
  count_(newtb) = 0;
  records_(newtb) = NULL;
  return tag_p(newtb,LTAG_TABLEOBJ);
}

val_t mk_str(const char* s) {
  // if the string is properly aligned just return a tagged pointer
  if (aligned(s,16)) return ((val_t)s) | LTAG_STR;
  int i = strsz(s);
  char* new = new_str(i);
  memcpy(new,s,i);

  return tag_p(new,LTAG_STR);
}

val_t mk_envframe(val_t e, val_t cls, int argc)
{
  PUSH(e);
  PUSH(cls);
  dvec_t* dv = new_dvec(2+argc);
  cls = POP();
  e = POP();
  dvec_elements_(dv)[0] = e;
  dvec_elements_(dv)[1] = cls;
  dv->elements |= DVECFLAG_ENVTFRAME;
  dv->curr = 2;
  return tag_p(dv,LTAG_DVEC);
}

val_t mk_fvec(int elements)
{
  fvec_t* out = new_fvec(elements);
  out->allocated = elements;
  return tag_p(out,LTAG_FVEC);
}

inline val_t mk_bool(int b) { return b ? R_TRUE : R_FALSE ; }

val_t mk_envt(val_t argl, int* flags)
{
  int l = check_argl(argl,flags);
  if (l == -1)
    {
      rsp_perror(SYNTAX_ERR,"Malformed argument list.");
      rsp_raise(SYNTAX_ERR);
    }

  PUSH(argl);
  table_t* names = (table_t*)rsp_allocw(4 + (4 * l));
  type_(names) = ((val_t)CORE_OBJECT_TYPES[LTAG_TABLEOBJ]) | TABLEFLAG_ENVT;
  count_(names) = 0;
  records_(names) = NULL;
  uchr8_t* free = (uchr8_t*)(names) + 32;

  list_t* argls = tolist_(POP());
  node_t** n = &records_(names);

  for (;argls;argls=rest_(argls),free += 32,count_(names)++)
    {
      node_insert(n,NULL,first_(argls),count_(names),0,free);
    }

  return tag_p(names,LTAG_TABLEOBJ);
}

inline val_t mk_char(int c) { return c < MIN_CODEPOINT || c > MAX_CODEPOINT ? R_EOF :  tag_v(c,LTAG_CHAR); }

val_t mk_dvec(int elements)
{
  dvec_t* out = new_dvec(elements);
  dvec_curr_(out) = 0;
  return tag_p(out,LTAG_DVEC);
}

val_t mk_sym(const char* s, int flags) {
  static int GS_COUNTER = 0;
  hash64_t h;
  size_t ssz;
  if (nextu8(s) == U':') flags |= SYMFLAG_KEYWORD;

  if (flags & SYMFLAG_GENSYM)
    {
      char gsbuf[74+strsz(s)];
      sprintf(gsbuf,"__GENSYM__%d__%s",GS_COUNTER,s);
      h = hash_str(gsbuf);
      ssz = strsz(gsbuf);
      GS_COUNTER++;

      if (flags & SYMFLAG_INTERNED) return intern_string(R_SYMTAB,gsbuf,h,flags);
      else
	{
	  sym_t* new_s = new_sym(8+ssz,NULL);
	  init_sym(new_s,s,h,flags);
	  return tag_p(new_s,LTAG_SYM);
	}
  }

  h = hash_str(s);
  ssz = strsz(s);

  if (flags & SYMFLAG_INTERNED) return intern_string(R_SYMTAB,s,h,flags);
  else
    {
      sym_t* new_s = new_sym(8+ssz,NULL);
      init_sym(new_s,s,h,flags);
      return tag_p(new_s,LTAG_SYM);
    }
}


val_t mk_builtin(rsp_cfunc_t cfnc, int argc, bool vargs, int index)
{
  BUILTIN_FUNCTIONS[index] = cfnc;
  BUILTIN_FUNCTION_ARGCOS[index] = argc;
  BUILTIN_FUNCTION_VARGS[index] = vargs;

  return tag_v(index,LTAG_BUILTIN);
}


val_t mk_function(val_t tmpl, val_t argl, val_t envt, int macro) {
  int flags = macro;
  PUSH(tmpl); // save arguments in case allocation triggers the GC
  PUSH(envt);
  val_t locals = mk_envt(argl,&flags);
  PUSH(locals);
  function_t* fnc = new_function();
  locals = POP();
  envt = POP();
  tmpl = POP();
  type_(fnc) |= flags;
  fnargco_(fnc) = count_(locals);
  template_(fnc) = tmpl;
  closure_(fnc) = envt;
  envt_(fnc) = locals;

  return tag_p(fnc,LTAG_FUNCOBJ);
}

inline val_t mk_int(int i) { return tag_v(i,LTAG_INT); }

inline val_t mk_float(float f) { return tag_v(f,LTAG_FLOAT);}


val_t copy_cons(type_t* to,val_t c)
{
  cons_t* co = tocons(c);
  gc_trace(&car_(co));
  gc_trace(&cdr_(co));
  return rsp_copy(to,(uchr8_t*)co,&TOSPACE,16);
}


val_t copy_node(type_t* to, val_t c)
{
  if (left_(c))
    {
      val_t left_cp = copy_node(to,(val_t)left_(c));
      left_(c) = (node_t*)addr(left_cp);
    }
  if (right_(c))
    {
      val_t left_cp = copy_node(to,(val_t)left_(c));
      left_(c) = (node_t*)addr(left_cp);
    }
  hashkey_(c) = gc_trace(&hashkey_(c));
  if (fieldcnt_(c))
    {
      for (size_t i = 1; i <= fieldcnt_(c);i++)
	{
	  bindings_(c)[i] = gc_trace(&bindings_(c)[i]);
	}
    }
  return rsp_copy(to,(uchr8_t*)addr(c),&FREE,8*(4+fieldcnt_(c)));
}

val_t copy_fvec(type_t* to, val_t fv)
{
  fvec_t* fvo = tofvec_(fv);
  val_t allc = fv_alloc_(fvo);
  val_t* elements = fv_values_(fvo);

  for (size_t i = 0; i < allc;i++)
    {
      gc_trace(elements+i);
    }
  return rsp_copy(to,(uchr8_t*)fvo,&FREE,8*(2+allc));
}

val_t copy_dvec(type_t* to, val_t dv)
{
  dvec_t* dvo = todvec_(dv);
  gc_trace(dv_values_(dvo));
  return rsp_copy(to,(uchr8_t*)dvo,&FREE,16);
}

val_t copy_function(type_t* to, val_t fnc)
{
  function_t* fnco = tofunction_(fnc);
  gc_trace(&template_(fnco));
  gc_trace(&envt_(fnco));
  gc_trace(&closure_(fnco));
  return rsp_copy(to,(uchr8_t*)fnco,&FREE,48);
}


val_t _car(val_t c, const char* fl, int ln, const char* fnm) {
  if (hascar(c)) return car_(c);
  else {
    _rsp_perror(fl,ln,fnm,TYPE_ERR,"Expected cons or list, got %s",val_type_name(c));
    rsp_raise(TYPE_ERR);
  }
  return R_NONE;
}

val_t _cdr(val_t c, const char* fl, int ln, const char* fnm) {
  if (hascar(c)) return cdr_(c);
  else
    {
    _rsp_perror(fl,ln,fnm,TYPE_ERR,"Expected cons or list, got %s",val_type_name(c));
    rsp_raise(TYPE_ERR);
    }

  return R_NONE;
}

inline int balance_factor(node_t* n)
{
  if (!n) return 3;
  int b = 3;
  if (left_(n)) b -= balance_(left_(n));
  if (right_(n)) b += balance_(right_(n));
  return b;
}

// add a node to an AVL tree, rebalancing if needed
node_t* node_search(node_t* n, val_t x)
{
  while (n)
    {
      int result = vm_ord(x,hashkey_(n));
      if (result < 0) n = left_(n);
      else if (result > 0) n = right_(n);
      else break;
    }

  return n;
}

int node_insert(node_t** rt, node_t** new, val_t hk, int order, int nsz, uchr8_t* m)
{
  if (!(*rt)) // empty tree
    {
      
      node_t* new_nd = new_node(nsz,&m);
      hashkey_(new_nd) = hk;
      *rt = new_nd;
      if (new) *new = new_nd;
      return 1;
    }

  else
    {
      int comp = vm_ord(hk,hashkey_(*rt)), result;
      if (comp < 0)
	{
	  result = node_insert(&left_(*rt),new,hk,order,nsz,m);
	  balance_(*rt) -= result;
	}
      else if (comp > 0)
	{
	  result = node_insert(&right_(*rt),new,hk,order,nsz,m);
	  balance_(*rt) += result;
	}
      else
	{
	  if (new) *new = *rt;
	  return 0;
	}

      if (unbalanced(*rt)) balance_node(rt);
      return result;
    }
}


int node_intern(node_t** rt, val_t* sr, char* sn, hash64_t h, int fl, int order)
{
  if (!(*rt)) // empty tree
    {
      size_t sz = strsz(sn);
      uchr8_t* m = rsp_alloc(32 + 8 + sz);
      node_t* new_nd = new_node(0,&m);
      sym_t* new_sm = new_sym(sz,&m);
      *sr = tag_p(new_sm,LTAG_SYM);
      hashkey_(new_nd) = *sr;
      *rt = new_nd;
      return 1;
    }

  else
    {
      val_t s = hashkey_(*rt);
      int comp = compare(h,symhash_(s)) ?: u8strcmp(sn,symname_(s)), result;
      if (comp < 0)
	{
	  result = node_intern(&left_(*rt),sr,sn,h,fl,order);
	  balance_(*rt) -= result;
	}
      else if (comp > 0)
	{
	  result = node_intern(&right_(*rt),sr,sn,h,fl,order);
	  balance_(*rt) += result;
	}
      else
	{
	  return 0;
	}

      if (unbalanced(*rt)) balance_node(rt);
      return result;
    }
}

int node_delete(node_t** rt, val_t x)
{
  if (!(*rt)) // empty tree
    {
      return 0;
    }

  else
    {
      int comp = vm_ord(x,hashkey_(*rt)), result;
      if (comp < 0)
	{
	  result = node_delete(&left_(*rt),x);
	  balance_(*rt) += result;
	}
      else if (comp > 0)
	{
	  result = node_delete(&right_(*rt),x);
	  balance_(*rt) -= result;
	}
      else
	{
	  *rt = NULL;                    // unhook the node from the tree
	  return 1;
	}

      if (unbalanced(*rt)) balance_node(rt);
      return result;
    }
}

void balance_node(node_t** rt)
{
  node_t* tmpx = *rt, *tmpy;             // the two nodes to be rebalanced
  if (right_heavy(tmpx))                 // right-heavy case
    {
      tmpy = right_(tmpx);
      if (!left_heavy(tmpy)) *rt = rotate_ll(tmpx,tmpy);

      else *rt = rotate_lr(tmpx,tmpy); 
    }                                   
  else
    {
      tmpy = left_(tmpx);
      if (!right_heavy(tmpy)) *rt = rotate_rr(tmpx,tmpy);

      else *rt = rotate_rl(tmpx,tmpy);
    }

  return;
}

node_t* rotate_ll(node_t* x, node_t* y)
{
  node_t* out = y;
  right_(x) = left_(y);
  left_(y) = x;
  // recalculate balance for x and y
  balance_(x) = balance_factor(x);
  balance_(y) = balance_factor(y);

  // return the new root
  return out;
}

node_t* rotate_rr(node_t* x, node_t* y)
{
  node_t* out = y;
  left_(x) = right_(y);
  right_(y) = x;
  // recalculate balance for x and y
  balance_(x) = balance_factor(x);
  balance_(y) = balance_factor(y);
  // return the new root
  return out;
}


node_t* rotate_lr(node_t* x, node_t* y)
{
  node_t* z = rotate_rr(y,left_(y));
  return rotate_ll(x,z);
}

node_t* rotate_rl(node_t*x, node_t* y) {
  node_t* z = rotate_ll(y,right_(y));
  return rotate_rr(x,z);
}

int nodesz(table_t* t)
{
  switch (flags_(t))
    {
    case TABLEFLAG_ENVT: case TABLEFLAG_SYMTAB: return 0;
    default: return 2;
    }
}

val_t table_insert(val_t t, val_t hk)
{
  table_t* tab = totable(t);
  int fl = flags_(tab), nsz = nodesz(tab);

  if (fl & (TABLEFLAG_ENVT | TABLEFLAG_NMSPC))
    {
      if (!issym(hk)) rsp_raise(TYPE_ERR);
    }

  node_t* rslt;
  node_insert(&records_(tab),&rslt,count_(tab),hk,nsz,NULL);
  return tag_p(rslt,LTAG_NODE);
}

val_t table_delete(val_t t, val_t hk)
{
  table_t* tab = totable(t);
  int fl = flags_(tab);

  if (fl & (TABLEFLAG_ENVT | TABLEFLAG_NMSPC | TABLEFLAG_SYMTAB))
    {
      rsp_perror(TYPE_ERR,"Can't remove elements from an envt or symbol table.");
      rsp_raise(TYPE_ERR);
    }


  node_delete(&records_(tab),hk);
  return t;
}

val_t table_lookup(val_t t, val_t hk)
{
  table_t* tab = totable(t);

  node_t* rslt = node_search(records_(tab),hk);
  return tag_p(rslt,LTAG_NODE);
}

val_t table_update(val_t t, val_t hk, val_t b)
{
  table_t* tab = totable(t);
  int fl = flags_(tab);

  if ((fl & (TABLEFLAG_ENVT || TABLEFLAG_SYMTAB)))
    {
      rsp_perror(TYPE_ERR,"Can't update bindings in an envt or symbol table.");
      rsp_raise(TYPE_ERR);
    }

  if ((fl & (TABLEFLAG_NMSPC)) && !issym(t))
    {
      rsp_perror(TYPE_ERR,"Namespace keys must be symbols.");
      rsp_raise(TYPE_ERR);
    }

  node_t* rslt = node_search(records_(tab),hk);

  if (!rslt) rsp_raise(UNBOUND_ERR);
  bindings_(rslt)[0] = b;
  return t;
}

val_t intern_string(val_t t, char* sn,hash64_t h, int fl)
{
  table_t* tab = totable(t);
  int tfl = flags_(tab);

  if (tfl != TABLEFLAG_SYMTAB)
    {
      rsp_perror(TYPE_ERR,"Can't intern in non symbol table.");
      rsp_raise(TYPE_ERR);
    }

  val_t s;
  node_intern(&records_(tab),&s,sn,h,strsz(sn),fl);
  return s;
}

// vector API

val_t* _vec_elements(val_t v,const char* fl, int ln, const char* fnc) {
  switch (ltag(v)) {
  case LTAG_FVEC:
    return fvec_elements_(v);
  case LTAG_DVEC:
    return dvec_elements_(v);
  default:
    _rsp_perror(fl,ln,fnc,TYPE_ERR,"Expected vector type, got %s",val_type_name(v));
    rsp_raise(TYPE_ERR);
    return (val_t*)0;
  }
}

unsigned _vec_allocated(val_t v, const char* fl, int ln, const char* fnc) {
  switch (ltag(v)) {
  case LTAG_FVEC:
    return fvec_allocated_(v);
  case LTAG_DVEC:
    return dvec_allocated_(v);
  default:
    _rsp_perror(fl,ln,fnc,TYPE_ERR,"Expected vector type, got %s",val_type_name(v));
    rsp_raise(TYPE_ERR);
    return 0u;
  }
}


unsigned _vec_curr(val_t v, const char* fl, int ln, const char* fnc) {
  switch (ltag(v)) {
  case LTAG_FVEC:
    return fvec_allocated_(v);
  case LTAG_DVEC:
    return dvec_curr_(v);
  default:
    _rsp_perror(fl,ln,fnc,TYPE_ERR,"Expected vector type, got %s",val_type_name(v));
    rsp_raise(TYPE_ERR);
    return 0u;
  }
}

val_t vec_append(val_t v, val_t el) {
  dvec_t* dv = todvec(v);
  unsigned allc = dvec_allocated_(dv);
  if (dvec_curr_(dv) == allc)
    {
      PUSH(v);
      PUSH(el);
      fvec_t* new = (fvec_t*)rsp_allocw(2 + (allc*2));
      el = POP();
      dv = todvec_(POP());

      val_t* new_elem = fvec_elements_(new);
      val_t* old_elem = dvec_elements_(dv);

      for (size_t i = 0; i < allc; i++)
	{
	  new_elem[i] = old_elem[i];
	}

      fvec_allocated_(new) = allc*2;
      dv->elements = tag_p(new,v & 0xf); // copy the old tag
    }

  dvec_elements_(dv)[dvec_curr_(dv)] = el;
  dvec_curr_(dv)++;

  return tag_p(dv,LTAG_DVEC);
}

inline hash64_t symhash(val_t s) { return tosym(s)->hash & (~0xful); }
inline char* symname(val_t s) { return tosym(s)->name; }

// environment API
int check_argl(val_t argl, int* flags)
{
  if (!islist(argl)) return -1;
  size_t l = 0;
      int va = -1;

      for (list_t* argls = tolist_(argl);argls;argls=rest_(argls))
	{
	  val_t curr = first_(argls);
	  if (!issym(curr)) return -1;
	  if (curr == R_VARGSSYM)
	    {
	      va = l;
	      continue;
	    }
	  l++;
	}

      if (va != -1)
	{
	  if (l - va != 1) return -1;
	  *flags |= FUNCFLAG_VARGS;
	}

  return l;
}

// environment API
int env_bind(val_t e,val_t b, bool vargs)
{

}

int env_bindn(val_t,size_t,bool);

val_t env_next(val_t e)
{
  if (isenvtframe(e))
    {
      return dvec_elements_(e)[1];
    }
  else if (isnmspc(e))
    {
      return totable_(e)->_table_fields[1];
    }
  else
    {
      rsp_perror(TYPE_ERR,"non-environtment of type %s has no closure.",val_type_name(e));
      rsp_raise(TYPE_ERR);
      return 0;
    }
}

val_t env_extend(val_t nm, val_t e)
{
  if (!issym(nm))
    {
      rsp_perror(TYPE_ERR,"Expected type sym, got %s",val_type_name(nm));
      rsp_raise(TYPE_ERR);
    }

  if (isenvtframe(e))
    {
      val_t enms = dvec_elements_(e)[0];
      return table_insert(enms,e);
    }

  else if (isnmspc(e))
    {
      return table_insert(e,nm);
    }

  else
    {
      rsp_perror(ENVT_ERR,"Invalid envt of type %s.",val_type_name(e));
      rsp_raise(ENVT_ERR);
      return 0;
    }
}

val_t env_assign(val_t nm, val_t e, val_t b)
{
  if (!issym(nm))
    {
      rsp_perror(ENVT_ERR,"Expected type sym, got %s.",val_type_name(nm));
      rsp_raise(ENVT_ERR);
    }

  if (isreserved(nm))
    {
      rsp_perror(NAME_ERR,"Attempted to rebind reserved name %s",symname_(nm));
    }

  if (!(e & (~0xful)))
    {
      rsp_perror(UNBOUND_ERR,"No binding for symbol %s", symname_(nm));
      rsp_raise(UNBOUND_ERR);
    }
  
  if (isenvtframe(e))
    {
      val_t* locals = dvec_elements_(e);
      val_t loc = table_lookup(locals[0],nm);
      if (!(loc & (~0xful)))
	{
	  return env_assign(nm,env_next(e),b);
	}
      else
	{
	  locals[order_(loc) + 2] = b;
	  return b;
	}
    }
  
  else if (isnmspc(e))
    {
      val_t loc = table_lookup(e,nm);
      if (!(loc & (~0xful)))
	{
	  return env_assign(nm,env_next(e),b);
	}
      else
	{
	  bindings_(loc)[0] = b;
	  return b;
	}
    }

  else
    {
      rsp_perror(TYPE_ERR,"Non environment of type %s.",val_type_name(e));
      rsp_raise(TYPE_ERR);
      return 0;
    }
}

val_t env_lookup(val_t nm, val_t e)
{
  if (!issym(nm))
    {
      rsp_perror(ENVT_ERR,"Expected type sym, got %s.",val_type_name(nm));
      rsp_raise(ENVT_ERR);
    }

  if (isreserved(nm))
    {
      rsp_perror(NAME_ERR,"Attempted to rebind reserved name %s",symname_(nm));
    }

  if (!(e & (~0xful)))
    {
      rsp_perror(UNBOUND_ERR,"No binding for symbol %s", symname_(nm));
      rsp_raise(UNBOUND_ERR);
    }
  
  if (isenvtframe(e))
    {
      val_t* locals = dvec_elements_(e);
      val_t loc = table_lookup(locals[0],nm);
      if (!(loc & (~0xful)))
	{
	  return env_lookup(nm,env_next(e));
	}
      else
	{
	  return locals[order_(loc) + 2];
	}
    }
  
  else if (isnmspc(e))
    {
      val_t loc = table_lookup(e,nm);
      if (!(loc & (~0xful)))
	{
	  return env_lookup(nm,env_next(e));
	}
      else
	{
	  return bindings_(loc)[0];
	}
    }

  else
    {
      rsp_perror(TYPE_ERR,"Non environment of type %s.",val_type_name(e));
      rsp_raise(TYPE_ERR);
      return 0;
    }
}


#define RSP_ALIGNED __attribute__((aligned (16)))


type_t CONS_TYPE_OBJ = {
  .type = 0,
  .val_base_size = 16,
  .val_fixed_size  = 1,
  .val_cnum = C_INT64,
  .val_cptr = C_PTR_VOID,
  .tp_atomic = 0,
  .val_ltag = LTAG_CONS,
  .tp_fields = NULL,
  .tp_val_size = NULL,
  .tp_new = rsp_cons,
  .tp_init = NULL,
  .tp_copy = copy_cons,
  .tp_prn = prn_cons,
};

type_t LIST_TYPE_OBJ = {
  .type = 0,
  .val_base_size = 16,
  .val_fixed_size  = 1,
  .val_cnum = C_INT64,
  .val_cptr = C_PTR_VOID,
  .tp_atomic = 0,
  .val_ltag = LTAG_LIST,
  .tp_fields = NULL,
  .tp_val_size = NULL,
  .tp_new = rsp_list,
  .tp_init = NULL,
  .tp_copy = copy_cons,
  .tp_prn = prn_list,
};


type_t STR_TYPE_OBJ = {
  .type = 0,
  .val_base_size = 16,
  .val_fixed_size  = 0,
  .val_cnum = C_INT64,
  .val_cptr = C_STR_PTR,
  .tp_atomic = 0,
  .val_ltag = LTAG_STR,
  .tp_fields = NULL,
  .tp_val_size = NULL,
  .tp_new = rsp_str,
  .tp_init = NULL,
  .tp_copy = copy_str,
  .tp_prn = prn_str,
};

type_t CFILE_TYPE_OBJ = {
  .type = 0,
  .val_base_size = 16,
  .val_fixed_size  = 0,
  .val_cnum = C_INT64,
  .val_cptr = C_FILE_PTR,
  .tp_atomic = 0,
  .val_ltag = LTAG_CFILE,
  .tp_fields = NULL,
  .tp_val_size = NULL,
  .tp_new = rsp_cfile,
  .tp_init = NULL,
  .tp_copy = NULL,
  .tp_prn = NULL,
};


type_t* CORE_OBJECT_TYPES[MAX_CORE_OBJECT_TYPES] = {&LIST_TYPE_OBJ, &CONS_TYPE_OBJ,};

*/
