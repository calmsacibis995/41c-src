h11895
s 00050/00000/00000
d D 1.1 83/02/23 13:17:02 cooper 1 0
c date and time created 83/02/23 13:17:02 by cooper
e
u
U
t
T
I 1
%{
/*
 * %M% %I% %G%
 */
%}
%%

--.*\n		;
[ \t\n]		;
ARRAY		{ return (ARRAY); }
BEGIN		{ return (_BEGIN); }
BOOLEAN		{ return (BOOLEAN); }
CARDINAL	{ return (CARDINAL); }
CHOICE		{ return (CHOICE); }
DEPENDS		{ return (DEPENDS); }
END		{ return (END); }
ERROR		{ return (ERROR); }
INTEGER		{ return (INTEGER); }
LONG		{ return (LONG); }
OF		{ return (OF); }
PROCEDURE	{ return (PROCEDURE); }
PROGRAM		{ return (PROGRAM); }
RECORD		{ return (RECORD); }
REPORTS		{ return (REPORTS); }
RETURNS		{ return (RETURNS); }
SEQUENCE	{ return (SEQUENCE); }
STRING		{ return (STRING); }
TYPE		{ return (TYPE); }
UNSPECIFIED	{ return (UNSPECIFIED); }
UPON		{ return (UPON); }
VERSION		{ return (VERSION); }

-?[0-9]+			{
				/*
				 * Only decimal numbers are supported.
				 */
				yylval.object = make(O_CONSTANT, atoi(yytext));
				return (number);
			}

[a-zA-Z_][a-zA-Z0-9_]*	{
				yylval.object = make(O_SYMBOL, yytext);
				return (identifier);
			}

.			{
				return ((int) yytext[0]);
			}

%%
E 1
