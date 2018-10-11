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

#define MAX_SIZE 8

typedef struct applicationLayer{
	int fileDescriptor;	/*Descritor correspondente à porta de série*/
	int status;	/*TRANSMITTER | RECEIVER*/
} applicationLayer;

typedef struct linkLayer{
	char port[20];	/*Dispositivo /dev/ttySx, x = 0, 1*/
	int baudRate;	/*Velocidade de transmissão*/
	unsigned int sequenceNumber;	/*Número de sequência da trama: 0, 1*/
	unsigned int timeout;	/*Valor do temporizador: 1s*/
	unsigned int numTransformations;	/*Número de tentativas em caso de falha*/
	char frame[MAX_SIZE];	/*Trama*/
} linkLayer;

//GETS

int getFileDescriptor(applicationLayer *a);

int getStatus(applicationLayer *a);

char getPort(linkLayer *ll);

int getBaudRate(linkLayer *ll);

unsigned int getSequenceNumber(linkLayer *ll);

unsigned int getTimeout(linkLayer *ll);

unsigned int getnumTransformations(linkLayer *ll);

char getFrame(linkLayer *ll);

//DEFINES

void defineFileDescriptor(applicationLayer *a, int fd);

void defineStatus(applicationLayer *a, int status);

void definePort(linkLayer *a, char *port);

void defineBaudRate(linkLayer *a, int baudRate);

void defineSequenceNumber(linkLayer *a, unsigned int sequenceNumber);

void defineTimeout(linkLayer *a, unsigned int timeout);

void defineNumTransformations(linkLayer *a, unsigned int numTransformations);

void defineFrame(linkLayer *a, char *frame);

//DESTRUCTORS

void destructFileDescriptor(applicationLayer *a);

void destructStatus(applicationLayer *a);

void destructPort(linkLayer *ll);

void destructBaudRate(linkLayer *ll);

void destructSequenceNumber(linkLayer *ll);

void destructTimeout(linkLayer *ll);

void destructNumTransformations(linkLayer *ll);

void destructFrame(linkLayer *ll);

#endif

