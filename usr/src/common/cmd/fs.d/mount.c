/*		copyright	"%c%" 	*/

#ident	"@(#)fs.cmds:common/cmd/fs.d/mount.c	1.82.3.10"

/***************************************************************************
 * Command: sbin/mount$
 * Inheritable Privileges: P_MOUNT,P_DACWRITE,P_DACREAD,P_MACWRITE,
 *			   P_MACREAD,P_SETFLEVEL,P_OWNER
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<string.h>
#include 	<limits.h>
#include 	<fcntl.h>
#include 	<unistd.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/statvfs.h>
#include	<sys/errno.h>
#include	<sys/mnttab.h>
#include	<sys/vfstab.h>
#include	<locale.h>
#include	<ctype.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<sys/param.h>
/* new includes added for security */
#include	<mac.h>
#include	<priv.h>

#define	VFS_PATH	"/usr/lib/fs"
#define ALT_PATH	"/etc/fs"
#define	REMOTE		"/etc/dfs/fstypes"
#define SEM_FILE	"/etc/.mnt.lock"

#define	ARGV_MAX	16
#define	TIME_MAX	50
#define	LINE_MAX	64
#define	FSTYPE_MAX	16
#define	REMOTE_MAX	64

#define	OLD	0
#define	NEW	1

#define	READONLY	0
#define	READWRITE	1
#define SUID 		2
#define NOSUID		3

/* Exit codes */
#define	RET_OK		0	/* success */
#define	RET_USAGE	1	/* usage error */
#define	RET_OPTIONS	2	/* invalid combinations of options */
#define	RET_ARGS	4	/* invalid arguments for options */

#define	RET_MAC_INSTALL	3	/* mac not installed */

#define	RET_FSTYPE_MAX	5	/* FSType exceeds MAX characters */
#define	RET_FSTYPE_ONE	6	/* more than one FSType specified */
#define	RET_FSTYPE_UNK	32	/* FSType is unknown */

#define	RET_VFS_OPEN	7	/* cannot open vfstab */
#define	RET_VFS_GETENT	8	/* getvfsany() error */

#define	RET_MNT_OPEN    9	/* cannot open mnttab */
#define	RET_MNT_LOCK	10	/* cannot lock mnttab */
#define	RET_MNT_GETENT	11	/* getmntent() error */

#define	RET_MP_STAT	12	/* cannot stat mount point */
#define	RET_MP_UNK	13	/* mount point cannot be determined */
#define	RET_MP_ABSPATH	14	/* mount point is not an absolute pathname */
#define	RET_MP_EXIST	15	/* mount point does not exist */
#define	RET_MP_UNKLVL	16	/* cannot determine mount point's level */
#define	RET_MP_RANGE	17	/* mount point level not within device rang */

#define	RET_DEV_UNK	18	/* special device cannot be determined */

#define	RET_DDB_NOENT	19	/* device not found in device database (ddb) */
#define	RET_DDB_INVAL	20	/* security attr. for device is invalid in ddb */
#define	RET_DDB_ACCESS	21	/* cannot access ddb */

#define	RET_LVL_INVAL	22	/* invalid security level specified */
#define	RET_CEILING	23	/* error in validating level ceiling */
#define	RET_LVL_RANGE	24	/* level ceiling not within device range */
#define	RET_LTDB_ACCESS	25	/* no access to LTDB */
#define	RET_LVL_PRINT	26	/* error while printing level info */

#define	RET_MALLOC_ERR	27	/* malloc() error */
#define	RET_WAIT_ERR	28	/* wait() error */
#define	RET_FORK_ERR	29	/* fork() error */
#define	RET_EXEC_ERR	30	/* exec() error - not applicable */
#define	RET_EXEC_ACCESS	31	/* exec() error - cannot execute */


/*
 * Format does not include newline because an extra field may be 
 * printed when security is installed.
 */
#define	FORMAT	 gettxt(":64","%a %b %e %H:%M:%S %Y")
				/* date time format */
				/* a - abbreviated weekday name */
				/* b - abbreviated month name */
				/* e - day of month */
				/* H - hour */
				/* M - minute */
				/* S - second */
				/* Y - Year */

extern int	errno;
extern int	optind;
extern char	*optarg;

#ifdef __STDC__
extern void	usage(void);
extern char	*flags(char *, int);
extern char	*remote(char *, FILE *);
#else
extern void	usage();
extern char	*flags();
extern char	*remote();
#endif

static void	valid_ceiling();	/* security routine */
static void	dev_validate();		/* security routine */

