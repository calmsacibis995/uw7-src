/*		copyright	"%c%" 	*/

#ident	"@(#)fuser:fuser.c	1.27.6.1"

/***************************************************************************
 * Command : fuser
 * Inheritable Privileges : P_DEV,P_MACREAD,P_DACREAD,P_OWNER,P_COMPAT
 *       Fixed Privileges :
 * Notes:
 *
 ***************************************************************************/
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <priv.h>
#include <sys/mnttab.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/var.h>
#include <sys/utssys.h>
#include <sys/ksym.h>
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>

void exit(), perror();
extern char *malloc();
extern int errno;

/*
*Procedure:     spec_to_mount
*
* Restrictions:
                 fopen: P_MACREAD
                 getmntany: none
                 fclose:none
                 fprintf: none
*/
/*
 * Return a pointer to the mount point matching the given special name, if
 * possible, otherwise, exit with 1 if mnttab corruption is detected, else
 * return NULL.
 *
 * NOTE:  the underlying storage for mget and mref is defined static by
 * libos.  Repeated calls to getmntany() overwrite it; to save mnttab
 * structures would require copying the member strings elsewhere.
 */
char *
spec_to_mount(specname)
char	*specname;
{
	FILE	*frp;
	int 	ret;
	struct mnttab mref, mget;

	procprivl(CLRPRV,MACREAD_W,(priv_t)0);
	/* get mount-point */
	if ((frp = fopen(MNTTAB, "r")) == NULL) {
		procprivl(SETPRV,MACREAD_W,(priv_t)0);
		return NULL;
	}
	procprivl(SETPRV,MACREAD_W,(priv_t)0);
	mntnull(&mref);
	mref.mnt_special = specname;
	ret = getmntany(frp, &mget, &mref);
	fclose(frp);
	if (ret == 0) {
		return (mget.mnt_mountp);
	} else if (ret > 0) {
		pfmt(stderr, MM_ERROR, ":68:mnttab is corrupted\n");
		exit(1);
	} else {
		return NULL;
	}
}

/* symbol names */
#define V_STR	"v"
#define MEMF	"/dev/kmem"

/*
 * Procedure:     get_f_user_buf
 *
 * Restrictions:
                 open(2): P_MACREAD
                 fprintf: none
                 perror: none
		 ioctl(2): none
                 printf: none
                 read(2): none
*/
/*
 * The main objective of this routine is to allocate an array of f_user_t's.
 * In order for it to know how large an array to allocate, it must know
 * the value of v.v_proc in the kernel.  To get this, use the MICO_READKSYM
 * ioctl on /dev/kmem to read the struct var at symbol v into
 * the local v.  Return the allocation result.  Exit with a status of 1
 * if any error is encountered in this process.
 */

f_user_t *
get_f_user_buf()
{
	int mem, tmperr;	/* errno is cleared by successful printfs */
	struct var v;
	struct mioc_rksym rks;


	procprivl(CLRPRV,MACREAD_W,(priv_t)0);
	/* open file to access memory */
	if ((mem = open(MEMF, O_RDONLY)) == -1) {
		tmperr = errno;
		pfmt(stderr, MM_ERROR,
			":69:open of %s failed: ", MEMF);
		errno = tmperr;
		perror("");
		exit(1);
	}
	procprivl(SETPRV,MACREAD_W,(priv_t)0);
	rks.mirk_symname = V_STR;
	rks.mirk_buf = (char *) &v;
	rks.mirk_buflen = sizeof(struct var);
	if (ioctl(mem, MIOC_READKSYM, &rks) != 0) {
			tmperr = errno;
			pfmt(stderr, MM_ERROR,
				":70:MIOC_REAKSYM ioctl on %s failed: ", MEMF);
			errno = tmperr;
			perror("");
			exit(1);
	}
	return (f_user_t *)malloc(v.v_proc * sizeof(f_user_t));
}

/*
 * Procedure:     usage
 *
 * Restrictions:
                 fprintf: none
*/
/*
 * display the fuser usage message and exit
 */
void
usage()
{
	pfmt(stderr,MM_ACTION,
	    ":71:Usage: fuser [-ku[c|f]] files [-[ku[c|f]] files]\n");
	exit(1);
}

struct co_tab {
	int	c_flag;
	char	c_char;
};

static struct co_tab code_tab[] = {
	{F_CDIR, 'c'},
	{F_RDIR, 'r'},
	{F_TEXT, 't'},
	{F_OPEN, 'o'},
	{F_MAP, 'm'},
/*	{F_TTY, 'y'}, 		does not work */
	{F_TRACE, 'a'}		/* trace file */
};


