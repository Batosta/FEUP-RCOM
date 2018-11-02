#include "api.h"

//DEFINES
void definePort(linkLayer *a, char *port){
	strcpy(a->port, port);
}

void defineBaudRate(linkLayer *a, int baudRate){
	a->baudRate = baudRate;
}

void defineTimeout(linkLayer *a, unsigned short timeout){
	a->timeout = timeout;
}

void defineNumTransformations(linkLayer *a, unsigned short numTransformations){
	a->numTransformations = numTransformations;
}

void anotherTry(linkLayer *a) {
	a->tries++;
}

void setFlag(linkLayer *a, unsigned short v) {
	a->flag = v;
}

void resetTries(linkLayer *a) {
	a->tries = 0;
}


//GETS
linkLayer * getLinkLayer(unsigned short nTries, unsigned short timeout, char * port) {
	linkLayer * a = (linkLayer*) malloc(sizeof(linkLayer));

	a->timeout = timeout;

	a->numTransformations = nTries;

	a->tries = 0;

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

unsigned short getTimeout(linkLayer *ll){
	return ll->timeout;
}

unsigned short getnumTransformations(linkLayer *ll){
	return ll->numTransformations;
}

void initializeStateMachine(linkLayer *a, unsigned char parameter, int mode) {
	if(a->controller) {
		free(a->controller);
	}
	a->controller = newStateMachine(parameter, mode);
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
