/*
 * Author: Edward Fattell
 * File: parser.c
 * Purpose: Parser for G2, and eventually C-- compiler
 */
#include "parser.h"
#include "ast.h"
#include "scanner.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int curr_tok;

// Symbol Table pointers
symboltab *globl = NULL;
symboltab *local = NULL;

Quad *ast_root = NULL;
enum scope { GLOBAL, LOCAL, EITHER } typedef scopetype;
scopetype curscope = GLOBAL;
symboltab *getentry(char *id, scopetype scope);
Quad *new_quad(NodeType t);
void match(Token expected);

// Grammar Rule Procedures
void prog();
void var_decl();
void decl_or_func();
void type();
int opt_formals(Quad **subtree);
int formals(Quad **subtree);
void opt_var_decls();
void opt_stmt_list(Quad **subtree);
void stmt(Quad **subtree);
void if_stmt(Quad **subtree);
void while_stmt(Quad **subtree);
void return_stmt(Quad **subtree);
void assg_stmt(Quad **subtree);
void fn_call(Quad **subtree, char *id);
int opt_expr_list(Quad **subtree, int expected_argcnt);
int expr_list(Quad **subtree, int expected_argcnt);
void bool_exp(Quad **subtree);
void arith_exp(Quad **subtree);
NodeType relop();

// Helper procedures
void check_var(char *lexeme);
int check_arg_count(char *lexemeLoc);
void linepexit(Token t, char *lexeme, char *str);
void freeQuad(Quad *quad);
void freeSymTab(symboltab *tabin);
void freeTabs(void);

// Token Name array for printing error messages
char *token_name[] = {
    "UNDEF",  "ID",    "INTCON", "LPAREN", "RPAREN", "LBRACE",  "RBRACE",
    "COMMA",  "SEMI",  "kwINT",  "kwIF",   "kwELSE", "kwWHILE", "kwRETURN",
    "opASSG", "opADD", "opSUB",  "opMUL",  "opDIV",  "opEQ",    "opNE",
    "opGT",   "opGE",  "opLT",   "opLE",   "opAND",  "opOR",    "opNOT",
};

symboltab *createEntry(char *lexeme) {
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
  return newHd;
}

symboltab *createFuncEntry(char *lexeme, int argcnt) {
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
  return newHd;
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
  // iterate to simulate recursion
  while (curr_tok == kwINT) {
    match(kwINT);
    decl_or_func();
  }

  if (curr_tok == EOF) {
    // Epsilon
  } else {
    linepexit(curr_tok, lexeme, "expected EOF");
  }
}

void id_list_rest() {
  while (curr_tok == COMMA) {
    match(COMMA);
    if (curr_tok == ID) {
      createEntry(lexeme);
    }
    match(ID);
  }
}

void id_list() {
  if (curr_tok == ID) {
    createEntry(lexeme);
  }
  match(ID);
  if (curr_tok != COMMA)
    return;
  id_list_rest();
}

/*
 * Creates a new subtree of type expr_list
 * each node in the list points to an ID node representing each arg
 */
int formals(Quad **subtree) {
  assert(*subtree == NULL);

  Quad *newSubtree = NULL;
  Quad *curSubtree = NULL;

  int formalcnt = 0;

  if (curr_tok == kwINT) {
    newSubtree = new_quad(EXPR_LIST);
    *subtree = newSubtree;
    curSubtree = newSubtree;
    type();

    curSubtree->child0 = new_quad(IDENTIFIER);
    curSubtree->child0->tableentry = createEntry(lexeme);
    match(ID);

    formalcnt++;
  }

  while (curr_tok == COMMA) {
    match(COMMA);

    type();
    if (curr_tok == ID) {
      curSubtree->child1 = new_quad(EXPR_LIST);
      curSubtree = curSubtree->child1;

      curSubtree->child0 = new_quad(IDENTIFIER);
      curSubtree->child0->tableentry = createEntry(lexeme);
    }
    match(ID);
    formalcnt++;
  }

  return formalcnt;
}

/*
 * func Quad:
 * type = FUNC
 * tableentry = `function's table entry
 * child0 = expr_lit of formals
 * child1 = body Quad
 */
