#ident	"@(#)ksh93:src/cmd/ksh93/include/timeout.h	1.1"
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
extern const char	e_timeout_id[];
extern const char	e_timewarn[];
extern const char	e_timewarn_id[];
