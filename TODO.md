# Memory
* Convert safecast to new system that checks for/updates forwarding pointers
* Finish testing garbage collection with new safecaster

# Text and IO
* Fix the 
* Implement the UCP builtin type
* Integrate the UCP type with the string type and implement assr, assv, setr, and rplcv for
  strings
* Implement the full port interface
* Implement a simple read table interface

# Types
* Implement subtyping and new type creation

# Advanced features
* Implement methods and multiple dispatch


  future plan for usage of ltags:

  In the future, the ltag labels will be something like:

  LTAG_LIST,
  LTAG_CONS,
  LTAG_STR,
  LTAG_FUNCTION,  // a function or metaobject (including types)
  LTAG_SYM,
  LTAG_FVEC,      // a fixed vector that can be simple or bitmapped
  LTAG_NODE,      // highly optimized 
  LTAG_TUPLE,     // building block for inductively defined types
  LTAG_ARRAY,     // an array that can be a combination of multidimensional, typed, or unboxed
  LTAG_OBJECT,    // an object with an explicit type tag
  LTAG_BIGNUM,    // large numeric data
  LTAG_DIRECT,

  // direct tags

  LTAG_INT,         // 56-bit integer, most common integer type
  LTAG_INT32,
  LTAG_UINT32,
  LTAG_IMAG32,
  LTAG_FLOAT32,
  LTAG_C32,
  LTAG_CHAR,
  LTAG_BOOL,
  LTAG_IMAG,
  LTAG_NONE,

  In this scheme, LTAG_NODE, LTAG_TUPLE, and LTAG_FVEC are intended as low-level types that can be used as rich building blocks
  for users to create their own inductively defined types. NODE is an AVL node that can be used as an underlying data structure for
  other types, FVEC can be a simple, efficient, length-prefixed vector or a bitmapped vector, and TUPLE is a more generic, flexible
  fixed-size record for users to use.

  proposed classes in an advanced implementation (if classes are used):

  seq   - seq, fst, rst (besides fst and rst, provides access to map, filter, reduce, cat, conj, &c)
  eql   - eql (type can use =, /=)
  eqv   - promote (type can be compared using eql or ord with all the types it implements a promote method for)
  num   - ?? (provides numeric operators)
  ord   - cmp (type can use =, /=, <, <=, >, =>)
  atom  - hash, ord (makes type a licit dict or set key)
  
  

*/
