/*
 * Author: Edward Fattell
 * File: parser.c
 * Purpose: Parser for G0, and eventually C-- compiler
 */
#include "scanner.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int get_token();
extern int parse();

extern int chk_decl_flag;
extern int curr_tok;
extern int linecnt;
extern char *lexeme;

int curr_tok;

void match(Token expected);

// Grammar Rules
void prog();
void var_decl();
void decl_or_func(); // TODO: Where is ID list handled
void type();
void func_defn();
int opt_formals();
int formals();
void opt_var_decls();
void opt_stmt_list();
void stmt();
void if_stmt();
void while_stmt();
void return_stmt();
void assg_stmt();
void fn_call(char *id);
int opt_expr_list(int expected_argcnt);
int expr_list(int expected_argcnt);
void bool_exp();
void arith_exp();
void relop();
void check_var(char *lexeme);
int check_arg_count(char *lexemeLoc);
void linepexit(Token t, char *lexeme, char *str);

enum decltype { VAR, FUNC } typedef decltype;

enum scope { GLOBAL, LOCAL, EITHER } typedef scopetype;

scopetype curscope = GLOBAL;

struct symboltab {
  char *name;
  Token type;
  decltype dtype;
  int argcnt;
  struct symboltab *next;
} typedef symboltab;

symboltab *getentry(char *id, scopetype scope);

int oldline = 0;

void freeSymTab(symboltab *tabin);
void freeTabs(void);

symboltab *globl = NULL;
symboltab *local = NULL;

char *token_name[] = {
    "UNDEF",  "ID",    "INTCON", "LPAREN", "RPAREN", "LBRACE",  "RBRACE",
    "COMMA",  "SEMI",  "kwINT",  "kwIF",   "kwELSE", "kwWHILE", "kwRETURN",
    "opASSG", "opADD", "opSUB",  "opMUL",  "opDIV",  "opEQ",    "opNE",
    "opGT",   "opGE",  "opLT",   "opLE",   "opAND",  "opOR",    "opNOT",
};

void createEntry(char *lexeme) {
  /*
  if (!strcmp(lexeme, "teehee1")) {
    printf("creating a teehee1 entry in scope %d\n", curscope);
  }
  */
  // check if in table first
  symboltab **curtab = NULL;
  if (curscope == GLOBAL) {
    curtab = &globl;
  } else {
    curtab = &local;
  }

  if (getentry(lexeme, curscope)) {
    linepexit(curr_tok, lexeme, "symbol previously defined.");
  }

  symboltab *newHd = malloc(sizeof(symboltab));
  newHd->dtype = VAR;
  newHd->argcnt = 0;
  newHd->name = strdup(lexeme);
  newHd->next = *curtab;
  *curtab = newHd;
}

void createFuncEntry(char *lexeme, int argcnt) {
  /*
  if (!strcmp(lexeme, "teehee1")) {
    printf("creating a func teehee1 entry in scope %d\n", curscope);
  }
  */
  // check if in table first
  symboltab **curtab = &local;
  if (curscope == GLOBAL) {
    curtab = &globl;
  }

  if (getentry(lexeme, curscope)) {
    linepexit(curr_tok, lexeme, "symbol previously defined.");
  }

  symboltab *newHd = malloc(sizeof(symboltab));
  newHd->dtype = FUNC;
  newHd->argcnt = argcnt;
  newHd->name = strdup(lexeme);
  newHd->next = *curtab;
  *curtab = newHd;
}

int parse() {
  atexit(freeTabs);
  curr_tok = get_token();
  prog();
  match(EOF);
  return 0;
}

void match(Token expected) {
  if (curr_tok == expected) {
    curr_tok = get_token();
  } else {
    char msg[1024];
    sprintf(msg, "match error, expected %s", token_name[expected]);
    linepexit(curr_tok, lexeme, msg);
  }
}

void prog() {
  while (curr_tok == kwINT) {
    match(kwINT);
    decl_or_func();
  }

  // iterate to simulate recursion
  if (curr_tok == EOF) {
    // Epsilon
  } else {
    linepexit(curr_tok, lexeme, "expected EOF");
  }
}

void func_defn() {
  match(ID);
  match(LPAREN);
  opt_formals();
  // printf("curr_tok after formals %s", token_name[curr_tok]);
  match(RPAREN);
  match(LBRACE);
  opt_var_decls();
  opt_stmt_list();
  match(RBRACE);
  // printf("after func defn\n");
}

void id_list_rest() {
  while (curr_tok == COMMA) {
    // printf("at id list comma\n");
    match(COMMA);
    // printf("at id list ID\n");
    if (curr_tok == ID) {
      createEntry(lexeme);
    }
    match(ID);
  }
}

