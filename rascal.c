#include "rascal.h"


void repl() {
  while (true) {
    int_t error = setjmp(SAFETY);

    if (error) {
      fprintf(stderr,"Error caused escape to the top level.\n");
      return;
    }
    
    r_prn(R_PROMPT, R_STDOUT);
    val_t expr = r_read(R_STDIN);
    val_t result = eval_expr(expr,ROOT_ENV);
    fprintf(stdout, ">>> ");
    r_prn(result,R_STDOUT);
    vm_puts(toport_(R_STDOUT),"\n");
  }
}


int main(int argc, char** argv) {
  bootstrap_rascal();
  fprintf(stdout, "Welcome to rascal2 v 0.0.1.0\n");
  repl();
  return 0;
}
