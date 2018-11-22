#include "ReceiverStateMachine.h"
#include <stdlib.h>
#include <stdio.h>


receiverstatemachine * newReceiverStateMachine(unsigned char param) {
  receiverstatemachine * r;

  r = (receiverstatemachine *)malloc(sizeof(receiverstatemachine));

  r->parameter = param;

  r->frame = newFrame();

  r->escapeFlag = 0;

  r->state = START_STATE;

  return r;
}

void deleteStateMachine(receiverstatemachine *r) {
  delete(r->frame);

  free(r);
}

void setMachineState(receiverstatemachine *r, int newState) {
  r->state = newState;
}

int getState(receiverstatemachine *r) {
  return r->state;
}

unsigned char getSentBCC2(receiverstatemachine *r) {
	return elementAt(r->frame, length(r->frame) - 1);
}

unsigned char getCalculatedBCC2(receiverstatemachine *r) {
  return r->bcc2;
}

unsigned char * getDataFrame(receiverstatemachine *r) {
  unsigned char * f;
  int i;

  f = (unsigned char *) malloc(length(r->frame));

  for(i = 0; i < length(r->frame) - 1; i++) { //exclude BCC2
    f[i] = elementAt(r->frame, i);
  }

  return f;
}

int sizeOfFrame(receiverstatemachine *r) {
  return length(r->frame);
}

void interpretChar(receiverstatemachine *r, unsigned char s) {

  switch(r->state){
    case START_STATE:
      if(s == FLAG) {
        setMachineState(r, FLAG_STATE);
      }
      break;
    case FLAG_STATE:
      if(s == AE) {
        setMachineState(r, A_STATE);
      }
      else if(s == FLAG) {}
      else {
        setMachineState(r, ERROR_STATE);
      }
      break;
    case A_STATE:
      if(s == r->parameter) {
        setMachineState(r, C_STATE);
      }
      else {
        setMachineState(r, ERROR_STATE);
      }
      break;
    case C_STATE:
      if(s == (AE^(r->parameter))) {
        setMachineState(r, COLLECTING_STATE);
      }
      else{
        setMachineState(r, ERROR_STATE);
      }
      break;
    case COLLECTING_STATE:
      if(s == ESCAPE) {
        r->escapeFlag = 1;
      }
      else if(s == 0x5e && r->escapeFlag == 1) {
        insert(r->frame, 0x7e, -1);
        r->escapeFlag = 0;
      }
      else if(s == 0x5d && r->escapeFlag == 1) {
        insert(r->frame, 0x7d, -1);
        r->escapeFlag = 0;
      }
      else if(r->escapeFlag == 1) {
        insert(r->frame, 0x7d, -1);
        insert(r->frame, s, -1);
        r->escapeFlag = 0;
      }
      else if(s == FLAG && length(r->frame) != 0){
        r->bcc2 = r->bcc2 ^elementAt(r->frame, length(r->frame)-2);
        setMachineState(r, FINAL_STATE);
        return;
      }
      else if(s == FLAG && length(r->frame) == 0) {
        setMachineState(r, ERROR_STATE);
      }
      else {
        insert(r->frame, s, -1);
      }
      if(length(r->frame) == 3) {
        r->bcc2 = elementAt(r->frame, 0);
      }
      else if(length(r->frame) > 3 && r->escapeFlag == 0) {
        r->bcc2 = r->bcc2 ^elementAt(r->frame, length(r->frame)-3);
      }
      return;
    default:
      setMachineState(r, ERROR_STATE);
      break;
  }
}
