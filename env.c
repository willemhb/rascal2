#include "env.h"


rsym_t* vm_new_sym(chr_t* s, hash_t h, uint32_t interned) {
  uint32_t slen = strlen(s);
  uchr_t* obj = vm_allocate(sizeof(rsym_t) + slen);
  rsym_t* sobj = (rsym_t*)obj;

  sobj->head.meta = h;
  sobj->head.type = TYPECODE_SYM;
  sobj->head.flags = 0;
  setinterned(sobj, interned);
  sobj->binding = NONE;
  sobj->left = sobj->right = NULL;

  return sobj;
}

uint32_t vm_sym_sizeof(rsym_t* s) {
  return sizeof(rsym_t) + strlen(s->name);
}

static int32_t ord(int32_t x, int32_t y) {
  if (x < y) {
    return -1;
  } else if (x > y) {
    return 1;
  } else {
    return 0;
  }
}

static int32_t sym_cmp(chr_t* sx, hash_t hx, chr_t* sy, hash_t hy) {
  return ord(hx,hy) ? : strcmp(sx,sy);
}


rsym_t** vm_sym_lookup(chr_t* sx, hash_t hx, rsym_t** symtab) {
  if (hx == 0) {
    hx = hash_string(sx);
  }

  rsym_t** curr = symtab;
  
  while (*curr) {
    hash_t hy = symhash(*curr);
    chr_t* sy = (*curr)->name;
    int32_t result = sym_cmp(sx,hx,sy,hy);

    if (result < 0) {
      curr = &(*curr)->left;
    } else if (result > 0) {
      curr = &(*curr)->right;
    } else {
      break;
    }
  }
  
  return curr;
}

rsym_t* vm_assoc_sym(chr_t* sx, hash_t hx, rsym_t* symtab) {
  rsym_t* local = symtab;
  rsym_t** result = vm_sym_lookup(sx,hx,&local);
  return *result;
}

rsym_t* vm_intern_str(chr_t* string, rsym_t** st) {
  if (st == NULL) st = &GLOBALS;
  hash_t h = hash_string(string);
  rsym_t** result = vm_sym_lookup(string, h, st);

  if (*result == NULL) {
    *result = vm_new_sym(string, h,SYMFLAG_INTERNED);
  }

  return *result;
}

// environments
rcons_t* vm_new_env(rval_t n, rcons_t* b, rcons_t* parent) {
  // TODO: validate n and b
  rcons_t* locals = vm_new_cons(n, tagcons(b), REQUIRE_CONS);
  rcons_t* out = vm_new_list(tagcons(locals), tagcons(parent));
  return out;
}

rcons_t* vm_assoc_sym_env(rsym_t* v, rcons_t* e) {
  rcons_t* cr;

  if (e == NULL) {
    return NULL;
  }

  cr = vm_assoc_list(tagsym(v), _tocons(e->car));
    if (cr == NULL) {
      return vm_assoc_sym_env(v, _tocons(e->cdr));
	} else {
      return cr;
    }
}

rval_t vm_get_sym_env(rsym_t* v, rcons_t* e) {
  rcons_t* result = vm_assoc_sym_env(v,e);

  if (result == NULL) return v->binding;
  else return result->car;
}


// put_sym_env adds a symbol to the environment
void vm_put_sym_env(rsym_t* s, rcons_t* e) {
  if (e == NULL) return;

    rcons_t* names = _tocons(e->car);
    rcons_t* bindings = _tocons(e->cdr);

    vm_list_append(&names,tagsym(s));
    vm_list_append(&bindings,NIL);
}

// set_sym_env updates the value of an existing symbol
void vm_set_sym_env(rsym_t* s, rval_t v, rcons_t* e) {
  if (e == NULL) {
    s->binding = v;
    return;
  }

  rcons_t* result = vm_assoc_env(s, e);
  result->car = v;
  return;
}
