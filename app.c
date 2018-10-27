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
#define TRANSMITTER 1
#define RECEIVER 0
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81

#define DATA_C 1
#define START_C 2
#define END_C 3

#define SIZE_PARAMETER 0
#define NAME_PARAMETER 1

#define OCTETO1 0x7E
#define OCTETO2 0x7D

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

/*
	Função que tenta dar write dos frames.
*/
int write_frame(int fd, unsigned char *frame, unsigned int length){
	int aux, total = 0;

	while(total < length){
		aux = write(fd, frame, length);

		if(aux <= 0)
			return -1;

		total += aux;
	}
	return total;
}

int llopen(int path, int mode) {

	initializeStateMachine(linkL);

	switch(mode) {
		case RECEIVER:
			while(waitForData(path) == FALSE) {}
			printf("Data received \n");
			sendSetMessage(path);
		break;
		case TRANSMITTER:
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

// Error/Rej: -1
// RR: 1
// SET: 2
// DISC: 3
// UA: 4
int receiveSupervisionFrame(int fd){

	unsigned char *buffer;
	buffer = (unsigned char *) malloc(sizeof(char)*5);

	int r;
	r = read(fd, buffer, sizeof(char)*5);
	if(r < 0){
		printf("Erro no tamanho da Trama de Supervisao");
		return -1;
	}
	
	// FLAG
	if(buffer[0] != FLAG){
		printf("Erro na FLAG_1 da Trama de Supervisao");
		return -1;
	}

	// A
	if(buffer[1] != AE){
		printf("Erro no A da Trama de Supervisao");
		return -1;
	}

	// BCC
	if(buffer[3] != (buffer[1]^buffer[2])){
		printf("Erro no BCC da Trama de Supervisao");
		return -1;
	}

	// FLAG
	if(buffer[4] != FLAG){
		printf("Erro na FLAG_2 da Trama de Supervisao");
		return -1;
	}

	// C
	if((CFlag == 0 && buffer[2] == RR0) || (CFlag == 1 && buffer[2] == RR1)){
		printf("RR na Trama de Supervisao");
		return 1;
	} else if(buffer[2] == SET){
		printf("SET na Trama de Supervisao");
		return 2;
	}
	else if(buffer[2] == DISC){
		printf("DISC na Trama de Supervisao");
		return 3;
	}
	else if(buffer[2] == UA){
		printf("UA na Trama de Supervisao");
		return 4;
	}
	else{
		printf("Erro na Trama de Supervisao");
		return -1;
	}

	return -1;
}

// Sends a supervision frame to the emissor with the answer
void sendSupervisionFrame(unsigned char controlField, int fd){

	printf("Started creating the supervision frame\n");

	unsigned char buffer[5];

	// FLAG
	buffer[0] = FLAG;

	// A
	if(getStatus(app) == TRANSMITTER)
		buffer[1] = AE;
	else
		buffer[1] = AR;

	// C
	buffer[2] = controlField;
	// BCC
	buffer[3] = buffer[0] ^ buffer[1];

	// FLAG
	buffer[4] = FLAG;

	write_frame(fd, buffer, 5);
}

// Retorna 1 em caso de sucesso
// Retorna -1 em caso de erro
int llclose(int fd, int mode){

	//setState(linkL?, FINAL_STATE?);

	switch(mode){

		case RECEIVER:
			while(1){				//trocar a condição

				if(receiveSupervisionFrame(getFileDescriptor(app) != 3))
					return -1;
				sendSupervisionFrame(DISC, getFileDescriptor(app));
				if(receiveSupervisionFrame(getFileDescriptor(app) != 4))
					return -1;
				}
			return 1;	
		break;
		
		case TRANSMITTER:
			while(1){				//trocar a condição

				sendSupervisionFrame(DISC, getFileDescriptor(app));
				if(receiveSupervisionFrame(getFileDescriptor(app) != 3))
					return -1;
				sendSupervisionFrame(UA, getFileDescriptor(app));
				}
			return 1;
		break;
		default:
			return 1;
	}
	
	return 1;
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

	if(getStatus(app) == TRANSMITTER) {
		//LER NOME DO FICHEIRO e carregar fd para appLayer
		printf("Insert file path: ");
		scanf("%s", filePath);
		fd = openFile(filePath);
		setTargetDescriptor(app, fd);
		defineFileName(app, filePath);
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

unsigned char getBCC2(unsigned char *buffer, int length){
	
	unsigned char bcc2;
	int i;

	bcc2 = buffer[0]; 

	for(i = 1; i < length; i++){
		bcc2 ^= buffer[i];
	}

	return bcc2;
}

unsigned char *getStuffedData(unsigned char *buffer, int length){

	int i, j, contador = 0, stuffedLength;
	unsigned char *stuffed;


	for(i = 0; i < length; i++){
		if(buffer[i] == OCTETO1 || buffer[i] == OCTETO2)
			contador++;
	}

	stuffedLength = contador + length;

	stuffed = (unsigned char*)malloc(stuffedLength);

	for(i = 0, j = 0; i < length; i++){

		if(buffer[i] == OCTETO1){

			stuffed[j] = BST[0];
			j++;
			stuffed[j] = BST[1];
			j++;

		}else if(buffer[i] == OCTETO2){

			stuffed[j] = BST[0];
			j++;
			stuffed[j] = BST[2];
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

unsigned char *getUnstuffedData(unsigned char *buffer, int length){

	int i, j, contador = 0, unstuffedLength;
	unsigned char *unstuffed;

	for(i = 0; i < length; i++){
		if(buffer[i] == BST[0]){
			if(buffer[i+1] == BST[1] || buffer[i+1] == BST[2])
				contador++;
		}
	}

	unstuffedLength = length - contador;

	unstuffed = (unsigned char*)malloc(unstuffedLength);

	for(i = 0, j = 0; i < length; i++, j++){
		if(buffer[i] == BST[0] && buffer[i+1] == BST[1]){
			unstuffed[j] = OCTETO1;
			i++;
		}else if(buffer[i] == BST[0] && buffer[i+1] == BST[2]){
			unstuffed[j] = OCTETO2;
			i++;
		}else
			unstuffed[j] = buffer[i];
	}
	return unstuffed;
}

int getUnstuffedLength(unsigned char *buffer, int length){
	
	int i, contador = 0, unstuffedLength;

	for(i = 0; i < length; i++){
		if(buffer[i] == BST[0]){
			if(buffer[i+1] == BST[1] || buffer[i+1] == BST[2])
				contador++;
		}
	}

	unstuffedLength = length - contador;
	return unstuffedLength;	
} 

// Cria uma trama de informacao
int llwrite(int fd, unsigned char* buffer, int length){

	//Trama de informacao
	unsigned char *frame, *stuffed;
	//int superVisionFrame;
	int frameLength, i, j, stuffedLength;
	
	unsigned char bcc2;

	bcc2 = getBCC2(buffer, length);
	
	stuffedLength = getStuffedLength(buffer, length);
	
	stuffed = getStuffedData(buffer, length);

	frameLength = stuffedLength + 6; //length dos dados mais F,A,C,BCC1,BCC2,F

	frame = (unsigned char *) malloc(frameLength);

	frame[0] = FLAG;
	frame[1] = AE;

	if(CFlag == 0)
		frame[2] = 0x00;
	else
		frame[2] = 0x40;

	frame[3] = frame[1]^frame[2];

	for(i = 4, j = 0; j < stuffedLength; i++, j++){
		frame[i] = stuffed[j];
	}

	frame[stuffedLength + 4] = bcc2;

	frame[stuffedLength + 5] = FLAG;
	
	//while(1){ attempts < getNumTransformations(app)

		if(write_frame(fd, frame, frameLength) == -1){
			printf("Error writting frame\n");
			return -1;
		}

		//alarm(getTimeOut(app));

		//if(receiveSupervisionFrame() == 1){
		// attempts = 0;
		//alarm(0);
		//	CFlag = (CFlag + 1) % 2;
		//	return frameLength;
		//}
	//}

	printf("Unstuffed: %x ", frame[stuffedLength + 4]);
	printf("\n");

	return frameLength;
	//return -1;
}


int sendFileInfo(char * fileName, int control) {
	struct stat st;
	unsigned int startLength;
	unsigned char * controlStart;
	off_t sizeOfFile;
	int i, j;


	fstat(getTargetDescriptor(app), &st);

	sizeOfFile = st.st_size;
	//printf("sizeOfFile: %x ", sizeOfFile);

	startLength = 7 + sizeof(off_t) + strlen(fileName);
	controlStart = (unsigned char *)malloc(sizeof(unsigned char)*startLength);

	controlStart[0] = control;
	controlStart[1] = SIZE_PARAMETER;
	controlStart[2] = sizeof(off_t);

	memcpy(&controlStart[3], &sizeOfFile, sizeof(off_t));

	controlStart[3+sizeof(off_t)] = NAME_PARAMETER;
	controlStart[4+sizeof(off_t)] = strlen(fileName);
	
	for(i = 5 + sizeof(off_t), j = 0; j < strlen(fileName); i++, j++)
		controlStart[i] = fileName[j];
	
	//Acabamos de fazer o Data packet de Start
	if(llwrite(getFileDescriptor(app), controlStart, startLength) < 0){
		printf("Error on llwrite: Start Packet\n");
		return 1;
	}
	return (int)sizeOfFile;
}

int sendFileData(int fileSize){

	int fileSizeAux, bytes = 0, bytesRead = 0, nSeq = 0;
	unsigned char *buffer, bufferFile[4];

	//int packetSent = 0;

	fileSizeAux = fileSize;
	buffer = (unsigned char*) malloc(8); //[C,N,L2,L1,P1...PK]

	while(bytes < fileSizeAux){

		bytesRead = read(getTargetDescriptor(app), bufferFile, 4);

		if(bytesRead < 0){
			printf("Error reading %s\n", getFileName(app));
			continue;
		}

		buffer[0] = DATA_C;
		buffer[1] = (unsigned char)nSeq%255; //Nao sei se e necessario mudar para char
		buffer[2] = 4/8;
		buffer[3] = 4%8;
		
		memcpy(buffer+4, bufferFile, bytesRead); //Data packet, comeca na posiçao 5 e vai buscar a informacao no bufferFile

		if(llwrite(getFileDescriptor(app), buffer, bytesRead + 4) < 0 ){
			printf("Error llwrite: Data Packet %d", nSeq);
			return -1;
		}
		memset(bufferFile, 0, 4);

		bytes += bytesRead;
		nSeq++;
/*
		int k;
		for(k = 0; k < (bytesRead+4); k++)
			printf(" %c ", buffer[k]);
		printf("\n");*/
	}

	close(getTargetDescriptor(app));
	return 0;
}

void sendFile() {

	int sizeFile;

	sizeFile = sendFileInfo(getFileName(app), START_C);//Trama de controlo START

	sendFileData(sizeFile);//Tramas de dados do ficheiro
	
	sizeFile = sendFileInfo(getFileName(app), END_C);//Trama de controlo END

	//Obter tamanho, nome do ficheiro e permissões e enviar
	//Depois comecar a enviar tramas dos bytes lidos no FICHEIRO
	//enviar pacote de controlo com END
}

int llread(unsigned char * buffer){

	unsigned char frame[200];		// Trama stuffed
	unsigned char stuffedData[200];		// Dados stuffed
	unsigned char *unstuffedData;		// Dados destuffed/originais
	unsigned char expectedBCC2;		// BCC2 que é esperado
	

	int i; // Assume o valor do tamanho da trama stuffed
	for(i = 0; i < 200; i++){
		
		read(getFileDescriptor(app), &frame[i], 1);

		if(i != 0 && frame[i] == FLAG)
			break;
	}

	int k;	// Assume o valor do tamanho da data stuffed
	for(k = 4; k < i-1; k++){

		stuffedData[k-4] = frame[k];
	}

	// Dar destuff
	int unstuffedLength = getUnstuffedLength(stuffedData, k-4);
	unstuffedData = (unsigned char *) malloc(unstuffedLength);
	unstuffedData = getUnstuffedData(stuffedData, k-4);


	buffer[0] = unstuffedData[0];
	expectedBCC2 = unstuffedData[0];
	int j;
	for(j = 1; j < unstuffedLength; j++){

		expectedBCC2 ^= unstuffedData[j];
		buffer[j] = unstuffedData[j];
	}

	printf("exp: %x    frame: %x\n", expectedBCC2, frame[i-1]);
	if(frame[i-1] != expectedBCC2){
		printf("Error in BCC2.\n");
		return -1;
	}

	if(frame[0] != FLAG){
		printf("Error in FLAG_1.\n");
		return -1;
	}

	if(frame[1] != AE){
		printf("Error in A.\n");
		return -1;
	}

	if((CFlag == 0 && frame[2] != 0) || (CFlag == 1 && frame[2] != 0x40)){
		printf("Error in C.\n");
		return -1;
	}

	if(frame[3] != (frame[1] ^ frame[2])){
		printf("Error in BCC1.\n");
		return -1;
	}

	if(frame[i] != FLAG){
		printf("Error in FLAG_2.\n");
		return -1;
	}

	return i-6;
}


int readDataPackets(){

	char * filename;
	unsigned char buffer[200];
	unsigned char dataPacket[200];
	int dataSize = 0;

	while(1){

		printf("\n");
		if((dataSize = llread(buffer)) == -1){

			if(CFlag == 0){
				//printf("sendSupervisionFrame(REJ0)\n");
				//sendSupervisionFrame(REJ0, getFileDescriptor(app));
				continue;
			} else{
				printf("sendSupervisionFrame(REJ1)\n");
				//sendSupervisionFrame(REJ1, getFileDescriptor(app));
				continue;
			}
		}
		
		if(buffer[0] == 1){ // C = 1  -> Pacote de dados

			//printf("Pacote de dados");
			//if(buffer[1] == 0xFF)
			//	return -1;

			int k = 256 * buffer[2] + buffer[3];

			int i;
			for(i = 0; i < k; i++){

				dataPacket[i] = buffer[i+4];
			}
			write_frame(getTargetDescriptor(app), dataPacket, 4);	
		} 
		else if(buffer[0] == 2 || buffer[0] == 3){ // C = 2 -> Pacote de controlo start  //  C = 3 -> Pacote de controlo end

			unsigned char l = 0;
			unsigned char lLast = l;
			unsigned char t = 0;
			long int fileSize;

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

				} else if(t == 1){	// Nome do ficheiro - Funciona

					filename = malloc(l + 1);
					int k;
					for(k = 0; k < l; k++){
	
						filename[k] = buffer[1 + i*(1+1+lLast) + 2 + k];
					}
					filename[k] = 0;

					setTargetDescriptor(app, open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777));

				} else{

					printf("T with unknown value\n");
					return -1;
				}
			}

			if(CFlag == 0){
				
				printf("sendSupervisionFrame(RR0)\n");;
				//sendSupervisionFrame(RR0, getFileDescriptor(app));
			}
			else{

				printf("sendSupervisionFrame(RR1)\n");
				//sendSupervisionFrame(RR1, getFileDescriptor(app));
			}
 			//CFlag = (CFlag + 1) % 2;

			if(buffer[0] == 3){
				printf("3 end\n");
				//sendSupervisionFrame(DISC, getFileDescriptor(app));
				break;
			}

		} else{

			printf("buffer[0] with unknown value\n");
			return -1;
		}
	}

	printf("Out of the while\n");
	return 0;
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

	sleep(1);
	resetPortConfiguration(app);

	close(getFileDescriptor(app));

	return 0;
}
