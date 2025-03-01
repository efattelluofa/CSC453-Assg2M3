/*
 * Author: Edward Fattell
 * Date: 4 February 2025
 * Purpose: scanner is a basic implementation of a scanner for the G0
 *          subset of C that mainly implements the function get_char
 *          to be used to get tokens from stdin to be used with a parser
 *          at a later date
 *
 *          NOTE: This implementation puts characters from unclosed comments
 *                back onto stdin as discussed in class with Dr. Debray
 */
#include "scanner.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *lexeme;
int lval;
int BUFSZ = sizeof(char) * 1024;
int linecnt = 1;

struct tokenNode {
  Token toke;
  char *pattern;
  struct tokenNode *next;
} typedef tokenNode;

tokenNode *tokenTable = NULL;

int isIDChar(char ch);

void buildTable();

void freetable();

tokenNode *guessToken(char *buf, int idx, tokenNode *prevGuess);

int rm_cmt(int buflen);

/*
 * get_token() - returns a token enum value corresponding to the lexeme it finds
 * on stdin
 */
int get_token() {
  atexit(freetable);
  free(lexeme);
  lexeme = NULL;
  lval = 0;
  char ch;
  char buf[BUFSZ];
  // fill buffer with nulls (used for strlen later in guessToken)
  memset(buf, '\0', sizeof(buf));

  int idx = 0;
  char *ptr = buf;

  // Pointer to token node in table
  tokenNode *tokenGuess = tokenTable;

  while ((ch = getchar()) != EOF) {
    // printf("Char: %c\n", ch);
    if (tokenGuess) {
    } else {
    }
    int removedSpaced = 0;
    ungetc(ch, stdin);
    removedSpaced = rm_cmt(strlen(buf));

    if (removedSpaced && strlen(buf) > 0) {
      if (tokenGuess == NULL) {
        continue;
      }
      if (strlen(buf) == strlen(tokenGuess->pattern) ||
          !strcmp(tokenGuess->pattern, "")) {
        lexeme = strdup(buf);
        return tokenGuess->toke;
      }
      tokenGuess = tokenGuess->next;
      tokenGuess = guessToken(buf, idx, tokenGuess);
      // printf("guessing new token\n");

      lexeme = strdup(buf);
      return tokenGuess->toke;
    }

    char newCh = getchar();
    ch = newCh;
    // printf("New Char: %c\n", ch);
    if (strlen(buf) == 0) {
      removedSpaced = 0;
    }
    if (!tokenGuess && strlen(buf) == 1) {
      ungetc(ch, stdin);
      lexeme = strdup(buf);
      // printf("returning undef\n");
      return UNDEF;
    }
    *ptr = ch;
    tokenGuess = guessToken(buf, idx, tokenGuess);

    if (*ptr == '\0') {
      lexeme = strdup(buf);
      if (tokenGuess == NULL) {
        continue;
      }
      return tokenGuess->toke;
    }

    *ptr = ch;

    ptr++;
    idx++;
  }
  if (strlen(buf)) {
    if (tokenGuess) {
      if (!strcmp(buf, tokenGuess->pattern))
        return tokenGuess->toke;
    }
    // return UNDEF;
  }
  return -1;
}

/*
 * isIDChar() - helper function to determine if a char is a candidate for
 *              an ID lexeme
 *            char ch is the char to check
 *            returns boolean value of candidacy
 */
int isIDChar(char ch) {
  if (isalnum(ch) || ch == '_') {
    return 1;
  }
  return 0;
}

/*
 * guessToken() is the main helper gunction for get_token
 *              it infers the lexeme and token of a buffer
 *              based on the "token table"
 */
