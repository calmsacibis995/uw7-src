#ident	"@(#)callbacks.c	11.1"

/*
 * skeleton c-client callbacks for use by our daemons
 */

#include <stdio.h>
/* the c-client includes */
#include "c-client/mail.h"
#include "c-client/osdep.h"
#include "c-client/rfc822.h"
#include "c-client/misc.h"

/*
 * all of the callbacks for c-client
 * we only fill in the ones needed for appending, which are:
 *	logging errors
 *	coping with disk write errors
 */

void mm_log (char *string,long errflg)
{
	printf("log %s\n", string);
}

void mm_notify (MAILSTREAM *stream,char *string,long errflg)
{
	printf("notify %s\n", string);
}

long mm_diskerror (MAILSTREAM *stream,long errcode,long serious)
{
	printf("diskerror %d\n", errcode);
	return T;
}

void mm_fatal (char *string)
{
	printf("fatal %s\n", string);
}

void mm_searched (MAILSTREAM *stream,unsigned long number) {}
void mm_exists (MAILSTREAM *stream,unsigned long number) {}
void mm_expunged (MAILSTREAM *stream,unsigned long number) {}
void mm_flags (MAILSTREAM *stream,unsigned long number) {}
void mm_list (MAILSTREAM *stream,char delimiter,char *name,long attributes) {}
void mm_lsub (MAILSTREAM *stream,char delimiter,char *name,long attributes) {}
void mm_status (MAILSTREAM *stream,char *mailbox,MAILSTATUS *status) {}
void mm_dlog (char *string) {}
void mm_login (NETMBX *mb,char *user,char *pwd,long trial) {}
void mm_critical (MAILSTREAM *stream) {}
void mm_nocritical (MAILSTREAM *stream) {}
