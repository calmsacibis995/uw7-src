/*		copyright	"%c%" 	*/


#ident	"@(#)lp.pr.c	1.2"
#ident  "$Header$"
/***************************************************************************
 * Command: lp.pr
 * Inheritable Privileges: -
 *       Fixed Privileges: P_AUDIT
 * Notes:  mark level of the print job on the printed output
 *
 ***************************************************************************/

#include	<sys/types.h>
#include	<sys/param.h>
#include	<audit.h>
#include	<pwd.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<mac.h>
#include	<string.h>

#include	"lp.h"
#include	"debug.h"

int	AuditFlag = 0;		/*  To cut audit record and exit  */
int	EnhanceFlag = 0;	/*  Enhance header and footer  */
int	SimpleStrategy = 0;
int	ComplexStrategy = 0;
int	Pagelen	= 80;		/*  Length of page in columns  */
int	Pagewidth = 66;		/*  Length of page in lines  */
int	TabStops = 8;		/*  Tabstops every 'TabStop' spaces  */

char	StartEnhance [8];
char	EndEnhance [8];
char *	Jobidp;
char *  Lognamep;

#ifdef	__STDC__
static	void	Usage (FILE *);
static	void	ParseOptions (int, char **);
static	void	CutTruncAuditRec ();
static	void	CutNoLabelsAuditRec ();
static	void	CutNoSupportAuditRec ();
static	char *	Enhance (char *);
#else
static	void	Usage ();
static	void	ParseOptions ();
static	void	CutTruncAuditRec ();
static	void	CutNoLabelsAuditRec ();
static	void	CutNoSupportAuditRec ();
static	char *	Enhance ();
#endif

#ifdef	_STDC__
static	void
Usage (FILE *filep)
#else
static	void
Usage (filep)

FILE	*filep;
#endif
{
	(void)	fprintf (filep, "%s\n", "Usage:");
	(void)  fprintf (filep, "\t%s\n", "lp.pr -?[?]");
	(void)  fprintf (filep, "\t      %s\n", "-A jobid");
	(void)  fprintf (filep, "\t      %s %s %s %s %s\n", "[-l pagelen]",
		"[-w pagewidth]", "[-t tabstops]",
		"[-E strategy [-E reset]]", "jobid");
	return;
}

#ifdef	_STDC__
int
main (int argc, char **argv)
#else
int
main (argc, argv)