tokenNode *guessToken(char *buf, int idx, tokenNode *prevGuess) {
  if (!tokenTable) {
    buildTable();
  }
  if (prevGuess == NULL) {
    prevGuess = tokenTable;
  }

  tokenNode *nextGuess = prevGuess;
  tokenNode *matchedToken = NULL;

  // iterate through all lexemes in the table until one is found that matches
  // the buf
  int restart = 0;
  for (nextGuess = prevGuess; 1; nextGuess = nextGuess->next) {

    int i = 0;
    if (!restart) {
      i = strlen(buf) - 1;
    }

    if (nextGuess == NULL) {
      if (strlen(buf) == 1) {
        return NULL;
      }
      nextGuess = prevGuess->next;
      ungetc(buf[strlen(buf) - 1], stdin);
      buf[strlen(buf) - 1] = '\0';
      i = 0;

      if (strlen(buf) < 1) {
        return NULL;
      } else if (strlen(buf) == 1) {
        return prevGuess;
      }
    }

    for (i = i; i < strlen(buf); i++) {
      // if current guess is int constant, than must check using isdigit
      int wasIntCon = 0;
      if (nextGuess->toke == INTCON) {
        if (isdigit(buf[i])) {
          lval = lval * 10 + atoi(buf + idx);
          matchedToken = nextGuess;
          return matchedToken;
        } else if (strlen(buf) > 1 && i > 0) {
          // if it is not a digit, it might be an ID
          wasIntCon = 1;
        }
      }

      // if current guess is ID, than must check using isIDChar
      if (nextGuess->toke == ID || wasIntCon) {
        if (isIDChar(buf[i])) {
          if (wasIntCon) {
            nextGuess = nextGuess->next;
          }
          if (strlen(buf) == i + 1) {
            matchedToken = nextGuess;
            for (tokenNode *cur = tokenTable; cur != NULL; cur = cur->next) {
              if (!strcmp(buf, cur->pattern)) {
                return cur;
              }
            }
            return matchedToken;
          } else {
            continue;
          }

        } else if (wasIntCon) {
          ungetc(buf[strlen(buf) - 1], stdin);
          buf[strlen(buf) - 1] = '\0';
          for (tokenNode *cur = tokenTable; cur != NULL; cur = cur->next) {
            if (!strcmp(buf, cur->pattern)) {
              return cur;
            }
          }
          return nextGuess;

        } else if (strlen(buf) - 1 >= 1 && i > 0) {
          // printf("unget ID branch ungetting %c \n", buf[strlen(buf) - 1]);
          ungetc(buf[strlen(buf) - 1], stdin);
          buf[strlen(buf) - 1] = '\0';
          for (tokenNode *cur = tokenTable; cur != NULL; cur = cur->next) {
            if (!strcmp(buf, cur->pattern)) {
              return cur;
            }
          }
          return nextGuess;
        }
      }

      // For patterns that are not ID or INTCON
      if (nextGuess->pattern[i] == buf[i] &&
          strlen(buf) <= strlen(nextGuess->pattern)) {

        matchedToken = nextGuess;
        if (i == strlen(nextGuess->pattern) - 1 && strlen(buf) - 1 == i) {
          return matchedToken;
        }

        if (i == strlen(buf) - 1) {
          return matchedToken;
        }
      } else {
        restart = 1;
        break;
      }
    }
  }

  return matchedToken;
}

/*
 * buildTable() - dynamically allocated and bulds table of tokens and lexemes
 *                to be used by the guessToken function
 */
