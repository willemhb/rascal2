#include "builtin.h"

#define ADD(x,y)   (x + y)
#define SUB(x,y)   (x - y)
#define MUL(x,y)   (x * y)
#define DIV(x,y)   (x / y)
#define REM(x,y)   (x % y)
#define EQL(x,y)   (x == y)
#define LT(x,y)    (x < y)
#define IS_ZERO(x) (x == 0)


DESCRIBE_ARITHMETIC(r_add,ADD,r_int)
DESCRIBE_ARITHMETIC(r_sub,SUB,r_int)
DESCRIBE_ARITHMETIC(r_div,DIV,r_int)
DESCRIBE_ARITHMETIC(r_mul,MUL,r_int)
DESCRIBE_ARITHMETIC(r_rem,REM,r_int)
DESCRIBE_ARITHMETIC(r_eqnum,EQL,cbooltorbool)
DESCRIBE_ARITHMETIC(r_lt,LT,cbooltorbool)
DESCRIBE_PREDICATE(r_nilp,isnil)
DESCRIBE_PREDICATE(r_nonep,isnone)


static const chr_t* builtin_forms[] = {"setv", "quote", "let",
                                       "do", "fn", "macro", "if" };



const chr_t* builtin_fnames[] = {
  "+", "-", "*", "/", "rem",                  // arithmetic
  "<", "=", "eq?", "nil?","none?",            // predicates and comparison
  "cmp", "type", "isa?", "size", "cons",
  "sym", "str", "int", "car", "cdr",
  "rplca", "rplcd", "open", "close", "read",
  "prn", "reads", "load", "eval", "apply",         
};

static const int_t builtin_argcos[] = {
  2, 2, 2, 2, 2,
  2, 2, 2, 1, 1,
  2, 1, 2, 1, 2,
  1, 1, 1, 1, 1,
  2, 2, 2, 1, 1,
  2, 1, 1, 2, 2,
};

static const void* builtin_callables[] = {
  r_add, r_sub, r_mul, r_div, r_rem,
  r_lt, r_eqnum, r_eqp,				       
  r_nilp, r_nonep, r_cmp, r_typeof,
  r_isa, r_size, cons, r_sym,
  r_str, r_int, car, cdr, r_rplca, r_rplcd,
  open, close, read, prn, reads,
  load, r_eval, r_apply,
};


inline val_t r_isa(val_t v, val_t t) {
  type_t* tp = totype(t);
  return type_of(v) == tp ? T : NIL;
}

inline val_t r_eqp(val_t x, val_t y) {
  return x == y  ? T : NIL;
}

val_t r_size(val_t v) {
  return tagi(vm_size(v));
}

val_t r_cmp(val_t x, val_t y) {
  return tagi(cmpv(x,y));
}

inline val_t r_typeof(val_t v) {
  return tagp(type_of(v));
}

val_t r_rplca(val_t ca,val_t a) {
  cons_t* c = tocons(ca);
  car_(c) = a;
  return ca;
}

val_t r_rplcd(val_t cd,val_t d) {
    cons_t* c = tocons(cd);
  car_(c) = d;
  return cd;
}


val_t r_eval(val_t x,val_t e) {
  return eval_expr(-1,x,e);
}

val_t r_apply(val_t p, val_t args) {
  val_t mock_proc = cons(p,NIL);
  cdr_(mock_proc) = args;

  return eval_expr(-1,mock_proc,NONE);
}


val_t r_sym(val_t v) {
  chr_t* b;
  sym_t* out;
  switch(typecode(v)) {
  case TYPECODE_SYM:
    return v;
  case TYPECODE_INT:
    b = itoa(toint_(v),10);
    out = tosym_(sym(b));
    free(b);
    return tagp(out);
  case TYPECODE_STR:
    return sym(tostr_(v));
  default:
    escapef(TYPE_ERR,stderr,"Invalid argument of type %s",typename(v));
  }
}


val_t r_str(val_t v) {
  chr_t* b;
  str_t* out;
  switch(typecode(v)) {
  case TYPECODE_SYM:
    return tagp(new_str(name_(v)));
  case TYPECODE_INT:
    b = itoa(toint_(v),10);
    out = new_str(b);
    free(b);
    return tagp(out);
  case TYPECODE_STR:
    return v;
  default:
    escapef(TYPE_ERR,stderr,"Invalid argument of type %s",typename(v));
  }
}


