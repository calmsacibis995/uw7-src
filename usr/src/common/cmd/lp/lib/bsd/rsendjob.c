/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*		copyright	"%c%" 	*/

#ident	"@(#)rsendjob.c	1.3"
#ident	"$Header$"

#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "msgs.h"
#include "lpd.h"

extern MESG	*lp_Md;

/*
 * Package and send R_SEND_JOB to lpexec
 */
void
#if defined (__STDC__)
r_send_job(int status, char *msg)
#else
r_send_job(status, msg)
int	 status;
char	*msg;
#endif
{
	(void)mputm(lp_Md, R_SEND_JOB, Rhost, status, msg ? msize(msg) : 0, msg);
}
