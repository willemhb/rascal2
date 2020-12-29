#ifndef rascal_h

/* begin common.h */
#define rascal_h

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <wctype.h>
#include <locale.h>
#include "lib/strutil.h"
#include "lib/numutil.h"
#include "lib/ioutil.h"

/* 
   values, object heads, and tags

   All rascal values are represented by pointer-sized unsized integers that can be interpreted
   as half-word sized C data (stored in the high half of the word), C pointers (special cases),
   or pointers to objects in the lisp heap (most object data). All data accessible through
   rascal must be accessed through tagged values -- either wrapped direct data, a tagged C
   pointer, or a tagged pointer to an object head. object fields that don't fall into
   one of these categories cannot be returned directly as rascal values. The word-sized, tagged,
   rascal-legal value will be canonically referred to as the "value representation". The
   corresponding heap object (if any) will be referred to as the "object representation".

   Type information is stored in type_t structs, which record information about the type,
   including how to interpret the value representation, the memory layout of the object repres-
   entation, the constructor (if any), &c.

   The type of the value is encoded using a CL-inspired tagging system involving a 3- or 4-bit common
   tag (ltag), a 4-bit direct tag for direct data (dtag), and a 4-bit object tag for common
   heap objects. The full tag is 8-bits wide and is composed of (least-to-most significant bits)
   the narrow component and either the data component (if any) or the object component (if any).
   The resulting wide tag (wtag) identifies either a builtin type with a known location or
   the location of a pointer to the type object.

   The lisp heap is a contiguous array of word-sized, double-word aligned cells (the minimum
   allocation size is two machine words to support forwarding pointers). All objects in the
   lisp heap are cons cells, unicode strings, or objects. Cons cells and strings have special
   narrow tags to identify them, but general objects require a 4-bit object tag to distinguish
   them. These 4 bits are stored in the 4 least significant bits of the object head.

   There are two general categories of object head; object heads with explicit pointers to
   the type, and object heads that store flags and metadata. In general, The former are used
   for derived types while the latter are used for builtin types (whose locations are known).

   As such, all legal Rascal values must begin with a word-sized block that reserves its 4
   least-significant bits for the object tag.

   Below are enums defining the complete set of tags, as well as macros defining the various 
   types of object heads, as well as type definitions for casting to different kinds of
   objects. The object tag can always be accessed by casting to an obj_t object.

 */

typedef enum {

  /* 3-bit lowtags */
  
  LTAG_DIRECT           =0b0000ul,  // a special lowtag for direct data (read next 4 bits)
  LTAG_STR              =0b0010ul,  // a utf-8 C string
  LTAG_BSTR             =0b0100ul,  // a binary byte string (the first 8 bytes contains the length)
  LTAG_CFUNCTION        =0b0110ul,  // a C function (for now assume this is a builtin C function)

  /* 4-bit low tags */
  
  LTAG_CONS             =0b0001ul,  // a non-list cons cell
  LTAG_LIST             =0b0011ul,  // a list-structured chain of cons cells
  LTAG_SYM              =0b0101ul,  // a pointer to a symbol

  /* lowtags for header tagged objects */
  
  LTAG_TABLE            =0b0111ul,  // a pointer to a table
  LTAG_ARRAY            =0b1001ul,  // a pointer to an array
  LTAG_PROCEDURE        =0b1011ul,  // a pointer to a procedure
  LTAG_OBJ              =0b1101ul,  // other object type
  LTAG_NONE             =0b1111ul,  // special lowtag for none
} ltag_t;


typedef enum {
  DTAG_NIL              =0b0000ul,   // special value NIL
  DTAG_CHAR             =0b0001ul,   // a utf-32 character
  DTAG_INT              =0b0010ul,   // integers
  DTAG_BOOL             =0b0011ul,   // booleans
  DTAG_FLOAT            =0b0100ul,   // floats
  DTAG_IMAG             =0b0101ul,   // imaginary number (not implemented)
  DTAG_FDESC            =0b0110ul,   // a file descriptor
  DTAG_C32              =0b0111ul,   // arbitrary 32-bit C data (more type information held in the next byte; not yet implemented)
  DTAG_OMIT             =0b1111ul,   // sentinel for tagging functions indicating not to use the dtag
} dtag_t;

