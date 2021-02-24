#ifndef rascal_h

/* begin rascal.h */
#define rascal_h
#include "common.h"
#include "rtypes.h"
#include "globals.h"

/*
   convenience macros 

   in general, macros affixed with '_' are unsafe; they don't perform necessary safety checks
   before performing the requested operation. Those safety checks may be superfluous, but in general
   the safe version is preferred.

 */

// compiler hint macros
#define unlikely(x) __builtin_expect((x), 0)
#define likely(x)   __builtin_expect((x), 1)
#define popcount(x) __builtin_popcount(x)

// type-generic min, max, and compare macros
#define min(x,y)                     \
  ({                                 \
    typeof(x) __x__ = x ;            \
    typeof(y) __y__ = y ;            \
    __x__ > __y__ ? __y__ : __x__ ;  \
  })

#define max(x,y)                     \
  ({                                 \
     typeof(x) __x__ = x ;            \
     typeof(y) __y__ = y ;            \
     __x__ < __y__ ? __y__ : __x__ ;  \
  })

#define compare(x,y)                                     \
  ({                                                     \
     typeof(x) __x__ = x;                                \
     typeof(y) __y__ = y;                                \
     (__x__ < __y__ ? -1 : 0) || (__x__ > y ? 1 : 0);    \
  })

/* bit manipulation macros for tagging, untagging, and testing */

#define TRY                                                                       \
        rsp_ectx_t ectx__; int32_t eno__, __tr;	                                  \
	rsp_savestate(&ectx__);                                                   \
	err_stack = &ectx__;		                                          \
	eno__ = setjmp(ectx__.buf);                                               \
	if (eno__ == 0) {						          \
	  for (__tr=1; __tr; __tr=0, (void)(err_stack=err_stack->prev))

#define CATCH(x)                                                                  \
          } else if (eno__ == x) {						  \
          for (__tr=1; __tr; __tr=0, (void)(err_stack=rsp_restorestate(&ectx__))

#define ETRY                                                                      \
          } else								  \
	  {                                                                       \
	err_stack = rsp_restorestate(&ectx__) ;				          \
	rsp_raise(eno__) ;                                                        \
	  }

/* end rascal.h */
#endif
