/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/quotaon/quotaon.c	1.9.11.5"
#ident "$Header$"
/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

/*
 * Turn quota on/off for a filesystem.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mntent.h>
#include <fcntl.h>
#include <sys/fs/sfs_quota.h>
#include <stdio.h>
#include <sys/mnttab.h>
#include <sys/errno.h>
#include <sys/vfstab.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define	SEM_FILE	"/etc/.mnt.lock"

int	vflag;		/* verbose */
int	aflag;		/* all file systems */

#define QFNAME "quotas"
char	quotafile[MAXPATHLEN + 1];
char	*listbuf[50];
char	*whoami;

void	fixmntent();
static	char	*mntopt();
char	*hasvfsopt();

extern int	optind;
extern char	*optarg;
extern int errno;

main(argc, argv)
	int argc;
	char **argv;
{
	struct mnttab mntp;
	struct vfstab vfsbuf;
	char **listp;
	int listcnt;
	FILE *mtab, *vfstab, *tmp;
	int offmode = 0;
	int errs = 0;
	char *tmpname = "/etc/quottmpXXXXXX";
	int		status;
	int		opt;
	int	sem_file;
	int	write_fail = 0;
	
	whoami = (char *)strrchr(*argv, '/') + 1;
	if (whoami == (char *)1)
		whoami = *argv;
	if (strcmp(whoami, "quotaoff") == 0)
		offmode++;
	else if (strcmp(whoami, "quotaon") != 0) {
		fprintf(stderr, "Name must be quotaon or quotaoff not %s\n",
			whoami);
		exit(31+1);
	}
	while ((opt = getopt (argc, argv, "av")) != EOF) {
		switch (opt) {

		case 'v':
			vflag++;
			break;

		case 'a':
			aflag++;
			break;

		case '?':
			usage (whoami);
		}
	}
	if (argc <= optind && !aflag) {
		usage (whoami);
	}
	/*
	 * If aflag go through vfstab and make a list of appropriate
	 * filesystems.
	 */
	if (aflag) {

		listp = listbuf;
		listcnt = 0;
		if ((vfstab = fopen(VFSTAB, "r")) == NULL) {
			fprintf(stderr, "%s: cannot open %s : %s\n",
				whoami, VFSTAB, strerror(errno));
			exit(31+1);
		}
		while ((status = getvfsent(vfstab, &vfsbuf)) == NULL) {
			if (strcmp(vfsbuf.vfs_fstype, MNTTYPE_UFS) != 0 ||
			(! hasvfsopt(&vfsbuf, MNTOPT_RQ)))
				continue;
			*listp = malloc(strlen(vfsbuf.vfs_special) + 1);
			strcpy(*listp, vfsbuf.vfs_special);
			listp++;
			listcnt++;
		}
		fclose(vfstab);
		*listp = (char *)0;
		listp = listbuf;
	} else {
		listp = &argv[optind];
		listcnt = argc - optind;
	}

	/* Lock /etc/mnttab in agreed on manner that allows rename */
	if ((sem_file = creat(SEM_FILE, 0600)) == -1 ||
	    lockf(sem_file, F_LOCK, 0L) < 0) {
		fprintf(stderr, "%s: cannot lock %s\n", whoami, MNTTAB);
		exit(31+1);
	}
	
	/*
	 * Open temporary version of mnttab
	 */
	mktemp(tmpname);
	if (creat(tmpname, 0644) < 0) {
		fprintf ( stderr, "%s: cannot create %s : %s\n",
			whoami, tmpname, strerror(errno));
		exit(31+1);
	}
	if (chmod(tmpname, 0644) < 0) {
		fprintf ( stderr, "%s: cannot chmod %s : %s\n",
			whoami, tmpname, strerror(errno));
		exit(31+1);
	}
	if ((tmp = fopen(tmpname, "a")) == NULL) {
		fprintf ( stderr, "%s: cannot open %s : %s\n",
			whoami, tmpname, strerror(errno));
		exit(31+1);
	}
		
	/*
	 * Open real mnttab
	 */
	mtab = fopen(MNTTAB, "r");
	if (mtab == NULL) {
		fprintf ( stderr, "%s: cannot open %s : %s\n",
			whoami, MNTTAB, strerror(errno));
		exit(31+1);
	}
	/* check every entry for validity before we change mnttab */
	while ((status = getmntent(mtab, &mntp)) == 0)
		;
	if (status > 0)
		mnterror(status);
	rewind(mtab);

	/*
	 * Loop through mnttab writing mount record to temp mtab.
	 * If a file system gets turn on or off modify the mount
	 * record before writing it.
	 */
	while ((status = getmntent(mtab, &mntp)) == NULL) {
		if (strcmp(mntp.mnt_fstype, MNTTYPE_UFS) == 0 &&
		    !hasmntopt(&mntp, MNTOPT_RO) &&
		    (oneof(mntp.mnt_special, listp, listcnt) ||
		     oneof(mntp.mnt_mountp, listp, listcnt)) ) {
			errs += quotaonoff(&mntp, offmode);
		}
		if (putmntent(tmp, &mntp) < 0) {
			if (!write_fail)
				fprintf(stderr, 
		"%s: cannot write to %s; %s will not be modified\n",
					whoami, tmpname, MNTTAB);
			write_fail++;
		};
	}
	fclose(mtab);
	fclose(tmp);

	/*
	 * Move temp mtab to mtab
	 */
	if (!write_fail && (rename(tmpname, MNTTAB) < 0)) {
		fprintf ( stderr, "%s: cannot rename %s to %s : %s\n",
			whoami, tmpname, MNTTAB, strerror(errno));
		exit(31+1);
	}
	while (listcnt--) {
		if (*listp)
		fprintf(stderr, "%s: cannot do %s\n", whoami, *listp);
		listp++;
	}
	errs += write_fail;
	if (errs > 0)
		errs += 31;
	exit(errs);
}