int	argc;
char	**argv;
#endif
{
	int	n,
		pagination = 1, 	/*  Pagination flag */
		headlen = 0,		/*  Header length */
		footlen = 0,		/*  Footer length */
		column = 1,		/*  Current column (output)  */
		line = 1;		/*  Current line (output)  */
	char	ibuf [1024],		/*  Input buffer  */
		obuf [1024],		/*  Output buffer  */
		*headerp = (char *)0,	/*  Header  */
		*footerp = (char *)0,	/*  Footer  */
		*p;
	struct
	passwd	*passwdp;

	register char	*ip = ibuf;	/*  Current pos in ibuf  */
	register char	*op = obuf;	/*  Current pos in obuf  */

	DEFINE_FNNAME (main)
	OPEN_DEBUG_FILE ("lp.pr.debug")
	/*
	**  Find out who we are for any audit recs.
	**
	*/
	passwdp = getpwuid (getuid ());

	if (!passwdp)
	{
		endpwent ();
		return	1;
	}

	Lognamep = strdup (passwdp->pw_name);

	endpwent ();
	/*
	**
	*/
	ParseOptions (argc, argv);

	if (AuditFlag)
	{
		CutNoLabelsAuditRec ();
		return	0;
	}
	/*
	**
	*/
	p = GetProcLevel (LVL_FULL);

	if (p)
	{
		if (TruncateLevel (p, Pagewidth))
		{
			CutTruncAuditRec ();
		}
		if (EnhanceFlag)
		{
			if (headerp = Enhance (p))
			{
				(void)	free (p);
				p = headerp;
			}
		}
		headlen = strlen (p);

		headerp = footerp = (char *) malloc (headlen + 2);

		(void)	strcpy (headerp, p);

		headerp [headlen++] = '\n';
		headerp [headlen]   = '\0';

		footlen = headlen;
	}
	else
		pagination = 0;

	TRACEs (headerp) TRACEd (headlen)
	TRACEs (footerp) TRACEd (footlen)
	/*
	**  Only printable characters (0x20 - 0x7e)
	**  and the non-printables (0x08(BS), 0x09(TAB), 0x0a(LF),
	**  0x0c(FF), **  0x0d(CR)) are allowed.
	**  All other chars cause us to stop paginating and resume
	**  ``as-is'' throughput.
	*/
#define	BUF_LEN(basep, p)	((int) p - (int) basep)

	if (headerp)
	{
		(void)	write (1, headerp, headlen);
		line++;
	}
while ((n = read (0, ibuf, sizeof (ibuf))) > 0)
{
	if (!pagination)
	{
		(void)	write (1, ibuf, n);
		continue;
	}
	for (ip=ibuf; (int) ip < ((int) ibuf + n); )
	{
		/*
		**  Normal printable character
		*/
		if (*ip >= (char) 0x20  && *ip <= (char) 0x7e)
		{
			TRACEP ("Normal character")
			*op++ = *ip++;
			column++;
			if (column == Pagewidth)
				goto	LF;
			continue;
		}
		/*
		**  Backspace
		*/
		if (*ip == 0x08)
		{
			/*
			**  Ignore backspaces at the
			**  beginning of a line.
			*/
			TRACEP ("Backspace")
			ip++;

			if (column == 1)
				continue;

			*op++ = 0x08;

			column--;
		}
		/*
		**  Horizontal Tab
		**
		**  Expand them.
		*/
		if (*ip == 0x09)
		{
			int	motion;

			TRACEP ("Horizontal tab.")
			ip++;

			motion = TabStops - ((column-1) % TabStops);
			
			for (; motion; motion--)
			{
				*op++ = 0x20;
				column++;
				if (column == Pagewidth)
					goto	LF;
			}
			continue;
		}
		/*
		**  Carriage Return
		*/
		if (*ip == 0x0d)
		{
			/*
			**  If we are already in column 1 then
			**  ignore CR's.
			*/
			ip++;
			if (column == 1)
				continue;

			*op++ = (char) 0x0d;
			continue;
		}
		/*
		**  Line feed.
		*/
		if (*ip == 0x0a)
		{
			ip++;
LF:
			if (column == Pagewidth)
			{
				char	save;

				save = *(op-1);
				*(op-1) = (char) 0x0a;
				(void)	write (1, obuf, BUF_LEN(obuf, op));
				op = obuf;
				*op++ = save;
				column = 2;
			}
			else
			{
				*op++ = (char) 0x0a;
				(void)	write (1, obuf, BUF_LEN(obuf, op));
				op = obuf;
				column = 1;
			}
			line++;
			if (line == Pagelen && footerp)
			{
				(void)	write (1, footerp, footlen);
				line = 1;
			}
			if (line == 1)
				if (headerp)
				{
					(void)	write (1, headerp, headlen);
					line++;
				}
			continue;
		}
		/*
		**  Form feed.
		**
		**  Expand to nl's.
		*/
		if (*ip == 0x0c)
		{
			TRACEP ("Form feed.")
			ip++;
FF:
			for (;line < Pagelen; line++)
				*op++ = 0x0a;
			(void)	write (1, obuf, BUF_LEN(obuf, op));
			column = 1;
			op = obuf;
			if (footerp)
			{
				(void)	write (1, footerp, footlen);
			}
			else
			{
				(void)	write (1, "\n", 1);
			}
			line = 1;
			continue;
		}

abort_pagination:
		pagination = 0;

		if (BUF_LEN (obuf, op))
		{
			(void)	write (1, obuf, BUF_LEN (obuf, op));
		}
		(void)	write (1, ip, n - BUF_LEN(ibuf, ip));
		line = column = 0;
		CutNoSupportAuditRec ();
		break;
	} /*  for  */
} /*  while  */
	/*
	**  Flush what is left in the buffer
	*/
	if (line <= Pagelen)
	{
		for (;line < Pagelen; line++)
			*op++ = 0x0a;
		(void)	write (1, obuf, BUF_LEN(obuf, op));
		column = 1;
		op = obuf;
		if (footerp)
		{
			(void)	write (1, footerp, footlen);
		}
		else
		{
			(void)	write (1, "\n", 1);
		}
		line = 1;
	}

	return	n < 0 ? 1 : 0;
}

