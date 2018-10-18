#include "appLayer.h"


//GETS
applicationLayer * getAppLayer (int fd, int mode) {
	applicationLayer * a = (applicationLayer*) malloc(sizeof(applicationLayer));

	a->fileDescriptor = fd;
	a->status = mode;
}

int getFileDescriptor(applicationLayer *a){
	return a->fileDescriptor;
}

int getStatus(applicationLayer *a){
	return a->status;
}

struct termios getOldPortConfiguration(applicationLayer *a){
	return a->oldPortConfiguration;
}


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


//DESTRUCTORS
//void destructFileDescriptor(applicationLayer *a){
//	free(a->fileDescriptor);
//}
//
//void destructStatus(applicationLayer *a){
//	free(a->status);
//}

