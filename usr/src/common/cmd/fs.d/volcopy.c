/*		copyright	"%c%" 	*/

#ident	"@(#)fs.cmds:common/cmd/fs.d/volcopy.c	1.10.9.4"
#ident  "$Header$"


/***************************************************************************
 * Command: volcopy
 * Inheritable Privileges: P_SYSOPS P_MACREAD,P_MACWRITE,P_DACREAD,
 *				P_DACWRITE,P_DEV,P_SETFLEVEL
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

#include	<stdio.h>
#include 	<limits.h>
#include	<errno.h>
#include	<varargs.h>
#include	<sys/vfstab.h>
#include	<sys/types.h>
#include	<priv.h>
#include        <locale.h>
#include        <pfmt.h>

#define	ARGV_MAX	1024
#define	FSTYPE_MAX	8

#define	VFS_PATH	"/usr/lib/fs"

#define	EQ(X,Y,Z)	!strncmp(X,Y,Z)
#define	NEWARG()\
	(nargv[nargc++] = &argv[1][0],\
	 nargc == ARGV_MAX ? perr(":4:too many arguments.\n") : 1)

extern int	errno;

char	*nargv[ARGV_MAX];
int	nargc = 2;

char	vfstab[] = VFSTAB;
void pusage();


/*
 * Procedure:     main
 *
 * Restrictions:
 *                fopen: P_MACREAD;
 *                getvfsany: none
 *                rewind: none
 *                fclose: none
 *                printf: none
 */

char    *myname;	/* point to argv[0] */

main(argc, argv)
	int	argc;
	char	**argv;
{
	register char	cc;
	register int	ii, Vflg = 0, Fflg = 0;
	register char	*fstype = NULL;
	register FILE	*fd;
	struct vfstab	vget, vref;
	char    label[NAME_MAX];

	(void)setlocale(LC_ALL,"");
        (void)setcat("uxvolcopy");
	
	myname = strrchr(argv[0], '/');
        myname = (myname != 0)? myname+1: argv[0];
	sprintf(label, "UX:%s", myname);
	(void)setlabel(label);

	while (argc > 1 && argv[1][0] == '-') {
		if (EQ(argv[1], "-a", 2)) {
			NEWARG();
		} else if (EQ(argv[1], "-e", 2)) {
			NEWARG();
		} else if (EQ(argv[1], "-s", 2)) {
			NEWARG();
		} else if (EQ(argv[1], "-y", 2)) {
			NEWARG();
		} else if (EQ(argv[1], "-buf", 4)) {
			NEWARG();
		} else if (EQ(argv[1], "-bpi", 4)) {
			NEWARG();
			if ((cc = argv[1][4]) < '0' || cc > '9') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-feet", 5)) {
			NEWARG();
			if ((cc = argv[1][5]) < '0' || cc > '9') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-reel", 5)) {
			NEWARG();
			if ((cc = argv[1][5]) < '0' || cc > '9') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-r", 2)) { /* 3b15 only */
			NEWARG();
			if ((cc = argv[1][2]) < '0' || cc > '9') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-block", 6)) { /* 3b15 only */
			NEWARG();
			if ((cc = argv[1][6]) < '0' || cc > '9') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-V", 2)) {
			Vflg++;
		} else if (EQ(argv[1], "-F", 2)) {
			if (Fflg) {
				perr(":1:More than one FSType specified.\n%s");
				pusage();
			}
			Fflg++;
			if (argv[1][2] == '\0') {
				++argv;
				--argc;
				fstype = &argv[1][0];
			} else
				fstype = &argv[1][2];
			if(!fstype)
				pusage();
			else
				 if (strlen(fstype) > FSTYPE_MAX) {
					perr(":2:FSType %s exceeds %d characters\n", fstype, FSTYPE_MAX);
					exit(1);
				}
		} else if (EQ(argv[1], "-o", 2)) {
			NEWARG();
			if (argv[1][2] == '\0') {
				++argv;
				--argc;
				NEWARG();
			}
		} else if (EQ(argv[1], "-nosh", 5)) { /* 3b15 only */
			NEWARG();
		} else if (EQ(argv[1], "-?", 2)) { 
			if (Fflg) {
				nargv[2] = "-?";
				doexec(fstype, nargv);
			}
			else 
				pusage();
		} else{ 
			perr(":3:<%s> invalid option\n",argv[1]);
			pusage();
		}
		++argv;
		--argc;
	} /* argv[1][0] == '-' */

	if (argc != 6) /* if mandatory fields not present */
		pusage();

	if (nargc + 5 >= ARGV_MAX) {
		perr(":4:too many arguments.\n");
		exit(1);
	}

	for (ii = 0; ii < 5; ii++)
		nargv[nargc++] = argv[ii+1];

	if (fstype == NULL) {
		procprivl(CLRPRV, MACREAD_W,0);
		if ((fd = fopen(vfstab, "r")) == NULL) {
			perr(":5:cannot open %s.\n", vfstab);
			exit(1);
		}
		procprivl(SETPRV, MACREAD_W,0);

		vfsnull(&vref);
		vref.vfs_special = argv[2];
		ii = getvfsany(fd, &vget, &vref);
		if (ii == -1) {
			rewind(fd);
			vfsnull(&vref);
			vref.vfs_fsckdev = argv[2];
			ii = getvfsany(fd, &vget, &vref);
		}

		fclose(fd);

		switch (ii) {
		case -1:
			perr(":6:File system type cannot be identified.\n");
			exit(1);
		case 0:
			fstype = vget.vfs_fstype;
			break;
		case VFS_TOOLONG:
			perr(":7:line in vfstab exceeds %d characters\n", VFS_LINE_MAX-2);
			exit(1);
		case VFS_TOOFEW:
			perr(":8:line in vfstab has too few entries\n");
			exit(1);
		case VFS_TOOMANY:
			perr(":9:line in vfstab has too many entries\n");
			exit(1);
		}

	}
	if (Vflg) {
		printf("volcopy -F %s", fstype);
		for (ii = 2; nargv[ii]; ii++)
			printf(" %s", nargv[ii]);
		printf("\n");
		exit(0);
	}

	doexec(fstype, nargv);
}