void buildTable() {
  tokenNode *tbPtr = tokenTable;

  tbPtr = malloc(sizeof(tokenNode));
  tokenTable = tbPtr;
  tbPtr->toke = kwINT;
  tbPtr->pattern = strdup("int");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = kwIF;
  tbPtr->pattern = strdup("if");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = kwELSE;
  tbPtr->pattern = strdup("else");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = kwWHILE;
  tbPtr->pattern = strdup("while");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = kwRETURN;
  tbPtr->pattern = strdup("return");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opEQ;
  tbPtr->pattern = strdup("==");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opNE;
  tbPtr->pattern = strdup("!=");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opGE;
  tbPtr->pattern = strdup(">=");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opLE;
  tbPtr->pattern = strdup("<=");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opAND;
  tbPtr->pattern = strdup("&&");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opOR;
  tbPtr->pattern = strdup("||");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opGT;
  tbPtr->pattern = strdup(">");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opLT;
  tbPtr->pattern = strdup("<");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = LPAREN;
  tbPtr->pattern = strdup("(");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = RPAREN;
  tbPtr->pattern = strdup(")");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = LBRACE;
  tbPtr->pattern = strdup("{");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = RBRACE;
  tbPtr->pattern = strdup("}");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = COMMA;
  tbPtr->pattern = strdup(",");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = SEMI;
  tbPtr->pattern = strdup(";");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opASSG;
  tbPtr->pattern = strdup("=");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opADD;
  tbPtr->pattern = strdup("+");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opSUB;
  tbPtr->pattern = strdup("-");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opMUL;
  tbPtr->pattern = strdup("*");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = opDIV;
  tbPtr->pattern = strdup("/");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = INTCON;
  tbPtr->pattern = strdup("");

  tbPtr->next = malloc(sizeof(tokenNode));
  tbPtr = tbPtr->next;

  tbPtr->toke = ID;
  tbPtr->pattern = strdup("");

  tbPtr->next = NULL;
}

/*
 * charNode is the struct to be used to store comments that were popped
 * off stdin so they can be returned to stdin in the proper order
 */
struct charNode {
  char ch;
  struct charNode *next;
} typedef charStack;

/*
 * freeCharStack - helper function for rm_cmt that frees the char queue
 */
void freeCharStack(charStack *hd) {
  charStack *next = NULL;

  while (hd != NULL) {
    next = hd->next;
    free(hd);
    hd = next;
  }
}

/*
 * rm_cmt() - removes multiline comments and spaces from stdin using getchar
 */
int rm_cmt(int buflen) {
  int retVal = 0;

  char chSpace;
  while ((chSpace = getchar()) != EOF) {
    if (!isspace(chSpace) && chSpace != 10) {
      ungetc(chSpace, stdin);
      break;
    }
    if (buflen > 0) {
      ungetc(chSpace, stdin);
      return 1;
    }
    if (chSpace == '\n') {
      linecnt++;
    }
    retVal++;
  }

  if (chSpace == EOF) {
    ungetc(-1, stdin);
    return -1;
  }

  charStack *hd = malloc(sizeof(charStack));

  hd->ch = getchar();
  hd->next = NULL;

  if (hd->ch != '/') {
    ungetc(hd->ch, stdin);
    free(hd);
    return retVal;
  }
  charStack *newHd = malloc(sizeof(charStack));

  newHd->ch = getchar();
  if (newHd->ch != '*') {
    ungetc(newHd->ch, stdin);
    ungetc(hd->ch, stdin);
    free(newHd);
    free(hd);
    return retVal;
  }

  newHd->next = hd;
  hd = newHd;

  int wasStar = 0;
  while (1) {
    newHd = malloc(sizeof(charStack));

    if ((newHd->ch = getchar()) == EOF) {
      free(newHd);
      break;
    }

    newHd->next = hd;
    hd = newHd;

    if (wasStar) {
      if (hd->ch == '/') {
        retVal++;
        freeCharStack(hd);
        // must recurse to account for additional spaces or comments following
        // the one just found
        return retVal + rm_cmt(buflen);
      }
    }

    if (hd->ch == '\n') {
      linecnt++;
    }
    if (hd->ch == '*') {
      wasStar = 1;
    } else {
      wasStar = 0;
    }
  }

  // comment pattern not found, so must put chars next on stack
  charStack *next = NULL;

  while (hd != NULL) {
    next = hd->next;
    if (hd->ch == '\n') {
      linecnt--;
    }
    ungetc(hd->ch, stdin);
    free(hd);
    hd = next;
  }
  return retVal;
}

/*
 * freetable() - frees the token table as well as the lexeme buffer
 */
void freetable() {
  tokenNode *hd = tokenTable;
  tokenNode *next = NULL;

  while (hd != NULL) {
    next = hd->next;
    free(hd->pattern);
    free(hd);
    hd = next;
  }

  free(lexeme);
  tokenTable = NULL;
  lexeme = NULL;
}
