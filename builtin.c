#include "rascal.h"

#define ADD(x,y)   (x + y)
#define SUB(x,y)   (x - y)
#define MUL(x,y)   (x * y)
#define DIV(x,y)   (x / y)
#define REM(x,y)   (x % y)
#define EQL(x,y)   (x == y)
#define LT(x,y)    (x < y)
#define IS_ZERO(x) (x == 0)


DESCRIBE_ARITHMETIC(r_add,ADD,vm_int)
DESCRIBE_ARITHMETIC(r_sub,SUB,vm_int)
DESCRIBE_ARITHMETIC(r_div,DIV,vm_int)
DESCRIBE_ARITHMETIC(r_mul,MUL,vm_int)
DESCRIBE_ARITHMETIC(r_rem,REM,vm_int)
DESCRIBE_ARITHMETIC(r_eqnum,EQL,cbooltorbool)
DESCRIBE_ARITHMETIC(r_lt,LT,cbooltorbool)
DESCRIBE_PREDICATE(r_nilp,isnil)
DESCRIBE_PREDICATE(r_nonep,isnone)

inline val_t vm_int(int_t i) {
  return tagv(i);
}

inline val_t r_isa(val_t v, val_t t) {
  type_t* tp = totype(t);
  return type_of(v) == tp ? T : NIL;
}

inline val_t r_eqp(val_t x, val_t y) {
  return x == y  ? T : NIL;
}

val_t r_size(val_t v) {
  return tagv(vm_size(v));
}

val_t r_cmp(val_t x, val_t y) {
  return tagv(cmpv(x,y));
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
  return eval_expr(x,e);
}

val_t r_apply(val_t p, val_t args) {
  val_t mock_proc = cons(p,NIL);
  cdr_(mock_proc) = args;

  return eval_expr(mock_proc,NONE);
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
  val_t out;
  switch(typecode(v)) {
  case TYPECODE_SYM:
    return tagp(vm_str(name_(v)));
  case TYPECODE_INT:
    b = itoa(toint_(v),10);
    out = tagp(vm_str(b));
    free(b);
    return out;
  case TYPECODE_STR:
    return v;
  default:
    escapef(TYPE_ERR,stderr,"Invalid argument of type %s",typename(v));
  }
}


val_t r_int(val_t v) {
  switch(typecode(v)) {
  case TYPECODE_SYM:
    return tagv(atoi(name_(v)));
  case TYPECODE_INT:
    return v;
  case TYPECODE_STR:
    return tagv(atoi(tostr_(v)));
  default:
    escapef(TYPE_ERR,stderr,"Invalid argument of type %s",typename(v));
  }
}

void _new_builtin_function(const chr_t* fname, int_t argc, bool vargs, const void* f) {
  val_t obj = new_proc(argc,NIL,((val_t)f),CALLMODE_FUNC,BODYTYPE_CFNC);
  if (vargs) {
    vargs_(obj) = VARGS_TRUE;
  }
  intern_builtin(fname,obj);
  return;
}

void init_builtin_functions() {
  // the only builtin functions supplied are accessors
  // to essential interpreter variables
  // other C functions are written or supplied, but for simplicity
  // they're bootstrapped within Rascal by binding procedures to ccalls.
  // 
  static chr_t* builtin_fnames[] = {
    "*env*","*globals*", "*root*", "*read-table*",
  };

  static int_t builtin_argcos[] = {};

  static bool builtin_vargs[] = {};

  static void* builtin_callables[] = {};

  for (int_t i = 0; i < NUM_BUILTINS; i++) {
    _new_builtin_function(builtin_fnames[i],builtin_argcos[i],builtin_vargs[i],builtin_callables[i]);
  }
  return;
}

