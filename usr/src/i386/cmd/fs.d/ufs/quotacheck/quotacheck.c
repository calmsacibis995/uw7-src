/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ufs.cmds:i386/cmd/fs.d/ufs/quotacheck/quotacheck.c	1.8.9.3"
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
 * Fix up / report on disc quotas & usage
 */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mntent.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <sys/fs/sfs_inode.h>
#include <sys/fs/sfs_fs.h>
#include <sys/fs/sfs_quota.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mnttab.h>
#include <sys/vfstab.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>

union {
	struct	fs	sblk;
	char	dummy[MAXBSIZE];
} un;
#define	sblock	un.sblk

#define	ITABSZ	256
struct	dinode	itab[ITABSZ];
struct	dinode	*dp;

#define LOGINNAMESIZE	8
struct fileusage {
	struct fileusage *fu_next;
	u_long fu_curfiles;
	u_long fu_curblocks;
	u_short	fu_uid;
	char fu_name[LOGINNAMESIZE + 1];
};
#define FUHASH 997
struct fileusage *fuhead[FUHASH];
struct fileusage *lookup();
struct fileusage *adduid();
int highuid;

int fi;
ino_t ino;

/* list_elem is used to build a list of all FS to be checked */
struct	list_elem {
	struct	list_elem	*next;
	char	*mountp;
	char	*block;
	char	*raw;
	char	*arg_was;
	char	*qfname;
	int	found;		/* used for matching /etc/mnttab */
}; 

void	acct();
void	bread();
int	chkquota();
struct	dinode	*ginode();
char	*hasvfsopt();
struct	list_elem	*in_list();
struct	list_elem	*make_elem();
struct	list_elem	*make_list();
char	*makerawname();
static	char *mntopt();
int	preen();
int	quotactl();
struct	list_elem	*scan_vfstab();
void	usage();

extern int	optind;
extern char	*optarg;
extern int	errno;

int	vflag;		/* verbose */
int	aflag;		/* all file systems */
int	pflag;		/* fsck like parallel check */
char	*myname;

#define QFNAME "quotas"

#define	FALSE	0
#define	TRUE	1

struct fileusage zerofileusage;

main(argc, argv)
	int argc;
	char **argv;
{
	register struct fileusage *fup;
	struct list_elem *list, *listp;
	int errs = 0;
	struct passwd *pw;
	int	opt;

	myname = argv[0];
	while ((opt = getopt (argc, argv, "vap")) != EOF) {
		switch (opt) {

		case 'v':
			vflag++;
			break;

		case 'a':
			aflag++;
			break;
	
		case 'p':
			pflag++;
			break;

		case '?':
			usage ();
			/* NOTREACHED */
		}
	}
	if (argc <= optind && !aflag) {
		usage ();
	}

	setpwent();
	while ((pw = getpwent()) != 0) {
		fup = lookup((u_short)pw->pw_uid);
		if (fup == 0) {
			fup = adduid((u_short)pw->pw_uid);
			strncpy(fup->fu_name, pw->pw_name,
				sizeof(fup->fu_name));
		}
	}
	endpwent();

	if (quotactl(Q_ALLSYNC, NULL, 0, NULL) < 0 && errno == ENOTTY && vflag)
		(void) fprintf( stderr,
		"%s: Warning: Quotas are not compiled into this kernel\n",
			myname);
	sync();

	list = make_list(&argv[optind], argc - optind, aflag);

	if (pflag) {
		errs = preen(list);
	} else {
		for (listp = list ; listp; listp = listp->next) {
			if (listp->found)
				errs += chkquota(listp);
		}
	}
	if (errs > 0)
		errs += 31;
	exit(errs);
}

