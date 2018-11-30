#ifndef FTPCONTROLLER_H
#define FTPCONTROLLER_H

#include "url.h"

typedef struct ftpController
{
  int controlFd;
  int dataFd;
} ftpController;

ftpController *getController();

void setFtpControlFileDescriptor(ftpController *x, int fd);

struct sockaddr_in *getServerAdress(url *u);

int startConnection(url *u);

#endif