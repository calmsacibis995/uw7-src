/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

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

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/mount/mount.c	1.15.11.6"

/*
 * mount
 */
#include <ctype.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mntent.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/mnttab.h>
#include <sys/mount.h>		/* exit code definitions */
#include <sys/wait.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/vfstab.h>
#include <locale.h>
#include <ctype.h>
#include <pfmt.h>
#include <unistd.h>


int	ro = 0;
int	quota = 0;
int	fake = 0;
int	verbose = 0;
int	nomtab = 0;
int	nosuid = 0;

char	*basename;
char temp[] = "/etc/mnttab.tmp";


extern int	optind;
extern char	*optarg;
extern time_t	time();

#define TIME_MAX	16
#define	NAME_MAX	64
#define	FSTYPE		"ufs"

#define MNTTYPE_INVALID	"invalid"	/* invalid mount type */

char	fstype[]=FSTYPE;

extern int errno;

static void	replace_opts();
static void	printmtab();
void	dump_vfsent();
char	*hasvfsopt();
void	rmopt();

char * ufs_hasmntopt(struct mnttab *, char *);

static char	mountp[MNTMAXSTR];
static char	name[MNTMAXSTR];
static char	type[MNTMAXSTR];
static char	opts[MNTMAXSTR];
static char	tbuf[TIME_MAX];

/*
 * Structure used to build a mount tree.  The tree is traversed to do
 * the mounts and catch dependencies.
 */
struct mnttree {
	struct mnttab *mt_mnt;
	struct mnttree *mt_sib;
	struct mnttree *mt_kid;
};
static struct mnttree *maketree();


