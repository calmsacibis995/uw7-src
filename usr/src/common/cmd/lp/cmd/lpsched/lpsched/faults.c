/*		copyright	"%c%" 	*/


#ident	"@(#)faults.c	1.2"
#ident	"$Header$"

#include "lpsched.h"
#define WHO_AM_I	I_AM_LPSCHED
#include "oam.h"


/**
 ** printer_fault() - RECOGNIZE PRINTER FAULT
 **/

void
#ifdef	__STDC__
printer_fault (
	register PSTATUS *	pps,
	register RSTATUS *	prs,
	char *			alert_text,
	int			err
)
#else
printer_fault (pps, prs, alert_text, err)
	register PSTATUS	*pps;
	register RSTATUS	*prs;
	char			*alert_text;
	int			err;
#endif
{
	DEFINE_FNNAME (printer_fault)

	register char		*why;


	pps->status |= PS_FAULTED;

	/*  -F wait  */
	if (STREQU(pps->printer->fault_rec, NAME_WAIT))
		(void) disable (pps, CUZ_FAULT, DISABLE_STOP);

	/*  -F beginning  */
	else if (STREQU(pps->printer->fault_rec, NAME_BEGINNING))
		terminate (pps->exec);

	/*  -F continue  AND  the interface program died  */
	else if (!(pps->status & PS_LATER) && !pps->request) {
		load_str (&pps->dis_reason, CUZ_STOPPED);
		schedule (EV_LATER, WHEN_PRINTER, EV_ENABLE, pps);
	}

	if (err) {
		errno = err;
		why = makestr(alert_text, "(", PERROR, ")\n", (char *)0);
		if (!why)
			why = alert_text;
	} else
		why = alert_text;
	alert (A_PRINTER, pps, prs, why);
	if (why != alert_text)
		Free (why);

	return;
}

/**
 ** dial_problem() - ADDRESS DIAL-OUT PROBLEM
 **/

void
#ifdef	__STDC__
dial_problem (
	register PSTATUS *	pps,
	RSTATUS *		prs,
	int			rc
)
#else
dial_problem (pps, prs, rc)
	register PSTATUS	*pps;
	RSTATUS			*prs;
	int			rc;
#endif
{
	DEFINE_FNNAME (dial_problem)

	static struct problem {
		int			retry_max,
					dial_error;
	}			problems[] = {
		10,	 2, /* D_HUNG  */
		10,	 3, /* NO_ANS  */
		 0,	 6, /* L_PROB  */
		20,	 8, /* DV_NT_A */
		 0,	10, /* NO_BD_A */
		 0,	13, /* BAD_SYS */
		 0,	0
	};

	register struct problem	*p;

	register char		*msg;
	char			*retmsg(int, long int),
				*problem_msg(int);


	for (p = problems; p->dial_error; p++)
		if (p->dial_error == rc)
			break;

	if (!p->retry_max) {
		msg = Malloc(strlen(retmsg(E_SCH_DIALPREF)) + strlen(problem_msg(p->dial_error)) + 2);
		(void) sprintf (msg, "%s%s\n", retmsg(E_SCH_DIALPREF), problem_msg(p->dial_error));
		printer_fault (pps, prs, msg, 0);
		Free (msg);

	} else if (pps->last_dial_rc != rc) {
		pps->nretry = 1;
		pps->last_dial_rc = (short)rc;

	} else if (pps->nretry++ > p->retry_max) {
		pps->nretry = 0;
		pps->last_dial_rc = (short)rc;
		msg = Malloc(
		strlen(retmsg(E_SCH_DIALPREF)) + strlen(problem_msg(p->dial_error)) + strlen(retmsg(E_SCH_DIALSUF)) + 1
		);
		(void) sprintf (msg, "%s%s%s\n", retmsg(E_SCH_DIALPREF), problem_msg(p->dial_error), retmsg(E_SCH_DIALSUF));
		printer_fault (pps, prs, msg, 0);
		Free (msg);
	}

	if (!(pps->status & PS_FAULTED)) {
		load_str (&pps->dis_reason, problem_msg(p->dial_error));
		schedule (EV_LATER, WHEN_PRINTER, EV_ENABLE, pps);
	}

	return;
}

char *
#ifdef	__STDC__
problem_msg (
	int		problem
 )
#else
problem_msg (problem)
	int		problem;
#endif
{
	DEFINE_FNNAME (problem_msg)

	char	*prob_msg,
		*retmsg (int, long int);


	switch (problem) {
	case 2:
		prob_msg = retmsg(E_SCH_DIALP0);
    		break;
	case 3:
		prob_msg = retmsg(E_SCH_DIALP1);
    		break;
	case 6:
		prob_msg = retmsg(E_SCH_DIALP2);
		break;
	case 8:
		prob_msg = retmsg(E_SCH_DIALP3);
		break;
	case 10:
		prob_msg = retmsg(E_SCH_DIALP4);
		break;
	case 13:
		prob_msg = retmsg(E_SCH_DIALP5);
		break;
	case 0:
		prob_msg = retmsg(E_SCH_DIALP6);
		break;
	default:
		prob_msg = "";
		break;
	}

	return (prob_msg);
}
