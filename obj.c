#include "rascal.h"

/* safecasts and checking forwarding pointers */
DESCRIBE_VAL_SAFECAST(int_t,int)
DESCRIBE_VAL_SAFECAST(ucp_t,ucp)
DESCRIBE_PTR_SAFECAST(cons_t*,cons)
DESCRIBE_PTR_SAFECAST(sym_t*,sym)
DESCRIBE_PTR_SAFECAST(dict_t*,dict)
DESCRIBE_PTR_SAFECAST(type_t*,type)
DESCRIBE_PTR_SAFECAST(str_t*,str)
DESCRIBE_PTR_SAFECAST(port_t*,port)
DESCRIBE_PTR_SAFECAST(proc_t*,proc)

bool check_fptr(val_t v) {
  if (tag(v) == LOWTAG_DIRECT) return false;
  cons_t* cell = tocons_(v);
  return car_(cell) == FPTR; 
}

val_t get_fptr(val_t v) {
  if (check_fptr(v)) {
    return cdr_(v);
  } else {
    return v;
  }
}

val_t update_fptr(val_t* v) {
  while (check_fptr(*v)) {
    *v = get_fptr(*v);
  }
  return *v;
}

val_t trace_fptr(val_t v) {
  while (check_fptr(v)) {
    v = get_fptr(v);
  }
  return v;
}

/* type utilities */
uint_t typecode(val_t v) {
  uint_t t = tag(v);
  uint_t tc;
  switch (t) {
  case LOWTAG_CONSPTR:
    tc = TYPECODE_CONS;
    break;
  case LOWTAG_STRPTR:
    tc = TYPECODE_STR;
    break;
  case LOWTAG_LISTPTR:
    tc = TYPECODE_CONS;
    break;
  case LOWTAG_DIRECT:
    tc = ((v&UINT32_MAX) >> 3);
    break;
  case LOWTAG_OBJPTR:
  default:
    v = trace_fptr(v);
    return obj(v)->head.type;
  }
  return tc;
}

type_t* type_of(val_t v) {
  return TYPES[typecode(v)];
}

chr_t* typename(val_t v) {
static chr_t* builtin_typenames[NUM_BUILTIN_TYPES] = {
  "nil-type", "cons", "none-type", "str",
  "type", "sym", "dict", "proc", "port",
  "int", };

  int_t t = typecode(v);

  if (t < NUM_BUILTIN_TYPES) {
    return builtin_typenames[t];
  }
  
  else {
    return (TYPES[t])->tp_name->name; 
  }
}

inline bool atomic(val_t v) {
  switch (typecode(v)) {
  case TYPECODE_SYM:
  case TYPECODE_NIL:
  case TYPECODE_NONE:
  case TYPECODE_INT:
    return true;
  default:
    return false;
  }
}

inline bool callable(val_t v) {
  int_t t = typecode(v);
  return t == TYPECODE_TYPE || t == TYPECODE_PROC;
}

inline bool cmpable(val_t v) {
  return type_of(v)->flags.cmpable;
}

int_t vm_size(val_t v) {
  int_t t = typecode(v);
  switch (t) {
  case TYPECODE_NIL:
  case TYPECODE_NONE:
  case TYPECODE_INT:
    return 8;
  case TYPECODE_CONS:
    return sizeof(cons_t);
  case TYPECODE_PORT:
    return sizeof(port_t);
  case TYPECODE_PROC:
    return sizeof(proc_t);
  case TYPECODE_TYPE:
    return sizeof(type_t);
  case TYPECODE_DICT:
    return sizeof(dict_t);
  case TYPECODE_STR:
    return strlen(tostr(v)) + 1;
  case TYPECODE_SYM:
    return sizeof(sym_t) + strlen(name(v));
  default:
    return type_of(v)->tp_sizeof(v);
  }
}


static inline int_t alloc_size(uint_t bytes) {
  // return the minimum allocation size, or the ceiling
  // modulo 8 of the given number of bytes.
  if (bytes <= 16) {
    return 16;
  } else {
    while (bytes % HEAP_ALIGNSIZE) {
      bytes++;
    }

    return bytes;
  }
}

static inline void zfill(uchr_t* ostart, uchr_t* oend) {
  /* pad to the end of the allocated space with nullbytes */ 
  for (uchr_t* begin = ostart + 1; begin < oend; begin++) *begin = '\0';
  }

static inline void clearobjhead(uchr_t* heap) {
  /* clear old data from the head of the allocated object */
  obj_t* o = (obj_t*)heap;
  o->head.type = o->head.meta = o->head.flags_0 = o->head.flags_1 = o->head.flags_2 = 0;

  return;
}

/* gc */

// allows functions that 
inline void gc_if(uint_t b) {
  if (heap_limit(alloc_size(b))) gc(NULL,0);
  return;
}

