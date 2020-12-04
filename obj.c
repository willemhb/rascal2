#include "obj.h"

/* type utilities */
uint_t typecode(val_t v) {
  uint_t t = tag(v);
  switch (t) {
  case LOWTAG_CONSPTR:
  case LOWTAG_STRPTR:
    return t;
  case LOWTAG_LISTPTR:
    return TYPECODE_CONS;
  case LOWTAG_DIRECT:
    return (v & UINT_MAX) >> 3;
  case LOWTAG_OBJPTR:
  default:
    return obj(v)->head.type;
  }
}

type_t* type_of(val_t v) {
  return TYPES[typecode(v)];
}

chr_t* typename(val_t v) {
  int_t t = typecode(v);

  if (t < NUM_BUILTIN_TYPES) {
    return BUILTIN_TYPENAMES[t];
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
  case TYPECODE_TAB:
    return sizeof(tab_t);
  case TYPECODE_STR:
    return strlen(tostr_(v)) + 1;
  case TYPECODE_SYM:
    return sizeof(sym_t) + strlen(tosym_(v)->name) + 1;
  default:
    return type_of(v)->tp_sizeof(v);
  }
}


/* memory initialization */
void init_heap() {
  // initialize the heap, the tospace, and the memory management globals
  HEAPSIZE = INIT_HEAPSIZE;
  HEAP = malloc(HEAPSIZE);
  EXTRA = malloc(HEAPSIZE);
  FREE = HEAP;

  GROWHEAP = false;
  return;
}

void init_types() {
  TYPES = malloc(sizeof(type_t*) * INIT_NUMTYPES);
  TYPECOUNTER = 0;
  return;
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

void gc() {
  if (GROWHEAP) {
    HEAPSIZE *= 2;
    EXTRA = realloc(EXTRA, HEAPSIZE);
  }

  FROMSPACE = HEAP;
  TOSPACE = EXTRA;
  FREE = TOSPACE;

  gc_trace((val_t*)GLOBALS);

  for (int_t i = 0; i < SP; i++) {
    gc_trace(STACK + i);
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

val_t gc_trace(val_t* v) {
  val_t value = *v;

  // variables to hold references to memory being traced.
  uchr_t* newhead;
  val_t tmpleft, tmpright, env, formals, body;
  int_t tag;
  cons_t* c;
  tab_t* tab;
  proc_t* proc;

  switch (typecode(value)) {
  case TYPECODE_INT:
  case TYPECODE_NIL:
  case TYPECODE_TYPE:
  default:
    return value;
  case TYPECODE_PORT:
    newhead = gc_copy(heapobj(value), obj_size(value));
    tag = LOWTAG_OBJPTR;
    break;
    
  case TYPECODE_CONS:
    c = toc_(value);
    car_(c) = gc_trace(&(c->car));
    cdr_(c) = gc_trace(&(c->cdr));
    newhead = gc_copy(heapobj(value), obj_size(value));
    tag = tag(value) == LOWTAG_LISTPTR ? LOWTAG_LISTPTR : LOWTAG_CONSPTR;
    break;

  case TYPECODE_SYM:
    newhead = gc_copy(heapobj(value), obj_size(value));
    tag = LOWTAG_STROBJ;
    break;

  case TYPECODE_TAB:
    tab = totab_(value);
    tmpleft = tagptr(tab->left, LOWTAG_OBJPTR);
    tmpright = tagptr(tab->right, LOWTAG_OBJPTR);
    tab->key = gc_trace(&(tab->key));
    tab->binding = gc_trace(&(tab->binding));
    tab->left = totab_(gc_trace(&tmpleft));
    tab->right = totab_(gc_trace(&tmpright));
    newhead = gc_copy(heapobj(value), obj_size(value));
    tag = LOWTAG_OBJPTR;
    break;
 
  case TYPECODE_STR:
    newhead = gc_copy(heapobj(value), obj_size(value));
    tag = LOWTAG_STRPTR;
    break;

  case TYPECODE_PROC:
    proc = toproc_(value);
    env = proc->env;
    formals = proc->formals;
    body = proc->head.flags_1 == 0 ? tagptr(proc->body, LOWTAG_CONSPTR) : 0;

    proc->env = gc_trace(&env);
    proc->formals = gc_trace(&formals);

    if (body) {
      proc->body = gc_trace(&body);
    }

    newhead = gc_copy(heapobj(value), obj_size(value));
    tag = LOWTAG_OBJPTR;
    break;
  }

  value = tagptr(newhead,tag);
  *v = value;
  return value;
}


uchr_t* gc_copy(uchr_t* from, int_t numbytes) {
  cons_t* ascons = (cons_t*)from;
  int_t aligned_size = alloc_size(numbytes);

  if (ascons->car == FPTR) {
    return cptr(ascons->cdr);

  } else {

    memcpy(from, FREE, numbytes);
    ascons->car = FPTR;
    ascons->cdr = (uintptr_t)FREE;

    zfill(FREE + numbytes, FREE + aligned_size);

    return cptr(ascons->cdr);
  }
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
  if (heap_limit(numbytes)) gc();

  uchr_t* out = FREE;
  clearobjhead(out);
  zfill(out + numbytes, out + aligned_size);
  FREE += aligned_size;

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
    escapef(NULLPTR_ERR,stderr,"Unexpected null pointer in tagc.");
  }
  
  if (islist(cdr_(c))) {
    return tagptr(c, LOWTAG_LISTPTR);
  } else {
    return tagptr(c, LOWTAG_CONSPTR);
  }
}

inline val_t car(val_t v) {
  cons_t* c = toc(v);
  return car_(c);
}

inline val_t cdr(val_t v) {
  cons_t* c = toc(v);
  return cdr_(c);
}


/* generic cons accessor */
inline val_t cxr(val_t c, cons_sel s) {
  val_t curr = c;
  
  if (s == -1) {
    while (iscons(curr) && iscons(cdr_(curr))) {
      curr = cdr_(curr);
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

  obj->car = ca;
  obj->cdr = cd;

  return tagc(obj);
}

inline int_t ncells(val_t c) {
  if (c == NIL) {
    return 0;
  }

  int_t count = 0;
  
  while (isc(c)) count++;

  if (c == NIL) {
    return count;
  } else {
    return count + 1;
  }
}

val_t append(val_t* c, val_t v) {
  if (c == NULL) escapef(NULLPTR_ERR,stderr,"Unexpected null pointer in append.");
  else if (!islist(*c)) escapef(TYPE_ERR,stderr,"Attempt to append to non-list.");

  while (iscons(*c)) c = &cdr_(*c);

  *c = cons(v,NIL);
  
  return *c;
}


val_t extend(val_t* cx, val_t cy) {
  if (cx == NULL) escapef(NULLPTR_ERR,stderr,"Unexpected null pointer.");
  if (!islist(*cx) || !islist(cy)) escapef(TYPE_ERR,stderr,"Unexpected null pointer.");

  val_t* out = cx;

  while (iscons(*cx)) cx = &cdr_(*cx);
  
  *cx = cy;

  return *out;
}
