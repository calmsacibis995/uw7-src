/*		copyright	"%c%" 	*/


#ident	"@(#)alerts.c	1.2"
#ident  "$Header$"

#include "lpsched.h"
#define WHO_AM_I	I_AM_LPSCHED
#include "oam.h"

#ifdef	__STDC__
#include "stdarg.h"
#else
#include "varargs.h"
#endif



static void		pformat(),
			pwformat(),
			fformat();

static int		f_count(),
			p_count();

/*
 * Procedure:     alert
 *
 * Restrictions:
 *               Unlink: None
*/
/*VARARGS1*/
#ifdef	__STDC__
void
alert (int type, ...)
#else
void
alert (type, va_alist)

int	type;
va_dcl
#endif
{
	DEFINE_FNNAME (alert)

	PSTATUS		*pr;
	RSTATUS		*rp;
	FSTATUS		*fp;
	PWSTATUS		*pp;
	char		*text;
	va_list		args;

#ifdef		__STDC__
	va_start (args, type);
#else
	va_start (args);
#endif

	ENTRYP
	switch (type) {
	case A_PRINTER:
		TRACEP ("case A_PRINTER")
		pr = va_arg(args, PSTATUS *);
		rp = va_arg(args, RSTATUS *);
		text = va_arg(args, char *);
		TRACEx (pr) TRACEx (rp) TRACEs (text)
		pformat(pr->alert->msgfile, text, pr, rp);
		TRACEx (pr->alert->active)
		if (!pr->alert->active)
		{
			if (exec(EX_ALERT, pr) == 0)
				pr->alert->active = 1;
			else
			{
				if (errno == EBUSY)
					pr->alert->exec->flags |= EXF_RESTART;
				else
					(void) Unlink(pr->alert->msgfile);
			}
		}
		TRACEP ("case A_PRINTER break")
		break;

	case A_PWHEEL:
		TRACEP ("case A_PWHEEL")
		pp = va_arg(args, PWSTATUS *);
		TRACEx (pp->alert)
		pwformat(pp->alert->msgfile, pp);
		if (!pp->alert->active)
		{
			if (exec(EX_PALERT, pp) == 0)
				pp->alert->active = 1;
			else
			{
				if (errno == EBUSY)
					pp->alert->exec->flags |= EXF_RESTART;
				else
					(void) Unlink(pp->alert->msgfile);
			}
		}
		TRACEP ("case A_PWHEEL-break")
		break;

	case A_FORM:
		TRACEP ("case A_FORM")
		fp = va_arg(args, FSTATUS *);
		fformat(fp->alert->msgfile, fp);
		if (!fp->alert->active)
		{
			if (exec(EX_FALERT, fp) == 0)
				fp->alert->active = 1;
			else
			{
				if (errno == EBUSY)
					fp->alert->exec->flags |= EXF_RESTART;
				else
					(void) Unlink(fp->alert->msgfile);
			}
		}
		break;
	}
	va_end(args);
}

/*
 * Procedure:     pformat
 *
 * Restrictions:
 *               Access: None
 *               open_lpfile: None
 *               fputs: None
 *               close_lpfile: None
 *               fprintf: None
*/
static void
pformat(file, text, pr, rp)
char	*file;
char	*text;
PSTATUS	*pr;
RSTATUS	*rp;
{
	DEFINE_FNNAME (pformat)

	FILE	*filep;
	char	*retmsg (int,  long int);

	if (Access(pr->alert->msgfile, 0) == 0)
	{
		if ((filep = open_lpfile(file, "a", MODE_READ)) == NULL)
			return;
		if (text)
			(void) fputs(text, filep);
		(void) close_lpfile(filep);
	}
	else
	{
		if ((filep = open_lpfile(file, "w", MODE_READ)) == NULL)
			return;
		(void) fprintf(filep, retmsg(E_SCH_PFMSG0), NB(pr->printer->name),
			NB(pr->printer->name));
		if (STREQU(pr->printer->fault_rec, NAME_WAIT))
			(void) fputs(retmsg(E_SCH_PFMSG2), filep);
		else
		{
			if (pr->exec->pid > 0)
				(void) fputs(retmsg(E_SCH_PFMSG1), filep);
			else
			if (rp)
				(void) fprintf(filep, retmsg(E_SCH_PFMSG3), rp->secure->req_id);
		}
		(void) fprintf(filep, retmsg(E_SCH_PFMSG4));
		if (text)
		{
			while (*text == '\n' || *text == '\r')
			text++;
			(void) fprintf(filep, "%s", text);
		}
		(void) close_lpfile(filep);
	}
}

/*
 * Procedure:     pwformat
 *
 * Restrictions:
 *               open_lpfile: None
 *               fprintf: None
 *               close_lpfile: None
*/
static void
pwformat(file, pp)