// functions that allocate from the heap can use pre_gc to preemptively
// run the gc, passing it a list of local bindings to update
inline void pre_gc(uint_t b, val_t** locals, uint_t nlocals) {
  if (heap_limit(alloc_size(b))) gc(locals, nlocals);
}

void gc(val_t** locals, uint_t nlocals) {
  ALLOCATIONS = 0;
  if (GROWHEAP) {
    HEAPSIZE *= 2;
    EXTRA = realloc(EXTRA, HEAPSIZE);
  }

  FROMSPACE = HEAP;
  TOSPACE = EXTRA;
  FREE = TOSPACE;

  for (uint_t i = 0; i < TYPECOUNTER - 1; i++) {
    type_t* tp = TYPES[i];
    sym_t* tp_name = tp->tp_name;
    val_t tmp_name = tagp(tp_name);

    gc_trace(&tmp_name);
    tp->tp_name = tosym_(tmp_name);
    gc_trace(&(tp->tp_new));
  }

  gc_trace(&GLOBALS);

  for (int_t i = 0; i < SP; i++) {
    gc_trace(STACK + i);
  }

  // trace all the registers that might have data in them
  gc_trace(&EXP);
  gc_trace(&VAL);
  gc_trace(&NAME);
  gc_trace(&ENV);
  gc_trace(&UNEV);
  gc_trace(&ARGL);
  gc_trace(&PROC);
  gc_trace(&ROOT_ENV);
  gc_trace(&R_STDIN);
  gc_trace(&R_STDOUT);
  gc_trace(&R_STDERR);
  gc_trace(&R_PROMPT);
  gc_trace(&WRX);
  gc_trace(&WRY);
  gc_trace(&WRZ);

  if (locals != NULL) {
    for (uint_t i = 0; i < nlocals; i++) {
      gc_trace(locals[i]);
    }
  }
  
  if (GROWHEAP) {
    HEAP = realloc(HEAP,HEAPSIZE);
  }

  uchr_t* TMP = HEAP;
  HEAP = EXTRA;
  EXTRA = TMP;
  GROWHEAP = CHECK_RESIZE();
  
  return;
}

static bool inheap(val_t* v, uchr_t* h) {
  uchr_t* ho = (uchr_t*)v;
  return ho >= h && ho >= (h + HEAPSIZE);
}

void gc_trace(val_t* v) {
  if (inheap(v,TOSPACE)) {
    return;
  }

  val_t value = *v;
  if (tag(value) == LOWTAG_DIRECT) {
    return;
  } else { // check if the data has already been moved
    cons_t* tmpc = tocons_(v);
    val_t fp = car_(tmpc);
    if (fp == FPTR) {
      *v = cdr_(tmpc);
      return;
    } else {
      ALLOCATIONS++;
      uint_t tmptag;
      uchr_t* newhead;
      switch (typecode(value)) {
      case TYPECODE_TYPE: // types are handled separately since they don't move
	return;
      case TYPECODE_CONS:
	tmptag = tag(value);
	gc_trace(&car_(value));
	gc_trace(&cdr_(value));
	newhead = gc_copy(heapobj(value),16,tmptag);
	break;
 
      case TYPECODE_SYM:
	tmptag = LOWTAG_STROBJ;
	newhead = gc_copy(heapobj(value),sizeof(sym_t) + strlen(name_(value)),tmptag);
	break;

      case TYPECODE_DICT:
	tmptag = LOWTAG_OBJPTR;
	gc_trace(&key_(value));
	gc_trace(&binding_(value));
	gc_trace(&left_(value));
	gc_trace(&right_(value));

	newhead = gc_copy(heapobj(value), sizeof(dict_t),tmptag);
	break;

      case TYPECODE_PORT:
	tmptag = LOWTAG_OBJPTR;
	newhead = gc_copy(heapobj(value), sizeof(port_t),tmptag);
	break;
 
      case TYPECODE_STR:
	tmptag = LOWTAG_STRPTR;
	newhead = gc_copy(heapobj(value), strlen(tostr_(value)) + 1,tmptag);
	break;

      case TYPECODE_PROC:
	tmptag = LOWTAG_OBJPTR;
	gc_trace(&formals_(value));
	gc_trace(&env_(value));
	if (bodytype_(value) == BODYTYPE_EXPR) gc_trace(&body_(value));

	newhead = gc_copy(heapobj(value), sizeof(proc_t),tmptag);
	break;
      }

  *v = tagptr(newhead,tmptag);
  return;
      }
    }
  }


uchr_t* gc_copy(uchr_t* from, int_t numbytes, int_t tag) {
  cons_t* ascons = (cons_t*)from;
  int_t aligned_size = alloc_size(numbytes);

  memcpy(from, FREE, numbytes);
  // leave a special marker and the address of the moved object
  ascons->car = FPTR;
  ascons->cdr = tagptr(FREE,tag);
  zfill(FREE + numbytes, FREE + aligned_size);
  FREE += aligned_size;

  return cptr(ascons->cdr);
  }


