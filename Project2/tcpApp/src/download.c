#include <stdio.h>
#include <stdlib.h>
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

  connection = getController();

  if ((socketFd = startConnection(link)) == FAIL)
  {
    return FAIL;
  }

  setFtpControlFileDescriptor(connection, socketFd);

  if (ftpExpectCommand(connection, 220) == FAIL)
  {
    perror("HOST didn't send the right code");

    return FAIL;
  }

  return 0;
}