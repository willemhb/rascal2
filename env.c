#include "env.h"


inline val_t hash(val_t v) {
  sym_t* s = tosym(v);
  return hash_(s);
}

inline chr_t* name(val_t v) {
  sym_t* s = tosym(v);

  return name_(s);
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

inline int_t cmphs(chr_t* sx, uint_t hx, chr_t* sy, uint_t hy) {
  return cmpv(hx,hy) ? : cmps(sx,sy);
}

static tab_t* intern_string(chr_t* s, tab_t** st);

val_t sym(chr_t* s) {
  tab_t* cell = intern_string(s,&GLOBALS);
  sym_t* out = tosym_(key_(cell));
  return tagp(out);
}

static tab_t* intern_string(chr_t* s, tab_t** st) {
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

val_t tab_set(val_t k, val_t v, val_t t) {
  tab_t* root = totab(t);
  tab_t* node = tab_search(k, root);

  if (node == NULL) { escapef(NULLPTR_ERR,stderr,"Unexpected null pointer in tab_set."); }

  binding_(node) = v;
  
  return v;
}

val_t tab_get(val_t k, val_t t) {
  tab_t* root = totab(t);
  tab_t* node = tab_search(k, root);

  if (node == NULL) { escapef(NULLPTR_ERR,stderr,"Unexpected null pointer in tab_get."); }

  return binding_(node);
}

tab_t* new_tab() {
  tab_t* out = (tab_t*)vm_allocate(sizeof(tab_t));
  type_(out) = TYPECODE_TAB;
  meta_(out) = TYPECODE_SYM;        // the types of the keys in this table
  key_(out) = NONE;                      // for now, all keys must have the same type,
  binding_(out) = NONE;                  // and the only allowed type is symbol
  left_(out) = right_(out) = NULL;
  
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
val_t new_env(val_t n, val_t b, val_t p) {
  val_t locals = cons(n, b);

  return cons(locals, p);
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
    val_t ln = cxr(e,AA);
    val_t lb = cxr(e,AD);
    
    while (iscons(ln)) {
      if (car_(ln) == v) return lb;
      else ln = cdr_(ln); lb = cdr_(lb);
    }

    if (ln == v) return lb;
    else return env_assoc(v, cdr_(e));
  }
}


val_t env_put(val_t n, val_t e) {
  if (e == NIL) {
    return OK;
  } else {
    val_t* loc_n = &car_(car_(e));
    val_t* loc_b = &cdr_(car_(e));

    while (iscons(*loc_n)) {
      loc_n = &cdr_(*loc_n);
      loc_b = &cdr_(*loc_b);
    }

    *loc_n = cons(n, *loc_n);
    *loc_b = cons(NIL, *loc_b);
    
    return OK;
  }
}

val_t env_set(val_t n, val_t v, val_t e) {
  if (e == NIL) {
    return tab_set(n,v,tagp(GLOBALS));
  } else {
    val_t loc_b = env_assoc(n,e);
    car_(loc_b) = v;
    return v;
  }
}