char	*myopts[] = {
	"ro",
	"rw",
	"suid",
	"nosuid",
	NULL
};
char	*myname;		/* point to argv[0] */
char	mntflags[100];
int	mac_install;	/* defines whether security MAC is installed */

/*
 * Procedure:     main
 *
 * Restrictions: devalloc:  None
                 setlocale: None
                 getopt:    None
                 lvlproc(2):None
                 fprintf:   None
                 printf:    None
                 fopen:     P_MACREAD
                 getvfsany: None
                 rewind:    None
                 fclose:    None
                 stat(2):   None
                 perror:    None
		 lvlvfs:    None
 * Notes:
 *
 * This is /usr/sbin/mount: the generic command that in turn
 * execs the appropriate /usr/lib/fs/{fstype}/mount.
 * The -F flag and argument are NOT passed.
 * If the usr file system is not mounted a duplicate copy
 * can be found in /sbin and this version execs the 
 * appropriate /etc/fs/{fstype}/mount
 *
 * If the -F fstype, special or directory are missing,
 * /etc/vfstab is searched to fill in the missing arguments.
 *
 * -V will print the built command on the stdout.
 * It isn't passed either.
 */
main(argc, argv)
	int	argc;
	char	*argv[];
{
	char	*special,		/* argument of special/resource */
		*mountp,		/* argument of mount directory */
		*fstype,		/* wherein the fstype name is filled */
		*newargv[ARGV_MAX],	/* arg list for specific command */
		*vfstab = VFSTAB,
		*mnttab = MNTTAB,
		*farg = NULL, *Farg = NULL, *narg = NULL, *oarg = NULL, *larg = NULL;
	int	cflg, dflg, fflg, Fflg, nflg, oflg, pflg;
	int	rflg, vflg, Vflg, dashflg, questflg;
	int	lflg, zflg, Zflg; 
	int 	vfsflag = 0;
	int	ii, ij, ret, cc, err_flag = 0;
	struct stat	stbuf;
	struct stat	specbuf;
	struct vfstab	vget, vref;
	mode_t mode;
	struct dev_alloca	devddb;	/* security dev allocation attributes */
	FILE	*fd;
	extern int devalloc();
	int pid, retval, wstat, chexitval;
	char *vfsceiling = NULL;	/*  MAC ceiling stored in the vfstab */
	level_t hilid;	/* LID of MAC security ceiling */
	level_t level;
	char	label[NAME_MAX];
	char	*file_name;
	char	resolved[MAXPATHLEN];


	(void)setlocale(LC_ALL, "");
	(void)setcat("uxmount");
	myname = strrchr(argv[0], '/');
	myname = (myname != 0)? myname+1: argv[0];
	sprintf(label, "UX:%s", myname);
	(void)setlabel(label);

	/* Process the args.  */
	cflg = dflg = fflg = Fflg = lflg = nflg = oflg =
	pflg = rflg = vflg = Vflg = zflg= Zflg = dashflg = questflg = 0;
	while ((cc = getopt(argc, argv, "?cdf:F:l:n:o:prvVzZ")) != -1)
		switch (cc) {
			case 'c':
				cflg++;
				break;
			case 'd':
				dflg++;
				break;
			case 'f':
				fflg++;
				farg = optarg;
				break;
			case 'F':
				Fflg++;
				Farg = optarg;
				break;
			case 'l':
				lflg++;
				larg = optarg;
				break;
			case 'n':
				nflg++;
				narg = optarg;
				break; /* undocumented flag for rfs */
			case 'o':
				oflg++;
				oarg = optarg;
				break; /* fstype dependent options */
			case 'p':
				pflg++;
				break;
			case 'r':
				rflg++;
				break;
			case 'v':
				vflg++;
				break;
			case 'V':
				Vflg++;
				break;
			case 'z':
				zflg++;
				break;
			case 'Z':
				Zflg++;
				break;
			case '?':
				questflg++;
				break;
		}
	/* establish upfront if the enhanced security package is installed */
	if ((lvlproc(MAC_GET, &level) == -1) && (errno == ENOPKG))
		mac_install = 0;	
	else
		mac_install = 1;


	/* check for '-r' at end, for compatibility */
	if (strcmp(argv[argc-1], "-r") == 0) {
		rflg++;
		/* decrement so we don't consider as mount_point */
		argv[--argc] = NULL;
	}

	/* copy '--' to specific */
	if (strcmp(argv[optind-1], "--") == 0)
		dashflg++;

	/* option checking */
	/* more than two args not allowed */
	if (argc - optind > 2) {
		usage();
		exit(RET_USAGE);
	}

	if (oflg != 0 && oflg != 1) {
		pfmt(stderr, MM_ERROR,
			":65:more than one 'o' flag was specified\n");
		usage();
		exit(RET_OPTIONS);
	}

	/* pv mutually exclusive */
	if (pflg && vflg) {
		pfmt(stderr, MM_ERROR,
			":66:invalid combination of options -p & -v\n");
		usage();
		exit(RET_OPTIONS);
	}

	/* z,Z or -l flags used only when enhanced security is installed */
	if ((zflg || lflg || Zflg) && (!mac_install)) {
		pfmt(stderr, MM_ERROR,
			":67:invalid option\n");
		pfmt(stderr, MM_ERROR,
			":68:system service not installed\n");
		usage();
		exit(RET_MAC_INSTALL);
	}

	/* zZ mutually exclusive */
	if (zflg && Zflg) {
		pfmt(stderr, MM_ERROR,
			":69:invalid combination of options -z & -Z\n");
		usage();
		exit(RET_OPTIONS);
	}

	/* dfF mutually exclusive */
	if (dflg + fflg + Fflg > 1) {
		pfmt(stderr, MM_ERROR,
			":70:more than one FSType specified\n");
		usage();
		exit(RET_FSTYPE_ONE);
	}

	/* no arguments, only allow p,v,V,z,Z or [F]? */
	if (optind == argc) {
		if (cflg || dflg || fflg || lflg || nflg || oflg || rflg ) {
			usage();
			exit(RET_USAGE);
		}
		if (Fflg && !questflg) {
			usage();
			exit(RET_USAGE);
		}

		if (questflg) {
			if (Fflg) {
				newargv[2] = "-?";
                                newargv[3] = NULL;
				doexec(Farg, newargv);
			}
			usage();
			exit(RET_USAGE);
		}
	}

	if (questflg) {
		usage();
		exit(RET_USAGE);
	}

	/* one or two args, allow any but p,v */
	if (optind != argc) {
		if (pflg || vflg) {
			pfmt(stderr, MM_ERROR,
				":71:cannot use -p and -v with arguments\n");
			usage();
			exit(RET_ARGS);
		}
		else if (zflg || Zflg) {

			pfmt(stderr, MM_ERROR,
				":72:cannot use -z or -Z with arguments\n");
			usage();
			exit(RET_ARGS);
		}
	}
	/* if only reporting mnttab, generic prints mnttab and exits */
	if (optind == argc) {
		if (Vflg) {
			printf("%s", myname);
			if (pflg)
				printf(" -p");
			if (vflg)
				printf(" -v");
			if (zflg)
				printf(" -z");
			if (Zflg)
				printf(" -Z");
			printf("\n");
			exit(RET_OK);
		}

		exit(print_mnttab(vflg, pflg, zflg, Zflg));
		/* NOTREACHED */
	}
	/* get special and/or mount-point from arg(s) */
	special = argv[optind++];
	if (optind < argc) {
		mountp = argv[optind++];
	 } else
		mountp = NULL;

	/*	call realpath() to fully resolve the mount point arg.
		if mountp is non NULL, then resolve it, else
		only was arg was passed which may be a mount point
		and not a special, so we'll resolve it.
	*/
	if ( mountp ) {
		file_name=mountp;
		if ( realpath(mountp,resolved) )
			mountp = resolved;
	}
	else {
		file_name=special;
		if ( realpath(special,resolved) )
			special = resolved;
	}

	/* get fstype if one given */
	if (dflg)
		fstype = "rfs";
	else if (fflg) { 
		if ((strcmp(farg,"S51K")!=0) && (strcmp(farg, "S52K")!=0)) {
			fstype = farg;
		}
		else
			fstype = "s5";
	}
	else 	/* (Fflg) */
		fstype = Farg;
	if (fstype) {
		if (oflg && (strcmp(oarg, "remount") == 0)) {
			struct mnttab	mref, mget;

			procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);

			if ((fd = fopen(mnttab, "r")) == NULL) {
				pfmt(stderr, MM_ERROR,
					":9:cannot open mnttab\n");
				exit(RET_MNT_OPEN);
			}
			mntnull(&mref);
			mref.mnt_special = special;
			mref.mnt_mountp = mountp;
			mref.mnt_fstype = fstype;
			procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
			/* get a vfstab entry matching mountp or special */
			ret = getmntany(fd, &mget, &mref);

			if (ret == -1) {
				pfmt(stderr, MM_ERROR,
				":120:cannot remount %s file system which is not mounted or from different type\n", fstype); 
				fclose(fd);
				exit(1);
			}
			mountp=mref.mnt_mountp;
		}
	}
	/* lookup only if we need to */
	if (fstype == NULL || oarg == NULL || special == NULL || mountp == NULL 
	    || (mac_install && (larg == NULL))) {
	/*
	 * When the root file system is checked in /sbin/bcheckrc
	 * the MAC level of the process hasn't been initialized, so
	 * we need the MACREAD privilege to read VFSTAB,MNTTAB and 
	 * /etc/passwd files. 
	 *
	 * If the MAC level of the processes is initialized, the 
	 * process should be able to read these files without the 
	 * MACREAD privilege. In this case the MACREAD privilege
	 * is restricted.
	 */

		procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);

		if ((fd = fopen(vfstab, "r")) == NULL) {
			if (fstype == NULL || special == NULL || mountp == NULL) {
				pfmt(stderr, MM_ERROR,
					":73:cannot open vfstab\n");
				exit(RET_VFS_OPEN);
			}
		}
		vfsnull(&vref);
		vref.vfs_special = special;
		vref.vfs_mountp = mountp;
		vref.vfs_fstype = fstype;

		procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
		/* get a vfstab entry matching mountp or special */
		ret = getvfsany(fd, &vget, &vref);

		/* if no entry and there was only one argument */
		/* then the argument could be the mount point */
		/* and not special as we thought earlier */
		if (ret == -1 && mountp == NULL) {
			rewind(fd);
			mountp = vref.vfs_mountp = special;
			special = vref.vfs_special = NULL;
			ret = getvfsany(fd, &vget, &vref);
		}
		/* mount point from the command line and /etc/vfstab
		 * are both symbolic link file, use mountp/file_name to search
		 * for the matching vfs entry.
		 */
		if (special == NULL) {
			rewind(fd);
			special = vref.vfs_special = NULL;
			ret = getvfsfile(fd, &vget, mountp);
			if (ret == -1 ) {
				rewind(fd);
				special = vref.vfs_special = NULL;
				ret = getvfsfile(fd, &vget, file_name);
				if (ret == 0)
					special = vget.vfs_special;
			} else
				special = vget.vfs_special;
		}
			
			
		fclose(fd);

		if (ret > 0)
			vfserror(ret);

		if (ret == 0) {
			if (fstype == NULL)
				fstype = vget.vfs_fstype;
			if (special == NULL)
				special = vget.vfs_special;
			/* mount by devices */  
			if (mountp == NULL) {
				mountp = vget.vfs_mountp;
				if (lstat(mountp, &stbuf) == -1) {
					pfmt(stderr, MM_ERROR,
						":74:cannot stat mount point %s\n", mountp);
					exit(RET_MP_STAT);
				}
				/* mount point is /etc/vfstab is symlink */
				if ((stbuf.st_mode & S_IFMT) == S_IFLNK) {
					int llen;
					llen = readlink(mountp, file_name, 512);
					mountp = file_name;
					mountp[llen] = '\0';
				}
				
			}
			if (oflg == 0 && vget.vfs_mntopts) {
				oflg++;
				oarg = vget.vfs_mntopts;
			}
			/* get MAC level ceiling stored in vfstab */
			if (mac_install)
				vfsceiling = vget.vfs_macceiling;
		} else if (special == NULL) {
			if ((ij =stat(mountp, &stbuf)) == -1) {
				pfmt(stderr, MM_ERROR,
					":74:cannot stat mount point %s\n", mountp);
				exit(RET_MP_STAT);
			}
			if (((mode = (stbuf.st_mode & S_IFMT)) == S_IFBLK)||
				(mode == S_IFCHR )) {
				pfmt(stderr, MM_ERROR,
					":75:mount point cannot be determined\n");
				exit(RET_MP_UNK);
			} else
				{
				pfmt(stderr, MM_ERROR,
					":76:special cannot be determined\n");
				exit(RET_DEV_UNK);
			}

		} else if (fstype == NULL) {
			pfmt(stderr, MM_ERROR,
				":121:File system type cannot be determined\n");
			exit(RET_FSTYPE_UNK);
                }
	}

	if (strlen(fstype) > FSTYPE_MAX) {
		pfmt(stderr, MM_ERROR,
			":77:FSType %s exceeds %d characters\n",
				fstype, FSTYPE_MAX);
		exit(RET_FSTYPE_MAX);
	}
	if (*mountp == NULL) {
		pfmt(stderr, MM_ERROR,
			":75:mount point cannot be determined\n");
		exit(RET_MP_UNK);
	}
	if (*mountp != '/') {
		pfmt(stderr, MM_ERROR,
			":78:mount-point %s is not an absolute pathname.\n",
				mountp);
		exit(RET_MP_ABSPATH);
	}

	if (stat(mountp, &stbuf) < 0) {
		if (errno == ENOENT || errno == ENOTDIR)
			pfmt(stderr, MM_ERROR,
				":79:mount-point does not exist.\n");
		else {
			pfmt(stderr, MM_ERROR,
				":80:cannot stat mount-point.\n");
			pfmt(stderr, MM_ERROR|MM_NOGET,
				"%s\n", strerror(errno));
		}
		exit(RET_MP_STAT);
	}

	/* create the new arg list, and end the list with a null pointer */
	ii = 2;
	if (cflg)
		newargv[ii++] = "-c";
	if (nflg) {
		newargv[ii++] = "-n";
		newargv[ii++] = narg;
	}
	if (oflg) {
		newargv[ii++] = "-o";
		newargv[ii++] = oarg;
	}
	if (rflg)
		newargv[ii++] = "-r";
	if (dashflg)
		newargv[ii++] = "--";
	newargv[ii++] = special;
	newargv[ii++] = mountp;
	newargv[ii] = NULL;
	if (Vflg) {
		printf("%s -F %s", myname, fstype);
		for (ii = 2; newargv[ii]; ii++)
			printf(" %s", newargv[ii]);
		printf("\n");
		exit(RET_OK);
	}


	/* 
	 * the parent will fork a child to exec the mount dependent section 
	 * and will do a lvlvfs if enhanced security package is installed 
	 */

	/* assume special is not null */
	if (mac_install) {
		if (lflg)
			valid_ceiling(larg, &hilid);
		else if (vfsceiling != NULL) {
			valid_ceiling(vfsceiling, &hilid);
			vfsflag++;
		}
	    	if ((stat(special, &specbuf) == 0) &&
	            ((mode = (specbuf.st_mode & S_IFMT) == S_IFBLK)||
	             (mode == S_IFCHR ))) {
			if (devalloc(special, DEV_GET, &devddb) < 0) {  
				switch(errno) {
				case ENODEV:
					pfmt(stderr, MM_ERROR,
						":81:%s Device not found in Device Database\n", special);
					exit(RET_DDB_NOENT);
					break;
				case EINVAL:
					pfmt(stderr, MM_ERROR,
						":82:security attributes for %s missing or invalid in Device Database\n", special);
					exit(RET_DDB_INVAL);
					break;
				default:
					pfmt(stderr, MM_ERROR,
						":83:Device Database is inaccessible\n");
					exit(RET_DDB_ACCESS);
					break;
				}
			}
			dev_validate(mountp,&devddb);	
			/* 
		 	 * need to validate the new ceiling entered or the one stored
		 	 * in the vfstab
		 	 */
			if ((lflg || vfsflag) &&
			    (lvldom(&(devddb.hilevel), &hilid) <=0 )) {
				pfmt(stderr, MM_ERROR,
					":84:level ceiling not within device range\n");
				exit(RET_LVL_RANGE);
			}
		}
	}
	if ((pid = fork()) == -1) {
		pfmt(stderr, MM_ERROR, ":85:cannot fork\n");
		exit(RET_FORK_ERR);
	}
	else if (pid == 0) {
		doexec(fstype, newargv); /* child will exec fs dependent mount */
	}
	else {
	/* parent   -- wait for completion of child */
		retval = wait(&wstat);
		if ((retval == -1) || (retval != pid)) 
			exit(RET_WAIT_ERR);
		/* analyze exit code */
		if (wstat & 0xff !=0)
			exit(RET_WAIT_ERR);
		/* make sure child had an exit value of 0 */
		chexitval = ((wstat >> 8) & 0x00ff);
		if (mac_install && (!chexitval)) {
			/*
			 * set new level ceiling for mounted file system
			 * on failure, print warning and exit successfully
			 */
			if ((lflg || vfsflag) && 
			    (lvlvfs(mountp, MAC_SET, &hilid) == -1)) {
				pfmt(stderr, MM_WARNING,
					":86:could not set new ceiling on mounted file system\n");
			}
		}
		else exit(chexitval); 
	}
	exit(RET_OK);
}