main(argc, argv)
	int argc;
	char *argv[];
{
	struct mnttab mnt;
	struct mnttab mntp;
	struct vfstab vfsbuf;
	FILE *mnttab, *vfstab;
	int	c;
	char	label[NAME_MAX];

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmount");
	basename = strrchr(argv[0],'/');
	basename = (basename != 0)? basename+1: argv[0];
	sprintf(label,"UX:%s %s",fstype,basename);
	(void)setlabel(label);
	if (argc == 1) {
		mnttab = fopen(MNTTAB, "r");
		while (getmntent(mnttab, &mntp) == NULL) {
			dump_mntent (&mntp);
			if (strcmp(mntp.mnt_fstype, MNTTYPE_UFS) == 0)
				printent(&mntp);
		}
		(void) fclose(mnttab);
		exit(RET_OK);
	}

	opts[0] = '\0';
	(void) strcpy(type, MNTTYPE_UFS);

	/*
	 * Set options
	 */
	while ((c = getopt (argc, argv, "o:prV")) != EOF) {
		switch (c) {

		case 'o':
			(void) strcpy(opts, optarg);
			break;

		case 'p':
			if (argc != 2) {
				usage();
			}
			printmtab(stdout);
			exit(RET_OK);

		case 'r':
			ro++;
			break;

		case 'V':		/* Print command line */
			{
				char		*opt_text;
				int		opt_count;

				(void) fprintf (stdout, "mount -F ufs ");
				for (opt_count = 1; opt_count < argc ; opt_count++) {
					opt_text = argv[opt_count];
					if (opt_text)
						(void) fprintf (stdout, " %s ", opt_text);
				}
				(void) fprintf (stdout, "\n");
			}
			verbose++;
			break;

		case '?':
			usage();
		}
	}

	if (argc == optind)
		usage();

	/*
	 * Command looks like: mount <dev>|<dir>
	 * we walk through /etc/vfstab til we match either fsname or dir.
	 */
	if (optind == (argc - 1))  {
		struct stat vfstab_stat;
		long vfstab_size;
		int count;

		if (verbose)
			(void)pfmt(stdout, MM_ERROR,
				":26:Look for '%s' in '%s'\n",
					argv[optind], VFSTAB);
		vfstab = fopen(VFSTAB, "r");
		if (vfstab == NULL) {
			pfmt(stderr, MM_NOGET|MM_ERROR,
				"%s: %s\n", VFSTAB, strerror(errno));
			exit(RET_VFS_OPEN);
		}
		if (fstat(fileno(vfstab), &vfstab_stat) == -1) {
			pfmt(stderr, MM_NOGET|MM_ERROR,
				"%s: %s\n", VFSTAB, strerror(errno));
			exit(RET_VFS_STAT);
		}
		vfstab_size = vfstab_stat.st_size;

		for (count = 1;; count++) {
			if (getvfsent(vfstab, &vfsbuf) != NULL) {
				if (ftell(vfstab) >= vfstab_size)
					break;		/* it's EOF */
				(void)pfmt(stderr,  MM_ERROR,
					":27:%s: illegal entry on line %d\n",
		 				VFSTAB, count);
				continue;			   
		        }
			dump_vfsent (&vfsbuf);
		   	if (vfsbuf.vfs_fstype && strcmp(vfsbuf.vfs_fstype, MNTTYPE_UFS) != 0)
				continue;

			mntp.mnt_special = vfsbuf.vfs_special;
			mntp.mnt_mountp = vfsbuf.vfs_mountp;
			mntp.mnt_fstype = vfsbuf.vfs_fstype;
			mntp.mnt_mntopts = vfsbuf.vfs_mntopts;
			sprintf(tbuf, "%ld", time(0L));
			mntp.mnt_time = tbuf;
			if ((strcmp(mntp.mnt_special, argv[optind]) == 0) ||
			    (strcmp(mntp.mnt_mountp, argv[optind]) == 0) ) {
				if (opts[0] != '\0') {
					/*
					 * "-o" specified; override vfstab with
					 * command line options, unless it's
					 * "-o remount", in which case do
					 * nothing if the vfstab says R/O (you
					 * can't remount from R/W to R/O, and
					 * remounting from R/O to R/O is not
					 * only invalid but pointless).
					 */
					if (strcmp(opts, MNTOPT_REMOUNT) == 0
					  && hasvfsopt(&vfsbuf, MNTOPT_RO))
						exit(RET_OK);
					mntp.mnt_mntopts = opts;
				}
				/* 
				 * Make "rq" for quotas in vfstab look 
				 * like an "rw" in mnttab.
				 * If mounting read only, both "rq" 
				 * and "rw" should become "ro" 
				 */
				if (ufs_hasmntopt(&mntp, MNTOPT_RO))
					ro++;
				rmopt(&mntp, MNTOPT_RQ); 
				replace_opts(mntp.mnt_mntopts, ro,
					     MNTOPT_RO, MNTOPT_RW);
/*				if (hasvfsopt(&vfsbuf, MNTOPT_QUOTA)) {
					fprintf(stderr, "quota %d\n", quota);
					quota++;
				}
				replace_opts(mntp.mnt_mntopts, 1,
					    MNTOPT_NOQUOTA, MNTOPT_QUOTA);
*/
				mounttree(maketree((struct mnttree *)NULL,
				    &mntp));
				exit(RET_OK);
			}
		}
		(void)pfmt(stderr, MM_ERROR,
			":28:%s not found in %s\n", argv[optind], VFSTAB);
		exit(RET_VFS_NOENT);
	}

	strcpy(mountp, argv[optind+1]);

	(void) strcpy(name, argv[optind]);

	mnt.mnt_special = name;
	mnt.mnt_mountp = mountp;
	mnt.mnt_fstype = type;
	mnt.mnt_mntopts = opts;
	if (ufs_hasmntopt(&mnt, "f"))
		fake++;
	if (ufs_hasmntopt(&mnt, "n"))
		nomtab++;
	if (fake && nomtab) {
		(void)pfmt(stderr, MM_ERROR,
			":30:cannot specify both -o f and -o n\n");
		exit(RET_O_OPTION);
	}
	if (ufs_hasmntopt(&mnt, MNTOPT_NOSUID))
		nosuid++;
	replace_opts(opts, nosuid, MNTOPT_NOSUID, "suid");
	/* Make "rq" for quotas in vfstab look like an "rw" in mnttab */
	/* If mounting read only, both "rq" and "rw" should become "ro" */
/*
	if (eatmntopt(&mnt, MNTOPT_RO))
		ro++;
	rmopt(&mnt, MNTOPT_RQ); 
*/
	if (ro)
		replace_opts(opts, ro, MNTOPT_RO, MNTOPT_RW);
/*	replace_opts(opts, quota, MNTOPT_QUOTA, MNTOPT_NOQUOTA); */
	replace_opts(opts, 1, MNTOPT_NOQUOTA, MNTOPT_QUOTA);

	sprintf(tbuf, "%ld", time(0L));
	mnt.mnt_time = tbuf;
	mounttree(maketree((struct mnttree *)NULL, &mnt));
	exit(RET_OK);
}

