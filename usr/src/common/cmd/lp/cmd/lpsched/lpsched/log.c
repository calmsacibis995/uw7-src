/*		copyright	"%c%" 	*/


#ident	"@(#)log.c	1.2"
#ident  "$Header$"

#ifdef	__STDC__
#include "stdarg.h"
#else
#include "varargs.h"
#endif

#include "lpsched.h"

#define WHO_AM_I        I_AM_LPSCHED
#include "oam.h"

#include <locale.h>

#ifdef	__STDC__
static void		log ( char * , va_list );
static void		lplog ( int, int, long int, va_list );
#else
static void		log();
static void		lplog();
#endif

/*
 * Procedure:     open_logfile
 *
 * Restrictions:
 *               fopen: None
 *               Chown: None
 * open_logfile() - OPEN FILE FOR LOGGING MESSAGE
 * close_logfile() - CLOSE SAME
 */

FILE *
#ifdef	__STDC__
open_logfile (
	char *			name
)
#else
open_logfile (name)
	char			*name;
#endif
{
	DEFINE_FNNAME (open_logfile)

	register char		*path;

	register FILE		*fp;


#ifdef	MALLOC_3X
	/*
	 * Don't rely on previously allocated pathnames.
	 */
#endif
	path = makepath(Lp_Logs, name, (char *)0);
	fp = fopen(path, "a");
	(void) Chown (path, Lp_Uid, Lp_Gid);
	Free (path);
	return (fp);
}

/*
 * Procedure:     close_logfile
 *
 * Restrictions:
 *               fclose: None
*/
void
#ifdef	__STDC__
close_logfile (
	FILE *			fp
)
#else
close_logfile (fp)
	FILE			*fp;
#endif
{
	DEFINE_FNNAME (close_logfile)

	(void) fclose (fp);
	return;
}

/**
 ** fail() - LOG MESSAGE AND EXIT (ABORT IF DEBUGGING)
 **/

/*VARARGS1*/
void
#ifdef	__STDC__
fail (
	char *			format,
	...
)
#else
fail (format, va_alist)
	char			*format;
	va_dcl
#endif
{
	DEFINE_FNNAME (fail)

	va_list			ap;
    
#ifdef	__STDC__
	va_start (ap, format);
#else
	va_start (ap);
#endif
	log (format, ap);
	va_end (ap);

#ifdef	DEBUG
	if (debug & DB_ABORT)
		abort ();
	else
#endif
		exit (1);
	/*NOTREACHED*/
}

/**
 ** lpfail() - LOG INTERNATIONALIZED MESSAGE AND EXIT (ABORT IF DEBUGGING)
 **/

/*VARARGS1*/
void
#ifdef	__STDC__
lpfail (
        int                     severity,
        int                     seqnum,
        long int                arraynum,
	...
)
#else
lpfail (severity, seqnum, arraynum, va_alist)
        int                     severity;
        int                     seqnum;
        long int                arraynum;
	va_dcl
#endif
{
	DEFINE_FNNAME (lpfail)

	va_list			ap;
    
#ifdef	__STDC__
	va_start (ap, arraynum);
#else
	va_start (ap);
#endif
	lplog (severity, seqnum, arraynum, ap);
	va_end (ap);

#ifdef	DEBUG
	if (debug & DB_ABORT)
		abort ();
	else
#endif
		exit (1);
	/*NOTREACHED*/
}

/**
 ** note() - LOG MESSAGE
 **/

/*VARARGS1*/
void
#ifdef	__STDC__
note (
	char *			format,
	...
)
#else
note (format, va_alist)
	char			*format;
	va_dcl
#endif
{
	DEFINE_FNNAME (note)

	va_list			ap;

#ifdef	__STDC__
	va_start (ap, format);
#else
	va_start (ap);
#endif
	log (format, ap);
	va_end (ap);
	return;
}

/**
 ** lpnote() - LOG AN INTERNATIONALIZED MESSAGE
 **/

/*VARARGS1*/
void
#ifdef	__STDC__
lpnote (
        int                     severity,
        int                     seqnum,
        long int                arraynum,
	...
)
#else
lpnote (severity, seqnum, arraynum, va_alist)
        int                     severity;
        int                     seqnum;
        long int                arraynum;
	va_dcl
#endif
{
	DEFINE_FNNAME (lpnote)

	va_list			ap;

#ifdef	__STDC__
	va_start (ap, arraynum);
#else
	va_start (ap);
#endif
	lplog (severity, seqnum, arraynum, ap);
	va_end (ap);
	return;
}

/**
 ** schedlog() - LOG MESSAGE IF IN DEBUG MODE
 **/

/*VARARGS1*/
void
#ifdef	__STDC__
schedlog (
	char *			format,
	...
)
#else
schedlog (format, va_alist)
	char			*format;
	va_dcl
#endif
{
	DEFINE_FNNAME (schedlog)

	va_list			ap;

#ifdef	DEBUG
	if (debug & DB_SCHEDLOG) {

#ifdef	__STDC__
		va_start (ap, format);
#else
		va_start (ap);
#endif
		log (format, ap);
		va_end (ap);

	}
#endif
	return;
}

/**
 ** mallocfail() - COMPLAIN ABOUT MEMORY ALLOCATION FAILURE
 **/