typedef enum {
  /* table otags */
  OTAG_TABLE_SET        =0b0000ul,   // a set
  OTAG_TABLE_DICT       =0b0001ul,   // a dictionary
  OTAG_TABLE_SYM        =0b0010ul,   // symbol table
  OTAG_TABLE_READ       =0b0011ul,   // a specialized symbol table

  /* procedure otags */
  
  OTAG_PROCEDURE_COMPILED =0b0001ul,   // a bytecode object
  OTAG_PROCEDURE_LAMBDA   =0b0010ul,   // an uncompiled procedure
  OTAG_PROCEDURE_GENERIC  =0b0011ul,   // a generic function

  /* 
     array otags 

     once arrays are fully implemented, the array otags will be composable, using
     the first four tags below as bit flags. Some of these are mutually exclusive,
     and the invalid tags are repurposed to denote VM frames. All VM frames are
     either dynamic untyped vectors or static untyped vectors, so only the first flag
     is meaningful.

     The only array types currently implemented are dynamic vectors of tagged rascal
     values and fixed vectors of tagged rascal values.

   */

  OTAG_ARRAY_TYPED          =0b0001ul,  // the array has an explicit type pointer for all elems
  OTAG_ARRAY_UNWRAPPED      =0b0010ul,  // the array contains unwrapped data
  OTAG_ARRAY_MULTIDIM       =0b0100ul,  // the array is multidimensional
  OTAG_ARRAY_DYNAMIC        =0b1000ul,  // the array is dynamic

  OTAG_ARRAY_DVEC           =0b1000ul,  // a simple dynamic array
  OTAG_ARRAY_FVEC           =0b0000ul,  // a simple fixed vector
  OTAG_ARRAY_FRAME_ENV      =0b1011ul,  // an environment frame
  OTAG_ARRAY_FRAME_CONT     =0b0011ul,  // a continuation frame

  /* object otags */

  OTAG_OBJ_TYPE          =0b0000ul,  // a type object
  OTAG_OBJ_RECORD        =0b0001ul,  // a record object
  OTAG_OBJ_SUBTYPE       =0b0010ul,  // a type-prefixed subtype of a builtin type
  OTAG_OBJ_CDATA         =0b0011ul,  // type tagged C data
  OTAG_OBJ_CPTR          =0b0100ul,  // indirected C pointer with type information

  /* sentinel otag */
  
} otag_t;


/* special values NIL and NONE */

const uintptr_t NIL = 0x0ul;
const uintptr_t NONE = UINT_MAX;

/* 
   special sentinels - UNBOUND signals an interned symbol where a binding has not been provided, while FPTR signals a value that has been moved during garbage collection
   (the new location can be found by checking the next word-sized block). Both are equivalent to tagging the NULL pointer with a specific tag.

 */

const uintptr_t UNBOUND = LTAG_SYM;                     // this is the same as tagging the null pointer with LTAG_SYM
const uintptr_t FPTR = LTAG_OBJ;                        // this is the same as tagging the null pointer with LTAG_OBJ

/* booleans */

const uintptr_t R_TRUE = (UINT32_MAX + 1) | 0b0011000ul;
const uintptr_t R_FALSE = 0b001100ul;

/* common integer values */

const uintptr_t R_ZERO = 0b00100000ul; 
const uintptr_t R_ONE = (UINT32_MAX + 1) | 0b00100000ul;

/* important file descriptors */
const uintptr_t R_STDIN_FNO = 0b01100000ul;
const uintptr_t R_STDOUT_FNO = (UINT32_MAX + 1) | 0b01100000ul;
const uintptr_t R_STDERR_FNO = (UINT32_MAX + 2) | 0b01100000ul;

// object head macros and generic object types

/* cons - a classic! */
typedef struct {
  uintptr_t car;
  uintptr_t cdr;
} cons_t;


#define OBJECT_HEAD          \
  uintptr_t head

typedef struct {
  OBJECT_HEAD;
} obj_t;