#ifdef	__STDC__
static	void
ParseOptions (int argc, char **argv)
#else
static	void
ParseOptions (argc, argv)

int	argc;
char	**argv;
#endif
{
	int	c,
		pagelen = 0,
		pagewidth = 0,
		tabstops = 0,
		error = 0;

	extern	char	*optarg;
	extern	int	optind, opterr, optopt;

	while ((c = getopt (argc, argv, "AE:l:t:w:")) != EOF)
	switch	(c) {
	case	'?':
		if (error || pagelen || pagewidth)
		{
			error++;
			break;
		}
		Usage (stdout);
		exit (0);

	case	'A':
		if (error || pagelen || pagewidth || tabstops
			|| ComplexStrategy || EnhanceFlag || AuditFlag)
		{
			error++;
			break;
		}
		AuditFlag++;
		break;

	case	'E':
		if (error || AuditFlag || EnhanceFlag)
		{
			error++;
			break;
		}
		if (ComplexStrategy)
		{
			(void)	strcpy (EndEnhance, optarg);
			EnhanceFlag++;
		}
		else
		if (!strcmp (optarg, "cr") || !strcmp (optarg, "\r"))
		{
			
			EnhanceFlag++;
			SimpleStrategy = '\r';
		}
		else
		if (!strcmp (optarg, "bs") || !strcmp (optarg, "\b"))
		{
			EnhanceFlag++;
			SimpleStrategy = '\b';
		}
		else
		{
			(void)	strcpy (StartEnhance, optarg);
			ComplexStrategy++;
		}
		break;

	case	'l':
		if (pagelen || AuditFlag)
		{
			error++;
			break;
		}
		if (sscanf (optarg, "%d", &pagelen) != 1)
			error++;
		else
		if (pagelen > 0)
			Pagelen = pagelen;
		else
			error++;
		break;

	case	't':
		if (tabstops || AuditFlag)
		{
			error++;
			break;
		}
		if (sscanf (optarg, "%d", &tabstops) != 1)
			error++;
		else
		if (tabstops > 1)
			TabStops = tabstops;
		else
			error++;
		break;

	case	'w':
		if (pagewidth || AuditFlag)
		{
			error++;
			break;
		}
		if (sscanf (optarg, "%d", &pagewidth) != 1)
			error++;
		else
		if (pagewidth > 0)
			Pagewidth = pagewidth;
		else
			error++;
		break;
	}
	if (ComplexStrategy && !EnhanceFlag)
	{
		/*
		**  We never got 'EndEnhance'.
		*/
		error++;
	}
	if (error)
	{
		Usage (stderr);
		exit (1);
	}
	if (argv[optind])
	{
		Jobidp = argv[optind];
	}
	else
	{
		Usage (stderr);
		exit (1);
	}
	return;
}

/*
 * Procedure:     CutTruncAuditRec
 *
 * Restrictions:
 *               CutAuditRec: None
*/
#ifdef	__STDC__
static void
CutTruncAuditRec (void)
#else
static void
CutTruncAuditRec ()
#endif
{
	int	size;
	char *	recp;

	size = strlen (Lognamep) + strlen (Jobidp) + 2;
	recp = (char *)  malloc (size);
	if (!recp)
		exit (1);
	(void)	sprintf (recp, "%s:%s", Lognamep, Jobidp);

	CutAuditRec (ADT_TRUNC_LVL, 0, size, recp);
	free (recp);
	return;
}

