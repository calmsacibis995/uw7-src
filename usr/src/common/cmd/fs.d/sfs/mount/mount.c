/*		copyright	"%c%" 	*/

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

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/mount/mount.c	1.3.8.8"

/***************************************************************************
 * Command: mount
 * Inheritable Privileges: P_MOUNT,P_DACWRITE,P_MACWRITE,
			   P_SETFLEVEL P_MACREAD,P_DACREAD
 *       Fixed Privileges: None
 * Notes: mount file systems and remote resources
 *
 ***************************************************************************/
#include <sys/types.h>
#include <ctype.h>
#include <priv.h>
#include <string.h>
#include <sys/param.h>
#include <sys/mntent.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/mnttab.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/vfstab.h>
#include <mac.h>
#include <locale.h>
#include <ctype.h>
#include <pfmt.h>
#include <errno.h>
#include <unistd.h>


int	verbose = 0;
int	nomtab = 0;
int	fake = 0;
int	nosuid = 0;

static char	*basename;
char temp[] = "/etc/mnttab.tmp";

extern int	optind;
extern char	*optarg;
extern time_t	time();

#define TIME_MAX	16

#define MNTTYPE_INVALID	"invalid"	/* invalid mount type */

#define	NAME_MAX	64
#define	FSTYPE		"sfs"

char	fstype[]=FSTYPE;

extern int errno;

extern char	*realpath();

static void	mntcp();
static void	replace_opts();
static void	printent();
static void	printmtab();
static void	rmopt();
static int	rpterr();
static char	*hasvfsopt();

static char	name[MNTMAXSTR];
static char	mountp[MNTMAXSTR];
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
/*
 * Procedure:     main
 *
 * Restrictions:
                 fopen: 	P_MACREAD
                 getmntent: 	none
                 fclose: 	none
                 getopt: 	none
                 fprintf: 	none
                 printf: 	none
                 getvfsent: 	none
                 sprintf: 	none
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	struct mnttab mnt;
	struct mnttab mntp;
	struct vfstab vfsbuf;
	FILE *mnttab, *vfstab;
	int	c;
	char 	*x;
	int	ro = 0;
	char	label[NAME_MAX];

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmount");
	basename = strrchr(argv[0],'/');
	basename = (basename != 0)? basename+1: argv[0];
	sprintf(label,"UX:%s %s",fstype,basename);
	(void)setlabel(label);

	if (argc == 1) {
        	/* Clear P_MACREAD for fopen() of mnttab */
		(void)procprivl(CLRPRV,MACREAD_W,0);
		mnttab = fopen(MNTTAB, "r");
		(void)procprivl(SETPRV,MACREAD_W,0);
		while (getmntent(mnttab, &mntp) == NULL) {
			dump_mntent (&mntp);
			if (strcmp(mntp.mnt_fstype, MNTTYPE_SFS) == 0)
				printent(&mntp);
		}
		(void) fclose(mnttab);
		exit(RET_OK);
	}

	opts[0] = '\0';
	(void) strcpy(type, MNTTYPE_SFS);

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

				(void) fprintf (stdout, "mount -F sfs ");
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
			(void)pfmt(stdout, MM_NOSTD,
				":26:Look for '%s' in '%s'\n",
					argv[optind], VFSTAB);
		/* Clear P_MACREAD for fopen() of vfstab */
		(void)procprivl(CLRPRV, MACREAD_W, 0);
		vfstab = fopen(VFSTAB, "r");
		(void)procprivl(SETPRV, MACREAD_W, 0);
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
				(void)pfmt(stderr, MM_ERROR,
				    ":27:%s: illegal entry on line %d\n",
				    	VFSTAB, count);
				continue;			   
		        }
			dump_vfsent (&vfsbuf);
		   	if (vfsbuf.vfs_fstype && strcmp(vfsbuf.vfs_fstype, MNTTYPE_SFS) != 0)
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
					  && hasvfsopt(&mntp, MNTOPT_RO))
						exit(RET_OK);
					mntp.mnt_mntopts = opts;
				}
				replace_opts(mntp.mnt_mntopts, ro, MNTOPT_RO,
					     MNTOPT_RW);
				mounttree(maketree((struct mnttree *)NULL,
						   &mntp));
				exit(RET_OK);
			}
		}
		(void)pfmt(stderr, MM_ERROR,
			":28:%s not found in %s\n", argv[optind], VFSTAB);
		exit(RET_VFS_NOENT);
	}

	(void) strcpy(mountp, argv[optind + 1]);
	(void) strcpy(name, argv[optind]);

	mnt.mnt_special = name;
	mnt.mnt_mountp = mountp;
	mnt.mnt_fstype = type;
	mnt.mnt_mntopts = opts;
	if (hasmntopt(&mnt, "f"))
                fake++;
	if (hasmntopt(&mnt, "n"))
		nomtab++;
        if (fake && nomtab) {
                (void)pfmt(stderr, MM_ERROR,
                        ":30:cannot specify both -o f and -o n\n");
                exit(1);
        }

	if (hasmntopt(&mnt, MNTOPT_NOSUID))
		nosuid++;
	replace_opts(opts, nosuid, MNTOPT_NOSUID, "suid");
	replace_opts(opts, ro, MNTOPT_RO, MNTOPT_RW);

	sprintf(tbuf, "%ld", time(0L));
	mnt.mnt_time = tbuf;
	mounttree(maketree((struct mnttree *)NULL, &mnt));
	exit(RET_OK);
}