typedef struct {
  uintptr_t allocated : 60;
  uintptr_t otag      :  4;
  uintptr_t elcnt;
  uintptr_t elements;        // pointer to the elements, in case the vector needs to grow
} dvec_t;

typedef struct {
  uintptr_t allocated : 60;
  uintptr_t otag      :  4;
  uintptr_t elements[1];     // elements stored directly; cannot add more elements than allocatd
} fvec_t;


#define PROC_OBJ_HEAD                  \
  uintptr_t pargs           : 28;      \
  uintptr_t kwargs          : 28;      \
  uintptr_t method          :  1;      \
  uintptr_t vargs           :  1;      \
  uintptr_t varkw           :  1;      \
  uintptr_t macro           :  1;      \
  uintptr_t otag            :  4


/*

  the common fields are:

  formals         : a dict mapping variable names to offsets in the local environment

 */

#define PROC_COMMON_FIELDS             \
  uintptr_t formals;                   \
  uintptr_t closure_formals;           \
  uintptr_t closure_binding


/* signature for valid builtin C functions - the first argument is a stack of rascal values and the second is the number of arguments the function was called with */
typedef uintptr_t (*rsp_builtin_t)(uintptr_t*,int);

typedef struct {
  PROC_OBJ_HEAD;
  PROC_COMMON_FIELDS;
} proc_t;

typedef struct {
  PROC_OBJ_HEAD;
  PROC_COMMON_FIELDS;
  uintptr_t body;      // a pointer to the list of expressions comprising the function body
} lambda_t;

typedef struct {
  PROC_OBJ_HEAD;
  PROC_COMMON_FIELDS;
  uintptr_t code;     // a pointer to the bytecode sequence
  uintptr_t vals[1];  // an array of values used in the procedure body
} bytecode_t;

/* 
   atoms, symbols, and tables

   atomic values are values guaranteed to represent different data if their value representation
   is different (and vice versa). All direct data are atomic. Additionally, rascal continues the lisp tradition of
   using interned strings called symbols.

   while rascal currently only implements symbols, it's possible -- and I'd like -- to implement more general atoms
   that will work for arbitrary data. This would work as follows:

   * A value is 'atomic' if it is direct data, a type, or has an atomic representation.

   * A value has an atomic representation if it's a string or a collection of elements with an atomic representation.

   * Creating an atom involves creating and interning an atomic representation of a value, or looking up an interned representation (an atomic list '(1 2 3) is only created once).

   * An atom is like a symbol and has the same head as the symbol (including a hash). The atomic representation is stored directly.

   * For inductively defined types, atoms take care of the induction (don't worry about how yet).

     * Alternatively, inductively defined types could be converted to canonical vectorized forms, then the forms are hashed along with the types.

   * Atomic representations can be interacted with normally, but their fields become immutable. So you can still get the second element from an atomic list, for example, but you can't
     change its value.

   * Pointers to atomic representations become valid table keys.

   tables are balanced binary trees widely used to implement symbol tables, reader tables, environments, modules, user dicts, and user sets. 
   Symbols and tables are intimately related; interned symbols are embedded directly into the table itself.

   It would be entirely possible to make symbol tables a first-class dict type by tweaking the object layout but for now (for simplicity)
   they are somewhat distinct from other kinds of tables, and used only for interning symbols.

   the basic table variants are symbol table, reader table, environment, module, dict, and set. symbol tables store interned symbols.

   Reader tables and modules require their keys to be symbols and have different restrictions on their bindings.

   Environment bindings must be integers, except for a few special constant keys:

     *doc*       : the docstring for the function or macro that created this environment
     *module* : the module in which the environment was created

   Module bindings may be any rascal value, but modules have special constant keys as well:

     *doc*       : the docstring for the module
     *path*      : the full path to the file in which this module was defined
     *name*      : the name of this module (if none is provided, this is the filename minus the file extension)
     *qual-name* : the fully qualified name of the module
     *module* : a pointer to the module in effect when this module was created

   None of the above are implemented yet, but certainly will be.

   the fields in the TABLE_OBJ_HEAD have the following meanings:

   ord_key        : the interpretation of this field depends on the value of is_root.

   is_root        : a boolean flag, true if this table has no parent. If true, ord_key is the number of keys in the table. otherwise, ord_key is the insertion order of the current node (
                    this allows both tables and sets to remember the insertion order of the keys without additional overhead).

   is_constant    : indicates whether or not the binding can be changed.

   token_type     : 0 if the table is not a reader table, otherwise a flag indicating how to interpret the token

   balance_factor : used to implement AVL tree balancing algorithm

   otag           : reserved for the otag.
 */