/*
 * Procedure:     CutNoLabelsAuditRec
 *
 * Restrictions:
 *               CutAuditRec: None
*/

#ifdef	__STDC__
static void
CutNoLabelsAuditRec (void)
#else
static void
CutNoLabelsAuditRec ()
#endif
{
	int	size;
	char *	recp;

	size = strlen (Lognamep) + strlen (Jobidp) + 2;
	recp = (char *)  malloc (size);
	if (!recp)
		exit (1);
	(void)	sprintf (recp, "%s:%s", Lognamep, Jobidp);

	CutAuditRec (ADT_PRT_LVL, 0, size, recp);
	free (recp);
	return;
}
/*
 * Procedure:     CutNoSupportAuditRec
 *
 * Restrictions:
 *               CutAuditRec: none
*/

#ifdef	__STDC__
static void
CutNoSupportAuditRec (void)
#else
static void
CutNoSupportAuditRec ()
#endif
{
	int	size;
	char *	recp;

	size = strlen (Lognamep) + strlen (Jobidp) + 2;
	recp = (char *)  malloc (size);
	if (!recp)
		exit (1);
	(void)	sprintf (recp, "%s:%s", Lognamep, Jobidp);

	CutAuditRec (ADT_PAGE_LVL, 0, size, recp);
	free (recp);
	return;
}
#ifdef	__STDC__
static char *
Enhance (char *sp)
#else
static char *
Enhance (sp)

char *sp;
#endif
{
	int	size;
	char	*p;

	if (ComplexStrategy)
	{
		size = strlen (StartEnhance) + strlen (sp) +
		       strlen (EndEnhance);

		p = (char *) calloc (1, size+1);
		if (!p)
			return	(char *)0;

		(void)	strcpy (p, StartEnhance);
		(void)	strcat (p, sp);
		(void)	strcat (p, EndEnhance);
		return	p;
	}
	else
	if (SimpleStrategy == '\r')
	{
		size = 1 + (2 * strlen (sp));
		p = (char *) calloc (1, size+1);
		if (!p)
			return	(char *)0;
		(void)	strcpy (p, sp);
		(void)	strcat (p, "\r");
		(void)	strcat (p, sp);
		return	p;
	}
	else
	if (SimpleStrategy == '\b')
	{
		register
		char	*ep;
		int	i, slen;

		size = 3 * strlen (sp);
		p = (char *) calloc (1, size+1);
		if (!p)
			return	(char *)0;

		for (ep=p, i=0, slen=strlen(sp); i < slen; i++)
		{
			*ep++ = *sp;
			*ep++ = (char) '\b';
			*ep++ = *sp++;
		}
		return	p;
	}
	return	(char *)0;
}

#ifdef	STUBS
#ifdef	__STDC__
char *
GetProcLevel (int format)
#else
char *
GetProcLevel (format)

int	format;
#endif
{
	int	size, i;
	char	buf [16],
		*p;

	static	char	*Labels[] = {"Secret","Category"};

	format = format;

	size = (int) strlen (Labels[0]) + 1;
	size += ((int) strlen (Labels[1]) + 4) * 100;

	p = (char *) calloc (1, size+1);
	if (!p)
		return	(char *)0;

	strcpy (p, Labels[0]);
	strcat (p, ":");
	for (i=1; i < 100; i++)
	{
		(void)	sprintf (buf, "%s%03d,", Labels[1], i);
		(void)	strcat (p, buf);
	}
	(void)	sprintf (buf, "%s%03d", Labels[1], i);
	(void)	strcat (p, buf);
	
	return	p;
}
#endif	/*  STUBS  */
