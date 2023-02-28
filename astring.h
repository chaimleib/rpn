#ifndef _ASTRING_H
#define _ASTRING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // isalnum, ispunct

size_t checkRealloc(void **p, size_t cap, size_t typeSize, size_t size);

void *nomem(char *note);

size_t astrcat(char **p, size_t *pCap, size_t *pLen, char *q, size_t qLen);

void odstr(char *s);
#endif
