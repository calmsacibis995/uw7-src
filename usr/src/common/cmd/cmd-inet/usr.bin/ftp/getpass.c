/*
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

#ident	"@(#)getpass.c	1.2"
#ident	"$Header$"

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
#include <pfmt.h>
#include <sys/ttold.h>
#include <sys/types.h>
#include <sys/stropts.h>

static	struct termios ttyatt;
static	tcflag_t lflag;
static	FILE *fi;

extern int errno;

static void
intfix()
{
	ttyatt.c_lflag = lflag;
	if (fi != NULL) {
	        if (tcsetattr(fileno(fi), TCSANOW, &ttyatt) == -1)
                        pfmt(stderr, MM_ERROR, ":167:tcsetattr: %s\n",
				strerror(errno));
        }
	exit(SIGINT);
}

char *
mygetpass(prompt)
char *prompt;
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

	if (tcgetattr(fileno(fi), &ttyatt) == -1)
		pfmt(stderr, MM_ERROR, ":168:tcgetattr: %s\n",
			strerror(errno));	/* go ahead, anyway */
	lflag = ttyatt.c_lflag;
	ttyatt.c_lflag &= ~ECHO;
	if (tcsetattr(fileno(fi), TCSANOW, &ttyatt) == -1)
                pfmt(stderr, MM_ERROR, ":167:tcsetattr: %s\n",
			strerror(errno));
	fprintf(stderr, "%s", prompt); (void) fflush(stderr);
	for (p=pbuf; (c = getc(fi))!='\n' && c!=EOF;) {
		if (p < &pbuf[sizeof(pbuf)-1])
			*p++ = c;
	}
	*p = '\0';
	fprintf(stderr, "\n"); (void) fflush(stderr);
	ttyatt.c_lflag = lflag;
	if (tcsetattr(fileno(fi), TCSANOW, &ttyatt) == -1)
                pfmt(stderr, MM_ERROR, ":167:tcsetattr: %s\n",
			strerror(errno));
	(void) Signal(SIGINT, sig);

	if (fi != stdin)
		(void) fclose(fi);
	return(pbuf);
}
