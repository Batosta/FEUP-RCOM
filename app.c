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

linkLayer * linkL;
applicationLayer * app;

void atende()
{
	printf("Trying  to connect %hd of %hd times.\n", getNumberOFTries(linkL) + 1, getnumTransformations(linkL));
	setFlag(linkL, 0);
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

void getOldAttributes(int path) {
	struct termios* oldtio = (struct termios*) malloc(sizeof(struct termios));

	if ( tcgetattr(path, oldtio) == -1) { /* save current port settings */
		perror("tcgetattr");
		exit(-1);
	}

	defineOldPortAttr(app, *oldtio);
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

void serialPortSetup(int path) {

	struct termios *newtio = (struct termios*)malloc(sizeof(struct termios));

	setNewAttributes(newtio);

	tcflush(path, TCIOFLUSH);

	if(tcsetattr(path, TCSANOW, newtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}
}

int waitForData(int path) {
	unsigned char buf[1];

	if(getMachineState(linkL->controller) == FINAL_STATE) {
		return TRUE;
	}

	memset(buf, 0, 1);

	read(path,buf,1);

	interpretSignal(linkL->controller, buf[0]);

	if(getMachineState(linkL->controller) == FINAL_STATE) {
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

	initializeStateMachine(linkL);

	switch(mode) {
		case RECEIVER:
			while(waitForData(path) == FALSE) {}
			printf("Data received \n");
			sendSetMessage(path);
		break;
		case TRASNMITTER:
			while(getNumberOFTries(linkL) < getnumTransformations(linkL)){
				if(getFlag(linkL) == 0) {
					sendSetMessage(path);
					alarm(getTimeout(linkL));
					setFlag(linkL, 1);
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

int llwrite(int fd, char* buffer, int length){
/*	
	char *frame;
	frame[0] = FLAG;
	frame[1] = AE;
	frame[2] = 0;
	frame[3] = frame[1]^frame[2];

	char bcc2 = buffer[0];
	
	int i = 4, j = 0; 
	for(; j < length; i++, j++){
		frame[i] = buffer[j];
		
		if(j != 0)
			bcc2 = bcc2^buffer[j];
	}

	frame[length+4] = bcc2;
	frame[length+5] = FLAG;


	int numBytes = write(fd, frame, length+6);
	if(numBytes == -1){
		printf("Error writting on llwrite\n");
	}

	return numBytes;
*/
}
//Função responsável por ler o ficheiro a mandar e enviar por llwrite os pacotes de dados
int readFile(char *fileName){
/*
	int fd = open(fileName, O_RDONLY);
	if(fd == -1)
		printf("Unable to open file %s", filename);

	//buffer vai guardar o ficheiro
	char *buffer;
	int i, length, r = 0;
	
	while((r = read(fd, buffer[i*1024], 1024)) > 0){
		i++;
		length += r;
	}
	//Aqui constrói pacotes de dados
	
	char* control;
	control[0] = 2;

	control[1] = 1;
	control[2] = strlen(fileName);
	for(i = 3, j = 0; i < control[2]; i++, j++){
		control[i] = fileName[j]; 
	}

	control[i+1] = 0;
	control[i+2] = sizeof(int);
	control[i+3] = length;

	int send = llwrite(fd, control, i+3);
	int k = 0;
	
	while(1){
		char pacote[128];
		pacote[0] = 1;
		pacote[1] = 8; 
		pacote[2] = 124/256;
		pacote[3] = 124%256;
		
		k += memcpy( &pacote[4], &buffer[k], 124);
		
		llwrite(fd, pacote, 128);
	}

	return 0;
*/
}

void sigint_handler(int signo) {
	printf("Ctrl+C received\n");

	resetPortConfiguration(app);

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

void installSignalHandlers() {
	signal(SIGINT, sigint_handler);
	(void) signal(SIGALRM, atende);
}

int main(int argc, char** argv) {

	installSignalHandlers();

	analyseArgs(argc, argv);

	getOldAttributes(getFileDescriptor(app));

	configureLinkLayer(argv[1]);

	serialPortSetup(getFileDescriptor(app));

	if(llopen(getFileDescriptor(app), getStatus(app)) == TRUE) {
		printf("Connection established!\n");
	} else {
		perror("connection failed!\n");
		resetPortConfiguration(app);
		close(getFileDescriptor(app));
		exit(-1);
	}

	sleep(1);
	resetPortConfiguration(app);

	close(getFileDescriptor(app));

	return 0;
}
