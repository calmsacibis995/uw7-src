/*		copyright	"%c%"	*/

#ident	"@(#)stubs.c	15.1"

#include "sys/types.h"
#include <pfmt.h>
#include <stdio.h>
#include <ctype.h>
#include "priv.h"

#ifndef NULL
#define NULL 0
#endif

/*
 * stubs for security system calls that are not available on a
 * non-ES system
 */

int lvldom(const level_t *a, const level_t *b) {
	return(0);
}
int lvlfile(const char *a, int n, level_t *b){
	return(0);
}
int lvlproc(int a,  level_t *b){
	return(0);
}
int lvlin(const char *a, level_t *n){
	return(0);
}
int lvlout(const level_t *a, char *b, int c, int d){
	return(0);
}
int lvlvalid(const level_t *a){
	return(0);
}
int secsys(int a, char *b){
	return(0);
}
int	filepriv(const char *a, int b, priv_t *c, int d){
	return(0);
}
int	mldmode(int a){
	return(-1);  /* This function must always fail on SVR4.0 */
}

/*
 * stubs for message catalog routines.
 */

/*ARGSUSED*/
int
setlabel(label)
const char *label;
{
	return 0;
}

/*ARGSUSED*/
const char *
setcat(cat)
const char *cat;
{
	return NULL;
}

/* pfmt() - format and print */

/*ARGSUSED*/
int
#ifdef __STDC__
pfmt(FILE *stream, long flag, const char *format, ...)
#else
pfmt(stream, flag, format, va_alist)
FILE *stream;
long flag;
const char *format;
va_dcl
#endif
{
	va_list args;
	const char *ptr;
	int status;
	register int length = 0;

#ifdef __STDC__
	va_start(args,);
#else
	va_start(args);
#endif

	if (!(flag & MM_NOGET) && format) {
		ptr = format;
		while(*ptr++ != ':');
		*ptr++;
		while (isdigit(*ptr++));
		
		format = ptr;

	}

	if (stream){
		if ((status = vfprintf(stream, format, args)) < 0)
			return -1;
		length += status;
	}

	return length;
}

/* vpfmt() - format and print */

/*ARGSUSED*/
int
vpfmt(stream, flag, format, args)
FILE *stream;
long flag;
const char *format;
va_list args;
{
	const char *ptr;
	int status;
	register int length = 0;

	if (!(flag & MM_NOGET) && format) {
		ptr = format;
		while(*ptr++ != ':');
		*ptr++;
		while (isdigit(*ptr++));
		
		format = ptr;

	}

	if (stream){
		if ((status = vfprintf(stream, format, args)) < 0)
			return -1;
		length += status;
	}

	return length;
}

/*ARGSUSED*/
char *
gettxt(msgid, dflt)
const char *msgid, *dflt;
{
	return((char *)dflt);
}
