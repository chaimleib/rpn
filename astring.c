#include "astring.h"

#define MAX_ALLOC (16*1024)

// Ensures that provided cap(acity) of p(ointer) is able to contain
// typeSize*size bytes. If it can't, use realloc to grow the storage for p and
// return the new cap. If realloc returns NULL, set *p to NULL and return 0.
size_t checkRealloc(void **p, size_t cap, size_t typeSize, size_t size) {
  int shouldRealloc;
  while ((shouldRealloc = size >= cap)) {
    cap += (cap > 8192) ? 8192 : cap;
    if (cap > MAX_ALLOC) {
      fprintf(stderr, "error: exceeded MAX_ALLOC\n");
      return 0;
    }
  }
  if (shouldRealloc) {
    *p = realloc(*p, cap*typeSize);
  }
  return *p ? cap : 0;
}

void *nomem(char *note) {
  fprintf(stderr, "error: %snomem\n", note);
  return NULL;
}

// note: strlen does not count final \0, while sizeof and snprintf do
size_t astrcat(char **p, size_t *pCap, size_t *pLen, char *q, size_t qLen) {
  if (!(*pCap = checkRealloc(
      (void **)p, *pCap, sizeof(char), *pLen+qLen+1))) {
    nomem("astrcat(): ");
    return 0;
  }
  memcpy(&(*p)[*pLen], q, qLen);
  *pLen += qLen;
  return *pCap;
}

void odstr(char *s) {
  size_t i = 0;
  char c;
  char rep[8];
  do {
    c = s[i];
    if (isalnum(c) || ispunct(c)) {
      sprintf(rep, "%c", c);
    } else if (c == ' ') {
      strcpy(rep, "' '");
    } else if (c == '\n') {
      strcpy(rep, "\\n");
    } else if (c == '\t') {
      strcpy(rep, "\\t");
    } else if (c == '\0') {
      strcpy(rep, "\\0");
    } else {
      sprintf(rep, "0x%02x", c);
    }
    fprintf(stderr, "%ld:%s ", i, rep);
    ++i;
  } while (c);
  fprintf(stderr, "\n");
}

void _smartfree(int n, ...) {
  va_list ptrs;
  va_start(ptrs, n);
  for (int i = 0; i < n; i++) {
    void *ptr = va_arg(ptrs, void *);
    if (ptr != NULL) free(ptr);
  }
}
