/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)umount.c	1.3"
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
 * nfs umount
 */

#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include <signal.h>
#include <unistd.h>
#include <rpc/rpc.h>
#include <sys/param.h>
#include <sys/mnttab.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>

/*
 * exit values
 */
#define RET_OK		0
#define RET_ERR		33	/* usage error */
#define RET_ACCESS	34	/* permission denied */
#define RET_UMNTERR	35	/* not mounted */
#define RET_BUSY	36	/* mount point busy */

int	server_pre4dot0;
void	usage();
AUTH 	*my_authsys_create_default();
int	nfs_unmount();
void	inform_server();
void	delete_mnttab();
void	freemnttab();
struct mnttab *dupmnttab();
struct mnttab *mnttab_find();

main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	int c;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:nfs umount");

	/*
	 * Set options
	 */
	while ((c = getopt(argc, argv, "")) != EOF) {
		switch (c) {
		default:
			usage();
			exit(RET_ERR);
		}
	}
	if (argc - optind != 1) {
		usage();
		exit(RET_ERR);
	}

	exit (nfs_unmount(argv[optind]));
}

void
usage()
{
	pfmt(stderr, MM_ERROR,
	     ":24:Usage: umount {server:path | mount-point}\n");
	exit(RET_ERR);
}

int
nfs_unmount(name)
	char *name;
{
	struct mnttab *mntp;
	char resolved[MAXPATHLEN];
	int save_errno = 0;

	/*
	 * If name is the mount point, we want to resolve in
	 * case it is a symbolic link.
	 */
	if (strrchr(name, ':') == NULL) {
		if (realpath(name, resolved))
			name = resolved;
	}

	mntp = mnttab_find(name);
	if (mntp) {
		name = mntp->mnt_mountp;
	}
	
	if (umount(name) < 0) {
		save_errno = errno;
		if (errno == EBUSY) {
			pfmt(stderr, MM_ERROR, ":25:%s: is busy\n", name);
			return (RET_BUSY);
		} else if (errno == EPERM) {
			pfmt(stderr, MM_ERROR, ":26:%s: permission denied\n", 
			     name);
			return (RET_ACCESS);
		} else if (errno != EINVAL) {
			pfmt(stderr, MM_NOGET|MM_ERROR,
				"%s\n", strerror(save_errno));
			pfmt(stderr, MM_ERROR, ":60:%s failed for %s\n",
				"umount", name);
			return (RET_UMNTERR);
		} 		
	}
	if (mntp) {
		inform_server(mntp->mnt_special);
		delete_mnttab(mntp->mnt_mountp);
	}
	if (save_errno) {
		pfmt(stderr, MM_ERROR, ":27:%s: is not mounted\n", name);
		return (RET_UMNTERR);
	} else {
		return (RET_OK);
	}
}

/*  Find the mnttab entry that corresponds to "name".
 *  We're not sure what the name represents: either
 *  a mountpoint name, or a special name (server:/path).
 *  Return the last entry in the file that matches.
 */
struct mnttab *
mnttab_find(name)
	char *name;
{
	FILE *fp;
	struct mnttab mnt;
	struct mnttab *res = NULL;

	fp = fopen(MNTTAB, "r");
	if (fp == NULL) {
		pfmt(stderr, MM_ERROR, ":20:%s: cannot open %s: %s\n",
		     "mnttab_find", MNTTAB, strerror(errno));
		return NULL;
	}
	while (getmntent(fp, &mnt) == 0) {
		if (strcmp(mnt.mnt_mountp , name) == 0 ||
		    strcmp(mnt.mnt_special, name) == 0) {
			if (res)
				freemnttab(res);
			res = dupmnttab(&mnt);
		}
	}

	fclose(fp);
	return (res);
}

/*
 * This structure is used to build a list of
 * mnttab structures from /etc/mnttab.
 */
struct mntlist {
	struct mnttab  *mntl_mnt;
	struct mntlist *mntl_next;
};

struct mnttab *
dupmnttab(mnt)
	struct mnttab *mnt;
{
	struct mnttab *new;

