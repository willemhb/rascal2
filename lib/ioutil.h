#ifndef ioutil_h

/* begin ioutil.h */
#define ioutil_h
#include "strutil.h"
#include <stdio.h>


/*

  this module provides useful functions for file and socket io,
  as well as UTF-compatible IO utilities. My eventual hope is
  to provide a full replacement for the builtin FILE type that's
  fully generic, supports unicode, and smoothly incorporates
  memory mapped IO.

 */

// unicode compatible IO
int fgetuc(FILE*);
int fputuc(FILE*,utf32_t);
int fungetuc(FILE*,utf32_t);
int fpeekuc(FILE*);
int fputus(FILE*,const char*);
int fgetus(FILE*,char*);

// numeric IO
int fputi(FILE*,int);
int fputf(FILE*,float);

/* end ioutil.h */
#endif
