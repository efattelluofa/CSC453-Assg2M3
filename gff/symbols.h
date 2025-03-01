/* 
 * File: symbols.h
 * Author: S. Debray
 * Purpose: header file for managing symbol information
 */

#ifndef __SYMBOLS_H__
#define __SYMBOLS_H__

typedef struct sym {
  char *name;
  int terminal;
  int def;
  struct pl *prods;
  struct p *first, *follow;
  struct sym *next;
} *symptr;

#define Name(s)      ((s)->name)
#define Terminal(s)  ((s)->terminal)
#define Def(s)       ((s)->def)
#define Prods(s)     ((s)->prods)
#define First(s)     ((s)->first)
#define Follow(s)    ((s)->follow)
#define Next(s)      ((s)->next)

typedef struct p {
  symptr sym;
  struct p *next;
} *pptr;

#define Sym(p)       ((p)->sym)

typedef struct pl {
  pptr prod;
  struct pl *next;
} *plist;

#define Prod(x)      ((x)->prod)

#endif  /* __SYMBOLS_H__ */
