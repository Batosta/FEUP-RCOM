#include "utilities.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

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

//retorna descritor do ficheiro
int openFile(char *path)
{
  int f = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0777);

  if (f < 0)
  {
    perror(path);
    return -1;
  }

  return f;
}

void stripFileName(char *path, char *filename)
{
  int index = 0, indexAux = 0, length = strlen(path);

  do
  {
    index = indexAux;
    indexAux = findOcorrenceIndex(path, '/', index + 1);
  } while (indexAux != -1);

  memset(filename, 0, 256);

  memcpy(filename, ".", 1);

  memcpy(filename + 1, path + index, length - index + 1);

  filename[length - index + 1] = '\0';
}

void progressBar(int fileSize, int sentBytes)
{
  int progress = (int)(1.0 * sentBytes / fileSize * 100);
  int length = progress / 2;
  int i;

  printf("\rPROGRESS: |");
  for (i = 0; i < length - 1; i++)
  {
    printf("=");
  }

  printf(">");

  for (i = 0; i < 50 - length; i++)
  {
    printf(" ");
  }

  printf("| %d%% (%d / %d)", progress, sentBytes, fileSize);

  fflush(stdout);
}

double what_time_is_it()
{
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  return now.tv_sec + now.tv_nsec * 1e-9;
}