/*
 * attempt to mount file system, return errno or 0
 */
int
mountfs(print, mnt, opts)
	int print;
	struct mnttab *mnt;
	char *opts;
{
	extern int errno;
	int flags = 0;

	strcpy(mountp, mnt->mnt_mountp);
	if (ufs_hasmntopt(mnt, MNTOPT_REMOUNT) == 0) {
		if (mounted(mnt)) {
			(void)pfmt(stderr, MM_ERROR,
			    ":57:%s already mounted\n", mnt->mnt_special);
			return(RET_EBUSY);
		}
	} else
		if (print && verbose)
			(void)pfmt(stderr, MM_ERROR,
			    ":32:remount ignoring mnttab\n");

	if (strcmp(mnt->mnt_fstype, MNTTYPE_UFS) != 0) {
		(void)pfmt(stderr, MM_ERROR,
			":33:unknown filesystem type: %s\n", mnt->mnt_fstype);
		return(RET_ENOENT);
	}

	flags |= eatmntopt(mnt, MNTOPT_RO) ? MS_RDONLY : 0;
	flags |= eatmntopt(mnt, MNTOPT_NOSUID) ? MS_NOSUID : 0;
	flags |= eatmntopt(mnt, MNTOPT_REMOUNT) ? MS_REMOUNT : 0;
	flags |= eatmntopt(mnt, MNTOPT_SOFT) ? MS_SOFTMNT : 0;

	flags |= eatmntopt(mnt, MNTOPT_RO) ? MS_RDONLY : 0;
	/* quota is not an option passed to mount */
/*	(void) eatmntopt(mnt, MNTOPT_QUOTA); */

	if (verbose)
		(void)pfmt(stderr, MM_ERROR,
		":34:mount(mnt_special %s, mnt_mountp %s, flags 0x%x, mnt_fstype %s)\n",
			mnt->mnt_special, mnt->mnt_mountp, flags | MS_FSS, fstype);
	if (fake)
		goto itworked;

	if (mount(mnt->mnt_special, mountp, flags | MS_FSS, "ufs") < 0) {
		return(rpterr(print, mnt->mnt_special, mnt->mnt_mountp));
	}

	if (nomtab)
		return(0);

itworked:
	if (flags & MS_REMOUNT) {
		rmfrommtab(mnt);
		replace_opts(mnt->mnt_mntopts, 1, MNTOPT_RW, MNTOPT_RO);
	}
	fixopts(mnt, opts);
	if (*opts) {
		(void)pfmt(stderr, MM_WARNING,
		    ":58:%s on %s unknown options %s\n",
			mnt->mnt_special, mnt->mnt_mountp, opts);
	}
	addtomtab(mnt);
	return (0);
}

