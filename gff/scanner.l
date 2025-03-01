%{
#include <stdio.h>
#include "global.h"
#include "symbols.h"
#include "y.tab.h"

int LineNo = 1;
%}

%option nounput noinput
%x Comment1 Comment2

whitesp		[ \t\v\r]
letter		[_a-zA-Z]
digit		[0-9]
alphanum	[_a-zA-Z0-9]
%%
{whitesp}*	;
\n		{ LineNo++; }
"/*"                    { BEGIN(Comment1); }
<Comment1>\n            { LineNo++; }
<Comment1>"*"           { BEGIN(Comment2); }
<Comment1>[^*]          ;
<Comment1><<EOF>>       { fprintf(stderr, 
                                  "ERROR [line %d]: Unexpected EOF witin comment\n", 
                                  LineNo); 
                          return 0; }
<Comment2>\n            { LineNo++; BEGIN(Comment1); }
<Comment2>"*"           ;
<Comment2>"/"           { BEGIN(INITIAL); }
<Comment2>[^*/]         { BEGIN(Comment1); }
<Comment2><<EOF>>       { fprintf(stderr, 
                                  "ERROR [line %d]: Unexpected EOF witin comment\n", 
                                  LineNo); 
                          return 0; }

"%token"	{ return KEYWD_TOK; }
"%start"        { return KEYWD_START; }
"'"."'"         { return TOKEN; }
{letter}{alphanum}*	{ return IDENT; }
";"             { return SEMI; }
":"             { return COLON; }
"|"             { return BAR; }
^"%%"{whitesp}*$  { return PP; }
.		{ return yytext[0]; }
%%