struct list_elem	*make_list(argv, argc, aflag)
	char	*argv[];	/* args which should be file systems */
	int	argc;		/* number of such args */
	int	aflag;		/* ignore args if set; select 'all' */
{
	struct mnttab mntp;
	FILE *mtab;
	struct list_elem *list, *listp, *tail;
	int	i;
	int	status;
	
	/*
	 * Go through vfstab and make a list of appropriate
	 * filesystems.
	 */
	list = scan_vfstab(argv, argc, aflag, &tail);

	/* Now check mnttab */
	if ((mtab = fopen(MNTTAB, "r")) == NULL) {
		(void) fprintf(stderr, "%s: cannot open %s: %s\n",
				myname, MNTTAB, strerror(errno));
		exit(31+8);
	}
	
	while ((status = getmntent(mtab, &mntp)) != (-1)) {
		if (status > 0) {
			(void) fprintf(stderr, 
				"%s: error encountered reading %s\n",
				myname, MNTTAB);
			continue;
		}
		if ((strcmp(mntp.mnt_fstype, MNTTYPE_UFS) != 0) ||
		    (hasmntopt(&mntp, MNTOPT_RO)))
			continue;
		listp = in_list(list, &mntp);
		if (!listp) {	/* No match */
			if (aflag)
				continue;
			/* Maybe its an argument which isn't in vfstab */
			for (i = 0; i < argc; i++) {
			    if (!argv[i])
				continue;
			    if ((strcmp(argv[i], mntp.mnt_special) == 0)
			    || (strcmp(argv[i], mntp.mnt_mountp) == 0)) {
				/* Got a match */
				tail = make_elem(&list, tail, 
						mntp.mnt_mountp,
						mntp.mnt_special, 
						NULL,
						argv[i]);
				listp = tail;
				argv[i] = NULL;
			    }
			}
		}
		if (listp)
			listp->found = TRUE;
	}
			
	fclose(mtab);
	
	/* 
	 * Determine qfname for all elements of list.
	 * Also, deal with unknown raw device names (use makerawname)
	 * and block device names (fail, unless raw name known)
	 * Complain about elements (from vfstab) not found in /etc/mnttab
	 * Complain about explicit arguments not found in /etc/mnttab
	 */
	for (listp =list; listp; listp = listp->next) {
		if (listp->found) {
			listp->qfname = malloc(strlen(listp->mountp) +
						strlen(QFNAME) + 2);
			if (!listp->qfname) {
				(void) fprintf(stderr, "%s: out of memory\n", 
					myname);
				exit(31+8);
			} else {
				(void) sprintf(listp->qfname, "%s/%s", 
					listp->mountp, QFNAME);
			}
			if (!listp->raw) {
			    if (listp->block) {
				listp->raw 
				    = strdup(makerawname(listp->block));
				if (!listp->raw) {
				    (void) fprintf(stderr, 
					"%s: out of memory\n", myname);
				    exit(31+8);
				}
			    } else {
				(void) fprintf(stderr, 
			      "%s: cannot determine device name for %s\n",
				myname, 
				aflag ? listp->mountp : listp->arg_was);
				listp->found = FALSE;
			    }
			}
		} else {
			(void) fprintf(stderr, 
				"%s: cannot check %s\n", myname,
				aflag ? listp->raw : listp-> arg_was);
		}
	}
	if (!aflag) {
		for (i=0; i<argc; i++) {
			if (argv[i]) {
			    (void) fprintf(stderr, 
					"%s: cannot check %s\n", 
					myname, argv[i]);
			}
		}
	}
	
	return list;
}

/* Scan vfstab for file systems to be checked. Build initial list */
struct list_elem	*scan_vfstab(argv, argc, aflag, tailp)
	char	*argv[];	/* args which should be file systems */
	int	argc;		/* number of such args */
	int	aflag;		/* ignore args if set; select 'all' */
	struct	list_elem **tailp; /* return w/ ptr to last item of list*/
{
	struct vfstab vfsbuf;
	FILE *vfstab;
	struct list_elem *list, *tail;
	int	i;
	int	status;
	
	/*
	 * Go through vfstab and make a list of appropriate
	 * filesystems.
	 */
	tail = list = NULL;
	if ((vfstab = fopen(VFSTAB, "r")) == NULL) {
		(void) fprintf(stderr, "%s: cannot open %s: %s\n",
				myname, VFSTAB, strerror(errno));
		if (aflag)
			exit(31+8);
		else {
			/* Very picky. 
			   OK to try this even if vfstab unavail.
			   This mimics previous code, which ignored vfstab
			   unless -a was specified */
			*tailp = NULL;
			return NULL;
		}
	}
	
