/*		copyright	"%c%" 	*/

#ident	"@(#)sh:common/cmd/sh/stak.c	1.8.7.2"
#ident "$Header$"
/*
 * UNIX shell
 */

#include	"defs.h"


/* ========	storage allocation	======== */

#ifdef __STDC__
void *
#else
unsigned char *
#endif
getstak(asize)			/* allocate requested stack */
int	asize;
{
	register unsigned char	*oldstak;
	register int	size;

	size = round(asize, BYTESPERWORD);
	oldstak = stakbot;
	staktop = stakbot += size;
	return(oldstak);
}

/*
 * set up stack for local use
 * should be followed by `endstak'
 */
unsigned char *
locstak()
{
	if (brkend - stakbot < BRKINCR)
	{
		if (setbrk(brkincr) == (unsigned char *)-1)
			error(0, nospace, nospaceid);
		if (brkincr < BRKMAX)
			brkincr += 256;
	}
	return(stakbot);
}

unsigned char *
savstak()
{
	assert(staktop == stakbot);
	return(stakbot);
}

unsigned char *
endstak(argp)		/* tidy up after `locstak' */
register unsigned char	*argp;
{
	register unsigned char	*oldstak;

	*argp++ = 0;
	oldstak = stakbot;
	stakbot = staktop = (unsigned char *)round(argp, BYTESPERWORD);
	return(oldstak);
}

void
tdystak(x)		/* try to bring stack back to x */
register unsigned char	*x;
{
	while ((unsigned char *)stakbsy > x)
	{
		free(stakbsy);
		stakbsy = stakbsy->word;
	}
	staktop = stakbot = max(x, stakbas);
	rmtemp(x);
}

void
stakchk()
{
	if ((brkend - stakbas) > BRKINCR + BRKINCR)
		(void)setbrk(-BRKINCR);
}

#ifdef __STDC__
void *
#else
unsigned char *
#endif
cpystak(x)
unsigned char	*x;
{
	return(endstak(movstr(x, locstak())));
}