/*
 * Procedure:     usage
 *
 * Restrictions:
                 fprintf: None
*/

void
usage()
{
	if (mac_install)
		pfmt(stderr, MM_ACTION,
		":87:Usage:\n%s [-v | -p]\n%s [-l level] [-F FSType] [-V] [current_options] [-o specific_options] {special | mount_point}\n%s [-l level] [-F FSType] [-V] [current_options] [-o specific_options] special mount_point\n%s [-z | -Z]\n",
		myname, myname, myname, myname);
	else
		pfmt(stderr, MM_ACTION,
		   ":88:Usage:\n%s [-v | -p]\n%s [-F FSType] [-V] [current_options] [-o specific_options] {special | mount_point}\n%s [-F FSType] [-V] [current_options] [-o specific_options] special mount_point\n",
		myname, myname, myname);
}

/*
 * Procedure:     print_mnttab
 *
 * Restrictions:
                 fopen:     P_MACREAD
                 fprintf:   None
                 getmntent: None
                 cftime:    None
                 printf:    None
 *	 
 * this function  prints the mounted file system according to the mnttab file 
 * if the z option is invoked, the MAC level ceiling of the mounted file system is
 * printed, if there is an error on one entry, a warning is issued, and the next 
 * entry is printed.
 */

int
print_mnttab(vflg, pflg, zflg, Zflg)
	int	vflg, pflg, zflg, Zflg;
{
	FILE	*fd;
	FILE	*rfp;			/* this will be NULL if fopen fails */
	int	ret;
	char	time_buf[TIME_MAX];	/* array to hold date and time */
	char	*mnttab = MNTTAB;
	struct mnttab	mget;
	time_t	ltime;
	int	errflag = RET_OK;

	procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);

	if ((fd = fopen(mnttab, "r")) == NULL) {
		pfmt(stderr, MM_ERROR,
			":2:cannot open mnttab\n");
		return(RET_MNT_OPEN);
	}
	rfp = fopen(REMOTE, "r");

	procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);

	while ((ret = getmntent(fd, &mget)) == 0)
		if (mget.mnt_special && mget.mnt_mountp && mget.mnt_fstype && mget.mnt_time) {
			ltime = atol(mget.mnt_time);
			cftime(time_buf, FORMAT, &ltime);
			if (pflg)
				pfmt(stdout, MM_NOSTD,
					":89:%s - %s %s - no %s",
					mget.mnt_special,
					mget.mnt_mountp,
					mget.mnt_fstype,
					mget.mnt_mntopts);
			else if (vflg) {
				pfmt(stdout, MM_NOSTD,
					":90:%s on %s type %s %s%s on %s",
					mget.mnt_special,
					mget.mnt_mountp,
					mget.mnt_fstype,
					flags(mget.mnt_mntopts, NEW),
					remote(mget.mnt_fstype, rfp),
					time_buf);
			} else
				pfmt(stdout, MM_NOSTD,
					":91:%s on %s %s%s on %s",
					mget.mnt_mountp,
					mget.mnt_special,
					flags(mget.mnt_mntopts, OLD),
					remote(mget.mnt_fstype, rfp),
					time_buf);
			if (zflg) {
				if (print_level(mget.mnt_mountp, LVL_ALIAS))
					errflag = RET_LVL_PRINT;
			}
			else if (Zflg) {
				if (print_level(mget.mnt_mountp, LVL_FULL))
					errflag = RET_LVL_PRINT;
			}
			printf("\n");
		}

	if (ret > 0)
		mnterror(ret);

	return(errflag);
}