/*

 * Procedure:     report
 *
 * Restrictions:
                 fprintf: none 
                 fflush:  none  
                 getpwuid: none
                 kill(2): none
*/
/*
 * Show pids and usage indicators for the nusers processes in the users list.
 * When usrid is non-zero, give associated login names.  When gun is non-zero,
 * issue kill -9's to those processes.
 */
void
report(users, nusers, usrid, gun)
f_user_t *users;
int nusers;
int usrid;
int gun;
{
	int cind;

	for ( ; nusers; nusers--, users++) {
		fprintf(stdout, " %7d", users->fu_pid);
		fflush(stdout);
		for (cind = 0; cind < sizeof(code_tab) / sizeof(struct co_tab);
		    cind++) {
			if (users->fu_flags & code_tab[cind].c_flag) {
				fprintf(stderr, "%c", code_tab[cind].c_char);
			}
		}
		if (usrid) {
			/*
			 * print the login name for the process
			 */
			struct passwd *getpwuid(), *pwdp;
			if ((pwdp = getpwuid(users->fu_uid)) != NULL) {
			 	fprintf(stderr, "(%s)", pwdp->pw_name);
			}
		}
		if (gun) {
			(void)kill(users->fu_pid, 9);
		}
	}
}

/*
 * Procedure:     main
 *
 * Restrictions:
                 fprintf: none 
                 fflush: none 
								 utssys(2): none
                 perror: none
*/
/*
 * Determine which processes are using a named file or file system.
 * On stdout, show the pid of each process using each command line file
 * with indication(s) of its use(s).  Optionally display the login
 * name with each process.  Also optionally, issue a kill to each process.
 *
 * When any error condition is encountered, possibly after partially
 * completing, fuser exits with status 1.  If no errors are encountered,
 * exits with status 0.
 *
 * The preferred use of the command is with a single file or file system.
 */

main(argc, argv)
int argc;
char **argv;
{
	int gun = 0, usrid = 0, contained = 0, file_only = 0;
	int newfile = 0;
	register i, j, k;
	char *mntname;
	int nusers, tmperr;
	f_user_t *users;


	(void)setlocale(LC_ALL,"");
	(void)setcat("uxsysadm");
	(void)setlabel("UX:fuser");

	if (argc < 2) {
		usage();
	}
	if ((users = get_f_user_buf()) == NULL) {
		pfmt(stderr, MM_ERROR,
			":72:could not allocate buffer\n");
		exit(1);
	}
	for (i = 1; i < argc; i++) {
		int okay = 0;

		if (argv[i][0] == '-') {
			/* options processing */
			if (newfile) {
				gun = usrid = contained = file_only =
				    newfile = 0;
			}
			for (j = 1; argv[i][j] != '\0'; j++) {
				switch(argv[i][j]) {
				case 'k':
					if (gun) {
						usage();
					}
					gun = 1;
					break;
				case 'u':
					if (usrid) {
						usage();
					}
					usrid = 1;
					break;
				case 'c':
					if (contained) {
						usage();
					}
					if (file_only) {
						pfmt(stderr,MM_ERROR, 
						    ":73:'c' and 'f' can't both be used for a file\n");

						usage();
					}
					contained = 1;
					break;
				case 'f':
					if (file_only) {
						usage();
					}
					if (contained) {
						pfmt(stderr,MM_ERROR, 
						    ":73:'c' and 'f' can't both be used for a file\n");

						usage();
					}
					file_only = 1;
					break;
				default:
					pfmt(stderr,MM_ERROR,
					    ":74:Illegal option %c.\n",
					    	argv[i][j]);
					usage();
				}
			}
			continue;
		} else {
			newfile = 1;
		}

		/*
* if not file_only, attempt to translate special name to mount point, via
* /etc/mnttab.  issue: utssys -c mount point if found, and utssys special, too?
* take union of results for report?????
*/
		fflush(stdout);

		/* First print file name on stderr (so stdout (pids) can
		 * be piped to kill) */
		fprintf(stderr, "%s: ", argv[i]);

		if (!(file_only || contained) && (mntname =
		    spec_to_mount(argv[i])) != NULL) {
			if ((nusers = utssys(mntname, F_CONTAINED, UTS_FUSERS,
			    users)) != -1) {
				report(users, nusers, usrid, gun);
				okay = 1;
			}
		}
		nusers = utssys(argv[i], contained ? F_CONTAINED : 0,
		    UTS_FUSERS, users); 
		tmperr=errno;
		if (nusers == -1) {
			if (!okay) {
				errno=tmperr;
				perror("fuser");
				exit(1);
			}
		} else {
			report(users, nusers, usrid, gun);
		}
		fprintf(stderr,"\n");
	}

	/* newfile is set when a file is found.  if it isn't set here,
	 * then the user did not use correct syntax  */
	if (!newfile) {
		pfmt(stderr, MM_ERROR, ":75:missing file name\n");
		usage();
	}
	exit(0);
}