rpterr(print, bs, mp)
	int print;
        register char *bs, *mp;
{
        switch (errno) {
        case EPERM:
		if (print)
                  pfmt(stderr, MM_ERROR, ":20:permission denied\n");
		return(RET_EPERM);		
        case EINVAL:
		if (print)
                  pfmt(stderr, MM_ERROR,
			":59:%s is not an %s file system.\n", bs, fstype);
		return(RET_EINVAL);
        case ENXIO:
		if (print)
                  pfmt(stderr, MM_ERROR, ":5:%s no such device\n", bs);
		return(RET_ENXIO);
        case ENOENT:
		if (print)
                  pfmt(stderr, MM_ERROR,
			":60:%s no such file or directory\n",bs);
		return(RET_ENOENT);
        case ENOTBLK:
		if (print)
                  pfmt(stderr, MM_ERROR,
			":10:%s not a block device\n", bs);
		return(RET_ENOTBLK);
        case EROFS:
		if (print)
                  pfmt(stderr, MM_ERROR,
			":11:%s write-protected\n", bs);
		return(RET_EROFS);
        case ENOSPC:
		if (print)
                  pfmt(stderr, MM_ERROR,
			":23:%s is corrupted. Needs checking\n", bs);
		return(RET_ENOSPC);
        case ENODEV:
		if (print)
                  pfmt(stderr, MM_ERROR,
			":13:%s no such device or write-protected\n", bs);
		return(RET_ENODEV);
	case ENOLOAD:
		if (print)
		  pfmt(stderr, MM_ERROR,
			":14:%s file system module cannot be loaded\n", fstype);
		return(RET_ENOLOAD);
	case EBUSY:
		if (print)
		  pfmt(stderr, MM_ERROR,
			":61:%s is already mounted or %s is busy\n", bs, mp);
		return(RET_EBUSY);
        default:
		if (print)
		  pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s: %s\n", mp, strerror(errno));
		return(RET_MISC);
        }
}

printent(mnt)
	struct mnttab *mnt;
{
	char	fmt[100];

	if (ufs_hasmntopt (mnt, MNTOPT_RW) != 0)
		strcpy(fmt, (char *)gettxt(":36","read/write"));
	else
		if (ufs_hasmntopt (mnt, MNTOPT_RO) != 0)
			strcpy(fmt, (char *)gettxt(":37","read-only"));
		else
			strcpy(fmt, (char *)gettxt(":38","unknown"));
	/*
	 * Build the rest of the options list.
	 */
	if (ufs_hasmntopt (mnt, MNTOPT_QUOTA) != 0)
		strcat (fmt, (char *)gettxt(":62",",quota"));
	if (ufs_hasmntopt (mnt, MNTOPT_NOQUOTA) != 0)
		strcat (fmt, (char *)gettxt(":63",",noquota"));
	if (ufs_hasmntopt (mnt, MNTOPT_NOSUID) != 0)
		strcat (fmt, (char *)gettxt(":39",",nosuid"));
/*	if (ufs_hasmntopt (mnt, MNTOPT_GRPID) != 0)
		strcat (fmt, ",grpid"); */
	if (ufs_hasmntopt (mnt, MNTOPT_REMOUNT) != 0)
		strcat (fmt, (char *)gettxt(":40",",remount"));

	(void)pfmt(stdout, MM_NOSTD, ":41:%s on %s %s %s\n",
	    mnt->mnt_mountp, mnt->mnt_special, mnt->mnt_fstype, fmt);
}

static void
printmtab(outp)
	FILE *outp;
{
	FILE *mnttab;
	struct mnttab mntp;
	int maxfsname = 0;
	int maxdir = 0;
	int maxtype = 0;
	int maxopts = 0;

	/*
	 * first go through and find the max width of each field
	 */
	if (verbose)
		(void)pfmt(stdout, MM_NOSTD, ":42:Pretty print:\n");
	mnttab = fopen(MNTTAB, "r");
	while (getmntent(mnttab, &mntp) == NULL) {
		dump_mntent (&mntp);
		if ((int)strlen(mntp.mnt_special) > maxfsname) {
			maxfsname = strlen(mntp.mnt_special);
		}
		if ((int)strlen(mntp.mnt_mountp) > maxdir) {
			maxdir = strlen(mntp.mnt_mountp);
		}
		if ((int)strlen(mntp.mnt_fstype) > maxtype) {
			maxtype = strlen(mntp.mnt_fstype);
		}
		if ((int)strlen(mntp.mnt_mntopts) > maxopts) {
			maxopts = strlen(mntp.mnt_mntopts);
		}
	}
	(void) fclose(mnttab);
 
	/*
	 * now print them out in pretty format
	 */
	mnttab = fopen(MNTTAB, "r");
	while (getmntent(mnttab, &mntp) == NULL) {
		if (strcmp(mntp.mnt_fstype, MNTTYPE_UFS) == 0) {
			(void) fprintf(outp, "%-*s", maxfsname+1, mntp.mnt_special);
			(void) fprintf(outp, "%-*s", maxdir+1, mntp.mnt_mountp);
			(void) fprintf(outp, "%-*s", maxtype+1, mntp.mnt_fstype);
			(void) fprintf(outp, "%-*s", maxopts+1, mntp.mnt_mntopts);
			printf("\n");
		}
	}
	(void) fclose(mnttab);
}

