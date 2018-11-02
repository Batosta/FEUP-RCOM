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
#include <time.h>
#include "ReceiverStateMachine.h"
#include "StateMachine.h"
#include "api.h"
#include "appLayer.h"

#define BAUDRATE B230400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define TRANSMITTER 1
#define RECEIVER 0
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81
#define MAX_FRAME_SIZE 131594

#define DATA_C 1
#define START_C 2
#define END_C 3

#define SIZE_PARAMETER 0
#define NAME_PARAMETER 1

#define OCTETO1 0x7E
#define OCTETO2 0x7D

#define clear() printf("\033[H\033[J")

linkLayer * linkL;
applicationLayer * app;
int CFlag = 0;
unsigned int attempts = 0;
char BST[3] = {0x7D, 0x5E, 0x5D};

void atende(){
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
		return TRANSMITTER;
	}
	else {
		return RECEIVER;
	}
}

int openFile(char* path) {
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

	app = getAppLayer(openFile(argv[1]), getMode(argv[2]));
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

	newtio->c_cc[VTIME] = 1;   /* inter-character timer unused */
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

/*
Função que tenta dar write dos frames.
*/
int write_frame(int fd, unsigned char *frame, unsigned int length) {
	int aux, total = 0;

	while(total < length){
		aux = write(fd, frame, length);

		if(aux <= 0)
		return -1;

		total += aux;
	}

	return total;
}


// Sends a supervision frame to the emissor with the answer
void sendSupervisionFrame(unsigned char controlField, int fd) {

	unsigned char buffer[5];

	// FLAG
	buffer[0] = FLAG;

	// A
	if(getStatus(app) == TRANSMITTER) {
		if(controlField == DISC || controlField == SET) {
			buffer[1] = AE;
		}
		else {
			buffer[1] = AR;
		}
	}
	else if(getStatus(app) == RECEIVER) {
		if(controlField == DISC || controlField == SET) {
			buffer[1] = AR;
		}
		else {
			buffer[1] = AE;
		}
	}
	else {
		perror("Invalid mode\n");
		exit(-11);
	}

	// C
	buffer[2] = controlField;
	// BCC
	buffer[3] = buffer[1] ^ buffer[2];

	// FLAG
	buffer[4] = FLAG;

	write_frame(fd, buffer, 5);
}

void sigint_handler(int signo) {
	printf("Ctrl+C received\n");

	resetPortConfiguration(app);

	exit(0);
}

void configureLinkLayer(char * path) {
	unsigned short tm, tries;
	int fd;
	unsigned int dimension = 0;
	char filePath[256];

	printf("Insert timeout value (s): ");

	scanf("%hd", &tm);

	printf("Insert number of tries: ");

	scanf("%hd", &tries);

	linkL = getLinkLayer(tries, tm, path);

	if(getStatus(app) == TRANSMITTER) {
		//LER NOME DO FICHEIRO e carregar fd para appLayer
		printf("Insert file path: ");
		scanf("%s", filePath);
		fd = openFile(filePath);
		setTargetDescriptor(app, fd);
		defineFileName(app, filePath);

		do {
			printf("Insert frame size(min - 8 | max - 62000): ");
			scanf("%s", filePath);
			dimension = atoi(filePath);
		} while(dimension < 8 || dimension > 62000);

		defineSelectedFrameSize(app, dimension);
	}

	clear();
}

void installSignalHandlers() {
	signal(SIGINT, sigint_handler);
	(void) signal(SIGALRM, atende);
}

unsigned char getBCC2(unsigned char *buffer, int length){

	unsigned char bcc2;
	int i;

	bcc2 = buffer[0];

	for(i = 1; i < length; i++){
		bcc2 = bcc2^buffer[i];
	}

	return bcc2;
}

unsigned char *getStuffedData(unsigned char *buffer, int length){

	int i, j, contador = 0, stuffedLength;
	unsigned char *stuffed;


	for(i = 0; i < length; i++){
		if(buffer[i] == OCTETO1 || buffer[i] == OCTETO2) {
			contador++;
		}

	}

	stuffedLength = contador + length;

	stuffed = (unsigned char*)malloc(stuffedLength);

	for(i = 0, j = 0; i < length; i++){

		if(buffer[i] == 0x7e) {

			stuffed[j] = 0x7d;
			j++;
			stuffed[j] = 0x5e;
			j++;

		}else if(buffer[i] == 0x7d) {

			stuffed[j] = 0x7d;
			j++;
			stuffed[j] = 0x5d;
			j++;

		}else{

			stuffed[j] = buffer[i];
			j++;
		}
	}
	return stuffed;
}

int getStuffedLength(unsigned char *buffer, int length){

	int i, contador = 0, stuffedLength;

	for(i = 0; i < length; i++){
		if(buffer[i] == OCTETO1 || buffer[i] == OCTETO2)
		contador++;
	}

	stuffedLength = contador + length;
	return stuffedLength;
}

unsigned char * appendBCC2(unsigned char* buffer, int length, unsigned char bcc) {
	unsigned char * frame;

	frame = (unsigned char*)malloc((length + 1)*sizeof(unsigned char));

	memcpy(frame, buffer, length);

	frame[length] = bcc;

	return frame;
}

// Cria uma trama de informacao
int llwrite(int fd, unsigned char* buffer, int length){

	unsigned char *frame, *stuffed, bytesStream[1];

	int frameLength, stuffedLength;

	unsigned char bcc2;

	statemachine *detectREJ, *detectRR;

	bcc2 = getBCC2(buffer, length);

	buffer = appendBCC2(buffer, length, bcc2);

	length++;

	stuffedLength = getStuffedLength(buffer, length);

	stuffed = getStuffedData(buffer, length);

	frameLength = stuffedLength + 5; //length dos dados mais F,A,C,BCC1,BCC2,F

	frame = (unsigned char *) malloc(frameLength);

	frame[0] = FLAG;
	frame[1] = AE;

	frame[2] = CFlag == 0 ? 0x00 : 0x40;

	frame[3] = frame[1]^frame[2];

	memcpy(frame+4, stuffed, stuffedLength);

	frame[stuffedLength + 4] = FLAG;

	setFlag(linkL, 0);

	detectREJ = newStateMachine(CFlag == 0 ? REJ1 : REJ0, TRANSMITTER);

	detectRR = newStateMachine(CFlag == 0 ? RR1 : RR0, TRANSMITTER);

	while(getMachineState(detectRR) != FINAL_STATE) {
		if(getFlag(linkL) == 0) {
			if(write_frame(fd, frame, frameLength) == -1) {
				printf("Error writting frame\n");
				return -1;
			}
			alarm(getTimeout(linkL));
			setFlag(linkL, 1);
			resetStateMachine(detectREJ);
			resetStateMachine(detectRR);
		}

		read(fd, bytesStream, 1);

		interpretSignal(detectREJ, bytesStream[0]);
		interpretSignal(detectRR, bytesStream[0]);

		if(getMachineState(detectREJ) == FINAL_STATE) { //se entrar é sinal que detectou um REJ
			printf("REJ detected! Resending.\n");
			if(write_frame(fd, frame, frameLength) == -1) {
				printf("Error writting frame\n");
				return -1;
			}
			alarm(getTimeout(linkL));
			setFlag(linkL, 1);
			resetStateMachine(detectREJ);
			resetStateMachine(detectRR);
		}
	}

	alarm(0);

	return frameLength;
}


int sendFileInfo(char * fileName, int control) {
	struct stat st;
	unsigned int startLength;
	unsigned char * controlStart;
	off_t sizeOfFile;
	int i, j;


	fstat(getTargetDescriptor(app), &st);

	sizeOfFile = st.st_size;

	startLength = 7 + sizeof(off_t) + strlen(fileName);
	controlStart = (unsigned char *)malloc(sizeof(unsigned char)*startLength);

	controlStart[0] = control;
	controlStart[1] = SIZE_PARAMETER;
	controlStart[2] = sizeof(off_t);

	memcpy(&controlStart[3], &sizeOfFile, sizeof(off_t));

	controlStart[3+sizeof(off_t)] = NAME_PARAMETER;
	controlStart[4+sizeof(off_t)] = strlen(fileName);

	for(i = 5 + sizeof(off_t), j = 0; j < strlen(fileName); i++, j++) {
		controlStart[i] = fileName[j];
	}

	//Acabamos de fazer o Data packet de Start
	if(llwrite(getFileDescriptor(app), controlStart, startLength) < 0){
		printf("Error on llwrite: Start Packet\n");
		return 1;
	}

	return (int)sizeOfFile;
}

void progressBar(int fileSize, int sentBytes) {
	int progress = (int)(1.0 * sentBytes / fileSize * 100);
	int length = progress / 2;
	int i;

	printf("\rPROGRESS: |");
	for(i = 0; i < length-1; i++) {
		printf("=");
	}

	printf(">");

	for(i = 0; i < 50 - length; i++) {
		printf(" ");
	}

	printf("| %d%% (%d / %d)", progress, sentBytes, fileSize);

	fflush(stdout);
}

double what_time_is_it() {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec + now.tv_nsec*1e-9;
}

int sendFileData(int fileSize, int frameSize){

	int fileSizeAux, bytes = 0, bytesRead = 0, nSeq = 0;
	unsigned char *buffer, bufferFile[frameSize-4];
	double begin, end, delta;

	fileSizeAux = fileSize;

	buffer = (unsigned char*) malloc(frameSize); //[C,N,L2,L1,P1...PK]

	begin = what_time_is_it();

	while(bytes < fileSizeAux){

		bytesRead = read(getTargetDescriptor(app), bufferFile, frameSize-4);

		if(bytesRead < 0){
			printf("Error reading %s\n", getFileName(app));
			continue;
		}

		buffer[0] = DATA_C;
		buffer[1] = (unsigned char)nSeq%256; //Nao sei se e necessario mudar para char
		buffer[2] = bytesRead/256;
		buffer[3] = bytesRead%256;

		memcpy(buffer+4, bufferFile, bytesRead); //Data packet, comeca na posiçao 5 e vai buscar a informacao no bufferFile

		if(llwrite(getFileDescriptor(app), buffer, bytesRead + 4) < 0 ){
			printf("Error llwrite: Data Packet %d", nSeq);
			return -1;
		}

		memset(bufferFile, 0, frameSize-4);
		memset(buffer, 0, frameSize);

		bytes += bytesRead;
		nSeq++;

		progressBar(fileSizeAux, bytes);
	}

	end = what_time_is_it();

	delta = end-begin;

	printf("\nFile sent!\nElapsed time: %.1f seconds.\nAVG Speed: %.1f bytes per second.\n", delta, bytes/delta);

	close(getTargetDescriptor(app));

	return 0;
}

void sendFile() {

	int sizeFile;

	sizeFile = sendFileInfo(getFileName(app), START_C);//Trama de controlo START

	sendFileData(sizeFile, getSelectedFrameSize(app));//Tramas de dados do ficheiro

	sizeFile = sendFileInfo(getFileName(app), END_C);//Trama de controlo END
}

unsigned char * llread() {

	receiverstatemachine * x;
	unsigned char frame[1];
	unsigned char answer;
	unsigned char * data;

	x = newReceiverStateMachine(CFlag == 0 ? 0x00 : 0x40);

	do {
		read(getFileDescriptor(app), frame, 1);
		interpretChar(x, frame[0]);
	} while(getState(x) != FINAL_STATE && getState(x) != ERROR_STATE);

	if(getState(x) == ERROR_STATE) {
		/* if there is an error in the header we ignore the frame */
		return NULL;
	}

	/* if there is an error in the data we send REJ */
	if(getSentBCC2(x) != getCalculatedBCC2(x)) {
		answer = CFlag == 0 ? REJ1 : REJ0;
		sendSupervisionFrame(answer, getFileDescriptor(app));
		return NULL;
	}

	answer = CFlag == 0 ? RR1 : RR0;

	sendSupervisionFrame(answer, getFileDescriptor(app));

	data =  getDataFrame(x);

	deleteStateMachine(x);

	return data;
}

int readDataPackets(){

		char * filename;
		unsigned char * buffer;
		unsigned char dataPacket[MAX_SIZE+4];
		int receivedBytes = 0;
		long int fileSize;
		int orderByte = -1;
		double begin, end;
		double delta;

		begin = what_time_is_it();

		do {
			buffer = llread();

			if(buffer == NULL) {
				printf("ERROR\n");
			}
			if(buffer[0] == 1){ // C = 1  -> Pacote de dados
				if(buffer[1] != (orderByte + 1) % 256) {
					continue;
				}
				else {
					orderByte++;

					int k = 256 * buffer[2] + buffer[3];

					memcpy(dataPacket, buffer+4, k);

					write_frame(getTargetDescriptor(app), dataPacket, k);
					receivedBytes += k;
				}
			}
			else if(buffer[0] == 2){ // C = 2 -> Pacote de controlo start  //  C = 3 -> Pacote de controlo end

				unsigned char l = 0;
				unsigned char lLast = l;
				unsigned char t = 0;


				int i;
				for(i = 0; i < 2; i++){

					t = buffer[1 + i*(1+1+l)];
					lLast = l;
					l = buffer[1 + i*(1+1+lLast) + 1];

					if(t == 0){	// Tamanho do ficheiro - Funciona

						fileSize = 0x00;
						int k;
						for(k = 0; k < l; k++){
							fileSize |= buffer[10-k] << 8*(7-k);
						}

						printf("\nFILE SIZE: %ld bytes\n", fileSize);

					} else if(t == 1){	// Nome do ficheiro - Funciona

						filename = malloc(l + 1);
						int k;
						for(k = 0; k < l; k++){

							filename[k] = buffer[1 + i*(1+1+lLast) + 2 + k];
						}
						filename[k] = 0;

						printf("FILE NAME: %s\n", filename);

						setTargetDescriptor(app, open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777));

					} else {
						printf("T with unknown value\n");
						return -1;
					}
				}
			}
			progressBar(fileSize, receivedBytes);
		} while(buffer[0] != 3);

		end = what_time_is_it();

		delta = end - begin;

		printf("\nFile received!\nElapsed time: %.1f seconds.\nAVG Speed: %.1f bytes per second.\n", delta, fileSize/delta);
		close(getTargetDescriptor(app));
		fflush(stdout);;
		return 0;
}

