/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7E
#define AE 0x03
#define AR 0x01
#define SET 0x03
#define DISC 0x09
#define UA 0x07

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res, step = 0;
    struct termios oldtio,newtio;
    unsigned char buf[1];
    unsigned char SETUP [5];

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
    printf ("%x\n",buf);

    while (step != 5) {       /* loop for input */
      res = read (fd,buf,1);   
      printf("Received: %x STEP: %d res: %d\n ", buf[0], step, res);
      switch (step)
	{
	case 0:
	  if(buf[0] == FLAG) {
		printf("FLAG received.\n");
		step++;
		}
	  break;
	case 1:
	  if(buf[0] == AE) {
		printf("A received.\n");
		step++;
		}
	  else if(buf[0] == FLAG) {
		printf("FLAG received, same state.");
		}
	  else {
		printf("Received: %x. Back to initial state.", buf[0]);
		step = 0;
		}
	  break;
	case 2:
	  if(buf[0] == SET) {
		printf("C received.\n");
		step++;
		}
	  else if(buf[0] == FLAG) {
		printf("FLAG received, back to flag state.");
		step = 1;
		}
 	  else {
		printf("Received: %x. Back to initial state.", buf[0]);
		step = 0;
		}
	  break;
	case 3: 
	  if(buf[0] == (AE^SET)) {
		printf("BCC OK. Awaiting FLAG.\n");
		step++;
		}
	  else {
		printf("BCC incorrect. Back to initial state!\n");
		step = 0;
		}
	  break;
	case 4:
	  if(buf[0] == FLAG) {
		printf("FLAG received.\n");
		step++;
		}
	  else {
		printf("Received: %x. Back to initial state.", buf[0]);
		step = 0;
		}
	break;
	default:
		break;
	}

 	memset(buf, 0, 1);
    }

	printf("SET RECEIVED : MUST SEND UA\n");

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
