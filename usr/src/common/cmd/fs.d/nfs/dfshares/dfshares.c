#ident	"@(#)dfshares.c	1.3"
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
 * exit values
 */
#define RET_OK		0
#define RET_ERR		33	/* usage error */
#define RET_RPCERR	34	/* rpc error */
#define RET_NOEXP	35	/* no exports list */
#define RET_HNAME	36	/* gethostname (uname) failed */

/*
 * nfs dfshares
 */
#include <stdio.h>
#include <varargs.h>
#include <string.h>
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

int hflg, oflg;
int pr_exports();
void free_ex();
void usage();

main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	extern char *optarg;
 	char hostbuf[256];
	int i, c, retval = 0, rpcerr_remember = 0;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:nfs dfshares");

	while ((c = getopt(argc, argv, "ho")) != EOF) {
		switch (c) {
		case 'h':
			hflg++;
			break;
		case 'o':
			oflg++;
			break;
		default:
			usage();
			exit(RET_ERR);
		}
	}

	if (optind < argc) {
		for (i = optind ; i < argc ; i++) {
			retval = pr_exports(argv[i]);
			if (retval == RET_RPCERR)
				rpcerr_remember++;
		}
	} else {
		if (gethostname(hostbuf, sizeof(hostbuf)) < 0) {
			pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n",
			     "gethostname", strerror(errno));
			exit(RET_HNAME);
		}
		pr_exports(hostbuf);
	}

	if (rpcerr_remember)
		exit(RET_RPCERR);
	else
		exit(RET_OK);
}

int
pr_exports(host)
	char *host;
{
	CLIENT *cl;
	struct exports *ex = NULL;
	enum clnt_stat err;

	/*
	 * First try circuit, then drop back to datagram if
	 * circuit is unavailable (an old version of mountd perhaps)
	 * Using circuit is preferred because it can handle
	 * arbitrarily long export lists.
	 *
	 * We let the -o option override the above
	 */
	if (!oflg) {
		cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "circuit_n");
		if (cl == NULL) {
			cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "datagram_n");
			if (cl == NULL) {
				pfmt(stderr, MM_ERROR, 
				     ":47:%s: server not responding for %s\n",
				     host, clnt_spcreateerror("clnt_create"));
				return(RET_RPCERR);
			}
		}
	} else {
		cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "datagram_n");
		if (cl == NULL) {
			pfmt(stderr, MM_ERROR, 
			     ":47:%s: server not responding for %s\n",
			     host, clnt_spcreateerror("clnt_create"));
			return(RET_RPCERR);
		}
	}

	if (err = clnt_call(cl, MOUNTPROC_EXPORT,
			    xdr_void, 0, xdr_exports, (caddr_t) &ex, timeout)) {
		pfmt(stderr, MM_ERROR, 
		     ":47:%s: server not responding for %s\n",
		     host, clnt_sperror(cl, "clnt_call"));
		clnt_destroy(cl);
		return(RET_RPCERR);
	}

	if (ex == NULL) {
		clnt_destroy(cl);
		exit(RET_NOEXP);
	}

	if (!hflg) {
	     /*	printf("%-35s %12s %-8s  %s\n",
			"RESOURCE", "SERVER", "ACCESS", "TRANSPORT"); */
		pfmt(stdout, MM_NOSTD, 
		     ":51:RESOURCE                                  SERVER ACCESS    TRANSPORT\n");
		hflg++;
	}

	while (ex) {
		printf("%10s:%-24s %12s %-8s  %s\n",
			host, ex->ex_name, host, " -", " -");
		ex = ex->ex_next;
	}
	free_ex(ex);
	clnt_destroy(cl);

	return(0);
}

void
free_ex(ex)
	struct exports *ex;
{
	struct groups *gr, *tmpgr;
	struct exports *tmpex;

	while (ex) {
		free(ex->ex_name);
		gr = ex->ex_groups;
		while (gr) {
			tmpgr = gr->g_next;
			free((char *)gr);
			gr = tmpgr;
		}
		tmpex = ex;
		ex = ex->ex_next;
		free((char *)tmpex);
	}
}

void
usage()
{
	pfmt(stderr, MM_ACTION, ":50:Usage: dfshares [-h] [host ...]\n");
}
