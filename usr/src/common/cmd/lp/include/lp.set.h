/*		copyright	"%c%" 	*/

#ident	"@(#)lp.set.h	1.2"
#ident	"$Header$"


#if	!defined(_LP_LP_SET_H)
#define	_LP_LP_SET_H

/*
 * How far should we check for "compressed" horizontal pitch?
 * Keep in mind that (1) too far and the user can't read it, and
 * (2) some Terminfo entries don't limit their parameters like
 * they should. Keep in mind the other hand, though: What is too
 * compact for you may be fine for the eagle eyes next to you!
 */
#define MAX_COMPRESSED	30	/* CPI */

#define	E_SUCCESS	0
#define	E_FAILURE	1
#define	E_BAD_ARGS	2
#define	E_MALLOC	3

#define	OKAY(P)		((P) && (*P))
#define R(F)		(int)((F) + .5)

#if	!defined(CHARSETDIR)
# define CHARSETDIR	"/usr/share/lib/charset"
#endif

#if	defined(__STDC__)

int		set_pitch ( char * , int , int );
int		set_size ( char * , int , int );
int		set_charset ( char * , int , char * );

#else

int		set_pitch(),
		set_size(),
		set_charset();

#endif

#endif
