#ident  "@(#)libmain.c	7.1     97/10/31"
/* libmain - flex run-time support library "main" function */

/* $Header$ */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];
	{
	while ( yylex() != 0 )
		;

	return 0;
	}
