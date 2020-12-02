#include "env.h"


inline ival_t _vhash(val_t v) {
  sym_t* s = tosym(v);

  if (checkerr(s)) {
    failv(TYPE_ERR,-2,"Expected sym, got %s",typename(v));
  }

  return hash_(s);
}

inline ival_t _shash(sym_t* s) {
  if (s == NULL) return seterr(NULLPTR_ERR,-2);
  return hash_(s);
}

inline chr_t* _vname(val_t v) {
  sym_t* s = tosym(v);

  if (checkerr(s)) {
    failv(TYPE_ERR,NULL,"expected sym, got %s", typename(v));
  }
  
  return name_(s);
}

inline chr_t* _sname(sym_t* s) {
  if (s == NULL) return seterr(NULLPTR_ERR, NULL); 
  return name_(s);
}

// table accessors
inline val_t* _vkey(val_t v) {
  if (!istab(v)) return seterr(TYPE_ERR,NULL);
  return &(key_(totab_(v)));
}

inline val_t* _tkey(tab_t* t) {
  if (t == NULL) return seterr(NULLPTR_ERR,NULL);

  return &(key_(t));
}

inline val_t* _vbinding(val_t v) {
  if (!istab(v)) return seterr(TYPE_ERR,NULL);

  return &(binding_(totab_(v)));
}


inline val_t* _tbinding(tab_t* t) {
  if (t == NULL) return seterr(NULLPTR_ERR,NULL);

  return &(binding_(t));
}


inline tab_t** _vleft(val_t v) {
  if (!istab(v)) return seterr(TYPE_ERR,NULL);

  return &(left_(totab_(v)));
}


inline tab_t** _tleft(tab_t* t) {
  if (t == NULL) return seterr(NULLPTR_ERR,NULL);

  return &(left_(t));
}


inline tab_t** _vright(val_t v) {
  if (!istab(v)) return seterr(TYPE_ERR,NULL);

  return &(right_(totab_(v)));
}


inline tab_t** _tright(tab_t* t) {
  if (t == NULL) return seterr(NULLPTR_ERR,NULL);

  return &(right_(t));
}


/* simple comparison helpers */
inline int_t cmpv(val_t x, val_t y) {
  if (x < y) return -1;
  else if (x > y) return 1;
  else return 0;
}

inline int_t cmps(chr_t* sx, chr_t* sy) {
  int_t r = strcmp(sx,sy);
  if (r < 0) return -1;
  else if (r > 0) return 1;
  else return 0;
}

inline int_t cmph(hash_t x, hash_t y) {
  if (x < y) return -1;
  else if (x > y) return 1;
  else return 0;
}

inline int_t cmphs(chr_t* sx, uint_t hx, chr_t* sy, uint_t hy) {
  return cmph(hx,hy) ? : cmps(sx,sy);
}

inline int_t cmpq(sym_t* sx, sym_t* sy) {
  return cmphs(name_(sx),hash_(sx),name_(sy),hash_(sy));
}

sym_t* new_sym(chr_t* s) {
  tab_t* cell = intern_string(s,&GLOBALS);
  sym_t* out = tosym_(key_(cell));
  return out;
}

tab_t* intern_string(chr_t* s, tab_t** st) {
  hash_t h = hash_string(s);
  tab_t** curr = st;

  while (*curr) {
    sym_t* currk = tosym_(key_(*curr));
    int_t c = cmphs(s,h, name_(currk), hash_(currk));

    if (c < 0) curr = &(left_(*curr));
    else if (c > 0) curr = &(right_(*curr));
    else break;
  }

  if (*curr == NULL) {
    tab_t* new_t = new_tab();
    sym_t* new_s = (sym_t*)vm_allocate(sizeof(sym_t)+strlen(s));
    strcpy(name_(new_s),s);
    hash_(new_s) = h;
    type_(new_s) = TYPECODE_SYM;
    key_(new_t) = tagp(new_s);
    *curr = new_t;
  }

  return *curr;
}

/* tab_search takes advantage of the interning by doing a simple pointer comparison */
tab_t* tab_search(val_t k, tab_t* tab) {
  tab_t* curr = tab;

  while (curr) {
    val_t currk = (curr)->key;
    int_t order = cmpv(k, currk);

    if (order < 0) curr = curr->left;
    else if (order > 1) curr = curr->right;
    else break;
    }

  return curr;
}

tab_t* tab_setkey(val_t k, val_t v, tab_t* t) {
  tab_t* node = tab_search(k,t);

  if (checkerr(node)) return seterr(VALUE_ERR,NULL);
 
  if (isconst(node) && node->binding != NONE) {
    failv(TYPE_ERR,NULL,"Tried to reset constant binding");
  }

  node->binding = v;
  return node;
}

tab_t* new_tab() {
  tab_t* out = (tab_t*)vm_allocate(sizeof(tab_t));
  type_(out) = TYPECODE_TAB;
  meta_(out) = TYPECODE_SYM;        // the types of the keys in this table
  key_(out) = NONE;                      // for now, all keys must have the same type,
  binding_(out) = NONE;                  // and the only allowed type is symbol
  left_(out) = right_(out) = NULL;
  fl_const(out) = CONSTANT;
  
  return out;
}


bool isenvnames(val_t n) {
  if (issym(n)) return true;
  if (isnil(n)) return true;
  if (!iscons(n)) return false;

  while (iscons(n)) {
    if (!issym(car_(n))) {
      return false;
    }
    n = cdr_(n);
  }

  return isnil(n) || issym(n);
}

// environments
cons_t* new_env(val_t n, val_t b, val_t p) {
  // validate arguments
  if (!isenvnames(n)) {
    failv(TYPE_ERR,NULL,"Invalid environment names");
  }

  if (!islist(b)) {
    failv(TYPE_ERR,NULL,"Invalid environment bindings");
  }
  
  if (!islist(p)) {
    failv(TYPE_ERR,NULL,"Invalid environment tail");
}

  val_t locals = cons(n, b);
  cons_t* out = new_cons(locals, p);
  return out;
}

val_t env_assoc(val_t v, val_t e) {
  /* 
     env can safely assume that v is a symbol, e is an 
     and the local environment frame is correctly formed.
   
  */
  if (e == NIL) {
    tab_t* global = tab_search(v,GLOBALS);
    return tagp(global);

  } else {
    val_t ln = *cxr(e,AA);
    val_t lb = *cxr(e,AD);
    
    while (iscons(ln)) {
      if (car_(ln) == v) return lb;
      else ln = cdr_(ln); lb = cdr_(lb);
    }

    if (ln == v) return lb;
    else return env_assoc(v, cdr_(e));
  }
}