char	*
flags(mntopts, flag)
	char	*mntopts;
	int	flag;
{
	char	*value;

	strcpy(mntflags, "");
	if (mntopts)
		while (*mntopts != '\0')  {
			switch (getsubopt(&mntopts, myopts, &value)) {
			case READONLY:
				if (flag == OLD)
				    strcat(mntflags,
					gettxt(":92","read only"));
				else
				    strcat(mntflags,
					gettxt(":37","read-only"));
				break;
			case READWRITE:
				strcat(mntflags,
					gettxt(":36","read/write"));
				break;
			case SUID:
				strcat(mntflags,
					gettxt(":93","setuid"));
				break;
			case NOSUID:
				strcat(mntflags,
					gettxt(":94","nosuid"));
				break;
			default:
				strcat(mntflags, value);
				break;
			}
	/* if mntopts still exist, then cat '/' separator to mntflags */
			if (*mntopts != '\0') {
				strcat(mntflags, "/");
			}
		}
	if (strlen(mntflags) > 0)
		return mntflags;
	else
		return	gettxt(":36","read/write");
}

/*
 * Procedure:     remote
 *
 * Restrictions:
                 rewind: None
                 fgets: None
*/
char	*
remote(fstype, rfp)
	char	*fstype;
	FILE	*rfp;
{
	char	buf[BUFSIZ];
	char	*fs;
	extern char *strtok();

	if (rfp == NULL || fstype == NULL || strlen(fstype) > FSTYPE_MAX)
		return	"";	/* not a remote */
	rewind(rfp);
	while (fgets(buf, sizeof(buf), rfp) != NULL) {
		fs = strtok(buf, " \t");
		if (strcmp (fstype,fs) == 0) 
			return	"/remote";	/* is a remote fs */

	}
	return	"";	/* not a remote */
}