	new = (struct mnttab *)malloc(sizeof(*new));
	if (new == NULL)
		goto alloc_failed;
	memset((char *)new, 0, sizeof(*new));
	new->mnt_special = strdup(mnt->mnt_special);
	if (new->mnt_special == NULL)
		goto alloc_failed;
	new->mnt_mountp = strdup(mnt->mnt_mountp);
	if (new->mnt_mountp == NULL)
		goto alloc_failed;
	new->mnt_fstype = strdup(mnt->mnt_fstype);
	if (new->mnt_fstype == NULL)
		goto alloc_failed;
	new->mnt_mntopts = strdup(mnt->mnt_mntopts);
	if (new->mnt_mntopts == NULL)
		goto alloc_failed;
	new->mnt_time = strdup(mnt->mnt_time);
	if (new->mnt_time == NULL)
		goto alloc_failed;

	return (new);

alloc_failed:
	pfmt(stderr, MM_ERROR, ":28:%s: no memory\n", "dupmnttab");
	return (NULL);
}

void
freemnttab(mnt)
	struct mnttab *mnt;
{
	free(mnt->mnt_special);
	free(mnt->mnt_mountp);
	free(mnt->mnt_fstype);
	free(mnt->mnt_mntopts);
	free(mnt->mnt_time);
	free(mnt);
}

/*
 * Delete an entry from the mount table.
 */
void
delete_mnttab(mntpnt)
	char *mntpnt;
{
	FILE *fp;
	struct mnttab mnt;
	struct mntlist *mntl_head = NULL;
	struct mntlist *mntl_prev, *mntl;
	struct mntlist *delete = NULL;

	lockmntfile();

	fp = fopen(MNTTAB, "r+");
	if (fp == NULL) {
		pfmt(stderr, MM_ERROR, ":20:%s: cannot open %s: %s\n",
		     "delete_mnttab", MNTTAB, strerror(errno));
		return;
	}

	if (lockf(fileno(fp), F_LOCK, 0L) < 0) {
		pfmt(stderr, MM_ERROR, ":21:%s: cannot lock %s: %s\n", 
		     "delete_mnttab", MNTTAB, strerror(errno));
		fclose(fp);
		return;
	}

	/*
	 * Read the entire mnttab into memory.
	 * Remember the *last* instance of the unmounted
	 * mount point (have to take stacked mounts into
	 * account) and make sure that it's not written
	 * back out.
	 */
	while (getmntent(fp, &mnt) == 0) {
		mntl = (struct mntlist *) malloc(sizeof(*mntl));
		if (mntl == NULL)
			goto alloc_failed;
		if (mntl_head == NULL)
			mntl_head = mntl;
		else
			mntl_prev->mntl_next = mntl;
		mntl_prev = mntl;
		mntl->mntl_next = NULL;
		mntl->mntl_mnt = dupmnttab(&mnt);
		if (mntl->mntl_mnt == NULL)
			goto alloc_failed;
		if (strcmp(mnt.mnt_mountp, mntpnt) == 0)
			delete = mntl;
	}

	/* now truncate the mnttab and write almost all of it back */

	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT,  SIG_IGN);

	rewind(fp);
	if (ftruncate(fileno(fp), 0) < 0) {
		pfmt(stderr, MM_ERROR, ":29:%s: cannot truncate %s: %s\n",
		     "delete_mnttab", MNTTAB, strerror(errno));
		fclose(fp);
		return;
	}

	for (mntl = mntl_head ; mntl ; mntl = mntl->mntl_next) {
		if (mntl == delete)
			continue;
		if (putmntent(fp, mntl->mntl_mnt) <= 0) {
			pfmt(stderr, MM_ERROR, 
			     ":30:cannot put mount entry in %s: %s\n",
			     MNTTAB, strerror(errno));
			fclose(fp);
			return;
		}
	}

alloc_failed:
	(void) fclose(fp);
	return;
}