#define TABLE_OBJ_HEAD               \
  uintptr_t ord_key         : 52;    \
  uintptr_t is_root         :  1;    \
  uintptr_t constant        :  1;    \
  uintptr_t token_type	    :  3;    \
  uintptr_t balance_factor  :  3;    \
  uintptr_t otag            :  4

enum {
  TOKTYPE_NONE,                // ignore the toktype field; this is used by all table types except reader tables
  TOKTYPE_CHR,                 // the bytes in the token field are a null-terminated bytestring representing a single utf-8 character
  TOKTYPE_STR,                 // the bytes in the token field are a null-terminated bytestring representing multiple utf-8 characters, all of which must match
  TOKTYPE_CHRSET,              // the bytes in the token field are a null-terminated bytestring representing multiple utf-8 characters, one of which must match
  TOKTYPE_DLM_START,           // the bytes in the token field represent the start of a delimited sequence (the binding is a pointer to the end delimiter, whose binding is the actual reader function)
  TOKTYPE_DLM_END,             // the bytes in the token field represent the end of a delimited sequence
  TOKTYPE_PATTERN,             // the bytes in the token field represent a regular expression pattern that must match the token (not yet implemented).
};

typedef struct table_t table_t;

#define TABLE_COMMON_FIELDS           \
  uintptr_t left;                     \
  uintptr_t right;                    \
  uintptr_t hashkey

// because only 60 bits are available to store the hash, a slight modification is made to the FNV-1A algorithm
// the keyword field indicates that the symbol always evaluates to itself 
// the reserved field indicates that the symbol cannot be rebound or shadowed; it should only be used for special forms and
// a handful of special keywords
#define ATOM_OBJ_HEAD                \
  uintptr_t hash            : 60;    \
  uintptr_t interned        :  1;    \
  uintptr_t gensym          :  1;    \
  uintptr_t reserved        :  1;    \
  uintptr_t keyword         :  1


typedef struct {
  ATOM_OBJ_HEAD;
  char name[1];       // the name hangs off the end
} sym_t;

struct table_t {
  TABLE_OBJ_HEAD;
  TABLE_COMMON_FIELDS;
};

typedef struct {
  TABLE_OBJ_HEAD;
  TABLE_COMMON_FIELDS;
} set_t;

typedef struct {
  TABLE_OBJ_HEAD;
  TABLE_COMMON_FIELDS;
  uintptr_t binding;
} dict_t;

typedef struct {
  TABLE_OBJ_HEAD;
  TABLE_COMMON_FIELDS;
  uintptr_t binding;
  ATOM_OBJ_HEAD;               // interned symbols are embedded directly into the symbol table; pointers to interned symbols are always really pointers to an offset part of the symbol table
  char name[1];               
} symtab_t;


/* 

   the following api functions should be used to access or manipulate table keys and bindings, since different tables have slightly different layouts
   using this api internally improves the interoperability of symbol tables, reader tables, and dictionaries.

   get_key, lookup_key, put_key, and set_key all check the otag of the table.

   get_key works on any table type. it accesses the hashkey field (for symbol tables and reader tables, hashkey is implicit; may make this more sophisticated later)

   search_key is a lower-level function meant to be used for lookup, interning, and assignment alike. search_key traverses the table looking for a match to the supplied key,
   and returns a pointer to the last node that was searched before the search terminated (if the value at that address is NIL, the search failed). The return value can be used to
   add new values to a table, fetch value from the table, or modify existing data.
     * If the table is a dictionary or table, the provided key must be atomic.
     * If the table is a module, the provided key must be a symbol.
     * If the table is a reader table, the key must be a character or string.
     * If the table is a symbol, the key must be a string or an uninterned symbol.

   lookup_key traverses the table looking for a match to the supplied key, and returns the binding for that key (or none if the search failed). lookup_key has
   the same restrictions as search_key.

   put_key adds a key to a table if it doesn't exist and returns a sentinel indicating whether the key was added. lookup key has the same restrictions as search_key.

   set_key changes the binding associated with a symbol. It throws an error if it fails or the operation is not applicable. set_key has the same restrictions as
   search_key.
     * set_key is not applicable to sets or symbol tables.
     * set_key fails if the constant flag is true and the binding is not UNBOUND.
     * set_key also fails if the key is not found in the table.

 */

