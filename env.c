#include "rascal.h"



/* 
   the core string type

   
*/


str_t* vm_str(chr_t* s) {
  str_t* out = (str_t*)vm_allocate(strlen(s) + 1);
  strcpy(out,s);
  return out;
}


inline val_t hash(val_t v) {
  sym_t* s = tosym(v);
  return hash_(s);
}

inline chr_t* name(val_t v) {
  sym_t* s = tosym(v);

  return name_(s);
}


/* simple comparison helpers */
inline int_t cmpi(int_t x, int_t y){
  if (x < y) return -1;
  else if (x > y) return 1;
  else return 0;
}

inline int_t cmpui(uint_t x, uint_t y) {
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
  return cmpui(hx,hy) ? : cmps(sx,sy);
}

inline int_t cmpv(val_t x, val_t y) {
  if (!cmpable(x)) {
    escapef(TYPE_ERR,stderr,"Incomparable type %s in first arg",typename(x));
  } if (!cmpable(y)) {
    escapef(TYPE_ERR,stderr,"Incomparable type %s in second arg",typename(y));
  }

  if (typecode(x) != typecode(y)) {   
    return cmpi(typecode(x),typecode(y));

  } else switch (typecode(x)) {
    case TYPECODE_NONE:
    case TYPECODE_NIL:
      return 0;
    default:
    return type_of(x)->tp_cmp(x,y);
  }
}

/* type specific cmp implementations */
inline int_t cmp_int(val_t x, val_t y) {
  return cmpi(val(x),val(y));
}

inline int_t cmp_str(val_t x, val_t y) {
  return cmps(tostr_(x),tostr_(y));
}

inline int_t cmp_sym(val_t x, val_t y) {
  return cmphs(name(x),hash(x),name(y),hash(y));
}

inline int_t cmp_type(val_t x, val_t y) {
  return cmpui(typecode_self_(x),typecode_self_(y));
}


/* dict safe accessors */
val_t key(val_t d) {
  dict_t* dd = todict(d);
  return key_(dd);
}

val_t binding(val_t d) {
  dict_t* dd = todict(d);
  return binding_(dd);
}

int_t keytype(val_t d) {
  dict_t* dd = todict(d);
  return keytype_(dd);
}

static val_t* intern_string(chr_t*, val_t*);


val_t sym(chr_t* s) {
  val_t* loc = intern_string(s,&GLOBALS);
  return key_(*loc);
}

sym_t* intern_builtin(chr_t* s, val_t b) {
  val_t* cell = intern_string(s,&GLOBALS);

  val_t cb = binding(*cell);
  if (istype(cb)) {                     // types are initialized before builtins, we must 
    totype(cb)->tp_new = b;               // make sure not to overwrite the type with its
  } else {                               // constructor
    binding_(*cell) = b;
  }
  sym_t* g_name = tosym_(key(*cell));
  fl_const_(g_name) = CONSTANT;
  return g_name;
}


static val_t* intern_string(chr_t* s, val_t* d) {
  // the string may or may not live in the lisp heap, so we need to save it in a local buffer
  chr_t bs[256];
  strncpy(bs,s,256);

  // trigger the GC ahead of time, ensuring that the value of d is updated
  val_t* locals[1] = { d };
  pre_gc(sizeof(dict_t) + sizeof(sym_t) + strlen(s), locals, 1);

  hash_t h = hash_string(bs);

  while (*d) {
    sym_t* currk = tosym(key_(*d));
    int_t compare = cmphs(s,h,name_(currk),hash_(currk));

    if (compare < 0) d = &(left_(*d));
    else if (compare > 0) d = &(right_(*d));
    else break;
  }

  if (*d == NIL) {
    // perform both allocations at once
    uchr_t* memloc = vm_allocate(sizeof(dict_t)+sizeof(sym_t)+strlen(bs));
    dict_t* new_dict = (dict_t*)memloc;
    keytype_(new_dict) = SYMBOL_KEYS;
    type_(new_dict) = TYPECODE_DICT;
    left_(new_dict) = right_(new_dict) = NIL;
    binding_(new_dict) = NONE;

    sym_t* new_sym = (sym_t*)(memloc + sizeof(dict_t));
    type_(new_sym) = TYPECODE_SYM;
    hash_(new_sym) = h;
    strcpy(name_(new_sym),bs);
    
  }

  return d;
}

val_t* dict_searchk(val_t k, val_t* d) {
  if (keytype(*d) == SYMBOL_KEYS && !issym(k)) {
    escapef(TYPE_ERR,stderr,"Wrong type %s for symbol table",typename(k));
  }
  
  while (*d) {
    val_t currk = key(*d);
    int_t order = cmpv(k, currk);

    if (order < 0) {
      d = &(left_(*d));
      
    }
    else if (order > 0) {
      d = &(left_(*d));
    } else {
      break;
    }
    }

  return d;
}

val_t dict_setk(val_t k, val_t v, val_t* d) {
  val_t* locals[3] = {&k, &v, d};

  pre_gc(sizeof(dict_t),locals,3);
  
  if (issym(k) && fl_const_(k) == CONSTANT) {
    escapef(TYPE_ERR,stderr,"Tried to reset global constant");
  }

  int_t ktp = isnil(*d) ? GENERAL_KEYS : keytype(*d);
  val_t* node = dict_searchk(k, d);

  if (*node == NIL) {
    dict_t* dd = dict();
    keytype_(dd) = ktp;
    key_(dd) = k;
    *node = tagp(dd);
  }

  binding_(*node) = v;
  
  return v;
}