/* bounds and error checking */

bool heap_limit(uint_t numcells) {
  return FREE + numcells  > HEAP + HEAPSIZE;
}

bool type_limit(uint_t numtypes) {
  return TYPECOUNTER + numtypes > NUMTYPES;
}

bool stack_overflow(uint_t numcells) {
  return SP + numcells > MAXSTACK;
}

bool type_overflow(uint_t numtypes) {
  return TYPECOUNTER + numtypes > MAXTYPES;
}

uchr_t* vm_allocate(int_t numbytes) {
  uint_t aligned_size = alloc_size(numbytes);
  if (heap_limit(numbytes)) gc(NULL,0);

  uchr_t* out = FREE;
  clearobjhead(out);
  zfill(out + numbytes, out + aligned_size);
  FREE += aligned_size;
  ALLOCATIONS++;

  return out;
}

type_t* new_type() {
  if (type_limit(1)) {
    NUMTYPES = NUMTYPES * 2 < MAXTYPES ? NUMTYPES * 2: MAXTYPES;

    if (type_limit(1)) {
      
      escapef(OVERFLOW_ERR,stderr,"Ran out of types");

    }
    
    TYPES = realloc(TYPES,NUMTYPES*sizeof(type_t*));
  }

  type_t* new = malloc(sizeof(type_t));

  new->head.type = TYPECODE_TYPE;
  new->head.meta = TYPECOUNTER;

  TYPECOUNTER++;

  return new;
}


/* cons api */
inline bool islist(val_t v) {
  if (isnil(v)) return true;
  else return tag(v) == LOWTAG_LISTPTR;
}


// tagging helper
inline val_t tagc(cons_t* c) {
  if (c == NULL) {
    return NIL;
  } else {
    if (isnil(cdr_(c))) {
      return tagptr(c, LOWTAG_LISTPTR);
    } else if (tag(cdr_(c)) == LOWTAG_LISTPTR) {
      return tagptr(c, LOWTAG_LISTPTR);
  } else {
    return tagptr(c, LOWTAG_CONSPTR);
  }
 }
}

inline val_t car(val_t v) {
  cons_t* c = tocons(v);
  return car_(c);
}

inline val_t cdr(val_t v) {
  cons_t* c = tocons(v);
  return cdr_(c);
}


/* generic cons accessor */
inline val_t cxr(val_t c, cons_sel s) {
  val_t curr = c;
  
  if (s == -1) {
    while (iscons(curr) && iscons(cdr(curr))) {
      curr = cdr(curr);
    }

    return curr;
  }

  while (s != 1) {
    if (!iscons(curr)) {
      escapef(TYPE_ERR,stderr,"Tried to take cdr of non-cons with type %s", typename(curr));
    }

    if (s & 0x1) {
      curr = cdr(curr);
    } else {
      curr = car(curr);
    }
    s >>= 1;
  }
  
  return curr;
}

inline val_t cons(val_t ca, val_t cd) {
  cons_t* obj = (cons_t*)vm_allocate(16);

  // since vm_allocate might have moved ca and cd, update their values
  car_(obj) = trace_fptr(ca);
  cdr_(obj) = trace_fptr(cd);

  return tagc(obj);
}

inline uint_t ncells(val_t c) {
  uint_t count = 0;
  while (iscons(c)) {
    count++;
    c = cdr(c);
  }

  return count + (isnil(c) ? 0 : 1);
}

val_t append(val_t* c, val_t v) {
  if (c == NULL) escapef(NULLPTR_ERR,stderr,"Unexpected null pointer in append");
  else if (!islist(*c)) escapef(TYPE_ERR,stderr,"Attempt to append to non-list");

  val_t* localvs[2] = {c, &v };
  pre_gc(sizeof(cons_t),localvs,2);

  val_t* cc = c;

  // if allocation of a new cons triggered a garbage collection,
  while (iscons(*cc)) cc = &cdr_(*cc);

  *cc = cons(v, NIL);
  
  return *c;
}

val_t append_i(val_t* c, val_t v) {
  if (c == NULL) escapef(NULLPTR_ERR, stderr, "Unexpected null pointer in append");

  if (v == NIL) {
    return *c;
  }
  
  val_t* cc = c;

  while (iscons(*cc)) {
    if (iscons(cdr_(*cc))) {
      // correct the list pointer lowtag
      cdr_(*cc) = untag(cdr_(*cc)) | LOWTAG_CONSPTR;
    }
    cc = &cdr_(*cc);
  }

  *cc = v;
  *c = untag(*c) | LOWTAG_CONSPTR;
  return *c;
}
