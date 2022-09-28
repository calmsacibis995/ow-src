/* @(#)ce_lex.l	@(#)ce_lex.l	3.1 04/03/92 */

%{
#include "y.tab.h"
#define rparen 41
#define lparen 40
#define rcurly 125
#define lcurly 123
#define equals 61
#define comma 44
#define dollar 36

#define	token(x)	x

extern FILE *infile;
/* redefine input so that it doesn't read from yyin */
#undef input
#define input() ((((yytchar = (yysptr > yysbuf) ? U(*--yysptr) : getc(infile)) == 10) ? (yylineno++,yytchar) : yytchar) == EOF ? 0 : yytchar)

#undef YYLMAX
#define YYLMAX 100000
%}

/* set the max buffer size to something enourmous */
%o 100000

letter			[a-zA-Z_]
id_ok			[a-zA-Z_0-9\-]
attr_val		[^>]
begin_av		[<]
end_av			[>]
white_space		[ \t]
blank			[ \t]
%%
#.*\n                   ;
")"			return token(rparen);
"("			return token(lparen);
"}"			return token(rcurly);
"{"			return token(lcurly);
"="			return token(equals);
"NS_NAME"		return token(NS_NAME);
"NS_ATTR"		return token(NS_ATTR);
"NS_ENTRIES"		return token(NS_ENTRIES);
","			return token(comma);
"$"			return token(dollar);
{id_ok}*		return token(Id);
{begin_av}{attr_val}*{end_av} return token(av_token);
[0-9]+{white_space}*{begin_av}	{
				/* special hack to read n chars ahead... */
				int num;
				char *s;

				yytext[yyleng] ='\0';
				printf( "yytext = %s\n", yytext);

				num = atoi(yytext);

				printf("num = %d\n", num);

				/* fake up the input buffer */
				yytext[0] = '<';
				s = &yytext[1];
				while (num-- > 0) {
					*s++ = input();
					if (feof(infile) || ferror(infile)) {
						*s++ = '\0';
						break;
					}
				}

				/* pray that e next char is a '>' */
				*s++ = input();
				*s = '\0';
				yyleng = s - yytext;

				if (*(s-1) != '>') {
			yyerror("missing closing '>' from attribute field");
				}

				printf("yytext = %s\n", yytext);
				return token(av_token);
			}
\n			;
{white_space}*		;
%