char		*file;
PWSTATUS	*pp;
{
	DEFINE_FNNAME (pwformat)

	FILE	*filep;
	PSTATUS	*p;
	char	*retmsg (int, long int);

/*
 * This check doesn't cover the case that a request has been moved
 * to a different destination.
	if (Access(pp->alert->msgfile, 0) != 0
		|| pp->requests != pp->requests_last)
 */
	{
		if ((filep = open_lpfile(file, "w", MODE_READ)) == NULL)
			return;
		(void) fprintf(filep, retmsg(E_SCH_PAMSG0), NB(pp->pwheel->name),
			NB(pp->pwheel->name));
		for (p = walk_ptable(1); p; p = walk_ptable(0))
			if (p->printer->daisy
				&& !SAME(p->pwheel_name, pp->pwheel->name)
				&& searchlist(pp->pwheel->name,
					p->printer->char_sets))
			{
				register int	n = p_count(pp,
					p->printer->name);

				if (n)
					(void) fprintf(filep, retmsg(E_SCH_PAMSG1),
					p->printer->name, n);
			}
		(void) fprintf(filep, retmsg(E_SCH_PAMSG2), pp->requests);
		(void) close_lpfile(filep);
		pp->requests_last = pp->requests;
	}
}

/*
 * Procedure:     fformat
 *
 * Restrictions:
 *               open_lpfile: None
 *               fprintf: None
 *               fputs: None
 *               close_lpfile: None
*/
static void
fformat(file, fp)
char	*file;
FSTATUS	*fp;
{
	DEFINE_FNNAME (fformat)

	FILE		*filep;
	PSTATUS		*p;
	char		*retmsg (int, long int);

/*
 * This check doesn't cover the case that a request has been moved
 * to a different destination, or the ribbon or character set have
 * changed.
	if (Access(fp->alert->msgfile, 0) != 0
		|| fp->requests != fp->requests_last)
 */
	{
		if ((filep = open_lpfile(file, "w", MODE_READ)) == NULL)
			return;
		(void) fprintf(filep, retmsg(E_SCH_FAMSG0), NB(fp->form->name),
			NB(fp->form->name));
		for (p = walk_ptable(1); p; p = walk_ptable(0))
			if (p->form != fp
				&& allowed(fp->form->name,
				p->forms_allowed, p->forms_denied))
			{
				register int	n = f_count(fp,
					p->printer->name);

				if (n)
					(void) fprintf(filep, retmsg(E_SCH_FAMSG1),
					p->printer->name, n);
			}
		(void) fprintf(filep, retmsg(E_SCH_FAMSG2), fp->requests);
		if (fp->form->rcolor && !STREQU(fp->form->rcolor, NAME_ANY))
			(void) fprintf(filep, retmsg(E_SCH_FAMSG3), NB(fp->form->rcolor));
		else
			(void) fputs(retmsg(E_SCH_FAMSG4), filep);
		if (fp->form->chset && !STREQU(fp->form->chset, NAME_ANY))
			(void) fprintf(filep, retmsg(E_SCH_FAMSG5), NB(fp->form->chset));
		else
			(void) fputs(retmsg(E_SCH_FAMSG6), filep);
		(void) close_lpfile(filep);
		fp->requests_last = fp->requests;
	}
}

/*
 * Procedure:     cancel_alert
 *
 * Restrictions:
 *               Unlink: None
*/

/* VARARGS1 */
#ifdef	__STDC__
void
cancel_alert (int type, ...)
#else
void
cancel_alert (type, va_alist)
int	type;
va_dcl
#endif
{
	DEFINE_FNNAME (cancel_alert)

	ALERT		*ap;
	va_list		args;

#ifdef		__STDC__
	va_start (args, type);
#else
	va_start (args);
#endif

	switch (type)
	{
		case A_PRINTER:
		ap = va_arg(args, PSTATUS *)->alert;
		break;

		case A_PWHEEL:
		ap = va_arg(args, PWSTATUS *)->alert;
		break;

		case A_FORM:
		ap = va_arg(args, FSTATUS *)->alert;
		break;

		default:
		return;
	}
	va_end(args);

	ap->active = 0;
	terminate(ap->exec);
	(void) Unlink(ap->msgfile);
	return;
}

static int
#ifdef		__STDC__
dest_equivalent_printer (
		char *						dest,
		char *						printer
)
#else
dest_equivalent_printer (dest, printer)
		char *						dest;
		char *						printer;
#endif
{
		DEFINE_FNNAME (dest_equivalent_printer)

		CSTATUS *				pc;

		return (
				STREQU(dest, printer)
		|| STREQU(dest, NAME_ANY)
		|| (
						(pc = search_ctable(dest))
				&& searchlist(printer, pc->class->members)
				)
		);
}

static int
f_count(fp, name)
register FSTATUS 		*fp;
register char				*name;
{
	DEFINE_FNNAME (f_count)

	register int				count = 0;
	register RSTATUS				*rp;

	BEGIN_WALK_BY_FORM_LOOP(rp, fp)
		if (dest_equivalent_printer(rp->request->destination, name))
		count++;
	END_WALK_LOOP
	if (
		NewRequest
	 && NewRequest->form == fp
	 && dest_equivalent_printer(NewRequest->request->destination, name)
	)
		count++;
	return(count);
}

static int
p_count(pp, name)
register PWSTATUS 		*pp;
register char				*name;
{
	DEFINE_FNNAME (p_count)

	register int				count = 0;
	register RSTATUS				*rp;

	BEGIN_WALK_LOOP(rp, rp->pwheel == pp)
		if (dest_equivalent_printer(rp->request->destination, name))
		count++;
	END_WALK_LOOP
	if (
		NewRequest
	 && NewRequest->pwheel == pp
	 && dest_equivalent_printer(NewRequest->request->destination, name)
	)
		count++;
	return(count);
}
