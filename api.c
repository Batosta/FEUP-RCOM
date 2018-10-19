#include "api.h"

#define MAX_SIZE 8


//DEFINES
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

void anotherTry(linkLayer *a) {
	a->tries++;
}

void setFlag(linkLayer *a, unsigned short v) {
	a->flag = v;
}


//GETS
linkLayer * getLinkLayer(unsigned short nTries, unsigned short timeout, char * port) {
	linkLayer * a = (linkLayer*) malloc(sizeof(linkLayer));

	a->timeout = timeout;

	a->numTransformations = nTries;

	return a;
}

unsigned short getFlag(linkLayer *a) {
	return a->flag;
}

unsigned short getNumberOFTries(linkLayer *a) {
	return a->tries;
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

void initializeStateMachine(linkLayer *a) {
	a->controller = newStateMachine();
}

//DESTRUCTORS

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
