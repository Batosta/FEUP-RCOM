#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "url.h"

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

void parseURL(int Mode, url *link, char *inserted)
{
}