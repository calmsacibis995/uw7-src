/*	copyright	"%c%"	*/

#ident	"@(#)bfs.cmds:common/cmd/fs.d/bfs/mount.c	1.7.8.4"

/***************************************************************************
 * Command: mount
 * Inheritable Privileges: P_MOUNT,P_DACWRITE,P_MACWRITE
 *       Fixed Privileges: None
 * Notes: mount BFS file systems
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<signal.h>
#include	<unistd.h>
#include	<errno.h>
#include	<sys/mnttab.h>
#include	<sys/mount.h>
#include	<sys/types.h>
#include	<locale.h>
#include	<ctype.h>
#include	<pfmt.h>

#define	TIME_MAX	16
#define	NAME_MAX	64	/* sizeof "fstype myname" */

#define	FSTYPE		"bfs"

#define	RO_BIT		1

#define	READONLY	0
#define	READWRITE	1

extern int	errno;
extern int	optind;
extern char	*optarg;

extern char	*strrchr();
extern time_t	time();

/* Forward declarations */
void	usage();
void	do_mount();

char	typename[NAME_MAX], *myname;
char	mnttab[] = MNTTAB;
char	fstype[] = FSTYPE;
char	*myopts[] = {
	"ro",
	"rw",
	NULL
};

/*
 * Procedure:     main
 *
 * Restrictions:
                 sprintf:	none
                 getopt:	none
                 fprintf:	none
                 fopen:		none
                 lockf:		none
                 fseek:		none
 */
main(argc, argv)
	int	argc;
	char	**argv;
{
	FILE	*fwp;
	char	*special, *mountp;
	char	*options, *value;
	char	tbuf[TIME_MAX];
	int	errflag = 0;
	int	cc, rwflag = 0;
	struct mnttab	mm;
	char	label[NAME_MAX];
	int	roflag = 0;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmount");
	myname = strrchr(argv[0], '/');
	myname = (myname != 0)? myname+1: argv[0];
	sprintf(typename, "%s %s", fstype, myname);
	strcpy(label, "UX:");
	strcat(label, typename);
	(void)setlabel(label);
	argv[0] = typename;

	/*
	 *	check for proper arguments
	 */

	while ((cc = getopt(argc, argv, "?o:r")) != -1)
		switch (cc) {
		case 'r':
			if (roflag & RO_BIT || rwflag)
				errflag = 1;
			else
				roflag |= RO_BIT;
			break;
		case 'o':
			options = optarg;
			while (*options != '\0')
				switch (getsubopt(&options, myopts, &value)) {
				case READONLY:
					if ((roflag & RO_BIT) || rwflag)
						errflag = 1;
					else
						roflag |= RO_BIT;
					break;
				case READWRITE:
					if ((roflag & RO_BIT) || rwflag)
						errflag = 1;
					else
						rwflag = 1;
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
	sprintf(tbuf, "%ld", time(0L));	/* assumes ld == long == time_t */
	mm.mnt_time = tbuf;


	/* Open /etc/mnttab read-write to allow locking the file */
	if ((fwp = fopen(mnttab, "a")) == NULL) {
		pfmt(stderr, MM_ERROR, ":2:cannot open mnttab\n");
		exit(31+8);
	}

	/*
	 * Lock the file to prevent many updates at once.
	 * This may sleep for the lock to be freed.
	 * This is done to ensure integrity of the mnttab.
	 */
	if (lockf(fileno(fwp), F_LOCK, 0L) < 0) {
		pfmt(stderr, MM_ERROR, ":3:cannot lock mnttab\n");
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		exit(31+8);
	}

	/* end of file may have changed behind our backs */
	fseek(fwp, 0L, 2);

	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT,  SIG_IGN);

	/*
	 *	Perform the mount.
	 *	Only the low-order bit of "roflag" is used by the system
	 *	calls (to denote read-only or read-write).
	 */

	do_mount(special, mountp, roflag & RO_BIT);

	putmntent(fwp, &mm);

	exit(0);
}

/*
 * Procedure:     rpterr
 *
 * Restrictions:
                 fprintf:	none
 */
void	rpterr(bs, mp)
	register char *bs, *mp;
{
	switch (errno) {
	case EPERM:
		pfmt(stderr, MM_ERROR, ":4:not privileged user\n");
		break;
	case ENXIO:
		pfmt(stderr, MM_ERROR, ":5:%s no such device\n", bs);
		break;
	case ENOTDIR:
		pfmt(stderr, MM_ERROR,
			":6:%s not a directory\n\tor a component of %s is not a directory\n",
				mp, bs);
		break;
	case ENOENT:
		pfmt(stderr, MM_ERROR,
			":7:%s or %s, no such file or directory\n",
				bs, mp);
		break;
	case EINVAL:
		pfmt(stderr, MM_ERROR,
			":8:%s is not this fstype.\n", bs);
		break;
	case EBUSY:
		pfmt(stderr, MM_ERROR,
			":9:%s is already mounted, %s is busy,\n\tor allowable number of mount points exceeded\n",
				bs, mp);
		break;
	case ENOTBLK:
		pfmt(stderr, MM_ERROR,
			":10:%s not a block device\n", bs);
		break;
	case EROFS:
		pfmt(stderr, MM_ERROR, ":11:%s write-protected\n", bs);
		break;
	case ENOSPC:
		pfmt(stderr, MM_ERROR,
			":12:the state of %s is not okay\n", bs);
		break;
	case ENODEV:
		pfmt(stderr, MM_ERROR,
			":13:%s no such device or write-protected\n",
			bs);
		break;
	case ENOLOAD:
		pfmt(stderr, MM_ERROR,
			":14:%s file system module cannot be loaded\n", fstype);
		break;
	default:
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		pfmt(stderr, MM_ERROR,
			":15:cannot mount %s\n", bs);
	}
}

/*
 * Procedure:     do_mount
 *
 * Restrictions:
                 mount(2):	none
 */
void	do_mount(special, mountp, rflag)
	char	*special, *mountp;
	int	rflag;
{
	if (mount(special, mountp, rflag | MS_DATA, fstype)) {
		rpterr(special, mountp);
		exit(31+8);
	}
}

/*
 * Procedure:     usage
 *
 * Restrictions:
                 fprintf:	none
 */
void	usage()
{
	pfmt(stderr, MM_ACTION,
		":16:Usage:\n%s [-F %s] [-r] [-o rw|ro] {special | mount_point}\n%s [-F %s] [-r] [-o rw|ro] special mount_point\n",
		myname, fstype, myname, fstype);
	exit(31+8);
}