void id_list() {
  if (curr_tok == ID && chk_decl_flag) {
    // printf("create entry at id_list\n");
    createEntry(lexeme);
  }
  match(ID);
  if (curr_tok != COMMA)
    return;
  // match(COMMA);
  // printf("Before id list rest\n");
  id_list_rest();
}

int formals() {
  int formalcnt = 0;

  if (curr_tok == kwINT) {
    type();
    createEntry(lexeme);
    match(ID);
    formalcnt++;
  }

  while (curr_tok == COMMA) {
    match(COMMA);

    type();
    if (curr_tok == ID)
      createEntry(lexeme);
    match(ID);
    formalcnt++;
  }

  return formalcnt;
}

void decl_or_func() {
  char funcName[1024];
  strcpy(funcName, lexeme);
  match(ID);
  // printf("decl or func after ID tok: %d\n", curr_tok);
  if (curr_tok == SEMI) {
    createEntry(funcName);
    match(SEMI);

  } else if (curr_tok == COMMA) {

    createEntry(funcName);
    // must be decl
    id_list_rest();
    match(SEMI);

  } else {

    // must be func, match the rest
    match(LPAREN);
    curscope = LOCAL;

    int argcnt = formals();

    curscope = GLOBAL;
    // printf("creating new func %s with %d args\n", funcName, argcnt);
    createFuncEntry(funcName, argcnt);
    curscope = LOCAL;

    match(RPAREN);
    match(LBRACE);
    opt_var_decls();
    // printf("after var decls\n");
    opt_stmt_list();
    // printf("curr_tok after stmt list %s\n", token_name[curr_tok]);
    match(RBRACE);

    freeSymTab(local);
    local = NULL;
    curscope = GLOBAL;
  }
}

int opt_formals() {
  if (curr_tok == kwINT) {
    return formals();
  }
  return 0;
}

void var_decl() {
  type();
  id_list();
  match(SEMI);
}

void opt_var_decls() {
  if (curr_tok == EOF) {
    return;
  }

  while (curr_tok == kwINT) {
    var_decl();
  }
}

void opt_stmt_list() {
  // iterate while the curr_tok is in the first set of stmt
  while (curr_tok == ID || curr_tok == kwWHILE || curr_tok == kwIF ||
         curr_tok == kwRETURN || curr_tok == LBRACE || curr_tok == SEMI) {
    // printf("Calling stmt\n");
    stmt();
  }
  // printf("after stmt list\n");
}

void stmt() {
  // printf("In sgtmt lexeme: %s\n", lexeme);
  char id[1024];
  switch (curr_tok) {
  case (ID):
    // printf("In fn call or assg\n");
    if (curr_tok == ID && strlen(lexeme)) {
      strcpy(id, lexeme);
    }
    match(ID);
    // Could be either assignment or fn call; leftfactor
    if (curr_tok == opASSG) {
      // printf("In op assign\n");
      check_var(id);
      match(opASSG);
      arith_exp();
      match(SEMI);
      return;
    }
    // match the rest of fn call
    fn_call(id);

    match(SEMI);
    break;
  case (kwWHILE):
    // printf("In while block\n");
    while_stmt();
    break;
  case (kwIF):
    // printf("In if block\n");
    if_stmt();
    break;
  case (kwRETURN):
    // printf("In return\n");
    return_stmt();
    break;
  case (LBRACE):
    match(LBRACE);
    // printf("In lbrace \n");
    opt_stmt_list();
    // printf("Tok after stmt list: %s token: %s\n", lexeme,
    // token_name[curr_tok]);
    match(RBRACE);
    break;
  case (SEMI):
    // printf("In semi\n");
    match(SEMI);
    break;
  default:
    linepexit(curr_tok, lexeme, "illegal statemet.");
    break;
  }
}

void if_stmt() {
  match(kwIF);
  match(LPAREN);
  bool_exp();
  match(RPAREN);
  stmt();
  if (curr_tok == kwELSE) {
    match(kwELSE);
    stmt();
  }
}

void while_stmt() {
  match(kwWHILE);
  match(LPAREN);
  bool_exp();
  match(RPAREN);
  stmt();
  // printf("Exited while stmt\n");
}

void return_stmt() {
  match(kwRETURN);
  if (curr_tok == SEMI) {
    match(SEMI);
  } else {
    arith_exp();
    match(SEMI);
  }
}

void assg_stmt() {
  match(ID);
  match(opASSG);
  arith_exp();
  match(SEMI);
}

void fn_call(char *id) {
  // printf("\n\n\n ------------IN FUCNTION CALL lexeme %s------------\n\n\n\n",
  // lexeme);
  match(LPAREN);
  int expected_argcnt = check_arg_count(id);
  // printf("Function Call to %s expects %d args\n", id, expected_argcnt);
  int exprcnt = opt_expr_list(expected_argcnt);
  match(RPAREN);

  if (chk_decl_flag) {
    symboltab *tableentry = getentry(id, EITHER);
    if (!tableentry) {
      linepexit(curr_tok, id, "symbol undefined.");
    }
    if (tableentry->dtype != FUNC) {
      linepexit(curr_tok, id,
                "symbol defined as a variable but used as a function.");
    }
    if (tableentry->argcnt != exprcnt) {
      char msg[1024];
      sprintf(msg,
              "wrong number of args for function call, expected %d, got %d\n",
              tableentry->argcnt, exprcnt);
      linepexit(curr_tok, id, msg);
    }
    return;
  }
}

