/*		copyright	"%c%" 	*/


#ident	"@(#)notify.c	1.2"
#ident  "$Header$"

#include "lpsched.h"
#define WHO_AM_I	I_AM_LPSCHED
#include "oam.h"


static	short 			N_Reason[] = {
	MNODEST,
	MERRDEST,
	MDENYDEST,
	MDENYDEST,
	MNOMEDIA,
	MDENYMEDIA,
	MDENYMEDIA,
	MNOFILTER,
	MNOMOUNT,
	MNOSPACE,
	MNOOPEN,
	MNORIGHTS,
	MSEELOG,
	MNOATTACH,
	MNOLOGIN,
	-1,
};
	
	
#ifdef	__STDC__
static void		print_reason ( FILE * , int );
#else
static void             print_reason();
#endif

/*
 * Procedure:     notify
 *
 * Restrictions:
 *               open_lpfile: None
 *               close_lpfile: None
 * Notes - NOTIFY USER OF FINISHED REQUEST
 */
	
void
#ifdef	__STDC__
notify (
	register RSTATUS *	prs,
	char *			errbuf,
	int			k,
	int			e,
	int			slow
)
#else
notify (prs, errbuf, k, e, slow)
	register RSTATUS        *prs;
	char                    *errbuf;
	int                     k,
				e,
				slow;
#endif
{
	DEFINE_FNNAME (notify)

	register char		*cp;

	char			*file,
				*retmsg(int, long int);

	FILE                    *fp;


	ENTRYP
	/*
	 * Screen out cases where no notification is needed.
	 */
	if (!(prs->request->outcome & RS_NOTIFY))
		return;
	if (
		!(prs->request->actions & (ACT_MAIL|ACT_WRITE|ACT_NOTIFY))
	     && !prs->request->alert
	     && !(prs->request->outcome & RS_CANCELLED)
	     && !e && !k && !errbuf       /* exited normally */
	)
		return;

	/*
	 * Create the notification message to the user.
	 */
	file = makereqerr(prs);
	if ((fp = open_lpfile(file, "w", MODE_NOREAD))) {
		if (prs->request->outcome & RS_PRINTED)
		(void) fprintf (
			fp,
			retmsg(E_SCH_NMSG0),
			prs->secure->req_id,
			prs->secure->req_id,
			prs->request->destination,
				prs->printer->printer->name
		);
	

		if (prs->request->outcome & RS_CANCELLED)
			if (!(prs->request->actions & 
			     (ACT_MAIL|ACT_WRITE|ACT_NOTIFY)))
			(void) fprintf (
				fp,
				(prs->request->outcome & RS_FAILED)?
				retmsg(E_SCH_NMSG11) : retmsg(E_SCH_NMSG1),
				prs->secure->req_id,
				prs->secure->req_id,
				prs->request->destination
			);
			else
			(void) fprintf (
				fp,
				(prs->request->outcome & RS_FAILED)?
				retmsg(E_SCH_NMSG12) : retmsg(E_SCH_NMSG10),
				prs->secure->req_id,
				prs->secure->req_id,
				prs->request->destination
			);
		if (prs->request->outcome & RS_FAILED) {
			if (slow)
		(void) fprintf (
			fp,
			retmsg(E_SCH_NMSG2),
			prs->secure->req_id,
			prs->secure->req_id,
			prs->request->destination
			);
			else
				(void) fprintf (
					fp,
					retmsg(E_SCH_NMSG3),
					prs->secure->req_id,
					prs->secure->req_id,
					prs->request->destination,
					prs->printer->printer->name
				);
	
			if (e > 0)
				(void) fprintf (fp, (slow ? retmsg(E_SCH_NMSG4) : retmsg(E_SCH_NMSG5)), e);
			else if (k)
				(void) fprintf (fp, (slow ? retmsg(E_SCH_NMSG6) : retmsg(E_SCH_NMSG7)), k);
		}
	
		if (errbuf) {
			for (cp = errbuf; *cp && *cp == '\n'; cp++)
				;
			(void) fprintf (fp, retmsg(E_SCH_NMSG8), cp);
			if (prs->request->outcome & RS_CANCELLED)
				(void) fputs ("\n", fp);
		}

		if (prs->request->outcome & RS_CANCELLED)
			print_reason (fp, prs->reason);

		(void) close_lpfile (fp);
		schedule (EV_NOTIFY, prs);

	}
	if (file)
		Free (file);

	EXITP
	return;
}

