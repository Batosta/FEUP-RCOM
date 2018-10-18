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
#include "api.h"
#include "appLayer.h"

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define TRASNMITTER 1
#define RECEIVER 0
#define FLAG 0x7E
#define AE 0x03
#define AR 0x01
#define SET 0x03
#define DISC 0x09
#define UA 0x07

struct termios oldtio;
int path;
statemachine * controller;
linkLayer * linkL;
applicationLayer * app;

int flag=0, conta=1;

void atende()
{
	printf("Trying  to connect %hd of %hd times.\n", getNumberOFTries(linkL) + 1, getnumTransformations(linkL));
	flag=0;
	anotherTry(linkL);
}

void printUsage() {
	printf("Usage:\tnserial SerialPort mode<transmitter/receiver>\n\tex: nserial /dev/ttyS1 transmitter\n");
	exit(1);
}

int getMode(char * mode) {
	if(strcmp(mode, "transmitter") == 0) {
		return TRASNMITTER;
	}
	else {
		return RECEIVER;
	}
}

int openSerialPort(char* path) {
	int f = open(path, O_RDWR | O_NOCTTY );

	if(f < 0) {
		perror(path);
		exit(-1);
	}

	return f;
}

void analyseArgs(int argc, char** argv) {
	if(argc < 3) {
		printUsage();
	}

	/* Analyse first argument */
	if((strcmp("/dev/ttyS0", argv[1]) != 0) && (strcmp("/dev/ttyS1", argv[1]) != 0)) {
		printUsage();
	}

	/* Analyse second argument */
	if(strcmp("receiver", argv[2]) != 0 && strcmp("transmitter", argv[2]) != 0) {
		printUsage();
	}

	app = getAppLayer(openSerialPort(argv[1]), getMode(argv[2]));
}

void getOldAttributes(int path, struct termios* oldtio) {
	if ( tcgetattr(path ,oldtio) == -1) { /* save current port settings */
		perror("tcgetattr");
		exit(-1);
	}
}

void setNewAttributes(struct termios *newtio) {
	bzero(newtio, sizeof(struct termios));
	newtio->c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio->c_iflag = IGNPAR;
	newtio->c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio->c_lflag = 0;

	newtio->c_cc[VTIME] = 0;   /* inter-character timer unused */
	newtio->c_cc[VMIN] = 0;   /* blocking read until 5 chars received */
}

void serialPortSetup(int path, struct termios *newtio) {

	setNewAttributes(newtio);

	tcflush(path, TCIOFLUSH);

	if(tcsetattr(path, TCSANOW, newtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}
}

int waitForData(int path) {
	unsigned char buf[1];

	if(getMachineState(controller) == FINAL_STATE) {
		return TRUE;
	}

	memset(buf, 0, 1);

	read(path,buf,1);

	interpretSignal(controller, buf[0]);

	if(getMachineState(controller) == FINAL_STATE) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int sendSetMessage(int path) {
	unsigned char setMessage[5];
	int writtenBytes = 0;

	setMessage[0] = FLAG;
	setMessage[1] = AE;
	setMessage[2] = SET;	// If this was the receptor SET[2] would be 0x01
	setMessage[3] = setMessage[1]^setMessage[2];
	setMessage[4] = FLAG;

	//ESCREVER UM BYTE DE CADA VEZ

	writtenBytes = write(path, setMessage, 5);


	if(writtenBytes != 5) {
		return FALSE;
	}

	return TRUE;
}

int llopen(int path, int mode) {

	switch(mode) {
		case RECEIVER:
			while(waitForData(path) == FALSE) {}
			printf("Data received \n");
			sendSetMessage(path);
		break;
		case TRASNMITTER:
			while(getNumberOFTries(linkL) < getnumTransformations(linkL)){
				if(flag == 0) {
					sendSetMessage(path);
					alarm(getTimeout(linkL));
					flag = 1;
				}
				if(waitForData(path) == TRUE){
					alarm(0);
					return TRUE;
				}
			}
			return FALSE;
		break;
		default:
			return FALSE;
	}
	
	return TRUE;
}

void sigint_handler(int signo) {
	printf("Ctrl+C received\n");

	if(tcsetattr(path, TCSANOW, &oldtio) == -1) {
		perror("signal tcsetattr");
		exit(-1);
	}

	exit(0);
}

void configureLinkLayer(char * path) {
	unsigned short tm, tries;

	printf("Insert timeout value (s): ");

	scanf("%hd", &tm);

	printf("Insert number of tries: ");

	scanf("%hd", &tries);

	linkL = getLinkLayer(tries, tm, path);
}

int main(int argc, char** argv) {

	int fd, mode;
	struct termios newtio;

	signal(SIGINT, sigint_handler);
	(void) signal(SIGALRM, atende);

	analyseArgs(argc, argv);

	fd = openSerialPort(argv[1]);

	path = fd;

	getOldAttributes(fd, &oldtio);

	defineOldPortAttr(app, oldtio);

	configureLinkLayer(argv[1]);

	serialPortSetup(fd, &newtio);

	if(strcmp(argv[2], "transmitter") == 0) {
		mode = TRASNMITTER;
	} else {
		mode = RECEIVER;
	}

	controller = newStateMachine();

	if(llopen(fd, mode) == TRUE) {
		printf("Connection established!");
	} else {
		perror("connection failed!");

		tcsetattr(fd,TCSANOW,&oldtio);
		close(fd);
		exit(-1);
	}

	sleep(1);
	tcsetattr(fd,TCSANOW,&oldtio);

	close(fd);

	return 0;
}