int opt_expr_list(int expected_argcnt) {
  if (curr_tok == ID || curr_tok == INTCON) {
    return expr_list(expected_argcnt);
  }
  if (chk_decl_flag && expected_argcnt > 0) {
    char msg[1024];
    sprintf(msg,
            "wrong number of args for function call, expected %d, got %d\n",
            expected_argcnt, 0);

    linepexit(curr_tok, lexeme, msg);
  }
  return 0;
}

int expr_list(int expected_argcnt) {
  // printf("In expr list\n");
  int exprcnt = 0;
  arith_exp();
  exprcnt++;

  if (chk_decl_flag && exprcnt > expected_argcnt) {
    char msg[1024];
    sprintf(msg,
            "wrong number of args for function call, expected %d, got %d\n",
            expected_argcnt, exprcnt);
    linepexit(curr_tok, lexeme, msg);
  }

  while (curr_tok == COMMA) {
    match(COMMA);
    arith_exp();
    exprcnt++;

    if (chk_decl_flag && exprcnt > expected_argcnt) {
      char msg[1024];
      sprintf(msg,
              "wrong number of args for function call, expected %d, got %d\n",
              expected_argcnt, exprcnt);
      linepexit(curr_tok, lexeme, msg);
    }
  }
  return exprcnt;
}

void bool_exp() {
  arith_exp();
  relop();
  arith_exp();
}

void arith_exp() {
  if (curr_tok == ID) {

    if (chk_decl_flag) {
      symboltab *tableentry = getentry(lexeme, EITHER);

      if (!tableentry) {
        linepexit(curr_tok, lexeme, "symbool undefined.");
      }

      if (tableentry->dtype != VAR) {
        linepexit(curr_tok, lexeme, "using a function as a variable");
      }
    }

    match(ID);
    return;
  }
  match(INTCON);
}

void relop() {
  switch (curr_tok) {
  case (opEQ):
    match(opEQ);
    break;
  case (opNE):
    match(opNE);
    break;
  case (opLE):
    match(opLE);
    break;
  case (opLT):
    match(opLT);
    break;
  case (opGE):
    match(opGE);
    break;
  case (opGT):
    match(opGT);
    break;
  default:
    linepexit(curr_tok, lexeme, "undefined operator.");
  }
}

void type() {
  match(kwINT);
  //
}

symboltab *getentry(char *id, scopetype scope) {
  /*
  if (!strcmp(id, "teehee1")) {
    printf("getting teehee1 in scope %d, line %d\n", scope, linecnt);
  }
  */
  for (int i = 0; i < 2; i++) {
    symboltab *iscope = local;

    if (i > 0 || scope == GLOBAL)
      iscope = globl;

    for (symboltab *jtab = iscope; jtab != NULL; jtab = jtab->next) {
      // printf("at itab | %s\n", itab->name);
      if (!strcmp(id, jtab->name)) {
        return jtab;
      }
    }

    if (scope != EITHER) {
      break;
    }
  }
  return NULL;
}

void check_var(char *id) {
  if (chk_decl_flag) {
    symboltab *tableentry = getentry(id, EITHER);

    if (!tableentry) {
      linepexit(curr_tok, id, "symbol undefined.");
    }

    if (tableentry->dtype != VAR) {
      linepexit(curr_tok, id,
                "symbol declared as a function but used as a variable");
    }
  }
}

int check_arg_count(char *id) {
  if (chk_decl_flag) {
    symboltab *tableentry = getentry(id, EITHER);

    if (!tableentry) {
      linepexit(curr_tok, id, "symbol undefined.");
    }

    if (tableentry->dtype != FUNC) {
      linepexit(curr_tok, id,
                "symbol declared as a variable but used as a function.");
    }

    return tableentry->argcnt;
  }
  return 0;
}

void linepexit(Token t, char *lexeme, char *msg) {
  fprintf(stderr, "ERROR LINE %d at token %s, at lexeme %s, %s\n", linecnt,
          token_name[t], lexeme, msg);
  exit(1);
}

void freeSymTab(symboltab *tabin) {
  symboltab *nexttab = tabin;
  while (tabin != NULL) {
    nexttab = tabin->next;
    free(tabin->name);
    free(tabin);
    tabin = nexttab;
  }
}

void freeTabs(void) {
  freeSymTab(local);
  local = NULL;
  freeSymTab(globl);
  globl = NULL;
}