	while ((status = getvfsent(vfstab, &vfsbuf)) != (-1)) {
		if (status > 0) {
			(void) fprintf(stderr, 
				"%s: error encountered reading %s\n",
				myname, VFSTAB);
			continue;
		}
		if (strcmp(vfsbuf.vfs_fstype, MNTTYPE_UFS) != 0)
			continue;
		if (aflag) {
			if (hasvfsopt(&vfsbuf, MNTOPT_RQ)) {
				/* This entry from vfstab, tentatively, 
				   is to be checked*/
				tail = make_elem(&list, tail, 
						vfsbuf.vfs_mountp,
						vfsbuf.vfs_special, 
						vfsbuf.vfs_fsckdev, NULL);
			} else {
				continue;
			}
		} else {	/* Start of !aflag condition */
			for ( i = 0; i < argc; i++)
			{
			    if (!argv[i])
				continue;
			    if ((strcmp(argv[i], vfsbuf.vfs_special)) &&
				(strcmp(argv[i], vfsbuf.vfs_fsckdev)) &&
				(strcmp(argv[i], vfsbuf.vfs_mountp)))
					continue;
			    /* Have found an explicitly specified fs in 
				vfstab */
			    tail = make_elem(&list, tail, 
				   vfsbuf.vfs_mountp, vfsbuf.vfs_special,
				   vfsbuf.vfs_fsckdev, argv[i]);
			    argv[i] = NULL;
			    break;
			}
		}
	}
	fclose(vfstab);
	*tailp = tail;
	return list;
}

/* 
 * Create a list element. Put into list. Return ptr to new element 
 * All fields EXCEPT ARG copied into malloc'd space.
 * Exit w/ message if memory not available.
 * Handle empty list correctly (caller must deal w/ setting list head).
 * Also handle empty (NULL) elements correctly.
 * set found to false.
 */
struct	list_elem	*make_elem(headp, tail, mountp, block, raw, arg)
	struct	list_elem	**headp;
	struct	list_elem	*tail;
	char	*mountp;
	char	*block;
	char	*raw;
	char	*arg;
{
	struct	list_elem	*new;
	
	new = malloc(sizeof(struct list_elem));
	if (!new) {
		(void) fprintf(stderr, "%s: out of memory\n", myname);
		exit(31+8);
	}
	if (tail) {
		tail->next = new;
	} else if (! (*headp)) { /* 1st elem of list; check is paranoid */
		*headp = new;
	}
	new->next = NULL;

	if (mountp) {
		new->mountp = strdup(mountp);
		if (!new->mountp) {
			(void) fprintf(stderr, 
					"%s: out of memory\n", myname);
			exit(31+8);
		}
	} else {
		new -> mountp = NULL;
	}
	if (block) {
		new->block = strdup(block);
		if (!new->block) {
			(void) fprintf(stderr, 
					"%s: out of memory\n", myname);
			exit(31+8);
		}
	} else {
		new -> block = NULL;
	}
	if (raw) {
		new->raw = strdup(raw);
		if (!new->raw) {
			(void) fprintf(stderr, 
					"%s: out of memory\n", myname);
			exit(31+8);
		}
	} else {
		new -> raw = NULL;
	}
	new->arg_was = arg;
	new->found = FALSE;
	return new;
}

struct	list_elem	*in_list(list, mntp)
	struct	list_elem	*list;
	struct	mnttab	*mntp;
{
	while (list) {
		if ((strcmp(list->mountp, mntp->mnt_mountp) == 0) ||
		    (strcmp(list->block, mntp->mnt_special) == 0)) {
			list->found = TRUE;
			return (list);
		} else list = list->next;
	}
	return (NULL);
}

/* 
 * Check all file systems in list, in parallel.
 * Return sum of error return values encountered 
 * (likely to be number of errors).
 * Function name is 'historical'.
 */
int	preen(listp)
	struct	list_elem	*listp;
{
	int errs = 0;
	int status;

