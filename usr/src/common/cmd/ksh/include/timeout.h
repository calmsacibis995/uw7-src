/*		copyright	"%c%" 	*/

#ident	"@(#)ksh:include/timeout.h	1.1.6.2"
#ident "$Header$"

/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	AT&T Bell Laboratories
 *
 */

#define TGRACE		60	/* grace period before termination */
				/* The time_warn message contains this number */
extern long		sh_timeout;
extern const char	e_timeout[];
extern const char	e_timewarn[];
