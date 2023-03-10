/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:inc/mail.h	1.1.4.3"

/* Structures and constants for sendmail.c (and maybe readmail.c) */

#define ADDRESS	'T'	/* label types */
#define ALIASES	'A'
#define DIRECT	'D'

#define DONE_ADDR	FBUT+7
#define DONE_VERIFY	FBUT+8

#define NOTFOUND	-1
#define FOUND		0

#define BINMAIL	0	/* codes for which mailer to use */
#define POST	1

#define EM	0	/* return codes for input format */
#define NAME	1
#define PAPER	2
#define XED	0
