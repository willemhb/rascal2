#ifndef describe_h
#define describe_h

#define MK_EQUALITY_PREDICATE(gl,nm)                                              \
  inline bool is##nm(val_t v) { return v == gl; }

# define MK_LTAG_PREDICATE(lt,nm)                                                 \
  inline bool is##nm(val_t v) { return (v & LTAG_MASK) == lt; }

#define MK_TYPE_PREDICATE(lt,tk,tnm)		                                  \
  inline bool is##tnm(val_t v)                                                    \
  {                                                                               \
    int t = v & LTAG_MASK;						          \
    if (v & PTR_MASK)								  \
       return  t == lt && tpkey(v) == tk;                                         \
    									          \
    return false;                                                                 \
  }

#define MK_SAFECAST_V(ctype,rtype,cnvt)                                           \
  ctype to##rtype(const chr_t* fl, int32_t ln, const chr_t* fn, val_t v)          \
  {                                                                               \
    if (!is##rtype(v))                                                            \
      {                                                                           \
        rsp_perror(fl,ln,fn,TYPE_ERR, #rtype, val_typename(v)) ;	          \
	exit(EXIT_FAILURE)                                     ;		  \
        return (ctype)0                                        ;                  \
      }                                                                           \
    return (ctype)cnvt(v)                                      ;		  \
  }



#define MK_SAFECAST_P(ctype,rtype,cnvt)                                              \
  ctype sf_to##rtype(const chr_t* fl, int32_t ln, const chr_t* fn, val_t* v)         \
  {                                                                                  \
    if (!is##rtype(*v))                                                              \
      {                                                                              \
        rsp_perror(fl,ln,fn,TYPE_ERR, #rtype, val_typename(*v)) ;                    \
	exit(EXIT_FAILURE)                                      ;		     \
        return (ctype)0                                         ;                    \
      }                                                                              \
    return (ctype)cnvt(_update_fptr(v))                         ;                    \
  }

#endif