val_t r_int(val_t v) {
  switch(typecode(v)) {
  case TYPECODE_SYM:
    return tagi(atoi(name_(v)));
  case TYPECODE_INT:
    return v;
  case TYPECODE_STR:
    return tagi(atoi(tostr_(v)));
  default:
    escapef(TYPE_ERR,stderr,"Invalid argument of type %s",typename(v));
  }
}

void _new_builtin_function(const chr_t* fname, int_t argc, const void* f) {
  val_t obj = new_proc(tagi(argc),NIL,((val_t)f),CALLMODE_FUNC,BODYTYPE_CFNC);
  intern_builtin(fname,obj);
  return;
}

void init_builtin_functions() {
  for (int_t i = 0; i < NUM_BUILTINS; i++) {
    _new_builtin_function(builtin_fnames[i],builtin_argcos[i],builtin_callables[i]);
  }
  return;
}

void init_forms() {
  for (int_t i = EV_SETV; i < NUM_FORMS; i++) {
    val_t new_form = _new_self_evaluating(builtin_forms[i]);
    BUILTIN_FORMS[i] = new_form;
  }
  return;
}

val_t _new_self_evaluating(chr_t* v) {
  val_t name = sym(v);
  intern_builtin(v,name);
  return name;
}

void init_special_variables() {
  intern_builtin("nil", NIL);
  intern_builtin("none",NONE);
  intern_builtin("t",T);
  intern_builtin("ok",OK);

  val_t r_stdin = _std_port(stdin);
  intern_builtin("stdin",r_stdin);
  R_STDIN = r_stdin;
  
  val_t r_stdout = _std_port(stdout);
  intern_builtin("stdout",r_stdout);
  R_STDOUT = r_stdout;
  
  val_t r_stderr = _std_port(stderr);
  intern_builtin("stderr",r_stderr);
  R_STDERR = r_stderr;

  intern_builtin("*globals*",tagp(GLOBALS));
  R_PROMPT = tagp(sym("rsc> "));
  intern_builtin("*prompt*",R_PROMPT);

  return;
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

void init_registers() {
  NIL = 0; NONE = TYPECODE_NONE << 3; FPTR = LOWTAG_CONSOBJ;
  EXP = VAL = CONTINUE = NAME = ENV = UNEV = ARGL = PROC = NIL;
  WRX = WRY = WRZ = NIL;
  STACK = malloc(MAXSTACK * 8);
  SP = -1;
  GLOBALS = NULL;
  T = tagi(1);
  OK = tagi(2);
  TOKPTR = 0;
  TOKTYPE = TOK_NONE;
  return;
}

void init_types() {
  TYPES = malloc(sizeof(type_t*) * INIT_NUMTYPES);
  TYPECOUNTER = NUM_BUILTIN_TYPES + 1;
  return;
}

void init_builtin_types() {
  static const chr_t* builtin_typenames[] = { "nil-type", "cons", "none-type", "str",
                                            "type", "sym", "tab", "proc", "port", "int", };
  /* 

     the builtin types are initialized with their names set to null; their names are interned
     and bound after all types have been initialized.

   */

  // nil type
  type_t* niltype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_NIL] = niltype;
  niltype->head.meta = TYPECODE_NONE;
  niltype->head.type = TYPECODE_TYPE;
  niltype->flags.base_size = 8;
  niltype->flags.val_lowtag = LOWTAG_DIRECT;
  niltype->flags.atomic = 1;
  niltype->flags.callable = 0;
  niltype->flags.free = 0;
  niltype->tp_new = NIL;
  niltype->tp_sizeof = NULL;
  niltype->tp_prn = NULL;

  // cons type
  type_t* constype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_CONS] = constype;
  constype->head.meta = TYPECODE_CONS;
  constype->head.type = TYPECODE_TYPE;
  constype->flags.base_size = 16;
  constype->flags.val_lowtag = LOWTAG_CONSPTR;
  constype->flags.atomic = 0;
  constype->flags.callable = 0;
  constype->flags.free = 0;
  constype->tp_new = NIL;
  constype->tp_sizeof = NULL;
  constype->tp_prn = prn_cons;

  type_t* nonetype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_NONE] = nonetype;
  nonetype->head.meta = TYPECODE_NONE;
  nonetype->head.type = TYPECODE_TYPE;
  nonetype->flags.base_size = 8;
  nonetype->flags.val_lowtag = LOWTAG_DIRECT;
  nonetype->flags.atomic = 1;
  nonetype->flags.callable = 0;
  nonetype->flags.free = 0;
  nonetype->tp_new = NIL;
  nonetype->tp_sizeof = NULL;
  nonetype->tp_prn = NULL;

  
  type_t* strtype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_STR] = strtype;
  strtype->head.meta = TYPECODE_STR;
  strtype->head.type = TYPECODE_TYPE;
  strtype->flags.base_size = 16;
  strtype->flags.val_lowtag = LOWTAG_STRPTR;
  strtype->flags.atomic = 0;
  strtype->flags.callable = 0;
  strtype->flags.free = 0;
  strtype->tp_new = NIL;
  strtype->tp_sizeof = NULL;
  strtype->tp_prn = prn_str;

  type_t* typetype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_TYPE] = typetype;
  typetype->head.meta = TYPECODE_TYPE;
  typetype->head.type = TYPECODE_TYPE;
  typetype->flags.base_size = sizeof(type_t);
  typetype->flags.val_lowtag = LOWTAG_OBJPTR;
  typetype->flags.atomic = 1;
  typetype->flags.callable = 1;
  typetype->flags.free = 0;
  typetype->tp_new = NIL;
  typetype->tp_sizeof = NULL;
  typetype->tp_prn = prn_type;

  
  type_t* symtype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_SYM] = symtype;
  symtype->head.meta = TYPECODE_SYM;
  symtype->head.type = TYPECODE_TYPE;
  symtype->flags.base_size = 16;
  symtype->flags.val_lowtag = LOWTAG_STROBJ;
  symtype->flags.atomic = 1;
  symtype->flags.callable = 0;
  symtype->flags.free = 0;
  symtype->tp_new = NIL;
  symtype->tp_sizeof = NULL;
  symtype->tp_prn = prn_sym;


  type_t* tabtype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_TAB] = tabtype;
  tabtype->head.meta = TYPECODE_TAB;
  tabtype->head.type = TYPECODE_TYPE;
  tabtype->flags.base_size = sizeof(tab_t);
  tabtype->flags.val_lowtag = LOWTAG_OBJPTR;
  tabtype->flags.atomic = 0;
  tabtype->flags.callable = 0;
  tabtype->flags.free = 0;
  tabtype->tp_new = NIL;
  tabtype->tp_sizeof = NULL;
  tabtype->tp_prn = NULL;


  type_t* proctype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_PROC] = proctype;
  proctype->head.meta = TYPECODE_PROC;
  proctype->head.type = TYPECODE_TYPE;
  proctype->flags.base_size = sizeof(proc_t);
  proctype->flags.val_lowtag = LOWTAG_OBJPTR;
  proctype->flags.atomic = 0;
  proctype->flags.callable = 0;
  proctype->flags.free = 0;
  proctype->tp_new = NIL;
  proctype->tp_sizeof = NULL;
  proctype->tp_prn = prn_proc;


