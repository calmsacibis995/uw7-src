#ident	"@(#)memfs.cmds:common/cmd/fs.d/memfs/mount.c	1.5.1.1"

#include	<stdio.h>
#include	<sys/signal.h>
#include	<unistd.h>	/* defines F_LOCK for lockf */
#include	<sys/errno.h>
#include	<sys/mnttab.h>
#include	<sys/mount.h>	/* exit code definitions */
#include	<sys/types.h>
#include	<sys/statvfs.h>
#include	<sys/fs/memfs.h>
#include        <locale.h>
#include        <pfmt.h>

#define	TIME_MAX	16
#define	NAME_MAX	64	/* sizeof "fstype myname" */

#define	FSTYPE		"memfs"

#define	RO_BIT		1

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

char *string;

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
	int	mntflag = 0;
	int	s, cc, ret, rwflag = 0;
	struct mnttab	mm,mget;
	struct memfs_args margs;
	int	roflag = 0;
	char    label[NAME_MAX];

	(void)setlocale(LC_ALL,"");
        (void)setcat("uxmount");
	myname = strrchr(argv[0], '/');
	if (myname)
		myname++;
	else
		myname = argv[0];
	sprintf(typename, "%s %s", fstype, myname);
	strcpy(label,"UX:");
        strcat(label,typename);
        (void)setlabel(label);

	argv[0] = typename;

	margs.swapmax = 0;
	margs.rootmode = 0;
	margs.sfp = 0;
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
			/*
			 * memfs specific options.
			 */
			string = optarg;
			while (*string != '\0') {
				if (match("swapmax=")) 
					margs.swapmax = number();
				else if (match("rootmode=")) {
					sscanf(string,"%o", &margs.rootmode);
					number();
				}else if (match("sfp=")) {
					s = number();
					margs.sfp = s;
				}

				/* remove the sepatator */
				if (*string == ',') string++;
				else if (*string == '\0') break;
				else {
					pfmt(stderr, MM_ERROR, ":1:illegal option: %s\n",
						string);
					usage();
				}
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
		pfmt(stderr, MM_WARNING,":17:conflicting suboptions\n");
		usage();
	}
		
	if ( ((argc - optind) != 2) || (errflag) )
		usage();

	special = argv[optind++];
	mountp = argv[optind++];


	mm.mnt_special = special;
	mm.mnt_mountp = mountp;
	mm.mnt_fstype = fstype;
	mm.mnt_mntopts = "rw";
	sprintf(tbuf, "%ld", time(0L));	/* assumes ld == long == time_t */
	mm.mnt_time = tbuf;


	if ((fwp = fopen(mnttab, "r")) == NULL) {
		pfmt(stderr, MM_ERROR,
		":2:cannot open mnttab\n");
	}

	/* Open /etc/mnttab read-write to allow locking the file */
	if ((fwp = fopen(mnttab, "r+")) == NULL) {
		pfmt(stderr, MM_ERROR,
		":2: cannot open mnttab\n");
		exit(RET_MNT_OPEN);
	}

	/*
	 * Lock the file to prevent many updates at once.
	 * This may sleep for the lock to be freed.
	 * This is done to ensure integrity of the mnttab.
	 */
	if (lockf(fileno(fwp), F_LOCK, 0L) < 0) {
		pfmt(stderr, MM_ERROR,
		":3: cannot lock mnttab\n");
		perror(myname);
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

	do_mount(special, mountp, mntflag, &margs);
	fseek(fwp, 0L, 2);
	putmntent(fwp, &mm);
	fclose(fwp);

	exit(RET_OK);
	/* NOTREACHED */
}

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
		":21:%s or %s, no such file or directory or no previous mount was performed\n", bs, mp);
		pfmt(stderr, MM_ERROR,
		":7:%s or %s, no such file or directory\n", bs, mp);
		return(RET_ENOENT);
	case EINVAL:
		pfmt(stderr, MM_ERROR,
		":22:%s is not %s file system,\n\tor %s is busy.\n", 
			bs, fstype,mp);
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
                        ":11:%swrite-protected\n", bs);
		return(RET_EROFS);
	case ENOSPC:
		pfmt(stderr, MM_ERROR,
                        ":23:%s is corrupted. Needs checking\n", bs);
		return(RET_ENOSPC);
	case ENODEV:
		pfmt(stderr, MM_ERROR,
                        ":24:%s no such device or device is write-protected\n",bs);
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
do_mount(special, mountp, flag, margp)
	char	*special, *mountp;
	int	flag;
	struct memfs_args *margp;
{
	register char *ptr;
	struct statvfs stbuf;

	if (mount(special, mountp, flag | MS_DATA, fstype, margp,
						sizeof(struct memfs_args)))
 		exit(rpterr(special, mountp));

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

int match(s)
char *s;
{
        register char *cs;
 
        cs = string;
        while (*cs++ == *s)
        {
                if (*s++ == '\0')
                {
                        goto true;
                }
        }
        if (*s != '\0')
        {
                return(0);
        }
 
true:
        cs--;
        string = cs;
        return(1);
}

int
number()
{
        register char *cs;
        long n;

        cs = string;
        n = 0;
        while ((*cs >= '0') && (*cs <= '9'))
        {
                n = n*10 + *cs++ - '0';
        }
        for (;;)
        {
                switch (*cs++)
                {

                case 'k':
                        n *= 1024;
                        continue;
                /* Fall into exit test, recursion has read rest of string */
                /* End of string, check for a valid number */
 
		case ',':
                case '\0':
			cs--;
			string = cs;
                        return(n);
 
                default:
			pfmt(stderr, MM_ERROR,
                        	":125: bad numeric arg: \"%s\"\n", string);
                        exit(-1);

                }
        } /* never gets here */
}
usage()
{
	pfmt(stderr, MM_ACTION,
		":126:Usage:\n%s [-F %s] [generic_options] [-r] [-o swapmax=xx,rootmode=xx]  special mount_point\n",
		myname, fstype);
	exit(RET_USAGE);
	/* NOTREACHED */
}
