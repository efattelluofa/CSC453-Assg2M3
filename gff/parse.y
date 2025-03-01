%{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "global.h"
#include "symbols.h"
#include "proto.h"

extern char *yytext;
extern int LineNo;
extern int yylex();
extern void yyerror();
extern symptr SymList;
extern symptr Epsilon;

symptr stmp;
symptr first_nonterminal = NULL;
int errOcc = FALSE;
%}

%token KEYWD_TOK KEYWD_START TOKEN IDENT COLON SEMI BAR PP

%union {
  plist prod_list;
  pptr  prod_body;
}

%type <prod_body> body;
%type <prod_list> body_list

%start Gram
%%
Gram : tokens PP prods 
;

tokens : tokens KEYWD_TOK toklist
       | tokens KEYWD_START IDENT { set_startsym(yytext); }
       |
;

toklist : toklist IDENT { sym_insert(yytext, TRUE); }
        | IDENT { sym_insert(yytext, TRUE); }
;

prods : prods prod
      |
;

prod : IDENT 
       { stmp = sym_insert(yytext, FALSE); 
	 if (first_nonterminal == NULL) {
	   first_nonterminal = stmp;
	 }
       } 
       COLON 
       body_list
       SEMI 
       { Prods(stmp) = $4; }
;

body_list : body_list BAR body { $$ = MkProdList($1, $3); }
     | body { $$ = MkProdList(NULL, $1); }
;

body : body IDENT { $$ = ProdAttach($1, sym_add(yytext, FALSE)); }
     | body TOKEN  { $$ = ProdAttach($1, sym_add(yytext, TRUE)); }
     | { $$ = NULL; }
;

%%
void yyerror(char *s)
{
  fprintf(stderr, "line %d: %s near: %s\n", LineNo, s, yytext);
  errOcc = TRUE;
}