/*
 * Procedure:     mountfs
 *
 * Restrictions:
                 fprintf: 	none
                 mount(2): 	none

 * Notes: attempt to mount file system, 
 *	  return non-zero exit code or 0
 */
int
mountfs( mnt, opts)
	struct mnttab *mnt;
	char *opts;
{
	extern int errno;
	int flags = 0;

	strcpy(mountp, mnt->mnt_mountp);
	if (hasmntopt(mnt, MNTOPT_REMOUNT) == 0) {
		if (mounted(mnt)) {
			(void)pfmt(stderr, MM_ERROR,
				       ":31:%s already mounted, or %s busy\n",
				       mnt->mnt_special, mnt->mnt_mountp);
			return(RET_EBUSY);
		}
	} else
		if (verbose)
			(void)pfmt(stderr, MM_ERROR,
			    ":32:remount ignoring mnttab\n");

	if (strcmp(mnt->mnt_fstype, MNTTYPE_SFS) != 0) {
		(void)pfmt(stderr, MM_ERROR,
			    ":33:unknown filesystem type: %s\n",
			    	mnt->mnt_fstype);
		return(RET_ENOENT);
	}

	flags |= eatmntopt(mnt, MNTOPT_RO) ? MS_RDONLY : 0;
	flags |= eatmntopt(mnt, MNTOPT_NOSUID) ? MS_NOSUID : 0;
	flags |= eatmntopt(mnt, MNTOPT_REMOUNT) ? MS_REMOUNT : 0;
	flags |= eatmntopt(mnt, MNTOPT_SOFT) ? MS_SOFTMNT : 0;

	if (verbose)
		(void)pfmt(stderr, MM_ERROR,	
			":34:mount(mnt_special %s, mnt_mountp %s, flags 0x%x, mnt_fstype %s)\n",
				mnt->mnt_special, mnt->mnt_mountp, flags | MS_FSS, "sfs");
	if (fake)
                goto itworked;
	
	if (mount(mnt->mnt_special, mountp, flags | MS_FSS, "sfs") < 0) {
		return(rpterr(mnt->mnt_special,mnt->mnt_mountp));
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
		(void)pfmt(stderr, MM_ERROR,
			       ":35:%s on %s: unknown options %s\n",
			       mnt->mnt_special, mnt->mnt_mountp, opts);
		usage();
	}
	addtomtab(mnt);
	return (0);
}

/*
 * Procedure:     printent
 *
 * Restrictions:
                 fprintf: 	none
 */
static void
printent(mnt)
	struct mnttab *mnt;
{
	char	fmt[100];

