#ident	"@(#)chmod:chmod.c	1.9.4.3"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
*/

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/***************************************************************************
 * Command: chmod
 * Inheritable Privileges: P_OWNER,P_MACREAD,P_MACWRITE,P_DACREAD
 *       Fixed Privileges: None
 * Notes: changes the mode of file/directory.
 *
 ***************************************************************************/

/*
 * chmod option mode files
 * where 
 *	mode is [ugoa]{+|-|=}[rwxXlstugo] or an octal number
 *	option is -R
 */

/*
 *  Note that many convolutions are necessary
 *  due to the re-use of bits between locking
 *  and setgid
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/dir.h>
#include <priv.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>	/* for isdigit */
#include <unistd.h>	/* for chdir, etc... */ 
#include <stdlib.h>	/* for getopt, getcwd, etc... */ 

#define	USER	05700	/* user's bits */
#define	GROUP	02070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	07777	/* all */
#define NONE	00000	/* none */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */
#define	SETID	06000	/* set[ug]id */
#define	LOCK	02000	/* lock permit */
#define	STICKY	01000	/* sticky bit */

char	*ms;
char *msp;
int rflag;
static mode_t  mask;
static mode_t newmode(), octmode(), who();
static int what();
int chmodr();
int dochmod();
static void errmsg();
static char posix_var[] = "POSIX2";
static int posix;


extern int	optind;

static const char badstat[] = ":5:Cannot access %s: %s\n";
static const char badchmod[] = ":13:chmod() failed on %s: %s\n";
static const char errfmt[] = ":12:%s: %s\n";
static const char badmode[] = ":14:Invalid mode\n";
static const char Usage[] =
	":1152:Usage:\n\t\t chmod [-R] mode file ... \n\t\t chmod [-R] [ugoa]{+|-|=}[rwxXlstugo] file ...\n";

/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: 	none
                 pfmt: 		none
*/
main(argc, argv)
int argc;
char **argv;
{
	register int i;
	register int c;
	int status = 0;
	int errflg = 0;
	int done = 0;


	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:chmod");
 
	if (getenv(posix_var) != 0) {
		posix = 1;
	} else {
		posix = 0;
	}
	
	/* P2/D11 4.7.3, 4.7.10 */
	/*
	 * getopt doesn't work well here, since -r, etc. are not options,
	 * but arguments. Sigh...
	 */

	argv++;					/* skip the command name */
	argc--;
	while (!done && argc) {
		if (argv[0][0] != '-') {
			break;
		}
		switch(argv[0][1]) {
			case 'R':
				if (argv[0][2] != '\0') {
					done = 1;
					errflg++;
					pfmt(stderr, MM_ERROR,
					"uxlibc:1:Illegal option -- %c\n",
						argv[0][2]);
					break;
				}
				rflag++;
				argc--;
				argv++;
				break;
			case 'r':
			case 'w':
			case 'x':
			case 'X':
			case 's':
			case 't':
			case 'l':
			case 'u':
			case 'g':
			case 'o':
				done = 1;
				break;
			case '-':
				argc--;
				argv++;
				done = 1;
				break;
			default:
				done = 1;
				errflg++;
				pfmt(stderr, MM_ERROR,
					"uxlibc:1:Illegal option -- %c\n",
					argv[0][1]);
				break;
		}
	}

	/*
	 * Check for sufficient arguments
	 * or a usage error.
	 */

	if (argc < 2 || errflg){
		if (errflg == 0)
			pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, Usage) ;
		exit(2) ;
	}

	if (posix) {
		mask = umask(0) ;
	} else {
		mask = NONE;		/* for non-POSIX stuff, no mask */
	}
	ms = argv[0];
	for (i = 1; i < argc; i++)
		status += dochmod(argv[i]);

	exit(status);

}

/*
 * Procedure:     dochmod
 *
 * Restrictions:
                 lstat(2): 	none
                 stat(2): 	none
                 chmod(2): 	none
*/
int
dochmod(name)
	char *name;
{
	static struct stat st;
	int errmode;


	if (lstat(name, &st) < 0) {
		errmsg(MM_WARNING, 0, badstat, name, strerror(errno));
		return 1;
	}

	if (rflag && (st.st_mode&S_IFMT) == S_IFDIR)
		return chmodr(name, st.st_mode);


	if ((st.st_mode & S_IFMT) == S_IFLNK) {
		if (stat(name, &st) < 0) {
			errmsg(MM_WARNING, 0, badstat, name, strerror(errno));
			return 1;
		}
	}

	if (chmod(name, newmode(st.st_mode, name, &errmode)) == -1) {
		errmsg(MM_WARNING, 0, badchmod, name, strerror(errno));
		return 1;
	}
	if (errmode)
		return 1;

	return 0;
}

