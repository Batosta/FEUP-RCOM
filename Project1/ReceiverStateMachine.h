#ifndef RECEIVERSTATEMACHINE
#define RECEIVERSTATEMACHINE

#include "ReceiverFrame.h"

#define START_STATE 0
#define FLAG_STATE 1
#define A_STATE 2
#define C_STATE 3
#define BCC_STATE 4
#define COLLECTING_STATE 6
#define FINAL_STATE 5
#define ERROR_STATE 7

#define FLAG 0x7E
#define AE 0x03
#define ESCAPE 0x7d

typedef struct {
  unsigned char parameter;
  int state;
  int escapeFlag;
  frame * frame;
  unsigned char bcc2;
} receiverstatemachine;

receiverstatemachine * newReceiverStateMachine(unsigned char param);

void deleteStateMachine(receiverstatemachine *r);

void setMachineState(receiverstatemachine *r, int newState);

int getState(receiverstatemachine *r);

void interpretChar(receiverstatemachine *r, unsigned char s);

unsigned char getSentBCC2(receiverstatemachine *r);

unsigned char getCalculatedBCC2(receiverstatemachine *r);

unsigned char * getDataFrame(receiverstatemachine *r);

int sizeOfFrame(receiverstatemachine *r);
#endif