void
#ifdef	__STDC__
mallocfail (
	void
)
#else
mallocfail ()
#endif
{
	DEFINE_FNNAME (mallocfail)

	lpfail (ERROR, E_SCH_MEMFAILED);
	/*NOTREACHED*/
}

/**
 ** log() - LOW LEVEL ROUTINE THAT LOGS MESSSAGES
 **/

static void
#ifdef	__STDC__
log (
	char *			format,
	va_list			ap
)
#else
log (format, ap)
	char			*format;
	va_list			ap;
#endif
{
	DEFINE_FNNAME (log)

	int			close_it;

	FILE			*fp;

	static int		nodate	= 0;


	if (!am_in_background) {
		fp = stdout;
		close_it = 0;
	} else {
		if (!(fp = open_logfile("lpsched")))
			return;
		close_it = 1;
	}

	if (am_in_background && !nodate) {
		long			now;

		(void) time (&now);
		(void) fprintf (fp, "%24.24s: ", ctime(&now));
	}
	nodate = 0;

	(void) vfprintf (fp, format, ap);
	if (format[strlen(format) - 1] != '\n')
		nodate = 1;

	if (close_it)
		close_logfile (fp);
	else
		(void) fflush (fp);

	return;
}

/**
 ** lplog() - LOW LEVEL ROUTINE THAT LOGS INTERNATIONALIZED MESSSAGES
 **/

static void
#ifdef	__STDC__
lplog (
        int			severity,
	int			seqnum,
	long int		arraynum,
	va_list			ap
)
#else
lplog (severity, seqnum, arraynum, ap)
        int			severity;
	int			seqnum;
	long int		arraynum;
	va_list			ap;
#endif
{
	DEFINE_FNNAME (lplog)

	int			close_it;

	FILE			*fp;

	static int		nodate	= 0;

        char                    buf[MSGSIZ];
        char                    text[MSGSIZ];
        char                    msg_text[MSGSIZ];

	if (!am_in_background) {
		fp = stdout;
		close_it = 0;
	} else {
		if (!(fp = open_logfile("lpsched")))
			return;
		close_it = 1;
	}

	if (am_in_background && !nodate) {
		long			now;

		(void) time (&now);
		(void) fprintf (fp, "%24.24s: ", ctime(&now));
	}
	nodate = 0;
        (void)setlocale(LC_ALL, "");
        setcat("uxlp");
        setlabel(who_am_i);
        (void) sprintf(msg_text,":%d:%s\n",seqnum,agettxt(arraynum,buf,MSGSIZ));
        vpfmt(fp,severity,msg_text,ap);
	if (msg_text[strlen(msg_text) - 1] != '\n')
		nodate = 1;
        (void) sprintf(text,"%s",agettxt(arraynum + 1,buf,MSGSIZ));
        if (strncmp(text, "", 1) != 0)  {
           (void) sprintf(msg_text,":%d:%s\n",seqnum + 1,text);
           pfmt(fp,MM_ACTION,msg_text);
        }

	if (close_it)
		close_logfile (fp);
	else
		(void) fflush (fp);

	return;
}
   
/**
 ** execlog()
 **/

/*VARARGS1*/
void
#ifdef	__STDC__
execlog (
	char *			format,
	...
)
#else
execlog (format, va_alist)
	char			*format;
	va_dcl
#endif
{
	DEFINE_FNNAME (execlog)

	va_list			ap;

#ifdef	DEBUG
	FILE			*fp	= open_logfile("exec");

	time_t			now = time((time_t *)0);

	char			buffer[BUFSIZ];

	EXEC *			ep;

	static int		nodate	= 0;

#ifdef	__STDC__
	va_start (ap, format);
#else
	va_start (ap);
#endif
	if (fp) {
		setbuf (fp, buffer);
		if (!nodate)
			(void) fprintf (fp, "%24.24s: ", ctime(&now));
		nodate = 0;
		if (!STREQU(format, "%e")) {
			(void) vfprintf (fp, format, ap);
			if (format[strlen(format) - 1] != '\n')
				nodate = 1;
		} else switch ((ep = va_arg(ap, EXEC *))->type) {
		case EX_INTERF:
			(void) fprintf (
				fp,
				"      EX_INTERF %s %s\n",
				ep->ex.printer->printer->name,
				ep->ex.printer->request->secure->req_id
			);
			break;
		case EX_SLOWF:
			(void) fprintf (
				fp,
				"      EX_SLOWF %s\n",
				ep->ex.request->secure->req_id
			);
			break;
		case EX_ALERT:
			(void) fprintf (
				fp,
				"      EX_ALERT %s\n",
				ep->ex.printer->printer->name
			);
			break;
		case EX_FALERT:
			(void) fprintf (
				fp,
				"      EX_FALERT %s\n",
				ep->ex.form->form->name
			);
			break;
		case EX_PALERT:
			(void) fprintf (
				fp,
				"      EX_PALERT %s\n",
				ep->ex.pwheel->pwheel->name
			);
			break;
		case EX_NOTIFY:
			(void) fprintf (
				fp,
				"      EX_NOTIFY %s\n",
				ep->ex.request->secure->req_id
			);
			break;
		default:
			(void) fprintf (fp, "      EX_???\n");
			break;
		}
		close_logfile (fp);
	}
#endif
	return;
}
