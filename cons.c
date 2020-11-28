#include "cons.h"

/* list predicate */
bool islist(rval_t v) {
  if (isnil(v)) {
    return true;
  } else if (iscons(v)) {
    return obj(v)->head.flags == CONSFLAG_LIST;
  } else {
    return false;
  }
}

int32_t norm_arglist(rval_t* v) {
  if (issym(*v)) {
    *v = tagcons(vm_new_cons(*v, NIL));
    return PROCFLAGS_VARGS_TRUE;
  }

  if (isnil(*v)) {
    return PROCFLAGS_VARGS_FALSE;
  }
  
  rval_t val = *v;
  rcons_t* prev;

  while (iscons(val)) {
    prev = _tocons(val);
    rcons_t* c = prev;

    if (!issym(c->car)) {
      return -1;

    } else {
      val = c->cdr;
    }
  }

  if (issym(val)) {
    prev->cdr = tagcons(vm_new_cons(val, NIL));
    vm_fix_list(*v);
    return PROCFLAGS_VARGS_TRUE;
  } else if (isnil(val)) {
    return PROCFLAGS_VARGS_FALSE;
  } else {
    return -1;
  }
}


int32_t vm_list_len(rval_t v) {
  if (isnil(v)) {
    return 0;
  } else if (iscons(v) && obj(v)->head.flags == CONSFLAG_LIST) {
    return obj(v)->head.meta;
  } else {
    return -1;
  }
}

rval_t vm_fix_list(rval_t v) {
  rval_t cv = v;
  rcons_t* c;
  uint32_t counter = 0;
  while (iscons(cv)) {
    c = _tocons(cv);
    c->head.flags = CONSFLAG_LIST;
    counter++;
  }

  cv = v;

  for (uint32_t i = 0; i < counter; i++) {
    c = _tocons(cv);
    c->head.meta = i;
  }

  return v;
}

/* creating cons objects */

rcons_t* vm_new_cons(rval_t ca, rval_t cd) {
  uchr_t* obj = vm_allocate(24);
  rcons_t* cobj = (rcons_t*)obj;
  cobj->car = ca;
  cobj->cdr = cd;

  if (islist(cd)) { 
  cobj->head.flags = CONSFLAG_LIST;
  cobj->head.meta = vm_list_len(cd) + 1;
  } else {
    cobj->head.flags = CONSFLAG_NOLIST;
  }

  cobj->head.type = TYPECODE_CONS;
  return cobj;
}

rcons_t* vm_new_list(rval_t ca, rval_t tl) {
  if (!islist(tl)) {
    eprintf(stderr, "Invalid list tail with typecode %d\n.", typecode(tl));
    escape(ERROR_TYPE);
  }

  return vm_new_cons(ca, tl);
}

rval_t vm_cons_constructor(rval_t ca, rval_t cd) {
  return tagcons(vm_new_cons(ca, cd));
}


uint32_t vm_cons_sizeof() {
  return 16;
}

/* utility functions */
rcons_t* vm_cons_lastpair(rcons_t* c) {
  while (iscons(c->cdr)) {
    c = _tocons(c->cdr);
  }

  return c;
}

rcons_t* vm_cons_append(rcons_t* c, rval_t v) {
  if (!c) {
    return vm_new_cons(v, NIL);
  }

  rval_t new = tagcons(vm_new_cons(v, NIL));
  rcons_t* curr = c, *prev;

  while (curr) {
    prev = curr;
    curr->head.meta += 1;       // increment the list length
    curr = _tocons(curr->cdr);
  }

  prev->cdr = new;
  return c;
}


rcons_t* vm_assoc_list(rval_t v, rcons_t* alist) {
  rval_t names = alist->car;
  rval_t bindings = alist->cdr;

  while (iscons(names)) {
    if (_tocons(names)->car == v) {
      return _tocons(bindings);
    } else {
      names = _tocons(names)->cdr;
      bindings = _tocons(bindings)->cdr;
    }
  }

  return NULL;
}
