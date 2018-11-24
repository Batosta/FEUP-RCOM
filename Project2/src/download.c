#include <stdio.h>

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

  return 0;
}