int llopen(int path, int mode) {

	switch(mode) {
		case RECEIVER:
		initializeStateMachine(linkL, SET, RECEIVER);
		while(waitForData(path) == FALSE) {/*printf("\r Waiting.");fflush(stdout);*/}
		sendSupervisionFrame(SET, getFileDescriptor(app));
		return TRUE;
		case TRANSMITTER:
		initializeStateMachine(linkL, SET, TRANSMITTER);
		resetTries(linkL);
		setFlag(linkL, 0);
		while(getNumberOFTries(linkL) < getnumTransformations(linkL)){
			if(getFlag(linkL) == 0) {
				sendSupervisionFrame(SET, getFileDescriptor(app));
				alarm(getTimeout(linkL));
				setFlag(linkL, 1);
			}
			if(waitForData(path) == TRUE){
				alarm(0);
				return TRUE;
			}
		}
		return FALSE;
		default:
		return TRUE;
	}

	return TRUE;
}

// Retorna 1 em caso de sucesso
// Retorna -1 em caso de erro
int llclose(int path, int mode) {

	switch(mode){
		case RECEIVER:
		initializeStateMachine(linkL, DISC, RECEIVER);
		while(waitForData(path) == FALSE) {}
		sendSupervisionFrame(DISC, getFileDescriptor(app));
		initializeStateMachine(linkL, UA, RECEIVER);
		while(waitForData(path) == FALSE) {}
		return TRUE;
		case TRANSMITTER:
		initializeStateMachine(linkL, DISC, TRANSMITTER);
		resetTries(linkL);
		setFlag(linkL, 0);
		while(getNumberOFTries(linkL) < getnumTransformations(linkL)) {
			if(getFlag(linkL) == 0) {
				sendSupervisionFrame(DISC, getFileDescriptor(app));
				alarm(getTimeout(linkL));
				setFlag(linkL, 1);
			}
			if(waitForData(path) == TRUE) {
				alarm(0);
				sendSupervisionFrame(UA, getFileDescriptor(app));
				return TRUE;
			}
		}
		return FALSE;
		default:
		return FALSE;
	}

	return FALSE;
}

void createConnection() {
	if(llopen(getFileDescriptor(app), getStatus(app)) == TRUE) {
		printf("Connection established!\n");
	} else {
		perror("connection failed!\n");
		resetPortConfiguration(app);
		close(getFileDescriptor(app));
		exit(-1);
	}
}

void closeConnection() {
	if(llclose(getFileDescriptor(app), getStatus(app)) == 1) {
		printf("\nConnection closed successfully!\n");
	} else {
		printf("Failed to close connection!\n");
	}
}

int main(int argc, char** argv) {

	installSignalHandlers();

	analyseArgs(argc, argv);

	getOldAttributes(getFileDescriptor(app));

	configureLinkLayer(argv[1]);

	serialPortSetup(getFileDescriptor(app));

	createConnection();

	if(getStatus(app) == TRANSMITTER) {
		sendFile();
	}

	else if(getStatus(app) == RECEIVER){
		readDataPackets();
	}
	else {
		perror("Wrong mode!\n");
	}

	closeConnection();

	sleep(1);
	resetPortConfiguration(app);

	close(getFileDescriptor(app));

	return 0;
}
