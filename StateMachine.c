#include "StateMachine.h"

//VER REPOSITIORIO DE PROG 2 NO GitHub

statemachine * newStateMachine(unsigned char Cparameter) {

  statemachine *t = (statemachine*) malloc(sizeof(statemachine));

  t->parameter = Cparameter;

  return t;
}

void setState(statemachine *a, int newState) {
  a->state = newState;
}

int getMachineState(statemachine *a) {
  return a->state;
}

void resetStateMachine(statemachine *a) {
  a->state = 0;
}

void interpretSignal(statemachine *a, unsigned char s) {

  switch(a->state) {
    case START_STATE:
      if(s == FLAG) {
        setState(a, FLAG_STATE);
      }
      break;
    case FLAG_STATE:
      if(s == AE) {
        setState(a, A_STATE);
      }
      else if(s == FLAG) {}
      else {
        resetStateMachine(a);
      }
      break;
    case A_STATE:
      if(s == a->parameter) {
        setState(a, C_STATE);
      }
      else if(s == FLAG) {
        setState(a, FLAG_STATE);
      }
      else {
        setState(a, START_STATE);
      }
      break;
    case C_STATE:
      if(s == (AE^(a->parameter))) {
        setState(a, BCC_STATE);
      }
      else{
        resetStateMachine(a);
      }
      break;
    case BCC_STATE:
      if(s == FLAG) {
        setState(a, FINAL_STATE);
      }
      else{
        resetStateMachine(a);
      }
      break;
    default:
      resetStateMachine(a);
      break;
  }
}
