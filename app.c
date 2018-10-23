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

void sigint_handler(int signo) {
	printf("Ctrl+C received\n");

	resetPortConfiguration(app);

	exit(0);
}

void configureLinkLayer(char * path) {
	unsigned short tm, tries;
	int fd;
	char filePath[256];

	printf("Insert timeout value (s): ");

	scanf("%hd", &tm);

	printf("Insert number of tries: ");

	scanf("%hd", &tries);

	linkL = getLinkLayer(tries, tm, path);

	if(getStatus(app) == TRASNMITTER) {
		//LER NOME DO FICHEIRO e carregar fd para appLayer
		printf("Insert file path: ");
		scanf("%s", filePath);
		fd = openFile(filePath);
		setTargetDescriptor(app, fd);
	}
}

void installSignalHandlers() {
	signal(SIGINT, sigint_handler);
	(void) signal(SIGALRM, atende);
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

void sendFile() {
	struct stat st;

	fstat(getTargetDescriptor(app), &st);

	mode_t sizeOfFile = st.st_size;

	printf("size: %d\n", sizeOfFile);

	//Obter tamanho, nome do ficheiro e permissões e enviar
	//Depois comecar a enviar tramas dos bytes lidos no FICHEIRO
	//enviar pacote de controlo com END
}

int llwrite(int fd, unsigned char* buffer, int length){

	unsigned char *frame;

	frame = (unsigned char *) malloc(length + 6);
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
}

//Função responsável por ler o ficheiro a mandar e enviar por llwrite os pacotes de dados
int readFile(char *fileName, int Nbytes){

	int fd = open(fileName, O_RDONLY);
	int j;
	if(fd == -1)
		printf("Unable to open file %s", fileName);
	//buffer vai guardar o ficheiro
	unsigned char *buffer;
	int i, length, r = 0;

	//ir buscar o tamanho do ficheiro

	//buffer = (unsigned char *) malloc()

	// while((r = read(fd, buffer[i*1024], 1024)) > 0){
	// 	i++;
	// 	length += r;
	// }
	//Aqui constrói pacotes de dados

	unsigned char* control = (char *) malloc(strlen(fileName) + 6);
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

	buffer = (char *) malloc(sizeof(control));

	while(1){
		unsigned char pacote[128];
		pacote[0] = 1;
		pacote[1] = 8;
		pacote[2] = 124/256;
		pacote[3] = 124%256;

		memcpy( &pacote[4], &buffer[k], 124);

		llwrite(fd, pacote, 128);
	}
	return 0;
}

// Recebe a trama e checka se está bem
int llread(int fd, unsigned char * buffer){

	// Trama
	unsigned char array[134];	// 128 bytes do pacote de dados + 6 bytes da trama

	int i;	// tramaSize
	// Guardar no array
	for(i = 0; i < 134; i++){

		read(fd, array, 1);

		if(i != 0 && array[0] == FLAG)
			break;
	}

	if(i == 134)
		return -1;

	// First Flag
	if(array[0] != FLAG)
		return -1;

	// A
	if(array[1] != AE)
		return -1;

	// C
	if(array[2] != 0)
		return -1;

	// BCC1
	if(array[3] != (array[1] ^ array[2]))
		return -1;

	// Calcula o valor que o bcc2 deve ter
	unsigned char bcc2 = array[4];
	int k;
	for(k = 5; k < i-2; k++){

		bcc2 ^= array[k];
		buffer[k-5] = array[k];
	}

	// BCC2
	if(array[i-2] != bcc2){
		return -1;
	}

	// Second Flag
	if(array[i-1] != FLAG){
		return -1;
	}

	return i - 6;
}

// Checka se o pacote de dados está bem
// -1 = wrong
int readDataPackets(){

	unsigned char buffer[128];
	unsigned char dataPacket[128];
	char * filename;

	while(1){

		llread(getFileDescriptor(app), buffer);

		if(buffer[0] == 1){

			if(buffer[1] == 0xFF)
				return -1;

			int k = 256 * buffer[2] + buffer[3];

			int i;
			for(i = 0; i < k; i++) {
				dataPacket[i] = buffer[i];
			}

			// passar dataPacket para o ficheiro filename agora
			// open do ficheiro filename (em baixo) e dar write para la do dataPacket

		} else if(buffer[0] == 2 || buffer[0] == 3){

			int i;
			unsigned char l = 0;
			for(i = 0; i < 2; i++){

				unsigned char t = buffer[1 + i*(1+1+l)];

				if(t == 1){
					l = buffer[1 + i*(1+1+l) + 1];
					filename = malloc(l + 1);
					int k;
					for(k = 0; k < l; k++){

						filename[k] = buffer[1 + i*(1+1+l) + 2 + k];
					}
					filename[k] = 0;

					//fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);

				} else if(t == 0){

					// tamanho do ficheiro
				} else
					return -1;
			}

			if(buffer[0] == 3)
				break;
		}
	}

	return 0;
}

int main(int argc, char** argv) {

	installSignalHandlers();

	analyseArgs(argc, argv);

	getOldAttributes(getFileDescriptor(app));

	configureLinkLayer(argv[1]);

	serialPortSetup(getFileDescriptor(app));

	createConnection();

	if(getStatus(app) == TRASNMITTER) {
		sendFile();
	}
	else if(getStatus(app) == RECEIVER){

	}
	else {
		perror("Wrong mode!\n");
	}

	sleep(1);
	resetPortConfiguration(app);

	close(getFileDescriptor(app));

	return 0;
}
