/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)update.c	1.2"
#ident	"$Header$"


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice
*
* Notice of copyright on this source code product does not indicate
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991,1992  UNIX System Laboratories, Inc.
*          All rights reserved.
*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

/*
 * Administrative tool to add a new user to the publickey database
 */
#include <stdio.h>
#include <stdlib.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#ifdef YP
#include <rpcsvc/ypclnt.h>
#include <sys/wait.h>
#include <netdb.h>
#endif	/* YP */
#include <pwd.h>
#include <string.h>
#include <sys/resource.h>

#ifdef YP
#define	MAXMAPNAMELEN 256
#else
#define	YPOP_CHANGE 1			/* change, do not add */
#define	YPOP_INSERT 2			/* add, do not change */
#define	YPOP_DELETE 3			/* delete this entry */
#define	YPOP_STORE  4			/* add, or change */
#endif

#ifdef YP
static char *basename();
static char SHELL[] = "/sbin/sh";
static char YPDBPATH[]="/var/yp";
static char PKMAP[] = "publickey.byname";
static char UPDATEFILE[] = "updaters";
#else
static char PKFILE[] = "/etc/publickey";
#endif	/* YP */

#ifdef YP
/*
 * Determine if requester is allowed to update the given map,
 * and update it if so. Returns the yp status, which is zero
 * if there is no access violation.
 */