/*
 * Procedure:     vfserror
 *
 * Restrictions:
                 fprintf:None
*/

vfserror(flag)
	int	flag;
{
	switch (flag) {
	case VFS_TOOLONG:
		pfmt(stderr, MM_ERROR,
			":95:line in vfstab exceeds %d characters\n",
				VFS_LINE_MAX-1);
		break;
	case VFS_TOOFEW:
		pfmt(stderr, MM_ERROR,
			":96:line in vfstab has too few entries\n");
		break;
	case VFS_TOOMANY:
		pfmt(stderr, MM_ERROR,
			":97:line in vfstab has too many entries\n");
		break;
	}
	exit(RET_VFS_GETENT);

}

/*
 * Procedure:     mnterror
 *
 * Restrictions:
                 fprintf: None
*/

mnterror(flag)
	int	flag;
{
	switch (flag) {
	case MNT_TOOLONG:
		pfmt(stderr, MM_ERROR,
			":54:line in mnttab exceeds %d characters\n",
				MNT_LINE_MAX-2);
		break;
	case MNT_TOOFEW:
		pfmt(stderr, MM_ERROR,
			":55:line in mnttab has too few entries\n");
		break;
	case MNT_TOOMANY:
		pfmt(stderr, MM_ERROR,
			":56:line in mnttab has too many entries\n");
		break;
	}
	exit(RET_MNT_GETENT);
}
/*
 * Procedure:     doexec
 *
 * Restrictions:
                 sprintf: None
                 creat(2): None
                 lockf: None
                 fprintf: None
                 access(2):P_MACREAD
                 execv(2):P_MACREAD
*/

