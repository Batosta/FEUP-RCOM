#include "ftpController.h"
#include "url.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>

ftpController *getController()
{
  ftpController *x = malloc(sizeof(ftpController));

  return x;
}

void setFtpControlFileDescriptor(ftpController *x, int fd)
{
  x->controlFd = fd;
}

struct sockaddr_in *getServerAdress(url *u)
{
  struct sockaddr_in *serverAddr;

  serverAddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

  bzero((char *)serverAddr, sizeof(serverAddr));
  serverAddr->sin_family = AF_INET;
  serverAddr->sin_addr.s_addr = inet_addr((const char *)getIpAdress(u));
  serverAddr->sin_port = htons(getPort(u));

  return serverAddr;
}

int startConnection(url *u)
{
  int fd;
  struct sockaddr_in *serverAddr;

  serverAddr = getServerAdress(u);

  fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0)
  {
    perror("Failed to open socket!");
    return FAIL;
  }

  if (connect(fd, (struct sockaddr *)serverAddr, sizeof(struct sockaddr_in)) < 0)
  {
    perror("Failed to connect to server");
    return FAIL;
  }

  free(serverAddr);

  return fd;
}

int ftpExpectCommand(ftpController *connection, int expectation)
{
  int code = -1;
  int response;
  char frame[FRAME_LENGTH];
  char *codeAux = (char *)malloc(3);

  do
  {
    memset(frame, 0, FRAME_LENGTH);
    memset(codeAux, 0, 3);
    read(connection->controlFd, frame, FRAME_LENGTH);
    memcpy(codeAux, frame, 3);
    code = atoi(codeAux);
  } while (code <= 0);

  response = code == expectation ? SUCCESS : FAIL;

  free(codeAux);

  return response;
}