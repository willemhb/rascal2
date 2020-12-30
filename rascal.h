#ifndef rascal_h

/* begin common.h */
#define rascal_h

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <uchar.h>
#include <wctype.h>
#include <locale.h>

typedef uintptr_t uptr_t;
typedef uintptr_t val_t;   // a rascal value; all functions that expect rascal values or pointers to rascal values should use this type
typedef uint64_t hash64_t;


// value tags for determining type - the current system is forward compatible with planned extensions 
typedef enum {
  /* 3-bit lowtags - least significant bit is 0 */
  LTAG_NIL              =0b00000000ul,  // a special lowtag for NIL
  LTAG_STR              =0b00000010ul,  // a utf-8 C string
  LTAG_CFILE            =0b00000100ul,  // a C file object
  LTAG_CFUNCTION        =0b00000110ul,  // a C function (for now assume this is a builtin C function)
  /* 4-bit low tags - least significant bit is 1 */
  LTAG_CONS             =0b00000001ul,  // a non-list cons cell
  LTAG_LIST             =0b00000011ul,  // a list-structured chain of cons cells
  LTAG_SYM              =0b00000101ul,  // a pointer to a symbol
  /* the lowtags below will correspond to objects with an otag once the object model is more robust - at present the otag is only used by the VM for special flags */
  LTAG_TABLE            =0b00000111ul,  // a pointer to a table
  LTAG_VECTOR           =0b00001001ul,  // a pointer to a vector
  LTAG_PROCEDURE        =0b00001011ul,  // a pointer to a procedure object
  LTAG_RECORD           =0b00001101ul,  // other object type (implicitly type in the current version)
  /* 0xf is a special sentinel for direct data; */
  LTAG_INT              =0b00001111ul,  // direct data (read the next 4-bits)
  LTAG_CHAR             =0b00011111ul,
  LTAG_BOOL             =0b00101111ul,
  LTAG_NONE             =0b11111111ul,
} ltag_t;

#define LTAG_DIRECT 0xf

// the otags below will be extended in the future, but for now it is only used to indicate special cases of types and store information about symbols
typedef enum {
  // table otags
  OTAG_SYMTAB    =0b0001ul,
  // vector otags
  OTAG_ENVFRAME  =0b0001ul,
  OTAG_CONTFRAME =0b0010ul,
  // symbol otags - really just flags
  OTAG_INTERNED  =0b0001ul,
  OTAG_KEYWORD   =0b0010ul,
  OTAG_RESERVED  =0b0100ul,
  OTAG_GENSYM    =0b1000ul,
} otag_t;

 /* types for setting and fetching object head flags */

typedef struct {
  val_t allocated : 60;
  val_t otag      :  4;
} array_flags_t;

typedef struct {
  val_t padding  :  1;
  val_t pargs    : 28;
  val_t kwargs   : 28;
  val_t macro    :  1;
  val_t vargs    :  1;
  val_t varkw    :  1;
  val_t otag     :  4;
} procedure_flags_t;

typedef struct {
    val_t ordkey   : 55;
    val_t is_root  :  1;
    val_t constant :  1;
    val_t balance  :  3;
    val_t otag     :  4;
} table_flags_t;

typedef struct {
  /* flags encoding size information */
  val_t val_size_base     : 47;      // the minimum size for objects of this type
  /* flags encoding the size and C representation for values of this type */
  val_t val_cnum          :  4;      // the C numeric type of the value representation
  val_t val_cptr          :  2;      // the C pointer type of the value representation
  val_t val_rheap         :  1;      // whether values live in the rascal heap
  val_t val_ltag          :  4;
  /* misc other data about values */
  val_t tp_atomic         :  1;     // legal for use in a plain dict
  val_t tp_callable       :  1;     // legal as first element in an application
  val_t otag              :  4;
} type_flags_t;

typedef struct {
  val_t padding : 60;
  val_t otag    :  4;
} object_flags_t;

typedef union {
  val_t raw;
  array_flags_t arr_flags;
  procedure_flags_t proc_flags;
  table_flags_t table_flags;
  type_flags_t type_flags;
  object_flags_t obj_flags;
} object_head_flags_t;

// union type to represent C version of all builtin direct types
typedef union {
  int integer;
  float float32;
  char32_t unicode;
  bool boolean;
  unsigned bits;
} rsp_direct_ctypes_t;

/* the current object head layouts imply various sizing limits, which are defined here */

