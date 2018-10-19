#ifndef API
#define API

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

#define MAX_SIZE 8

typedef struct linkLayer {
	char port[20];	/*Dispositivo /dev/ttySx, x = 0, 1*/
	int baudRate;	/*Velocidade de transmissão*/
	unsigned short sequenceNumber;	/*Número de sequência da trama: 0, 1*/
	unsigned short timeout;	/*Valor do temporizador: 1s*/
	unsigned short numTransformations;	/*Número de tentativas em caso de falha*/
	unsigned short flag;
	unsigned short tries; //Number of tries that have already been made
	char frame[MAX_SIZE];	/*Trama*/
	statemachine * controller;
} linkLayer;

//GETS

linkLayer * getLinkLayer(unsigned short nTries, unsigned short timeout, char * port);

unsigned short getFlag(linkLayer *a);

unsigned short getNumberOFTries(linkLayer *a);

char * getPort(linkLayer *ll);

int getBaudRate(linkLayer *ll);

unsigned short getSequenceNumber(linkLayer *ll);

unsigned short getTimeout(linkLayer *ll);

unsigned short getnumTransformations(linkLayer *ll);

char * getFrame(linkLayer *ll);

//DEFINES

void setFlag(linkLayer *a, unsigned short v);

void anotherTry(linkLayer *a);

void definePort(linkLayer *a, char *port);

void defineBaudRate(linkLayer *a, int baudRate);

void defineSequenceNumber(linkLayer *a, unsigned short sequenceNumber);

void defineTimeout(linkLayer *a, unsigned short timeout);

void defineNumTransformations(linkLayer *a, unsigned short numTransformations);

void defineFrame(linkLayer *a, char *frame);

void initializeStateMachine(linkLayer *a);

//DESTRUCTORS

// void destructPort(linkLayer *ll);
//
// void destructBaudRate(linkLayer *ll);
//
// void destructSequenceNumber(linkLayer *ll);
//
// void destructTimeout(linkLayer *ll);
//
// void destructNumTransformations(linkLayer *ll);
//
// void destructFrame(linkLayer *ll);

#endif
