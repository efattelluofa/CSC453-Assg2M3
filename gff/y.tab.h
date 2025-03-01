#ifndef _yy_defines_h_
#define _yy_defines_h_

#define KEYWD_TOK 257
#define KEYWD_START 258
#define TOKEN 259
#define IDENT 260
#define COLON 261
#define SEMI 262
#define BAR 263
#define PP 264
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union YYSTYPE {
  plist prod_list;
  pptr  prod_body;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;

#endif /* _yy_defines_h_ */
