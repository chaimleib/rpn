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

/* fngetuntil() writes chars from f to s until the first of the following:
 *   * a complete line or lines matching tok (up until tokn) are found, or
 *   * n chars are written to s (including the \0), or
 *   * we reach the EOF.
 *
 * The terminating tok is not written to s. The number of chars written to s,
 * including the trailing \0 or \n\0, overwrites n; 0 chars written indicates
 * an error. The cursor of f is left at the first char of the line following
 * tok, or if tok was not found, wherever we stopped writing chars to s.
 *
 * We return 1 if tok was found, else 2 if EOF was found, else 3 if we got cut
 * off by the n-char limit, and 0 if there is an error. If we return nonzero, s
 * is guaranteed to contain a \0.
 *
 * isLineStart should be 0 if f's cursor does not point to the beginning of a
 * line. Otherwise, isLineStart should be non-zero. This is useful if n is not
 * large enough to fit a complete line, and you are calling fngetuntil() in a
 * loop.
 *
 * tok is allowed to contain a \n for multiline matches.
 *
 * s must not be smaller than n chars. n must be greater than 0, to hold at
 * least the \0 byte.
 *
 * tok must not be smaller than tokn chars. tokn must be greater than 0, to
 * hold at least the \0 byte, indicating an empty line.
 */
char fngetuntil(char *s, size_t *n, char *tok, size_t tokn, char isLineStart, FILE *f) {
  fpos_t tokStart; // Once we begin a match, we stop writing chars to s. If the
                   // match does not complete, we use this fpos_t to go back
                   // and grab the chars we weren't sure about.
  size_t i = 0; // index within tok
  const size_t n1 = *n-1;
  const size_t tokn1 = tokn-1;
  int c; // fgetc(f)
  char tokc; // tok[i]
  char isEOT; // end of tok
  char isEOF;
  size_t sEnd = 0; // index of \0 in s
  char retval;

  if (*n <= 0 || tokn <= 0) {
    return 0;
  }

  while (1) {
    c = fgetc(f);
    isEOT = i >= tokn1 || !(tokc = tok[i]);
    isEOF = c == EOF;

    retval = 0;
    // end of tok and either EOL or EOF
    if (isEOT && (c == '\n' || isEOF)) {
      retval = 1;
    // not in match
    } else if (!i) {
      if (isEOF) {
        retval = 2;
      // only start match if isLineStart
      } else if (isLineStart && c == tok[0]) {
        i = 1;
        fgetpos(f, &tokStart);
      // reached n chars, including \0
      } else if (sEnd >= n1) {
        ungetc(c, f);
        retval = 3;
      // not matching tok right now
      } else {
        s[sEnd++] = c;
      }
    // end of tok (and not EOL or EOF, above), or mismatch, or EOF inside
    // match-in-progress
    } else if (isEOT || c != tokc || isEOF) {
      // reset match until next isLineStart
      i = 0;
      fsetpos(f, &tokStart);
      // fake value that is not \n, so that we don't set isLineStart and start
      // a new match. Unlike fskipuntil(), we're not jumping to nextTryMark,
      // since we need to iterate over missed chars to write them to s.
      c = 'x';
    // not first of tok and match continues
    } else {
      i++;
    }
    if (retval) {
      s[sEnd] = '\0';
      *n = sEnd+1;
      return retval;
    }

    isLineStart = c == '\n';
  }
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
int testPrint(char *buf, size_t n, FILE* f) {
  size_t slurpLen;
  if (!f) {
    fprintf(stderr, "error: failed to read current dir\n");
    return 1;
  }
  while (1) {
    slurpLen = getline1(buf, n, f);
    if (!slurpLen) {
      fprintf(stderr, "error: slurpLen==0\n");
      break;
    } else if (slurpLen == EOF) {
      printf("EOF\n");
      break;
    }

    if (buf[slurpLen-1] == '\n') {
      printf("%2zu: [%s]", slurpLen, buf);
    } else {
      printf("%2zu: [%s]-\\n\n", slurpLen, buf);
    }
  }
  return 0;
}

#define INPUT_TOK "### INPUT"
#define EXPECT_TOK "### EXPECT"
char parseTest(
  char* buf,
  size_t* n,
  char* fpath
) {
  FILE *f;
  char status, isLineStart;
  const size_t cap = *n;
  size_t slurpLen;

  printf("Reading %s...\n", fpath);
  f = fopen(fpath, "r");
  if (!f) {
    fprintf(stderr, "error: failed to read current dir\n");
    return 1;
  }

  fnskipuntil(INPUT_TOK, sizeof(INPUT_TOK), f);
  buf[0] = '\0';
  isLineStart = 1;
  *n = 0;
  do {
    slurpLen = cap;
    status = fngetuntil(
        buf, &slurpLen, EXPECT_TOK, sizeof(EXPECT_TOK), isLineStart, f);
    *n += slurpLen-1; // exclude \0
    printf("buf[%zu], status %d: [%s]\n", slurpLen, status, buf);
    isLineStart = buf[slurpLen-1] == '\n';
  } while (status==3 && slurpLen);
  ++*n; // for \0
  testPrint(buf, cap, f);
  fclose(f);
  return 0;
}

int main(int argc, char** argv) {
  if (argc<2) {
    usage();
    return 1;
  }

  char buf[32];
  size_t len = sizeof(buf);
  parseTest(buf, &len, argv[1]);
  return 0;
}