type_t* porttype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_PORT] = porttype;
  porttype->head.meta = TYPECODE_PORT;
  porttype->head.type = TYPECODE_TYPE;
  porttype->flags.base_size = sizeof(port_t);
  porttype->flags.val_lowtag = LOWTAG_OBJPTR;
  porttype->flags.atomic = 0;
  porttype->flags.callable = 0;
  porttype->flags.free = 0;
  porttype->tp_new = NIL;
  porttype->tp_sizeof = NULL;
  porttype->tp_prn = NULL;


type_t* inttype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_INT] = inttype;
  inttype->head.meta = TYPECODE_PORT;
  inttype->head.type = TYPECODE_TYPE;
  inttype->flags.base_size = sizeof(port_t);
  inttype->flags.val_lowtag = LOWTAG_OBJPTR;
  inttype->flags.atomic = 0;
  inttype->flags.callable = 0;
  inttype->flags.free = 0;
  inttype->tp_new = NIL;
  inttype->tp_sizeof = NULL;
  inttype->tp_prn = prn_int;

  for (int_t i = 0; i < NUM_BUILTIN_TYPES; i++) {
    sym_t* tpname = intern_builtin(builtin_typenames[i],tagp(TYPES[i]));
    TYPES[i]->tp_name = tpname;
  }

  return;
}

void bootstrap_rascal() {
  init_heap();
  init_types();
  init_registers();
  init_builtin_types();
  init_builtin_functions();
  init_special_variables();
  init_forms();

  fprintf(stdout,"Initialization succeeded.\n");
}
