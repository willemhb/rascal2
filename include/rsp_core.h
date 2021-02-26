#ifndef rascal_h

/* begin rascal.h */
#define rascal_h
#include "common.h"
#include "rtypes.h"
#include "globals.h"
#include "strlib.h"
#include "hashing.h"

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
     typeof(x) __x__ = x ;           \
     typeof(y) __y__ = y ;           \
     __x__ < __y__ ? __y__ : __x__ ; \
  })

#define compare(x,y)                                   \
  ({                                                   \
     typeof(x) __x__ = x                             ; \
     typeof(y) __y__ = y                             ; \
     (__x__ < __y__ ? -1 : 0) || (__x__ > y ? 1 : 0) ; \
  })


/* error handling */

#define assert(cond, eno, ...)                                         \
  do                                                                   \
    {                                                                  \
      if (!(cond))						       \
	{                                                              \
	  rsp_perror(__FILE__,__LINE__,__func__,eno, ## __VA_ARGS__) ; \
	  exit(EXIT_FAILURE)                                         ; \
        }		                                               \
    } while (0)


#define argcount(fargc,argc)  assert((argc) == (fargc), ARITY_ERR, fargc, argc)
#define vargcount(fargc,argc) assert((argc) >= (fargc), ARITY_ERR, fargc, argc)


void rsp_perror(const chr_t* fl, int32_t ln, const chr_t* cfnc, int32_t eno, ...)
{
  const chr_t* ename = ERROR_NAMES[eno];
  const chr_t* efmt  = ERROR_FMTS[eno];
  fprintf(stderr,ERRINFO_FMT,fl,ln,cfnc,ename);
  va_list vargs;
  va_start(vargs,eno);
  vfprintf(stderr,efmt,vargs);
  va_end(vargs);
  return;
}

#define TRY                                                               \
        rsp_ectx_t ectx__; int32_t eno__, __tr                          ; \
	rsp_savestate(&ectx__)                                          ; \
	err_stack = &ectx__                                             ; \
	eno__ = setjmp(ectx__.buf)                                      ; \
	if (eno__ == 0) {						  \
	  for (__tr=1; __tr; __tr=0, (void)(err_stack=err_stack->prev))

#define CATCH(x)                                                                 \
          } else if (eno__ == x) {						 \
          for (__tr=1; __tr; __tr=0, (void)(err_stack=rsp_restorestate(&ectx__))

#define ETRY                                                                     \
          } else								 \
	  {                                                                      \
	err_stack = rsp_restorestate(&ectx__) ;				         \
	rsp_raise(eno__)                      ;                                  \
	  }

/* end rascal.h */
#endif
