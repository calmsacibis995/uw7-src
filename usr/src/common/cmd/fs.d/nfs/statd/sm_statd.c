/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)sm_statd.c	1.3"
#ident	"$Header$"

/*
 * sm_statd.c consists of routines used for the intermediate
 * statd implementation(3.2 rpc.statd);
 * it creates an entry in "current" directory for each site that it monitors;
 * after crash and recovery, it moves all entries in "current"
 * to "backup" directory, and notifies the corresponding statd of its recovery.
 */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <dirent.h>
#include <rpc/rpc.h>
#include <malloc.h>
#include "sm_inter.h"
#include <errno.h>
#include <memory.h>
#include "sm_statd.h"
#include <pfmt.h>

#define MAXPGSIZE 8192
#define SM_INIT_ALARM 15
extern int debug;
extern int errno;
extern char STATE[MAXHOSTNAMELEN], CURRENT[MAXHOSTNAMELEN], BACKUP[MAXHOSTNAMELEN];
extern char *strcpy(), *strcat();
int LOCAL_STATE;

struct name_entry {
	char *name;
	int count;
	struct name_entry *prev;
	struct name_entry *nxt;
};
typedef struct name_entry name_entry;

name_entry *find_name();
name_entry *insert_name();
name_entry *record_q;
name_entry *recovery_q;

char hostname[MAXNAMLEN];
extern char hostname[MAXNAMLEN];

sm_notify(ntfp)
	stat_chge *ntfp;
{
	if (debug)
		printf("sm_notify: %s state =%d\n", ntfp->name, ntfp->state);
	send_notice(ntfp->name, ntfp->state);
}

/*
 * called when statd first comes up; it searches /etc/sm to gather
 * all entries to notify its own failure
 */
statd_init()
{
	int cc, fd;
	char buf[MAXPGSIZE];
	long base;
	struct dirent *dirp;
	DIR 	*dp;
	char from[MAXNAMLEN], to[MAXNAMLEN], path[MAXNAMLEN];
	FILE *fp, *fopen();

	if (debug)
		printf("enter statd_init\n");
	(void) gethostname(hostname, MAXNAMLEN);

	if ((fp = fopen(STATE, "a+")) == (FILE *)NULL) {
		pfmt(stderr, MM_ERROR, ":147:%s: %s failed for %s: %s\n",
		     "rpc.statd", "fopen", STATE, strerror(errno));
		exit(1);
	}
	if (chmod(STATE, 00664) == -1) {
		pfmt(stderr, MM_ERROR, ":147:%s: %s failed for %s: %s\n",
		     "rpc.statd", "chmod", STATE, strerror(errno));
		exit(1);
	}
	if (fseek(fp, 0, 0) == -1) {
		pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n",
		     "fseek", strerror(errno));
		exit(1);
	}
	if ((cc = fscanf(fp, "%d", &LOCAL_STATE)) == EOF) {
		if (debug >= 2)
			printf("empty file\n");
		LOCAL_STATE = 0;
	}
	LOCAL_STATE = ((LOCAL_STATE%2) == 0) ? LOCAL_STATE+1 : LOCAL_STATE+2;
	if (fseek(fp, 0, 0) == -1) {
		pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n",
		     "fseek", strerror(errno));
		exit(1);
	}
	fprintf(fp, "%d", LOCAL_STATE);
	(void) fflush(fp);
	if (fsync(fileno(fp)) == -1) {
		pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n",
		     "fsync", strerror(errno));
		exit(1);
	}
	(void) fclose(fp);
	if (debug)
		printf("local state = %d\n", LOCAL_STATE);

	if ((mkdir(CURRENT, 00775)) == -1) {
		if (errno != EEXIST) {
			pfmt(stderr, MM_ERROR, ":147:%s: %s failed for %s: %s\n",
			     "rpc.statd", "mkdir", CURRENT, strerror(errno));
			exit(1);
		}
	}
	if ((mkdir(BACKUP, 00775)) == -1) {
		if (errno != EEXIST) {
			pfmt(stderr, MM_ERROR, ":147:%s: %s failed for %s: %s\n",
			     "rpc.statd", "mkdir", BACKUP, strerror(errno));
			exit(1);
		}
	}

	/* get all entries in CURRENT into BACKUP */
	if ((dp = opendir(CURRENT)) == (DIR *)NULL) {
		pfmt(stderr, MM_ERROR, ":147:%s: %s failed for %s: %s\n",
		     "rpc.statd", "opendir", CURRENT, strerror(errno));
		exit(1);
	}
	for (dirp = readdir(dp); dirp != (struct dirent *)NULL; dirp = readdir(dp)) {
		if (debug)
			printf("d_name = %s\n", dirp->d_name);
		if (strcmp(dirp->d_name, ".") != 0  &&
		strcmp(dirp->d_name, "..") != 0) {
		/* rename all entries from CURRENT to BACKUP */
			(void) strcpy(from, CURRENT);
			(void) strcpy(to, BACKUP);
			(void) strcat(from, "/");
			(void) strcat(to, "/");
			(void) strcat(from, dirp->d_name);
			(void) strcat(to, dirp->d_name);
			if (rename(from, to) == -1) {
				pfmt(stderr, MM_ERROR,
				     ":148:%s: %s failed from %s to %s: %s\n",
				     "rpc.statd", "rename",
				     from, to, strerror(errno));
				exit(1);
			}
			if (debug >= 2)
				printf("rename: %s to %s\n", from, to);
		}
	}
	closedir(dp);

	/* get all entries in BACKUP into recovery_q */
	if ((dp = opendir(BACKUP)) == (DIR *)NULL) {
		pfmt(stderr, MM_ERROR,
		     ":147:%s: %s failed for %s: %s\n",
		     "rpc.statd", "opendir", BACKUP, strerror(errno));
		exit(1);
	}
	for (dirp = readdir(dp); dirp != (struct dirent *)NULL; dirp = readdir(dp)) {
		if (strcmp(dirp->d_name, ".") != 0  &&
		strcmp(dirp->d_name, "..") != 0) {
			if (debug > 2)
				printf("inserting name %s\n", dirp->d_name);
			(void) insert_name(&recovery_q, dirp->d_name);
		}
	}

	closedir(dp);

	if (recovery_q != (name_entry *)NULL) {
		if (debug >= 2)
			printf("setting alarm\n");
		(void) alarm(SM_INIT_ALARM);
	}
}

