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

linkLayer * linkL;
applicationLayer * app;
int CFlag = 0;
unsigned int attempts = 0;

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
		CFlag = (CFlag + 1) % 2;
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

	printf("Started creating the supervision frame");

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

	printf("About to write the supervision frame");
	write(fd, buffer, 5);
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


// Cria uma trama de informacao
int llwrite(int fd, unsigned char* buffer, int length){

	//Trama de informacao
	unsigned char *frame;
	int superVisionFrame;
	int frameLength;

	frame = (unsigned char *) malloc(length + 6);
	frame[0] = FLAG;
	frame[1] = AE;
	if(CFlag == 0)
		frame[2] = 0x00;
	else
		frame[2] = 0x40;
	frame[3] = frame[1]^frame[2];
	char bcc2 = buffer[0];

	// BBC2
	int i = 4, j = 0;
	for(; j < length; i++, j++){
		frame[i] = buffer[j];

		if(j != 0)
			bcc2 = bcc2^buffer[j];
	}
	frame[length+4] = bcc2;
	//finished creating bcc2

	frame[length+5] = FLAG;
	// Checkpoint: frame = trama de informacao
	
//while(tentativas < numero maximo de tentativas){
	frameLength = length + 6; //length dos dados mais F,A,C,BCC1,BCC2,F
	if(write_frame(fd, frame, length+6) == -1){
		printf("Error writting frame\n");
		return -1;
	}

	superVisionFrame = receiveSupervisionFrame(fd);
	if(superVisionFrame != 0){
		attempts = 0;
		alarm(0);
		if(superVisionFrame == 1) //verifica se o frame é RR
			return frameLength;
	}

	alarm(0);
	
//}

	//printf("Connection timed out\n");
	return -1;
}

