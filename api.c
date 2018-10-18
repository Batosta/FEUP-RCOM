#include "api.h"

#define MAX_SIZE 8

//DEFINES

void defineOldPortAttr(applicationLayer *a, struct termios b) {
	a->oldPortConfiguration = b;
}

void defineFileDescriptor(applicationLayer *a, int fd){
	a->fileDescriptor = fd;
}

void defineStatus(applicationLayer *a, int status){
	a->status = status;
}

void definePort(linkLayer *a, char *port){
	strcpy(a->port, port);
}

void defineBaudRate(linkLayer *a, int baudRate){
	a->baudRate = baudRate;
}

void defineSequenceNumber(linkLayer *a, unsigned short sequenceNumber){
	a->sequenceNumber = sequenceNumber;
}

void defineTimeout(linkLayer *a, unsigned short timeout){
	a->timeout = timeout;
}

void defineNumTransformations(linkLayer *a, unsigned short numTransformations){
	a->numTransformations = numTransformations;
}

void defineFrame(linkLayer *a, char *frame){
	strcpy(a->frame,frame);
}


//GETS
linkLayer * getLinkLayer(unsigned short nTries, unsigned short timeout, char * port) {
	linkLayer * a = (linkLayer*) malloc(sizeof(linkLayer));

	a->timeout = timeout;

	a->numTransformations = nTries;

	return a;
}

applicationLayer * getAppLayer (int fd, int mode) {
	applicationLayer * a = (applicationLayer*) malloc(sizeof(applicationLayer));

	a->fileDescriptor = fd;
	a->status = mode;
}

unsigned short getFlag(linkLayer *a) {
	return a->flag;
}

unsigned short getNumberOFTries(linkLayer *a) {
	return a->tries;
}

int getFileDescriptor(applicationLayer *a){
	return a->fileDescriptor;
}

int getStatus(applicationLayer *a){
	return a->status;
}

char * getPort(linkLayer *ll){
	return ll->port;
}

int getBaudRate(linkLayer *ll){
	return ll->baudRate;
}

unsigned short getSequenceNumber(linkLayer *ll){
	return ll->sequenceNumber;
}

unsigned short getTimeout(linkLayer *ll){
	return ll->timeout;
}

unsigned short getnumTransformations(linkLayer *ll){
	return ll->numTransformations;
}

char * getFrame(linkLayer *ll){
	return ll->frame;
}

void anotherTry(linkLayer *a) {
	a->tries++;
}

//DESTRUCTORS

// void destructFileDescriptor(applicationLayer *a){
// 	free(a->fileDescriptor);
// }
//
// void destructStatus(applicationLayer *a){
// 	free(a->status);
// }
//
// void destructPort(linkLayer *ll){
// 	free(ll->port);
// }
//
// void destructBaudRate(linkLayer *ll){
// 	free(ll->baudRate);
// }
//
// void destructSequenceNumber(linkLayer *ll){
// 	free(ll->sequenceNumber);
// }
//
// void destructTimeout(linkLayer *ll){
// 	free(ll->timeout);
// }
//
// void destructNumTransformations(linkLayer *ll){
// 	free(ll->numTransformations);
// }
//
// void destructFrame(linkLayer *ll){
// 	free(ll->frame);
// }