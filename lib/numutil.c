#include <math.h>
#include <stdlib.h>
#include "numutil.h"

/* floating point manipulation */

inline int fltob(float f) {
  fl_bits_t flb = { .f = f};

  return flb.i;
}


inline float btofl(int i) {
  fl_bits_t flb = { .i = i };

  return flb.f;
}


inline int flsign(float f) {
  fl_bits_t flb = { .f = f};

  return flb.b.sign;
}


inline int flexpt(float f) {
  fl_bits_t flb = { .f = f};

  return flb.b.expt;
}


inline int flmtis(float f) {
  fl_bits_t flb = { .f = f};

  return flb.b.mtis;
}

inline int flint(float f) {
  return (int)f;
}

inline float flfrac(float f) {
  float ipart;
  
  return modff(f,&ipart);

}


inline int flfrac_r(float f) {
  float ipart;
  float fpart = modff(f,&ipart);

  return flmtis(fpart);
}


float logx(float x, float b) {
  return log2(x) / log2(b);
}


int logceil(int n, int b) {
  float l = logx(n,b);

  if (flfrac(l) == 0 && n % b == 0) return (int)l;
  else return (int)l + 1;
}


int modceil(int n, int d) {
  div_t r = div(n, d);

  if (r.rem) return r.quot + 1;
  else return r.quot;
}


int _ncmp(double x, double y) {
  
}
