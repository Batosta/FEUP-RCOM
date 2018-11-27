#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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

int parseURL(int Mode, url *link, char *inserted)
{
  char *username, *password, *host, *portStr, *path;

  int usernameStart, usernameEnd, pwEnd, hostEnd, portEnd, length;

  length = strlen(inserted);

  if (Mode == USERAUTH)
  {
    usernameStart = findOcorrenceIndex(inserted, '[', 0) + 1;
    usernameEnd = findOcorrenceIndex(inserted, ':', usernameStart);
    username = (char *)malloc(usernameEnd - usernameStart);
    memcpy(username, inserted + usernameStart, usernameEnd - usernameStart);

    pwEnd = findOcorrenceIndex(inserted, '@', usernameEnd + 1);
    password = (char *)malloc(pwEnd - (usernameEnd + 1));
    memcpy(password, inserted + usernameEnd + 1, pwEnd - (usernameEnd + 1));

    hostEnd = findOcorrenceIndex(inserted, ':', pwEnd + 1);
    host = (char *)malloc(hostEnd - (pwEnd + 2));
    memcpy(host, inserted + pwEnd + 2, hostEnd - (pwEnd + 2));

    portEnd = findOcorrenceIndex(inserted, '/', hostEnd + 1);
    portStr = (char *)malloc(portEnd - (hostEnd + 1));
    memcpy(portStr, inserted + hostEnd + 1, portEnd - (hostEnd + 1));

    path = (char *)malloc(length - (portEnd + 1));
    memcpy(path, inserted + portEnd + 1, length - (portEnd + 1));

    printf("\nUsername: %s \n", username);
    printf("Password: %s \n", password);
    printf("Host: %s \n", host);
    printf("Port: %s \n", portStr);
    printf("Path: %s \n", path);

    return 0;
  }
  else if (Mode == ANONIMOUS)
  {
    return 0;
  }
  else
  {
    return -1;
  }
}