#include "capi.h"

/* 
   inlined functional bindings for C arithmetic 

 */

// the following macros provide boilerplate definitions for C api functions of up to 4 arguments
// future plans are to implement a more robust C api, but this should make it simpler to write basic C library bindings

#define DECLARE_API_0(fname,rtp,wrp)	      \
  val_t capi_ ## fname ## _(val_t* a, int c)  \
  {					      \
     if (c != 0) rsp_raise(ARITY_ERR);	      \
     rtp _result = fname();		      \
     return wrp(_result);		      \
  }


#define DECLARE_API_1(fname,xtp,cstx,rtp,wrp)	        \
  val_t capi_ ## fname ## _(val_t* a, int c)            \
  {					                \
    val_t _x; xtp _xc;				        \
     if (c != 1) rsp_raise(ARITY_ERR) ;	                \
     _x = pop(a);				        \
     _xc = cstx(_x);					\
     rtp _result = fname(_xc) ;		                \
     return wrp(_result)      ;		                \
  }



#define DECLARE_API_2(fname,xtp,ytp,cstx,csty,rtp,wrp)			\
  val_t capi_ ## fname ## _(val_t* a, int c)                            \
  {					                                \
    val_t _x, _y; xtp _xc; ytp _yc;				        \
     if (c != 2) rsp_raise(ARITY_ERR) ;	                                \
     _x = pop(a); _y = pop(a);						\
     _xc = cst1(_x); _yc = cst2(_y);					\
     rtp _result = fname(_yc,_xc) ;		                        \
     return wrp(_result)    ;		                                \
  }


#define DECLARE_API_3(fname,xtp,ytp,ztp,cstx,csty,cstz,rtp,wrp)		         \
  val_t capi_ ## fname ## _(val_t* a, int c)                                     \
  {					                                         \
    val_t _x, _y, _z; xtp _xc; ytp _yc; ztp _zc;			         \
     if (c != 3) rsp_raise(ARITY_ERR) ;	                                         \
     _x = pop(a); _y = pop(a);	_z = pop(a);				         \
     _xc = cstx(_x); _yc = csty(_y); _zc = cstz(_z);			         \
     rtp _result = fname(_zc,_yc,_xc) ;				                 \
     return wrp(_result)    ;		                                         \
  }

#define DECLARE_API_4(fname,xtp,ytp,ztp,wtp,cstx,csty,cstz,cstw,rtp,wrp)                               \
  val_t capi_ ## fname ## _(val_t* a, int c)                                                           \
  {					                                                               \
    val_t _x, _y, _z, _w; xtp _xc; ytp _yc; ztp _zc; wtp _wc;		                               \
     if (c != 4) rsp_raise(ARITY_ERR) ;	                                                               \
     _x = pop(a); _y = pop(a);	_z = pop(a); _w = pop(a);		                               \
     _xc = cstx(_x); _yc = cstx(_y); _zc = cstz(_z); _wc = cstw(_w);	                               \
     rtp _result = fname(_wc,_zc,_yc,_xc) ;				                               \
     return wrap(_result)    ;		                                                               \
  }

