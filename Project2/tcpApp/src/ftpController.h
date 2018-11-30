#ifndef FTPCONTROLLER_H
#define FTPCONTROLLER_H

#include "url.h"

#define FRAME_LENGTH 1024

typedef struct ftpController
{
  int controlFd;
  int dataFd;
} ftpController;

ftpController *getController();

void setFtpControlFileDescriptor(ftpController *x, int fd);

struct sockaddr_in *getServerAdress(url *u);

int startConnection(url *u);

int ftpExpectCommand(ftpController *connection, int expectation);

#endif