/*
 * Procedure:     print_reason
 *
 * Restrictions:
 *               fprintf: None
 *               fputs: None
 * Notes - PRINT REASON FOR AUTOMATIC CANCEL
 */

static void
#ifdef	__STDC__
print_reason (
	FILE *			fp,
	int			reason
 )
#else
print_reason (fp, reason)
	FILE			*fp;
	register int		reason;
#endif
{
	DEFINE_FNNAME (print_reason)

	register int		i;
	char			*msg;
	char			*retmsg(int, long int);
	char			*reason_msg(short);


#define P(BIT,MSG)	if (chkprinter_result & BIT) (void) fputs (MSG, fp)

	for (i = 0; N_Reason[i] != -1; i++)
		if (N_Reason[i] == reason) {
			if (reason == MDENYDEST && chkprinter_result)
				i++;
			if (reason == MDENYMEDIA && chkprinter_result)
				i++;
			msg = Malloc(strlen(retmsg(E_SCH_NMSG9))+2);
			(void) sprintf (msg, "%s", retmsg(E_SCH_NMSG9));
			(void) fprintf (fp, msg, reason_msg(i));
			(void) Free (msg);
			if (reason == MDENYDEST && chkprinter_result) {
				P (PCK_TYPE,	retmsg(E_LP_PCK_TYPE));
				P (PCK_CHARSET,	retmsg(E_LP_PCK_CHARSET));
				P (PCK_CPI,	retmsg(E_LP_PCK_CPI));
				P (PCK_LPI,	retmsg(E_LP_PCK_LPI));
				P (PCK_WIDTH,	retmsg(E_LP_PCK_WIDTH));
				P (PCK_LENGTH,	retmsg(E_LP_PCK_LENGTH));
				P (PCK_BANNER,	retmsg(E_LP_PCK_BANNER));
				P (PCK_LOCALE,	retmsg(E_LP_PCK_LOCALE));
			}
			break;
		}

	return;
}

char *
#ifdef	__STDC__
reason_msg (
	short			reason
 )
#else
reason_msg ( reason)
	short		reason;
#endif
{
	DEFINE_FNNAME (reason_msg)

	char	*reas_msg,
		*retmsg (int, long int);


	switch (reason) {
	case 0 /*MNODEST*/:
		reas_msg = retmsg(E_SCH_REAS0);
    		break;
	case 1 /*MERRDEST*/:
		reas_msg = retmsg(E_SCH_REAS1);
    		break;
	case 2 /*MDENYDEST*/:
		reas_msg = retmsg(E_SCH_REAS2);
		break;
	case 3 /*MDENYDEST*/:
		reas_msg = retmsg(E_SCH_REAS3);
		break;
	case 4 /*MNOMEDIA*/:
		reas_msg = retmsg(E_SCH_REAS4);
		break;
	case 5 /*MDENYMEDIA*/:
		reas_msg = retmsg(E_SCH_REAS5);
		break;
	case 6 /*MDENYMEDIA*/:
		reas_msg = retmsg(E_SCH_REAS6);
		break;
	case 7 /*MNOFILTER*/:
		reas_msg = retmsg(E_SCH_REAS7);
		break;
	case 8 /*MNOMOUNT*/:
		reas_msg = retmsg(E_SCH_REAS8);
		break;
	case 9 /*MNOSPACE*/:
		reas_msg = retmsg(E_SCH_REAS9);
		break;
	case 10 /*MNOOPEN*/:
		reas_msg = retmsg(E_SCH_REAS10);
		break;
	case 11 /*MNORIGHTS*/:
		reas_msg = retmsg(E_SCH_REAS11);
		break;
	case 12 /*MSEELOG*/:
		reas_msg = retmsg(E_SCH_REAS12);
		break;
	case 13 /*MNOATTACH*/:
		reas_msg = retmsg(E_SCH_REAS13);
		break;
	case 14 /*MNOLOGIN*/:
		reas_msg = retmsg(E_SCH_REAS14);
		break;
	default:
		reas_msg = "";
		break;
	}

	return (reas_msg);
}
