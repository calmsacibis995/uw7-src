/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)chkey.c	1.3"
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
 * Command to change one's public key in the public key database
 */
#include <stdio.h>
#include <stdlib.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#ifdef YP
#include <rpcsvc/ypclnt.h>
#else
#define	YPOP_STORE	4
#endif
#include <pwd.h>
#include <string.h>
#include <locale.h>
#include <pfmt.h>
#include "msg.h"

#define	index strchr
extern char *crypt();
#ifdef YPPASSWD
struct passwd *ypgetpwuid();
#endif

#ifdef YP
static char *domain;
static char PKMAP[] = "publickey.byname";
#else
static char PKFILE[] = "/etc/publickey";
#endif	/* YP */
static char ROOTKEY[] = "/etc/.rootkey";

char    *msg_label = "UX:chkey";

main(argc, argv)
	int argc;
	char **argv;
{
	char name[MAXNETNAMELEN+1];
	char public[HEXKEYBYTES + 1];
	char secret[HEXKEYBYTES + 1];
	char crypt1[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	char crypt2[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	int status;
	char *pass;
	struct passwd *pw;
	uid_t uid;
	int force = 0;
	char *self;
#ifdef YP
	char *master;
#endif

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxchkey");
	(void)setlabel(msg_label);

	self = argv[0];
	for (argc--, argv++; argc > 0 && **argv == '-'; argc--, argv++) {
		if (argv[0][2] != 0) {
			usage(self);
		}
		switch (argv[0][1]) {
		case 'f':
			force = 1;
			break;
		default:
			usage(self);
		}
	}
	if (argc != 0) {
		usage(self);
	}

#ifdef YP
	(void)yp_get_default_domain(&domain);
	if (yp_master(domain, PKMAP, &master) != 0) {
		pfmt (stderr, MM_ERROR, MSG1);
		exit(1);
	}
#endif
	uid = geteuid();
	if (uid == 0) {
		if (host2netname(name, NULL, NULL) == 0) {
			pfmt (stderr, MM_ERROR, MSG2);
			exit(1);
		}
	} else {
		if (user2netname(name, uid, NULL) == 0) {
			pfmt (stderr, MM_ERROR, MSG3);
			exit(1);
		}
	}
	pfmt (stdout, MM_INFO, MSG4, name);

	if (!force) {
		if (uid != 0) {
#ifdef YPPASSWD
			pw = ypgetpwuid(uid);
#else
			pw = getpwuid(uid);
#endif
			if (pw == NULL) {
#ifdef YPPASSWD
			pfmt (stderr, MM_ERROR, MSG5);
#else
			pfmt (stderr, MM_ERROR, MSG6);
#endif
				exit(1);
			}
		} else {
			pw = getpwuid(0);
			if (pw == NULL) {
				pfmt (stderr, MM_ERROR, MSG6);
				exit(1);
			}
		}
	}
	pass = getpass("Password:");
#ifdef YPPASSWD
	if (!force) {
		if (strcmp(crypt(pass, pw->pw_passwd), pw->pw_passwd) != 0) {
			pfmt (stderr, MM_ERROR, MSG7);
			exit(1);
		}
	}
#else
	force = 1;	/* Make this mandatory */
#endif
	genkeys(public, secret, pass);

	memcpy(crypt1, secret, HEXKEYBYTES);
	memcpy(crypt1 + HEXKEYBYTES, secret, KEYCHECKSUMSIZE);
	crypt1[HEXKEYBYTES + KEYCHECKSUMSIZE] = 0;
	xencrypt(crypt1, pass);

	if (force) {
		memcpy(crypt2, crypt1, HEXKEYBYTES + KEYCHECKSUMSIZE + 1);
		xdecrypt(crypt2, getpass("Retype password:"));
		if (memcmp(crypt2, crypt2 + HEXKEYBYTES, KEYCHECKSUMSIZE) != 0 ||
		    memcmp(crypt2, secret, HEXKEYBYTES) != 0) {
			pfmt (stderr, MM_ERROR, MSG8);
			exit(1);
		}
	}

#ifdef YP
	pfmt (stdout, MM_INFO, MSG9, master);
#endif
	status = setpublicmap(name, public, crypt1);
	if (status != 0) {
#ifdef YP
		pfmt (stderr, MM_ERROR, MSG10, self, status, yperr_string(status));
#else
		pfmt (stderr, MM_ERROR, MSG11, self);
#endif
		exit(1);
	}

	if (uid == 0) {
		/*
		 * Root users store their key in /etc/$ROOTKEY so
		 * that they can auto reboot without having to be
		 * around to type a password. Storing this in a file
		 * is rather dubious: it should really be in the EEPROM
		 * so it does not go over the net.
		 */
		int fd;

		fd = open(ROOTKEY, O_WRONLY|O_TRUNC|O_CREAT, 0);
		if (fd < 0) {
			perror(ROOTKEY);
		} else {
			char newline = '\n';

			if (write(fd, secret, strlen(secret)) < 0 ||
			    write(fd, &newline, sizeof (newline)) < 0) {
				(void)fprintf(stderr, "%s: ", ROOTKEY);
				perror("write");
			}
		}
	}

	if (key_setsecret(secret) < 0) {
		pfmt (stderr, MM_ERROR, MSG12);
		exit(1);
	}
	pfmt (stdout, MM_INFO, MSG13);
	exit(0);
	/* NOTREACHED */
}

usage(name)
	char *name;
{
	pfmt (stderr, MM_ERROR, MSG14, name);
	exit(1);
	/* NOTREACHED */
}


/*
 * Set the entry in the public key file
 */
setpublicmap(name, public, secret)
	char *name;
	char *public;
	char *secret;
{
	char pkent[1024];

	(void)sprintf(pkent, "%s:%s", public, secret);
#ifdef YP
	return (yp_update(domain, PKMAP, YPOP_STORE,
		name, strlen(name), pkent, strlen(pkent)));
#else
	return (localupdate(name, PKFILE, YPOP_STORE,
		strlen(name), name, strlen(pkent), pkent));
#endif
}

#ifdef YPPASSWD
struct passwd *
ypgetpwuid(uid)
	uid_t uid;
{
	char uidstr[10];
	char *val;
	int vallen;
	static struct passwd pw;
	char *p;

	(void)sprintf(uidstr, "%d", uid);
	if (yp_match(domain, "passwd.byuid", uidstr, strlen(uidstr),
			&val, &vallen) != 0) {
		return (NULL);
	}
	p = index(val, ':');
	if (p == NULL) {
		return (NULL);
	}
	pw.pw_passwd = p + 1;
	p = index(pw.pw_passwd, ':');
	if (p == NULL) {
		return (NULL);
	}
	*p = 0;
	return (&pw);
}
#endif	/* YPPASSWD */
