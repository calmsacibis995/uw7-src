/*		copyright	"%c%" 	*/

#ident	"@(#)sh:common/cmd/sh/mac.h	1.8.5.2"
#ident "$Header$"
/*
 *	UNIX shell
 */

#define TRUE	(-1)
#define FALSE	0
#define LOBYTE	0377
#define QUOTE	0200

#ifndef EOF
#define EOF	0
#endif
#define NL	'\n'
#define SP	' '
#define LQ	'`'
#define RQ	'\''
#define MINUS	'-'
#define COLON	':'
#define TAB	'\t'


#define MAX(a,b)	((a)>(b)?(a):(b))

#define blank()		prc(SP)
#define	tab()		prc(TAB)
#define newline()	prc(NL)