	for ( ; listp; listp=listp->next) {
		if (!listp->found)
			continue;
		switch (fork()) {
			case -1:
				(void) fprintf(stderr, 
						"%s: Fork failed: %s\n",
						myname, strerror(errno));
				exit(31+8);
				break;

			case 0:
				exit(chkquota(listp));
			default:
				break;
		}
	}

	while ((wait(&status) != -1) || (errno != ECHILD)) { 
		/* if a child exitted, accumulate exit codes */
		/* Otherwise it could be anything from null starting 
		   listp to someone playing w/ stop + cont
		   to children crashing. So don't count what we don't
		   understand */
		if (WIFEXITED(status))
			errs+=WEXITSTATUS(status);
	}
	
	return (errs);
}

int	chkquota(listp)
	struct list_elem *listp;
{
	register struct fileusage *fup;
	dev_t quotadev;
	FILE *qf;
	register u_short uid;
	register u_short highquota;
	int cg, i;
	char *rawdisk = listp->raw;
	char *fsdev = listp->block;
	char *fsfile = listp->mountp;
	char *qffile = listp->qfname;
	struct stat statb;
	struct dqblk dqbuf;
	extern int errno;
	int	rv;
	
	/* Try to open quota file. 
	   If not found, just ignore this file system.
	*/
	qf = fopen(qffile, "r+");
	if (qf == NULL) {
		if (errno == ENOENT) {
			rv = 0;
		}
		else {
			rv = 1;
			(void) fprintf(stderr, "%s: cannot open %s: %s\n",
				myname, qffile, strerror(errno));
		}
		close(fi);
		return (rv);
	}
	
	if (vflag)
		(void) printf("*** Checking quotas for %s (%s)\n", 
				rawdisk, fsfile);
	fi = open(rawdisk, 0);
	if (fi < 0) {
		(void) fprintf(stderr, "%s: cannot open %s: %s\n",
				myname, rawdisk, strerror(errno));
		fclose(qf);
		return (1);
	}
	if (fstat(fileno(qf), &statb) < 0) {
		(void) fprintf(stderr, "%s: cannot stat %s: %s\n",
				myname, qffile, strerror(errno));
		fclose(qf);
		close(fi);
		return (1);
	}
	quotadev = statb.st_dev;
	if (stat(fsdev, &statb) < 0) {
		(void) fprintf(stderr, "%s: cannot stat %s: %s\n",
				myname, fsdev, strerror(errno));
		fclose(qf);
		close(fi);
		return (1);
	}
	if (quotadev != statb.st_rdev) {
		(void) fprintf(stderr, 
			"%s: %s dev (0x%x) mismatch %s dev (0x%x)\n",
			myname, qffile, quotadev, fsdev, statb.st_rdev);
		fclose(qf);
		close(fi);
		return (1);
	}
	bread(SBLOCK, (char *)&sblock, SBSIZE);
	ino = 0;
	for (cg = 0; cg < sblock.fs_ncg; cg++) {
		dp = NULL;
		for (i = 0; i < sblock.fs_ipg; i++)
			acct(ginode());
	}
	highquota = 0;
	for (uid = 0; (int)uid <= (int)highuid; uid++) {
		(void) fseek(qf, (long)dqoff(uid), 0);
		(void) fread(&dqbuf, sizeof(struct dqblk), 1, qf);
		if (feof(qf))
			break;
		fup = lookup(uid);
		if (fup == 0)
			fup = &zerofileusage;
		if (dqbuf.dqb_bhardlimit || dqbuf.dqb_bsoftlimit ||
		    dqbuf.dqb_fhardlimit || dqbuf.dqb_fsoftlimit) {
			highquota = uid;
		} else {
			fup->fu_curfiles = 0;
			fup->fu_curblocks = 0;
		}
		if (dqbuf.dqb_curfiles == fup->fu_curfiles &&
		    dqbuf.dqb_curblocks == fup->fu_curblocks) {
			fup->fu_curfiles = 0;
			fup->fu_curblocks = 0;
			continue;
		}
		if (vflag) {
			if (pflag || aflag)
				(void) printf("%s: ", rawdisk);
			if (fup->fu_name[0] != '\0')
				(void) printf("%-10s fixed:", 
						fup->fu_name);
			else
				(void) printf("#%-9d fixed:", uid);
			if (dqbuf.dqb_curfiles != fup->fu_curfiles)
				(void) printf("  files %d -> %d",
				    dqbuf.dqb_curfiles, fup->fu_curfiles);
			if (dqbuf.dqb_curblocks != fup->fu_curblocks)
				(void) printf("  blocks %d -> %d",
					dqbuf.dqb_curblocks, 
					fup->fu_curblocks);
			(void) printf("\n");
		}
		dqbuf.dqb_curfiles = fup->fu_curfiles;
		dqbuf.dqb_curblocks = fup->fu_curblocks;
		(void) fseek(qf, (long)dqoff(uid), 0);
		(void) fwrite(&dqbuf, sizeof(struct dqblk), 1, qf);
		(void) quotactl(Q_SETQUOTA, fsfile, uid, &dqbuf);
		fup->fu_curfiles = 0;
		fup->fu_curblocks = 0;
	}
	(void) fflush(qf);
	(void) ftruncate(fileno(qf), (highquota + 1) * sizeof(struct dqblk));
	fclose(qf);
	close(fi);
	return (0);
}