mapupdate(requester, mapname, op, keylen, key, datalen, data)
	char *requester;
	char *mapname;
	u_int op;
	u_int keylen;
	char *key;
	u_int datalen;
	char *data;
{
	char updater[MAXMAPNAMELEN + 40];
	FILE *childargs;
	FILE *childrslt;
#ifdef WEXITSTATUS
	int status;
#else
	union wait status;
#endif
	pid_t pid;
	u_int yperrno;


#ifdef DEBUG
	printf("%s %s\n", key, data);
#endif
        (void)sprintf(updater, "/var/yp/ypbuild SHELL=/sbin/sh -s -f %s %s",
			UPDATEFILE, mapname);
	pid = _openchild(updater, &childargs, &childrslt);
	if (pid < 0) {
		return (YPERR_YPERR);
	}

	/*
	 * Write to child
	 */
	(void)fprintf(childargs, "%s\n", requester);
	(void)fprintf(childargs, "%u\n", op);
	(void)fprintf(childargs, "%u\n", keylen);
	(void)fwrite(key, (int)keylen, 1, childargs);
	(void)fprintf(childargs, "\n");
	(void)fprintf(childargs, "%u\n", datalen);
	(void)fwrite(data, (int)datalen, 1, childargs);
	(void)fprintf(childargs, "\n");
	(void)fclose(childargs);

	/*
	 * Read from child
	 */
	(void)fscanf(childrslt, "%d", &yperrno);
	(void)fclose(childrslt);

	(void)wait(&status);
#ifdef WEXITSTATUS
	if (WEXITSTATUS(status) != 0) {
#else
	if (status.w_retcode != 0) {
#endif
		return (YPERR_YPERR);
	}
	return (yperrno);
}

/*
 * returns pid, or -1 for failure
 */
static
_openchild(command, fto, ffrom)
	char *command;
	FILE **fto;
	FILE **ffrom;
{
	int i;
	pid_t pid;
	int pdto[2];
	int pdfrom[2];
	char *com;
	struct rlimit rl;

	if (pipe(pdto) < 0) {
		goto error1;
	}
	if (pipe(pdfrom) < 0) {
		goto error2;
	}
#ifdef VFORK
	switch (pid = vfork()) {
#else
	switch (pid = fork()) {
#endif
	case -1:
		goto error3;

	case 0:
		/*
		 * child: read from pdto[0], write into pdfrom[1]
		 */
		(void)close(0);
		(void)dup(pdto[0]);
		(void)close(1);
		(void)dup(pdfrom[1]);
		getrlimit(RLIMIT_NOFILE, &rl);
		for (i = rl.rlim_cur - 1; i >= 3; i--) {
			(void) close(i);
		}
		com = malloc((unsigned) strlen(command) + 6);
		if (com == NULL) {
			_exit(~0);
		}
		(void)sprintf(com, "exec %s", command);
		execl(SHELL, basename(SHELL), "-c", com, NULL);
		_exit(~0);

	default:
		/*
		 * parent: write into pdto[1], read from pdfrom[0]
		 */
		*fto = fdopen(pdto[1], "w");
		(void)close(pdto[0]);
		*ffrom = fdopen(pdfrom[0], "r");
		(void)close(pdfrom[1]);
		break;
	}
	return (pid);

	/*
	 * error cleanup and return
	 */
error3:
	(void)close(pdfrom[0]);
	(void)close(pdfrom[1]);
error2:
	(void)close(pdto[0]);
	(void)close(pdto[1]);
error1:
	return (-1);
}

static char *
basename(path)
	char *path;
{
	char *p;

	p = strrchr(path, '/');
	if (p == NULL) {
		return (path);
	} else {
		return (p + 1);
	}
}

#else /* YP */

#define	ERR_ACCESS	1
#define	ERR_MALLOC	2
#define	ERR_READ	3
#define	ERR_WRITE	4
#define	ERR_DBASE	5
#define	ERR_KEY		6
extern char *malloc();

/*
 * Determine if requester is allowed to update the given map,
 * and update it if so. Returns the status, which is zero
 * if there is no access violation. This function updates
 * the local file and then shuts up.
 */
localupdate(name, filename, op, keylen, key, datalen, data)
	char *name;	/* Name of the requestor */
	char *filename;
	u_int op;
	u_int keylen;	/* Not used */
	char *key;
	u_int datalen;	/* Not used */
	char *data;
{
	char line[256];
	FILE *rf;
	FILE *wf;
	char *tmpname;
	int err;

	/*
	 * Check permission
	 */
	if (strcmp(name, key) != 0) {
		return (ERR_ACCESS);
	}
	if (strcmp(name, "nobody") == 0) {
		/*
		 * Can't change "nobody"s key.
		 */
		return (ERR_ACCESS);
	}

	/*
	 * Open files
	 */
	tmpname = malloc(strlen(filename) + 4);
	if (tmpname == NULL) {
		return (ERR_MALLOC);
	}
	sprintf(tmpname, "%s.tmp", filename);
	rf = fopen(filename, "r");
	if (rf == NULL) {
		return (ERR_READ);
	}
	wf = fopen(tmpname, "w");
	if (wf == NULL) {
		return (ERR_WRITE);
	}
	err = -1;
	while (fgets(line, sizeof (line), rf)) {
		if (err < 0 && match(line, name)) {
			switch (op) {
			case YPOP_INSERT:
				err = ERR_KEY;
				break;
			case YPOP_STORE:
			case YPOP_CHANGE:
				fprintf(wf, "%s %s\n", key, data);
				err = 0;
				break;
			case YPOP_DELETE:
				/* do nothing */
				err = 0;
				break;
			}
		} else {
			fputs(line, wf);
		}
	}
	if (err < 0) {
		switch (op) {
		case YPOP_CHANGE:
		case YPOP_DELETE:
			err = ERR_KEY;
			break;
		case YPOP_INSERT:
		case YPOP_STORE:
			err = 0;
			fprintf(wf, "%s %s\n", key, data);
			break;
		}
	}
	fclose(wf);
	fclose(rf);
	if (err == 0) {
		if (rename(tmpname, filename) < 0) {
			return (ERR_DBASE);
		}
	} else {
		if (unlink(tmpname) < 0) {
			return (ERR_DBASE);
		}
	}
	return (err);
}

static
match(line, name)
	char *line;
	char *name;
{
	int len;

	len = strlen(name);
	return (strncmp(line, name, len) == 0 &&
		(line[len] == ' ' || line[len] == '\t'));
}
#endif /* !YP */
