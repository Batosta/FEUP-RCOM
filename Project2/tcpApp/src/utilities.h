#ifndef UTILITIES_H
#define UTILITIES_H

#define clear() printf("\033[H\033[J")

void printUsage();

int findOcorrenceIndex(char *str, char toFind, int startSearch);

int write_frame(int fd, char *frame, unsigned int length);

int openFile(char *path);

char *stripFileName(char *path);

void progressBar(int fileSize, int sentBytes);

double what_time_is_it();
#endif