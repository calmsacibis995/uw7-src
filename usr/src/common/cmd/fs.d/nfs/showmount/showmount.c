#ident	"@(#)showmount.c	1.2"
#ident	"$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
 
*/
/*
 * showmount
 */
#include <stdio.h>
#include <varargs.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <locale.h>
#include <pfmt.h>

struct timeval timeout = { 25, 0 };

int sorthost();
int sortpath();
void printex();

#define	NTABLEENTRIES	2048
struct mountlist *table[NTABLEENTRIES];

main(argc, argv)
	int argc;
	char **argv;
{
	
	int aflg = 0, dflg = 0, eflg = 0;
	int err;
	struct mountlist *ml = NULL;
	struct mountlist **tb, **endtb;
	char *host, hostbuf[256];
	char *last;
	CLIENT *cl;
	extern int optind;
	extern char *optarg;
	int c;

 	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:showmount");

	while ((c = getopt(argc, argv, "ade")) != EOF) {
		switch (c) {
		case 'a':
			aflg++;
			break;
		case 'd':
			dflg++;
			break;
		case 'e':
			eflg++;
			break;
		default:
			usage();
			exit(1);
		}
	}

	switch (argc - optind) {
	case 0:		/* no args */
		if (gethostname(hostbuf, sizeof(hostbuf)) < 0) {
			pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n", 
			     "gethostname", strerror(errno));
			exit(1);
		}
		host = hostbuf;
		break;
	case 1:		/* one arg */
		host = argv[optind];
		break;
	default:	/* too many args */
		usage();
		exit(1);
	}

	/*
	 * First try circuit, then drop back to datagram if
	 * circuit is unavailable (an old version of mountd perhaps)
	 * Using circuit is preferred because it can handle
	 * arbitrarily long export lists.
	 */
	cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "circuit_n");
	if (cl == NULL) {
		cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "datagram_n");
		if (cl == NULL) {
			pfmt(stderr, MM_ERROR, 
			     ":47:%s server not responding for %s\n", 
			     host, clnt_spcreateerror("clnt_create"));
			exit(1);
		}
	}

	if (eflg) {
		printex(cl, host);
		if (aflg + dflg == 0) {
			exit(0);
		}
	}

	if (err = clnt_call(cl, MOUNTPROC_DUMP, xdr_void, 0, 
			    xdr_mountlist, (caddr_t) &ml, timeout)) {
		pfmt(stderr, MM_ERROR, ":47:%s server not responding for %s\n", 
		     host, clnt_sperror(cl, "clnt_call"));
		exit(1);
	}
	tb = table;
	for (; ml != NULL && tb < &table[NTABLEENTRIES]; ml = ml->ml_nxt)
		*tb++ = ml;
	if (ml != NULL && tb == &table[NTABLEENTRIES])
		pfmt(stderr, MM_WARNING, 
		     ":49:table overflow: only %d entries shown\n", 
		     NTABLEENTRIES);
	endtb = tb;
	if (dflg)
	    qsort(table, endtb - table, sizeof(struct mountlist *), sortpath);
	else
	    qsort(table, endtb - table, sizeof(struct mountlist *), sorthost);
	if (aflg) {
		for (tb = table; tb < endtb; tb++)
			printf("%s:%s\n", (*tb)->ml_name, (*tb)->ml_path);
	}
	else if (dflg) {
		last = "";
		for (tb = table; tb < endtb; tb++) {
			if (strcmp(last, (*tb)->ml_path))
				printf("%s\n", (*tb)->ml_path);
			last = (*tb)->ml_path;
		}
	}
	else {
		last = "";
		for (tb = table; tb < endtb; tb++) {
			if (strcmp(last, (*tb)->ml_name))
				printf("%s\n", (*tb)->ml_name);
			last = (*tb)->ml_name;
		}
	}
	exit(0);
	/* NOTREACHED */
}

sorthost(a, b)
	struct mountlist **a,**b;
{
	return strcmp((*a)->ml_name, (*b)->ml_name);
}

sortpath(a, b)
	struct mountlist **a,**b;
{
	return strcmp((*a)->ml_path, (*b)->ml_path);
}

usage()
{
	pfmt(stderr, MM_ACTION, ":76:Usage: showmount [-a] [-d] [-e] [host]\n");
}

void
printex(cl, host)
	CLIENT *cl;
	char *host;
{
	struct exports *ex = NULL;
	struct exports *e;
	struct groups *gr;
	enum clnt_stat err;
	int max;

	if (err = clnt_call(cl, MOUNTPROC_EXPORT,
			    xdr_void, 0, xdr_exports, (caddr_t) &ex, timeout)) {
		pfmt(stderr, MM_ERROR, ":47:%s server not responding for %s\n",
		     host, clnt_sperror(cl, "clnt_call"));
		exit(1);
	}

	if (ex == NULL) {
		pfmt(stdout, MM_NOSTD, ":77:no exported file systems for %s\n", 
		     host);
	} else {
		pfmt(stdout, MM_NOSTD, ":78:export list for %s:\n", host);
	}
	max = 0;
	for (e = ex; e != NULL; e = e->ex_next) {
		if (strlen(e->ex_name) > max) {
			max = strlen(e->ex_name);
		}
	}
	while (ex) {
		printf("%-*s ", max, ex->ex_name);
		gr = ex->ex_groups;
		if (gr == NULL) {
			pfmt(stdout, MM_NOSTD, ":79:(everyone)");
		}
		while (gr) {
			printf("%s", gr->g_name);
			gr = gr->g_next;
			if (gr) {
				printf(",");
			}
		}
		printf("\n");
		ex = ex->ex_next;
	}
}
