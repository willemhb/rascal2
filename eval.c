#include "rascal.h"

/* stack interface */
inline void push(val_t v) {
  if (stack_overflow(1)) {
    escapef(OVERFLOW_ERR,stderr,"stack overflow");
  }

  SP++;
  STACK[SP] = v;
  return;
}

inline val_t pop() {
  if (SP == -1) {
    escapef(OVERFLOW_ERR,stderr,"pop on empty stack");
  }
  
  val_t out = STACK[SP];
  SP--;
  return out;
}

inline val_t peek(int_t offset) {
  if (SP - offset < 0) {
    escapef(OVERFLOW_ERR,stderr,"peek on empty stack");
  }

  return STACK[SP - offset];
}

inline void fetch() {
  INSTR = IP[0];
  IP++;
  return;
}

inline void decode() {

}

void execute();                      // execute the current instruction
