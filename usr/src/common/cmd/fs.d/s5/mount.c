/*		copyright	"%c%" 	*/

#ident	"@(#)s5.cmds:common/cmd/fs.d/s5/mount.c	1.15.12.15"

#include	<stdio.h>
#include	<sys/signal.h>
#include	<unistd.h>	/* defines F_LOCK for lockf */
#include	<sys/errno.h>
#include	<sys/mnttab.h>
#include	<sys/mount.h>	/* exit code definitions */
#include	<sys/types.h>
#include	<sys/statvfs.h>
#include	<locale.h>
#include	<ctype.h>
#include	<pfmt.h>

#define	TIME_MAX	16
#define	NAME_MAX	64	/* sizeof "fstype myname" */

#define	FSTYPE		"s5"

#define	RO_BIT		1

#define	READONLY	0
#define	READWRITE	1
#define SUID		2
#define NOSUID		3
#define REMOUNT		4

extern int	errno;
extern int	optind;
extern char	*optarg;

extern char	*strrchr();
extern time_t	time();

/* Forward definitions */
void	do_mount();

char	typename[NAME_MAX], *myname;
char	mnttab[] = MNTTAB;
char	temp[] = "/etc/mnttab.tmp";
char	fstype[] = FSTYPE;
char	*myopts[] = {
	"ro",
	"rw",
	"suid",
	"nosuid",
	"remount",
	NULL
};

main(argc, argv)
	int	argc;
	char	**argv;
{
	FILE	*fwp,*frp;
	char	*special, *mountp;
	char	*options, *value;
	char	tbuf[TIME_MAX];
	int	errflag = 0;
	int	confflag = 0;
	int 	suidflg = 0; 
	int 	nosuidflg = 0;
	int	remountflg = 0;
	int	mntflag =0;
	int	cc, ret, rwflag = 0;
	struct mnttab	mm,mget;
	char	label[NAME_MAX];
	int	roflag = 0;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmount");
	myname = strrchr(argv[0], '/');
	myname = (myname != 0)? myname+1: argv[0];
	sprintf(typename, "%s %s", fstype, myname);
	strcpy(label,"UX:");
	strcat(label,typename);
	(void)setlabel(label);

	argv[0] = typename;

	/*
	 *	check for proper arguments
	 */

	while ((cc = getopt(argc, argv, "?o:r")) != -1)
		switch (cc) {
		case 'r':
			if ((roflag & RO_BIT) || rwflag )
				confflag = 1;
			else if (rwflag)
				confflag = 1;
			else {
				roflag |= RO_BIT;
				mntflag |= MS_RDONLY;
			}
			break;
		case 'o':
			options = optarg;
			while (*options != '\0')
				switch (getsubopt(&options, myopts, &value)) {
				case READONLY:
					if (rwflag || remountflg)
						confflag = 1;
					else {
						roflag |= RO_BIT;
						mntflag |= MS_RDONLY;
					}
					break;
				case READWRITE:
					if (roflag & RO_BIT)
						confflag = 1;
					else if (rwflag)
						errflag = 1;
					else
						rwflag = 1;
					break;
				case SUID:
					if (nosuidflg)
						confflag = 1;
					else if (suidflg)
						errflag = 1;
					else 
						suidflg++; 
					break;
				case NOSUID:
					if (suidflg)
						confflag = 1;
					else if (nosuidflg)
						errflag = 1;
					else {
						mntflag |= MS_NOSUID;
						nosuidflg++; 
					}
					break;
				case REMOUNT:
					if (roflag)
						confflag = 1;
					else if (remountflg)
						errflag = 1;
					else if (roflag & RO_BIT)
						confflag = 1;
					else {
						remountflg++;
						mntflag |= MS_REMOUNT;
					}
					break;
				default:
					pfmt(stderr, MM_ERROR,
						":1:illegal -o suboption -- %s\n", value);
					errflag++;
				}
			break;
		case '?':
			errflag = 1;
			break;
		}


	/*
	 *	There must be at least 2 more arguments, the
	 *	special file and the directory.
	 */

	if (confflag) {
		pfmt(stderr, MM_WARNING,
			":17:conflicting suboptions\n");
		usage();
	}
		
	if ( ((argc - optind) != 2) || (errflag) )
		usage();

	special = argv[optind++];
	mountp = argv[optind++];


	mm.mnt_special = special;
	mm.mnt_mountp = mountp;
	mm.mnt_fstype = fstype;
	if (roflag & RO_BIT)
		mm.mnt_mntopts = "ro";
	else
		mm.mnt_mntopts = "rw";
	if (nosuidflg)
		strcat(mm.mnt_mntopts, ",nosuid");
	else
		strcat(mm.mnt_mntopts, ",suid");
	sprintf(tbuf, "%ld", time(0L));	/* assumes ld == long == time_t */
	mm.mnt_time = tbuf;

	if ((fwp = fopen(mnttab, "r")) == NULL) {
		pfmt(stderr, MM_WARNING,
			":2:cannot open mnttab\n");
	}

	/* Open /etc/mnttab read-write to allow locking the file */
	if ((fwp = fopen(mnttab, "r+")) == NULL) {
		pfmt(stderr, MM_ERROR,
			":2:cannot open mnttab\n");
		exit(RET_MNT_OPEN);
	}

	/*
	 * Lock the file to prevent many updates at once.
	 * This may sleep for the lock to be freed.
	 * This is done to ensure integrity of the mnttab.
	 */
	if (lockf(fileno(fwp), F_LOCK, 0L) < 0) {
		pfmt(stderr, MM_ERROR,
			":3:cannot lock mnttab\n");
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		exit(RET_MNT_LOCK);
	}

	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT,  SIG_IGN);

	/*
	 *	Perform the mount.
	 *	Only the low-order bit of "roflag" is used by the system
	 *	calls (to denote read-only or read-write).
	 */

	do_mount(special, mountp, mntflag);
	if (remountflg) {
		if ((frp = fopen(temp, "w")) == NULL) {
			pfmt(stderr, MM_ERROR,
				":18:cannot open %s for writing\n", temp);
			exit(RET_TMP_OPEN);
		}
	
		rewind(fwp);
		/* Make sure writes happen right away, so we see errors.*/
		setbuf(frp, NULL);
		/* for each entry ... */
		while ((ret = getmntent(fwp, &mget)) != -1)  {
			/* if it's a valid entry and not the one we got above ... */
			if ( ret == 0 && (strcmp(mm.mnt_special, mget.mnt_special) != 0))
				/* put it out */
				if (putmntent(frp, &mget) <0) {
					pfmt(stderr, MM_ERROR,
					    ":19:cannot write to %s\n",
						temp);
					exit(RET_TMP_WRITE);
				}
		}

		putmntent(frp, &mm);
		fclose(frp);
		rename(temp,mnttab);
		fclose(fwp);
	}
	else {
		fseek(fwp, 0L, 2);
		putmntent(fwp, &mm);
		fclose(fwp);
	}

	exit(RET_OK);
	/* NOTREACHED */
}

