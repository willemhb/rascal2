#include "rascal.h"

/* tag manipulation */
// extract the tag and the address
inline int ltag(val_t v) { int nt = ntag(v); return nt <= 0xf ? nt : (int)wtag(v); }
inline bool isdirect(val_t v)
{
  int t = ltag(v);
  return t > 0xf ? t != LTAG_BUILTIN : !v;
}

inline bool iscallable(val_t v)
{
  int t = ltag(v);
  return t == LTAG_FUNCOBJ || t == LTAG_BUILTIN || t == LTAG_METAOBJ;
}

inline bool isatomic(val_t v) { return get_val_type(v)->tp_atomic; }

inline bool isnumeric(val_t v)
{
  int t = ltag(v);
  return t == LTAG_BIGNUM || t == LTAG_INT || t == LTAG_FLOAT;
}

inline bool hasflags(val_t v) {
  switch(ltag(v)) {
  case LTAG_SYM:
  case LTAG_DVEC:
  case LTAG_TABLEOBJ:
  case LTAG_OBJECT:
  case LTAG_FUNCOBJ:
    return true;
  default:
    return false;
  }
}

inline int oflags(val_t v) { return hasflags(v) ? flags_(v) : FLAG_NONE; }

// get the type object associated with a value
type_t* get_val_type(val_t v) {
  int t = ltag(v);

  if (t == LTAG_LIST) return (t ? CORE_OBJECT_TYPES : CORE_DIRECT_TYPES)[t];
  else if (t < LTAG_OBJECT) return CORE_OBJECT_TYPES[t];
  else if (t > LTAG_METAOBJ) return CORE_DIRECT_TYPES[t>>4];
  else return (type_t*)(type_(v) & (~0xful));
}

int get_val_size(val_t v) {
  if (isdirect(v)) return 8;

  type_t* to = get_val_type(v);
  
  if (to->val_fixed_size) return to->val_base_size;

  else return to->tp_val_size(v); 
}

int val_len(val_t v)
{
  switch (ltag(v))
    {
    case LTAG_CONS: return 2;
    case LTAG_LIST:
      {
	list_t* lv = tolist_(v);
	size_t l = 0;
	while (lv)
	  {
	    l++; lv = rest_(lv);
	  }

	return l;
      }
    case LTAG_FVEC: return fvec_allocated_(v);
    case LTAG_DVEC: return dvec_curr_(v);
    case LTAG_STR: return u8strlen(tostr_(v));
    case LTAG_TABLEOBJ: return count_(v);
    default:
      {
	rsp_raise(TYPE_ERR);
	return 0;
      }
    }
}

val_t rsp_itof(val_t v) {
  rsp_c64_t cv = { .value = v };
  cv.direct.data.float32 = (float)cv.direct.data.integer;
  cv.direct.lowtag = LTAG_FLOAT;
  return cv.value;
}

val_t rsp_ftoi(val_t v) {
  rsp_c64_t cv = { .value = v };
  cv.direct.data.integer = (int)cv.direct.data.float32;
  cv.direct.lowtag = LTAG_INT;
  return cv.value;
}

inline char* get_val_type_name(val_t v) { return get_val_type(v)->name; }
inline c_num_t get_val_cnum(val_t v) { return get_val_type(v)->val_cnum; }
inline c_num_t get_common_cnum(val_t x, val_t y) { return get_val_cnum(x) | get_val_cnum(y) ; }

