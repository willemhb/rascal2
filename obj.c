#include "rascal.h"

/* module-internal declarations */
node_t*  node_search(node_t*,val_t);
int32_t  node_insert(node_t**,node_t**,val_t);
int32_t  node_intern(node_t**,node_t**,chr8_t*,hash32_t);
int32_t  node_delete(node_t*,val_t);
node_t*  node_balance(node_t*);
node_t*  rotate_ll(node_t*,node_t*);
node_t*  rotate_rr(node_t*,node_t*);
node_t*  rotate_lr(node_t*,node_t*);
node_t*  rotate_rl(node_t*,node_t*);


/* memory management and bounds checking */
size_t calc_mem_size(size_t nbytes) {
  size_t basesz = max(nbytes, 16u);
  while (basesz % 16)
    basesz++;

  return basesz;
}

bool gc_check(size_t b, bool aligned)
{
  if (!aligned)
    b = calc_mem_size(b);

  return (FREE + b) >= (FROMSPACE + (HEAPSIZE * 8));
}

bool in_heap(val_t v, uchr8_t* base, uint64_t heapsz)
{
  uptr_t a = addr(v), b = (uptr_t)base, c = (uptr_t)(base + heapsz * 8);
  return a >= b && a <= c;
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

  if (gc_check(alloc_size,true))
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

size_t vm_allocsz_copies(type_t* to, size_t argc, val_t* args) { return to->tp_base * argc; }

inline size_t vm_allocsz_words(type_t* to, size_t argc, val_t* args) { return argc * 8; }

// entry point for non-builtin constructors and constructor calls that couldn't be resolved at compile time
val_t rsp_new(type_t* to, size_t argc, val_t* args)
{
  if (type_builtin_p(to))
    {
      cprim_t* tnw = type_new(to);
      if (tnw->vargs)
	return tnw->callable.varg_fun(argc,args);
      else
	return tnw->callable.n_farg_fun(args);
    }

  else
    {
      // get the total object size and allocate joined space, initialize the portion for the current type
      // and pass remaining arguments to the builtin initializer
      size_t tp_base = type_base(to), tp_nfields = type_nfields(to), sub_argc = argc - tp_nfields;
      val_t* sub_args = args + tp_nfields;

      size_t allcsz = type_alloc_sz(to)(type_origin(to),sub_argc, sub_args);

      uchr8_t* out = vm_alloc(tp_base, allcsz), *sub_out = out + tp_base;
      ((val_t*)out)[0] = ((val_t)(to)) | OTAG_OBJECT;

      memcpy(out + 8,(uchr8_t*)args, tp_base);

      type_init(to)(sub_out, sub_argc, sub_args);

      return (val_t)out | type_vtag(to);
    }
}

// reallocate 
val_t* vm_vec_realloc(vec_t* varr, size_t curr, size_t new)
{
  size_t new_sz = new + vec_initsz(varr) + vec_dynamic(varr);
  size_t old_sz = curr + vec_initsz(varr) + vec_dynamic(varr);
  
  val_t* newvals;
  if (gc_check(new_sz * 8,true))
    {
      DPUSH(tagp(varr));
      newvals = (val_t*)vm_alloc(0,new_sz);
      varr = ptr(vec_t*,DPOP());
    }

  val_t* cvals = vec_values(varr);
  for (size_t i = 0; i < old_sz; i++)
    newvals[i] = cvals[i];

  cvals[0] = R_FPTR;
  cvals[1] = (val_t)newvals;
  return newvals;
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
	  TARR[i] = gc_trace(TARR[i],VTAG_NONE);
	}
    }

  // trace the top level environment
  R_MAIN = gc_trace(R_MAIN,VTAG_NONE);

  // trace EVAL, DUMP, and the main registers
  for (size_t i = 0; i < SP; i++) EVAL[i] = gc_trace(EVAL[i],VTAG_NONE);
  for (size_t i = 0; i < DP; i++) DUMP[i] = gc_trace(DUMP[i],VTAG_NONE);
  for (size_t i = 0; i < 4; i++) REGISTERS[i] = gc_trace(REGISTERS[i],VTAG_NONE);

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
  size_t sz = type_size(tp) ? type_size(tp)(v) : type_base(tp);
  size_t asz = calc_mem_size(sz);
  uchr8_t* frm = ptr(uchr8_t*,v);
  memcpy(to,frm,asz);
  val_t out = (val_t)frm;
  // install the forwarding pointer
  car_(frm) = R_FPTR;
  cdr_(frm) = out | type_vtag(tp);
  return out;
}

