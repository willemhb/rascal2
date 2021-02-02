#ifndef describe_h

#define describe_h
#include "rsp_core.h"
/*

  This module mainly contains obnoxious function-defining macros

 */


#define EQUALITY_PREDICATE(nm,value) inline bool is ##nm(val_t v) { return v == value ; }

#define TPKEY_PREDICATE_SINGLE(nm,tv)   inline bool is##nm(val_t v) { return tpkey(v) == tv; }


#define TPKEY_PREDICATE(nm,tv)                                                           \
  inline bool v_is##nm(val_t v)    { return tpkey(v) == tv; }                            \
  inline bool o_is##nm(obj_t* o)   { return o ? o->obj_tpkey == tv : false ; }           \
  inline bool g_is##nm(void* o)    { return o ? ((obj_t*)o)->obj_tpkey == tv : false ; } \
  inline bool t_is##nm(tpkey_t t)  { return t == tv ; }			                 \



#define TPKEY_RANGE_PREDICATE(nm,ts,te)					\
  inline bool v_is##nm(val_t v)                                         \
  {									\
    switch (v_tpkey(v))							\
      {									\
    case ts ... te:							\
    return true;							\
    default:								\
    return false;							\
      }}								\
  inline bool o_is##nm(obj_t* v)                                        \
  {									\
    switch (o_tpkey(v))							\
      {									\
    case ts ... te:							\
    return true;							\
    default:								\
    return false;							\
      }}								\
   inline bool g_is##nm(void* v)                                        \
  {									\
    switch (g_tpkey(v))							\
      {									\
    case ts ... te:							\
    return true;							\
    default:								\
    return false;							\
      }}                                                                \
   inline bool t_is##nm(tpkey_t t)                                      \
  {									\
    switch (t)							        \
      {									\
    case ts ... te:							\
    return true;							\
    default:								\
    return false;							\
      }}


/* safety functions */
#define SAFECAST_PTR(ctype,rtype,test)	                                       \
  ctype to ## rtype(const chr_t* fl, int32_t ln, const chr_t* fnc,val_t v)     \
  {                                                                            \
    if (!test(v))                                                              \
  {                                                                            \
    rsp_vperror(fl,ln,fnc,TYPE_ERR,rsp_efmt(TYPE_ERR),#rtype,type_name(v)) ;   \
    rsp_raise(TYPE_ERR) ;						       \
    return (ctype)0;                                                           \
  }                                                                            \
    return ptr(ctype,trace_fp(v));					       \
  }



#define SAFECAST_VAL(ctype,rtype,test,cnvt)                                    \
  ctype to ## rtype(const chr_t* fl, int32_t ln, const chr_t* fnc,val_t v)     \
  {                                                                            \
    if (!test(v))                                                              \
  {                                                                            \
    rsp_vperror(fl,ln,fnc,TYPE_ERR,rsp_efmt(TYPE_ERR),#rtype,type_name(v)) ;   \
    rsp_raise(TYPE_ERR) ;						       \
    return (ctype)0;                                                           \
  }                                                                            \
    return cnvt(v);							       \
  }


#endif
