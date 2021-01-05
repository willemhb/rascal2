#include "rascal.h"

/* tag manipulation */
// extract the tag and the address
inline int ltag(val_t v) { int nt = ntag(v); return nt < 0xf ? nt : (int)wtag(v); }
inline bool isdirect(val_t v) { return !v || ltag(v) >= 0xf; }
inline bool iscallable(val_t v) { int t = ltag(v); return t == LTAG_FUNCTION || t == LTAG_BUILTIN || t == LTAG_META; }
inline bool isatomic(val_t v) { return get_val_type(v)->tp_atomic; }

inline int oflags(val_t v) {
  ltag_t t = ltag(v);

  switch (t) {
  case LTAG_SYM:
  case LTAG_FVEC:
  case LTAG_OBJECT:
  case LTAG_FUNCTION:
  case LTAG_NODE:
    return flags_(v);
  default:
    return FLAG_NONE;
  }
}

// get the type object associated with a value
type_t* get_val_type(val_t v) {
  int t;
  switch((t = ntag(v))) {
  case LTAG_LIST: return v ? CORE_OBJECT_TYPES[LTAG_LIST] : CORE_DIRECT_TYPES[LTAG_LIST];
  case LTAG_DIRECT: return CORE_DIRECT_TYPES[wtag(v) >> 4];
  case LTAG_FUNCTION: case LTAG_OBJECT: return type_(v);
  default: return CORE_OBJECT_TYPES[t];
  }
}

int get_val_size(val_t v) {
  if (isdirect(v)) return 8;

  type_t* to = get_val_type(v);
  
  if (to->val_fixed_size) return to->val_base_size;

  else return to->tp_val_size(v); 
}

inline char* get_val_type_name(val_t v) { return get_val_type(v)->name; }
inline c_num_t get_val_cnum(val_t v) { return get_val_type(v)->val_cnum; }
inline c_num_t get_common_cnum(val_t x, val_t y) { return get_val_cnum(x) | get_val_cnum(y) ; }

/* safecasts and checking forwarding pointers */
val_t check_fptr(val_t* v) {
  if (!isobj(*v)) return *v;
  return update_fptr(v);
}

val_t update_fptr(val_t* v) {
  // follow the forwarding pointer possibly more than once
  while (obj_(*v)->type == R_FPTR) *v = ptr_(val_t*,*v)[1];

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

unsigned char* initialize_obj(val_t* new, val_t* args, int argc) {
  for (int i = 0; i < argc; i++) {
    check_fptr(args);
    *new = *args;
    new++;
    args++;
  }

  return (unsigned char*)new;
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

  // trace the shared stacks
  for (size_t i = 0; i < SP; i++) gc_trace(STACK + i);

  for (size_t i = 0; i < EP; i++) gc_trace(EVAL + i);

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
  if (isdirect(vv)) return vv;
  if (t == LTAG_CFILE || t == LTAG_META || t >= 0xf) return vv;

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
inline val_t mk_int(int i) { return tag_v(i,LTAG_INT); }
inline val_t mk_char(int c) { return c < MIN_CODEPOINT || c > MAX_CODEPOINT ? R_EOF :  tag_v(c,LTAG_CHAR); }
inline val_t mk_bool(int b) { return b ? R_TRUE : R_FALSE; }
inline val_t mk_cfile(FILE* f) { return tag_p(f,LTAG_CFILE); }
inline val_t mk_cons() { return tag_p(rsp_alloc(16),LTAG_CONS); }
inline val_t mk_list(int cnt) { return cnt == 0 ? R_NIL : tag_p(rsp_allocw(cnt * 2), LTAG_LIST); }
inline val_t mk_node() { return tag_p(rsp_alloc(32), LTAG_NODE) ; }
inline val_t mk_table() { return tag_p(rsp_alloc(16), LTAG_TABLE); }

// vector constructors
val_t mk_fvec(int elements) {
  fvec_t* out = (fvec_t*)rsp_allocw(2 + elements);
  out->allocated = elements;
  return tag_p(out,LTAG_FVEC);
}

val_t mk_dvec(int elements) {
  val_t store = mk_fvec(elements);
  SAVE(store); // in case allocating the dynamic part initiates GC, save the store
  dvec_t* out = (dvec_t*)rsp_allocw(4);
  out->curr = 0;
  out->elements = RESTORE();

  return tag_p(out,LTAG_DVEC);
}

// more complex constructors
val_t mk_str(const char* s) {
  // if the string is properly aligned just return a tagged pointer; otherwise create a copy on the heap and return a pointer to that
  if (aligned(s,16)) return ((val_t)s) | LTAG_STR;
  int i = strsz(s);
  unsigned char* new = rsp_alloc(i);
  memcpy(new,s,i);

  return ((val_t)new) | LTAG_STR;
}


val_t mk_sym(char* s, int i, int gs, int kw, int rsv) {
  static int GS_COUNTER = 0;
  char* snm = s;

  if (gs == SYMFLAG_GENSYM) {
    char gsbuf[74 + strsz(s)];
    sprintf(gsbuf,"__GENSYM__%d__%s",GS_COUNTER,s);
    snm = gsbuf;
    GS_COUNTER++;
  }

  hash64_t h = hash_str(snm);

  if (i != SYMFLAG_INTERNED) {
    h = (h & (~0xful)) | i | gs | kw | rsv;
    int slen = strsz(snm);
    sym_t* new = (sym_t*)rsp_alloc(8 + slen);
    new->record = h;
    memcpy(new->name,snm,slen);

    return tag_p(new,LTAG_SYM);

  } else {
    return intern_string(R_SYMTAB,s,h,i,gs,kw,rsv);
  }
}

val_t mk_builtin(val_t(*)(val_t*,int),int,int); // C function, argument count, vargs flag
val_t mk_lambda(val_t,val_t,val_t,int);         // argument list, body, captured environment, macro flag
val_t mk_envframe(val_t,val_t,int);
val_t mk_contframe(int);

// object comparison
int ord(val_t vx, val_t vy) {
  if (!isatomic(vx) || !isatomic(vy)) rsp_raise(TYPE_ERR);

  // if types differ, order by type
  val_t tx = (val_t)get_val_type(vx), ty = (val_t)get_val_type(vy);
  if (tx != ty) return compare(tx,ty);
  
}

// special accessors for builtin types used by the VM

// field-, index-, and key-based object access (generic)
val_t    rsp_getf(val_t,val_t);         // return the value of a named field
val_t    rsp_setf(val_t,val_t);         // set the value of a named field
val_t    rsp_assck(val_t,val_t);        // key-based search
val_t    rsp_asscn(val_t,val_t);        // index-based search
val_t    rsp_rplck(val_t,val_t,val_t);  // change the value associated with a particular key
val_t    rsp_rplcn(val_t,val_t,val_t);  // replace the nth element of a collection
val_t    rsp_conj(val_t,val_t);         // add a new element to a collection in a way dispatched by the type

// 
val_t* search_string(val_t*,char*,hash64_t);
val_t* search_atom(val_t*,val_t);

// vm builtins and API
val_t vm_eval(val_t,val_t);
val_t vm_apply(val_t,val_t);
val_t vm_evlis(val_t,val_t);

// reader support functions
rsp_tok_t vm_get_token(FILE*);
val_t     vm_read_expr(FILE*);
val_t     vm_read_list(FILE*);

