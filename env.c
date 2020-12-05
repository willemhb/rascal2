#include "rascal.h"


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

inline int_t cmps(const chr_t* sx, const chr_t* sy) {
  int_t r = strcmp(sx,sy);
  if (r < 0) return -1;
  else if (r > 0) return 1;
  else return 0;
}

inline int_t cmphs(const chr_t* sx, uint_t hx, const chr_t* sy, uint_t hy) {
  return cmpv(hx,hy) ? : cmps(sx,sy);
}

static dict_t* intern_string(const chr_t*, dict_t**);


val_t sym(chr_t* s) {
  dict_t* loc = intern_string(s,&GLOBALS);
  sym_t* out = tosym_(key_(loc));
  return tagp(out);
}

sym_t* intern_builtin(const chr_t* s, val_t b) {
  dict_t* cell = intern_string(s,&GLOBALS);
  
  if (istype(binding_(cell))) { // types are initialized before builtins, so we need to 
    totype_(binding_(cell))->tp_new = b; // make sure not to overwrite the type with its
  } else {                               // constructor
    binding_(cell) = b;
  }
  sym_t* g_name = tosym_(key_(cell));
  fl_const_(g_name) = CONSTANT;
  return g_name;
}

static dict_t* intern_string(const chr_t* s, dict_t** d) {
  dict_t** curr = d;
  hash_t h = hash_string(s);

  while (*curr) {
    sym_t* currk = tosym((*curr)->key);
    int_t compare = cmphs(s,h,name_(currk),hash_(currk));

    if (compare < 0) curr = &(left_(*curr));
    else if (compare > 0) curr = &(right_(*curr));
    else break;
  }

  if (*curr == NULL) {
    *curr = dict(TYPECODE_SYM);
    sym_t* new_sym = (sym_t*)vm_allocate(sizeof(sym_t)+strlen(s));
    type_(new_sym) = TYPECODE_SYM;
    hash_(new_sym) = h;
    strcpy(name_(new_sym),s);
    key_(*curr) = tagp(new_sym);
  }

  return *curr;
}

static dict_t** dict_sym_search(sym_t* s, dict_t** d) {
  dict_t** curr = d;

  while (*curr) {
    sym_t* currk = tosym((*curr)->key);
    int_t compare = cmphs(name_(s),hash_(s),name_(currk),hash_(currk));

    if (compare < 0) curr = &(left_(*curr));
    else if (compare > 0) curr = &(right_(*curr));
    else break;
  }

  return curr;
}


dict_t** dict_searchk(val_t k, dict_t** dict) {
  dict_t** curr = dict;

  if (issym(k) && keytype_(*dict) == TYPECODE_SYM) {
    return dict_sym_search(tosym(k),dict);
  }

  while (*curr) {
    val_t currk = (*curr)->key;
    int_t order = cmpv(k, currk);

    if (order < 0) curr = &((*curr)->left);
    else if (order > 0) curr = &((*curr)->right);
    else break;
    }

  return curr;
}

dict_t* dict_setk(val_t k, val_t v, dict_t* d) {
  if (d == NULL) {
    d = dict(TYPECODE_ANY);
    key_(d) = k;
    binding_(d) = v;
    return d;
  }
  uint_t kt = keytype_(d);
  if (kt != TYPECODE_ANY && kt != typecode(k)) {
    escapef(TYPE_ERR,stderr,"Bad key of type %s",typename(k));
  }

  if (issym(k) && fl_const_(k) == CONSTANT) {
    escapef(TYPE_ERR,stderr,"Tried to reset global constant");
  }

  dict_t** node = dict_searchk(k, &d);

  if (*node == NULL) {
    *node = dict(kt);
    key_(*node) = k;
  }
  binding_(*node) = v;
  
  return d;
}

dict_t* dict_searchv(val_t v, dict_t* d) {
  if (d == NULL) {
    return d;
  } else if (v == binding_(d)) {
    return d;
  } else {
    dict_t* search_left = dict_searchv(v,left_(d));
    if (search_left != NULL) {
      return search_left;
    } else {
      return dict_searchv(v,right_(d));
    }
  }
}

dict_t* dict(int_t tc) {
  dict_t* out = (dict_t*)vm_allocate(sizeof(dict_t));
  type_(out) = TYPECODE_DICT;
  keytype_(out) = tc;        
  key_(out) = NONE;                     
  binding_(out) = NONE;                  
  left_(out) = right_(out) = NULL;
  
  return out;
}

val_t r_dict(val_t args) {
  if (!iscons(args)) {
    escapef(TYPE_ERR, stderr, "Malformed arguments list");
  }
  dict_t* out = NULL;
  while (iscons(args)) {
    val_t k = car_(args);
    args = cdr_(args);
    val_t v = isnil(args) ? NIL : car_(args);
    args = cdr_(args);
    out = dict_setk(k,v,out);
  }

  if (out == NULL) {
    escapef(TYPE_ERR, stderr, "Cannot create a dictionary with no keys.");
  }

  return tagp(out);
}