/*
 * Check to see if mntck is already mounted.
 * We have to be careful because getmntent modifies its static struct.
 */
mounted(mntck)
	struct mnttab *mntck;
{
	int found = 0;
	struct mnttab mnt, mntsave;
	FILE *mnttab;

	if (nomtab) {
		return (0);
	}
	mnttab = fopen(MNTTAB, "r");
	if (mnttab == NULL) {
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s: %s\n", MNTTAB, strerror(errno));
		exit(RET_MNT_OPEN);
	}
	mntcp(mntck, &mntsave);
	if (verbose)
		(void)pfmt(stdout, MM_NOSTD,
			":43:Check to see if '%s' is already mounted:\n",
				mntsave.mnt_special);
	while (getmntent(mnttab, &mnt) == NULL) {
		dump_mntent (&mnt);
		if (strcmp(mnt.mnt_fstype, MNTTYPE_IGNORE) == 0) {
			continue;
		}
		if ((strcmp(mntsave.mnt_special, mnt.mnt_special) == 0) &&
		    (strcmp(mntsave.mnt_mountp, mnt.mnt_mountp) == 0) &&
		    (strcmp(mntsave.mnt_mntopts, mnt.mnt_mntopts) == 0) ) {
			found = 1;
			break;
		}
	}
	(void) fclose(mnttab);
	*mntck = mntsave;
	return (found);
}

mntcp(mnt1, mnt2)
	struct mnttab *mnt1, *mnt2;
{
	static char fsname[128], dir[128], type[128], opts[128], time[128];

	mnt2->mnt_special = fsname;
	(void) strcpy(fsname, mnt1->mnt_special);
	if (mountp != 0)
		mnt2->mnt_mountp = mountp;
	else{
		(void) strcpy(mnt2->mnt_mountp, mnt1->mnt_mountp);
	}
	(void) strcpy(dir, mnt1->mnt_mountp);
	mnt2->mnt_fstype = type;
	(void) strcpy(type, mnt1->mnt_fstype);
	mnt2->mnt_mntopts = opts;
	(void) strcpy(opts, mnt1->mnt_mntopts);
	mnt2->mnt_time = time;
	(void) strcpy(time, mnt1->mnt_time);
}

/*
 * same as ufs_hasmntopt but remove the option from the option string and return
 * true or false
 */
eatmntopt(mnt, opt)
	struct mnttab *mnt;
	char *opt;
{
	int has;

	has = (ufs_hasmntopt(mnt, opt) != NULL);
	rmopt(mnt, opt);
	return (has);
}

/*
 * remove an option string from the option list
 */
void
rmopt(mnt, opt)
	struct mnttab *mnt;
	char *opt;
{
	char *str;
	char *optstart;

	if (optstart = ufs_hasmntopt(mnt, opt)) {
		if (*(optstart + strlen(opt)) != ','
			&& *(optstart + strlen(opt)) != '\0')
			return;
		for (str = optstart; *str != ',' && *str != '\0'; str++)
			;
		if (*str == ',') {
			str++;
		} else if (optstart != mnt->mnt_mntopts) {
			optstart--;
		}
		while (*optstart++ = *str++)
			;
	}
}

