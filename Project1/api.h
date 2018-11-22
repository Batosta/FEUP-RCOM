#ifndef DATALAYER
#define DATALAYER

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

#define MAX_SIZE 65792

typedef struct linkLayer {
	char port[20];	/*DEvice /dev/ttySx, x = 0, 1*/
	int baudRate;	/*Transmission speed*/
	unsigned short timeout;	/*Time out value 1s*/
	unsigned short numTransformations;	/*Number of tries*/
	unsigned short flag;
	unsigned short tries; //Number of tries that have already been made
	statemachine * controller;
} linkLayer;

//GETS

linkLayer * getLinkLayer(unsigned short nTries, unsigned short timeout, char * port);

unsigned short getFlag(linkLayer *a);

unsigned short getNumberOFTries(linkLayer *a);

char * getPort(linkLayer *ll);

int getBaudRate(linkLayer *ll);

unsigned short getTimeout(linkLayer *ll);

unsigned short getnumTransformations(linkLayer *ll);

//DEFINES

void setFlag(linkLayer *a, unsigned short v);

void anotherTry(linkLayer *a);

void definePort(linkLayer *a, char *port);

void defineBaudRate(linkLayer *a, int baudRate);

void defineTimeout(linkLayer *a, unsigned short timeout);

void defineNumTransformations(linkLayer *a, unsigned short numTransformations);

void initializeStateMachine(linkLayer *a, unsigned char parameter, int mode);

void resetTries(linkLayer *a);

#endif
