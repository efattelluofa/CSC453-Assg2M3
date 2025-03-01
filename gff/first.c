/*
 * File: first.c
 * Author: Saumya Debray
 * Purpose: Compute FIRST sets
 */

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "symbols.h"
#include "proto.h"

extern symptr SymList;
extern symptr Epsilon;

/**********************************************************************
 * is_nullable() returns TRUE if the sequence of grammar symbols it   *
 * is given can derive the null string EPSILON, else it returns FALSE *
 **********************************************************************/

int is_nullable(pptr p0)
{
  if (p0 == NULL) return TRUE;

  while (p0) {
    if (!p_lookup(First(Sym(p0)), Epsilon)) {
      return FALSE;
    }
    
    p0 = Next(p0);
  }

  return TRUE;
}

/**********************************************************************
 * CompFirst() iterates over the symbols, computing FIRST sets until  *
 * there is no change to any FIRST set.                               *
 **********************************************************************/

void CompFirst()
{
  int chg = TRUE;
  symptr sptr;
  plist pl;
  pptr pb, s0;
  int eps;

  while (chg) {
    chg = FALSE;
    /*
     * sptr iterates over the symbols of the grammar; skips terminals
     */
    for (sptr = SymList; sptr; sptr = Next(sptr)) {
      if (Terminal(sptr)) continue;

      /*
       * pl iterates over the productions of the nonterminal symbol 
       * that sptr points to
       */
      for (pl = Prods(sptr); pl; pl = Next(pl)) {
	/*
	 * pb iterates over the RHS of the production pl points to
	 */
	for (pb = Prod(pl); pb; pb = Next(pb)) {
	  /*
	   * Add each non-epsilon symbol in FIRST(pb) to FIRST(sptr)
	   */
	  eps = FALSE;
	  for (s0 = First(Sym(pb)); s0; s0 = Next(s0)) {
	    if (Sym(s0) == Epsilon) {
	      eps = TRUE;
	    }
	    else {
	      if (!p_lookup(First(sptr), Sym(s0))) {
		chg = TRUE;
		First(sptr) = p_add(First(sptr), Sym(s0));
	      }
	    }
	  }
	  /* 
	   * if FIRST(pb) doesn't contain epsilon, don't look at any further
	   * symbols in the body.
	   */
	  if (eps == FALSE) {
	    break;
	  }
	}
	/* 
	 * if the entire body of the production can derive epsilon, 
	 * add epsilon to FIRST(sptr).
	 */
	if (is_nullable(Prod(pl)) && !p_lookup(First(sptr), Epsilon)) {
	  chg = TRUE;
	  First(sptr) = p_add(First(sptr), Epsilon);
	}
      }
    }
  }
}

/**********************************************************************
 * GetFirst() takes as argument a list of grammar symbols L and       *
 * returns a list of grammar symbols FIRST(L).  It assumes that       *
 * CompFirst() has been called already, so that each terminal and     *
 * nonterminal has had its FIRST set computed, and all that remains   *
 * is to propagate this to sequences of symbols.                      *
 **********************************************************************/

pptr GetFirst(pptr L)
{
  pptr flist = NULL;
  pptr ftmp, fseq;
  int eps;

  for (fseq = L ; fseq; fseq = Next(fseq)) {
    eps = FALSE;  /* indicates whether the symbol being considered can go to epsilon */

    for (ftmp = First(Sym(fseq)); ftmp; ftmp = Next(ftmp)) {
      eps |= (Sym(ftmp) == Epsilon);
      if (!p_lookup(flist, Sym(ftmp))) {
	flist = p_add(flist, Sym(ftmp));
      }
    }  /* for ftmp = ... */
    
    if (!eps) break;    
  }

  if (is_nullable(L)) {
    p_add(flist, Epsilon);
  }

  return flist;
}
