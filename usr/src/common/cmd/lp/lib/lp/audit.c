/*		copyright	"%c%" 	*/


#ident	"@(#)audit.c	1.2"
#ident	"$Header$"

#include <sys/param.h>
#include <audit.h>

#ifdef	__STDC__
void
CutAuditRec (int type, int status, int size , char *msgp)
#else
void
CutAuditRec (type, status, size, msgp)

int	type;	/* event type */
int	status;	/* event exit status */
int	size;	/* size of msgp */
char *	msgp;	/* message */
#endif
{
        arec_t	rec;		/* auditdmp(2) structure */

	rec.rtype	= type;
	rec.rstatus	= status;
	rec.rsize	= size;
	rec.argp	= msgp;

	auditdmp (&rec, sizeof (arec_t));

        return;
}
