#ident	"@(#)specfs.cmds:common/cmd/fs.d/specfs/mount.c	1.1"

#include	<stdio.h>
#include	<signal.h>
#include	<unistd.h>	/* defines F_LOCK for lockf */
#include	<errno.h>
#include	<sys/mnttab.h>
#include	<sys/mount.h>
#include	<sys/types.h>
#include        <sys/fs/snode.h>


#define	TIME_MAX	16
#define	NAME_MAX	64	/* sizeof "fstype myname" */

#define	FSTYPE		"specfs"

#define	RO_BIT		1

extern int	errno;
extern int	optind;
extern char	*optarg;

extern char	*strrchr();
extern time_t	time();

int	roflag = 0;

char	typename[NAME_MAX], *myname;
char	mnttab[] = MNTTAB;
char	fstype[] = FSTYPE;
char	*myopts[] = {
#define OPT_READONLY 0
	"ro",
#define OPT_READWRITE 1
	"rw",
#define OPT_DEV 2
	"dev",
	NULL
};

void
prt_err(char *fmt, char *str) {
	fprintf(stderr, fmt, str);
}

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
	struct specfs_args margs;

	myname = strrchr(argv[0], '/');
	if (myname)
		myname++;
	else
		myname = argv[0];
	sprintf(typename, "%s %s", fstype, myname);
	argv[0] = typename;

	/*
	 *	check for proper arguments
	 */

	while ((cc = getopt(argc, argv, "?o:r")) != -1)
		switch (cc) {
		case 'r':
			if (roflag & RO_BIT)
				errflag++;
			else
				roflag |= RO_BIT;
			break;
		case 'o':
			options = optarg;
			while (*options != '\0')
				switch (getsubopt(&options, myopts, &value)) {
				case OPT_READONLY:
					if ((roflag & RO_BIT) || rwflag)
						errflag++;
					else
						roflag |= RO_BIT;
					break;
				case OPT_READWRITE:
					if ((roflag & RO_BIT) || rwflag)
						errflag++;
					else
						rwflag = 1;
					break;
				case OPT_DEV:
					if (value == NULL) {
						errflag++;
						prt_err("no dev parameter supplied", 0);
						break;
					}
					else if(strlen(value) >= SPEC_DRV_NAME_MAX){
						errflag++;
						prt_err("dev parameter too long", 0);
						break;
					}
					strcpy(margs.a_drvname, value);
					break;
				default:
					prt_err("bad token %s\n", value);
					errflag++;
					break;
				}
			break;
		case '?':
			errflag++;
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
		fprintf(stderr, "%s: cannot open mnttab\n", myname);
		exit(1);
	}

	/*
	 * Lock the file to prevent many updates at once.
	 * This may sleep for the lock to be freed.
	 * This is done to ensure integrity of the mnttab.
	 */
	if (lockf(fileno(fwp), F_LOCK, 0L) < 0) {
		fprintf(stderr, "%s: cannot lock mnttab\n", myname);
		perror(myname);
		exit(1);
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

	do_mount(special, mountp, roflag & RO_BIT, &margs);

	putmntent(fwp, &mm);

	exit(0);
}


/*
 * Procedure:     rpterr
 *
 * Restrictions:
                 fprintf:	none
                 perror:	none
*/
rpterr(bs, mp)
	register char *bs, *mp;
{
	switch (errno) {
	case EPERM:
		fprintf(stderr, "%s: not privileged user\n", myname);
		break;
	case ENXIO:
		fprintf(stderr, "%s: %s no such device\n", myname, bs);
		break;
	case ENOTDIR:
		fprintf(stderr,
			"%s: %s not a directory\n\tor a component of %s is not a directory\n",
			myname, mp, bs);
		break;
	case ENOENT:
		fprintf(stderr, "%s: %s or %s, no such file or directory\n",
			myname, bs, mp);
		break;
	case EINVAL:
		fprintf(stderr, "%s: %s is not this fstype.\n", myname, bs);
		break;
	case EBUSY:
		fprintf(stderr,
			"%s: %s is already mounted, %s is busy,\n\tor allowable number of mount points exceeded\n",
			myname, bs, mp);
		break;
	case ENOTBLK:
		fprintf(stderr, "%s: %s not a block device\n", myname, bs);
		break;
	case EROFS:
		fprintf(stderr, "%s: %s write-protected\n", myname, bs);
		break;
	case ENOSPC:
		fprintf(stderr,
			"%s: the state of %s is not okay\n\tand it was attempted to mount read/write\n",
			myname, bs);
		break;
	default:
		perror(myname);
		fprintf(stderr, "%s: cannot mount %s\n", myname, bs);
	}
}


do_mount(special, mountp, rflag,  margp)
	char	*special, *mountp;
	int	rflag;
	struct	specfs_args *margp;

{
	if (mount(special, mountp, rflag | MS_DATA, fstype, margp,
	   sizeof(struct specfs_args))) {
		rpterr(special, mountp);
		exit(2);
	}
}


/*
 * Procedure:     usage
 *
 * Restrictions:
                 fprintf:	none
*/
usage()
{
	fprintf(stderr,
		"%s usage:\n%s [-F %s] [-r] [-o specific_options] {special | mount_point}\n%s [-F %s] [-r] [-o specific_options] special mount_point\n",
		fstype, myname, fstype, myname, fstype);
	exit(1);
}
