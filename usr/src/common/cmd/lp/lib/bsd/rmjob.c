/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*		copyright	"%c%" 	*/

#ident	"@(#)rmjob.c	1.4"
#ident	"$Header$"

#include <string.h>
#include "lp.h"
#include "msgs.h"
#include "requests.h"
#include "lpd.h"
#include "oam_def.h"

#if defined (__STDC__)
static	char	* mkuser(char *, char *);
static	int	  cancel(char *, char *);
static	int	  isowner(char *, char *, char *);
#else
static	char	* mkuser();
static	int	  cancel();
static	int	  isowner();
#endif

/*
 * Remove local print job(s)
 */
void
#if defined (__STDC__)
rmjob(void)
#else
rmjob()
#endif
{
	int	  i;
	int	  all = 0;
	int	  user_all = 0;
	short	  status;
	char	 *owner;
	char	 *reqid;
	char	 *host;
	char	**rmjobs = NULL;

	if (!isprinter(Printer)) {
		fatal("unknown printer");
		/*NOTREACHED*/
	}

	/* - option was used if Nusers < 0 */
	if (Nusers < 0)
		all = 1;
	else
		if (Nusers > 0) /* user id specified, remove all for that user*/
			user_all = 1;

	if (STREQU(Person, ALL)) {	/* "-all" same as "root" */
		if (!Rhost) {		/* only allowed from remote */
			fatal("The login name \"%s\" is reserved", ALL);
			/*NOTREACHED*/
		}
		all = 1;
	}
	if (all) {
		if (getuid() == 0)
		cancel(Rhost ? mkuser(Rhost, NAME_ALL) : mkuser(NULL, NAME_ALL),"");
		else
		cancel(Rhost ? mkuser(Rhost, Person) : mkuser(NULL, Person),"");
		return;
	}
	if (user_all) {
		if (getuid() == 0) {
			for (i=0; i<Nusers; i++) 
				cancel(mkuser(NULL, User[i]),"");
			return;
		}
	}

	if (!Nusers && !Nrequests) {	/* cancel current job */
		if (STREQU(Person, "root"))
			cancel(Rhost ? mkuser(Rhost, "") : "!", CURRENT_REQ);
		else
			cancel(mkuser(Rhost, Person), CURRENT_REQ);
		return;
	}
	snd_msg(S_INQUIRE_REQUEST, "", Printer, "", "", "");
	do {
		size_t	 size;
		time_t	 date;
		level_t	 lid;
		short	 outcome;
		char	*dest, *form, *pwheel;
		int	 match = 0;

		rcv_msg(R_INQUIRE_REQUEST, &status, 
					   &reqid, 
					   &owner, 
					   &size, 
					   &date, 
					   &outcome, 
					   &dest, 
					   &form, 
					   &pwheel,
					   &lid);
		switch(status) {
		case MOK:
		case MOKMORE:
			logit(LOG_DEBUG, 
			"R_INQUIRE_REQUEST(\"%s\", \"%s\", \"%s\", 0x%x)",
						reqid, owner, dest, outcome);
			if (outcome & RS_DONE)
				break;
			parseUser(owner, &host, &owner);

			for (i=0; !match && i<Nrequests; i++) {
				if (STREQU(reqid, Request[i]) &&
				    isowner(reqid, host, owner))
					match++;
			}
			for (i=0; !match && i<Nusers; i++)
				if (STREQU(owner, User[i]) &&
				    isowner(reqid, host, owner))
					match++;
			if (match) {
				char	buf[100];
				
				if (!strcmp(Lhost,host))
				sprintf(buf, "%s %s", mkuser(NULL, owner), 
						      reqid);
				else
				sprintf(buf, "%s %s", mkuser(host, owner), 
						      reqid);

				appendlist(&rmjobs, buf);
			}
			break;

		default:
			break;
		}
	} while (status == MOKMORE);
	if (rmjobs) {
		char	**job;

		for (job = rmjobs; *job; job++) {
			reqid = strchr(*job, ' ');
			*reqid++ = NULL;
			cancel(*job, reqid);		/* *job == owner */
		}
		freelist(rmjobs);
	}
	fflush(stdout);
}

static
#if defined (__STDC__)
cancel(char *user, char *reqid)
#else
cancel(user, reqid)
char	*user;
char	*reqid;
#endif
{
	char	*host;
	short	 status1; 
	long	 status2;

	logit(LOG_DEBUG, "S_CANCEL(\"%s\", \"%s\", \"%s\")", 
					Printer, user, reqid);
	snd_msg(S_CANCEL, Printer, user, reqid);
	do {
		rcv_msg(R_CANCEL, &status1, &status2, &reqid);
		logit(LOG_DEBUG, "R_CANCEL(%d, %d, \"%s\")", status1, status2, reqid);
		if (status1 == MNOINFO)		/* quiet if job doesn't exist */
			break;
		if (Rhost)
			printf("%s: ", Lhost);
		switch(status2) {

		case MOK:
			/*
		 	* It is not possible to know if remote really responded
		 	* to request for job removal; however, the job should
			* eventually be removed.
		 	*/
			printf("%s dequeued\n", reqid);
			break;

		case MNOPERM:		/* This should be caught earlier */
					/* (except for CURRENT_REQ)	 */
			printf("%s: Permission denied\n", reqid);
			break;

		case M2LATE:
			printf("cannot dequeue %s\n", reqid);
			break;

		default:
			/* What error message to put here?
			 * At least finish the current line (wasn't
			 * before)
			 */
			printf("\n");
			break;

		}
	} while (status1 == MOKMORE);
	fflush(stdout);
}

static
#if defined (__STDC__)
isowner(char *reqid, char *host, char *user)
#else
isowner(reqid, host, user)
char	*reqid;
char	*host;
char	*user;
#endif
{
	if (STREQU(Person, "root")) {	/* trapped later if impostor */
		if (!Rhost || STREQU(Rhost, host)) {
			return(1);
		}
		/* only 'root' on a trusted, remote, system can cancel
		 * requests */
		if (Rhost_trusted)
			return (1);
	} else if (STREQU(Person, user))
		if (Rhost ? STREQU(Rhost, host) : STREQU(Lhost, host))
			return(1);
	if (Rhost)
		printf("%s: ", Lhost);
	printf("%s %s: Permission denied\n", reqid, host);
	return(0);
}

static char *
#if defined (__STDC__)
mkuser(char *host, char *user)
#else
mkuser(host, user)
char	*host;
char	*user;
#endif
{
	static char	buf[50];

	sprintf(buf, "%s%s%s", host ? host : "",
			       host ? "!"  : "",
			       user ? user : "");
	return(buf);
}
