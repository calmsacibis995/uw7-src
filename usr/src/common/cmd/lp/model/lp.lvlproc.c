/*		copyright	"%c%" 	*/


#ident	"@(#)lp.lvlproc.c	1.2"
#ident	"$Header$"


#include	<sys/types.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<mac.h>
#include	"lp.h"

#ifdef	__STDC__
static	void	Usage (FILE *);
#else
static	void	Usage ();
#endif


#ifdef	__STDC__
int
main (int argc, char **argv)
#else
int
main (argc, argv)

int	argc;
char	**argv;
#endif
{

	int	c,
		lvllen = 0,
		lvlwraplen = 0,
		lvlalias = 0,
		lvlfull = 0,
		lvltrunc = 0,
		lvlwrap = 0,
		error = 0;
	char	*lvlbufp,
		*p;
	
	extern	char	*optarg;
	extern	int	optopt;

	while ((c = getopt (argc, argv, "?zZt:w:")) != EOF)
	switch	(c) {
	case	'?':
		if (lvlalias || lvlfull || lvltrunc || lvlwrap || c != optopt)
		{
			error++;
			break;
		}
		Usage (stdout);
		return	0;

	case	't':
		if (lvlalias || lvltrunc)
		{
			error++;
			break;
		}
		if (sscanf (optarg, "%d", &lvllen) != 1)
			error++;
		else
			lvltrunc++;
		break;

	case	'w':
		if (!lvlfull || lvlwrap)
		{
			error++;
			break;
		}
		if (sscanf (optarg, "%d", &lvlwraplen) != 1)
			error++;
		else
			lvlwrap++;
		break;

	case	'z':
		if (lvlalias || lvlfull || lvltrunc || lvlwrap)
		{
			error++;
			break;
		}
		lvlalias++;
		break;

	case	'Z':
		if (lvlalias || lvlfull)
		{
			error++;
			break;
		}
		lvlfull++;
		break;
	}
	if (!lvlalias && !lvlfull && !lvltrunc)
		lvlalias++;

	if (error)
	{
		Usage (stderr);
		return	1;
	}
	lvlbufp = GetProcLevel (lvlalias ? LVL_ALIAS : LVL_FULL);

	if (!lvlbufp)
	{
		perror ("GetProcLevel");
		return	1;
	}
	if (lvltrunc)
		if (!TruncateLevel (lvlbufp, lvllen) && errno)
		{
			perror ("TruncateLevel");
			return	1;
		}
	if (lvlwrap)
		if (! (lvlbufp = WrapLevel (p = lvlbufp, lvlwraplen)))
		{
			perror ("WrapLevel");
			return	1;
		}

	free (p);
	(void)	printf ("%s\n", lvlbufp);

	return	0;
}

#ifdef	__STDC__
static	void
Usage (FILE *filep)
#else
static	void
Usage (filep)

FILE	*filep;
#endif
{
	(void)	fprintf (filep,
	"Usage:  lvlproc [-?[?]|-z|-Z[t len][w len]\n");
	return;
}