rpterr(bs, mp, fstype)
	register char *bs, *mp, *fstype;
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
			":21:%s or %s, no such file or directory or no previous mount was performed\n",
				bs, mp);
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
			":14:%s file system module cannot be loaded\n", fstype);
		return(RET_ENOLOAD);
	default:
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		pfmt(stderr, MM_ERROR,
			":15:cannot mount %s\n", bs);
		return(RET_MISC);
	}
}

void
do_mount(special, mountp, flag)
	char	*special, *mountp;
	int	flag;
{
	register char *ptr;
	struct statvfs stbuf;

	if (mount(special, mountp, flag | MS_DATA, fstype, NULL, 0))
 		exit(rpterr(special, mountp, fstype));

	/*
	 *	compare the basenames of the mount point
	 *	and the volume name, warning if they differ.
	 */

	if (statvfs(mountp, &stbuf) == -1)
		return;

	ptr = stbuf.f_fstr;
	while (*ptr == '/')
		ptr++;

}

usage()
{
	pfmt(stderr, MM_ACTION,
		":25:Usage:\n%s [-F %s] [generic_options] [-r] [-o {[rw|ro],[suid|nosuid],[remount]}] {special | mount_point}\n%s [-F %s] [generic_options] [-r] [-o {[rw|ro],[suid|nosuid],[remount]}] special mount_point\n",
		myname, fstype, myname, fstype);
	exit(RET_USAGE);
	/* NOTREACHED */
}