const uint64_t MAX_ARRAY_ELEMENTS = (1ul << 60) - 1;
const uint64_t MAX_TABLE_ELEMENTS = (1ul << 52) - 1;
const uint64_t MAX_OBJECT_BASESIZE = (1ul << 46) - 1;
const uint64_t MAX_PROCEDURE_ARGCO = (1ul << 28) - 1;

/*  structs to represent builtin object types */

#define OBJECT_HEAD  \
  val_t head

typedef struct {
  val_t car;
  val_t cdr;
} cons_t;

// the most general kind of object, useful for retrieving the lowtag
typedef struct {
  OBJECT_HEAD;
} obj_t;

// if a vector is dynamic, the first element is the number of slots used and the second
// is a pointer to the elements. If a vector is fixed, then the elements are stored
// immediately. The second element of a byte vector is a pointer to the first byte in the array
typedef struct {
  OBJECT_HEAD;
  val_t elements[1];
} vec_t;

// type signature for builtin functions
typedef val_t (*rsp_builtin_t)(val_t*,int);

// uncompiled code
typedef struct {
  OBJECT_HEAD;
  val_t formals;     // a list of formal names
  val_t closure_env; // a captured environment frame
  val_t body;        // the sequence of expressions in the body
} lambda_t;

typedef struct {
  OBJECT_HEAD;
  val_t formals;      // a map of environment names
  val_t closure_env;  // a captured environment frame
  val_t code;         // a pointer to the bytecode sequence
  val_t vals[1];      // an array of values used in the procedure body
} bytecode_t;

typedef struct {
  val_t record;       // this symbol's entry in a symbol table
  char name[1];       // the name hangs off the end
} sym_t;

typedef struct {
  OBJECT_HEAD;
  val_t left;
  val_t right;
  val_t hashkey;
  val_t binding;
} table_t;

typedef struct type_t {
  OBJECT_HEAD;

  /* layout information */

  val_t tp_name;                    // a pointer to the symbol this type is bound to
  val_t tp_fields;                  // a dict mapping rascal-accessible fields to offsets

  /* function pointers */

  val_t tp_new;                            // the rascal-callable function dispatched when the type is called - it determines whether space needs to be allocated,
                                           // passes appropriate arguments to the memory management functions, and returns a correctly tagged object
  val_t tp_move;                           // used by the garbage collector to relocate values of this type
  val_t tp_print;                          // used to print values
  val_t tp_print_traverse;                 // used to print recursive structures
} type_t;

/* C api and C descriptions of rascal values - The current goal is to implement a C api that covers rascal values which can be represented as numeric types or strings. */

typedef enum {
  C_UCHR8   =0b0000,
  C_CHR8    =0b0010,
  C_SCHR8   =0b0011,
  C_UINT8   =0b0100,
  C_INT8    =0b0101,
  C_UINT16  =0b0110,
  C_INT16   =0b0111,
  C_UINT32  =0b1000,
  C_INT32   =0b1001,
  C_FLOAT32 =0b1010,
  C_UINT64  =0b1100,
  C_INT64   =0b1101,
  C_FLOAT64 =0b1110,
} c_num_t;

typedef enum {
  C_PTR_NONE =0b00,   // value is the type indicated by c_num_t
  C_PTR_VOID =0b01,   // value is a pointer of unknown type
  C_PTR_TO   =0b10,   // value is a pointer to the indicated type
  C_FPTR     =0b11,   // special case - value is a C file pointer
} c_ptr_t;

/* vm labels and opcodes */

typedef enum {
  OP_HALT,
  OP_LOAD_CONSTANT,
} opcode_t;

typedef enum {
  EV_HALT,
  EV_LITERAL,
  EV_VARIABLE,
} ev_label_t;

/* builtin reader tokens */

typedef enum {
  TOK_LPAR,
  TOK_RPAR,
  TOK_DOT,
  TOK_QUOT,
  TOK_NUM,
  TOK_STR,
  TOK_SYM,
  TOK_SLASH,
  TOK_EOF,
  TOK_NONE,
  TOK_STXERR,
} r_tok_t;

/* constant definitions and variable declarations */
// special constants - common values, singletons, and standard streams
const val_t NIL = 0x0ul;
const val_t NONE = UINT_MAX;
const val_t EMPTY = LTAG_LIST;  // empty list
const val_t UNBOUND = LTAG_SYM; // an unbound symbol
const val_t FPTR = LTAG_RECORD;    // a forwarding pointer
const val_t R_TRUE = (UINT32_MAX + 1) | LTAG_BOOL;
const val_t R_FALSE = LTAG_BOOL;
const val_t R_ZERO = LTAG_INT; 
const val_t R_ONE = (UINT32_MAX + 1) | LTAG_INT;
const val_t R_TWO = (UINT32_MAX + 2) | LTAG_INT;

