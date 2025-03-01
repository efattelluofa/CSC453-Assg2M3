#include <stdio.h>
#include "global.h"
#include "symbols.h"
#include "y.tab.h"
#include "proto.h"

extern char *yytext;
extern symptr SymList;

void prtok(int tok)
{
  switch (tok) {
  case KEYWD_TOK: 
    printf("KEYWD_TOK: %s\n", yytext);
    break;

  case KEYWD_START: 
    printf("KEYWD_START: %s\n", yytext);
    break;

  case TOKEN:
    printf("TOKEN: %c\n", yytext[1]);
    break;

  case IDENT:
    printf("IDENT: %s\n", yytext);
    break;

  case COLON:
    printf("_:_\n");
    break;

  case SEMI:
    printf("_;_\n");
    break;

  case BAR:
    printf("_|_\n");
    break;

  case PP:
    printf("_%%%%_\n");
    break;
  }
}

void print_prod_body(plist p0)
{
  pptr p1;

  for (p1 = Prod(p0); p1 != NULL; p1 = Next(p1)) {
    printf("%s ", Name(Sym(p1)));
  }
  printf("\n");
}

void print_prod_list(symptr sptr)
{
  plist p0;

  printf("productions for %s:\n", Name(sptr));

  for (p0 = Prods(sptr); p0 != NULL; p0 = Next(p0)) {
    printf("  %s -> ", Name(sptr));
    print_prod_body(p0);
}
  printf("----------\n");
}

void print_all_prods()
{
  symptr stmp;

  for (stmp = SymList; stmp; stmp = Next(stmp)) {
    if (Terminal(stmp)) continue;

    print_prod_list(stmp);
  }
}

void PrintPSet(pptr p)
{
  if (p == NULL) {
    printf("-EMPTYSET-\n");
  }
  else {
    for ( ; p; p = Next(p)) {
      printf("%s ", Name(Sym(p)));
    }
    printf("\n");
  }
}	

void PrintSets()
{
  symptr sptr;

  printf("\nFIRST sets:\n");
  for (sptr = SymList; sptr; sptr = Next(sptr)) {
    if (Terminal(sptr)) continue;

    printf("  %s:   ", Name(sptr));
    PrintPSet(First(sptr));
  }

  printf("\nFOLLOW sets:\n");
  for (sptr = SymList; sptr; sptr = Next(sptr)) {
    if (Terminal(sptr)) continue;

    printf("  %s:   ", Name(sptr));
    PrintPSet(Follow(sptr));
  }
}