doexec(fstype, newargv)
	char	*fstype, *newargv[];
{
	char	full_path[PATH_MAX];
	char	alter_path[PATH_MAX];
	char	dir_path[PATH_MAX];
	char	*vfs_path = VFS_PATH;
	char	*alt_path = ALT_PATH;
	int	sem_file, nfile;

	/* build the full pathname of the fstype dependent command. */
	sprintf(full_path, "%s/%s/%s", vfs_path, fstype, myname);
	sprintf(alter_path, "%s/%s/%s", alt_path, fstype, myname);

	/* set the new argv[0] to the filename */
	newargv[1] = myname;

	if ((sem_file = creat(SEM_FILE,0600)) == -1 || lockf(sem_file,F_LOCK, 0L) <0 ) {
		if ((errno == EACCES) || (errno == EPERM)) {
			fprintf(stderr, "%s: %s\n", myname,
				strerror(errno));
			/* No seperate exit code because the GUI that
			   uses them doesn't care, and we've run out
			   of distinct exit codes < 32 */
			exit(RET_MNT_LOCK);
		} else {
			pfmt(stderr, MM_ERROR,
				":98:cannot lock temp file <%s>\n", SEM_FILE);
			exit(RET_MNT_LOCK);
		}
	}

	(void)procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);
	/* Try to exec the fstype dependent portion of the mount. */
	/* See if the directory is there before trying to exec dependent */
	/* portion.  This is only useful for eliminating the '..mount: not found' */
	/* message when '/usr' is mounted */
	if (access(full_path,0)==0 ) {
		execv(full_path, &newargv[1]);
		if (errno == EACCES) {
			pfmt(stderr, MM_ERROR,
				":99:cannot execute %s - permission denied\n", full_path);
		}
		if (errno == ENOEXEC) {
			newargv[0] = "sh";
			newargv[1] = full_path;
			execv("/sbin/sh", &newargv[0]);
		}
	}
	execv(alter_path, &newargv[1]);
	if (errno == EACCES) {
		pfmt(stderr, MM_ERROR,
			":99:cannot execute %s - permission denied\n", alter_path);
		exit(RET_EXEC_ACCESS);
	}
	if (errno == ENOEXEC) {
		newargv[0] = "sh";
		newargv[1] = alter_path;
		execv("/sbin/sh", &newargv[0]);
	}
	
	/* If we have reached here, both attempts to exec have failed.
	   This might be due to a bogus file system type,
	   or it might be because the operation is unsupported.
	   Distinguish these cases. */
	
	sprintf(dir_path, "%s/%s", vfs_path, fstype);
	if (access(dir_path, 0) != 0) {
		sprintf(dir_path, "%s/%s", alt_path, fstype);
		if (access(dir_path, 0) != 0) {
			/* looks like a bogus (or uninstalled) 
			   file system type */
			pfmt(stderr, MM_ERROR,
				":100:%s: no such file system type\n",
				fstype);
			exit(RET_EXEC_ERR);
		}
	}
	pfmt(stderr, MM_ERROR,
		":101:operation not applicable to FSType %s\n", fstype);
	exit(RET_EXEC_ERR);
}