//Função responsável por ler o ficheiro a mandar e enviar por llwrite os pacotes de dados
//Nível aplicação
int readFile(char *fileName){

	int i, j;
	struct stat st; //estrutura que vai conter informacao do fichero
	int bytes = 0, bytesRead = 0; //Variável a utilizar para sabermos quantos bytes já foram enviados
	int fileSizeAux; //Variavel a utilizar nos pacotes de dados pois para START e END utilizamos off_t
	unsigned char *buffer; //buffer para data packets
	unsigned char bufferFile[252]; //usado para ler o ficheiro de 252 em 252
	int nSeq = 0; //Sequencia para os pacotes de dados.

	int fd = open(fileName, O_RDONLY);
	if(fd == -1){
		printf("Unable to open file %s", fileName);
		return 1;
	}

	fstat(fd, &st); //Poe valores na estrutura baseados no ficheiro
	off_t fileSize = st.st_size; //Dimensao do ficheiro

	//Packet de início, utiliza START_C
	/*
		C = 2; START_C
		T1 = 0;
		L1 = sizeof(off_t);
		V1 = fileSize;

		T2 = 1;
		L2 = sizeof(char);
		V2 = fileName;
	*/
	unsigned int startLength;
	startLength = 7 + sizeof(off_t) + (sizeof(fileSize) + strlen(fileName));
	unsigned char* controlStart = (unsigned char *)malloc(sizeof(char)*startLength);
	
	controlStart[0] = START_C;
	controlStart[1] = 0;
	controlStart[2] = sizeof(off_t);
	controlStart[3] = fileSize;

	controlStart[4] = 1;
	controlStart[5] = sizeof(char);
	
	for(i = 6, j = 0; j < strlen(fileName); i++, j++)
		controlStart[i] = fileName[j];
	
	//Acabamos de fazer o Data packet de Start

	if(llwrite(getFileDescriptor(app), controlStart, startLength) < 0){
		printf("Error on llwrite: Start Packet\n");
		return 1;
	}

	//PACKET DE DATA
	/*
		C = DATA_C;
		N = Numero de sequencia: packet 1, 2, 3...
		L2 = 252%256 
		L1 = 252/256
		P = campo de dados

		Utilizamos 252 em L1 e L2 pois vamos ler do ficheiro 252 de cada vez
	*/
	fileSizeAux = (int) fileSize;
	buffer = (unsigned char*) malloc(256); //[C,N,L2,L1,P1...PK]

	while(bytes < fileSizeAux){
		bytesRead = read(fd, bufferFile, 252);
		if(bytesRead < 0)
			printf("Error reading %s\n", fileName);

		buffer[0] = DATA_C;
		buffer[1] = nSeq; //Nao sei se e necessario mudar para char
		buffer[2] = 252/256;
		buffer[3] = 252%256;
		
		memcpy(buffer+4, bufferFile, bytesRead); //Data packet, comeca na posiçao 5 e vai buscar a informacao no bufferFile

		if(llwrite(getFileDescriptor(app), buffer, bytesRead + 4) < 0 ){
			printf("Error llwrite: Data Packet %d", nSeq);
			return -1;
		}

		bytes += bytesRead;
		nSeq++;
		printf("Packet successfuly sent\n");
	}
	close(fd);
	//Packet de fim, utiliza END_C
	/*
		C = 2; END_C
		T1 = 0;
		L1 = sizeof(off_t);
		V1 = fileSize;

		T2 = 1;
		L2 = sizeof(char);
		V2 = fileName;
	*/
	controlStart[0] = END_C;

	if(llwrite(getFileDescriptor(app), controlStart, startLength) < 0){
		printf("Error on llwrite: End Packet\n");
		return -1;
	}
	
	/*
		if(llclose(getFileDescriptor(app)) < 0){
			printf("Error llclose");
			return -1;
		}
	*/
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

		int size;
		if ((size = llread(getFileDescriptor(app), buffer)) == -1){

			
			if(CFlag == 0)
				sendSupervisionFrame(REJ0, getFileDescriptor(app));
			else
				sendSupervisionFrame(REJ1, getFileDescriptor(app));
		}
		

		if(buffer[0] == 1){
		
			/*if(buffer[1] == 0xFF)
				return -1;*/

			// Numero de bytes do campo de dados
			int k = 256 * buffer[2] + buffer[3];

			int i;
			for(i = 0; i < k; i++){

				dataPacket[i] = buffer[i];
				write(getFileDescriptor(app), &dataPacket[i], 1);
			}

		} else if(buffer[0] == 2 || buffer[0] == 3){

			int i;
			int k;
			unsigned char l = 0;
			unsigned char t = 0;
			unsigned char fileSize;

			for(i = 0; i < 2; i++){

				l = buffer[1 + i*(1+1+l) + 1];
				t = buffer[1 + i*(1+1+l)];

				if(t == 0){

					fileSize = 0x00;
					for(k = 0; k < l; k++){
						
						// Começa no ultimo byte 
						fileSize |= buffer[1 + i*(1+1+l) + 1 - k] << 8*k;
					}

				} else if(t == 1){

					filename = malloc(l + 1);
					for(k = 0; k < l; k++){
	
						filename[k] = buffer[1 + i*(1+1+l) + 2 + k];
					}
					filename[k] = 0;

					setTargetDescriptor(app, open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777));
				
				} else
					return -1;
			}

			if(CFlag == 0)
				sendSupervisionFrame(RR0, getFileDescriptor(app));
			else
				sendSupervisionFrame(RR1, getFileDescriptor(app));
			CFlag = (CFlag + 1) % 2;

			if(buffer[0] == 3){
				sendSupervisionFrame(DISC, getFileDescriptor(app));
				break;
			}
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

	if(getStatus(app) == TRANSMITTER) {
		sendFile();
	}

	else if(getStatus(app) == RECEIVER){
		//Checkar o que é que isto tem de fazer
	}
	else {
		perror("Wrong mode!\n");
	}

	sleep(1);
	resetPortConfiguration(app);

	close(getFileDescriptor(app));

	return 0;
}
