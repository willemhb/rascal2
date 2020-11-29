#include "cons.h"

/* list predicate */
bool islist(rval_t v) {
  if (isnil(v)) return true;
  else return tag(v) == TAG_LIST;
}

// tagging helper
rval_t tagcons(rcons_t* c) {
  if (islist(c->cdr)) {
    return tagptr(c, LOWTAG_CONSPTR);
  } else {
    return tagptr(c, TAG_LIST);
  }
}

int32_t vm_cons_len(rval_t v, ls_enum_t lt) {
  if (lt == ALLOW_ANY) {
    int32_t t = typecode(v);
    if (t == TYPECODE_NIL) {
      return 0;
    } else if (t == TYPECODE_CONS) {
      return vm_cons_len(v, REQUIRE_CONS);
    } else {
      return 1;
    }
  }

  int32_t l = 0;
  while (iscons(v)) {
    v = _tocons(v)->cdr;
    l++;
  }

  if (isnil(v)) {
    return l;
  } else if (lt == REQUIRE_CONS) {
    return l + 1;
  } else {
    return -1;
  }
}


int32_t vm_cons_of(rval_t v, uint32_t tc, ls_enum_t lt) {
  if (lt == ALLOW_ANY) {
    uint32_t vtc = typecode(v);
    if (vtc == TYPECODE_NIL) {
      return 1;
    } else if (vtc == TYPECODE_CONS) {
      return vm_cons_of(v, tc, REQUIRE_CONS);
    } else {
      return vtc == tc;
    }
  }

  while (iscons(v)) {
    rval_t ca = _tocons(v)->car;
    if (typecode(ca) != tc) {
      return 0;
    }
    v = _tocons(v)->cdr;
  }

  if (isnil(v)) {
    return 1;
  } else if (lt == REQUIRE_CONS) {
    return typecode(v) == tc;
  } else {
    return -1;
  }
}


rcons_t* vm_new_cons(rval_t ca, rval_t cd, ls_enum_t lt) {
  if (lt == REQUIRE_LIST && !islist(cd)) {
    eprintf(stderr, "Invalid list tail of type %s.\n", vm_val_typename(cd));
    escape(ERROR_TYPE);
  }
  
  uchr_t* obj = vm_allocate(16);
  rcons_t* cobj = (rcons_t*)obj;
  cobj->car = ca;
  cobj->cdr = cd;

  return cobj;
}


uint32_t vm_cons_sizeof(rval_t v) {
  return 16;
}

/* utility functions */
rcons_t* vm_cons_lastpair(rcons_t* c) {
  while (iscons(c->cdr)) {
    c = _tocons(c->cdr);
  }

  return c;
}

rcons_t* vm_cons_append(rcons_t** c, rval_t v, ls_enum_t lt) {
  if (lt == REQUIRE_LIST) {
    v = tagcons(vm_new_list(v, NIL));
  }
  
  if (!c) {
    *c = _tocons(v);
  }

  rcons_t* last_pair = vm_cons_lastpair(*c);
  last_pair->cdr = v;

  return *c;
}


rcons_t* vm_assoc_cons(rval_t v, rcons_t* alist, ls_enum_t lt) {
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

  if (lt != REQUIRE_LIST && !isnil(names) && v == names) {
    return _tocons(bindings);
  } else {
    return NULL;
  }
}