/*
 * Procedure:     print_level
 *
 * Restrictions:
                 lvlvfs:  None
                 lvlout:  None
                 fprintf: None
                 printf:  None
 * Notes:
 * this routine is used to print the level ceiling of the mounted file system
 * it does a lvlvfs with command set to MAC_GET to get the mac ceiling of the
 * mounted file system and then translate that LID into a level alias
 * or full text representation depending on the request.
 * if there is a partial failure, an error value is set so the command will
 * exit with a non-zero value.
 * the contents of buf are printed on success
 */
int
print_level(name, lvlformat)
	char *name;
	int lvlformat;

{
	#define INITBUFSIZ	512	/*initial MAX buffer for storing level name */

	/*
	 * Pre-allocate buffer to store level name. Just like in the ls command.
	 * save a lvlout call in most cases.  Double size dynamically if level
	 * wont't fit in buffer.
	 */

	 static char	lvl_buf[INITBUFSIZ];
	 static char	*lvl_name = lvl_buf;
	 static int	lvl_namez = INITBUFSIZ;
	 level_t	tmp_lid;

	if (lvlvfs(name, MAC_GET, &tmp_lid) == 0) {
		while(lvlout(&tmp_lid, lvl_name, lvl_namez, lvlformat) == -1) {
			if ((lvlformat == LVL_FULL) && (errno == ENOSPC)) {
				char *tmp_name;
				if ((tmp_name = malloc(lvl_namez*2)) == (char *)NULL) {
					pfmt(stderr, MM_ERROR,
						":102:no memory to print level ceiling of file system %s\n", name);
					exit(RET_MALLOC_ERR);
				}
			lvl_namez *= 2;
			if (lvl_name !=lvl_buf)
				free(lvl_name);
			lvl_name = tmp_name;
			} else {
				pfmt(stderr, MM_ERROR,
					":103:cannot translate level to text format %s \n", name);
				return(1);
			}
		}
		printf(" %s", lvl_name);
		if (lvl_name != lvl_buf)
			free(lvl_name);
	}
	else {
		pfmt(stderr, MM_ERROR, ":20:permission denied\n");
		return(1);
	}
	return(0);
}

