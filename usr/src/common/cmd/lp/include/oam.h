/*		copyright	"%c%" 	*/

#ident	"@(#)oam.h	1.2"
#ident	"$Header$"

#include <pfmt.h>
#include <stdarg.h>

char			*agettxt(
#if	defined(__STDC__)
				long	msg_id, char *	buf, int	buflen
#endif
			);

void		fmtmsg(
#if	defined(__STDC__)
				char * label,
			        int severity,
			        int seqnum, 
			        long int arraynum,
                                int logind,
                                ...
#endif
			);

void		outmsg(
#if	defined(__STDC__)
				char * label,
			        int severity,
			        int seqnum, 
			        long int arraynum,
                                int logind,
                                ...
#endif
			);

/*
 * Possible values of "severity":
 */
#define	HALT		MM_HALT
#define	ERROR		MM_ERROR
#define	WARNING		MM_WARNING
#define	INFO		MM_INFO

/*
 **
 ** LP Spooler specific error message handling.
 **/

#define	MSGSIZ		512
#define	LOG             1 
#define	NOLOG           0   

/*
 **
 ** Variable argument handling.
 **/
#define MAXARGS	31

typedef struct stva_list {
	va_list ap;
} stva_list;


#if	defined(WHO_AM_I)

#include "oam_def.h"

#if	WHO_AM_I == I_AM_CANCEL
static char		*who_am_i = "UX:cancel";

#elif	WHO_AM_I == I_AM_COMB
static char		*who_am_i = "UX:comb           ";
				  /* changed inside pgm */

#elif	WHO_AM_I == I_AM_LPMOVE
static char		*who_am_i = "UX:lpmove";

#elif	WHO_AM_I == I_AM_LPUSERS
static char		*who_am_i = "UX:lpusers";

#elif	WHO_AM_I == I_AM_LPNETWORK
static char		*who_am_i = "UX:lpnetwork";

#elif	WHO_AM_I == I_AM_LP
static char		*who_am_i = "UX:lp";

#elif	WHO_AM_I == I_AM_LPADMIN
static char		*who_am_i = "UX:lpadmin";

#elif	WHO_AM_I == I_AM_LPFILTER
static char		*who_am_i = "UX:lpfilter";

#elif	WHO_AM_I == I_AM_LPFORMS
static char		*who_am_i = "UX:lpforms";

#elif	WHO_AM_I == I_AM_LPPRIVATE
static char		*who_am_i = "UX:lpprivate";

#elif	WHO_AM_I == I_AM_LPSCHED
static char		*who_am_i = "UX:lpsched";

#elif	WHO_AM_I == I_AM_LPSHUT
static char		*who_am_i = "UX:lpshut";

#elif	WHO_AM_I == I_AM_LPSTAT
static char		*who_am_i = "UX:lpstat";

#elif	WHO_AM_I == I_AM_LPSYSTEM
static char		*who_am_i = "UX:lpsystem";

#elif	WHO_AM_I == I_AM_LPDATA
static char		*who_am_i = "UX:lpdata";

#else
static char		*who_am_i = "UX:mysterious";

#endif

#define	LP_ERRMSG(C,X) \
	fmtmsg (who_am_i, C, X, NOLOG)
			   
#define	LP_ERRMSG1(C,X,A) \
	fmtmsg (who_am_i, C, X, NOLOG, A)
			 
#define	LP_ERRMSG2(C,X,A1,A2) \
	fmtmsg (who_am_i, C, X, NOLOG, A1, A2)
			 
#define	LP_ERRMSG3(C,X,A1,A2,A3) \
	fmtmsg (who_am_i, C, X, NOLOG, A1, A2, A3)
			 
#define	LP_LERRMSG(C,X) \
	fmtmsg (who_am_i, C, X, LOG)
			   
#define	LP_LERRMSG1(C,X,A) \
	fmtmsg (who_am_i, C, X, LOG, A)
			 
#define	LP_LERRMSG2(C,X,A1,A2) \
	fmtmsg (who_am_i, C, X, LOG, A1, A2)
   
#define	LP_OUTMSG(C,X) \
	outmsg (who_am_i, C, X, NOLOG)
			   
#define	LP_OUTMSG1(C,X,A) \
	outmsg (who_am_i, C, X, NOLOG, A)
			 
#define	LP_OUTMSG2(C,X,A1,A2) \
	outmsg (who_am_i, C, X, NOLOG, A1, A2)
			 
#define	LP_OUTMSG3(C,X,A1,A2,A3) \
	outmsg (who_am_i, C, X, NOLOG, A1, A2, A3)
			 
#define	LP_OUTMSG4(C,X,A1,A2,A3,A4) \
	outmsg (who_am_i, C, X, NOLOG, A1, A2, A3, A4)
			 
#endif	/* WHO_AM_I */