// macros since the actual locations of stdin, stdout, and stderr change
#define R_STDIN  (((val_t)stdin)  | LTAG_CFILE)
#define R_STDOUT (((val_t)stdout) | LTAG_CFILE)
#define R_STDERR (((val_t)stderr) | LTAG_CFILE)

// memory and GC 
extern unsigned char *HEAP, *EXTRA, *FREE;
extern val_t HEAPSIZE, ALLOCATIONS;
extern bool GROWHEAP;

/* 
   vm & registers

   VALUE           - the result of the last expression
   ENVT            - the environment the code is executing in
   CONT            - holds the caller's continuation frame
   TEMPLATE        - holds the body of the executing expression
   PC              - holds the program counter
   
   EVAL (EP)       - holds a list of evaluated sub-expressions needed by the current expression
   STACK (SP)      - a more general stack for holding
   READSTACK (RP)  - a separate stack for the reader

   WRX             - an array of general registers

*/

extern val_t VALUE, ENVT, CONT, TEMPLATE, PC;

// the stacks used by the virtual machine
extern val_t *EVAL, *EP, *STACK, *SP, *READSTACK, *RP;

// working registers
extern val_t WRX[16];

// 

// builtin types live in known locations - nil, none, bool, and type currently have phantom type objects, or are handled specially (though they're still considered to have types)
extern type_t*
// direct data
INT_TYPE_OBJ,
FLOAT_TYPE_OBJ,
// types with narrow ltags
STR_TYPE_OBJ,
CFILE_TYPE_OBJ,
BUILTIN_TYPE_OBJ,
// types with wide ltags
CONS_TYPE_OBJ,
LIST_TYPE_OBJ,
SYM_TYPE_OBJ,
// vector type objects
VEC_TYPE_OBJ,                // includes fvecs, dvecs, and frames
// table type objects
DICT_TYPE_OBJ,
// procedure type objects
PROC_TYPE_OBJ;

// reader
extern const int TOKBUFF_SIZE;
extern char TOKBUFF[];
extern int TOKPTR;
extern r_tok_t TOKTYPE;

// builtin form names
const char* FORM_NAMES[] = {"setv", "def", "quote", "if", "fn", "macro", "do", "let", "c-call"};

const char* SPECIAL_VARIABLES[] = {"*env*", "*stdin*", "*stdout*", "*stderr*",
                                   "nil", "none", "empty", "t", "f"};

const char* BUILTIN_FUNCTIONS[] = {"bltn-eval", "bltn-apply", "bltn-read", "nil?"};


extern val_t F_SETV, F_DEF, F_QUOTE, F_IF, F_FN, F_MACRO, F_DO, F_LET, F_CCALL;
/* function declarations & apis */

// manipulating tags and accessing object metadata
#define tag_width(v)         (((v) & 0x1) ? 4 : 3)
#define bits(d)              (((rsp_direct_ctypes_t)(d)).bits)
#define pad_b(d)             (bits(d) << 32)
#define unpad_b(d)           ((d) >> 32)

ltag_t get_ltag(val_t);                  // get the ltag component
val_t  get_addr(val_t);                  // mask out the correct bits to extract the address
ltag_t get_value_ltag(type_t*);          // get the wtag used by values from the type object
type_t* get_obj_type(val_t);             // get the type object associated with the value
type_flags_t get_obj_type_flags(val_t);  // get the type flags for a value
int get_obj_size(val_t);                 // get the size of an object
char* get_obj_type_name(val_t);          // get the object type name

// fast comparisons and tests
#define isnil(v)     ((v) == R_NIL)
#define isnone(v)    ((v) == R_NONE)
#define istrue(v)    ((v) == R_TRUE)
#define isfalse(v)   ((v) == R_FALSE)
#define isfptr(v)    ((v) == R_FPTR)
#define isunbound(v) ((v) == R_UNBOUND)
#define isempty(v)   ((v) == R_EMPTY)

// tag-based tests for builtin types
#define hasltag(v,w)   (get_ltag(v) == (w))
#define isstr(v)       hasltag(v,LTAG_STR)
#define iscfile(v)     hasltag(v,LTAG_CFILE)
#define isbuiltin(v)   hasltag(v,LTAG_BUILTIN)
#define iscons(v)      hasltag(v,LTAG_CONS)
#define islist(v)      hasltag(v,LTAG_LIST)
#define issym(v)       hasltag(v,LTAG_SYM)
#define isint(v)       hasltag(v,LTAG_INT)
#define isbool(v)      hasltag(v,LTAG_BOOL)
#define ischar(v)      hasltag(v,LTAG_CHAR)
#define isvec(v)       hasltag(v,LTAG_VEC)
#define istable(v)     hasltag(v,LTAG_TABLE)
#define istype(v)      hasltag(v,LTAG_RECORD)

