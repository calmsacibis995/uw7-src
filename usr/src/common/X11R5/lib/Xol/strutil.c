/*		copyright	"%c%" 	*/

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olmisc:strutil.c	1.6"
#endif

/*
 * strutil.c
 *
 */

#include <stdio.h>
#include <strutil.h>
#include <X11/memutil.h>


/*
 * strmch
 *
 * The \fIstrmch\fR function is used to compare two strings \fIs1\fR
 * and \fIs2\fR.  It returns the index of the first character which 
 * does not match in the two strings.  The value -1 is returned if 
 * the strings match.
 *
 * See also:
 *
 * strnmch(3)
 *
 * Synopsis:
 *
 *#include <strutil.h>
 * ...
 */

extern int strmch(s1, s2)
char * s1;
char * s2;
{

char * save = s1;

if(s1 == NULL || s2 == NULL)
	return 0;		/* defend against NULL pointers */

while (*s1 == *s2 && *s1 && *s2)
   {
   s1++;
   s2++;
   }
return (*s1 == *s2) ? -1 : s1 - save;

} /* end of strmch */
/*
 * strnmch
 *
 * The \fIstrnmch\fR function is used to compare two strings \fIs1\fR
 * and \fIs2\fR through at most \fIn\fR characters and return the
 * index of the first character which does not match in the two strings.
 * The value -1 is returned if the strings match for the specified number
 * of characters.
 *
 * See also:
 *
 * strmch(3)
 *
 * Synopsis:
 *
 *#include <strutil.h>
 * ...
 */

extern int strnmch(s1, s2, n)
char * s1;
char * s2;
int    n;
{
char * save = s1;

if(s1 == NULL || s2 == NULL)
	return 0;		/* defend against NULL pointers */

while (*s1 == *s2 && n)
   {
   s1++;
   s2++;
   n--;
   }

return (*s1 == *s2) ? -1 : s1 - save;

} /* end of strnmch */
/*
 * strndup
 *
 * The \fIstrndup\fR function is used to create a null-terminated copy of 
 * the first \fIl\fR characters stored in \fIs\fR.
 *
 * Synopsis:
 *
 *#include <strutil.h>
 * ...
 */

extern char * strndup(s, l)
char * s;
int l;
{
char * p = (char *) MALLOC(l + 1);

if (p)
   {
   memcpy(p, s, l);
   p[l] = '\0';
   }

return (p);

} /* end of strndup */