/*
 * Procedure:     doexec
 *
 * Restrictions:
 *                sprintf: none
 *                execv(2): P_MACREAD;
 */

doexec(fstype, nargv)
	char	*fstype, *nargv[];
{
	char	full_path[PATH_MAX];
	char	*vfs_path = VFS_PATH;

	/* build the full pathname of the fstype dependent command. */
	sprintf(full_path, "%s/%s/volcopy", vfs_path, fstype);

	/* set the new argv[0] to the filename */
	nargv[1] = "volcopy";

	/* Try to exec the fstype dependent portion of the mount. */
	procprivl(CLRPRV,MACREAD_W,0);
	execv(full_path, &nargv[1]);
	if (errno == EACCES) {
		perr(":10:cannot execute %s - permission denied\n", full_path);
		exit(1);
	}
	if (errno == ENOEXEC) {
		nargv[0] = "sh";
		nargv[1] = full_path;
		execv("/sbin/sh", &nargv[0]);
	}
	perr(":11:Operation not applicable for FSType %s\n", fstype);
	exit(1);
}


/*
 * Procedure:     perr
 *
 * Restrictions:
 *               vfprintf:  none
 * Notes:
 *	Print error messages.
 */

int
perr(va_alist)
va_dcl
{
	register char *fmt_p;
	va_list v_Args;

	va_start(v_Args);
	fmt_p = va_arg(v_Args, char *);
	vpfmt(stderr, MM_ERROR, fmt_p, v_Args);
	va_end(v_Args);
}

void
pusage()
{
	pfmt(stderr, MM_ACTION,
		":12:Usage:\nvolcopy [-F Fstype] [-V] [current_options] [-o specific_options] operands\n");

	exit(1);
}
