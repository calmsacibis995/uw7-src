/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)keyserv.c	1.3"
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
 * Copyright (c) 1986 - 1991 by Sun Microsystems, Inc.
 */

/*
 * Keyserver
 * Store secret keys per uid. Do public key encryption and decryption
 * operations. Generate "random" keys.
 * Do not talk to anything but a local root
 * process on the local transport only
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/param.h>
#include <sys/file.h>
#include <pwd.h>
#include <rpc/des_crypt.h>
#include <rpc/key_prot.h>

#include <locale.h>
#include <pfmt.h>
#include "msg.h"

char    *msg_label = "UX:keyserv";

#ifdef KEYSERV_RANDOM
extern long random();
#endif
#ifndef NGROUPS
/*
 * This value should be increased to 32.
 */
#define	NGROUPS 16
#endif

extern keystatus pk_setkey();
extern keystatus pk_encrypt();
extern keystatus pk_decrypt();
static void randomize();
static void usage();
static int getrootkey();
static int root_auth();

#ifdef DEBUG
static int debugging = 1;
#else
static int debugging = 0;
#endif

static void keyprogram();
static des_block masterkey;
char *getenv();
static char ROOTKEY[] = "/etc/.rootkey";

main(argc, argv)
	int argc;
	char *argv[];
{
	int nflag = 0;
	extern char *optarg;
	extern int optind;
	int c;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxkeyserv");
	(void)setlabel(msg_label);


	while ((c = getopt(argc, argv, "ndD")) != -1)
		switch (c) {
		case 'n':
			nflag++;
			break;
		case 'd':
			pk_nodefaultkeys();
			break;
		case 'D':
			debugging = 1;
			break;
		default:
			usage();
		}

	if (optind != argc) {
		usage();
	}

	/*
	 * Initialize
	 */
	(void) umask(066);	/* paranoia */
	if (geteuid() != 0) {
		pfmt (stderr, MM_ERROR, MSG32, argv[0]);
		exit(1);
	}
	setmodulus(HEXMODULUS);
	getrootkey(&masterkey, nflag);

	if (svc_create_local_service(keyprogram, KEY_PROG, KEY_VERS,
		"netpath", "keyserv") == 0) {
		pfmt (stderr, MM_ERROR, MSG33, argv[0]);
		exit(1);
	}

	if (svc_create_local_service(keyprogram, KEY_PROG, KEY_VERS2,
		"netpath", "keyserv") == 0) {
		pfmt (stderr, MM_ERROR, MSG34, argv[0]);
		exit(1);
	}
	if (!debugging) {
		detachfromtty();
	}
	svc_run();
	abort();
	/* NOTREACHED */
}

/*
 * In the event that we don't get a root password, we try to
 * randomize the master key the best we can
 */
static void
randomize(master)
	des_block *master;
{
	int i;
	int seed;
	struct timeval tv;
	int shift;

	seed = 0;
	for (i = 0; i < 1024; i++) {
		(void) gettimeofday(&tv, (struct timezone *) NULL);
		shift = i % 8 * sizeof (int);
		seed ^= (tv.tv_usec << shift) | (tv.tv_usec >> (32 - shift));
	}
#ifdef KEYSERV_RANDOM
	srandom(seed);
	master->key.low = random();
	master->key.high = random();
	srandom(seed);
#else
	/* use stupid dangerous bad rand() */
	srand(seed);
	master->key.low = rand();
	master->key.high = rand();
	srand(seed);
#endif
}

/*
 * Try to get root's secret key, by prompting if terminal is a tty, else trying
 * from standard input.
 * Returns 1 on success.
 */
