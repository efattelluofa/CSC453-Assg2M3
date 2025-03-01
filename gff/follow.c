/*
 * File: follow.c
 * Author: Saumya Debray
 * Purpose: Compute FOLLOW sets
 */

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "symbols.h"
#include "proto.h"

extern symptr SymList;
extern symptr Epsilon;

/**********************************************************************
 * CompFollow() iterates over the symbols, computing FOLLOW sets      *
 * until there is no change to any FOLLOW set.                        *
 **********************************************************************/

void CompFollow()
{
  int chg = TRUE;
  symptr sptr, currsym;
  plist pl;
  pptr pb, s0;
  pptr flist;

  while (chg) {
    chg = FALSE;
    for (sptr = SymList; sptr; sptr = Next(sptr)) {
      if (Terminal(sptr)) continue;
      for (pl = Prods(sptr); pl; pl = Next(pl)) {
	for (pb = Prod(pl); pb; pb = Next(pb)) {
	  currsym = Sym(pb);
	  if (Terminal(currsym)) continue;
	  
	  if (Next(pb)) {
	    /*
	     * Add each non-epsilon symbol in the FIRST set of the
	     * symbols after pb to FOLLOW(currsym)
	     */
	    flist = GetFirst(Next(pb));
	    for (s0 = flist; s0; s0 = Next(s0)) {
	      if (Sym(s0) == Epsilon) continue;
	      if (!p_lookup(Follow(currsym), Sym(s0))) {
		chg = TRUE;
		Follow(currsym) = p_add(Follow(currsym), Sym(s0));
	      }
	    }
	  } /* if (Next(pb)) */
	  /*
	   * if pb is the rightmost symbol of the production, or if
	   * the symbols to its right are nullable, i.e., can go to
	   * epsilon, add everything in FOLLOW(sptr) to FOLLOW(currsym).
	   */
	  if (!Next(pb) || is_nullable(Next(pb))) {
	    for (s0 = Follow(sptr); s0; s0 = Next(s0)) {
	      if (!p_lookup(Follow(currsym), Sym(s0))) {
		chg = TRUE;
		Follow(currsym) = p_add(Follow(currsym), Sym(s0));
	      }
	    }
	  }
	}
      }
    }
  }
}
