#ifndef FTPCONTROLLER_H
#define FTPCONTROLLER_H

#include "url.h"

#define FRAME_LENGTH 1024

#define SERVICE_READY_NEW_USER 220
#define SERVICE_NEED_PASSWORD 331
#define SERVICE_USER_LOGGEDIN 230

typedef struct ftpController
{
  int controlFd;
  int dataFd;
} ftpController;

ftpController *getController();

void setFtpControlFileDescriptor(ftpController *x, int fd);

struct sockaddr_in *getServerAdress(url *u);

int startConnection(url *u);

int ftpSendCommand(ftpController *connection, char *command);

int ftpExpectCommand(ftpController *connection, int expectation);

int login(ftpController *connection, url *link);

#endif