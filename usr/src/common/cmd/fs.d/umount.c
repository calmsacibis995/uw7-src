/*		copyright	"%c%" 	*/

#ident	"@(#)fs.cmds:common/cmd/fs.d/umount.c	1.17.11.3"

/***************************************************************************
 * Command: umount
 * Inheritable Privileges: P_MOUNT,P_SETFLEVEL,P_MACWRITE P_MACREAD,
 *                         P_DACREAD,P_OWNER,P_DACWRITE
 *       Fixed Privileges: None
 * Notes: Unmount Filesystems from mount points.
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<limits.h>
#include	<signal.h>
#include	<unistd.h>
#include	<mac.h>
#include	<sys/types.h>
#include	<priv.h>
#include	<sys/mnttab.h>
#include	<sys/errno.h>
#include	<sys/stat.h>
#include	<stdlib.h>
#include	<sys/param.h>
#include	<string.h>
#include	<locale.h>
#include	<ctype.h>
#include	<pfmt.h>

#define	FS_PATH		"/usr/lib/fs"
#define ALT_PATH	"/etc/fs"
#define SEM_FILE	"/etc/.mnt.lock"
#define MNTTAB_UID	0
#define MNTTAB_GID	3
#define	FULLPATH_MAX	32
#define	FSTYPE_MAX	16	
#define	ARGV_MAX	16

#define	questFLAG	0x01
#define	dFLAG		0x02
#define	oFLAG		0x08
#define	VFLAG		0x10
#define	dashFLAG	0x20

void	usage(), mnterror();
int	rpterr();

extern int	errno;
extern	char	*optarg;	/* used by getopt */
extern	int	optind, opterr;

char	*myname;
char	fs_path[] = FS_PATH;
char	alt_path[] = ALT_PATH;
char	mnttab[] = MNTTAB;
char	temp[] = "/etc/mnttab.temp";

/* Exit Codes */
#define	RET_OK		0	/* success */
#define	RET_USAGE	1	/* usage error */

#define	RET_FSTYPE_MAX	2	/* FSType name exceeds max chars */
#define	RET_NOEXEC	3	/* cannot execute path */

#define	RET_MNT_OPEN	4	/* cannot open mnttab */
#define	RET_MNT_LOCK	5	/* cannot lock mnttab */
#define	RET_MNT_TOOLONG	6	/* line in mnttab exceeds max chars */
#define	RET_MNT_TOOFEW	7	/* line in mnttab has too few entries */
#define	RET_MNT_MISC	8	/* misc errors from getmntent() */

#define	RET_TMP_OPEN	9	/* cannot open temp file */
#define	RET_TMP_WRITE	10	/* cannot write to temp file */

#define	RET_EPERM	11	/* umount() - permission denied */
#define	RET_ENODEV	12	/* umount() - no such device */
#define	RET_ENOENT	13	/* umount() - no such file/dir */
#define	RET_EINVAL	14	/* umount() - not mounted */
#define	RET_EBUSY	15	/* umount() - mount pt busy */
#define	RET_ENOTBLK	16	/* umount() - block device required */
#define	RET_ECOMM	17	/* umount() - broken link detected */
#define	RET_MISC	18	/* umount() - misc error */


