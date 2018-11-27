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

void setUser(url *u, char *username);

void setPassword(url *u, char *password);

void setHost(url *u, char *host);

void setPort(url *u, char *portStr);

void setPath(url *u, char *path);

int validURL(char *insertedURL);

int parseUserAuthUrl(url *link, char *inserted);

int parseAnonimousAuth(url *link, char *inserted);

int parseURL(int Mode, url *link, char *inserted);

#endif