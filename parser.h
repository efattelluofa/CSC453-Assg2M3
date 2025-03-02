
/*
 * File: parser.h
 * Author: Edward Fattell
 * Purpose: External facing function and variable declaations, and typedefs
 */

#include "scanner.h"

#ifndef __PARSER_H__
#define __PARSER_H__

extern int get_token();
extern int parse();

extern int chk_decl_flag;
extern int curr_tok;
extern int linecnt;
extern char *lexeme;
extern int lval;

typedef enum { VAR, FUNC } DeclType;

struct symboltab {
  char *name;
  Token type;
  DeclType dtype;
  int argcnt;
  struct symboltab *next;
} typedef symboltab;

#endif
