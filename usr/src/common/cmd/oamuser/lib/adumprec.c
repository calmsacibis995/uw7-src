#ident	"@(#)adumprec.c	1.2"
#ident  "$Header$"

#include <sys/types.h>
#include <sys/param.h>
#include <audit.h>

/*
 * USER level interface to auditdmp(2)
 * for USER level audit event records
 */
void
adumprec(rtype,status,size,argp)
int rtype;			/* event type */
int status;			/* event status */
int size;			/* size of argp */
char *argp;			/* data/arguments */
{
        arec_t rec;

        rec.rtype = rtype;
        rec.rstatus = status;
        rec.rsize = size;
        rec.argp = argp;

        auditdmp(&rec, sizeof(arec_t));
        return;
}
