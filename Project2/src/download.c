#include <stdio.h>
#include "url.h"

#define clear() printf("\033[H\033[J")

void printUsage()
{
  clear();

  printf("Usage:");
  printf("\n\t./download ftp://<user>:<password>@<host>:<port>/<url-path>");
  printf("\nOR");
  printf("\n\t./download ftp://<host>/<url-path>\n");
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printUsage();
    return 0;
  }

  int validation = validURL(argv[1]);

  if (validation == FAIL)
  {
    printUsage();
    printf("\n invalid URL.\n");
    return 0;
  }

  url *link = getUrl();

  parseURL(validation, link, argv[1]);

  return 0;
}