/*
 * Procedure:     chmodr
 *
 * Restrictions:
                 getcwd: 	none
                 chmod(2): 	none
                 chdir(2): 	none
                 opendir: 	none
*/
chmodr(dir, mode)
	char *dir;
	mode_t  mode;
{
	register DIR *dirp;
	register struct dirent *dp;
	struct stat st;
	char savedir[1024];
	int ecode, errmode;


	if (getcwd(savedir, 1024) == 0) { 
		errmsg(MM_ERROR, 255, 
			":129:Cannot determine current directory");
	}

	/*
	 * Change what we are given before doing it's contents
	 */
	if (chmod(dir, newmode(mode, dir, &errmode)) < 0) {
		errmsg(MM_WARNING, 0, badchmod, dir, strerror(errno));
		return (1);
	}

	if (chdir(dir) < 0) {
		errmsg(MM_WARNING, 0, errfmt, dir, strerror(errno));
		return (1);
	}
	if ((dirp = opendir(".")) == NULL) {
		errmsg(MM_WARNING, 0, errfmt, dir, strerror(errno));
		return (1);
	}
	dp = readdir(dirp);
	dp = readdir(dirp); /* read "." and ".." */
	ecode = 0;
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
		ecode += dochmod(dp->d_name);
	(void) closedir(dirp);
	if (chdir(savedir) < 0) {
		errmsg(MM_ERROR, 255, ":11:Cannot change back to %s: %s\n",
			savedir, strerror(errno));
	}
	return (ecode);
}



