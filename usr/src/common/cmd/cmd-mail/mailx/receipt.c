#ident	"@(#)receipt.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident "@(#)receipt.c	1.6 'attmail mail(1) command'"
#include "rcv.h"

static int		icsubstr ARGS((char *s1, char *s2));

void
receipt(mp)
struct message *mp;
{
	char	head[LINESIZE];
	char	buf[BUFSIZ];
	FILE	*pp;
	char	*mailprog, *s;
	MAILSTREAM *fp;
	MESSAGECACHE *elt = NIL;

	/*
	 * this code is generating return receipt messages when a message
	 * containing the special strings is read. This isn't conformant 
	 * to the return receipt protocols I'm aware of, so we'll leave
	 * it out for now. This will have to be discussed in more detail
	 * later.
	 */
	if ((mailprog = value("sendmail")) == 0)
		mailprog = MAIL;
	if (icsubstr(hfield("default-options", mp, addone), "/receipt") ||
	    icsubstr(hfield(">to", mp, addto), "/receipt")) {
		sprintf(buf, "%s %s", mailprog, skin(nameof(mp)));
		if ((pp = npopen(buf, "w")) != 0) {
			fp = setinput(mp);
			elt = mail_elt(fp, mp - message + 1);
			fprintf(pp, "Original-Date: %s\n",
				mail_cdate(head, elt));
			if ((s = hfield("message-id", mp, addone)) != 0)
				fprintf(pp, "Original-Message-ID: %s\n", s);
			s = hfield("subject", mp, addone);
			fprintf(pp, "Subject: RR: %s\n", s ? s : "(none)");
			npclose(pp);
		}
	}
}

static int
icsubstr(s1, s2)
char	*s1, *s2;
{
	char	buf[LINESIZE];

	if (s1 && s2) {
		istrcpy(buf, s1);
		return substr(buf, s2) != -1;
	} else
		return 0;
}