xdr_notify(xdrs, ntfp)
	XDR *xdrs;
	stat_chge *ntfp;
{
	if (!xdr_string(xdrs, &ntfp->name, MAXNAMLEN+1)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &ntfp->state)) {
		return (FALSE);
	}
	return (TRUE);
}

statd_call_statd(name)
	char *name;
{
	stat_chge ntf;
	int err;

	ntf.name =hostname;
	ntf.state = LOCAL_STATE;
	if (debug)
		printf("statd_call_statd at %s\n", name);

	err = myrpc_call(name, SM_PROG, SM_VERS, SM_NOTIFY,
		xdr_notify, &ntf, xdr_void, NULL, "visible", 0, 1);

	if (err == RPC_SUCCESS || err == RPC_TIMEDOUT || 
			err == RPC_PROGNOTREGISTERED) {
		return (0);
	} else {
		pfmt(stderr, MM_ERROR, 
		     ":149:rpc.statd: cannot talk to statd at %s\n", name);
		return (-1);
	}
}

void
sm_try(int i)
{
	name_entry *nl, *next;
	struct dirent *dirp;
	DIR 	*dp;
	char path[MAXNAMLEN];
	int fd = -1;

	if (debug >= 2)
		printf("enter sm_try: recovery_q = %s\n", recovery_q->name);

	fd = open("/etc/smworking", O_RDWR | O_CREAT | O_EXCL, 0644);
	if (fd == -1) {
		if (debug)
			printf("could not create working file\n");
	}

	next = recovery_q;
	while ((nl = next) != (name_entry *)NULL) {
		next = next->nxt;
		if (statd_call_statd(nl->name) == 0) {
			/*
			 * delete from backup dir
			 */
			(void) strcpy(path, BACKUP);
			(void) strcat(path, "/");
			(void) strcat(path, nl->name);
			if (debug >= 2)
				printf("remove monitor entry %s\n", path);
			if (unlink(path) == -1) {
				pfmt(stderr, MM_ERROR,
				     ":147:%s: %s failed for %s: %s\n",
				     "rpc.statd", "unlink", path,
				     strerror(errno));
				exit(1);
			}
			/*
			 * also remove from recovery queue
			 */
			delete_name(&recovery_q, nl->name);
		}
	}
	if (recovery_q != (name_entry *)NULL) {
		(void) signal(SIGALRM, sm_try);
		(void) alarm(SM_INIT_ALARM);
	}

	if (fd != -1) {
		close(fd);

		if (unlink("/etc/smworking")) {
			if (debug)
				printf("could not remove working file\n");
		}
	}
}