val_t gc_trace(val_t v, vtag_t t)
{
  if (!isallocated(v) || in_heap(v,TOSPACE,HEAPSIZE * 8)) return v;
  else if (isfptr(car_(v)))
    {
      return trace_fp(v);
    }

  else
    {
      bool return_tagged;
      if (t == VTAG_NONE)
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
      type_t* to = val_type(v);

      switch (t)
	{
	 case VTAG_OBJECT:
	   varr = ptr(val_t*,v);
	   for (size_t i = 1; i <= type_nfields(to); i++)
	       varr[i] = gc_trace(varr[i],VTAG_NONE);
	   break;

	 case VTAG_NODE:	   
	   nd_left_(v) = (node_t*)gc_trace((val_t)nd_left_(v),VTAG_NODE);
	   nd_right_(v) = (node_t*)gc_trace((val_t)nd_right_(v),VTAG_NODE);
	   nd_data_(v) = gc_trace(nd_data_(v),VTAG_NONE);
	   break;

	 case VTAG_CONS:
	 case VTAG_LIST:
	   car_(v) = gc_trace(car_(v),VTAG_NONE);
	   cdr_(v) = gc_trace(cdr_(v),VTAG_NONE);
	   break;

	 case VTAG_TABLE:
	   tb_records_(v) = (node_t*)gc_trace((val_t)tb_records_(v),VTAG_NODE);
	   break;

	 case VTAG_VEC:
	   varr = vec_values_(v); elcnt = vec_datasz_(v);
	   for (size_t i = 0;i < elcnt; i++)
	     varr[i] = gc_trace(varr[i],VTAG_NONE);
	   
	   break;
	 case VTAG_METHOD:
	   meth_code_(v) = (vec_t*)gc_trace((val_t)meth_code_(v),VTAG_VEC);
	   meth_names_(v) = (table_t*)gc_trace((val_t)meth_names_(v),VTAG_TABLE);
	   meth_envt_(v) = (vec_t*)gc_trace((val_t)meth_envt_(v),VTAG_VEC);
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



inline val_t get_nd_key(node_t* n)
{
  val_t d = nd_data(n);
  return iscons(d) ? car_(d) : d;
}

inline val_t get_nd_binding(node_t* n)
{
  val_t d = nd_data(n);
  return iscons(d) ? ptr(val_t*,d)[1] : d;
}


list_t* mk_list(bool stacked, size_t argc, ...)
{
  if (!argc)
    return (list_t*)(R_NIL);

  val_t* stk;
  va_list vargs;
  va_start(vargs,argc);
  if (stacked)
    stk = va_arg(vargs,val_t*);

  else
    {
      val_t offset = stk_reserve(&DUMP,&DP,&DUMPSIZE,argc);
      stk = DUMP + offset;

      for (size_t i = 0; i < argc; i++)
	stk[i] = va_arg(vargs,val_t);
    }
  va_end(vargs);
  
  val_t out = rsp_new_list(argc,stk);

  DPOPN(argc);

  return (list_t*)out;
}

val_t rsp_new_list(size_t argc, val_t* args)
{
  uchr8_t* out = vm_alloc(0,16 * argc);
  cons_t* curr = (cons_t*)out;
  size_t i;

  for (i = 0; i < argc - 1; i++, curr++)
    {
      car_(curr) = args[i];
      cdr_(curr) = (val_t)(curr + 1);
    }

  car_(out) = args[i];
  cdr_(out) = 0;
  return (val_t)out;
}

cons_t* mk_cons(val_t ca, val_t cd)
{
  uchr8_t* hd;

  if (gc_check(16,true))
    {
      val_t base = DPUSHN(2,ca,cd);
      hd = vm_alloc(0,16);
      ca = DUMP[base];
      cd = DUMP[base+1];
      DPOPN(2);
    }

  else
    hd = vm_alloc(0,16);

  cons_t* out = (cons_t*)hd;
  car_(hd) = ca;
  cdr_(hd) = cd;

  return out;
}

val_t rsp_new_cons(val_t ca, val_t cd)
{
  cons_t* c = mk_cons(ca,cd);

  if (islist(cdr_(c)))
    return (val_t)c;

  else
    return tagp(c);

}


rstr_t* mk_str(chr8_t* s)
{
  size_t b = strsz(s);
  size_t t = calc_mem_size(b);
  uchr8_t* new;

  if (in_heap((val_t)s,FROMSPACE,HEAPSIZE) && gc_check(t,true))
    {
      DPUSH(tagp(s));
      new = vm_alloc(0,b);
      s = ptr(chr8_t*,DPOP());
    }

  else
    new = vm_alloc(0,b);

  strcpy((chr8_t*)new,s);
  return (chr8_t*)new;
}


val_t rsp_new_str(val_t a)
{
  vtag_t t = vtag(a);
  rstr_t* out;
  size_t csz;
  chr8_t buff[128];
  
  switch (t)
    {
      case VTAG_STR:
	out = mk_str(ptr(rstr_t*,a));
	break;

     case VTAG_SYM:
       out = mk_str(sm_name(a));
       break;

     case VTAG_INT:
       sprintf(buff,"%d",ival(a));
       out = mk_str(buff);
       break;

     case VTAG_FLOAT:
       sprintf(buff,"%.2f",fval(a));
       out = mk_str(buff);
       break;

     case VTAG_CHAR:
       csz = wctomb(buff,ival(a));
       buff[csz] = 0;
       out = mk_str(buff);
       break;

     default:
       rsp_perror(TYPE_ERR,"[%s:%d:%s] %s error: no method to convert string from type %s", type_name(a));
       rsp_raise(TYPE_ERR);
       break;
    }

  return tagp(out);
}

bstr_t* mk_bstr(size_t nb, uchr8_t* b)
{
  bstr_t* out = (bstr_t*)vm_alloc(8,nb);
  bs_sz(out) = nb;
  memcpy(bs_bytes(out),b,nb);
  return out;
}

val_t rsp_new_bstr(size_t argc, val_t* args)
{
  rstr_t* srcstr, *buffer; size_t sz; uint8_t fill;
  bstr_t* out;

  switch (argc)
    {
    case 1:
      srcstr = ecall(tostr,args[0]);
      sz = strsz(srcstr);
      buffer = (chr8_t*)vm_cmalloc(sz);
      memcpy(buffer,srcstr,sz);
      break;

    case 2: default:
      sz = ecall(toint,args[0]);
      fill = ecall(toint,args[1]);
      buffer = (chr8_t*)vm_cmalloc(sz);
      memset(buffer,fill,sz);
      break;
    }

  out = mk_bstr(sz,(uchr8_t*)buffer);
  vm_cfree((uchr8_t*)buffer);

  return tagp(out);
}


iostrm_t* mk_iostrm(chr8_t* fname, chr8_t* fmode)
{
  iostrm_t* out = fopen(fname,fmode);

  if (!out)
    {
      rsp_perror(IO_ERR,"[%s:%d:%s] %s error: %s", strerror(errno));
      errno = 0;
      rsp_raise(IO_ERR);
    }

  return out;
}

val_t rsp_new_iostrm(size_t argc, val_t* args)
{
  chr8_t* fmode, *fname;

  fname = strval(args[0]);

  if (argc == 2)
    fmode = strval(args[1]);

  else
    fmode = "r+";

  iostrm_t* out = mk_iostrm(fname,fmode);
  return tagp(out);

}

sym_t* vm_new_sym(chr8_t* sn, size_t ssz, hash32_t h, int32_t fl)
{
  sym_t* out;

  if (in_heap((val_t)sn,FROMSPACE,HEAPSIZE) && gc_check(8 + ssz,false))
    {
      DPUSH(tagp(sn));
      out = (sym_t*)vm_alloc(8,ssz);
      sn = ptr(chr8_t*,DPOP());
    }

  else
    out = (sym_t*)vm_alloc(8,ssz);

  ((val_t*)out)[0] = ((val_t)h) | fl;
  strcpy(sn,sm_name(out));

  return out;
}

sym_t* mk_gensym(chr8_t* sn, size_t ssz, int32_t fl)
{
  static int32_t GS_COUNTER = 0;
  chr8_t gsbuf[ssz+46];
  sym_t* out;

  if (ssz)
    sprintf(gsbuf,"__GS__%d__%s",GS_COUNTER,sn);
  else
    sprintf(gsbuf,"__GS__%d",GS_COUNTER);

  GS_COUNTER++;

  hash32_t h = hash_str(gsbuf);

  if (fl & SMFL_INTERNED)
    out = intern_string(gsbuf,ssz,h,fl);

  else
    out = vm_new_sym(gsbuf,ssz,h,fl);

  return out;
}

sym_t* mk_sym(chr8_t* sn, int32_t fl)
{
  size_t ssz = strsz(sn);
  
  if (fl & SMFL_GENSYM)
    return mk_gensym(sn,ssz,fl);

  hash32_t h = hash_str(sn);
  sym_t* out;

  if (fl & SMFL_INTERNED)
    out = intern_string(sn,ssz,h,fl);

  else
    out = vm_new_sym(sn,ssz,h,fl);

  return out;
}


sym_t* intern_string(chr8_t* sn, size_t ssz, hash32_t h, int32_t fl)
{

  if (in_heap((val_t)sn,FROMSPACE,HEAPSIZE) && gc_check(8 + ssz + 32,false))
    {
      DPUSH(tagp(sn));
      gc_run();
      sn = ptr(chr8_t*,DPOP());
    }

  symtab_t* st = ecall(tosymtab,R_SYMTAB);
  node_t** records = &tb_records(st), *rslt;
  sym_t* out;
  
  node_intern(records,&rslt,sn,h);

  if (nd_data(rslt) == R_UNBOUND)
    {
      out = vm_new_sym(sn,ssz,h,fl);
      nd_data(rslt) = tagp(out);
    }

  else
    {
      out = ptr(sym_t*,nd_data(rslt));
      nd_order(rslt) = tb_order(st);
      tb_order(st)++;
    }

  return out;
}

// add a node to an AVL tree, rebalancing if needed
node_t* node_search(node_t* n, val_t x)
{
  while (n)
    {
      int32_t result = vm_ord(x,nd_key(n));

      if (result < 0)
	n = nd_left(n);

      else if (result > 0)
	n = nd_right(n);

      else break;
    }

  return n;
}

int32_t node_insert(node_t** rt, node_t** new, val_t hk)
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


int node_intern(node_t** rt, node_t** sr, char* sn, hash32_t h)
{
  if (!(*rt)) // empty tree
    {
      node_t* new_nd = (node_t*)vm_alloc(32,0);
      nd_balance(new_nd) = 0;
      nd_data(new_nd) = R_UNBOUND;
      nd_left(new_nd) = nd_right(new_nd) = NULL;
      *rt = new_nd;
      *sr = new_nd;
      return 1;
    }

  else
    {
      sym_t* s = ptr(sym_t*,nd_data(*rt));
      int32_t result, b, comp = ordhash(h,sm_hash(s)) || u8strcmp(sn,sm_name(s));
      if (comp < 0)
	{
	  result = node_intern(&nd_left(*rt),sr,sn,h);
	  b = -1;
	}
      else if (comp > 0)
	{
	  result = node_intern(&nd_right(*rt),sr,sn,h);
	  b = 1;
	}
      else
	{
	  return 0;
	}

      b *= result;
      
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
  node_t* z = rotate_rr(y,nd_left(y));
  return rotate_ll(x,z);
}

node_t* rotate_rl(node_t*x, node_t* y) {
  node_t* z = rotate_ll(y,right_(y));
  return rotate_rr(x,z);
}


val_t     rsp_new_sym(size_t,val_t*);
method_t* mk_meth(table_t*,vec_t*,vec_t*,uint64_t);       // local names, bytecode, closure, flags
cprim_t*  mk_cprim(rcfun_t,size_t,uint64_t);              // callable, argcount, flags
vec_t*    mk_vec(otag_t,uint64_t,bool,size_t,...);        // otag, flags, whether the arguments are already stacked, and argcount
val_t     rsp_new_vec(size_t,val_t*);
node_t*   mk_node(size_t,val_t,bool);                     // order, hashkey, and whether to allocate space for bindings
table_t*  mk_table(bool,otag_t,size_t,...);
val_t     rsp_new_table(val_t*,int32_t);
val_t     mk_bool(int32_t);
val_t     rsp_new_bool(val_t*);
val_t     mk_char(int32_t);
val_t     rsp_new_char(val_t*);
val_t     mk_int(int32_t);
val_t     rsp_new_int(val_t*);
val_t     mk_float(flt32_t);
val_t     rsp_new_float(val_t*);
type_t*   mk_type(type_t*,size_t,...);              // mk_type is only called for extension types, so they require a parent (the default is 'object')            
val_t     rsp_new_type(type_t*,size_t,val_t*);




/*


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