void	acct(ip)
	register struct dinode *ip;
{
	register struct fileusage *fup;

	if (ip == NULL)
		return;
	if (ip->di_eftflag != EFT_MAGIC) {
		ip->di_mode = ip->di_smode;
		ip->di_uid = ip->di_suid;
	}
	if (ip->di_mode == 0)
		return;
	fup = adduid((u_short)ip->di_uid);
	fup->fu_curfiles++;
	if ((ip->di_mode & IFMT) == IFCHR || (ip->di_mode & IFMT) == IFBLK)
		return;
	fup->fu_curblocks += ip->di_blocks;
}

struct dinode *
ginode()
{
	register unsigned long iblk;

	if (dp == NULL || ++dp >= &itab[ITABSZ]) {
		iblk = itod(&sblock, ino);
		bread((u_long)fsbtodb(&sblock, iblk),
		    (char *)itab, sizeof itab);
		dp = &itab[(int)ino % (int)INOPB(&sblock)];
	}
	if (ino++ < SFSROOTINO)
		return(NULL);
	return(dp);
}

void bread(bno, buf, cnt)
	long unsigned bno;
	char *buf;
{
	extern off_t lseek();
	register off_t pos;

	pos = (off_t)dbtob(bno);
	if (lseek(fi, pos, 0) != pos) {
		(void) fprintf(stderr, "%s: lseek failed: %s\n", 
			myname, strerror(errno));
		exit(31+1);
	}

	(void) lseek(fi, (long)dbtob(bno), 0);
	if (read(fi, buf, cnt) != cnt) {
		(void) fprintf(stderr, "%s: read failed: %s\n", 
			myname, strerror(errno));
		exit(31+1);
	}
}

struct fileusage *
lookup(uid)
	register u_short uid;
{
	register struct fileusage *fup;

	for (fup = fuhead[uid % FUHASH]; fup != 0; fup = fup->fu_next)
		if (fup->fu_uid == uid)
			return (fup);
	return ((struct fileusage *)0);
}

struct fileusage *
adduid(uid)
	register u_short uid;
{
	struct fileusage *fup, **fhp;

	fup = lookup(uid);
	if (fup != 0)
		return (fup);
	fup = (struct fileusage *)calloc(1, sizeof(struct fileusage));
	if (fup == 0) {
		(void) fprintf(stderr, 
			"%s: out of memory for fileusage structures\n",
			myname);
		exit(31+1);
	}
	fhp = &fuhead[uid % FUHASH];
	fup->fu_next = *fhp;
	*fhp = fup;
	fup->fu_uid = uid;
	if ((int)uid > (int)highuid)
		highuid = uid;
	return (fup);
}

