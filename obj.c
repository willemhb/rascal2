#include "rascal.h"

/* memory management and bounds checking */
static size_t calc_mem_size(size_t nbytes) {
  size_t basesz = max(nbytes, 16u);
  while (basesz % 16) basesz++;

  return basesz;
}

unsigned char* rsp_alloc(size_t nbytes) {
  // maintain padding and alignment
  size_t alloc_size = calc_mem_size(nbytes);
  if (heap_limit(alloc_size)) gc();

  unsigned char* out = FREE;
  FREE += alloc_size;

  return out;
}

unsigned char* rsp_allocw(size_t nwords) {
  size_t alloc_size = calc_mem_size(8 * nwords);
  if (heap_limit(alloc_size)) gc();

  unsigned char* out = FREE;
  FREE += alloc_size;

  return out;
}

unsigned char* rsp_alloco(type_t* to, size_t extra, size_t n)
{
  size_t each = calc_mem_size(to->val_base_size + extra);
  size_t total = each * n;
  unsigned char* out = rsp_alloc(total);
  return out;
}

val_t rsp_new(type_t* to, val_t* args, int argc) {
  rsp_cfunc_t afnc = to->tp_new;

  if (afnc == NULL) rsp_raise(TYPE_ERR);

  val_t new = afnc(args,argc);

  if (to->tp_init != NULL) to->tp_init(new,args,argc);

  return new;
}

val_t rsp_copy(type_t* tp, unsigned char* frm, unsigned char** to, size_t nbytes) {
  memcpy(frm,*to,nbytes);
  size_t padded_size = calc_mem_size(nbytes);
  unsigned char* new_head = *to;
  val_t out = tag_p(new_head,tp->val_ltag);
  *to += padded_size;
  // install the forwarding pointer
  car_(frm) = R_FPTR;
  cdr_(frm) = tag_p(new_head,tp->val_ltag);
  return out;
}

/* trace the global registers, stack, and symbol table */ 
void gc() {
  if (GROWHEAP) {
    HEAPSIZE *= 2;
    TOSPACE = realloc(TOSPACE, HEAPSIZE);
  }

  // reassign the head of the FREE area to point to the tospace
  FREE = TOSPACE;

  // trace the global symbol table and the top level environment
  gc_trace(&R_SYMTAB);
  gc_trace(&R_TOPLEVEL);

  // trace the EVAL stack
  for (size_t i = 0; i < SP; i++) gc_trace(EVAL + i);

  // trace all the registers
  gc_trace(&VALUE);
  gc_trace(&ENVT);
  gc_trace(&CONT);
  gc_trace(&TEMPLATE);

  // swap the fromspace & the tospace
  unsigned char* TMP = FROMSPACE;
  FROMSPACE = TOSPACE;
  TOSPACE = TMP;
  GROWHEAP = (FREE - FROMSPACE) > (HEAPSIZE * RAM_LOAD_FACTOR);
  
  return;
}

val_t gc_trace(val_t* v) {
  val_t vv = *v;
  val_t newl;
  ltag_t t = ltag(vv);

  // don't collect objects that live outside the heap or direct data
  if (!vv || t == LTAG_CFILE || t == LTAG_METAOBJ || t > 0xf) return vv;

  if (isfptr(car_(vv)))
    {
      newl = safe_ptr(vv);
      *v = newl;
      return newl;
    }

  else if (isstr(vv))
    {
      char* orig = tostr_(vv);
      *v = rsp_copy(CORE_OBJECT_TYPES[LTAG_STR],(unsigned char*)orig,&FREE,strsz(orig));
      return *v;
    }

  else if (issym(vv))
    {
      sym_t* orig = tosym_(vv);
      *v = rsp_copy(CORE_OBJECT_TYPES[LTAG_SYM],(unsigned char*)orig,&FREE,8+strsz(symname_(orig)));
      return *v;
    }

  // get the type object
  type_t* to = get_val_type(vv);
  // move the object to the new heap
  vv = to->tp_copy(to,vv);
  // apply the correct ltag
  *v = vv;
  return vv;
}

// simple limit checks
bool in_heap(val_t v, val_t* base, val_t* curr) {
  uptr_t a = addr(v), b = (uptr_t)base, c = (uptr_t)curr;
  return a >= b && a <= c;
}


bool heap_limit(unsigned long addition) {
  return (FREE + addition) >= (FROMSPACE + HEAPSIZE);
}

bool stack_overflow(val_t* base, unsigned long sptr, unsigned long addition) {
  return (base + STACKSIZE) < (base + sptr + addition);
}

/* 
   object apis 

   allocators

 */

inline list_t* new_list(int cnt) { return cnt == 0 ? NULL : (list_t*)rsp_allocw(cnt*2) ; }
inline cons_t* new_cons() { return (cons_t*)rsp_allocw(2); }
inline char* new_str(int ssz) { return (char*)rsp_alloc(ssz) ; }