void
inform_server(fsname)
	char *fsname;
{
	char *host, *path;
	struct timeval timeout;
	CLIENT *cl;
	enum clnt_stat rpc_stat;

	host = strdup(fsname);
	if (host == NULL) {
		pfmt(stderr, MM_ERROR, ":3:%s: no memory for %s\n",
		     "inform_server", "host");
		return;
	}
	path = strchr(host, ':');
	if (path == NULL) {
		pfmt(stderr, MM_ERROR, 
		     ":31:%s is not in 'server:path' format\n", host);
		return;
	}
	*path++ = '\0';

	cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "datagram_v");
	if (cl == NULL) {
		pfmt(stderr, MM_ERROR,
		     ":12:%s:%s: server not responding for %s\n",
		     host, path, clnt_spcreateerror("clnt_create"));
		return;
	}
	if (client_bindresvport(cl) < 0) {
		pfmt(stderr, MM_ERROR, ":11:could not bind to reserved port\n");
		clnt_destroy(cl);
		return;
	}
	timeout.tv_usec = 0;
	timeout.tv_sec = 5;
	clnt_control(cl, CLSET_RETRY_TIMEOUT, (char *)&timeout);
	cl->cl_auth = my_authsys_create_default();
	timeout.tv_sec = 25;
	rpc_stat = clnt_call(cl, MOUNTPROC_UMNT, xdr_path, (caddr_t)&path,
			     xdr_void, (caddr_t)NULL, timeout);
	if (rpc_stat != RPC_SUCCESS) {
		if (rpc_stat == RPC_AUTHERROR) {
			/*
			 * could be pre 4.0 nfs server, try with 8 groups
			 */
			server_pre4dot0 = 1;
			if (cl->cl_auth)
				AUTH_DESTROY(cl->cl_auth);
			cl->cl_auth = my_authsys_create_default();
			rpc_stat = clnt_call(cl, MOUNTPROC_UMNT, xdr_path,
					     (caddr_t)&path, xdr_void,
					     (char *)NULL, timeout);
			if (rpc_stat != RPC_SUCCESS) {
				pfmt(stderr, MM_ERROR, 
				     ":12:%s:%s: server not responding for %s\n", 
				     host, path, clnt_sperror(cl, "clnt_call"));
			}
		} else {
			pfmt(stderr, MM_ERROR,
			     ":17:%s:%s: server not responding for %s\n",
			     host, path, clnt_sperror(cl, "clnt_call"));
		}
	}

	if (cl->cl_auth)
		AUTH_DESTROY(cl->cl_auth);
	clnt_destroy(cl);
	return;
}

/*
 * Create and lock a semaphore file to protect /etc/mnttab
 * against simultanous changes. Several programs (e.g. umount)
 * create a temporary file and rename it to /etc/mnttab,
 * so locking just /etc/mnttab does not suffice.
 * lockmntfile() must be called before /etc/mnttab is opened.
 */

#define SEM_FILE	"/etc/.mnt.lock"

lockmntfile()
{
	int sem_file;

	if ((sem_file = creat(SEM_FILE,0600)) == -1) {
		pfmt(stderr, MM_WARNING,
		     ":22:%s: cannot create %s: %s\n",
		     "lockmntfile", SEM_FILE, strerror(errno));
	}
	if (lockf(sem_file,F_LOCK, 0L) < 0) {
		pfmt(stderr, MM_WARNING,
		     ":21:%s: cannot lock %s: %s\n",
		     "lockmntfile", SEM_FILE, strerror(errno));
	}
}

AUTH *
my_authsys_create_default()
{
        char    machname[MAX_MACHINE_NAME + 1];
        int     len;
        uid_t   uid;
        gid_t   gid;
        gid_t   gids[NGRPS];
        AUTH    *dummy;

        if (gethostname(machname, MAX_MACHINE_NAME) == -1) {
                return ((AUTH *)NULL);
        }

        machname[MAX_MACHINE_NAME] = 0;
        uid = geteuid();
        gid = getegid();
        if ((len = getgroups(NGRPS, gids)) < 0) {
                return ((AUTH *)NULL);
        }

        if (server_pre4dot0)
                len = 8;

        dummy = authsys_create(machname, uid, gid, len, gids);
        return (dummy);
}