char *
xmalloc(len)
	unsigned len;
{
	char *new;

	if ((new = malloc(len)) == 0) {
		pfmt(stderr, MM_ERROR, ":102:%s: %s failed: %s\n",
		     "rpc.statd", "malloc", strerror(errno));
		return ((char *)NULL);
	}
	else {
		(void) memset(new, 0, len);
		return (new);
	}
}

/*
 * the following two routines are very similar to
 * insert_mon and delete_mon in sm_proc.c, except the structture
 * is different
 */
name_entry *
insert_name(namepp, name)
	name_entry **namepp;
	char *name;
{
	name_entry *new;

	new = (name_entry *) xmalloc (sizeof (struct name_entry));
	new->name = xmalloc(strlen(name) + 1);
	(void) strcpy(new->name, name);
	new->nxt = *namepp;
	if (new->nxt != (name_entry *)NULL)
		new->nxt->prev = new;
	*namepp = new;
	return (new);
}

delete_name(namepp, name)
	name_entry **namepp;
	char *name;
{
	name_entry *nl;

	nl = *namepp;
	while (nl != (name_entry *)NULL) {
		if (strcmp(nl->name, name) == 0) { /* found */
			if (nl->prev != (name_entry *)NULL)
				nl->prev->nxt = nl->nxt;
			else
				*namepp = nl->nxt;
			if (nl->nxt != (name_entry *)NULL)
				nl->nxt->prev = nl->prev;
			free(nl->name);
			free(nl);
			return;
		}
		nl = nl->nxt;
	}
	return;
}

name_entry *
find_name(namep, name)
	name_entry *namep;
	char *name;
{
	name_entry *nl;

	nl = namep;
	while (nl != (name_entry *)NULL) {
		if (strcmp(nl->name, name) == 0) {
			return (nl);
		}
		nl = nl->nxt;
	}
	return ((name_entry *)NULL);
}

record_name(name, op)
	char *name;
	int op;
{
	name_entry *nl;
	int fd;
	char path[MAXNAMLEN];

	if (op == 1) { /* insert */
		if ((nl = find_name(record_q, name)) == (name_entry *)NULL) {
			nl = insert_name(&record_q, name);
			/* make an entry in current directory */
			(void) strcpy(path, CURRENT);
			(void) strcat(path, "/");
			(void) strcat(path, name);
			if (debug >= 2)
				printf("create monitor entry %s\n", path);
			if ((fd = open(path, O_CREAT, 00200)) == -1){
				pfmt(stderr, MM_ERROR,
				     ":147:%s: %s failed for %s: %s\n",
				     "rpc.statd", "open", path, strerror(errno));
				if (errno != EACCES)
					exit(1);
			}
			else {
				if (debug >= 2)
					printf("%s is created\n", path);
				if (close(fd)) {
					pfmt(stderr, MM_ERROR,
					     ":102:%s: %s failed: %s\n",
					     "rpc.statd", "close",
					     strerror(errno));
					exit(1);
				}
			}
		}
		nl->count++;
	}
	else { /* delete */
		if ((nl = find_name(record_q, name)) == (name_entry *)NULL) {
			return;
		}
		nl->count--;
		if (nl->count == 0) {
			delete_name(&record_q, name);
		/* remove this entry from current directory */
			(void) strcpy(path, CURRENT);
			(void) strcat(path, "/");
			(void) strcat(path, name);
			if (debug >= 2)
				printf("remove monitor entry %s\n", path);
			if (unlink(path) == -1) {
				pfmt(stderr, MM_ERROR,
				     ":147:%s: %s failed for %s: %s\n",
				     "rpc.statd", "unlink", path,
				     strerror(errno));
				exit(1);
			}

		}
	}
}

sm_crash()
{
	name_entry *nl, *next;

	if (record_q == (name_entry *)NULL)
		return;
	next = record_q;	/* clean up record queue */
	while ((nl = next) != (name_entry *)NULL) {
		next = next->nxt;
		delete_name(&record_q, nl->name);
	}

	if (recovery_q != (name_entry *)NULL) { /* clean up all onging recovery act */
		if (debug)
			printf("sm_crash clean up\n");
		(void) alarm(0);
		next = recovery_q;
		while ( (nl = next) != (name_entry *)NULL) {
			next = next ->nxt;
			delete_name(&recovery_q, nl->name);
		}
	}
	statd_init();
}
