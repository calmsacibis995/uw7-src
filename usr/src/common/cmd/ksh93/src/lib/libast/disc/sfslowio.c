#ident	"@(#)ksh93:src/lib/libast/disc/sfslowio.c	1.1"
#pragma prototyped

/*
 * AT&T Bell Laboratories
 *
 * make stream interruptable
 */

#include <ast.h>
#include <error.h>
#include <sfdisc.h>

/*
 * sfslowio exception handler
 * EOF in interrupt
 * free on close
 */

static int
slowexcept(Sfio_t* sp, int op, Sfdisc_t* dp)
{
	NoP(sp);
	switch (op)
	{
	case SF_CLOSE:
		free(dp);
		break;
	case SF_READ:
	case SF_WRITE:
		if (errno == EINTR)
			return(-1);
		break;
	}
	return(0);
}

/*
 * create and push sfio slowio discipline
 */

int
sfslowio(Sfio_t* sp)
{
	Sfdisc_t*	dp;

	if (!(dp = newof(0, Sfdisc_t, 1, 0)))
		return(-1);
	dp->exceptf = slowexcept;
	return(sfdisc(sp, dp) ? 0 : -1);
}
