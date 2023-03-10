/* @(#)getpass.c	1.3
 *
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

#include <stdio.h>
#include <signal.h>
#include <termio.h>
#include <sys/ttold.h>
#include <sys/types.h>
#include <sys/stropts.h>
#include <signal.h>
#include "ldaplog.h"

static	struct termios ttyatt;
static	tcflag_t lflag;
static	FILE *fi;

extern int errno;

/*
 * This version of Signal() checks if we are a background process
 * or not.
 */
typedef void (*sig_t)();

static sig_t
Signal(sig, hand)
	int sig;
	sig_t hand;
{
	sig_t old;
	static struct {
		sig_t	orig;
		int	isset;
	}	first_val[MAXSIG];

	/*
	 * If this is not a signal that can be generated by tty,
	 * then set it unconditionally.
	 */
	if (sig != SIGINT && sig != SIGQUIT && sig != SIGHUP)
		return sigset(sig, hand);

	/*
	 * Assume for now that we are a background process, and so don't
	 * want to get zapped by keyboard.
	 */
	old = sigset(sig, SIG_IGN);
	/*
	 * If this is the first time we have set this signal
	 * save its old handler.
	 */
	if (!first_val[sig].isset) {
		first_val[sig].orig = old;
		first_val[sig].isset = 1;
	}
	/*
	 * If the original handler was not SIG_IGN, then set the new handler
	 * as requested; we are (or started out as) a foreground process.
	 */
	if (first_val[sig].orig != SIG_IGN)
		sigset(sig, hand);
	return old;
}


static void
intfix()
{
	ttyatt.c_lflag = lflag;
	if (fi != NULL) {
	        if (tcsetattr(fileno(fi), TCSANOW, &ttyatt) == -1) {

			logDebug(LDAP_LOG_CLNT,
			    "(intfix) Call to tcsetattr failed: %s\n",
			    strerror(errno),0,0);

		}
        }
	exit(SIGINT);
}

char *
ldap_getpass(char *prompt)
{
	register char *p;
	register int c;
	static char pbuf[50+1];
	void (*sig)();
	void (*Signal())();

	if ((fi = fopen("/dev/tty", "r")) == NULL)
		fi = stdin;
	else
		setbuf(fi, (char *)NULL);

	sig = Signal(SIGINT, (void (*)())intfix);

	if (tcgetattr(fileno(fi), &ttyatt) == -1) {
		fprintf(stderr,"tcgetattr: %s\n",
			strerror(errno));	/* go ahead, anyway */
	}

	lflag = ttyatt.c_lflag;
	ttyatt.c_lflag &= ~ECHO;
	if (tcsetattr(fileno(fi), TCSANOW, &ttyatt) == -1) {
                fprintf(stderr, "tcsetattr: %s\n", strerror(errno));
	}
	fprintf(stderr, "%s", prompt); (void) fflush(stderr);
	for (p=pbuf; (c = getc(fi))!='\n' && c!=EOF;) {
		if (p < &pbuf[sizeof(pbuf)-1])
			*p++ = c;
	}
	*p = '\0';
	fprintf(stderr, "\n"); (void) fflush(stderr);
	ttyatt.c_lflag = lflag;
	if (tcsetattr(fileno(fi), TCSANOW, &ttyatt) == -1) {
                fprintf(stderr, "tcsetattr: %s\n", strerror(errno));
	}
	(void) Signal(SIGINT, sig);

	if (fi != stdin)
		(void) fclose(fi);
	return(pbuf);
}
