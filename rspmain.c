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
  R_STREAMS[0] = ((val_t)stdin) | LTAG_CFILE;
  R_STREAMS[1] = ((val_t)stdout) | LTAG_CFILE;
  R_STREAMS[2] = ((val_t)stderr) | LTAG_CFILE;

  bootstrap_rascal();
  fprintf(stdout, "Welcome to rascal2 v 0.0.1.0\n");

  // load the lisp core
  port_t* r_system = vm_open("system.rsp", "r");
  vm_load(r_system);
  repl();
  return 0;
}