/*
 * Procedure:     valid_ceiling
 *
 * Restrictions:
                 lvlin:   P_MACREAD
                 fprintf: None
 * Notes:
 * 
 * this routine validates the new MAC level ceiling specified to be a valid level
 */
static void
valid_ceiling(ceilingp, levelp)
	char *ceilingp;
	level_t *levelp;

{

	(void)procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);
		

	if (lvlin(ceilingp, levelp) == -1) {
		switch(errno) {
		case EINVAL:	/* invalid security level specified */
			pfmt(stderr, MM_ERROR,
				":104:invalid security level specified\n");
			exit(RET_LVL_INVAL);
			break;
		case EACCES:	/* no access to LTDB */
			pfmt(stderr, MM_ERROR,
				":105:LTDB is inaccessible\n");
			exit(RET_LTDB_ACCESS);
			break;
		default:
			pfmt(stderr, MM_ERROR,
				":106:error in validating level ceiling,  errno =%d\n", errno);
			exit(RET_CEILING);
			break;
		}
	}

	(void)procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
		
}
/*
 * Procedure:     dev_validate
 *
 * Restrictions:
                 lvlfile: None
                 fprintf: None
 * Notes:
 * this routines checks that the mount point is within the device range on which the
 * file system is to be mounted 
 */
static void
dev_validate(mountp, devddbp)
	char *mountp;
	struct dev_alloca *devddbp;

{
	level_t mountplid;


	/* get level of mount point */
	if (lvlfile(mountp, MAC_GET, &mountplid) == -1) {
		pfmt(stderr, MM_ERROR,
			":107:cannot get level of mount point\n");
		exit(RET_MP_UNKLVL);
	}
	/* 
	 * level of mount point must be enclosed by the device range 
	 * level of mount point will be level of root of mounted file system 
	 * and floor of mounted file system 
	 */


	if (!((lvldom(&(devddbp->hilevel), &mountplid) > 0) &&
	      (lvldom(&mountplid, &(devddbp->lolevel)) > 0))) {
		pfmt(stderr, MM_ERROR,
			":108:mount point level not within device range\n");
		exit(RET_MP_RANGE);
	}	

}

