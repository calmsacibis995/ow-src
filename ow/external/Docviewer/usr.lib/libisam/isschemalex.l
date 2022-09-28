%{
/*
 @(#)isschemalex.l	1.3 06/11/93 SMI
*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isschemalex.l
 *
 * Description:
 *	lex analyzer for isschema.y parser
 */

char *instalstring();
static int	linenumber = 1;
%}

%%
table			{ return (TABLE); }
long			{ return (LONG); }
int			{ return (INT); }
short			{ return (SHORT); }
double			{ return (DOUBLE); }
float			{ return (FLOAT); }
char			{ return (CHAR); }
bin                     { return (BIN); }
key			{ return (KEY); }
as			{ return (AS); }
primary			{ return (PRIMARY); }
alternate		{ return (ALTERNATE); }
duplicates		{ return (DUPLICATES); }
asc                     { return (ASC); }
desc                    { return (DESC); }
declare                 { /* skip */ ; }
[a-zA-Z_][a-zA-Z_0-9]*	{ yylval.cval = instalstring (yytext) ;return (IDENT); }
[0-9]+			{ yylval.ival = atoi (yytext); return (INTNUMB); }
[\(\)\,]		{ return (*yytext); }
[\t ]			;
.                       {  
                           char buf [100];

                           (void)sprintf (buf,"invalid character '%s'",yytext);
			   yyerror (buf);
		        }
\/\*                    {
                 int c;

		 loop:
		    if ((c = input()) == 0)
			yyerror ("end of file within a comment");
		    if (c == '\n') linenumber++; 
		    if (c != '*') goto loop;
		
		    switch (input()) {
		    case '/': break;
		    case '*': unput ('*'); goto loop;
		    case '\n': linenumber++; goto loop;
		    case 0: yyerror ("end of file within a comment");
		    default: goto loop;
		    }
                        }
\n			{ linenumber++; }
%%

char *instalstring (s)
    char	*s;
{
    extern char *_ismalloc();
    char	*p;

    p = _ismalloc (strlen (s) + 1);
    (void)strcpy (p,s);

    return (p);
}
