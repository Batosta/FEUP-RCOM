#include "utilities.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void printUsage()
{
  clear();

  printf("Usage:");
  printf("\n\t./download ftp://<user>:<password>@<host>:<port>/<url-path>");
  printf("\nOR");
  printf("\n\t./download ftp://<host>/<url-path>\n");
}

int findOcorrenceIndex(char *str, char toFind, int startSearch)
{
  int strSize = strlen(str);
  int i;

  if (startSearch < 0 || startSearch > strSize)
  {
    startSearch = strSize;
  }

  for (i = startSearch + 1; i < strSize; i++)
  {
    if (str[i] == toFind)
    {
      return i;
    }
  }

  return -1;
}

/*
Função que tenta dar write dos frames.
*/
int write_frame(int fd, char *frame, unsigned int length)
{
  int aux, total = 0;

  while (total < length)
  {
    aux = write(fd, frame, length);

    if (aux <= 0)
      return -1;

    total += aux;
  }

  return total;
}