/*
 * Procedure:     main
 *
 * Restrictions:
                 getopt:	None
		 fopen:		P_MACREAD  MNTTAB file verification
                 fprintf:	None
                 getmntany:	P_MACREAD  
                 rewind:	None
                 fclose:	None
                 execv(2):	None
                 umount(2):	None
                 creat(2):	None
                 lockf:		None
                 getmntent:	P_MACREAD  MNTTAB file verification
                 chmod(2):	None
                 lvlfile(2):	P_MACREAD  /etc/security/mac verification
                 chown(2):	None
                 rename(2):	None
Note: Validity of file levels are only done once.
*/
main(argc, argv)
	int	argc;
	char	**argv;
{
	FILE	*frp, *fwp;
	int 	ii, cc, ret, tmperr, cmd_flags = 0, err_flag = 0, ein_flag = 0;
	int	no_mnttab = 0;
	int	sem_file = 0;
	int	exitcode;
	char	*oarg = NULL;
	char	full_path[FULLPATH_MAX];
	char	alter_path[FULLPATH_MAX];
	char	resolved[MAXPATHLEN];
	char	mgetsave[MNT_LINE_MAX], *mp;
	char	*newargv[ARGV_MAX];
	struct mnttab	mget;
	struct mnttab	mref;
	level_t		lid = 0;
	char	label[NAME_MAX];

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmount");
	myname = strrchr(argv[0],'/');
	myname = (myname != 0)? myname+1: argv[0]; 
	sprintf(label, "UX:%s", myname);
	(void)setlabel(label);

	/*
	 * Process the args.
	 * Usage:
	 *	umount [-V] [-d] [-o options] {special | mount_point}
	 *
	 * "-d" for compatibility
	 */
	while ((cc = getopt(argc, argv, "o:V?d")) != -1)
		switch (cc) {
		case '?':
			if (cmd_flags & questFLAG)
				err_flag++;
			else {
				cmd_flags |= questFLAG;
				err_flag++;
			}
			break;
		case 'd':
			if (cmd_flags & dFLAG)
				err_flag++;
			else
				cmd_flags |= dFLAG;
			break;
		case 'o':
			if (cmd_flags & (oFLAG))
				err_flag++;
			else {
				cmd_flags |= (oFLAG);
				oarg = optarg;
			}
			break;
		case 'V':
			if (cmd_flags & VFLAG)
				err_flag++;
			else
				cmd_flags |= VFLAG;
			break;
		default:
			err_flag++;
			break;
		}

	/* copy '--' to specific */
	if (strcmp(argv[optind-1], "--") == 0)
		cmd_flags |= dashFLAG;

	/* option checking */
		/* more than one arg not allowed */
	if (argc - optind > 1 ||
		/* no arguments, only allow ? */
	    (optind == argc && (cc = (cmd_flags & questFLAG)) != questFLAG ) ||
		/* one arg, allow d,o,V */
	    (optind != argc && (cmd_flags & ~(dFLAG|oFLAG|VFLAG))))
		err_flag++;

	if (err_flag) {
		usage();
		exit(RET_USAGE);
	}

	/*	get mount-point.
		realpath() will be used to resolve the pathname.
		We don't call realpath if /etc/mnttab is not accessible
		since the reason we call realpath is to
		make sure /etc/mnttab is updated correctly.
	*/
	(void) lvlfile(mnttab, MAC_GET, &lid);
	/* clear MACREAD privilege to validate mnttab file is at correct level */
	procprivl(CLRPRV, pm_work(P_MACREAD),(priv_t)0);
	if ((frp = fopen(mnttab, "r")) == NULL) {
		pfmt(stderr, MM_ERROR,
			":2:cannot open mnttab\n");
		mget.mnt_special = argv[optind];
		no_mnttab++;
	}
else {
	mntnull(&mref);
	mref.mnt_mountp = argv[optind];
	if ( realpath(mref.mnt_mountp,resolved) )
		mref.mnt_mountp=resolved;
	if ((ret = getmntany(frp, &mget, &mref)) == -1) {
		mref.mnt_special = mref.mnt_mountp;
		mref.mnt_mountp = NULL;
		rewind(frp);
		if ((ret = getmntany(frp, &mget, &mref)) == -1) {
			pfmt(stderr, MM_WARNING,
				":109:%s not in mnttab\n", mref.mnt_special);
			mntnull(&mget);
			mget.mnt_special = mget.mnt_mountp = mref.mnt_special;
		}
	}
	fclose(frp);

	if (ret > 0)
		mnterror(ret);

  }

	procprivl(SETPRV, pm_work(P_MACREAD),(priv_t)0);

	/* checking length of mget.mnt_fstype here (instead of 2 ifs later)
	   only to preserve exact existing behaviour when -V is specified 
	   along with an essentially impossible FStype being in /etc/mnttab.
	   There is no particular reason for this behaviour. */
	if ((mget.mnt_fstype != NULL) &&
	    (strlen(mget.mnt_fstype) > FSTYPE_MAX)) {
			pfmt(stderr, MM_ERROR,
				":77:FSType %s exceeds %d characters\n", 
					mget.mnt_fstype, FSTYPE_MAX);
			exit(RET_FSTYPE_MAX);
	}


	/* set up for -V mode or file system dependent portion */
	if ((mget.mnt_fstype != NULL) || (cmd_flags & VFLAG)) {

		/* create the new arg list, and end the list with a null pointer */
		ii = 2;
		if (cmd_flags & oFLAG) {
			newargv[ii++] = "-o";
			newargv[ii++] = oarg;
		}
		if (cmd_flags & dashFLAG) {
			newargv[ii++] = "--";
		}
		newargv[ii++] = argv[optind];
		newargv[ii] = NULL;

		/* set the new argv[0] to the filename */
		newargv[1] = myname;
        }

	if (cmd_flags & VFLAG) {
		printf("%s", myname);
		for (ii = 2; newargv[ii]; ii++)
			printf(" %s", newargv[ii]);
		printf("\n");
		exit(RET_OK);
	}

	/* Now we know we want to try to find an FStype dependent umount */
	if (mget.mnt_fstype != NULL) {
		/* build the full pathname of the fstype dependent command. */
		sprintf(full_path, "%s/%s/%s", fs_path, mget.mnt_fstype, myname);
		sprintf(alter_path, "%s/%s/%s", alt_path, mget.mnt_fstype, myname);

		/* Try to exec the fstype dependent umount. */
		execv(full_path, &newargv[1]);
		if (errno == ENOEXEC) {
			newargv[0] = "sh";
			newargv[1] = full_path;
			execv("/sbin/sh", &newargv[0]);
		}
		newargv[1] = myname;
		execv(alter_path, &newargv[1]);
		if (errno == ENOEXEC) {
			newargv[0] = "sh";
			newargv[1] = alter_path;
			execv("/sbin/sh", &newargv[0]);
		}
		/* exec failed */
		if (errno != ENOENT) {
			pfmt(stderr, MM_ERROR,
				":110:cannot execute %s\n", full_path);
			exit(RET_NOEXEC);
		}
	}

	/* If we get here, there is no FS dependent code, 
	   or we don't know FS type */

	/* don't use -o with generic */
	if (cmd_flags & oFLAG) {
		pfmt(stderr, MM_ERROR,
			":111:%s specific umount does not exist\n",
				mget.mnt_fstype ? mget.mnt_fstype : "" );
		pfmt(stderr, MM_ERROR,
			":112:-o suboptions will be ignored\n");
	}

	/*
	 * Try to umount the mountpoint.
	 * If that fails, try the corresponding special.
	 * (This ordering is necessary for nfs umounts.)
	 * (for remote resources:  if the first umount returns EBUSY
	 * don't call umount again - umount() with a resource name
	 * will return a misleading error to the user
	 */
	if (((ret = umount(mget.mnt_mountp)) < 0) &&
	    (errno != EBUSY) && 
	    (!mget.mnt_fstype || (strcmp(mget.mnt_fstype, "rfs") != 0))){
		ret = umount(mget.mnt_special);
	}
	tmperr=errno;

	if (ret < 0) {
		errno=tmperr;
		exitcode = rpterr(argv[optind]);
		if (errno != EINVAL) {
			exit(exitcode);
		}
		else {
			ein_flag = 1;
		}
	}
	/* if the mnttab doesn't exist exit after the umount */
		if (no_mnttab) {
			exit(RET_MNT_OPEN);
		}
	/* remove entry from mnttab */

	/* save the mget entry */
	mp = mgetsave;
	strcpy(mp, mget.mnt_special);
	mget.mnt_special = mp;


	if ((sem_file = creat(SEM_FILE,0600)) == -1 || lockf(sem_file,F_LOCK, 0L) <0 ) {
		pfmt(stderr, MM_WARNING,
			":98:cannot lock temp file <%s>\n",SEM_FILE);
	}


  /* mnttab already ascertained at the right level when
		 MACREAD was restricted above. No need to do it again */

	/* remove entry from mnttab */
	if ((frp = fopen(mnttab, "r+")) == NULL) {
			pfmt(stderr, MM_ERROR,
				":2:cannot open mnttab\n");
			exit(RET_MNT_OPEN);
	}

	/*
	 * Lock the file to prevent many unmounts at once.
	 * This may sleep for the lock to be freed.
	 * This is done to ensure integrity of the mnttab.
	 * Leave P_MACWRITE, P_DACWRITE for lockf.
	 */
	if (lockf(fileno(frp), F_LOCK, 0L) < 0) {
		pfmt(stderr, MM_ERROR,
			":3:cannot lock mnttab\n");
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		exit(RET_MNT_LOCK);
	}

	/* check every entry for validity before we umount */
	while ((ret = getmntent(frp, &mref)) == 0)
		;
	if (ret > 0)
		mnterror(ret);
	rewind(frp);

	if ((fwp = fopen(temp, "w")) == NULL) {
		pfmt(stderr, MM_ERROR,
			":18:cannot open %s for writing\n", temp);
		exit(RET_TMP_OPEN);
	}
	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT,  SIG_IGN);

  /* MNTTAB file should be at SYSPUBLIC */
	procprivl(CLRPRV, pm_work(P_MACREAD),(priv_t)0);
	/* Make sure writes occur now, so errors get reported now */
	setbuf(fwp, NULL);
	/* for each entry ... */
	while ((ret = getmntent(frp, &mref)) != -1)  {
		/* if it's a valid entry and not the one we got above ... */
		if ( ret == 0 && (strcmp(mgetsave, mref.mnt_special) != 0))
			/* put it out */
			if (putmntent(fwp, &mref) <0) {
				pfmt(stderr, MM_ERROR,
					":19:cannot write to %s\n", temp);
				exit(RET_TMP_WRITE);
			}
	}
	procprivl(SETPRV, pm_work(P_MACREAD),(priv_t)0);

	fclose(fwp);

	/* 
	 * Set correct mode on new mnttab.
	 */
	(void) chmod(temp, 0444); 

	/*
	 * If lid is set then set level of temp to lid
	 */

	if (lid)
		(void) lvlfile(temp, MAC_SET, &lid);

	/* 
	 * Set correct ownership on new mnttab.
	 */
	(void) chown(temp, MNTTAB_UID, MNTTAB_GID);

	/*
	 * Rename new mnttab as the real mnttab.
	 */
	rename(temp, mnttab);

	fclose(frp);

