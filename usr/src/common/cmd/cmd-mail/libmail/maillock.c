#ident	"@(#)maillock.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)maillock.c	2.26 'attmail mail(1) command'"
#include "libmail.h"


/*
    NAME
	maillock - set lock for user
	maildlock - set lock for user in a given directory
	mailunlock - unset lock for user
	mailrdlock - set read lock for user
	mailurdlock - unset read lock for user

    SYNOPSIS
	int maillock(char *user, int retrycnt);
	int maildlock(char *user, int retrycnt, char *dir, int showerrs);
	int mailunlock(void);
	int mailrdlock(char *user);
	int mailurdlock(void);

    DESCRIPTION
	Manage locks in the mail directory and and mail readlock directory.
*/
static	char	curlock[FILENAME_MAX];
static	int	locked = 0;
static	int	lffd = -1;	/* lock file file descriptor */

int maillock (user, retrycnt)
char	*user;
int	retrycnt;
{
	return maildlock(user, retrycnt, MAILDIR, 1);
}

int maildlock (user, retrycnt, maildir, showerrs)
char	*user;
int	retrycnt;
char	*maildir;
int	showerrs;
{
	char		tmplock[FILENAME_MAX];
	char	 	buf[80];
	register int	i;
	pid_t 		lpid;
	int		len;
	extern	int	errno;
	FILE	 	*fp;
	pid_t		pid = getpid();
	struct stat	statbuf;
	int		oneshot = 0;
	uid_t           uid;

	if (locked) {
		return (L_SUCCESS);
	}

	/*
		If retrycnt == 0, then we do the test once but don't block.
	*/
	if (retrycnt == 0) {
		retrycnt = 2;
		oneshot = 1;
	}

	uid = getuid();
	/*
		13 characters, as we couldn't discern between the
		Cannot create a lockfile with a basename of more than
		lockfile and the file itself.
	

	if (strlen(user) > 13) {
		return (L_NAMELEN);
	}

	*/

	(void) sprintf(tmplock,"%s/%s", maildir, ":saved");
	if ((stat(tmplock, &statbuf) == 0) && ((statbuf.st_mode & S_IFMT) == S_IFDIR))
		(void) sprintf(tmplock,"%s/%s/LCK..%ld", maildir, ":saved", (long)pid);
	else
		(void) sprintf(tmplock,"%s/LCK..%ld", maildir, (long)pid);
	(void) unlink (tmplock); /* In case it's left over from some disaster */
	if ((lffd = open(tmplock, O_WRONLY | O_CREAT | O_TRUNC, oneshot ? 0664 : 02664)) == -1) {
		if (showerrs)
			pfmt(stderr, MM_ERROR,
				":115:Cannot open '%s': %s\n", tmplock, strerror(errno));
		return (L_TMPLOCK);
	}
	(void) sprintf(buf,"%ld",(long)pid);
	len = strlen(buf) + 1;
	if (write(lffd, buf, (unsigned)len) != len) {
		if (showerrs)
			pfmt(stderr, MM_ERROR,
				":116:Cannot write pid to '%s': %s\n",
				tmplock, strerror(errno));
		(void) close(lffd);
		(void) unlink(tmplock);
		return (L_TMPWRITE);
	}
#ifndef NOLOCKF
	if (lockf(lffd, F_TLOCK, (long)0) == -1) {
		if (showerrs)
			pfmt(stderr, MM_ERROR,
				":117:Cannot set mandatory lock on '%s': %s\n",
				tmplock, strerror(errno));
		(void) close(lffd);
		(void) unlink(tmplock);
		return (L_MANLOCK);
	}
#endif
	/* Don't close lock file here to keep mandatory file lock in place. */
	/* It gets closed below in mailunlock() */

	/*
	 * Attempt to link temp lockfile to real lockfile.
	 * If link fails because real lockfile already exists,
	 * check that the pid it contains refers to a 'live'
	 * process. If not, remove it and try again...
	 */
	(void) sprintf(curlock,"%s/%u.lock", maildir, uid);
	for (i = 0; i < retrycnt; i++) {
		if (link(tmplock, curlock) == -1) {
			/* linking to real lockfile failed */
			if (errno == EEXIST) {
				/* lock file already exists */
				if ((fp = fopen(curlock,"r+")) != NULL) {
					if (fscanf(fp,"%ld",&lpid) == 1) {
					    if ((kill(lpid,0) == (pid_t)-1) &&
						(errno == ESRCH)) {
						    /* process no longer exists */
						    rewind(fp);
						    (void) sprintf(buf,"%ld\n",(long)pid);
						    (void) fwrite(buf, strlen(buf), 1, fp);
						    (void) unlink(curlock);
					    }
					}
					(void)fclose(fp);
				}
				if (oneshot)
					/* in case the pid was good, we try again */
					continue;
				(void) sleep((unsigned)(5*(i+1)));
			} else {
				/* lock file doesn't exist; some other error */
				if (showerrs)
					pfmt(stderr, MM_ERROR,
						":118:Link of mailfile lock failed: %s\n",
						strerror(errno));
				(void) close(lffd);
				(void) unlink(tmplock);
				return (L_ERROR);
			}
		} else {
			/* link worked */
			(void) unlink(tmplock);
			locked++;
			return (L_SUCCESS);
		}
	}

	/* ran out of tries */
	(void) close(lffd);
	(void) unlink(tmplock);
	return (L_MAXTRYS);
}