char *
makerawname(name)
	char *name;
{
	register char *cp;
	char tmp, ch;
	static char rawname[MAXPATHLEN];
	struct stat statb;

	cp = (char *)strrchr(name, '/');
	if (cp == NULL)
		return (name);
	/* for device naming convention /dev/dsk/c1d0s2 */
	(void) sprintf(rawname, "/dev/rdsk/%s", cp + 1);
	if (stat(rawname, &statb) == 0)
		return(rawname);

	/* for device naming convention /dev/save */

	strcpy(rawname, name);
	cp = (char *)strrchr(rawname, '/');
	cp++;
	for (ch = 'r'; *cp != '\0'; ) {
		tmp = *cp;
		*cp++ = ch;
		ch = tmp;
	}
	*cp++ = ch;
	*cp = '\0';
	return (rawname);
}

void	usage ()
{
	(void) fprintf(stderr, "ufs usage:\n");
	(void) fprintf(stderr, "quotacheck [-v] [-p] -a\n");
	(void) fprintf(stderr, "quotacheck [-v] [-p] filesys ...\n");
	exit(31+1);
}

int
quotactl(cmd, mountp, uid, addr)
	int		cmd;
	char		*mountp;
	int		uid;
	caddr_t		addr;
{
	int		save_errno;
	int 		fd;
	int 		status;
	struct quotctl	quota;
	char		mountpoint[256];
	FILE		*fstab;
	struct mnttab	mntp;


	if ((mountp == NULL) && (cmd == Q_ALLSYNC)) {
		/*
		 * Find the mount point of any ufs file system.   This is
		 * because the ioctl that implements the quotactl call has
		 * to go to a real file, and not to the block device.
		 */
		if ((fstab = fopen(MNTTAB, "r")) == NULL) {
			(void) fprintf(stderr, "%s: cannot open %s: %s\n",
					myname, MNTTAB, strerror(errno));
			exit (31+1);
		}
		fd = (-1);
		while ((status = getmntent(fstab, &mntp)) == NULL) {
			if (strcmp(mntp.mnt_fstype, MNTTYPE_UFS) != 0 ||
				hasmntopt(&mntp, MNTOPT_RO))
				continue;
			(void) sprintf(mountpoint, "%s/%s", 
					mntp.mnt_mountp, QFNAME);
			if ((fd = open(mountpoint, O_RDWR)) >= 0)
				break;
		}
		fclose(fstab);
		if (fd < 0) {
			/* No quotas file found. This is not an error. */
			errno = ENOENT;
			return(-1);
		}
	} else {
		if (mountp == NULL || mountp[0] == '\0') {
			errno =  ENOENT;
			return (-1);
		}
		(void) sprintf(mountpoint, "%s/%s", mountp, QFNAME);
		if ((fd = open (mountpoint, O_RDWR)) < 0) {
			(void) fprintf(stderr, 
					"%s: quotactl: cannot open %s: %s\n",
					myname, mountpoint, strerror(errno));
			exit (31+1);
		}
	}	/* else */

	quota.op = cmd;
	quota.uid = uid;
	quota.addr = addr;
	status = ioctl (fd, Q_QUOTACTL, &quota);
	save_errno = errno;
	close (fd);
	errno = save_errno;
	return (status);
}

char *
hasmntopt(mnt, opt)
        register struct mnttab *mnt;
        register char *opt;
{
        char *f, *opts;
        static char *tmpopts;
	char	*mntopt();

        if (tmpopts == 0) {
                tmpopts = (char *)calloc(256, sizeof (char));
                if (tmpopts == 0)
                        return (0);
        }
        strcpy(tmpopts, mnt->mnt_mntopts);
        opts = tmpopts;
        f = mntopt(&opts);
        for (; *f; f = mntopt(&opts)) {
                if (strncmp(opt, f, strlen(opt)) == 0)
                        return (f - tmpopts + mnt->mnt_mntopts);
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
        strcpy(tmpopts, vfs->vfs_mntopts);
        opts = tmpopts;
        f = mntopt(&opts);
        for (; *f; f = mntopt(&opts)) {
                if (strncmp(opt, f, strlen(opt)) == 0)
                        return (f - tmpopts + vfs->vfs_mntopts);
        }
        return (NULL);
}
