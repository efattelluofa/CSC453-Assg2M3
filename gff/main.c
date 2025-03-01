#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "symbols.h"
#include "proto.h"

extern FILE *yyin;
extern char *yytext;
extern int yyparse();
extern symptr SymList, Epsilon, first_nonterminal;
extern char *startsym;
extern symptr Start;
extern int errOcc;

int currtok;

/**********************************************************************
 * Check() checks to see whether every nonterminal has a production.  *
 **********************************************************************/

void Check()
{
  symptr sptr;

  for (sptr = SymList; sptr; sptr = Next(sptr)) {
    if (Terminal(sptr)) continue;

    if (!Prods(sptr)) {
      fprintf(stderr, "ERROR: no productions found for nonterminal %s\n", Name(sptr));
      errOcc = TRUE;
    }
  }
}

/**********************************************************************
 * Init() initializes various symbols and sets.                       *
 **********************************************************************/

void Init()
{
  symptr sptr;
  
  /*
   * initialize the start symbol.
   */
  if (startsym) {  /* start symbol explicitly specified via '%start' */
    for (sptr = SymList; sptr; sptr = Next(sptr)) {
      if (strcmp(Name(sptr), startsym) == 0) {
	if (Terminal(sptr)) {
	  fprintf(stderr, "ERROR: start symbol %s cannot be a terminal\n",
		  startsym);
	  exit(-1);
	}
	Start = sptr;
	break;
      }
    }
  }
  else {   /* start symbol not explicitly specified */
    Start = first_nonterminal;
  }
  /*
   * if we haven't found the start symbol, bail.
   */
  if (!Start) {
    if (startsym) {
      fprintf(stderr, "Could not find start symbol %s -- exiting\n", startsym);
    }
    exit(0);
  }
  /*
   * set the first set of each terminal symbol to the singleton set
   * containing itself.
   */
  for (sptr = SymList; sptr; sptr = Next(sptr)) {
    if (!Terminal(sptr)) continue;

    First(sptr) = p_add(NULL, sptr);
  }
  /*
   * add the end-of-input marker to the FOLLOW set of the start symbol.
   */
  Follow(Start) = p_add(NULL, sym_insert("<EOF>", TRUE));
}

int main(int argc, char *argv[])
{
  FILE *fp;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s file\n", argv[0]);
    exit(-1);
  }

  fp = fopen(argv[1], "r");
  if (!fp) {
    perror(argv[1]);
    exit(-1);
  }

  yyin = fp;
  
  Epsilon = sym_insert("<epsilon>", TRUE);  /* init */

  yyparse();  

#if 0
  print_all_prods();
#endif

  Check();

  if (errOcc) {
    fprintf(stderr, "\nInput file contains errors: aborting...\n");
    exit(-1);
  }

  Init();

  CompFirst();

  CompFollow();

  PrintSets();

  return 0;
}