int mailunlock()
{
	if (locked) {
		(void) close(lffd); lffd = -1;
		(void) unlink(curlock);
		locked = 0;
	}
	return (L_SUCCESS);
}

/*
#ifdef F_SETLK
    Maintain a kernel readlock on the /var/mail/:readlock/user file.
#else
    Since maildlock() currently only manages a single lock at a time,
    we fake things out by saving and restoring the static values that
    maildlock() manipulates, and using our own values while calling
    the routines.
#endif
*/

#ifdef F_SETLK
static	FILE	*rdlk_fp;
#else
static	int	rdlk_locked = 0;
static	int	rdlk_lffd = -1;	/* lock file file descriptor */
#endif
static	char	rdlk_curlock[FILENAME_MAX];

int mailrdlock (user)
char *user;
{
#ifdef F_SETLK
    struct flock l;
	uid_t uid;
	uid = getuid();

    /* make certain that the :readlock directory is there */
    if (access(RDLKDIR, F_OK) != 0)
	{
	struct group *grpptr = getgrnam("mail");
	int okay = 0;
	if (grpptr != NULL)
	    {
	    gid_t mailgrp = grpptr->gr_gid;	/* numeric id of group 'mail' */
	    int	omask = umask(0);		/* we need to control the mode of the dir */
	    char *lockfile = strdup(RDLKDIR);	/* copy name of dir so can get rid of trailing / */
	    if (lockfile)
		{
		int len = strlen(lockfile);
		if (lockfile[len-1] == '/')	/* zap the trailing / */
		    lockfile[len-1] = '\0';
		if (mkdir(lockfile, 1777) != -1)/* make the directory */
		    {
		    (void) chown(lockfile, 0, mailgrp);	/* ???? posix_chown? */
		    okay = 1;
		    }
		}
	    (void)umask(omask);
	    }
	if (!okay)
		pfmt(stderr, MM_ERROR, ":123:Cannot create %s: %s\n", RDLKDIR, strerror(errno));
	}

    (void) sprintf(rdlk_curlock, "%s/%u.lock", RDLKDIR, uid);
    rdlk_fp = fopen(rdlk_curlock, "w");
    if (!rdlk_fp)
	return L_TMPLOCK;

    l.l_type = F_WRLCK;
    l.l_whence = 0;
    l.l_start = l.l_len = 0L;
    if (fcntl(fileno(rdlk_fp), F_SETLK, &l) < 0)
	{
	(void) fclose(rdlk_fp);
	rdlk_fp = 0;
	return L_MAXTRYS;
	}
    return L_SUCCESS;
#else
    char temp_curlock[FILENAME_MAX];
    int	temp_locked;
    int	temp_lffd;
    int	ret;

    /* save the current lock's values */
    (void) memcpy(temp_curlock, curlock, sizeof(curlock));
    temp_locked = locked;
    temp_lffd = lffd;

    /* set current readlock's values */
    (void) memcpy(curlock, rdlk_curlock, sizeof(curlock));
    locked = rdlk_locked;
    lffd = rdlk_lffd;

    /* attempt the lock */
    ret = maildlock(user, 0, RDLKDIR, 0);

    /* save the current readlock's values */
    (void) memcpy(rdlk_curlock, curlock, sizeof(curlock));
    rdlk_locked = locked;
    rdlk_lffd = lffd;

    /* save the current lock's values */
    (void) memcpy(curlock, temp_curlock, sizeof(curlock));
    locked = temp_locked;
    lffd = temp_lffd;

    return ret;
#endif
}

int mailurdlock()
{
#ifdef F_SETLK
    if (rdlk_fp)
	{
	(void) fclose(rdlk_fp);
	rdlk_fp = 0;
	(void) unlink(rdlk_curlock);
	}
    return L_SUCCESS;
#else
    char temp_curlock[FILENAME_MAX];
    int	temp_locked;
    int	temp_lffd;
    int	ret;

    /* save the current lock's values */
    (void) memcpy(temp_curlock, curlock, sizeof(curlock));
    temp_locked = locked;
    temp_lffd = lffd;

    /* set current readlock's values */
    (void) memcpy(curlock, rdlk_curlock, sizeof(curlock));
    locked = rdlk_locked;
    lffd = rdlk_lffd;

    /* attempt the lock */
    ret = mailunlock();

    /* save the current readlock's values */
    (void) memcpy(rdlk_curlock, curlock, sizeof(curlock));
    rdlk_locked = locked;
    rdlk_lffd = lffd;

    /* save the current lock's values */
    (void) memcpy(curlock, temp_curlock, sizeof(curlock));
    locked = temp_locked;
    lffd = temp_lffd;

    return ret;
#endif
}