static
getrootkey(master, prompt)
	des_block *master;
	int prompt;
{
	char *getpass();
	char *passwd;
	char name[MAXNETNAMELEN + 1];
	char secret[HEXKEYBYTES + 1];
	char *crypt();
	char *strrchr();
	int fd;

	if (!prompt) {
		/*
		 * Read secret key out of $ROOTKEY
		 */
		fd = open(ROOTKEY, O_RDONLY, 0);
		if (fd < 0) {
			randomize(master);
			return (0);
		}
		if (read(fd, secret, HEXKEYBYTES) < HEXKEYBYTES) {
			pfmt (stderr, MM_ERROR, MSG35, ROOTKEY);
			(void) close(fd);
			return (0);
		}
		(void) close(fd);
		secret[HEXKEYBYTES] = 0;
		(void) pk_setkey(0, secret);
		return (1);
	}
	/*
	 * Decrypt yellow pages publickey entry to get secret key
	 */
	passwd = getpass("root password:");
	passwd2des(passwd, master);
	getnetname(name);
	if (!getsecretkey(name, secret, passwd)) {
		pfmt (stderr, MM_ERROR, MSG25, name);
		return (0);
	}
	if (secret[0] == 0) {
		pfmt (stderr, MM_ERROR, MSG36, name);
		return (0);
	}
	(void) pk_setkey(0, secret);
	/*
	 * Store it for future use in $ROOTKEY, if possible
	 */
	fd = open(ROOTKEY, O_WRONLY|O_TRUNC|O_CREAT, 0);
	if (fd > 0) {
		char newline = '\n';

		write(fd, secret, strlen(secret));
		write(fd, &newline, sizeof (newline));
		close(fd);
	}
	return (1);
}

/*
 * Procedures to implement RPC service
 */
char *
strstatus(status)
	keystatus status;
{
	switch (status) {
	case KEY_SUCCESS:
		return ("KEY_SUCCESS");
	case KEY_NOSECRET:
		return ("KEY_NOSECRET");
	case KEY_UNKNOWN:
		return ("KEY_UNKNOWN");
	case KEY_SYSTEMERR:
		return ("KEY_SYSTEMERR");
	default:
		return ("(bad result code)");
	}
}

keystatus *
key_set_1_svc(uid, key)
	uid_t uid;
	keybuf key;
{
	static keystatus status;

	if (debugging) {
		(void) fprintf(stderr, "set(%d, %.*s) = ", uid,
				sizeof (keybuf), key);
	}
	status = pk_setkey(uid, key);
	if (debugging) {
		(void) fprintf(stderr, "%s\n", strstatus(status));
		(void) fflush(stderr);
	}
	return (&status);
}

cryptkeyres *
key_encrypt_pk_2_svc(uid, arg)
	uid_t uid;
	cryptkeyarg2 *arg;
{
	static cryptkeyres res;

	if (debugging) {
		(void) fprintf(stderr, "encrypt(%d, %s, %08x%08x) = ", uid,
				arg->remotename, arg->deskey.key.high,
				arg->deskey.key.low);
	}
	res.cryptkeyres_u.deskey = arg->deskey;
	res.status = pk_encrypt(uid, arg->remotename, &(arg->remotekey),
				&res.cryptkeyres_u.deskey);
	if (debugging) {
		if (res.status == KEY_SUCCESS) {
			(void) fprintf(stderr, "%08x%08x\n",
					res.cryptkeyres_u.deskey.key.high,
					res.cryptkeyres_u.deskey.key.low);
		} else {
			(void) fprintf(stderr, "%s\n", strstatus(res.status));
		}
		(void) fflush(stderr);
	}
	return (&res);
}

cryptkeyres *
key_decrypt_pk_2_svc(uid, arg)
	uid_t uid;
	cryptkeyarg2 *arg;
{
	static cryptkeyres res;

	if (debugging) {
		(void) fprintf(stderr, "decrypt(%d, %s, %08x%08x) = ", uid,
				arg->remotename, arg->deskey.key.high,
				arg->deskey.key.low);
	}
	res.cryptkeyres_u.deskey = arg->deskey;
	res.status = pk_decrypt(uid, arg->remotename, &(arg->remotekey),
				&res.cryptkeyres_u.deskey);
	if (debugging) {
		if (res.status == KEY_SUCCESS) {
			(void) fprintf(stderr, "%08x%08x\n",
					res.cryptkeyres_u.deskey.key.high,
					res.cryptkeyres_u.deskey.key.low);
		} else {
			(void) fprintf(stderr, "%s\n", strstatus(res.status));
		}
		(void) fflush(stderr);
	}
	return (&res);
}

