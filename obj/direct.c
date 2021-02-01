#include "../include/rsp_core.h"


val_t mk_bool(int32_t i)
{
  if (i)
    return R_TRUE;

  else
    return R_FALSE;
}

val_t cnvt_bool(val_t v)
{
  if (isbool(v))
    return v;

  else if (isnil(v))
    return R_FALSE;

  else
    return R_TRUE;
}

val_t mk_char(int32_t i)
{
  if (i < UCP_LIMIT_1 || i > UCP_LIMIT_4)
    return R_EOF;

  else
    return tag_v(i,CHAR);
}

inline val_t mk_int(int32_t i) { return tag_v(i,INT);  }


val_t stoi(chr_t* s)
{
  val_t out = R_EOF;

  if (iswspace(s[0]))
    return out;

  chr_t* end;

  int64_t cnvt = strtol(s,&end,0);

  if (*end != '\0')
    return out;

  else
    {
      cnvt = min(cnvt,INT32_MAX);
      out = (val_t)cnvt << 32 | INT;
      return out;
    }
}

val_t stof(chr_t* s)
{
  val_t out = R_NAN;
  if (iswspace(s[0]))
    return out;

  chr_t* end;

  flt32_t fl = strtof(s,&end);
  if (*end != '\0')
    return R_NAN;

  else
    return tag_v(fl,FLOAT);
}

inline val_t mk_float(flt32_t f) { return tag_v(f,FLOAT_TP); }

val_t cnvt_float(val_t v)
{
  switch (tpkey(v))
    {
    case FLOAT:
      return v;

    case INT:
      return val_itof(v);

    case STRING: case ATOM:
      return stof(strval(v));

    default:
      rsp_raise(TYPE_ERR);
      return R_NAN;
    }
}

hash32_t hash_small(val_t v)
{
  uchr_t buf[4]; uint32_t sbits = unpad(v);
  memcpy(buf,&sbits,4);
  return hash_bytes(buf,4);
}

void prn_float(val_t fl, iostrm_t* f)
{
  flt32_t fv = fval(fl);
  fprintf(f,"%.2f",fv);
  return;
}

void prn_char(val_t ch, iostrm_t* f)
{
  ichr32_t chv = cval(ch);
  fputwc(chv,f);
  return;
}

void prn_int(val_t i, iostrm_t* f)
{
  int32_t iv = ival(i);
  fprintf(f,"%d",iv);
  return;
}

void prn_bool(val_t b, iostrm_t* f)
{
  if (b == R_TRUE)
    fputwc('t',f);

  else
    fputwc('f',f);

  return;
}

int32_t ord_ival(val_t xi, val_t yi)
{
  int32_t xii = ival(xi), yii = ival(yi);
  return compare(xii,yii);
}

int32_t ord_fval(val_t xf, val_t yf)
{
  flt32_t xff = fval(xf), yff = fval(yf);
  return compare(xff,yff);
}

val_t val_ftoi(val_t f)
{
  flt32_t fl = ecall(tofloat,f);
  return mk_int((int)fl);
}

val_t val_itof(val_t i)
{
  int32_t iv = ecall(toint,i);
  return mk_float((flt32_t)iv);
}
