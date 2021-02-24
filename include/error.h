#ifndef error_h
#define error_h
#include "rascal.h"

#define assert(cond, eno, ...)                                          \
  do                                                                    \
    {                                                                   \
      if (!(cond))							\
	{                                                               \
	  rsp_perror(__FILE__,__LINE__,__func__,eno, ## __VA_ARGS__) ;	\
	  exit(EXIT_FAILURE);						\
        }		                                                \
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

#endif
