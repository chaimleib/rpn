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

/* fgetuntil() reads from the current position in the provided file and writes
 * bytes to s until it encounters either a line equal to tok or the EOF,
 * whichever comes first. To be matched, the token string must be followed, in
 * the file data, by either a newline or the EOF.
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
size_t fgetuntil(char *s, size_t n, char *tok, size_t toksize, FILE *f) {
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

/* fskipuntil() moves the cursor of f to either the first char after the first
 * complete line(s) equal to tok, or to the EOF, whichever comes first. If tok
 * is found, returns 1, else returns 0. A line is allowed to end with the EOF
 * or a \n.
 *
 * Assumes that f is already cued to the beginning of a line.
 *
 * tok is allowed to contain \n, in which case this token matcher will
 * backtrack in the file as needed (though never prior to the initial cursor
 * position) to ensure every line start it encounters is checked for a match.
 */
int fskipuntil(char *tok, FILE *f) {
  fpos_t bookmark; // in case of \n in tok, for retrying match from second line
                   // encountered during partial match
  char isBookmark = 0;
  size_t i = 0; // index within tok
  char isLineStart = 1;
  int c = '\0';
  char tokc; // tok[i]

  while (1) {
    c = fgetc(f);
    tokc = tok[i];
    if (!tokc && (c == '\n' || c == EOF)) { // end of tok and EOL
      return 1;
    }
    if (c == EOF) {
      return 0;
    }
    if (!tokc || c != tokc) { // end of tok and not EOL or EOF, or mismatch
                              // reset match until next isLineStart
      i = 0;
      if (isBookmark) {
        fsetpos(f, &bookmark);
        isBookmark = 0;
      }
    } else if (i) { // not first of tok and match continues
      i++;
    } else if (isLineStart && c == tok[0]) { // only start match if isLineStart
      i = 1;
    }
    isLineStart = c == '\n';
    if (isLineStart && i && !bookmark) {
      fgetpos(f, &bookmark);
      isBookmark = 1;
    }
  }
}

/* fnskipuntil() moves the cursor of f to either the first char after the first
 * complete line(s) equal to tok up to n-1 chars where index n-1 is treated as
 * \0, or to the EOF, whichever comes first. If tok is found, returns 1, else
 * returns 0. A line is allowed to end with the EOF or a \n.
 *
 * Assumes that f is already cued to the beginning of a line.
 *
 * tok is allowed to contain \n, in which case this token matcher will
 * backtrack in the file as needed (though never prior to the initial cursor
 * position) to ensure every line start it encounters is checked for a match.
 */
int fnskipuntil(char *tok, size_t n, FILE *f) {
  fpos_t bookmark; // in case of \n in tok, for retrying match from second line
                   // encountered during partial match
  char isBookmark = 0;
  size_t i = 0; // index within tok
  size_t n1 = n-1;
  char isLineStart = 1;
  int c = '\0';
  char tokc; // tok[i]
  char isEOT; // end of tok

  while (1) {
    c = fgetc(f);
    isEOT = i >= n1 || !(tokc = tok[i]);
    if (isEOT && (c == '\n' || c == EOF)) { // end of tok and EOL
      return 1;
    }
    if (c == EOF) {
      return 0;
    }
    if (isEOT || c != tokc) { // end of tok, and not EOL or EOF, or mismatch
                              // reset match until next isLineStart
      i = 0;
      if (isBookmark) {
        fsetpos(f, &bookmark);
        isBookmark = 0;
      }
    } else if (i) { // not first of tok and match continues
      i++;
    } else if (isLineStart && c == tok[0]) { // only start match if isLineStart
      i = 1;
    }
    isLineStart = c == '\n';
    if (isLineStart && i && !bookmark) {
      fgetpos(f, &bookmark);
      isBookmark = 1;
    }
  }
}

/* testPrint() uses getline1 to retrieve lines, labeling each slurp into buf
 * with however many bytes arrived, and whether \n, EOF, or the sizeof(buf)
 * terminated the slurp.
 */
int testPrint(FILE* f) {
  char buf[256];
  size_t n;

  if (f == 0) {
    fprintf(stderr, "error: failed to read current dir\n");
    return 1;
  }
  while (1) {
    n = getline1((char *)buf, sizeof(buf), f);
    if (!n) {
      fprintf(stderr, "error: n==0\n");
      break;
    } else if (n == EOF) {
      printf("EOF\n");
      break;
    }

    if (buf[n-1] == '\n') {
      printf("%2zu: %s", n, buf);
    } else {
      printf("%2zu: %s\n[-\\n]\n", n, buf);
    }
  }
  return 0;
}

#define INPUT_TOK "### INPUT"
#define EXPECT_TOK "### EXPECT"
char parseTest(
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

  fnskipuntil(INPUT_TOK, sizeof(INPUT_TOK), f);
  input[0] = '\0';
  *inputLen = fgetuntil(input, *inputLen, EXPECT_TOK, sizeof(EXPECT_TOK), f);
  printf("input[%zu]: [%s]\n", *inputLen, input);
  testPrint(f);
  fclose(f);
  /* return !*inputLen; */
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
