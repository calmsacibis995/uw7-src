#ident	"@(#)acomp:common/ansisup.h	1.3"
/* ansisup.h */

/* Provide declarations for ANSI C library routines on
** systems with no ANSI C environment.  Assume, in other
** words, that __STDC__ is not defined.
*/

/* string conversion */
unsigned long strtoul();

/* wide character support */
typedef long wchar_t;
extern int mbtowc();

#define LC_CTYPE 1	/* value is irrelevant for stub */
extern char * setlocale();
