#ident	"@(#)callbacks.c	11.1"

/*
 * skeleton c-client callbacks for use by our daemons
 */

/* the c-client includes */
#include <pwd.h>
#include "../c-client/mail.h"
/* for mailx purposes */
#include "rcv.h"
/*
#include "osdep.h"
#include "rfc822.h"
#include "misc.h"
*/

extern unsigned long ulgMsgCnt, ulgNewMsgCnt, ulgUnseenMsgCnt;

/*
 * all of the callbacks for c-client
 * we only fill in the ones needed for appending, which are:
 *	logging errors
 *	coping with disk write errors
 */
void mm_log (char *string,long errflg)
{
    if (debug)
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
	printf("FATAL: %s\n", string);
	panic(":335:Internal error");
}

void mm_searched (MAILSTREAM *stream,unsigned long number) {}
void mm_exists (MAILSTREAM *stream,unsigned long number)
{
	ulgMsgCnt = number;
	ulgUnseenMsgCnt = 0;
	ulgNewMsgCnt = 0;
}
void mm_expunged (MAILSTREAM *stream,unsigned long number) {}
void mm_flags (MAILSTREAM *stream,unsigned long number) {}
void mm_list (MAILSTREAM *stream,char delimiter,char *name,long attributes) {}
void mm_lsub (MAILSTREAM *stream,char delimiter,char *name,long attributes) {}
void mm_status (MAILSTREAM *stream,char *mailbox,MAILSTATUS *status)
{
	ulgNewMsgCnt = status->recent;
	ulgUnseenMsgCnt = status->unseen;
	ulgMsgCnt = status->messages;
}
void mm_dlog (char *string) {}
void mm_login (NETMBX *mb,char *user,char *pwd,long trial)
{
	static char *myhost = NIL, *myuser = NIL;
	static char *mymailbox = NIL, *myservice = NIL;
	static char *mypassword = NIL;

	if (debug) {
		printf("mm_login called\n");
		printf("\tHOST       = %s\n", mb->host);
		printf("\tmyhost     = %s\n", myhost);
		printf("\tUSER       = %s\n", mb->user);
		printf("\tmyuser     = %s\n", myuser);
		printf("\tMBOX       = %s\n", mb->mailbox);
		printf("\tmymailbox  = %s\n", mymailbox);
		printf("\tSRVC       = %s\n", mb->service);
		printf("\tmyservice  = %s\n", myservice);
		printf("\tmypassword = %s\n", (mypassword)?mypassword:"\"\"");
		printf("\ttrial      = %d\n", trial);
	}
	if (!trial && myuser && mypassword && strcmp(myhost, mb->host) == 0) {
		strcpy(user, myuser);
		strcpy(pwd, mypassword);
	}
	else {
		pfmt(stdout, MM_NOSTD, ":681:User [%s] ",
			(myuser)?myuser:myname);
		fflush(stdout);

		fgets(user, MAILTMPLEN-1, stdin);
		if (user[0])
			user[strlen(user)-1] = 0;
		if (strlen(user) == 0)
			strcpy(user, myname);
		else {
			if (cascmp(user, myname)) {
				readonly = 1;
			}
		}
		if (myuser)
			free(myuser);
		myuser = (char *)strdup(user);
		pfmt(stdout, MM_NOSTD, ":682:Password: ");
		fflush(stdout);
		ioctlon();

		fgets(pwd, MAILTMPLEN-1, stdin);
		if (pwd[0])
			pwd[strlen(pwd)-1] = 0;
		printf("\n");
		if (mypassword)
			free(mypassword);
		mypassword = strdup(pwd);
		ioctloff();
	}
	if (myhost)
		free(myhost);
	myhost = (char *)strdup(mb->host);
	if (debug) {
		printf("\tuser  = \"%s\"\n", user);
		printf("\tpwd   = \"%s\"\n", pwd);
	}
	fflush(stdout);
	fflush(stdin);
}
void mm_critical (MAILSTREAM *stream) {}
void mm_nocritical (MAILSTREAM *stream) {}