quotaonoff(mntp, offmode)
	register struct mnttab *mntp;
	int offmode;
{

	if (offmode) {
		if (quotactl(Q_QUOTAOFF, mntp->mnt_mountp, 0, NULL) < 0)
			goto bad;
		if (vflag)
			printf("%s: quotas turned off\n", mntp->mnt_mountp);
	} else {
		(void) sprintf(quotafile, "%s/%s", mntp->mnt_mountp, QFNAME);
		if (quotactl(Q_QUOTAON, mntp->mnt_mountp, 0, quotafile) < 0)
			goto bad;
		if (vflag)
			printf("%s: quotas turned on\n", mntp->mnt_mountp);
	}
	fixmntent(mntp, offmode);
	return (0);
bad:
	return (1);
}

oneof(target, olistp, on)
	char *target;
	register char **olistp;
	register int on;
{
	int n = on;
	char **listp = olistp;

	while (n--) {
		if (*listp && strcmp(target, *listp) == 0) {
			*listp = (char *)0;
			return (1);
		}
		listp++;
	}
	return (0);
}

char opts[1024];

void	fixmntent(mntp, offmode)
	register struct mnttab *mntp;
	int offmode;
{
	register char *qst, *qend;

	if (offmode) {
		if (hasmntopt(mntp, MNTOPT_NOQUOTA))
			return;
		qst = hasmntopt(mntp, MNTOPT_QUOTA);
	} else {
		if (hasmntopt(mntp, MNTOPT_QUOTA))
			return;
		qst = hasmntopt(mntp, MNTOPT_NOQUOTA);
	}
	if (qst) {
		qend = (char *)strchr(qst, ',');
		if (qend == NULL) {
			if (qst != mntp->mnt_mntopts)
				qst--;			/* back up to ',' */
			*qst = '\0';
		} else {
			if (qst == mntp->mnt_mntopts)
				qend++;			/* back up to ',' */
			while (*qst++ = *qend++);
		}
	}
	sprintf(opts, "%s,%s", mntp->mnt_mntopts,
	    offmode? MNTOPT_NOQUOTA : MNTOPT_QUOTA);
	mntp->mnt_mntopts = opts;
}

