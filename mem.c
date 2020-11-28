#include "mem.h"

/* memory initialization */
void vm_init_heap() {
  // initialize the heap, the tospace, and the memory management globals
  HEAPSIZE = INIT_HEAPSIZE;
  HEAP = malloc(HEAPSIZE);
  EXTRA = malloc(HEAPSIZE);
  FREE = HEAP;

  GROWHEAP = false;
  return;
}

void vm_init_types() {
  TYPES = malloc(sizeof(rtype_t*) * INIT_NUMTYPES);
  TYPECOUNTER = 0;
  return;
}

/* memory alignment helper */
int32_t vm_align_memory(uchr_t** var, uchr_t* limit) {
  uintptr_t memory_head = (uintptr_t)(*var);
  int32_t status = 0;

  while (memory_head % 8) {
    if (*var == limit) {
      status = -1;
      break;
    }
    
    *var += 1;
    memory_head = (uintptr_t)(*var);
  }

  return status;
}

/* gc */

void vm_gc() {
  if (GROWHEAP) {
    HEAPSIZE *= 2;
    EXTRA = realloc(EXTRA, HEAPSIZE);
  }

  FROMSPACE = HEAP;
  TOSPACE = EXTRA;
  TOFREE = EXTRA;

  vm_gc_trace((rval_t*)GLOBALS);

  for (int32_t i = 0; i < SP; i++) {
    vm_gc_trace(STACK + i);
  }

  if (GROWHEAP) {
    HEAP = realloc(HEAP,HEAPSIZE);
  }

  uchr_t* TMP = HEAP;
  HEAP = EXTRA;
  EXTRA = TMP;
  FREE = TOFREE;
  GROWHEAP = CHECK_RESIZE();
  
  return;
}

rval_t vm_gc_trace(rval_t* v) {
  rval_t value = *v;

  // variables to hold references to memory being traced.
  uchr_t* newhead;
  rval_t tmpleft, tmpright, env, arglist, body;
  int32_t tag, bodyflags;
  rcons_t* c;
  rsymt_t* st;
  rproc_t* proc;

  switch (vm_val_typecode(value)) {
  case TYPECODE_INT:
  case TYPECODE_NIL:
  case TYPECODE_TYPE:
  default:
    return value;
  case TYPECODE_PORT:
    newhead = vm_gc_copy(heapaddr(value),TOSPACE,vm_obj_size(value));
    tag = LOWTAG_OBJPTR;
    break;
  case TYPECODE_CONS:
    c = _tocons(value);
    c->car = vm_gc_trace(&(c->car));
    c->cdr = vm_gc_trace(&(c->cdr));
    newhead = vm_gc_copy(heapaddr(value),TOSPACE,vm_obj_size(value));
    tag = LOWTAG_CONS;
    break;
  case TYPECODE_SYM:
    newhead = vm_gc_copy(heapaddr(value),TOSPACE,vm_obj_size(value));
    tag = LOWTAG_SYM;
    break;
  case TYPECODE_SYMT:
    st = _tosymt(value);
    tmpleft = tagptr(st->left, LOWTAG_OBJPTR);
    tmpright = tagptr(st->right, LOWTAG_OBJPTR);
    st->binding = vm_gc_trace(&(st->binding));
    st->left = _tosymt(vm_gc_trace(&tmpleft));
    st->right = _tosymt(vm_gc_trace(&tmpright));
    newhead = vm_gc_copy(heapaddr(value),TOSPACE,vm_obj_size(value));
    tag = LOWTAG_OBJPTR;
    break;
  case TYPECODE_STR:
    newhead = vm_gc_copy(heapaddr(value),TOSPACE,vm_obj_size(value));
    tag = LOWTAG_OBJPTR;
    break;
  case TYPECODE_PROC:
    proc = _toproc(value);
    env = proc->env;
    arglist = proc->argl;
    bodyflags = hflags(proc->head) >> 1;
    body = bodyflags == PROCFLAGS_BODYTYPE_EXPR ? tagptr(proc->body, LOWTAG_CONS) : 0;

    proc->env = vm_gc_trace(&env);
    proc->argl = vm_gc_trace(&arglist);

    if (body) {
      body = vm_gc_trace(&body);
    }

    newhead = vm_gc_copy(heapaddr(value),TOSPACE,vm_obj_size(value));
    tag = LOWTAG_OBJPTR;
    break;
  }

  value = tagptr(newhead,tag);
  *v = value;
  return value;
}


uchr_t* vm_gc_copy(uchr_t* from, uchr_t* to, int32_t numbytes) {
  rcons_t* ascons = (rcons_t*)from;

  if (ptr(ascons->car) == NULL) {
    return heapaddr(ascons->cdr);
  } else {
    memcpy(from, to, numbytes);
    ascons->car = 0;
    ascons->cdr = (uintptr_t)to;
    TOFREE += numbytes;
    if (vm_align_memory(&TOFREE,TOSPACE + HEAPSIZE) == -1) {
      eprintf(stderr,"Ran out of memory during garbage collection.");
      escape(ERROR_OVERFLOW);
    }
    return heapaddr(ascons->cdr);
  }
}

/* bounds and error checking */

bool vm_heap_limit(uint32_t numcells) {
  return ALLOCATIONS() + numcells  > HEAPSIZE;
}

bool vm_type_limit(uint32_t numtypes) {
  return TYPECOUNTER + numtypes > NUMTYPES;
}

bool vm_stack_overflow(uint32_t numcells) {
  return SP + numcells > MAXSTACK;
}

bool vm_type_overflow(uint32_t numtypes) {
  return TYPECOUNTER + numtypes > MAXTYPES;
}

uchr_t* vm_allocate(int32_t numbytes) {
  if (vm_heap_limit(numbytes)) vm_gc();
  
  uchr_t* out = FREE;
  FREE += numbytes;

  if (vm_align_memory(&FREE,HEAP + HEAPSIZE) == -1) {
    eprintf(stderr, "Heap overflow during allocation.\n");
    escape(ERROR_OVERFLOW);
  }

  return out;
}

rtype_t* vm_new_type(chr_t* typename) {
  if (vm_type_limit(1)) {
    NUMTYPES = NUMTYPES * 2 < MAXTYPES ? NUMTYPES * 2: MAXTYPES;
    if (vm_type_limit(1)) {
      eprintf(stderr, "Type overflow.\n");
      escape(ERROR_OVERFLOW);
    }
    TYPES = realloc(TYPES,NUMTYPES*sizeof(rtype_t*));
  }
  rtype_t* new = malloc(sizeof(rtype_t) + strlen(typename) + 1);

  strcpy(new->tp_name,typename);
  new->head.oh_type = 0;
  new->head.oh_meta = TYPECOUNTER;

  TYPECOUNTER++;

  return new;
}
