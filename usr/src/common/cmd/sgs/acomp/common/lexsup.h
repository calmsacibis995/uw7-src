#ident	"@(#)acomp:common/lexsup.h	1.3"
/* lexsup.h */

/* Definitions of support routines for lexical analysis.
** These routines were split out so there would be one
** source copy whether the compiler was merged with the
** preprocessor or not.
*/

extern unsigned int doescape();
extern wchar_t lx_mbtowc();
extern int lx_keylook();
