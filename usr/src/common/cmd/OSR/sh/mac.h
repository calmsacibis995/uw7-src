#ident	"@(#)OSRcmds:sh/mac.h	1.1"
#pragma comment(exestr, "@(#) mac.h 25.1 92/09/16 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1992 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Modification history
 *
 *	S000	wooch!scol	May 31,1989	
 *	Internationalization Phase I - define toint(),
 *	toascii() and todigit();
 *
 *	L001	scol!markhe	16th March, 1992
 *	- removed S000.  Use the definitions in <ctype.h>
 */
 
/* #ident	"@(#)sh:mac.h	1.3" */
/*
 *	UNIX shell
 */

#define TRUE	(-1)
#define FALSE	0
#define LOBYTE	0377
#define QUOTE	0200

#define EOF	0
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