/* safety functions */
#define SAFECAST(ctype,rtype,test,cnvt)	                                       \
  ctype _to ## rtype(val_t v,const char* fl, int ln, const char* fnc)          \
  {                                                                            \
    if (!test(v))                                                              \
  {                                                                            \
    _rsp_perror(fl,ln,fnc,TYPE_ERR,TYPEERR_FMT,#rtype,get_val_type_name(v)) ;  \
    rsp_raise(TYPE_ERR) ;						       \
    return (ctype)0;                                                           \
  }                                                                            \
   return cnvt(safe_ptr(v));						       \
  }

SAFECAST(list_t*,list,islist,tolist_)
SAFECAST(cons_t*,cons,iscons,tocons_)
SAFECAST(char*,str,isstr,tostr_)
SAFECAST(FILE*,cfile,iscfile,tocfile_)
SAFECAST(sym_t*,sym,issym,tosym_)
SAFECAST(node_t*,node,isnode,tonode_)
SAFECAST(fvec_t*,fvec,isfvec,tofvec_)
SAFECAST(dvec_t*,dvec,isdvec,todvec_)
SAFECAST(code_t*,code,iscode,tocode_)
SAFECAST(obj_t*,obj,isobj,toobj_)
SAFECAST(table_t*,table,istable,totable_)
SAFECAST(function_t*,function,isfunction,tofunction_)
SAFECAST(type_t*,type,istype,totype_)
SAFECAST(int,rint,isint,toint_)
SAFECAST(float,rfloat,isfloat,tofloat_)
SAFECAST(wchar_t,rchar,ischar,tochar_)
SAFECAST(rsp_cfunc_t,builtin,isbuiltin,tobuiltin_)

/* safecasts and checking forwarding pointers */
// follow a forwarding pointer without updating it
val_t safe_ptr(val_t v) {
  if (!isobj(v)) return v;
  while (tocons_(v)->car == R_FPTR) v = tocons_(v)->cdr;
  return v;
}

val_t check_fptr(val_t* v) {
  if (!isobj(*v)) return *v;
  return update_fptr(v);
}

val_t update_fptr(val_t* v) {
  // follow the forwarding pointer possibly more than once
  while (tocons_(*v)->car == R_FPTR) *v = tocons_(*v)->cdr;

  return *v;
}

inline bool cbool(val_t v) {
  if (v == R_NIL) return false;
  if (v == R_NONE) return false;
  if (v == R_FALSE) return false;
  return true;
}

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

val_t rsp_new(type_t* to, val_t* args, int argc) {
  rsp_cfunc_t afnc = to->tp_new;

  if (afnc == NULL) rsp_raise(TYPE_ERR);

  val_t new = afnc(args,argc);

  if (to->tp_init != NULL) to->tp_init(new,args,argc);

  return new;
}

unsigned char* rsp_copy(unsigned char* frm, unsigned char* to, size_t nbytes) {
  memcpy(frm,to,nbytes);
  size_t padded_size = calc_mem_size(nbytes);
  return to + padded_size;
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
  ltag_t t = ltag(vv);

  // don't collect objects that live outside the heap or direct data
  if (!vv || t == LTAG_CFILE || t == LTAG_METAOBJ || t > 0xf) return vv;

  // get the type object
  type_t* to = get_val_type(vv);
  // move the object to the new heap
  vv = to->tp_copy(vv);
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

/* object apis */
// low level constructors - make rascal objects from existing C data or allocate appropriate space for an object of the desired type
// simple low-level constructors
inline val_t mk_list(int cnt)
{
  return cnt == 0 ? R_NIL : tag_p(rsp_allocw(cnt * 2), LTAG_LIST);
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

inline cons_t* new_cons() { return (cons_t*)rsp_allocw(2); }
inline val_t mk_cons() { return tag_p(new_cons(),LTAG_CONS); }

inline val_t vm_cons(val_t ca, val_t cd)
{
  PUSH(ca);
  PUSH(cd);
  cons_t* c = new_cons();
  cdr_(c) = POP();
  car_(c) = POP();
  if (islist(cd)) return tag_p(c,LTAG_LIST);
  else return tag_p(c,LTAG_CONS);
}

int init_cons(val_t new, val_t* argv, int argc) {
  car_(new) = argv[1];
  cdr_(new) = argv[0];
  return 0;
}

void prn_cons(val_t c, FILE* f) {
  fputwc('(',f);
  vm_print(car_(c),f);
  fputs(" . ",f);
  vm_print(cdr_(c),f);
  fputwc(')',f);
  return;
}

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


inline val_t mk_cfile(FILE* f) { return tag_p(f,LTAG_CFILE); }

val_t new_sym(char* s, size_t ssz, hash64_t h, int flags,unsigned char* mem)
{
  if (mem == NULL) mem = rsp_alloc(8+ssz);
  h = (h & (~0xful)) | flags;
  sym_t* new = (sym_t*)mem;
  symhash_(new) = h;
  memcpy(symname_(new),s,ssz);
  return tag_p(new,LTAG_SYM);
}

inline node_t* new_node(int nb, int order, unsigned char* m)
{
  if (m == NULL) m = rsp_allocw(4+nb);
  node_t* new = (node_t*)m;
  balance_(new) = 3;
  order_(new) = order;
  fieldcnt_(new) = nb;
  left_(new) = right_(new) = NULL;
  return new;
}

inline val_t mk_node(int nb, int order, unsigned char* m)
{
  return tag_p(new_node(nb,order,m),LTAG_NODE);
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
      
      node_t* new_nd = new_node(nsz,order,m);
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
      node_t* new_nd = new_node(0,order,m);
      val_t new_sm = new_sym(sn,sz,h,fl,m + 32);
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

inline val_t mk_table(int cnt, int rsz)
{
  rsz = min(rsz,2);
  table_t* newt = (table_t*)rsp_allocw(4 + ((4 + rsz) * cnt));
  count_(newt) = 0;
  records_(newt) = NULL;
  newt->type = ((val_t)(CORE_OBJECT_TYPES[LTAG_TABLEOBJ])) | rsz;
  return tag_p(newt,LTAG_TABLEOBJ);
}

int nodesz(table_t* t)
{
  switch (flags_(t))
    {
    case TABLEFLAG_ENVT: case TABLEFLAG_SYMTAB: return 0;
    default: return 2;
    }
}

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

static void tbl_prn_traverse(node_t* n, FILE* f) {
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

void prn_table(val_t t, FILE* f) {
  fputwc('{',f);
  tbl_prn_traverse(records_(t),f);
  fputwc('}',f);
}


int vm_ord(val_t x, val_t y)
{
  if (!isatomic(x) || !isatomic(y)) rsp_raise(TYPE_ERR);
  type_t* tx = get_val_type(x), *ty = get_val_type(y);

  if (tx != ty) return compare((val_t)tx,(val_t)ty);

  switch (tx->val_ltag) {
  case LTAG_STR:
    return u8strcmp(tostr_(x),tostr_(y));
  case LTAG_SYM:
    return compare(symhash_(x),symhash_(y)) ?: u8strcmp(symname_(x),symname_(y));
  case LTAG_INT:
    return compare(toint_(x),toint_(y));
  case LTAG_CHAR:
    return compare(tochar_(x),tochar_(y));
  case LTAG_FLOAT:
    return compare(tofloat_(x),tofloat_(y));
  default:
    return compare(x,y);
  }
}

// vector API
val_t mk_fvec(int elements) {
  fvec_t* out = (fvec_t*)rsp_allocw(2 + elements);
  out->allocated = elements;
  return tag_p(out,LTAG_FVEC);
}

void prn_fvec(val_t v, FILE* f) {
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

val_t mk_dvec(int elements)
{
  elements = elements < 32 ? 32 : cpow2_32(elements);
  val_t store = mk_fvec(elements);
  PUSH(store); // in case allocating the dynamic part initiates GC, save the store
  dvec_t* out = (dvec_t*)rsp_allocw(4);
  out->curr = 0;
  out->elements = POP();

  return tag_p(out,LTAG_DVEC);
}

void prn_dvec(val_t v, FILE* f) {
  fputs("#d[",f);
  val_t* el = dvec_elements_(v);
  int elcnt = dvec_allocated_(v);

  for (int i = 0; i < elcnt; i++) {
    vm_print(el[i],f);
    if (elcnt - i > 1) fputwc(' ', f);
  }

  fputwc(']',f);
  return;
}

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

// more complex constructors
val_t mk_str(const char* s) {
  // if the string is properly aligned just return a tagged pointer
  if (aligned(s,16)) return ((val_t)s) | LTAG_STR;
  int i = strsz(s);
  unsigned char* new = rsp_alloc(i);
  memcpy(new,s,i);

  return tag_p(new,LTAG_STR);
}

void prn_str(val_t s, FILE* f) {
  fputwc('"',f);
  fputs(tostr_(s),f);
  fputwc('"',f);

}

// symbols
val_t mk_sym(char* s, int flags) {
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
      else return new_sym(gsbuf,ssz,h,flags,NULL);
  }

  h = hash_str(s);
  ssz = strsz(s);

  if (flags & SYMFLAG_INTERNED) return intern_string(R_SYMTAB,s,h,flags);
  else return new_sym(s,h,ssz,flags,NULL);
}


void prn_sym(val_t s, FILE* f) {
  fputs(symname_(s),f);
  return;
}

inline hash64_t symhash(val_t s) { return tosym(s)->hash & (~0xful); }
inline char* symname(val_t s) { return tosym(s)->name; }

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
  function_t* fnc = (function_t*)rsp_allocw(6);
  locals = POP();
  envt = POP();
  tmpl = POP();
  type_(fnc) = ((val_t)CORE_OBJECT_TYPES[LTAG_FUNCOBJ]) | flags;
  fnargco_(fnc) = count_(locals);
  template_(fnc) = tmpl;
  closure_(fnc) = envt;
  envt_(fnc) = locals;

  return tag_p(fnc,LTAG_FUNCOBJ);
}

// integers
inline val_t mk_int(int i) { return tag_v(i,LTAG_INT); }
void prn_int(val_t i, FILE* f)
{
  fprintf(f,"%d",toint_(i));
  return;
}

// characters
inline val_t mk_char(int c) { return c < MIN_CODEPOINT || c > MAX_CODEPOINT ? R_EOF :  tag_v(c,LTAG_CHAR); }

void prn_char(val_t c, FILE* f) {
  fputwc('\\',f);
  fputwc(tochar_(c),f);
  return;
}

// booleans
inline val_t mk_bool(int b) { return b ? R_TRUE : R_FALSE ; }

void prn_bool(val_t b, FILE* f) {
  if (b == R_TRUE) fputwc('t',f);
  else fputwc('f',f);

  return;
}

// environment API
static int check_argl(val_t argl, int* flags)
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

val_t mk_envframe(val_t e, val_t cls, int argc)
{
  PUSH(e);
  PUSH(cls);
  val_t frm = mk_dvec(argc + 2);
  dvec_t* dv = todvec_(frm);
  cls = POP();
  e = POP();
  dvec_elements_(dv)[0] = e;
  dvec_elements_(dv)[1] = cls;
  dv->elements |= DVECFLAG_ENVTFRAME;
  dv->curr = 2;
  return frm;
}

val_t save_cnt(val_t lbl, val_t* stk, int argc)
{
  val_t frm = mk_fvec(5 + argc);
  // save state
  val_t* rxs = fvec_elements_(frm);
  rxs[0] = lbl;
  rxs[1] = ENVT;
  rxs[2] = TEMPLATE;
  rxs[3] = PC;
  rxs[4] = CONT;

  for (int i = argc-1, j=5; i >= 0; i++,j++)
    {
      rxs[j] = stk[i];
    }

  POPN(argc); // remove the saved elements from the stack
  return frm;
}

val_t restore_cnt(val_t c)
{
  fvec_t* cfrm = tofvec(c);
  val_t* rxs = fvec_elements_(cfrm);
  ENVT = rxs[1];
  TEMPLATE = rxs[2];
  PC = rxs[3];
  CONT = rxs[4];

  // restore the contents of frame to the EVAL stack
  for (size_t i = 5; i < fvec_allocated_(cfrm); i++)
    {
      PUSH(rxs[i]);
    }

  // return the continuation label
  return rxs[0];
}

// environment API
val_t env_next(val_t e)
{
  if (isenvtframe(e))
    {
      return dvec_elements_(e)[1];
    }
  else if (isnmspc(e))
    {
      return totable_(e)->_table_fields[0];
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

// vm builtins and API
static rsp_label_t vm_analyze(val_t exp)
{
  switch (ltag(exp))
    {
    default: return LBL_LITERAL;
    case LTAG_SYM: return LBL_SYMBOL;
    case LTAG_LIST:
      {
	if (!exp) return LBL_LITERAL;
	val_t f = first_(exp);

	if (f == F_SETV) return LBL_SETV;
	else if (f == F_DEF) return LBL_DEF;
	else if (f == F_QUOTE) return LBL_QUOTE;
	else if (f == F_IF) return LBL_IF;
	else if (f == F_FUN) return LBL_FUN;
	else if (f == F_MACRO) return LBL_MACRO;
	else if (f == F_DO) return LBL_DO;
	else if (f == F_LET) return LBL_LET;
	else return LBL_FUNAPP;
      }
    }
}

val_t rsp_eval(val_t exp, val_t env)
{
  static void* ev_labels[] = {
    &&ev_literal, &&ev_symbol, &&ev_quote,
    &&ev_def, &&ev_setv, &&ev_fun, &&ev_macro,
    &&ev_let, &&ev_do, &&ev_if, &&ev_funapp,
  };

  val_t* a, *b, *c, *n, *names, *exprs, *fun, *args, *appenv, x, test, localenv, rslt;
  bool evargs;
  size_t bltnargco;
  val_t OSP = SP;            // save the original stack pointer (restored on return)
  val_t* envp = SAVE(env);

  goto *ev_labels[vm_analyze(exp)];

 ev_return:
  SP = OSP;     // restore stack state
  return rslt;

 ev_literal:
  rslt = exp;

  goto ev_return;

 ev_symbol:
  rslt = iskeyword(exp) ? exp : env_lookup(*envp,exp);

  goto ev_return;

 ev_quote:
  rslt = car(cdr(exp));

  goto ev_return;

 ev_def:
  n = SAVE(car(cdr(exp)));
  b = SAVE(car(cdr(cdr(exp))));
  env_extend(*envp,*n);
  c = SAVE(rsp_eval(*b,*envp));
  rslt = env_assign(*envp,*n,*c);

  goto ev_return;

 ev_setv:
  n = SAVE(car(cdr(exp)));
  b = SAVE(car(cdr(cdr(exp))));
  c = SAVE(rsp_eval(*b,*envp));
  rslt = env_assign(*envp,*n,*c);

  goto ev_return;

 ev_fun:
  n = SAVE(car(cdr(exp)));
  exprs = SAVE(cdr(cdr(exp)));
  rslt = mk_function(*exprs,*n,*envp,0);

  goto ev_return;

 ev_macro:
  n = SAVE(car(cdr(exp)));
  exprs = SAVE(cdr(cdr(exp)));
  rslt = mk_function(*exprs,*n,*envp,FUNCFLAG_MACRO);

  goto ev_return;

 ev_let:
  names = SAVE(car(cdr(exp)));
  exprs = SAVE(cdr(cdr(exp)));
  localenv = mk_envt(R_NIL,NULL);
  *envp = mk_envframe(localenv,*envp,0);

  // bind the names and arguments in the environment
  while (*names)
    {
      env_extend(*envp,car(*names));
      *names = cdr(*names);
      x = *names ? rsp_eval(car(*names),*envp) : R_UNBOUND;
      vec_append(x,*envp);
      if (*names) *names = cdr(*names);
    }

  rslt = rsp_evlis(*exprs,*envp);

  goto ev_return;

 ev_do:
  rslt = rsp_evlis(cdr(exp),*envp);

  goto ev_return;

 ev_if:
  exprs = SAVE(cdr(exp));

  while (*exprs)
    {
      
      if (isnil(cdr(*exprs))) break;

      else
	{
	  x = car(*exprs);
	  *exprs = cdr(*exprs);
	  test = rsp_eval(x,*envp);

	  if (cbool(test)) break;
	  else *exprs = cdr(*exprs);
	}
    }

  rslt = *exprs ? rsp_eval(car(*exprs),*envp) : R_NIL;

  goto ev_return;

 ev_funapp:
  fun = SAVE(rsp_eval(car(exp),*envp));
  args = SAVE(cdr(exp));
  if (isbuiltin(*fun)) goto ev_builtin;
  appenv = SAVE(mk_envframe(envt_(*fun),*envp,fnargco_(*fun)));
  evargs = !ismacro(*fun);

  for (;*args;*args=cdr(*args))
    {
      if (evargs) vec_append(*appenv,rsp_eval(car(*args),*envp));
      else vec_append(*appenv,car(*args));
    }

  rslt = rsp_evlis(template_(*fun),*appenv);
  if (ismacro(*fun)) rslt = rsp_eval(rslt,*envp);

  goto ev_return;

 ev_builtin:
  for (bltnargco = 0;*args;*args=cdr(*args),bltnargco++)
    {
      x = rsp_eval(car(*args),*envp);
      PUSH(x);
    }
  rsp_cfunc_t bltn = tobuiltin_(*fun);
  rslt = bltn(EVAL,bltnargco);
  
  goto ev_return;
}

val_t rsp_evlis(val_t exprs, val_t env)
{
  val_t curr;
  val_t* envp = SAVE(env);
  val_t* seq  = SAVE(exprs);

  while (*seq)
    {
      curr = rsp_eval(car(*seq),*envp);
      *seq = cdr(*seq);
    }

  POPN(2);
  return curr;
}

// a mediocre implementation apply
val_t rsp_apply(val_t fun, val_t argls)
{
  val_t OSP = SP;
  val_t* sfun = SAVE(fun);
  val_t* sargls = SAVE(argls);
  val_t* cls, *frm, x, rslt;
  size_t bltnargco; bool evargs;

  if (isbuiltin(*sfun)) goto ev_builtin;
  else goto ev_funapp;

 
  
 ev_funapp:
  cls = SAVE(closure_(*sfun));
  frm = SAVE(mk_envtframe(envt_(*sfun),*cls,fnargco_(*sfun)));
  evargs = !ismacro(*sfun);

  for (;*sargls;*sargls=cdr(*sargls))
    {
      x = car(*sargls);
      if (evargs) x = rsp_eval(x,*cls);
      vec_append(*frm,x);
    }

  rslt = rsp_evlis(template_(*sfun),*frm);
  if (evargs) rslt = rsp_eval(rslt,*cls);

  goto ev_return;

 ev_builtin:
  for (bltnargco = 0;*sargls;*sargls=cdr(*sargls),bltnargco++)
    {
      x = rsp_eval(car(*sargls),R_TOPLEVEL);
      PUSH(x);
    }

  rsp_cfunc_t bltn = tobuiltin_(*sfun);
  rslt = bltn(EVAL,bltnargco);

  goto ev_return;

 ev_return:
  SP = OSP;
  return rslt;
}

// reader support functions
#define TOKBUFF_SIZE 4096
char TOKBUFF[TOKBUFF_SIZE];
int TOKPTR = 0;
int LASTCHR;                  // to keep track of escapes
rsp_tok_t TOKTYPE = TOK_NONE;

static int accumtok(wint_t c)
{
  if (TOKBUFF_SIZE < (TOKPTR + 4))
    {
      rsp_perror(BOUNDS_ERR,"Token too long.");
      rsp_raise(BOUNDS_ERR);
      return -1;
    }

  else
    {
      int csz = wctomb(TOKBUFF+TOKPTR,c);
      TOKPTR += csz;
      return csz;
    }
}

// reset reader state
static inline void take() { TOKTYPE = TOK_NONE ; TOKPTR = 0; return; }

static inline rsp_tok_t finalize_void_tok(rsp_tok_t tt)
{
  TOKTYPE = tt;
  return tt;
}

static inline rsp_tok_t finalize_tok(wint_t c, rsp_tok_t tt)
{
  TOKTYPE = tt;
  accumtok(c);
  if (c != U'\0') TOKBUFF[++TOKPTR] = U'\0';
  return tt;
}

static rsp_tok_t get_char_tok(FILE* f)
{
  wint_t wc = peekwc(f);
  if (wc == WEOF)
    {
      TOKTYPE = TOK_STXERR;
      rsp_perror(SYNTAX_ERR,"Unexpected EOF reading character.");
      rsp_raise(SYNTAX_ERR);
    }

  if (wc != U'u' && wc != U'U') return finalize_tok(wc,TOK_CHAR);

  else while ((wc = peekwc(f)) != WEOF && !iswspace(wc))
	 {
	   if (!iswdigit(wc))
	     {
	       TOKTYPE = TOK_STXERR;
	       rsp_perror(SYNTAX_ERR,"Non-numeric character in codepoint literal.");
	       rsp_raise(SYNTAX_ERR);
	     }
	   accumtok(wc);
	 }

  if (wc == WEOF && TOKPTR == 0)
    {
	  TOKTYPE = TOK_STXERR;
	  rsp_perror(SYNTAX_ERR,"Unexpected EOF reading character.");
	  rsp_raise(SYNTAX_ERR);
    }

      return finalize_tok(U'\0',TOK_UNICODE);
    }

static rsp_tok_t get_str_tok(FILE* f)
{
  wint_t wc; LASTCHR = '"';

  while ((wc = peekwc(f)) != WEOF && !(wc == '"' && LASTCHR != '\\'))
    {
      accumtok(wc);
    }

  if (wc == WEOF)
    {
	  TOKTYPE = TOK_STXERR;
	  rsp_perror(SYNTAX_ERR,"Unexpected EOF reading character.");
	  rsp_raise(SYNTAX_ERR);
    }

  return finalize_tok(U'\0',TOK_STR);
}

static rsp_tok_t get_symbolic_tok(FILE* f, wint_t fc)
{
  bool hasdot = fc == U'.';
  bool hasint = iswdigit(fc) || fc == U'+' || fc == U'-';
  bool hasfloat = iswdigit(fc) || fc == U'+' || fc == U'-' || hasdot;
  int wc;

  while ((wc = peekwc(f)) != EOF && !iswspace(wc) && wc != U')')
    {
      if (!iswdigit(wc))
	{
	  hasint = false;
	  hasfloat = wc == U'.' && !hasdot;
	  hasdot = true;
	}

      accumtok(wc);
    }

  if (hasint) return finalize_tok(U'\0',TOK_INT);
  else if (hasfloat) return finalize_tok(U'\0',TOK_FLOAT);
  else return finalize_tok(U'\0',TOK_SYM);
}

rsp_tok_t vm_get_token(FILE* f)
{
  if (TOKTYPE != TOK_NONE) return TOKTYPE;

  wint_t wc = fskip(f); // skip initial whitespace
  if (wc == U';')       // skip comments as well
    {
      wc = fskipto(f, U'\n');
    }

  if (wc == WEOF)
    {
      TOKTYPE = TOK_EOF;
      return TOKTYPE;
    }

  switch (wc)
    {
    case U'(' : return finalize_void_tok(TOK_LPAR);
    case U')' : return finalize_void_tok(TOK_RPAR);
    case U'.' : return finalize_void_tok(TOK_DOT);
    case U'\'': return finalize_void_tok(TOK_QUOT);
    case U'\\': return get_char_tok(f);
    case U'\"' : return get_str_tok(f);
    default:
      {
	accumtok(wc);
	return get_symbolic_tok(f,wc);
      }
    }
}

val_t vm_read_expr(FILE* f)
{
  rsp_tok_t tt = vm_get_token(f);
  val_t expr, tl; int iexpr; float fexpr;

  switch (tt)
    {
    case TOK_LPAR:
      {
	take();
	return vm_read_list(f);
      }
    case TOK_QUOT:
      {
	take();
	tl = vm_read_list(f);
	return vm_cons(F_QUOTE,tl);
      }
    case TOK_INT:
      {
	iexpr = atoi(TOKBUFF);
	expr = mk_int(iexpr);
	take();
	return expr;
      }
    case TOK_FLOAT:
      {
	fexpr = atof(TOKBUFF);
	expr = mk_float(fexpr);
	take();
	return expr;
      }
    case TOK_CHAR:
      {
	iexpr = nextu8(TOKBUFF);
	expr = mk_char(iexpr);
	take();
	return expr;
      }
    case TOK_UNICODE:
      {
	iexpr = atoi(TOKBUFF);
	expr = mk_char(iexpr);
	take();
	return expr;
      }
    case TOK_STR:
      {
	expr = mk_str(TOKBUFF);
	take();
	return expr;
      }
    case TOK_SYM:
      {
	expr = mk_sym(TOKBUFF,0);
	take();
	return expr;
      }
    default:
      {
	take();
	rsp_raise(SYNTAX_ERR);
	return R_NONE;
      }
    }
}

val_t vm_read_cons(FILE* f)
{
  size_t cnt = 0; rsp_tok_t c;
  val_t expr, OSP = SP;

  while ((c = vm_get_token(f)) != TOK_RPAR && c != TOK_DOT)
    {
      if (c == TOK_EOF)
	{
	  take();
	  rsp_perror(SYNTAX_ERR,"Unexpected EOF reading cons");
	  rsp_raise(SYNTAX_ERR);
	}

      expr = vm_read_expr(f);
      PUSH(expr);
      cnt++;
    }

  if (c == TOK_RPAR)
    {
      take();
      val_t ls = mk_list(cnt);
      init_list(ls,EVAL,cnt);
      SP = OSP;
      return ls;
    }

  else // read the last expression, then begin consing up the dotted list
    {
      take();
      expr = vm_read_expr(f);
      c = vm_get_token(f);
      take();

      if (c != TOK_RPAR) rsp_raise(SYNTAX_ERR);

      val_t ca, cd = expr;

      while (SP != OSP)
	{
	  ca = POP();
	  cd = vm_cons(ca,cd);
	}

      return cd;
    }
}

void vm_print(val_t v, FILE* f)
{
  if (v == R_NIL)
    {
      fputs("nil",f);
      return;
    }
  
  else if (v == R_NONE)
    {
      fputs("none",f);
      return;
    }
  
  else if (isdirect(v))
    {
      fprintf(f,"#<builtin:%s>",BUILTIN_FUNCTION_NAMES[unpad(v)]);
      return;
    }

  else
    {
      type_t* to = get_val_type(v);
      if (to->tp_prn)
	{
	  to->tp_prn(v,f);
	  return;
	}
      else
	{
	  vm_print_val(v,f);
	  return;
	}
    }
}

void vm_print_val(val_t v, FILE* f) // fallback printer
{
  char* tn = get_val_type_name(v);
  if (isdirect(v)) fprintf(f,"#<%s:%u>",tn,(unsigned)unpad(v));
  else fprintf(f,"#<%s:%p>",tn,toobj_(v));
  return;
}

val_t vm_load(FILE* f,val_t e)
{
  val_t OSP = SP;
  val_t *envp = SAVE(e);
  val_t *base = (&EVAL[SP+1]);
  size_t cnt = 0;

  while (!feof(f))
    {
      val_t expr = vm_read_expr(f);
      PUSH(expr);
      cnt++;
    }

  // evaluate the saved expressions in the order they were read
  for (size_t i = 0; i < cnt; i++)
    {
      rsp_eval(base[i],*envp);
    }

  // restore the stack pointer and return
  SP = OSP;
  return R_NIL;
}
