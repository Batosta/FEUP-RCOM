#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "url.h"
#include "utilities.h"
#include "ftpController.h"

int main(int argc, char *argv[])
{
  url *link;
  ftpController *connection;
  int validation, socketFd;

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

  setDataFileDescriptor(connection);

  if (requestFile(connection, link) == FAIL)
  {
    perror("Request file from server failed!\n");
    return FAIL;
  }

  printInfo(link);

  if (downloadFile(connection, link) == FAIL)
  {
    perror("Failed to download file.\n");
    return FAIL;
  }

  if (logout(connection) == FAIL)
  {
    perror("Failed to logout.\n");
    return FAIL;
  }

  return SUCCESS;
}
