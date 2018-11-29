#ifndef UTILITIES_H
#define UTILITIES_H

#define clear() printf("\033[H\033[J")

void printUsage();

int findOcorrenceIndex(char *str, char toFind, int startSearch);

#endif