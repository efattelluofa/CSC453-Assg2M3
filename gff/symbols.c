/*
 * File: symbols.c
 * AUthor: S. Debray
 * Purpose: managing information about grammar symbols
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "global.h"
#include "symbols.h"

extern int LineNo;

symptr SymList = NULL;
static symptr SymTail = NULL;
symptr Epsilon = NULL;
symptr Start = NULL;
char *startsym = NULL;
int multi_start = 0;

/**********************************************************************
 * NewSym() creates a new symbol and returns a pointer to it.         *
 **********************************************************************/

symptr NewSym()
{
  symptr stmp;

  stmp = (symptr) malloc(sizeof(struct sym));
  if (!stmp) ERROR("Out of memory");

  Def(stmp) = FALSE;
  Prods(stmp) = NULL;
  First(stmp) = Follow(stmp) = NULL;
  Next(stmp) = NULL;

  return stmp;
}

/**********************************************************************
 * sym_lookup() looks up a character string in the list of symbols.   *
 * It returns a pointer to the symbol record if found, NULL o/w.      *
 **********************************************************************/

symptr sym_lookup(char *s)
{
  symptr list;
  assert(s);

  for (list = SymList; list != NULL; list = Next(list)) {
    if (strcmp(s, Name(list)) == 0) 
      return list;
  }

  return NULL;
}

/**********************************************************************
 * run down the list of symbols to find a insertion place where the   *
 * list will remain sorted alphabetically, and add it there.          *
 **********************************************************************/

symptr insert_into_symlist(char *s, int term)
{
  symptr stmp, s0;

  stmp = NewSym();
  Name(stmp) = strdup(s);
  Terminal(stmp) = term;
  /*
   * add to list of symbols
   */
  if (SymList == NULL) {
    SymList = SymTail = stmp;
  }
  else {
    for (s0 = SymList; s0; s0 = Next(s0)) {
      if (!Next(s0) || strcmp(Name(Next(s0)), s) > 0) break;
    }

    Next(stmp) = Next(s0);
    Next(s0) = stmp;

    if (s0 == SymTail) SymTail = stmp;
  }

  return stmp;
}  

/**********************************************************************
 * If the string given is not already in the list of symbols,         *
 * sym_add() adds it to the list.  It returns a pointer to the record *
 * for the symbol.                                                    *
 **********************************************************************/

symptr sym_add(char *s, int term)
{
  symptr stmp;

  assert(s);

  stmp = sym_lookup(s);
  if (stmp != NULL) {
    return stmp;
  }

#ifdef VERBOSE
  printf(">> ADDING: <%s> ... term = %d\n", s, term);
#endif

  stmp = insert_into_symlist(s, term);
  return stmp;
}

/**********************************************************************
 * If the string given is not already in the list of symbols,         *
 * sym_insert() adds it to the list and returns a pointer to the      *
 * record for the symbol; if it is already in the list, it generates  *
 * a warning.                                                         *
 **********************************************************************/

symptr sym_insert(char *s, int term)
{
  symptr stmp;

  stmp = sym_lookup(s);

  if (stmp != NULL) {
    if (Def(stmp)) {
      fprintf(stderr, "WARNING [line %d]: multiple declarations for %s\n", 
	      LineNo, s);
    }
  }
  else {
    stmp = insert_into_symlist(s, term);
  }

  Def(stmp) = TRUE;

#ifdef VERBOSE
  if (stmp != NULL) {
    printf("++ ADDED: %s [def = %d]\n", Name(stmp), (Def(stmp) == TRUE));
  }
#endif

  return stmp;

}

/**********************************************************************
 * set_startsym() sets the start symbol for the grammar.              *
 **********************************************************************/

void set_startsym(char *s)
{
  if (!startsym) {
    startsym = strdup(s); 
  }
  else {
    if (multi_start == 0) {
      fprintf(stderr, "WARNING: multiple %%start declarations -- using the first one\n");
      multi_start++;
    }
  }
}





/**********************************************************************
 * ProdCreat() creates a new production body containing only the      *
 * symbol given as an argument.                                       *
 **********************************************************************/

pptr ProdCreat(symptr sptr)
{
  pptr ptmp;

  assert(sptr);

  ptmp = (pptr)malloc(sizeof(*ptmp));
  if (!ptmp) ERROR("Out of memory!");

  Sym(ptmp) = sptr;
  Next(ptmp) = NULL;

  return ptmp;
}

/**********************************************************************
 * ProdAttach() takes a production body and attaches the given symbol *
 * at its end.                                                        *
 **********************************************************************/

pptr ProdAttach(pptr pbody, symptr sptr)
{
  pptr ptmp0, ptmp1;

  ptmp0 = ProdCreat(sptr);
  
  if (pbody == NULL) {
    return ptmp0;
  }

  /*
   * iterate down the body to the last element.
   */
  for (ptmp1 = pbody; ptmp1 && Next(ptmp1); ptmp1 = Next(ptmp1)) {
  }

  Next(ptmp1) = ptmp0;
  
  return pbody;
}

/**********************************************************************
 * MkProdList() takes a production body and attaches it at the end of *
 * the list of bodies given.  If the production body is NULL, it      *
 * creates an epsilon-production; if the list is NULL, it creates a   *
 * new list.  It returns a pointer to the head of the list.           *
 **********************************************************************/

plist MkProdList(plist pl, pptr bod)
{
  plist ptmp0, ptmp1;

  if (bod == NULL) {
    bod = ProdCreat(Epsilon);
  }

  ptmp0 = (plist) malloc(sizeof(*ptmp0));
  if (!ptmp0) ERROR("Out of memory!");

  Prod(ptmp0) = bod;
  Next(ptmp0) = NULL;

  if (!pl) {
    return ptmp0;
  }
  /*
   * iterate down the body to the last element.
   */
  for (ptmp1 = pl; Next(ptmp1) != NULL; ptmp1 = Next(ptmp1)) {
  }

  Next(ptmp1) = ptmp0;

  return pl;
}

/**********************************************************************
 * p_lookup() looks up a symbol in a set (list) of symbols, and       *
 * returns a pointer to the symbol record if found, NULL o/w.         *
 **********************************************************************/

symptr p_lookup(pptr set, symptr sptr) 
{
  pptr p0;

  for (p0 = set; p0; p0 = Next(p0)) {
    if (Sym(p0) == sptr) {
      return sptr;
    }
  }

  return NULL;
}

/**********************************************************************
 * p_add() adds a symbol to a set (list) of symbols.  It assumes that *
 * the symbol to be added does not already occur in the set, and does *
 * not check for that separately.  It returns the new set.            *
 **********************************************************************/

pptr p_add(pptr set, symptr sptr) 
{
  return ProdAttach(set, sptr);
}


