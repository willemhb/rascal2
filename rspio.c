#include "rascal.h"

void prn_list(val_t ls, FILE* f) {
  fputwc('(',f);
  list_t* lsx = tolist_(ls);
  while (lsx) {
    vm_print(first_(lsx),f);

    if (rest_(lsx)) fputwc(' ',f);

    lsx = rest_(lsx);
  }

  fputwc(')',f);
  return;
}

void prn_cons(val_t c, FILE* f)
{
  fputwc('(',f);
  vm_print(car_(c),f);
  fputs(" . ",f);
  vm_print(cdr_(c),f);
  fputwc(')',f);
  return;
}

void prn_str(val_t s, FILE* f)
{
  fputwc('"',f);
  fputs(tostr_(s),f);
  fputwc('"',f);
}


void prn_sym(val_t s, FILE* f) {
  fputs(symname_(s),f);
  return;
}


static void tbl_prn_traverse(node_t* n, FILE* f)
{
  if (!n) return;

  vm_print(hashkey_(n),f);
  if (fieldcnt_(n))
    {
      fputs(" => ",f);
      vm_print(bindings_(n)[0],f);
    }

  if (left_(n))
    {
      fputwc(' ',f);
      tbl_prn_traverse(left_(n),f);
    }

  if (n->right)
    {
      fputwc(' ',f);
      tbl_prn_traverse(left_(n),f);
    }

  return;
}

void prn_table(val_t t, FILE* f)
{
  fputwc('{',f);
  tbl_prn_traverse(records_(t),f);
  fputwc('}',f);
}

void prn_bool(val_t b, FILE* f)
{
  if (b == R_TRUE) fputwc('t',f);
  else fputwc('f',f);

  return;
}

void prn_char(val_t c, FILE* f)
{
  fputwc('\\',f);
  fputwc(tochar_(c),f);
  return;
}

void prn_fvec(val_t v, FILE* f)
{
  fputs("#f[",f);
  val_t* el = tofvec_(v)->elements;
  int elcnt = tofvec_(v)->allocated;

  for (int i = 0; i < elcnt; i++) {
    vm_print(el[i],f);
    if (elcnt - i > 1) fputwc(' ', f);
  }

  fputwc(']',f);
  return;
}


void prn_dvec(val_t v, FILE* f)
{
  fputs("#d[",f);
  val_t* el = dvec_elements_(v);
  int elcnt = dvec_allocated_(v);

  for (int i = 0; i < elcnt; i++)
    {
      vm_print(el[i],f);
      if (elcnt - i > 1) fputwc(' ', f);
    }

  fputwc(']',f);
  return;
}

void prn_int(val_t i, FILE* f)
{
  fprintf(f,"%d",toint_(i));
  return;
}
