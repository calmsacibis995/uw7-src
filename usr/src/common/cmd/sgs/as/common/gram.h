#ident	"@(#)nas:common/gram.h	1.2"
/*
* common/gram.h - common assembler parsing header
*
* Depends on:
*	<stdio.h>
*/

#ifdef __STDC__
void	initchtab(void);	/* initialize tokenization code */
void	setinput(FILE *);	/* setup stream for parsing */
int	yyparse(void);		/* parse current input stream */
char	*tokstr(int);		/* printable version of token */
#else
void	initchtab();
void	setinput();
int	yyparse();
char	*tokstr();
#endif
