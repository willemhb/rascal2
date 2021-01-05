#include "capi.h"


#define DECLARE_BUILTIN(fname,argc) DECLARE_BUILTIN_ ## argc(fname)

#define DECLARE_BUILTIN_0_(fname)	      \
  val_t bltn_ ## fname ## _(val_t* a, int c)  \
  {					      \
     if (c != 0) rsp_raise(ARITY_ERR) ;	      \
     return fname() ;			      \
  }

#define DECLARE_BUILTIN_1_(fname)	      \
  val_t bltn_ ## fname ## _(val_t* a, int c)  \
  {					      \
     val_t _x;				      \
     if (c != 1) rsp_raise(ARITY_ERR) ;	      \
     _x = pop(a) ;			      \
     return fname(_x) ;			      \
  }

#define DECLARE_BUILTIN_2_(fname)	      \
  val_t bltn_ ## fname ## _(val_t* a, int c)  \
  {					      \
     val_t _x, _y;			      \
     if (c != 2) rsp_raise(ARITY_ERR) ;	      \
     _x = pop(a) ; _y = pop(a) ;	      \
     return fname(_y,_x);                     \
  }

#define DECLARE_BUILTIN_3_(fname)	      \
  val_t bltn_ ## fname ## _(val_t* a, int c)  \
  {					      \
     val_t _x, _y, _z;			      \
     if (c != 3) rsp_raise(ARITY_ERR);	      \
     _x = pop(a); _y = pop(a); _z = pop(a);   \
     return fname(_z,_y,_x);		      \
}

#define DECLARE_BUILTIN_4_(fname)	      \
  val_t bltn_ ## fname ## _(val_t* a, int c)  \
  {					      \
     val_t _x, _y, _z, _w;		      \
     if (c != 4) rsp_raise(ARITY_ERR);	      \
     _x = pop(a); _y = pop(a); _z = pop(a);   \
     _w = pop(a);			      \
     return fname(_w,_z,_y,_x);		      \
}