#define compute_hashkey(st) ((((uintptr_t)(st)) + 32) | WTAG_SYM)  // the address of the head plus the offset of the embedded symbol (in bytes, with a symbol tag)

uintptr_t   get_key(table_t*);
uintptr_t*  search_key(table_t*,uintptr_t);
uintptr_t   lookup_key(table_t*,uintptr_t);
uintptr_t   put_key(table_t*,uintptr_t);
uintptr_t   set_key(table_t*,uintptr_t,uintptr_t);

uintptr_t intern_string(symtab_t*,const char*);

uintptr_t lookup_name(uintptr_t,uintptr_t);


/* C api and C descriptions of rascal values */

/* type objects */

typedef struct {
  uintptr_t padding      : 30;                 // ensure alignment
  uintptr_t tp_atomic    :  1;                 // whether values of this type can safely be compared by pointer
  uintptr_t tp_builtin   :  1;                 // whether or not this is a builtin type
  uintptr_t tp_fixed     :  1;                 // whether or not values of this type may be larger than their base size
  uintptr_t tp_storage   :  2;                 // possible values are 0 (direct), 1 (heap allocated), 2 (specially allocated), or 3 (allocation not managed by rascal)
  uintptr_t c_numval     :  4;                 // how to interpret the value representation of this type
  uintptr_t c_qual       :  4;                 // any additional C qualifiers that apply to the value representation
  uintptr_t tp_dtag      :  4;                 // appropriate dtag for values of this type (should be DTAG_OMIT if this is not a direct data type)
  uintptr_t tp_otag      :  4;                 // appropriate otag for values of this type (should be OTAG_OMIT if objects do not use an OTAG)
  uintptr_t tp_ltag      :  4;                 // appropriate ltag for values of this type
  uintptr_t tp_parent;                         // pointer to the parent type (if any)
  uintptr_t tp_fields;                         // pointer to a symbol table that maps field names to offsets (if the type has any rascal-accessible fields)
  uintptr_t tp_new;                            // produce a new value of this type based on the supplied arguments
  uintptr_t tp_print;                          // print values of this type
  uintptr_t tp_name;                           // pointer to a symbol holding the name
  
  /* the following fields are used internally but not accessible */

  uintptr_t tp_init;                           // initialize fields
  uintptr_t tp_move;                           // used by the garbage collector
} type_t;

/* variable declarations for builtin types */

extern type_t* ANY_TYPE_OBJ, NIL_TYPE_OBJ, NONE_TYPE_OBJ, INT_TYPE_OBJ,
  FLOAT_TYPE_OBJ, CHAR_TYPE_OBJ, IOSTREAM_TYPE_OBJ, STR_TYPE_OBJ, TYPE_TYPE_OBJ,
  CONS_TYPE_OBJ, LIST_TYPE_OBJ, SYM_TYPE_OBJ;

/* tokenizer */

typedef enum _r_tok_t {
  TOK_LPAR,
  TOK_RPAR,
  TOK_DOT,
  TOK_QUOT,
  TOK_NUM,
  TOK_STR,
  TOK_SYM,
  TOK_UCP,
  TOK_EOF,
  TOK_NONE,
  TOK_STXERR,
} r_tok_t;

#define TOKBUFF_SIZE 1024
char TOKBUFF[TOKBUFF_SIZE];
char STXERR[TOKBUFF_SIZE];
int TOKPTR;
r_tok_t TOKTYPE;

/* reader suport functions */