cryptkeyres *
key_encrypt_1_svc(uid, arg)
	uid_t uid;
	cryptkeyarg *arg;
{
	static cryptkeyres res;

	if (debugging) {
		(void) fprintf(stderr, "encrypt(%d, %s, %08x%08x) = ", uid,
				arg->remotename, arg->deskey.key.high,
				arg->deskey.key.low);
	}
	res.cryptkeyres_u.deskey = arg->deskey;
	res.status = pk_encrypt(uid, arg->remotename, NULL,
				&res.cryptkeyres_u.deskey);
	if (debugging) {
		if (res.status == KEY_SUCCESS) {
			(void) fprintf(stderr, "%08x%08x\n",
					res.cryptkeyres_u.deskey.key.high,
					res.cryptkeyres_u.deskey.key.low);
		} else {
			(void) fprintf(stderr, "%s\n", strstatus(res.status));
		}
		(void) fflush(stderr);
	}
	return (&res);
}

cryptkeyres *
key_decrypt_1_svc(uid, arg)
	uid_t uid;
	cryptkeyarg *arg;
{
	static cryptkeyres res;

	if (debugging) {
		(void) fprintf(stderr, "decrypt(%d, %s, %08x%08x) = ", uid,
				arg->remotename, arg->deskey.key.high,
				arg->deskey.key.low);
	}
	res.cryptkeyres_u.deskey = arg->deskey;
	res.status = pk_decrypt(uid, arg->remotename, NULL,
				&res.cryptkeyres_u.deskey);
	if (debugging) {
		if (res.status == KEY_SUCCESS) {
			(void) fprintf(stderr, "%08x%08x\n",
					res.cryptkeyres_u.deskey.key.high,
					res.cryptkeyres_u.deskey.key.low);
		} else {
			(void) fprintf(stderr, "%s\n", strstatus(res.status));
		}
		(void) fflush(stderr);
	}
	return (&res);
}

/* ARGSUSED */
des_block *
key_gen_1_svc(v, s)
	void	*v;
	struct svc_req	*s;
{
	struct timeval time;
	static des_block keygen;
	static des_block key;

	(void) gettimeofday(&time, (struct timezone *) NULL);
	keygen.key.high += (time.tv_sec ^ time.tv_usec);
	keygen.key.low += (time.tv_sec ^ time.tv_usec);
	ecb_crypt((char *)&masterkey, (char *)&keygen, sizeof (keygen),
		DES_ENCRYPT | DES_HW);
	key = keygen;
	des_setparity((char *)&key);
	if (debugging) {
		(void) fprintf(stderr, "gen() = %08x%08x\n", key.key.high,
					key.key.low);
		(void) fflush(stderr);
	}
	return (&key);
}

getcredres *
key_getcred_1_svc(uid, name)
	uid_t uid;
	netnamestr *name;
{
	static getcredres res;
	static u_int gids[NGROUPS];
	struct unixcred *cred;

	cred = &res.getcredres_u.cred;
	cred->gids.gids_val = gids;
	if (!netname2user(*name, (uid_t *) &cred->uid, (gid_t *) &cred->gid,
			(int *)&cred->gids.gids_len, (gid_t *)gids)) {
		res.status = KEY_UNKNOWN;
	} else {
		res.status = KEY_SUCCESS;
	}
	if (debugging) {
		(void) fprintf(stderr, "getcred(%s) = ", *name);
		if (res.status == KEY_SUCCESS) {
			(void) fprintf(stderr, "uid=%d, gid=%d, grouplen=%d\n",
				cred->uid, cred->gid, cred->gids.gids_len);
		} else {
			(void) fprintf(stderr, "%s\n", strstatus(res.status));
		}
		(void) fflush(stderr);
	}
	return (&res);
}

/*
 * RPC boilerplate
 */
