#ifndef APPLAYER
#define APPLAYER

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "StateMachine.h"

typedef struct applicationLayer {
	int fileDescriptor;	/*Descritor correspondente à porta de série*/
	int status;	/*TRANSMITTER | RECEIVER*/
	struct termios oldPortConfiguration;
	int targetFileDescriptor;
} applicationLayer;


//GETS
applicationLayer * getAppLayer (int fd, int mode);

int getFileDescriptor(applicationLayer *a);

int getStatus(applicationLayer *a);

struct termios getOldPortConfiguration(applicationLayer *a);

int getTargetDescriptor(applicationLayer *a);


//DEFINES
void setTargetDescriptor(applicationLayer *a, int fd);

void defineOldPortAttr(applicationLayer *a, struct termios b);

void defineFileDescriptor(applicationLayer *a, int fd);

void defineStatus(applicationLayer *a, int status);

void resetPortConfiguration(applicationLayer *a);


//DESTRUCTORS
// void destructFileDescriptor(applicationLayer *a);
//
// void destructStatus(applicationLayer *a);

#endif