val_t* dict_searchv(val_t v, val_t* d) {
  if (*d == NIL) {

    return d;

  } else if (keytype(*d) == SYMBOL_KEYS && !issym(v)) {

    escapef(TYPE_ERR,stderr,"Bad type for symbol table %s",typename(v));
    
  } else if (v == binding_(*d)) {
    return d;
  } else {
    val_t* search_left = dict_searchv(v,&left_(*d));
    if (search_left != NULL) {
      return search_left;
    } else {
      return dict_searchv(v,&right_(*d));
    }
  }
}

dict_t* dict() {
  dict_t* out = (dict_t*)vm_allocate(sizeof(dict_t));
  type_(out) = TYPECODE_DICT;     
  key_(out) = NONE;                     
  binding_(out) = NONE;                  
  left_(out) = right_(out) = NIL;
  
  return out;
}

val_t r_dict(val_t args) {
  if (!iscons(args)) {
    escapef(TYPE_ERR, stderr, "Malformed arguments list");
  }

  uint_t n_args = ncells(args);
  
  if (n_args % 2) {
    escapef(ARITY_ERR,stderr,"Bad parity %d for argument list",n_args);
  }

  uint_t n_nodes = n_args / 2;
  val_t* locals[1] = { &args };
  pre_gc(sizeof(dict_t) * n_nodes,locals,1);

  val_t out = NIL;
  while (iscons(args)) {
    val_t k = car(args);
    args = cdr(args);
    val_t v = car(args);
    args = cdr(args);
    dict_setk(k,v,&out);
  }

  return tagp(out);
}

/* generic functions for object indexing and search */
val_t r_assr(val_t k, val_t o) {
  if (isdict(o)) {
    val_t* r = dict_searchk(k, &o);
    if (*r == NIL) {
      return NIL;
    } else {
      return binding(*r);
    }
  } else if (islist(o)) {
    int_t idx = toint(k);

    while (idx && iscons(o)) {
      idx--;
      o = cdr(o);
    }

    return o;
    
  } else {
    escapef(TYPE_ERR, stderr, "assr only supports dicts and lists.");
  }
}

val_t r_assv(val_t v, val_t o) {
  if (isdict(o)) {
    val_t* r = dict_searchv(v, &o);

    if (r == NULL) {
      return NIL;
    } else {
      return binding(*r);
    }

  } else if (islist(o)) {
    int_t count = 0;
    while (iscons(o)) {
      if (v == car(o)) {
	return tagv(count);
      }
      count++;
      o = cdr(o);
    }
    return isnil(o) ? NIL : tagv(count);
    
    } else {
    escapef(TYPE_ERR, stderr, "assr only supports dicts and lists.");
  }
}

val_t r_setr(val_t r, val_t v, val_t o) {
  if (isdict(o)) {
    return dict_setk(r,v,&o);

  } else if (islist(o)) {
    int_t i = toint(r);
    val_t c = o;
 
    while (i && iscons(c)) {
      i--;
      c = cdr(c);
    }

    if (i) {
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
  if (islist(o)) {
    val_t c = o;
 
    while (iscons(c)) {
      if (car(c) == v) {
	car_(c) = n;
	return o;
      } else {
	c = cdr(c);
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
  if (e == NIL) {
    return NONE;
  }
  
  val_t* result = &NIL;
  val_t locals = car(e);

  if (isdict(locals)) {
    result = dict_searchk(v, &locals);

  } else if (iscons(locals)) {
      val_t ln = car(locals);
      val_t lb = cdr(locals);

      while (iscons(ln)) {
	if (car(ln) == v) {
	  result = &lb;
	  break;
	}

	ln = cdr(ln);
	lb = cdr(lb);
      }

      if (issym(ln)) result = ln == v ? &lb : &NIL;

  } else {
    escapef(TYPE_ERR,stderr,"Incorrect type for locals.");
  }

  return *result == NIL ? vm_asse(v,cdr(e)) : *result;
}

val_t vm_gete(val_t v, val_t e) {
  /* 
     env can safely assume that v is a symbol and e is an
     environment.
  */
  val_t loc = vm_asse(v,e);
  if (isdict(loc)) {
    return binding(loc);

  } else if (iscons(loc)) {
    return car(loc);

  } else {
    return NONE;
  }
}


val_t vm_pute(val_t n, val_t e) {
  sym_t* sn = tosym(n);

  if (fl_const_(sn) == CONSTANT) {
    escapef(TYPE_ERR,stderr,"Attempt to rebind constant");
  }

  val_t locals = car(e);

  if (isdict(locals)) {
    if (locals != GLOBALS) r_setr(n,NONE,locals);
      return OK;
      
  } else if (iscons(locals)) {
      val_t* localvs[1] = { &locals };
      pre_gc(sizeof(cons_t) * 2, localvs, 1);

      car_(locals) = cons(n,car(locals));
      cdr_(locals) = cons(NONE,cdr(locals));
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