void builtin_print(uintptr_t,FILE*); // fallback print function
uintptr_t builtin_load(FILE*);
char* toktype_name(r_tok_t);
r_tok_t get_token(FILE*);
uintptr_t read_expr(FILE*);
uintptr_t read_cons(FILE*);

/* memory, gc, & allocator */

#define AS_BYTES 8
#define AS_WORDS 1
#define MAXTYPES UINT_MAX >> 3 
#define MAXBASE USHRT_MAX
#define INIT_HEAPSIZE 8 * 4096  // in bytes
#define INIT_NUMTYPES 256       // number of records
#define RAM_LOAD_FACTOR 0.8
#define HEAP_ALIGNSIZE 8        // the alignment of the heap

#define CHECK_RESIZE() (FREE - HEAP) > (RAM_LOAD_FACTOR * HEAPSIZE) ? true : false

int align_heap(unsigned char**,unsigned char*);
void gc();
void gc_trace(uintptr_t*);
unsigned char* gc_copy(unsigned char*,int,int);
unsigned char* vm_allocate(int);

bool heap_limit(unsigned);
bool type_limit(unsigned);
bool type_overflow(unsigned);
bool stack_overflow(unsigned);
bool type_overflow(unsigned);

/* special registers holding important memory locations and parameters */

unsigned char *HEAP, *EXTRA, *FREE, *FROMSPACE, *TOSPACE;
uintptr_t HEAPSIZE, ALLOCATIONS;
bool GROWHEAP;

// opcodes, vm, & vm registers
/* 
   register architecture based on 
   https://www.cs.utexas.edu/ftp/garbage/cs345/schintro-v14/schintro_142.html#SEC275

   VALUE - the result of the last expression
   ENVT  - the environment the code is executing in

*/

uintptr_t VALUE, ENVT, CONT, TEMPLATE, PC;

// the stacks used by the virtual machine
uintptr_t *EVAL, *SP;

// working registers
uintptr_t WRX[16];

typedef enum {
  /* these opcodes are common to evaluated and compiled code */
  OP_HALT,
} rsp_opcode_t;

typedef enum {
  EV_HALT,
  EV_LITERAL,
  EV_VARIABLE,
  /* special forms */
  EV_QUOTE,
  EV_BACKQUOTE,
  EV_UNQUOTE,
  EV_SPLICE,
  EV_SETN,
  EV_DEF,
  EV_IF,
  EV_DO,
  EV_MACRO,            // capture the closing environment and create a new macro
  EV_FUN,              // capture the closing environment and create a new function
  EV_LET,

  /* application and argument evaluation */

  EV_APPLY,            // starting point for the evaluation process
  EV_APPLY_OP_DONE,    // set up the frame for the call without updating the frame pointer
  EV_MACRO_ARGLOOP,    // load the arguments into VARS without evaluating them; place the procedure body in EXP, set the return label to EV_MACRO_RETURN, update the FP, and dispatch
  EV_BUILTIN_ARGLOOP,  // 
  EV_BUILTIN_ACCUMARG, // 
  EV_FUNCTION_ARGLOOP, // 
  EV_ACCUMARG,         // accumulate an argument
  EV_ACCUMARG_LAST,    // accumulate the final argument
  EV_RETURN,           
  EV_MACRO_RETURN,     
} rsp_evlabel_t;

// a separate set of labels to jump to when ERRORCODE is nonzero
enum {
  EVERR_TYPE=1,
  EVERR_VALUE,
  EVERR_ARITY,
  EVERR_UNBOUND,
  EVERR_OVERFLOW,
  EVERR_IO,
  EVERR_NULLPTR,
  EVERR_SYNTAX,
  EVERR_INDEX,
  EVERR_APPLICATION,
};

// save the symbols bound to builtin forms for faster evaluation
extern uintptr_t BUILTIN_FORMS[];

#define isnil(v)         ((v) == NIL)
#define isnone(v)        ((v) == NONE)
#define isok(v)          ((v) == OK)
#define istrue(v)        ((v) == T)
#define isfptr(v)        ((v) == FPTR)
#define cbooltorbool(v)  ((v) ? T : NIL)
#define rbooltocbool(v)  ((v) == NIL || (v) == NONE ? false : true)

