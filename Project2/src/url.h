#ifndef URL_H
#define URL_H

#define PARAM_SIZE 256

#define USERAUTH 1
#define ANONIMOUS 0
#define FAIL -1

typedef struct url
{
  unsigned char user[PARAM_SIZE];
  unsigned char password[PARAM_SIZE];
  unsigned char host[PARAM_SIZE];
  unsigned char ip[PARAM_SIZE];
  unsigned char path[PARAM_SIZE];
  unsigned int port;
} url;

url *getUrl();

int validURL(char *insertedURL);

int parseURL(int Mode, url *link, char *inserted);

#endif