#include "fapi.h"

static i32_t simple_check_argco(i32_t argco, rval_t arglist) {
  i32_t counter = 0;
  rval_t currarg = arglist;
  rcons_t* argcons;

  while (counter < argco) {
    if (isnil(currarg)) {
      return ERR_ARGCO_DEFICIT;
    } else if (!iscons(currarg)) {
      return ERR_ARGLIST_TYPE;
    } else {
      argcons = _tocons(currarg);
      counter++;
      currarg = argcons->cdr;
    }
  }

  return isnil(currarg) ? ERR_OK : ERR_ARGCO_EXCESS;
}


i32_t check_argco(i32_t argco, rval_t arglist, rval_t* argarray) {
  /*
    Check the supplied arguments against the supplied arglist. If argarray is provided,
    place arguments in argarray as they're counted (NULL can be passed to count the arguments
    only).

    Return 0 if the number of arguments supplied matches the spec.

    Return 1 if too many arguments were supplied.

    Return 2 if too few arguments were supplied.

    Return 3 if the supplied 'arglist' wasn't a list.
   */

  if (!argarray) {
    return simple_check_argco(argco, arglist);
  }
  
  i32_t counter = 0;
  rval_t currarg = arglist;
  rcons_t* argcons;

  while (counter < argco) {
    if (isnil(currarg)) {
      return ERR_ARGCO_DEFICIT;
    } else if (!iscons(currarg)) {
      return ERR_ARGLIST_TYPE;
    } else {
      argcons = _tocons(currarg);
      argarray[counter] = argcons->car;
      counter++;
      currarg = argcons->cdr;
    }
  }

  return isnil(currarg) ? ERR_OK : ERR_ARGCO_EXCESS;

}

i32_t check_arg(u16_t cspec, rval_t arg) {
  u16_t argtype = vcode(arg);
  u16_t ctype = specdtype(cspec);
  u16_t cstorage = specstorage(cspec);

  switch (argtype) {
  case CODE_INT:
    if (cstorage) return ERR_ARGTYPE;
    return ctype == CS_I32 || ctype == CS_U32 ? ERR_OK : ERR_ARGTYPE;

  case CODE_FLOAT:
    if (cstorage) return ERR_ARGTYPE;
    return ctype == CS_F32 ? ERR_OK : ERR_ARGTYPE;

  case CODE_UCP:
    if (cstorage) return ERR_ARGTYPE;
    return ctype == CS_CHR32 ? ERR_OK : ERR_ARGTYPE;

  case CODE_BYTES:
  case CODE_STR:
    if (cstorage == CS_PTR) return ctype == CS_CHR ? ERR_OK : ERR_ARGTYPE;
    return ERR_ARGTYPE;

  case CODE_PORT:
    return cstorage == CS_TXTIO ? ERR_OK : ERR_ARGTYPE;

  default:
    return ERR_ARGTYPE;
  }
}


i32_t check_args(cspec_t cspec, rval_t argl, rval_t* argarray) {
  i32_t result;
  i32_t argco = cspec.argco;
  u16_t *argtypes = cspec.atypes;
  i32_t use_temp_argarr = argarray == NULL;

  if (use_temp_argarr) {
    argarray = malloc(sizeof(rval_t) * argco);
  }

  result = check_argco(argco, argl, argarray);

  if (!result) {
    for (int i = 0; i < argco; i++) {
    result = check_arg(argtypes[i], argarray[i]);
    if (result) break;
    }
  }

 if (use_temp_argarr) {
   free(argarray);
 }

  return result;
}