	if (hasmntopt (mnt, MNTOPT_RW) != 0)
		strcpy(fmt, (char *)gettxt(":36","read/write"));
	else
		if (hasmntopt (mnt, MNTOPT_RO) != 0)
			strcpy(fmt, (char *)gettxt(":37","read-only"));
		else
			strcpy(fmt, (char *)gettxt(":38","unknown"));
	/*
	 * Build the rest of the options list.
	 */
	if (hasmntopt (mnt, MNTOPT_NOSUID) != 0)
		strcat (fmt, (char *)gettxt(":39",",nosuid"));
	if (hasmntopt (mnt, MNTOPT_REMOUNT) != 0)
		strcat (fmt, (char *)gettxt(":40",",remount"));

	(void)pfmt(stdout, MM_NOSTD, ":41:%s on %s %s %s\n",
	    mnt->mnt_mountp, mnt->mnt_special, mnt->mnt_fstype, fmt);
}

/*
 * Procedure:     printmtab
 *
 * Restrictions:
                 printf: 	none
                 fopen: 	P_MACREAD
                 getmntent: 	none
                 fclose: 	none
                 fprintf: 	none
 */
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
	/* Clear P_MACREAD for fopen() of mnttab */
	(void)procprivl(CLRPRV,MACREAD_W,0);
	mnttab = fopen(MNTTAB, "r");
	(void)procprivl(SETPRV,MACREAD_W,0);
	while (getmntent(mnttab, &mntp) == NULL) {
		dump_mntent (&mntp);
		if (strlen(mntp.mnt_special) > (size_t)maxfsname) {
			maxfsname = strlen(mntp.mnt_special);
		}
		if (strlen(mntp.mnt_mountp) > (size_t)maxdir) {
			maxdir = strlen(mntp.mnt_mountp);
		}
		if (strlen(mntp.mnt_fstype) > (size_t)maxtype) {
			maxtype = strlen(mntp.mnt_fstype);
		}
		if (strlen(mntp.mnt_mntopts) > (size_t)maxopts) {
			maxopts = strlen(mntp.mnt_mntopts);
		}
	}
	(void) fclose(mnttab);
 
	/*
	 * now print them out in pretty format
	 */

	/* Clear P_MACREAD for fopen() of mnttab */
	(void)procprivl(CLRPRV,MACREAD_W,0);
	mnttab = fopen(MNTTAB, "r");
	(void)procprivl(SETPRV,MACREAD_W,0);
	while (getmntent(mnttab, &mntp) == NULL) {
		if (strcmp(mntp.mnt_fstype, MNTTYPE_SFS) == 0) {
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
 * Procedure:     mounted
 *
 * Restrictions:
                 fopen: 	P_MACREAD
                 fprintf: 	none
                 printf: 	none
                 getmntent: 	none
                 fclose: 	none
 * Notes:
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
	/* Clear P_MACREAD for fopen() of mnttab */
	(void)procprivl(CLRPRV,MACREAD_W,0);
	mnttab = fopen(MNTTAB, "r");
	(void)procprivl(SETPRV,MACREAD_W,0);
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

static void
mntcp(mnt1, mnt2)
	struct mnttab *mnt1, *mnt2;
{
	static char fsname[128], dir[128], type[128], opts[128], time[128];

	mnt2->mnt_special = fsname;
	(void) strcpy(fsname, mnt1->mnt_special);
	if (mountp == 0)
		strcpy(mnt2->mnt_mountp, mnt1->mnt_mountp);
	else
		mnt2->mnt_mountp = mountp;
	(void) strcpy(dir, mnt1->mnt_mountp);
	mnt2->mnt_fstype = type;
	(void) strcpy(type, mnt1->mnt_fstype);
	mnt2->mnt_mntopts = opts;
	(void) strcpy(opts, mnt1->mnt_mntopts);
	mnt2->mnt_time = time;
	(void) strcpy(time, mnt1->mnt_time);
}

/*
 * same as hasmntopt but remove the option from the option string and return
 * true or false
 */
eatmntopt(mnt, opt)
	struct mnttab *mnt;
	char *opt;
{
	int has;

	has = (hasmntopt(mnt, opt) != NULL);
	rmopt(mnt, opt);
	return (has);
}

/*
 * remove an option string from the option list
 */
static void
rmopt(mnt, opt)
	struct mnttab *mnt;
	char *opt;
{
	char *str;
	char *optstart;

	if (optstart = hasmntopt(mnt, opt)) {
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
 * and "hard").  If there are any options left after
 * this, they are uneaten because they are unknown; our caller will print an
 * error message.
 */
fixopts(mnt, opts)
	struct mnttab *mnt;
	char *opts;
{
	char *comma;
	char *ue;
	char uneaten[1024];

 	rmopt(mnt, MNTOPT_RW);
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
 * Procedure:     addtomtab
 *
 * Restrictions:
                 printf: 	none
                 fopen: 	none
                 fprintf: 	none
                 fclose: 	none
 *
 * Notes: update /etc/mnttab
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
			"%s: %s\n", MNTTAB, strerror(errno));
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
		(void)pfmt(stdout, MM_NOSTD, ":45:%s mounted on %s\n",
		    mnt->mnt_special, mnt->mnt_mountp);
	}
}

/*
 * Procedure:     rmfrommtab
 *
 * Restrictions:
                 fopen: 	P_MACREAD
                 fprintf: 	none
                 getmntent: 	none
                 rewind: 	none
                 fclose: 	none
                 lvlin: 	P_MACREAD
                 lvlfile(2): 	none
                 rename(2): 	none

 * Notes: Remove one entry from mnttab
 */
rmfrommtab(mntp)
struct mnttab *mntp;
{
	FILE *mtab, *fwp;
	int ret;
	struct mnttab omnt;
	level_t	level;

	/* Clear P_MACREAD for fopen() of mnttab */
	(void)procprivl(CLRPRV,MACREAD_W,0);
	mtab = fopen(MNTTAB, "r+");
	(void)procprivl(SETPRV,MACREAD_W,0);
	if (mtab == NULL) {
		pfmt(stderr, MM_ERROR, ":46:Cannot open %s\n", MNTTAB);
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s: %s\n", MNTTAB, strerror(errno));
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
	/* Make sure we'll see errors as they happen */
	setbuf(fwp, NULL);
	while ((ret = getmntent(mtab, &omnt)) != -1) {
		if (ret == 0 &&
		    	strcmp(omnt.mnt_special,mntp->mnt_special) != 0 &&
		     	strcmp(omnt.mnt_mountp, mntp->mnt_mountp) != 0) {
			if (putmntent(fwp, &omnt) < 0) {
				pfmt(stderr, MM_ERROR,
				    ":19:cannot write to %s\n", temp);
				exit(RET_TMP_WRITE);
			}
		}
	}
	fclose(fwp);

	/* Check to see if MAC is installed */
	if ((lvlproc(MAC_GET, &level) == 0)) {
		/* Clear P_MACREAD for lvlin() */
		(void)procprivl(CLRPRV, MACREAD_W, 0);
		lvlin ("SYS_PUBLIC", &level);
		(void)procprivl(SETPRV, MACREAD_W, 0);
		lvlfile (temp, MAC_SET, &level);
	}

	if (rename(temp, MNTTAB) != 0) {
		pfmt(stderr, MM_ERROR,
			":47:cannot rename %s to %s\n", temp, MNTTAB);
	}
	fclose(mtab);
}


/*
 * Procedure:     xmalloc
 *
 * Restrictions:
                 fprintf: 	none
 */
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

/*
 * Procedure:     printtree
 *
 * Restrictions:
                 printf: 	none
 */
printtree(mt)
	struct mnttree *mt;
{
	if (mt) {
		printtree(mt->mt_sib);
		(void) printf("   %s\n", mt->mt_mnt->mnt_mountp);
		printtree(mt->mt_kid);
	}
}

/*
 * Procedure:     mounttree
 *
 * Restrictions:
                 fprintf: 	none
 */
mounttree(mt)
	struct mnttree *mt;
{
	int error;

	if (mt) {
		char opts[1024];

		mounttree(mt->mt_sib);
		(void) strcpy(opts, mt->mt_mnt->mnt_mntopts);
		error = mountfs(mt->mt_mnt, opts);
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
        strcpy(tmpopts, mnt->mnt_mntopts);
        opts = tmpopts;
        f = mntopt(&opts);
        for (; *f; f = mntopt(&opts)) {
                if (strcmp(opt, f) == 0)
                        return (f - tmpopts + mnt->mnt_mntopts);
        }
        return (NULL);
}

static char *
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

/*
 * Procedure:     usage
 *
 * Restrictions:
                 fprintf: 	none
 */
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


	(void) strcpy(tmptopts, options);
	tmpoptsp = tmptopts;
	(void) strcpy(options, "");

	if (strcmp(tmptopts, "") == 0) {
		return;
	}
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

/*
 * Procedure:     dump_mntent
 *
 * Restrictions:
                 fprintf: 	none
 */
dump_mntent (mt)
	struct mnttab	*mt;
{
	if (verbose)
		(void)pfmt(stderr, MM_ERROR,
		    ":52:\nmntt_special %s mnt_mountp %s mnt_fstype %s mnt_mntopts %s\n",
			mt->mnt_special, mt->mnt_mountp, mt->mnt_fstype,
			mt->mnt_mntopts);
}


/*
 * Procedure:     dump_vfsent
 *
 * Restrictions:
                 fprintf: 	none
 */
dump_vfsent (vfs)
	struct vfstab	*vfs;
{
	if (verbose)
		(void)pfmt(stderr, MM_ERROR,
		    ":53:\nvfs_special %s vfs_mountp %s vfs_fstype %s vfs_mntopts %s\n",
			vfs->vfs_special, vfs->vfs_mountp, vfs->vfs_fstype,
			vfs->vfs_mntopts);
}

/*
 * Procedure:     mnterror
 *
 * Restrictions:
                 fprintf: 	none
 */
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

/*
 * Procedure: rpterr
 * Note: prints appropriate error message based on errno set by
 *	 mount() and returns appropriate exit code.
 */
static int
rpterr(bs, mp)
	register char *bs, *mp;
{
	switch (errno) {
	case EPERM:
		pfmt(stderr, MM_ERROR, ":20:permission denied\n");
		return(RET_EPERM);
	case ENXIO:
		pfmt(stderr, MM_ERROR, ":5:%s no such device\n", bs);
		return(RET_ENXIO);
	case ENOTDIR:
		pfmt(stderr, MM_ERROR,
			":6:%s not a directory\n\tor a component of %s is not a directory\n",
				mp, bs);
		return(RET_ENOTDIR);
	case ENOENT:
		pfmt(stderr, MM_ERROR,
			":7:%s or %s, no such file or directory\n", bs, mp);
		return(RET_ENOENT);
	case EINVAL:
		pfmt(stderr, MM_ERROR,
			":22:%s is not an %s file system,\n\tor %s is busy.\n",
				bs, fstype, mp);
		return(RET_EINVAL);
	case EBUSY:
		pfmt(stderr, MM_ERROR,
			":9:%s is already mounted, %s is busy,\n\tor allowable number of mount points exceeded\n",
				bs, mp);
		return(RET_EBUSY);
	case ENOTBLK:
		pfmt(stderr, MM_ERROR,
			":10:%s not a block device\n", bs);
		return(RET_ENOTBLK);
	case EROFS:
		pfmt(stderr, MM_ERROR,
			":11:%s write-protected\n", bs);
		return(RET_EROFS);
	case ENOSPC:
		pfmt(stderr, MM_ERROR,
			":23:%s is corrupted. Needs checking\n", bs);
		return(RET_ENOSPC);
	case ENODEV:
		pfmt(stderr, MM_ERROR,
			":24:%s no such device or device is write-protected\n", bs);
		return(RET_ENODEV);
	case ENOLOAD:
		pfmt(stderr, MM_ERROR,
			":14:%s file system module cannot be loaded\n",
				fstype);
		return(RET_ENOLOAD);
	default:
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		pfmt(stderr, MM_ERROR,
			":15:cannot mount %s\n", bs);
		return(RET_MISC);
	}
}
