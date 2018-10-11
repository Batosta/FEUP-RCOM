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

int flag=1, conta=1;

void atende()                  
{
	/*printf("alarme # %d\n", conta);
	flag=1;*/
	conta++;
}

void printUsage() {
  printf("Usage:\tnserial SerialPort mode<transmitter/receiver>\n\tex: nserial /dev/ttyS1 transmitter\n");
  exit(1);
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
}

int openSerialPort(char* path) {
  int f = open(path, O_RDWR | O_NOCTTY );

  if(f < 0) {
    perror(path);
    exit(-1);
  }

  return f;
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
  newtio->c_cc[VMIN] = 1;   /* blocking read until 5 chars received */
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
  int res;
  statemachine *controller = newStateMachine();

  while(getMachineState(controller) != FINAL_STATE) {
    memset(buf, 0, 1);
    res = read(path,buf,1);
    printf("Received: %x  res: %d\n", buf[0], res);
    interpretSignal(controller, buf[0]);
  }

  return TRUE;
}

int sendSetMessage(int path) {
  unsigned char setMessage[5];
  int i = 0;
  int writtenBytes = 0;

  	setMessage[0] = FLAG;
	setMessage[1] = AE;
	setMessage[2] = SET;	// If this was the receptor SET[2] would be 0x01
	setMessage[3] = setMessage[1]^setMessage[2];
	setMessage[4] = FLAG;

    writtenBytes = write(path, setMessage, 5);
  

  	if(writtenBytes != 5) {
    	return FALSE;
  	}

  	return TRUE;
}

int llopen(int path, int mode) {

  switch(mode) {
    case RECEIVER:
      if(waitForData(path) == TRUE)
		printf("Data received \n");
      sendSetMessage(path);
      break;
    case TRASNMITTER:
	  while(conta < 4){
	  	sendSetMessage(path);
		alarm(3);
printf("CONTA: %d\n", conta);
		if(waitForData(path) == TRUE){
			conta = 5;
			alarm(0);
		}
printf("CONTA: %d\n", conta);
	  }
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

int main(int argc, char** argv) {

  int fd, c, res, mode;
  struct termios newtio;

  signal(SIGINT, sigint_handler);
  (void) signal(SIGALRM, atende);

  analyseArgs(argc, argv);

  fd = openSerialPort(argv[1]);

  path = fd;

  getOldAttributes(fd, &oldtio);

  serialPortSetup(fd, &newtio);

  if(strcmp(argv[2], "transmitter") == 0) {
    mode = TRASNMITTER;
  } else {
    mode = RECEIVER;
  }

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
