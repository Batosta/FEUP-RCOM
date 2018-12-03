#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "url.h"
#include "utilities.h"
#include "ftpController.h"

int flag = 0, tries = 1, maxTries = 1;

void timeOutWarning()
{
  flag = 0;
  printf("\nAtempting to connect %d/%d\n", tries, TIMEOUT_MAX_TRIES);
  return;
}

int main(int argc, char *argv[])
{
  url *link;
  ftpController *connection;
  int validation, socketFd;

  (void)signal(SIGALRM, timeOutWarning);

  if (argc != 2)
  {
    printUsage();
    return FAIL;
  }

  validation = validURL(argv[1]);

  if (validation == FAIL)
  {
    printUsage();
    printf("\n invalid URL.\n");
    return FAIL;
  }

  link = getUrl();

  setMode(link, validation);

  if (parseURL(link, argv[1]) == FAIL)
  {
    printf("Error parsing URL.\n");
    return FAIL;
  }

  if (extractIp(link) == FAIL)
  {
    printf("Error getting IP from URL.\n");
    return FAIL;
  }

  printInfo(link);

  //clear();

  connection = getController();

  if ((socketFd = startConnection(getIpAdress(link), getPort(link))) == FAIL)
  {
    return FAIL;
  }

  setFtpControlFileDescriptor(connection, socketFd);

  if (ftpExpectCommand(connection, SERVICE_READY_NEW_USER) == FAIL)
  {
    perror("HOST didn't send the right code");

    return FAIL;
  }

  if (login(connection, link) == FAIL)
  {
    perror("Failed to authenticate\n");
    return FAIL;
  }

  printf("Authenticated!\n");

  if (enterPassiveMode(connection) == FAIL)
  {
    perror("Could not enter passive mode!\n");
    return FAIL;
  }

  printf("Passive mode enabled!\n");

  setDataFileDescriptor(connection);

  printf("Data scoket file descriptor set!\n");

  if (requestFile(connection, link) == FAIL)
  {
    perror("Request file from server failed!\n");
    return FAIL;
  }

  printf("File requested!\n");

  printInfo(link);

  if (downloadFile(connection, link) == FAIL)
  {
    perror("Failed to download file.\n");
    return FAIL;
  }

  printf("File dowloaded!\n");

  if (logout(connection) == FAIL)
  {
    perror("Failed to logout.\n");
    return FAIL;
  }

  printf("Logout sucessfull!\n");

  return SUCCESS;
}