// specialized type tests - these are functions since they must first determine if the value has an otag at all
bool issymtab(val_t);
bool isenv(val_t);
bool iscont(val_t);

// general object convenience functions and predicates
bool cbool(val_t);                // convert rascal value to C boolean
val_t rbool(int);                 // convert C boolean to rascal boolean
val_t update_fptr(val_t*);        // check if the object has been moved
bool isatomic(val_t);             // check if the value is atomic
bool iscallable(val_t);           // check if the value is legal in calling position

// memory management and bounds checking
unsigned char* allocate_obj(type_t*,int);                                    // generic allocator
unsigned char* initialize_obj(type_t*,val_t*,int,object_head_flags_t);       // generic initializer
unsigned char* copy_obj(type_t*,int);                                        // generic copier
void gc();
void gc_trace(val_t*);
bool heap_limit(unsigned long);
bool stack_overflow(unsigned long);

// special constructors for VM-internal objects and symbols
val_t new_closure(val_t,val_t);
val_t new_continuation(val_t);
sym_t* new_symbol(const char, int); // second argument includes symbol flags

// object apis
val_t bltn_getf(val_t*,int);   // expects 2 arguments - generic
val_t bltn_setf(val_t*,int);   // expects 3 arguments - generic
val_t bltn_len(val_t*,int);    // expects 1 argument  - generic
val_t bltn_assocv(val_t*,int); // expects 2 arguments - generic
val_t bltn_associ(val_t*,int); // expects 2 arguments - generic

// table, symtable, and environment API - search, lookup, extend, and assign accept lists, dicts, or environment frames as arguments
int      ord(val_t,val_t);                // overloaded comparison that works with any atomic values
int      cmp(val_t,val_t);                // comparison function that works with strings, direct data, and symbols, so long as the values are comparble types
table_t* table_search(table_t*,val_t);
val_t    table_lookup(table_t*,val_t);
val_t    table_extend(table_t*,val_t);
val_t    table_assign(table_t*,val_t);
val_t*   env_search(val_t,val_t);
val_t    env_lookup(val_t,val_t);
val_t    env_extend(val_t,val_t);
val_t    env_assign(val_t,val_t,val_t);
val_t    intern(table_t*,const char*);

// vm functions, builtins, and API
val_t bltn_eval(val_t*,int);        // expects 1 or 2 arguments
val_t bltn_compile(val_t*,int);     // expects 2 or 3 arguments
val_t push(val_t,val_t*);
val_t pop(val_t*);
val_t peek(val_t*);
val_t swap(val_t*);
val_t dup(val_t*);
val_t pushn(val_t*,val_t*,int);

// builtin read/print functions
val_t  bltn_print(val_t*,int);  // expects 2 arguments
val_t  bltn_load(val_t*,int);   // expects 1 argument
val_t  bltn_read(val_t*,int);   // expects 2 arguments
val_t  bltn_getc(val_t*,int);   // expects 2 arguments
val_t  bltn_gets(val_t*,int);   // expects 2 arguments
val_t  bltn_puts(val_t*,int);   // expects 2 arguments
val_t  bltn_open(val_t*,int);   // expects 2 arguments
val_t  bltn_close(val_t*,int);  // expects 1 argument

// reader support functions
r_tok_t get_token(FILE*);
val_t   vm_read_expr(FILE*);
val_t   vm_read_list(FILE*);

// C api
int unwrap(val_t,void*,c_num_t,c_ptr_t);

// arithmetic and comparison builtins
val_t bltn_cmp(val_t*,int);  // expects 2 arguments
val_t bltn_ord(val_t*,int);  // expects 2 arguments
val_t bltn_add(val_t*,int);  // expects 2 arguments
val_t bltn_sub(val_t*,int);  // expects 2 arguments

// utility functions
hash64_t hash_str(const char*);
int strsz(const char*);
int u8strlen(const char*);

// error handling


/* initialization */
// memory
void init_heap();
void init_types();
void init_registers();

// builtins
void init_builtin_types();
void init_special_forms();
void init_special_variables();
void init_builtin_functions();

/* toplevel bootstrapping function */
void bootstrap_rascal();

/* end rascal.h */
#endif
