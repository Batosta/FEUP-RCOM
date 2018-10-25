#ifndef STATEMACHINE
#define STATEMACHINE

#include <stdio.h>
#include <stdlib.h>

#define FLAG 0x7E
#define AE 0x03
#define AR 0x01
#define SET 0x03
#define DISC 0x0B
#define UA 0x07

#define START_STATE 0
#define FLAG_STATE 1
#define A_STATE 2
#define C_STATE 3
#define BCC_STATE 4
#define FINAL_STATE 5

typedef struct {
  int state;
} statemachine;

statemachine * newStateMachine();

void resetStateMachine(statemachine *a);

void setState(statemachine *a, int newState);

int getMachineState(statemachine *a);

void interpretSignal(statemachine *a, unsigned char s);

#endif
