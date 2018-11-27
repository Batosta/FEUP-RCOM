#ifndef URL_H
#define URL_H

#define PARAM_SIZE 256
#define h_addr h_addr_list[0] //The first address in h_addr_list.

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
  unsigned int mode;
} url;

url *getUrl();

void setUser(url *u, char *username);

void setPassword(url *u, char *password);

void setHost(url *u, char *host);

void setPort(url *u, char *portStr);

void setPath(url *u, char *path);

void setMode(url *u, int mode);

void setIp(url *u, char *path);

int getMode(url *u);

unsigned char *getHost(url *u);

int validURL(char *insertedURL);

int parseUserAuthUrl(url *link, char *inserted);

int parseAnonimousAuth(url *link, char *inserted);

int parseURL(url *link, char *inserted);

int extractIp(url *link);

#endif