/* errors and logging */

#define STDLOG_PATH "log/rascal.log"
#define STDHIST_PATH "log/rascal.history.log"

/* 
   Safety should be used to jump directly to the top level in the event of critical errors 
   like memory overflow, stack overflow, potential segfault, etc. Common errors should return
 
   Common errors should be signaled using a few conventional sentinel values based on the
   expected return type.

   NULL  - functions that return pointers should return NULL to signal an error.
   -2    - functions that return integers should return -2 to signal an error (
           -1 is used for interning symbols, so using -1 would lead to a lot of unnecessary
           error checking).
   NONE  - functions that return uintptr_t should return the special rascal value NONE to signal
           an error.

   Additionally, since these sentinels might be normal return values, the functions that return
   them should set the global ERRORCODE variable. An error occured if and only if a return value
   is one of the above, AND the value of ERRORCODE is non-zero.

   This module provides a few macros and functions to simplify raising and handling errors.

   the checks(v) macro checks that both error conditions are true.

   the fail(e,s,fmt,args...) macro sets the error status, writes the supplied message to stdlog,
   and returns the given sentinel.

   ifail, rfail, and cfail fail with the sentinels -2, NONE, and NULL respectively.

   the clear macro resets the value of ERRORCCODE to 0.

   if checks returns true, then it MUST be followed by a call to fail or clear, or future
   error codes will be unreliable.
*/

FILE* stdlog;
jmp_buf SAFETY;

typedef enum r_errc_t {
  OK_ERR,
  TYPE_ERR,
  VALUE_ERR,
  ARITY_ERR,
  UNBOUND_ERR,
  OVERFLOW_ERR,
  IO_ERR,
  NULLPTR_ERR,
  SYNTAX_ERR,
  INDEX_ERR,
  APPLICATION_ERR,
} r_errc_t;

#define escapef_p(e,file,cfile,line,cfnc,fmt,args...)			             \
  do                                                                                 \
    {                                                                                \
     fprintf(file,"%s: %d: %s: %s ERROR: ",cfile,line,cfnc,get_errname(e));          \
     fprintf(file,fmt,##args);                                                       \
     fprintf(file, ".\n");                                                           \
     longjmp(SAFETY,e);							             \
    } while (0)

#define eprintf(e, file, fmt, args...)				                     \
  do                                                                                 \
  {									             \
  fprintf(file, "%s: %d: %s: %s ERROR: ",__FILE__,__LINE__,__func__,get_errname(e)); \
  fprintf(file,fmt,##args);                                                          \
  fprintf(file,".\n");                                                               \
  } while (0)								             \

//  print common error messages to the log file.
#define elogf(e,fmt,args...) eprintf(e,stdlog,fmt,##args)
// shorthand for jumping to the safety point. The safety point prints an informative message
// and initiates program exit.

#define escape(e) longjmp(SAFETY, e)

// since they're called together a lot, escapef composes them 
#define escapef(e, file, fmt, args...)				                     \
  do                                                                                 \
  {									             \
  fprintf(file, "%s: %d: %s: %s ERROR: ",__FILE__,__LINE__,__func__,get_errname(e)); \
  fprintf(file,fmt,##args);                                                          \
  fprintf(file,".\n");                                                               \
  longjmp(SAFETY,e);							             \
  } while (0)								             \

// error reporting functions
char* get_errname(int_t);
void init_log();                 // initialize the global variable stdlog. Stdlog is 
char* read_log(char*, int_t);  // read the log file into a string buffer.
void dump_log(FILE*);            // dump the contents of the log file into another file.
int_t finalize_log();            // copy the contents of the log to the history


/* initialization */
/* bootstrapping builtin functions */
void _new_builtin_function(const char*, int_t, bool, const void*);  
uintptr_t  _new_form_name(char*);                 
uintptr_t  _new_self_evaluating(char*);

/* memory initialization */
void init_heap();
void init_types();
void init_registers();

/* initialization of core language */
void init_builtin_types();
void init_forms();
void init_special_variables();
void init_builtin_functions();

/* toplevel bootstrapping function */
void bootstrap_rascal();

/* end rascal.h */
#endif