/* if ein_flag is set, then this was not mounted - exit with nonzero ret code */
	if (ein_flag )
		exit(RET_EINVAL);
	else  
		exit(RET_OK);
/*NOTREACHED*/
}


/*
 * Procedure:     rpterr
 *
 * Restrictions:
                 fprintf: None
*/

rpterr(sp)
	char	*sp;
{
	int exitcode;

	switch (errno) {
	case EPERM:
		pfmt(stderr, MM_ERROR, ":20:permission denied\n");
		exitcode = RET_EPERM;
		break;
	case ENXIO:
		pfmt(stderr, MM_ERROR, ":113:%s no device\n", sp);
		exitcode = RET_ENODEV;
		break;
	case ENOENT:
		pfmt(stderr, MM_ERROR,
			":60:%s no such file or directory\n", sp);
		exitcode = RET_ENOENT;
		break;
	case EINVAL:
		pfmt(stderr, MM_ERROR, ":114:%s not mounted\n", sp);
		exitcode = RET_EINVAL;
		break;
	case EBUSY:
		pfmt(stderr, MM_ERROR, ":115:%s busy\n", sp);
		exitcode = RET_EBUSY;
		break;
	case ENOTBLK:
		pfmt(stderr, MM_ERROR,
			":116:%s block device required\n", sp);
		exitcode = RET_ENOTBLK;
		break;
	case ECOMM:
		pfmt(stderr, MM_WARNING, ":117:broken link detected\n");
		exitcode = RET_ECOMM;
		break;
	default:
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		pfmt(stderr, MM_ERROR, ":118:cannot unmount %s\n", sp);
		exitcode = RET_MISC;
		break;
	}
	return(exitcode);
}

/*
 * Procedure:     usage
 *
 * Restrictions:
                 fprintf:	None
*/
void
usage()
{
	pfmt(stderr, MM_ACTION,
	    ":119:Usage:\n%s [-V] [-o specific_options] {special | mount-point}\n",
		myname);
}

/*
 * Procedure:     mnterror
 *
 * Restrictions:
                 fprintf:	P_MACWRITE
*/
void
mnterror(flag)
	int	flag;
{
	int exitcode;

	switch (flag) {
	case MNT_TOOLONG:
		pfmt(stderr, MM_ERROR,
			":54:line in mnttab exceeds %d characters\n",
				MNT_LINE_MAX-2);
		exitcode = RET_MNT_TOOLONG;
		break;
	case MNT_TOOFEW:
		pfmt(stderr, MM_ERROR,
			":55:line in mnttab has too few entries\n");
		exitcode = RET_MNT_TOOFEW;
		break;
	default:
		exitcode = RET_MNT_MISC;
		break;
	}
	exit(exitcode);
}