/*
 * mnt->mnt_ops has un-eaten opts, opts is the original opts list.
 * Set mnt->mnt_opts to the original list minus the un-eaten opts.
 * Set "opts" to the un-eaten opts minus the "default" options ("rw",
 * "hard", "noquota", "noauto") and "bg".  If there are any options left after
 * this, they are uneaten because they are unknown; our caller will print a
 * warning message.
 */
fixopts(mnt, opts)
	struct mnttab *mnt;
	char *opts;
{
	char *comma;
	char *ue;
	char uneaten[1024];

 	rmopt(mnt, MNTOPT_RW);
	rmopt(mnt, MNTOPT_NOQUOTA);
/*	rmopt(mnt, MNTOPT_NOAUTO);*/
	rmopt(mnt, "suid");
	rmopt(mnt, "f");
	(void) strcpy(uneaten, mnt->mnt_mntopts);
	(void) strcpy(mnt->mnt_mntopts, opts);
	rmopt(mnt, "f");
	(void) strcpy(opts, uneaten);

	for (ue = uneaten; *ue; ) {
		for (comma = ue; *comma != '\0' && *comma != ','; comma++)
			;
		if (*comma == ',') {
			*comma = '\0';
			rmopt(mnt, ue);
			ue = comma+1;
		} else {
			rmopt(mnt, ue);
			ue = comma;
		}
	}
	if (*mnt->mnt_mntopts == '\0') {
		(void) strcpy(mnt->mnt_mntopts, MNTOPT_RW);
	}
}

/*
 * update /etc/mnttab
 */
addtomtab(mnt)
	struct mnttab *mnt;
{
	FILE *mnted;

	if (verbose)
		(void)pfmt(stdout, MM_NOSTD,
			":44:Update the '%s' file\n", MNTTAB);
	mnted = fopen(MNTTAB, "a+");
	if (mnted == NULL) {
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s %s\n", MNTTAB, strerror(errno));
		exit(RET_MNT_OPEN);
	}

	/*
         * Lock the file to prevent many updates at once.
         * This may sleep for the lock to be freed.
         * This is done to ensure integrity of the mnttab.
         */
        if (lockf(fileno(mnted), F_LOCK, 0L) < 0) {
                pfmt(stderr, MM_ERROR,
                        ":3:cannot lock mnttab\n");
                pfmt(stderr, MM_NOGET|MM_ERROR,
                        "%s\n", strerror(errno));
                exit(RET_MNT_LOCK);
        }

	putmntent(mnted, mnt);
	(void) fclose(mnted);

	if (verbose) {
		(void)pfmt(stdout, MM_NOSTD,
			":45:%s mounted on %s\n", mnt->mnt_special, mnt->mnt_mountp);
	}
}

/* 
 * Remove one entry from mnttab
 */
rmfrommtab(mntp)
struct mnttab *mntp;
{
	FILE *mtab, *fwp;
	int ret;
	struct mnttab omnt;

	mtab = fopen(MNTTAB, "r+");
	if (mtab == NULL) {
		pfmt(stderr, MM_ERROR, ":46:Cannot open %s\n", MNTTAB);
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s %s\n", MNTTAB, strerror(errno));
		exit(RET_MNT_OPEN);
	}
	/* check every entry for validity before we change mnttab */
	while ((ret = getmntent(mtab, &omnt)) == 0)
		;
	if (ret > 0)
		mnterror(ret);
	rewind(mtab);

	if ((fwp = fopen(temp, "w")) == NULL) {
		pfmt(stderr, MM_ERROR,
			":18:cannot open %s for writing\n", temp);
		exit(RET_TMP_OPEN);
	}

	/*
         * Lock the file to prevent many updates at once.
         * This may sleep for the lock to be freed.
         * This is done to ensure integrity of the mnttab.
         */
        if (lockf(fileno(mtab), F_LOCK, 0L) < 0) {
                pfmt(stderr, MM_ERROR,
                        ":3:cannot lock mnttab\n");
                pfmt(stderr, MM_NOGET|MM_ERROR,
                        "%s\n", strerror(errno));
                exit(RET_MNT_LOCK);
        }

