#include "ftpController.h"
#include "url.h"
#include "utilities.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>

int flag = 0, tries = 1, maxTries = 1;

void timeOutWarning()
{
  flag = 1;
  printf("\nAtempting to connect %d/%d\n", tries, TIMEOUT_MAX_TRIES);
}

ftpController *getController()
{
  ftpController *x = malloc(sizeof(ftpController));

  return x;
}

void setFtpControlFileDescriptor(ftpController *x, int fd)
{
  x->controlFd = fd;
}

int setPassiveIpAndPort(ftpController *x, int *ipInfo)
{
  if ((sprintf(x->passiveIp, "%d.%d.%d.%d", ipInfo[0], ipInfo[1], ipInfo[2], ipInfo[3])) < 0)
  {
    printf("Failed to set passive ip address.\n");
    return FAIL;
  }

  x->passivePort = ipInfo[4] * 256 + ipInfo[5];

  return SUCCESS;
}

int setDataFileDescriptor(ftpController *x)
{
  if ((x->dataFd = startConnection(x->passiveIp, x->passivePort)) < 0)
  {
    printf("Cannot connect data socket");
    return FAIL;
  }

  return SUCCESS;
}

struct sockaddr_in *getServerAdress(const char *ipAdress, int port)
{
  struct sockaddr_in *serverAddr;

  serverAddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

  bzero((char *)serverAddr, sizeof(serverAddr));
  serverAddr->sin_family = AF_INET;
  serverAddr->sin_addr.s_addr = inet_addr(ipAdress);
  serverAddr->sin_port = htons(port);

  return serverAddr;
}

int startConnection(const char *ip, int port)
{
  int fd;
  struct sockaddr_in *serverAddr;

  serverAddr = getServerAdress(ip, port);

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

int ftpSendCommand(ftpController *connection, char *command)
{
  if (write(connection->controlFd, command, strlen(command)) != strlen(command))
  {
    return FAIL;
  }

  //printf("COMMAND SENT: %s-> %ld\n", command, strlen(command));

  return SUCCESS;
}

int ftpExpectCommand(ftpController *connection, int expectation)
{
  int code = -1;
  int response;
  char frame[FRAME_LENGTH];
  char *codeAux = (char *)malloc(3);

  do
  {
    if (flag == 0)
    {
      alarm(1);
      tries++;
      flag = 1;
    }

    memset(frame, 0, FRAME_LENGTH);
    memset(codeAux, 0, 3);
    read(connection->controlFd, frame, FRAME_LENGTH);
    memcpy(codeAux, frame, 3);
    code = atoi(codeAux);

    if (tries == TIMEOUT_MAX_TRIES)
    {
      printf("TIMED OUT!");
      break;
    }

    //printf("%s \n", frame);

    //printf("%d =?= %d\n", code, expectation);

  } while (code != expectation && frame[3] != ' ');

  printf("RESPONSE: %d\n", code);

  response = code == expectation ? SUCCESS : FAIL;

  free(codeAux);

  alarm(0);
  tries = 1;
  flag = 0;

  return response;
}

int login(ftpController *connection, url *link)
{
  char *userCommand, *passwordCommand, *user, *password;

  user = (char *)link->user;

  password = (char *)link->password;

  userCommand = (char *)malloc(sizeof(user) + strlen("USER \r\n"));

  sprintf(userCommand, "USER %s\n", user);

  (void)signal(SIGALRM, timeOutWarning);

  if (ftpSendCommand(connection, userCommand) == FAIL)
  {
    printf("ERROR Sending user command\n");
    free(userCommand);
    return FAIL;
  }

  free(userCommand);
  //printf("User command sent!\n");

  if (ftpExpectCommand(connection, SERVICE_NEED_PASSWORD) == FAIL)
  {
    printf("ERROR Wrong response from server\n");
    return FAIL;
  }

  passwordCommand = (char *)malloc(sizeof(password) + strlen("PASS \r\n"));

  sprintf(passwordCommand, "PASS %s\r\n", password);

  if (ftpSendCommand(connection, passwordCommand) == FAIL)
  {
    printf("ERROR Sending user command\n");
    free(passwordCommand);
    return FAIL;
  }

  free(passwordCommand);

  if (ftpExpectCommand(connection, SERVICE_USER_LOGGEDIN) == FAIL)
  {
    printf("ERROR Wrong response from server\n");
    return FAIL;
  }

  return SUCCESS;
}

char *retriveMessageFromServer(ftpController *connection, int expectation)
{
  char frame[FRAME_LENGTH];
  char *message, *codeAux;
  int code = -1;

  codeAux = (char *)malloc(3);

  do
  {
    if (flag == 0)
    {
      alarm(1);
      tries++;
      flag = 1;
    }

    memset(frame, 0, FRAME_LENGTH);
    memset(codeAux, 0, 3);
    read(connection->controlFd, frame, FRAME_LENGTH);

    //printf("FRAME -> %s\n", frame);

    memcpy(codeAux, frame, 3);
    code = atoi(codeAux);

    if (tries == TIMEOUT_MAX_TRIES)
    {
      printf("TIMED OUT!");
      break;
    }

  } while (code != expectation && frame[3] != ' ');

  alarm(0);
  tries = 1;
  flag = 0;

  free(codeAux);

  if (code != expectation)
  {
    return NULL;
  }

  message = (char *)malloc(strlen(frame) * sizeof(char));

  memcpy(message, frame, strlen(frame));

  return message;
}

int *parsePassiveIp(char *serverMessage)
{
  int *ipInfo = (int *)malloc(6 * sizeof(int));

  if ((sscanf(serverMessage, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &ipInfo[0], &ipInfo[1], &ipInfo[2], &ipInfo[3], &ipInfo[4], &ipInfo[5])) < 0)
  {
    printf("Couldn't parse passive IP info.\n");
    return NULL;
  }

  return ipInfo;
}

int enterPassiveMode(ftpController *connection)
{
  char *passiveCommand, *serverAnswer;
  int *ipInfo;

  passiveCommand = (char *)malloc(strlen("PASV \n") * sizeof(char));

  memcpy(passiveCommand, "PASV \n", strlen("PASV \n"));

  if (ftpSendCommand(connection, passiveCommand) == FAIL)
  {
    printf("ERROR: can't send passive command!\n");
    free(passiveCommand);
    return FAIL;
  }

  free(passiveCommand);

  serverAnswer = retriveMessageFromServer(connection, SERVICE_ENTERING_PASSIVE_MODE);

  if (serverAnswer == NULL)
  {
    printf("SERVER didn't answer");
    return FAIL;
  }

  ipInfo = parsePassiveIp(serverAnswer);

  setPassiveIpAndPort(connection, ipInfo);

  printf("PASSIVE IP: %s PORT: %d\n", connection->passiveIp, connection->passivePort);

  return SUCCESS;
}