/* generic functions for object indexing and search */
val_t r_assr(val_t k, val_t o) {
  if (isdict(o)) {
    dict_t* d = todict_(o);
    dict_t** r = dict_searchk(k, &d);
    if (*r == NULL) {
      return NIL;
    } else {
      return cons(key_(*r), binding_(*r));
    }
  } else if (iscons(o)) {
    int_t idx = toint(k);

    while (idx && iscons(o)) {
      idx--;
      o = cdr_(o);
    }

    return o;
    
  } else {
    escapef(TYPE_ERR, stderr, "assr only supports dicts and lists.");
  }
}

val_t r_assv(val_t v, val_t o) {
  if (isdict(o)) {
    dict_t* d = todict_(o);
    dict_t* r = dict_searchv(v, d);
    if (r == NULL) {
      return NIL;
    } else {
      return cons(key_(r), binding_(r));
    }
  } else if (iscons(o)) {
    int_t count = 0;
    while (iscons(o)) {
      if (v == car_(o)) {
	return tagv(count);
      }
      count++;
      o = cdr_(o);
    }

    if (!isnil(o) && v == o) {
      return tagv(count);
    }
    else {
      return NIL;
    }
  } else if (issym(o)) {
    if (v == o) {
      return 0;
    } else {
      return NIL;
    }
    } else {
    escapef(TYPE_ERR, stderr, "assr only supports dicts and lists.");
  }
}

val_t r_setr(val_t r, val_t v, val_t o) {
  if (isdict(o)) {
    dict_t* d = todict_(o);
    return tagp(dict_setk(r,v,d));
  } else if (iscons(o)) {
    int_t i = toint(r);
    val_t c = 0;
    while (i && iscons(c)) {
      i--;
      c = cdr_(c);
    }

    if (i) {
      return NIL;
    } else if (!iscons(c)) {
      return NIL;
    } else {
      car_(c) = v;
      return o;
    }
  } else {
    escapef(TYPE_ERR,stderr,"setr not implemented for %s",typename(o));
  }
}

val_t r_rplcv(val_t v, val_t n, val_t o) {
  if (iscons(o)) {
    val_t c = o;
    while (iscons(c)) {
      if (car_(c) == v) {
	car_(c) = n;
	return o;
      } else {
	c = cdr_(c);
      }
    }

    return NIL;
  } else {
    escapef(TYPE_ERR,stderr,"Not implemented for %s",typename(o));
  }
}

bool isenvnames(val_t n) {
  if (issym(n)) return true;
  if (isnil(n)) return true;
  if (!iscons(n)) return false;

  return isenvnames(car(n)) && isenvnames(cdr(n));

}

// environments
val_t new_env(val_t n, val_t b, val_t p) {
  val_t locals = cons(n, b);

  return cons(locals, p);
}

val_t vm_asse(val_t v, val_t e) {
  /* 
     env can safely assume that v is a symbol and e is an
     environment.
  */
  if (e == NIL) {
    return NONE;
  } else {
    val_t locals = car(e);
    if (isdict(locals)) {
      dict_t* local_d = todict_(locals);
      dict_t** result = dict_searchk(v, &local_d);

      if (*result == NULL) {
	fprintf(stdout, "Key not found in dict environment.\n");
	return vm_asse(v, cdr_(e));
      } else {
	return tagp(*result);
      }
    } else if (iscons(locals)) {
      val_t ln = car_(locals);
      val_t lb = cdr_(locals);

      val_t i = r_assv(v,ln);
      if (isnil(i)) {
	return vm_asse(v, cdr_(e));
      } else {
	return r_assr(i,lb);
      }
    } else {
      escapef(TYPE_ERR,stderr,"Incorrect type for locals.");
    }
  }
}


val_t vm_pute(val_t n, val_t e) {
  sym_t* sn = tosym(n);
  if (fl_const_(sn) == CONSTANT) {
    escapef(TYPE_ERR,stderr,"Attempt to rebind constant");
  }
    val_t locals = car(e);
    if (isdict(locals)) {
      if (todict_(locals) == GLOBALS) {
	return OK;
      } else {
	r_setr(n,NONE,locals);
	return OK;
      }
    } else if (iscons(locals)) {
      car_(locals) = cons(n,car_(locals));
      cdr_(locals) = cons(NIL,cdr_(locals));
      return OK;
    } else {
      escapef(TYPE_ERR,stderr,"Incorrect environment type.");
    }
}

val_t vm_sete(val_t n, val_t v, val_t e) {
  sym_t* sn = tosym(n);
  if (fl_const_(sn) == CONSTANT) {
    escapef(TYPE_ERR,stderr,"Attempt to rebind constant");
  }

  if (e == NIL) {
    escapef(UNBOUND_ERR,stderr,"Couldn't find name in globals");
  } else {
    val_t loc = vm_asse(n,e);

    if (iscons(loc)) {
      car_(loc) = v;
      return v;
    } else if (isdict(loc)) {
      binding_(loc) = v;
      return v;
    } else {
      escapef(UNBOUND_ERR,stderr,"Couldn't find name in env");
    }
  }
}