	/*
	 * Loop through mtab writing mount record to temp mtab.
	 * If a file system gets turn on or off modify the mount
	 * record before writing it.
	 */
	/* Make writes happen immediately, so will see any error */
	setbuf(fwp, NULL);
	while ((ret = getmntent(mtab, &omnt)) != -1) {
		if (ret == 0 &&
		    	strcmp(omnt.mnt_special,mntp->mnt_special) != 0 &&
		     	strcmp(omnt.mnt_mountp, mntp->mnt_mountp) != 0)
			if (putmntent(fwp, &omnt) < 0) {
				pfmt(stderr,  MM_ERROR,
					":19:cannot write to %s\n", temp);
				exit(RET_TMP_WRITE);
			}
	}
	fclose(fwp);
	rename(temp, MNTTAB);
	fclose(mtab);
}


char *
xmalloc(size)
	int size;
{
	char *ret;
	
	if ((ret = (char *)malloc((unsigned)size)) == NULL) {
		(void)pfmt(stderr, MM_ERROR,
			":48:ran out of memory!\n");
		exit(RET_MALLOC);
	}
	return (ret);
}

struct mnttab *
mntdup(mnt)
	struct mnttab *mnt;
{
	struct mnttab *new;

	new = (struct mnttab *)xmalloc(sizeof(*new));

	new->mnt_special = (char *)xmalloc(strlen(mnt->mnt_special) + 1);
	(void) strcpy(new->mnt_special, mnt->mnt_special);

	new->mnt_mountp = (char *)xmalloc(strlen(mnt->mnt_mountp) + 1);
	(void) strcpy(new->mnt_mountp, mnt->mnt_mountp);

	new->mnt_fstype = (char *)xmalloc(strlen(mnt->mnt_fstype) + 1);
	(void) strcpy(new->mnt_fstype, mnt->mnt_fstype);

	new->mnt_mntopts = (char *)xmalloc(strlen(mnt->mnt_mntopts) + 1);
	(void) strcpy(new->mnt_mntopts, mnt->mnt_mntopts);

	new->mnt_time = (char *)xmalloc(strlen(mnt->mnt_time) + 1);
	(void) strcpy(new->mnt_time, mnt->mnt_time);

	return (new);
}

/*
 * Build the mount dependency tree
 */
static struct mnttree *
maketree(mt, mnt)
	struct mnttree *mt;
	struct mnttab *mnt;
{

	if (mt == NULL) {
		mt = (struct mnttree *)xmalloc(sizeof (struct mnttree));
		mt->mt_mnt = mntdup(mnt);
		mt->mt_sib = NULL;
		mt->mt_kid = NULL;
	} else {
		if (substr(mt->mt_mnt->mnt_mountp, mnt->mnt_mountp)) {
			mt->mt_kid = maketree(mt->mt_kid, mnt);
		} else {
			mt->mt_sib = maketree(mt->mt_sib, mnt);
		}
	}
	return (mt);
}

printtree(mt)
	struct mnttree *mt;
{
	if (mt) {
		printtree(mt->mt_sib);
		(void) printf("   %s\n", mt->mt_mnt->mnt_mountp);
		printtree(mt->mt_kid);
	}
}

mounttree(mt)
	struct mnttree *mt;
{
	int error;

	if (mt) {
		char opts[1024];

		mounttree(mt->mt_sib);
		(void) strcpy(opts, mt->mt_mnt->mnt_mntopts);
		error = mountfs(1, mt->mt_mnt, opts);
		if (!error) {
			mounttree(mt->mt_kid);
		} else {
			(void)pfmt(stderr, MM_ERROR,
				":49:giving up on:");
			(void) fprintf(stderr, "   %s\n", mt->mt_mnt->mnt_mountp);
			printtree(mt->mt_kid);
			exit(error);
		}
	}
}

/*
 * Returns true if s1 is a pathname substring of s2.
 */
