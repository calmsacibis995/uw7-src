#ident	"@(#)translate.c	11.1"
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

#ident "@(#)translate.c	1.10 'attmail mail(1) command'"
#include "rcv.h"

static void
readvar(fp, var)
	FILE *fp;
	char *var;
{
	char	 line[LINESIZE];

	if (fgets(line, sizeof line, fp)) {
		line[strlen(line)-1] = 0;
		if (*line)
			assign(var, line);
	}
}

struct name *
translate(np)
	struct name *np;
{
	struct name	*n, *t, *x;
	void	(*sigint)(), (*sigquit)();
	char	*xl = value("translate");
	char	line[LINESIZE];
	char	*cmd;
	FILE	*pp;
	int	i;

	if (!xl)
		return np;
	i = strlen(xl) + 1;
	for (n = np; n; n = n->n_link)
		if (! (n->n_type & GDEL))
			i += strlen(n->n_name) + 3;
	cmd = salloc((unsigned)i);
	strcpy(cmd, xl);
	for (n = np; n; n = n->n_link)
		if (! (n->n_type & GDEL)) {
			strcat(cmd, " \"");
			strcat(cmd, n->n_name);
			strcat(cmd, "\"");
		}
	if ((pp = npopen(cmd, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, failed, "pipe", Strerror(errno));
		senderr++;
		return np;
	}
	sigint = sigset(SIGINT, SIG_IGN);
	sigquit = sigset(SIGQUIT, SIG_IGN);
	readvar(pp, "postmark");
	for (n = np; n; n = n->n_link) {
		if (n->n_type & GDEL)
			continue;
		if (fgets(line, sizeof line, pp) == NULL)
			break;
		line[strlen(line)-1] = 0;
		if (!strcmp(line, n->n_name))
			continue;
		x = extract(line, n->n_type);
		n->n_type |= GDEL;
		n->n_name = "";
		if (x && !x->n_link && strpbrk(n->n_full, "(<"))
			x->n_full = splice(x->n_name, n->n_full);
		if (x) {
			t = tailof(x);
			t->n_link = n->n_link;
			n->n_link = x;
			n = t;
		}
	}
	if (value("askme"))
		deassign("askme");
	readvar(pp, "askme");
	if (npclose(pp) != 0 || n)
		senderr++;
	sigset(SIGINT, sigint);
	sigset(SIGQUIT, sigquit);
	return np;
}
