#include "StateMachine.h"

//VER REPOSITIORIO DE PROG 2 NO GitHub

statemachine * newStateMachine() {

  statemachine *t = (statemachine*) malloc(sizeof(statemachine));

  return t;
}

void resetStateMachine(statemachine *a) {
  a->state = 0;
}

void setState(statemachine *a, int newState) {
  a->state = newState;
}

int getMachineState(statemachine *a) {
  return a->state;
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
      if(s == SET) {
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
      if(s == (AE^SET)) {
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