static mode_t
newmode(nm, file, errmode)
mode_t  nm;
char  *file;
int *errmode;
{
	/* m contains USER|GROUP|OTHER information
	   o contains +|-|= information
	   b contains rwx(slt) information  */
	mode_t m, b;
	mode_t m_lost ;
	register int o;
	register int lcheck, scheck, xcheck, goon;
	mode_t om = nm;	/* save mode for file mode incompatibility return */

	*errmode = 0;
	msp = ms;
	if (isdigit(*msp))
		return(octmode(om));
	do {
		m = who();
		/* P2D11/4.7.7*/
		/* if who is not specified */
		if (m == 0) {
			m = ALL & ~mask;
			/*
		 	 * The m_lost is supplementing umask bits
			 * for '=' operation.
			 */
			m_lost = mask ;
		} else 
			m_lost = 0 ;

		while (o = what()) {
/*
	this section processes permissions
*/
			b = 0;
			goon = 0;
			lcheck = scheck = xcheck = 0;
			switch (*msp) {
			case 'u':
				b = (nm & USER) >> 6;
				goto dup;
			case 'g':
				b = (nm & GROUP) >> 3;
				goto dup;
			case 'o':
				b = (nm & OTHER);
		    dup:
				b &= (READ|WRITE|EXEC);
				b |= (b << 3) | (b << 6);
				msp++;
				goon = 1;
			}
			while (goon == 0) switch (*msp++) {
			case 'r':
				b |= READ;
				continue;
			case 'w':
				b |= WRITE;
				continue;
			case 'X':
				if(!S_ISDIR(om) && ((om & EXEC) == 0))
					continue;
				/* FALLTHRU */
			case 'x':
				b |= EXEC;
				xcheck = 1;
				continue;
			case 'l':
				b |= LOCK;
				m |= LOCK;
				lcheck = 1;
				continue;
			case 's':
				b |= SETID;
				scheck = 1;
				continue;
			case 't':
				b |= STICKY;
				continue;
			default:
				msp--;
				goon = 1;
			}

			b &= m;

			switch (o) {
			case '+':

				/* is group execution requested? */
				if ( xcheck == 1 && (b & GROUP & EXEC) == (GROUP & EXEC)) {

					/* not locking, too! */
					if ( lcheck == 1 ) {
						errmsg(MM_ERROR, 3, ":17:Group execution and locking not permitted together\n");
					}

					/* not if the file is already lockable */
					if ( (nm & GROUP & (LOCK | EXEC)) == LOCK ) {
						errmsg(MM_WARNING, 0, ":18:Group execution not permitted\n\ton %s, a lockable file\n", file);
						*errmode = 1;
						return(om);
					}
				}

				/* is setgid on execution requested? */
				if ( scheck == 1 && (b & GROUP & SETID) == (GROUP & SETID) ) {

					/* not locking, too! */
					if ( lcheck == 1 && (b & GROUP & EXEC) == (GROUP & EXEC) ) {
						errmsg(MM_ERROR, 4, ":19:Set-group-ID and locking not permitted together\n");
					}

					/* not if the file is already lockable */
					if ( (nm & GROUP & (LOCK | EXEC)) == LOCK ) {
						errmsg(MM_WARNING, 0, ":20:Set-group-ID not permitted\n\ton %s, a lockable file\n", file);
						*errmode = 1;
						return(om);
					}
				}

				/* is setid on execution requested? */
				if ( scheck == 1 ) {

					/* the corresponding execution must be requested or already set */
					if ( ((nm | b) & m & EXEC & (USER | GROUP)) != (m & EXEC & (USER | GROUP)) ) {
						errmsg(MM_WARNING, 0, ":21:Execute permission required for set-ID\n\ton execution for %s\n", file);
						*errmode = 1;
						return(om);
					}
				}

				/* is locking requested? */
				if ( lcheck == 1 ) {

					/* not if the file has group execution set */
					/* NOTE: this also covers files with setgid */
					if ( (nm & GROUP & EXEC) == (GROUP & EXEC) ) {
						errmsg(MM_WARNING, 0, ":22:Locking not permitted\n\ton %s, a group executable file\n", file);
						*errmode = 1;
						return(om);
					}
				}

				/* create new mode */
				nm |= b;
				break;
			case '-':

				/* don't turn off locking, unless it's on */
				if ( lcheck == 1 && scheck == 0 
				     && (nm & GROUP & (LOCK | EXEC)) != LOCK )
					b &= ~LOCK;

				/* don't turn off setgid, unless it's on */
				if ( scheck == 1 && lcheck == 0
				     && (nm & GROUP & (LOCK | EXEC)) == LOCK )
					b &= ~(GROUP & SETID);

				/* if execution is being turned off and the corresponding
				   setid is not, turn setid off, too & warn the user */
				if ( xcheck == 1 && scheck == 0
				     && ((m & GROUP) == GROUP || (m & USER) == USER)
				     && (nm & m & (SETID | EXEC)) == (m & (SETID | EXEC)) ) {
					errmsg(MM_WARNING, 0, ":23:Corresponding set-ID also disabled\n\ton %s since set-ID requires execute permission\n", file);
					if ( (b & USER & SETID) != (USER & SETID)
					     && (nm & USER & (SETID | EXEC)) == (m & USER & (SETID | EXEC)) )
						b |= USER & SETID;
					if ( (b & GROUP & SETID) != (GROUP & SETID)
					     && (nm & GROUP & (SETID | EXEC)) == (m & GROUP & (SETID | EXEC)) )
						b |= GROUP & SETID;
				}

				/* create new mode */
				nm &= ~b;
				break;
			case '=':

				/* is locking requested? */
				if ( lcheck == 1 ) {

					/* not group execution, too! */
					if ( (b & GROUP & EXEC) == (GROUP & EXEC) ) {
						errmsg(MM_ERROR, 3, ":17:Group execution and locking not permitted together\n");
					}

					/* if the file has group execution set, turn it off! */
					if ( (m & GROUP) != GROUP ) {
						nm &= ~(GROUP & EXEC);
					}
				}

/* is setid on execution requested?
   the corresponding execution must be requested, too! */
				if (scheck == 1 
				  && (b & EXEC & (USER | GROUP))  
				    != (m & EXEC & (USER | GROUP)) ) {
					errmsg(MM_ERROR, 2, ":24:Execute permission required for set-ID on execution\n");
				}

/* 
 * The ISGID bit on directories will not be changed when 
 * the mode argument is a string with "=". 
 */

				if ((om & S_IFMT) == S_IFDIR)
					b = (b & ~S_ISGID) | (om & S_ISGID);

				/* create new mode */
				nm &= ~(m|m_lost);
				nm |= b;
				break;
			}
		}
	} while (*msp++ == ',');
	if (*--msp) {
		errmsg(MM_ERROR, 5, badmode);
	}
	return(nm);
}

static mode_t
octmode(mode)
	mode_t mode;
{
	char *badchr;
	mode_t i;

	i = (mode_t)strtol(msp, &badchr, 8) ;
	if (*badchr != '\0')
		errmsg(MM_ERROR, 6, badmode);

/* The ISGID bit on directories will not be changed when the mode argument is
 * octal numeric. Only "g+s" and "g-s" arguments can change ISGID bit when
 * applied to directories.
 */
	if ((mode & S_IFMT) == S_IFDIR)
		return((i & ~S_ISGID) | (mode & S_ISGID));
	return(i);
}

static mode_t
who()
{
	register mode_t m;

	m = 0;
	for (;; msp++) switch (*msp) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		return m;
	}
}

static int
what()
{
	switch (*msp) {
	case '+':
	case '-':
	case '=':
		return *msp++;
	}
	return(0);
}

/*
 * Procedure:     errmsg
 *
 * Restrictions:
                 pfmt: 	none
*/
/*VARARGS*/ 
static void
errmsg(severity, code, format, a1, a2, a3)
long severity, code;
char *format, *a1, *a2, *a3;
{
	(void) pfmt(stderr, severity, format, a1, a2, a3);
	if (code != 0)
		exit(code);
	return;
}
