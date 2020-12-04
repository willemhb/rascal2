#include "rascal.h"


void repl() {
  while (true) {
    int_t error = setjmp(SAFETY);

    if (error) {
      fprintf(stderr,"Error caused escape to the top level.");
      continue;
    }
    
    prn(R_PROMPT, R_STDOUT);
    val_t expr = read(R_STDIN);
    eval_expr(-1,expr,NONE);
    prn(expr,R_STDOUT);
  }
}


int main(int argc, char** argv) {
  bootstrap_rascal();
  fprintf(stdout, "Welcome to rascal2 v 0.0.1.0\n");
  repl();
  return 0;
}