sym_t* new_sym(int ssz, unsigned char** mem)
{
  
  unsigned char* m;
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


inline node_t* new_node(int nb, unsigned char** mem)
{
  unsigned char* m;
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

/* initializers */

int init_table(val_t t, val_t* args, int argc)
{
  table_t* tab = totable_(t);

  unsigned char* free = (unsigned char*)tab + 32; // reserved space
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

/* low level constructors */
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
  unsigned char* free = (unsigned char*)(names) + 32;

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

/* printers */

void prn_list(val_t ls, FILE* f) {
  fputwc('(',f);
  list_t* lsx = tolist_(ls);
  while (lsx) {
    vm_print(first_(lsx),f);

    if (rest_(lsx)) fputwc(' ',f);

    lsx = rest_(lsx);
  }

  fputwc(')',f);
  return;
}

void prn_cons(val_t c, FILE* f)
{
  fputwc('(',f);
  vm_print(car_(c),f);
  fputs(" . ",f);
  vm_print(cdr_(c),f);
  fputwc(')',f);
  return;
}

void prn_str(val_t s, FILE* f)
{
  fputwc('"',f);
  fputs(tostr_(s),f);
  fputwc('"',f);
}


void prn_sym(val_t s, FILE* f) {
  fputs(symname_(s),f);
  return;
}


static void tbl_prn_traverse(node_t* n, FILE* f)
{
  if (!n) return;

  vm_print(hashkey_(n),f);
  if (fieldcnt_(n))
    {
      fputs(" => ",f);
      vm_print(bindings_(n)[0],f);
    }

  if (left_(n))
    {
      fputwc(' ',f);
      tbl_prn_traverse(left_(n),f);
    }

  if (n->right)
    {
      fputwc(' ',f);
      tbl_prn_traverse(left_(n),f);
    }

  return;
}

void prn_table(val_t t, FILE* f)
{
  fputwc('{',f);
  tbl_prn_traverse(records_(t),f);
  fputwc('}',f);
}

void prn_bool(val_t b, FILE* f)
{
  if (b == R_TRUE) fputwc('t',f);
  else fputwc('f',f);

  return;
}

void prn_char(val_t c, FILE* f)
{
  fputwc('\\',f);
  fputwc(tochar_(c),f);
  return;
}

void prn_fvec(val_t v, FILE* f)
{
  fputs("#f[",f);
  val_t* el = tofvec_(v)->elements;
  int elcnt = tofvec_(v)->allocated;

  for (int i = 0; i < elcnt; i++) {
    vm_print(el[i],f);
    if (elcnt - i > 1) fputwc(' ', f);
  }

  fputwc(']',f);
  return;
}


void prn_dvec(val_t v, FILE* f)
{
  fputs("#d[",f);
  val_t* el = dvec_elements_(v);
  int elcnt = dvec_allocated_(v);

  for (int i = 0; i < elcnt; i++)
    {
      vm_print(el[i],f);
      if (elcnt - i > 1) fputwc(' ', f);
    }

  fputwc(']',f);
  return;
}

void prn_int(val_t i, FILE* f)
{
  fprintf(f,"%d",toint_(i));
  return;
}


/* copiers */
val_t copy_cons(type_t* to,val_t c)
{
  cons_t* co = tocons(c);
  gc_trace(&car_(co));
  gc_trace(&cdr_(co));
  return rsp_copy(to,(unsigned char*)co,&TOSPACE,16);
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
  return rsp_copy(to,(unsigned char*)addr(c),&FREE,8*(4+fieldcnt_(c)));
}

val_t copy_fvec(type_t* to, val_t fv)
{
  fvec_t* fvo = tofvec_(fv);
  val_t allc = fvec_allocated_(fvo);
  val_t* elements = fvec_elements_(fvo);

  for (size_t i = 0; i < allc;i++)
    {
      gc_trace(elements+i);
    }
  return rsp_copy(to,(unsigned char*)fvo,&FREE,8*(2+allc));
}

val_t copy_dvec(type_t* to, val_t dv)
{
  dvec_t* dvo = todvec_(dv);
  gc_trace(dvec_elements_(dvo));
  return rsp_copy(to,(unsigned char*)dvo,&FREE,16);
}

val_t copy_function(type_t* to, val_t fnc)
{
  function_t* fnco = tofunction_(fnc);
  gc_trace(&template_(fnco));
  gc_trace(&envt_(fnco));
  gc_trace(&closure_(fnco));
  return rsp_copy(to,(unsigned char*)fnco,&FREE,48);
}

/* access, search, and mutation API functions */
val_t _car(val_t c, const char* fl, int ln, const char* fnm) {
  if (hascar(c)) return car_(c);
  else {
    _rsp_perror(fl,ln,fnm,TYPE_ERR,"Expected cons or list, got %s",get_val_type_name(c));
    rsp_raise(TYPE_ERR);
  }
  return R_NONE;
}

val_t _cdr(val_t c, const char* fl, int ln, const char* fnm) {
  if (hascar(c)) return cdr_(c);
  else
    {
    _rsp_perror(fl,ln,fnm,TYPE_ERR,"Expected cons or list, got %s",get_val_type_name(c));
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

int node_insert(node_t** rt, node_t** new, val_t hk, int order, int nsz, unsigned char* m)
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
      unsigned char* m = rsp_alloc(32 + 8 + sz);
      node_t* new_nd = new_node(0,&m);
      sym_t* new_sm = new_sym(sz,&m);
      *sr = new_sm;
      hashkey_(new_nd) = new_sm;
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
    _rsp_perror(fl,ln,fnc,TYPE_ERR,"Expected vector type, got %s",get_val_type_name(v));
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
    _rsp_perror(fl,ln,fnc,TYPE_ERR,"Expected vector type, got %s",get_val_type_name(v));
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
    _rsp_perror(fl,ln,fnc,TYPE_ERR,"Expected vector type, got %s",get_val_type_name(v));
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
      rsp_perror(TYPE_ERR,"non-environtment of type %s has no closure.",get_val_type_name(e));
      rsp_raise(TYPE_ERR);
      return 0;
    }
}

val_t env_extend(val_t nm, val_t e)
{
  if (!issym(nm))
    {
      rsp_perror(TYPE_ERR,"Expected type sym, got %s",get_val_type_name(nm));
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
      rsp_perror(ENVT_ERR,"Invalid envt of type %s.",get_val_type_name(e));
      rsp_raise(ENVT_ERR);
      return 0;
    }
}

val_t env_assign(val_t nm, val_t e, val_t b)
{
  if (!issym(nm))
    {
      rsp_perror(ENVT_ERR,"Expected type sym, got %s.",get_val_type_name(nm));
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
      rsp_perror(TYPE_ERR,"Non environment of type %s.",get_val_type_name(e));
      rsp_raise(TYPE_ERR);
      return 0;
    }
}

val_t env_lookup(val_t nm, val_t e)
{
  if (!issym(nm))
    {
      rsp_perror(ENVT_ERR,"Expected type sym, got %s.",get_val_type_name(nm));
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
      rsp_perror(TYPE_ERR,"Non environment of type %s.",get_val_type_name(e));
      rsp_raise(TYPE_ERR);
      return 0;
    }
}

/*
    OBJECT_HEAD;
  val_t parent;                          // pointer to the parent type
  
  val_t val_base_size     : 47;         // the minimum size for objects of this type
  val_t val_fixed_size    :  1;
  val_t val_cnum          :  8;         // the C numeric type of the value representation
  val_t val_cptr          :  3;         // the C pointer type of the value representation
  val_t tp_atomic         :  1;        // legal for use in a plain dict
  val_t val_ltag          :  4;

  table_t* tp_readable;                               // a dict mapping rascal-accessible fields to offsets
  table_t* tp_writeable;                              // a dict mapping rascal-writeable fields to offsets
  int   (*tp_val_size)(val_t);                          // called when the size can't be determined from flags alone
  val_t (*tp_new)(val_t*,int);                          // the rascal callable constructor
  int   (*tp_init)(val_t,val_t*,int);                   // the initializer
  val_t (*tp_copy)(type_t*,val_t);                      // garbage collector
  void  (*tp_prn)(val_t,FILE*);                         // used to print values

  C_UINT8   =0b000010,
  C_INT8    =0b000001,
  C_UINT16  =0b000110,
  C_INT16   =0b000011,
  C_UINT32  =0b001110,
  C_INT32   =0b000111,
  C_INT64   =0b001111,
  C_FLOAT32 =0b100111,
  C_FLOAT64 =0b101111,

  C_PTR_NONE     =0b000,   // value is the type indicated by c_num_t
  C_PTR_VOID     =0b001,   // value is a pointer of unknown type
  C_PTR_TO       =0b010,   // value is a pointer to the indicated type
  C_FILE_PTR     =0b011,   // special case - value is a C file pointer
  C_BLTN_PTR     =0b100,   // special case - value is a C function pointer to a builtin function
  C_STR_PTR      =0b101,   // special case - a common string pointer
*/

#define ALIGNED(x) __attribute__((aligned(x)))


type_t CONS_TYPE_OBJ = {
  .type = 0,
  .val_base_size = 16,
  .val_fixed_size  = 1,
  .val_cnum = C_INT64,
  .val_cptr = C_PTR_VOID,
  .tp_atomic = 0,
  .val_ltag = LTAG_CONS,
  .tp_readable = NULL,
  .tp_writeable = NULL,
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
  .tp_readable = NULL,
  .tp_writeable = NULL,
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
  .tp_readable = NULL,
  .tp_writeable = NULL,
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
  .tp_readable = NULL,
  .tp_writeable = NULL,
  .tp_val_size = NULL,
  .tp_new = rsp_cfile,
  .tp_init = NULL,
  .tp_copy = NULL,
  .tp_prn = NULL,
};


type_t* CORE_OBJECT_TYPES[MAX_CORE_OBJECT_TYPES] = {&LIST_TYPE_OBJ, &CONS_TYPE_OBJ,};
