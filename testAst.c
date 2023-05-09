#include <stdio.h>
#include <string.h>

void usage() {
  puts("usage: testAst file.ast");
  puts("");
  puts("Compares actual AST produced by parsing the INPUT section with"
      " the EXPECT section of the .ast file.");
  puts("");
  puts("Example .ast file:");
  puts("");
  puts("### INPUT");
  puts("1");
  puts("### EXPECT");
  puts("NUM 1.00");
}

/* getline1() is like fgets, but helps avoid extra O(n) string iteration by
 * returning the length of the string written to s. Unlike getline(), getline1()
 * does not try to use malloc(), the n argument is not a pointer, and if n-1 is
 * exceeded we immediately return n rather than attempt to read to the end of
 * the line.
 *
 * If any chars are read, we return the lesser of the index of the \0 byte, or
 * n (if more than n-1 chars are in the line).
 *
 * If no chars were read and the end of the file has been reached, we return
 * EOF.
 *
 * We write characters from f to s until n-1 characters, or until and including
 * \n, or until the character before EOF, whichever comes first, and then write
 * a null byte. If more than n-1 characters are encountered, we return n.
 *
 * If n is positive, s is guaranteed to contain a null byte. If n is zero or
 * negative, we return 0.
 */
size_t getline1(char *s, size_t n, FILE *f) {
  int c;
  size_t i, n1;
  if (n<=0)
    return 0;
  n1 = n-1;
  for (i=0; i<n; i++) {
    c = fgetc(f);
    if (c==EOF) {
      s[i] = '\0';
      return i?i:EOF;
    }
    if (i<n1) {
      s[i] = (char)c;
      if (c == '\n') {
        s[++i] = '\0';
        return i;
      }
    } else {
      ungetc(c, f);
      s[i] = '\0';
      return i;
    }
  }
  return 0;
}

/* getuntil() reads from the current position in the provided file and writes
 * the subsequent data to s until it encounters either a line equal to tok or
 * the EOF, whichever comes first. The token string may be followed by either a
 * newline in the file data or the EOF; it will still be considered a line.
 *
 * If n (the size of s) is not large enough to hold all the data, n-1 bytes
 * will be written followed by a \0, and n will be the return value.
 * Otherwise, the index of the \0 byte will be returned.
 *
 * n must be at least large enough to contain the token, even though the token
 * is excluded from s.
 *
 * If any error occurs, returns 0. Otherwise, s is guaranteed to contain a \0
 * byte.
 */
size_t getuntil(char *s, size_t n, char *tok, size_t toksize, FILE *f) {
  size_t i, lineLen;
  int lineInProgress;

  if (n < toksize) {
    fprintf(stderr, "error: getuntil(\"%s\"): n was %zu, which is smaller than toksize, %zu\n",
        tok, n, toksize);
    return 0;
  }

  s[0] = '\0';
  i = 0; // index of \0
  lineInProgress = 0;
  for(;;) {
    lineLen = getline1(&s[i], n-i, f);
    if (lineLen == EOF) {
      return i;
    } else if (lineLen == 0) {
      fprintf(stderr, "error: args to getline1(&s[%zu], %zu, FILE*)\n",
        i, n-i);
      return 0;
    }

    if (n-i > lineLen) { // finished a line
      if (!lineInProgress && !strcmp(&s[i], tok)) { // found tok
        s[i] = '\0';
        break;
      } else {
        i += lineLen;
      }
    } else { // truncating to sizeof(s)
      return n;
    }

    lineInProgress = s[i-1] != '\n';
  }
  return i;
}

int testPrint(char* fpath) {
  FILE *f;
  char buf[256];
  size_t n;

  f = fopen(fpath, "r");
  if (f == 0) {
    fprintf(stderr, "error: failed to read current dir\n");
    return 1;
  }
  while ( 0<(n=getline1((char *)buf, sizeof(buf), f)) ) {
    if (buf[n-1] == '\n') {
      printf("%2zu: %s", n, buf);
    } else {
      printf("%2zu: %s\n[-\\n]\n", n, buf);
    }
  }
  return 0;
}

#define INPUT_TOK "### INPUT\n"
#define EXPECT_TOK "### EXPECT\n"
#define INPUTLEN_MIN (sizeof(INPUT_TOK) > sizeof(EXPECT_TOK) ? sizeof(INPUT_TOK) : sizeof(EXPECT_TOK))
int parseTest(
  char* input,
  size_t* inputLen,
  char* fpath
) {
  FILE *f;
  /* modes:
   * 0: looking for "### INPUT"
   * 1: reading "### INPUT"
   * 2: discarding remainder of ### INPUT
   * end: f is at line after "### EXPECT" or EOF
   */
  size_t n, lineLen;
  int lineInProgress, mode;

  printf("Reading %s...\n", fpath);
  f = fopen(fpath, "r");
  if (!f) {
    fprintf(stderr, "error: failed to read current dir\n");
    return 1;
  }

  input[0] = '\0';
  n = 0; // index of \0
  lineInProgress = 0;
  mode = 0;
  for(;;) {
    lineLen = getline1(&input[n], *inputLen-n, f);
    if (lineLen == EOF) {
      fprintf(stderr, "error: unexpected EOF\n");
      fclose(f);
      return 1;
    } else if (lineLen == 0) {
      fprintf(stderr, "error: args to getline1(&input[%zu], %zu, <%s>)\n",
        n, *inputLen-n, fpath);
      fclose(f);
      return 1;
    }

    if (mode == 0) { // discard until INPUT token
      if (!lineInProgress && !strcmp(&input[n], INPUT_TOK)) {
        mode = 1;
      }
    } else if (mode == 1) { // write to input
      if (*inputLen-n > lineLen) {
        if (!lineInProgress && !strcmp(&input[n], EXPECT_TOK)) {
          input[n] = '\0';
          break;
        } else {
          n += lineLen;
        }
      } else {
        n = *inputLen-1;
        mode = 2;
      }
    } else if (mode == 2) { // discard unstoreable INPUT
      if (!lineInProgress && !strcmp(&input[n], EXPECT_TOK)) {
        break;
      }
    }

    lineInProgress = input[lineLen-1] != '\n';
  }
  /* printf("\nFound EXPECT\n"); */
  /* printf("\nFile cursor at EXPECT data\n"); */
  *inputLen = n;
  printf("input[%zu]: [%s]\n", *inputLen, input);
  fclose(f);
  return 0;
}

int main(int argc, char** argv) {
  if (argc<2) {
    usage();
    return 1;
  }

  char buf[256]; // large enough to hold line contents plus "### EXPECT\n"
  size_t len = sizeof(buf);
  parseTest(buf, &len, argv[1]);
  return 0;
}