void decl_or_func() {
  Quad *subtreeHd = NULL;
  Quad **subtree = &subtreeHd;

  char funcName[1024];
  strcpy(funcName, lexeme);
  match(ID);

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

    Quad *newSubtree = new_quad(EXPR_LIST);
    *subtree = newSubtree;

    match(LPAREN);
    curscope = LOCAL;

    int argcnt = formals(&newSubtree->child0);

    curscope = GLOBAL;
    // printf("creating new func %s with %d args\n", funcName, argcnt);
    newSubtree->tableentry = createFuncEntry(funcName, argcnt);
    curscope = LOCAL;

    match(RPAREN);
    match(LBRACE);
    newSubtree->type = FUNC_DEF;

    opt_var_decls();
    opt_stmt_list(&newSubtree->child1);
    match(RBRACE);

    if (print_ast_flag)
      print_ast(*subtree);

    freeQuad(*subtree);
    freeSymTab(local);
    local = NULL;
    curscope = GLOBAL;
  }
}

int opt_formals(Quad **subtree) {
  if (curr_tok == kwINT) {
    return formals(subtree);
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

void opt_stmt_list(Quad **subtree) {
  assert(*subtree == NULL);

  Quad *newSubtree = NULL;
  Quad *curSubtree = NULL;

  if (curr_tok == ID || curr_tok == kwWHILE || curr_tok == kwIF ||
      curr_tok == kwRETURN || curr_tok == LBRACE || curr_tok == SEMI) {
    newSubtree = new_quad(STMT_LIST);
    *subtree = newSubtree;
    curSubtree = newSubtree;
  }

  // iterate while the curr_tok is in the first set of stmt
  while (curr_tok == ID || curr_tok == kwWHILE || curr_tok == kwIF ||
         curr_tok == kwRETURN || curr_tok == LBRACE || curr_tok == SEMI) {
    stmt(&curSubtree->child0);

    if (curSubtree->child0) {
      curSubtree->child1 = new_quad(STMT_LIST);
      curSubtree = curSubtree->child1;
    }
  }
}

void stmt(Quad **subtree) {
  assert(*subtree == NULL);

  char id[1024];
  switch (curr_tok) {
  case (ID):
    if (curr_tok == ID && strlen(lexeme)) {
      strcpy(id, lexeme);
    }
    match(ID);
    // Could be either assignment or fn call; leftfactor
    if (curr_tok == opASSG) {
      Quad *newSubtree = new_quad(ASSG);
      *subtree = newSubtree;
      // printf("In op assign\n");
      match(opASSG);
      check_var(id);

      newSubtree->tableentry = getentry(id, EITHER);
      newSubtree->child0 = new_quad(IDENTIFIER);
      newSubtree->child0->tableentry = newSubtree->tableentry;

      arith_exp(&newSubtree->child1);
      match(SEMI);
      return;
    }
    // match the rest of fn call

    fn_call(subtree, id);

    match(SEMI);
    break;
  case (kwWHILE):
    while_stmt(subtree);
    break;
  case (kwIF):
    if_stmt(subtree);
    break;
  case (kwRETURN):
    return_stmt(subtree);
    break;
  case (LBRACE):
    match(LBRACE);
    opt_stmt_list(subtree);
    match(RBRACE);
    break;
  case (SEMI):
    match(SEMI);
    break;
  default:
    linepexit(curr_tok, lexeme, "illegal statemet.");
    break;
  }
}

void if_stmt(Quad **subtree) {
  assert(*subtree == NULL);

  Quad *newSubtree = new_quad(IF);
  *subtree = newSubtree;

  match(kwIF);
  match(LPAREN);

  bool_exp(&newSubtree->child0);
  match(RPAREN);

  stmt(&newSubtree->child1);

  if (curr_tok == kwELSE) {
    match(kwELSE);
    stmt(&newSubtree->child2);
  }
}

void while_stmt(Quad **subtree) {
  assert(*subtree == NULL);

  Quad *newSubtree = new_quad(WHILE);
  *subtree = newSubtree;

  match(kwWHILE);
  match(LPAREN);

  bool_exp(&newSubtree->child0);

  match(RPAREN);

  stmt(&newSubtree->child1);
}

void return_stmt(Quad **subtree) {
  assert(*subtree == NULL);

  Quad *newSubtree = new_quad(EXPR_LIST);
  *subtree = newSubtree;

  newSubtree->type = RETURN;

  match(kwRETURN);
  if (curr_tok == SEMI) {
    match(SEMI);
  } else {
    arith_exp(&newSubtree->child0);
    match(SEMI);
  }
}

void assg_stmt(Quad **subtree) {
  assert(*subtree == NULL);

  Quad *newSubtree = new_quad(EXPR_LIST);
  *subtree = newSubtree;

  newSubtree->type = ASSG;
  if (curr_tok == ID) {
    newSubtree->tableentry = getentry(lexeme, curscope);
  }
  match(ID);
  match(opASSG);
  arith_exp(&newSubtree->child1);
  match(SEMI);
}

void fn_call(Quad **subtree, char *id) {
  assert(*subtree == NULL);

  Quad *newSubtree = new_quad(FUNC_CALL);
  *subtree = newSubtree;

  newSubtree->tableentry = getentry(id, GLOBAL);

  match(LPAREN);
  int expected_argcnt = check_arg_count(id);
  int exprcnt = opt_expr_list(&newSubtree->child0, expected_argcnt);
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

int opt_expr_list(Quad **subtree, int expected_argcnt) {
  if (curr_tok == ID || curr_tok == INTCON) {
    return expr_list(subtree, expected_argcnt);
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

int expr_list(Quad **subtree, int expected_argcnt) {
  assert(*subtree == NULL);

  Quad *newSubtree = NULL;
  Quad *curSubtree = NULL;

  newSubtree = new_quad(EXPR_LIST);
  *subtree = newSubtree;
  curSubtree = newSubtree;

  print_ast(newSubtree);
  int exprcnt = 0;

  arith_exp(&curSubtree->child0);

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
    exprcnt++;

    curSubtree->child1 = new_quad(EXPR_LIST);
    curSubtree = curSubtree->child1;

    arith_exp(&curSubtree->child0);

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

void bool_exp(Quad **subtree) {
  assert(*subtree == NULL);

  Quad *newSubtree = new_quad(DUMMY);
  *subtree = newSubtree;

  arith_exp(&newSubtree->child0);
  newSubtree->type = relop();
  arith_exp(&newSubtree->child1);
}

void arith_exp(Quad **subtree) {
  assert(*subtree == NULL);

  Quad *newSubtree = new_quad(DUMMY);
  *subtree = newSubtree;

  if (curr_tok == ID) {
    symboltab *tableentry;

    tableentry = getentry(lexeme, EITHER);

    if (chk_decl_flag) {
      if (!tableentry) {
        linepexit(curr_tok, lexeme, "symbol undefined.");
      }

      if (tableentry->dtype != VAR) {
        linepexit(curr_tok, lexeme, "using a function as a variable");
      }
    }

    match(ID);
    newSubtree->type = IDENTIFIER;
    newSubtree->tableentry = tableentry;
    return;
  }

  newSubtree->type = INTCONST;
  newSubtree->immediate = lval;
  match(INTCON);
}

NodeType relop() {
  switch (curr_tok) {
  case (opEQ):
    match(opEQ);
    return EQ;
  case (opNE):
    match(opNE);
    return NE;
  case (opLE):
    match(opLE);
    return LE;
  case (opLT):
    match(opLT);
    return LT;
  case (opGE):
    match(opGE);
    return GE;
  case (opGT):
    match(opGT);
    return GT;
  default:
    linepexit(curr_tok, lexeme, "undefined operator.");
    return -1;
  }
}

void type() {
  match(kwINT);
  //
}

symboltab *getentry(char *id, scopetype scope) {
  for (int i = 0; i < 2; i++) {
    symboltab *iscope = local;

    if (i > 0 || scope == GLOBAL)
      iscope = globl;

    for (symboltab *jtab = iscope; jtab != NULL; jtab = jtab->next) {
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
  symboltab *tableentry = getentry(id, EITHER);
  if (chk_decl_flag) {

    if (!tableentry) {
      linepexit(curr_tok, id, "symbol undefined.");
    }

    if (tableentry->dtype != FUNC) {
      linepexit(curr_tok, id,
                "symbol declared as a variable but used as a function.");
    }
  }
  return tableentry->argcnt;
}

void linepexit(Token t, char *lexeme, char *msg) {
  fprintf(stderr, "ERROR LINE %d at token %s, at lexeme %s, %s\n", linecnt,
          token_name[t], lexeme, msg);
  exit(1);
}

Quad *new_quad(NodeType t) {
  Quad *new_quad = malloc(sizeof(Quad));

  // Fill in Quad with empty value to prevent memory errors
  new_quad->type = t;
  new_quad->tableentry = NULL;
  new_quad->immediate = 0;
  new_quad->child0 = NULL;
  new_quad->child1 = NULL;
  new_quad->child2 = NULL;

  return new_quad;
}

void freeQuad(Quad *quad) {
  if (quad->child0)
    freeQuad(quad->child0);
  if (quad->child1)
    freeQuad(quad->child1);
  if (quad->child2)
    freeQuad(quad->child2);
  free(quad);
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
