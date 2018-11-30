#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "url.h"
#include "utilities.h"

const char *REuserAuth = "ftp://([([A-Za-z0-9])*:([A-Za-z0-9])*@])*([A-Za-z0-9.~-])+:([0-9])+/([[A-Za-z0-9/~._-])+";

const char *REAnonimousAuth = "ftp://([A-Za-z0-9.~-])+/([[A-Za-z0-9/~._-])+";

url *getUrl()
{
  url *x;

  x = (url *)malloc(sizeof(url));

  memset(x->host, 0, PARAM_SIZE);
  memset(x->ip, 0, PARAM_SIZE);
  memset(x->password, 0, PARAM_SIZE);
  memset(x->path, 0, PARAM_SIZE);
  memset(x->user, 0, PARAM_SIZE);

  x->port = 0;

  return x;
}

void setUser(url *u, char *username)
{
  memcpy(u->user, username, strlen(username));
}

void setPassword(url *u, char *password)
{
  memcpy(u->password, password, strlen(password));
}

void setHost(url *u, char *host)
{
  memcpy(u->host, host, strlen(host));
}

void setPort(url *u, char *portStr)
{
  int port = atoi(portStr);

  u->port = port;
}

void setPath(url *u, char *path)
{
  memcpy(u->path, path, strlen(path));
}

void setMode(url *u, int mode)
{
  u->mode = mode;
}

void setIp(url *u, char *ip)
{
  memcpy(u->ip, ip, strlen(ip));
}

int getMode(url *u)
{
  return u->mode;
}

unsigned char *getIpAdress(url *u)
{
  return u->ip;
}

int getPort(url *u)
{
  return u->port;
}

unsigned char *getHost(url *u)
{
  return u->host;
}

int validURL(char *insertedURL)
{
  regex_t *detectUser, *detectAnonimous;

  size_t matchSize = strlen(insertedURL);

  regmatch_t match[matchSize];

  int userAuthTest, anonimousAuthTest;

  detectUser = (regex_t *)malloc(sizeof(regex_t));

  detectAnonimous = (regex_t *)malloc(sizeof(regex_t));

  if (regcomp(detectUser, REuserAuth, REG_EXTENDED) != 0)
  {
    perror("User auth regular expression is wrong.");

    return FAIL;
  }

  if (regcomp(detectAnonimous, REAnonimousAuth, REG_EXTENDED) != 0)
  {
    perror("Anonimous regular expression is wrong.");

    return FAIL;
  }

  userAuthTest = regexec(detectUser, insertedURL, matchSize, match, REG_EXTENDED);

  anonimousAuthTest = regexec(detectAnonimous, insertedURL, matchSize, match, REG_EXTENDED);

  if (userAuthTest == 0)
  {
    return USERAUTH;
  }
  else if (anonimousAuthTest == 0)
  {
    return ANONIMOUS;
  }
  else
  {
    return FAIL;
  }
}

int parseUserAuthUrl(url *link, char *inserted)
{
  char *username, *password, *host, *portStr, *path;

  int usernameStart, usernameEnd, pwEnd, hostEnd, portEnd, length;

  length = strlen(inserted);

  if ((usernameStart = findOcorrenceIndex(inserted, '[', 0) + 1) == -1)
  {
    return FAIL;
  }
  if ((usernameEnd = findOcorrenceIndex(inserted, ':', usernameStart)) == -1)
  {
    return FAIL;
  }
  username = (char *)malloc(usernameEnd - usernameStart);
  memcpy(username, inserted + usernameStart, usernameEnd - usernameStart);

  if ((pwEnd = findOcorrenceIndex(inserted, '@', usernameEnd + 1)) == -1)
  {
    return FAIL;
  }
  password = (char *)malloc(pwEnd - (usernameEnd + 1));
  memcpy(password, inserted + usernameEnd + 1, pwEnd - (usernameEnd + 1));

  if ((hostEnd = findOcorrenceIndex(inserted, ':', pwEnd + 1)) == -1)
  {
    return FAIL;
  }
  host = (char *)malloc(hostEnd - (pwEnd + 2));
  memcpy(host, inserted + pwEnd + 2, hostEnd - (pwEnd + 2));

  if ((portEnd = findOcorrenceIndex(inserted, '/', hostEnd + 1)) == -1)
  {
    return FAIL;
  }
  portStr = (char *)malloc(portEnd - (hostEnd + 1));
  memcpy(portStr, inserted + hostEnd + 1, portEnd - (hostEnd + 1));

  path = (char *)malloc(length - portEnd);
  memcpy(path, inserted + portEnd, length - portEnd);

  setUser(link, username);
  setPassword(link, password);
  setHost(link, host);
  setPort(link, portStr);
  setPath(link, path);

  free(username);
  free(password);
  free(host);
  free(portStr);
  free(path);

  return 0;
}

int parseAnonimousAuth(url *link, char *inserted)
{
  int hostStart, hostEnd, length;
  char *host, *path;

  length = strlen(inserted);

  if ((hostStart = findOcorrenceIndex(inserted, '/', 0)) == -1)
  {
    return FAIL;
  }

  hostStart += 2; // /->/-> <path>

  if ((hostEnd = findOcorrenceIndex(inserted, '/', hostStart)) == -1)
  {
    return FAIL;
  }

  host = (char *)malloc(hostEnd - hostStart);
  memcpy(host, inserted + hostStart, hostEnd - hostStart);

  path = (char *)malloc(length - hostEnd);
  memcpy(path, inserted + hostEnd, length - hostEnd);

  setHost(link, host);
  setPath(link, path);

  free(host);
  free(path);

  return 0;
}

int parseURL(url *link, char *inserted)
{
  switch (getMode(link))
  {
  case USERAUTH:
    return parseUserAuthUrl(link, inserted);
  case ANONIMOUS:
    return parseAnonimousAuth(link, inserted);
  default:
    return FAIL;
  }
}

int extractIp(url *link)
{
  const char *host;
  char *ip;
  struct hostent *hostInfo;

  host = (char *)getHost(link);

  if ((hostInfo = gethostbyname(host)) == NULL)
  {
    perror("gethostbyname");
    return FAIL;
  }

  ip = inet_ntoa(*((struct in_addr *)hostInfo->h_addr));

  setIp(link, ip);

  return 0;
}