void init_forms() {  
  static chr_t* builtin_forms[] = {
    "setv", "quote", "let",
    "do", "if", "ccall",                // ccall example (ccall fputs stdout "a string.")
    "lambda", };                        // the first argument to ccall should evaluate to
                                        // an integer that's used to look up the specified
                                        // method. lambda is a generalization of 'fun' and
                                        // 'macro' that takes an explicit argument to indicate
                                        // how to call it and calls the proc constructor.
                                        // hopefully implementing ccall should allow me to
                                        // to minimize the amount of C library code I'm writing
                                        // just to get the language working
                                        // example usage of lambda:
                                        // (lambda :function (x y z) (+ x y z))
                                        // (setv 'macro (lambda :macro ...))
  
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

  // intern_builtin("*globals*",tagp(GLOBALS));
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
  NIL = 0ul; NONE = TYPECODE_NONE << 3; FPTR = LOWTAG_CONSOBJ;
  EXP = VAL = CONTINUE = NAME = ENV = UNEV = ARGL = PROC = NIL;
  WRX = WRY = WRZ = NIL;
  STACK = malloc(MAXSTACK * 8);
  SP = -1;
  GLOBALS = NULL;
  T = tagval(1, INT_LOWTAG);
  OK = tagval(2, INT_LOWTAG);
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
  static chr_t* builtin_typenames[] = { "nil-type", "cons", "none-type", "str",
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
  niltype->flags.cmpable = 1;
  niltype->flags.callable = 0;
  niltype->flags.free = 0;
  niltype->tp_cmp = NULL;
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
  constype->flags.cmpable = 0;
  constype->flags.free = 0;
  constype->tp_cmp = NULL;
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
  nonetype->flags.cmpable = 1;
  nonetype->tp_cmp = NULL;
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
  strtype->flags.cmpable = 1;
  strtype->tp_cmp = cmp_str;
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
  typetype->flags.cmpable = 1;
  typetype->flags.free = 0;
  typetype->tp_cmp = cmp_type;
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
  symtype->flags.cmpable = 1;
  symtype->flags.free = 0;
  symtype->tp_cmp = cmp_sym;
  symtype->tp_new = NIL;
  symtype->tp_sizeof = NULL;
  symtype->tp_prn = prn_sym;


  
  type_t* dicttype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_DICT] = dicttype;
  dicttype->head.meta = TYPECODE_DICT;
  dicttype->head.type = TYPECODE_TYPE;
  dicttype->flags.base_size = sizeof(dict_t);
  dicttype->flags.val_lowtag = LOWTAG_OBJPTR;
  dicttype->flags.atomic = 0;
  dicttype->flags.callable = 0;
  dicttype->flags.free = 0;
  dicttype->flags.cmpable = 0;
  dicttype->tp_cmp = NULL;
  dicttype->tp_new = NIL;
  dicttype->tp_sizeof = NULL;
  dicttype->tp_prn = prn_dict;


  type_t* proctype = (type_t*)malloc(sizeof(type_t));
  TYPES[TYPECODE_PROC] = proctype;
  proctype->head.meta = TYPECODE_PROC;
  proctype->head.type = TYPECODE_TYPE;
  proctype->flags.base_size = sizeof(proc_t);
  proctype->flags.val_lowtag = LOWTAG_OBJPTR;
  proctype->flags.atomic = 0;
  proctype->flags.callable = 0;
  proctype->flags.free = 0;
  proctype->flags.cmpable = 0;
  proctype->tp_cmp = NULL;
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
  porttype->flags.cmpable = 0;
  porttype->tp_cmp = NULL;
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
  inttype->flags.cmpable = 1;
  inttype->tp_cmp = cmp_int;
  inttype->tp_new = NIL;
  inttype->tp_sizeof = NULL;
  inttype->tp_prn = NULL;

  for (int_t i = 0; i < NUM_BUILTIN_TYPES; i++) {
    sym_t* tpname = intern_builtin(builtin_typenames[i],tagp(TYPES[i]));
    TYPES[i]->tp_name = tpname;
  }

  return;
}

void bootstrap_rascal() {
  // set the locale to UTF-8
  setlocale(LC_ALL,"en_US.UTF-8");
  fprintf(stdout, "Initialization begining.\n");
  init_log();
  fprintf(stdout, "Log initialized.\n");
  init_heap();
  fprintf(stdout,"Heap initialized.\n");
  init_types();
  fprintf(stdout,"Types initialized.\n");
  init_registers();
  fprintf(stdout,"Registers initialized.\n");
  init_builtin_types();
  fprintf(stdout,"Builtin types initialized.\n");
  init_builtin_functions();
  fprintf(stdout,"Builtin functions initialized.\n");
  init_special_variables();
  fprintf(stdout,"Special variables initialized.\n");
  init_forms();
  fprintf(stdout,"Special forms initialized.\n");

  // set the default environment tail
  ROOT_ENV = cons(tagp(GLOBALS),NIL);

  fprintf(stdout,"Initialization succeeded.\n");
}