usage (whoami)
	char	*whoami;
{

	fprintf(stderr, "ufs usage:\n");
	fprintf(stderr, "\t%s [-v] -a\n", whoami);
	fprintf(stderr, "\t%s [-v] filesys ...\n", whoami);
		exit(31+1);
}


quotactl (cmd, mountpt, uid, addr)
	int		cmd;
	char		*mountpt;
	int		uid;
	caddr_t		addr;
{
	int		fd;
	int		status;
	struct quotctl	quota;
	char		mountpoint[256];
	
	if (mountpt == NULL || mountpt[0] == '\0') {
		errno = ENOENT; 
		return (-1);   
	}
	(void) sprintf(mountpoint, "%s/%s", mountpt, QFNAME);
	if ((fd = open (mountpoint, O_RDWR)) < 0) {
		fprintf (stderr, "quotactl: %s ", mountpoint);
		perror ("open");
		return (-1);
	}

	quota.op = cmd;
	quota.uid = uid;
	quota.addr = addr;
	status = ioctl (fd, Q_QUOTACTL, &quota);
	close (fd);
	if (status < 0) {
		fprintf(stderr, "quotactl: ");
		perror(mountpt);
	}
	return (status);
}

char *
hasmntopt(mnt, opt)
        register struct mnttab *mnt;
        register char *opt;
{
        char *f, *opts;
        static char *tmpopts;

        if (tmpopts == 0) {
                tmpopts = (char *)calloc(256, sizeof (char));
                if (tmpopts == 0)
                        return (0);
        }
	if (mnt->mnt_mntopts)
        	strcpy(tmpopts, mnt->mnt_mntopts);
        opts = tmpopts;
        f = mntopt(&opts);
        for (; *f; f = mntopt(&opts)) {
                if (strncmp(opt, f, strlen(opt)) == 0)
                        return (f - tmpopts + mnt->mnt_mntopts);
        }
        return (NULL);
}

char *
hasvfsopt(vfs, opt)
        register struct vfstab *vfs;
        register char *opt;
{
        char *f, *opts;
        static char *tmpopts;

        if (tmpopts == 0) {
                tmpopts = (char *)calloc(256, sizeof (char));
                if (tmpopts == 0)
                        return (0);
        }
	if (vfs->vfs_mntopts)
        	strcpy(tmpopts, vfs->vfs_mntopts);
        opts = tmpopts;
        f = mntopt(&opts);
        for (; *f; f = mntopt(&opts)) {
                if (strncmp(opt, f, strlen(opt)) == 0)
                        return (f - tmpopts + vfs->vfs_mntopts);
        }
        return (NULL);
}

static char *
mntopt(p)
        char **p;
{
        char *cp = *p;
        char *retstr;

        while (*cp && isspace(*cp))
                cp++;
        retstr = cp;
        while (*cp && *cp != ',')
                cp++;
        if (*cp) {
                *cp = '\0';
                cp++;
        }
        *p = cp;
        return (retstr);
}

mnterror(flag)
	int	flag;
{
	switch (flag) {
	case MNT_TOOLONG:
		fprintf(stderr, "%s: line in mnttab exceeds %d characters\n",
			whoami, MNT_LINE_MAX-2);
		break;
	case MNT_TOOFEW:
		fprintf(stderr, "%s: line in mnttab has too few entries\n",
			whoami);
		break;
	case MNT_TOOMANY:
		fprintf(stderr, "%s: line in mnttab has too many entries\n",
			whoami);
		break;
	}
	exit(1);
}
