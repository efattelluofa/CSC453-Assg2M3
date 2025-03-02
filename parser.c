/*
 * Author: Edward Fattell
 * File: parser.c
 * Purpose: Parser for G0, and eventually C-- compiler
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

void match(Token expected);

// Grammar Rules
void prog();
void var_decl();
void decl_or_func(Quad **subtree); // TODO: Where is ID list handled
void type();
int opt_formals();
int formals();
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

enum scope { GLOBAL, LOCAL, EITHER } typedef scopetype;

scopetype curscope = GLOBAL;

// Helper procedures
void check_var(char *lexeme);
int check_arg_count(char *lexemeLoc);
void linepexit(Token t, char *lexeme, char *str);

symboltab *getentry(char *id, scopetype scope);

Quad *new_quad(NodeType t);

void freeSymTab(symboltab *tabin);
void freeTabs(void);

symboltab *globl = NULL;
symboltab *local = NULL;

Quad *ast_root = NULL;

char *token_name[] = {
    "UNDEF",  "ID",    "INTCON", "LPAREN", "RPAREN", "LBRACE",  "RBRACE",
    "COMMA",  "SEMI",  "kwINT",  "kwIF",   "kwELSE", "kwWHILE", "kwRETURN",
    "opASSG", "opADD", "opSUB",  "opMUL",  "opDIV",  "opEQ",    "opNE",
    "opGT",   "opGE",  "opLT",   "opLE",   "opAND",  "opOR",    "opNOT",
};

symboltab *createEntry(char *lexeme) {
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
  return newHd;
}

symboltab *createFuncEntry(char *lexeme, int argcnt) {
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
  Quad *newASThead = new_quad(STMT_LIST);

  Quad *cur_quad = newASThead;

  // iterate to simulate recursion
  while (curr_tok == kwINT) {
    match(kwINT);

    decl_or_func(&cur_quad->child0);

    if (cur_quad->child0) {
      cur_quad->child1 = new_quad(STMT_LIST);
      cur_quad = cur_quad->child1;
    }
  }

  ast_root = newASThead;

  if (curr_tok == EOF) {
    // Epsilon
  } else {
    linepexit(curr_tok, lexeme, "expected EOF");
  }
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

void decl_or_func(Quad **subtree) {
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
    assert(subtree == NULL);

    Quad *newSubtree = new_quad(EXPR_LIST);
    *subtree = newSubtree;

    match(LPAREN);
    curscope = LOCAL;

    int argcnt = formals();

    curscope = GLOBAL;
    // printf("creating new func %s with %d args\n", funcName, argcnt);
    symboltab *tableentry = createFuncEntry(funcName, argcnt);
    curscope = LOCAL;

    match(RPAREN);
    match(LBRACE);
    newSubtree->type = FUNC_DEF;

    opt_var_decls();
    // printf("after var decls\n");
    opt_stmt_list(&newSubtree->child1);
    // printf("curr_tok after stmt list %s\n", token_name[curr_tok]);
    match(RBRACE);

    freeSymTab(local);
    local = NULL;
    curscope = GLOBAL;
  }
}

// TODO: Remove subtree arg on var decls
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

void opt_stmt_list(Quad **subtree) {
  assert(subtree == NULL);

  Quad *newSubtree = new_quad(STMT_LIST);
  *subtree = newSubtree;

  Quad *curSubtree = newSubtree;

  // iterate while the curr_tok is in the first set of stmt
  while (curr_tok == ID || curr_tok == kwWHILE || curr_tok == kwIF ||
         curr_tok == kwRETURN || curr_tok == LBRACE || curr_tok == SEMI) {
    // printf("Calling stmt\n");
    stmt(&curSubtree->child0);

    if (curSubtree->child0) {
      newSubtree->child1 = new_quad(STMT_LIST);
      newSubtree = newSubtree->child1;
    }
  }
  // printf("after stmt list\n");
}

void stmt(Quad **subtree) {
  assert(subtree == NULL);

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
    // printf("In while block\n");
    while_stmt(subtree);
    break;
  case (kwIF):
    // printf("In if block\n");
    if_stmt(subtree);
    break;
  case (kwRETURN):
    // printf("In return\n");
    return_stmt(subtree);
    break;
  case (LBRACE):
    match(LBRACE);
    // printf("In lbrace \n");
    opt_stmt_list(subtree);
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

void if_stmt(Quad **subtree) {
  assert(subtree == NULL);

  Quad *newSubtree = new_quad(IF);
  *subtree = newSubtree;

  match(kwIF);
  match(LPAREN);

  bool_exp(&newSubtree->child0);
  match(RPAREN);

  stmt(&newSubtree->child1);

  Quad *else_subtree = NULL;
  if (curr_tok == kwELSE) {
    match(kwELSE);
    stmt(&newSubtree->child2);
  }
}

void while_stmt(Quad **subtree) {
  assert(subtree == NULL);

  Quad *newSubtree = new_quad(WHILE);
  *subtree = newSubtree;

  match(kwWHILE);
  match(LPAREN);

  bool_exp(&newSubtree->child0);

  match(RPAREN);

  stmt(&newSubtree->child1);
  // printf("Exited while stmt\n");
}

void return_stmt(Quad **subtree) {
  assert(subtree == NULL);

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

// TODO: Look over
void assg_stmt(Quad **subtree) {
  assert(subtree == NULL);

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
  assert(subtree == NULL);

  Quad *newSubtree = new_quad(EXPR_LIST);
  *subtree = newSubtree;

  // printf("\n\n\n ------------IN FUCNTION CALL lexeme %s------------\n\n\n\n",
  // lexeme);
  match(LPAREN);
  int expected_argcnt = check_arg_count(id);
  // printf("Function Call to %s expects %d args\n", id, expected_argcnt);
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
  assert(subtree == NULL);

  Quad *newSubtree = new_quad(EXPR_LIST);
  *subtree = newSubtree;

  // printf("In expr list\n");
  int exprcnt = 0;

  Quad *curSubtree = newSubtree;

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
  assert(subtree == NULL);

  Quad *newSubtree = new_quad(DUMMY);
  *subtree = newSubtree;

  arith_exp(&newSubtree->child0);
  newSubtree->type = relop();
  arith_exp(&newSubtree->child1);
}

void arith_exp(Quad **subtree) {
  assert(subtree == NULL);

  Quad *newSubtree = new_quad(DUMMY);
  *subtree = newSubtree;

  if (curr_tok == ID) {
    symboltab *tableentry;

    if (chk_decl_flag) {
      tableentry = getentry(lexeme, EITHER);

      if (!tableentry) {
        linepexit(curr_tok, lexeme, "symbool undefined.");
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