substr(s1, s2)
	char *s1;
	char *s2;
{
	while (*s1 == *s2) {
		s1++;
		s2++;
	}
	if (*s1 == '\0' && *s2 == '/') {
		return (1);
	}
	return (0);
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
ufs_hasmntopt(mnt, opt)
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
        strcpy(tmpopts, mnt->mnt_mntopts);
        opts = tmpopts;
        f = mntopt(&opts);
        for (; *f; f = mntopt(&opts)) {
                if (strcmp(opt, f) == 0)
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
        strcpy(tmpopts, vfs->vfs_mntopts);
        opts = tmpopts;
        f = mntopt(&opts);
        for (; *f; f = mntopt(&opts)) {
                if (strncmp(opt, f, strlen(opt)) == 0)
                        return (f - tmpopts + vfs->vfs_mntopts);
        }
        return (NULL);
}

usage()
{
	(void)pfmt(stdout, MM_ACTION,
	    ":50:Usage: %s [-F %s] [generic options] [-o f,n] {special | mount_point}\n",
		basename, fstype);
	(void)pfmt(stdout, MM_ACTION,
	    ":51:Usage: %s [-F %s] [generic options] [-o ro,rw,nosuid,remount,soft] special mount_point\n",
		basename, fstype);
	exit(RET_USAGE);
}

/*
 * Returns the next option in the option string.
 */
static char *
getnextopt(p)
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

/*
 * "trueopt" and "falseopt" are two settings of a Boolean option.
 * If "flag" is true, forcibly set the option to the "true" setting; otherwise,
 * if the option isn't present, set it to the false setting.
 */
static void
replace_opts(options, flag, trueopt, falseopt)
	char *options;
	int flag;
	char *trueopt;
	char *falseopt;
{
	register char *f;
	char tmptopts[MNTMAXSTR];
	char *tmpoptsp;
	register int found;

	if (options == NULL)
		return;
	strcpy(tmptopts, options);
	tmpoptsp = tmptopts;
	(void) strcpy(options, "");
	found = 0;
	for (f = getnextopt(&tmpoptsp); *f; f = getnextopt(&tmpoptsp)) {
		if (options[0] != '\0')
			(void) strcat(options, ",");
		if (strcmp(f, trueopt) == 0) {
			(void) strcat(options, f);
			found++;
		} else if (strcmp(f, falseopt) == 0) {
			if (flag)
				(void) strcat(options, trueopt);
			else
				(void) strcat(options, f);
			found++;
		} else
			(void) strcat(options, f);
        }
	if (!found) {
		if (options[0] != '\0')
			(void) strcat(options, ",");
		(void) strcat(options, flag ? trueopt : falseopt);
	}
}

dump_mntent (mt)
	struct mnttab	*mt;
{
	if (verbose)
		(void)pfmt(stderr, MM_ERROR,
		":52:\nmntt_special %s mnt_mountp %s mnt_fstype %s mnt_mntopts %s\n",
		    mt->mnt_special, mt->mnt_mountp, mt->mnt_fstype,
			mt->mnt_mntopts);
}

void
dump_vfsent (vfs)
	struct vfstab	*vfs;
{
	if (verbose)
		(void)pfmt(stderr, MM_ERROR,
		":53:\nvfs_special %s vfs_mountp %s vfs_fstype %s vfs_mntopts %s\n",
		    vfs->vfs_special, vfs->vfs_mountp, vfs->vfs_fstype,
			vfs->vfs_mntopts);
}

mnterror(flag)
	int	flag;
{
	switch (flag) {
	case MNT_TOOLONG:
		pfmt(stderr, MM_ERROR,
			":54:line in mnttab exceeds %d characters\n",
				MNT_LINE_MAX-2);
		exit(RET_MNT_TOOLONG);
		break;
	case MNT_TOOFEW:
		pfmt(stderr, MM_ERROR,
			":55:line in mnttab has too few entries\n");
		exit(RET_MNT_TOOFEW);
		break;
	case MNT_TOOMANY:
		pfmt(stderr, MM_ERROR,
			":56:line in mnttab has too many entries\n");
		exit(RET_MNT_TOOMANY);
		break;
	}
}
