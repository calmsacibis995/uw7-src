/*		copyright	"%c%" 	*/

#ident	"@(#)dumpfilters.c	1.2"
#ident	"$Header$"

#include "stdio.h"
#include "string.h"

#include "lp.h"
#include "filters.h"
#include "printers.h"

#if	defined(__STDC__)
static void		q_print ( FILE * , char * );
#else
static void		q_print();
#endif

/**
 ** dumpfilters() - WRITE FILTERS FROM INTERNAL STRUCTURE TO FILTER TABLE
 **/

int
#if	defined(__STDC__)
dumpfilters (
	char *			file
)
#else
dumpfilters (file)
	char			*file;
#endif
{
	register _FILTER	*pf;

	register TEMPLATE	*pt;

	register TYPE		*pty;

	register char		*p,
				*sep;

	register int		fld;

	FILE			*fp;

	level_t			lid;

	int			n, freeit;

	if (!(fp = open_filtertable(file, "w")))
		return (-1);

	printlist_setup ("", "", LP_SEP, "");
	if (filters) for (pf = filters; pf->name; pf++) {

		for (fld = 0; fld < FL_MAX; fld++) switch (fld) {
		case FL_IGN:
			break;
		case FL_NAME:
			p = pf->name;
			goto String;
		case FL_CMD:
			p = pf->command;
String:			(void)fprintf (fp, "%s%s", FL_SEP, (p? p : ""));
			break;
		case FL_TYPE:
			(void)fprintf (
				fp,
				"%s%s",
				FL_SEP,
				(pf->type == fl_fast? FL_FAST : FL_SLOW)
			);
			break;
		case FL_PTYPS:
			pty = pf->printer_types;
			goto Types;
		case FL_ITYPS:
			pty = pf->input_types;
			goto Types;
		case FL_OTYPS:
			pty = pf->output_types;
Types:			(void)fprintf (fp, "%s", FL_SEP);
			sep = "";
			if (pty) {
				for (; pty->name; pty++) {
					(void)fprintf (
						fp,
						"%s%s",
						sep,
						pty->name
					);
					sep = ",";
				}
			} else
				(void)fprintf (fp, "%s", NAME_ANY);
			break;
		case FL_PRTRS:
			(void)fprintf (fp, "%s", FL_SEP);
			if (pf->printers)
				printlist (fp, pf->printers);
			else
				(void)fprintf (fp, "%s", NAME_ANY);
			break;
		case FL_TMPS:
			(void)fprintf (fp, "%s", FL_SEP);
			sep = "";
			if ((pt = pf->templates))
				for(; pt->keyword; pt++) {
					(void)fprintf (
						fp,
						"%s%s ",
						sep,
						pt->keyword
					);
					q_print (fp, pt->pattern);
					(void)fprintf (fp, " = ");
					q_print (fp, pt->result);
					sep = ",";
				}
			break;
		}
		(void)fprintf (fp, FL_END);
	}

	close_filtertable (fp);
	lid = PR_SYS_PUBLIC;
	if (!file) {
		if (!(file = getfilterfile(FILTERTABLE)))
			return -1;
		freeit = 1;
	} else
		freeit = 0;

	while ((n=lvlfile (file, MAC_SET, &lid)) < 0 && errno == EINTR)
	    continue;

	if (freeit)
	    Free (file);
	if (n < 0 && errno != ENOSYS)
	    return -1;

	return 0;
}

/**
 ** q_print() - PRINT STRING, QUOTING SEPARATOR CHARACTERS
 **/

static void
#if	defined(__STDC__)
q_print (
	FILE *			fp,
	char *			str
)
#else
q_print (fp, str)
	register FILE		*fp;
	register char		*str;
#endif
{
	/*
	 * There are four reasons to quote a character: It is
	 * a quote (backslash) character, it is a field separator,
	 * it is a list separator, or it is a template separator.
	 * "loadfilters()" strips the quote (backslash), but not
	 * in one place.
	 */
	if (!str)
		return;

	while (*str) {
		if (
			*str == '\\'		/* quote reason #1 */
		     || strchr(FL_SEP, *str)	/* quote reason #2 */
		     || strchr(LP_SEP, *str)	/* quote reason #3 */
		     || strchr("=", *str)	/* quote reason #4 */
		)
			(void)putc ('\\', fp);
		(void)putc (*str, fp);
		str++;
	}
	return;
}