static void
keyprogram(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	union {
		keybuf key_set_1_arg;
		cryptkeyarg key_encrypt_1_arg;
		cryptkeyarg key_decrypt_1_arg;
		des_block key_gen_1_arg;
	} argument;
	char *result;
	bool_t(*xdr_argument)(), (*xdr_result)();
	char *(*local) ();
	uid_t uid;
	int check_auth;

	switch (rqstp->rq_proc) {
	case NULLPROC:
		svc_sendreply(transp, xdr_void, (char *)NULL);
		return;

	case KEY_SET:
		xdr_argument = xdr_keybuf;
		xdr_result = xdr_int;
		local = (char *(*)()) key_set_1_svc;
		check_auth = 1;
		break;

	case KEY_ENCRYPT:
		xdr_argument = xdr_cryptkeyarg;
		xdr_result = xdr_cryptkeyres;
		local = (char *(*)()) key_encrypt_1_svc;
		check_auth = 1;
		break;

	case KEY_DECRYPT:
		xdr_argument = xdr_cryptkeyarg;
		xdr_result = xdr_cryptkeyres;
		local = (char *(*)()) key_decrypt_1_svc;
		check_auth = 1;
		break;

	case KEY_GEN:
		xdr_argument = xdr_void;
		xdr_result = xdr_des_block;
		local = (char *(*)()) key_gen_1_svc;
		check_auth = 0;
		break;

	case KEY_GETCRED:
		xdr_argument = xdr_netnamestr;
		xdr_result = xdr_getcredres;
		local = (char *(*)()) key_getcred_1_svc;
		check_auth = 0;
		break;

	case KEY_ENCRYPT_PK:
		xdr_argument = xdr_cryptkeyarg2;
		xdr_result = xdr_cryptkeyres;
		local = (char *(*)()) key_encrypt_pk_2_svc;
		check_auth = 1;
		break;

	case KEY_DECRYPT_PK:
		xdr_argument = xdr_cryptkeyarg2;
		xdr_result = xdr_cryptkeyres;
		local = (char *(*)()) key_decrypt_pk_2_svc;
		check_auth = 1;
		break;


	default:
		svcerr_noproc(transp);
		return;
	}
	if (check_auth) {
		if (root_auth(transp, rqstp) == 0) {
			if (debugging) {
				pfmt (stderr, MM_ERROR, MSG37);
			}
			svcerr_weakauth(transp);
			return;
		}
		if (rqstp->rq_cred.oa_flavor != AUTH_SYS) {
			if (debugging) {
				pfmt (stderr, MM_ERROR, MSG38);
			}
			svcerr_weakauth(transp);
			return;
		}
		uid = ((struct authsys_parms *)rqstp->rq_clntcred)->aup_uid;
	}
	memset((char *) &argument, 0, sizeof (argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t)&argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local) (uid, &argument);
	if (!svc_sendreply(transp, xdr_result, (char *) result)) {
		if (debugging)
			pfmt (stderr, MM_ERROR, MSG39);
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (caddr_t)&argument)) {
		if (debugging)
			pfmt (stderr, MM_ERROR, MSG40);
		exit(1);
	}
	return;
}

static int
root_auth(trans, rqstp)
	SVCXPRT *trans;
	struct svc_req *rqstp;
{
	uid_t uid;

	if (_rpc_get_local_uid(trans, &uid) < 0) {
		if (debugging)
			pfmt (stderr, MM_ERROR, MSG41,trans->xp_netid, trans->xp_tp);
		return (0);
	}
	if (debugging)
		fprintf(stderr, "local_uid  %d\n", uid);
	if (uid == 0)
		return (1);
	if (rqstp->rq_cred.oa_flavor == AUTH_SYS) {
		if (((uid_t) ((struct authunix_parms *)rqstp->rq_clntcred)->aup_uid)
			== uid) {
			return (1);
		} else {
			if (debugging)
				pfmt (stderr, MM_ERROR, MSG42, uid, ((uid_t) 
					((struct authunix_parms *)rqstp->rq_clntcred)->aup_uid));
			return (0);
		}
	} else {
		if (debugging)
			pfmt (stderr, MM_ERROR, MSG43);
		return (0);
	}
}

static void
usage()
{
	pfmt (stderr, MM_ERROR, MSG44);
	pfmt (stderr, MM_ERROR, MSG45);
	exit(1);
}
