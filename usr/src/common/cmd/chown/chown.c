/*		copyright	"%c%" 	*/

#ident	"@(#)chown:chown.c	1.12.3.1"

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

/*
 * chown [-hR] uid file ...
 */

/***************************************************************************
 * Command: chown
 * Inheritable Privileges: P_OWNER,P_MACREAD,P_MACWRITE,P_DACREAD
 *       Fixed Privileges: None
 * Notes: changes the owner of the files to owner
 *
 ***************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <priv.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

struct	passwd	*pwd;
struct	group	*grp;
struct	stat	stbuf;
uid_t	uid;
gid_t	gid;
char	*owner;
char	*group;
int	status;
int	hflag, rflag = 0;

#define GROUP_SIGN	':'

static const char usage[] =
	":1153:Usage: chown [-h] [-R] owner[:group] file ...\n";

/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: 	none
                 getopt: 	none
                 pfmt:		none
                 getpwnam: 	none
                 lstat(2): 	none
                 stat(2): 	none
                 chown(2): 	none
                 lchown(2): 	none
*/
main(argc, argv)
int argc;
char *argv[];
{
	register c;
#ifdef __STDC__
	uid_t atoxid(char *, int);
#else
	uid_t atoxid();
#endif
	int ch;
	extern int optind;
	int errflg = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:chown");


	while((ch = getopt(argc, argv, "hR")) != EOF)
	switch(ch) {
		case 'h' :
			hflag++;
			break;
		case 'R' :
			rflag++;
			break;
		default :
			errflg++;
			break;
	}

        /*
         * Check for sufficient arguments
         * or a usage error.
         */

        argc -= optind;
        argv = &argv[optind];

        if(errflg || argc < 2) {
        	if (argc < 2)
        		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
                pfmt(stderr, MM_ACTION, usage);
                exit(4);
        }

	if ((owner = strdup(argv[0])) == NULL) {
		(void) Perror("strdup");
		exit(255);
	}

	group = strchr(owner,GROUP_SIGN) ;
	if (group != NULL) {
		*group = '\0' ;
		group++ ;
		if (*group == '\0') {
        		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
                	pfmt(stderr, MM_ACTION, usage );
                	exit(4);
		} else if ((grp = getgrnam(group)) == NULL) {
			if(isnumber(group)) {
				gid = (gid_t) atoxid(group,1);
			} else {
				pfmt(stderr, MM_ERROR,
					":1154:Unknown group id: %s\n",group);
				exit(4);
			}
		} else {
			gid = grp->gr_gid;
		}
	} else {
		gid = (gid_t) -1;
	}
	if (*owner == '\0' ) {
       		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
               	pfmt(stderr, MM_ACTION, usage );
               	exit(4);
	}
	if ((pwd=getpwnam(owner)) == NULL) {
		if(isnumber(owner)) {
			uid = atoxid(owner,0);
		} else {
			pfmt(stderr, MM_ERROR,
				":1116:Unknown user: %s\n", owner);
			exit(4);
		}
	} else {
		uid = pwd->pw_uid;
	}


	/*
	 * For each file, find its statistics.  If it is a symbolic
	 * link and the hflag is set, stat the link.  Otherwise,
	 * stat the referenced file.  Then, call chownr(), lchown(),
	 * or chown() accordingly.
	 */
	for(c=1; c<argc; c++) {
		if (hflag) {
			if (lstat(argv[c], &stbuf) < 0) {
				status += Perror(argv[c]);
				continue;
			}
		}
		else {
			if (stat(argv[c], &stbuf) < 0) {
				status += Perror(argv[c]);
				continue;
			}
		}

		if (rflag && ((stbuf.st_mode & S_IFMT) == S_IFDIR)) {
                        status += chownr(argv[c], uid, gid);
		}
		else if (hflag) {
			if(lchown(argv[c], uid, gid) < 0)
				status = ChownPerror(argv[c]);
		}
		else {
			if(chown(argv[c], uid, gid) < 0)
				status = ChownPerror(argv[c]);
		}
	}
	exit(status);
	/* NOTREACHED */
}

/*
 * Procedure:     chownr
 *
 * Restrictions:
                 getcwd: 	none
                 chdir(2): 	none
                 opendir: 	none
                 lstat(2): 	none
                 stat(2): 	none
                 chown(2): 	none
                 lchown(2): 	none
                 pfmt: 		none
                 strerror: 	none
*/
chownr(dir, uid, gid)
char *dir;
uid_t uid;
gid_t gid;
{
        register DIR *dirp;
        register struct dirent *dp;
        struct stat st;
        char savedir[PATH_MAX];
        extern char *getcwd();


        if (getcwd(savedir, PATH_MAX) == (char *)0) {
                Perror("getcwd");
                exit(255);
        }

        if (chdir(dir) < 0) 
                return(Perror(dir));

        if ((dirp = opendir(".")) == NULL)
                return(Perror(dir));

        dp = readdir(dirp);
        dp = readdir(dirp); /* read "." and ".." */
        for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (hflag) {
			if (lstat(dp->d_name, &st) < 0) {
				status += Perror(dp->d_name);
				continue;
			}
		}
		else {
			if (stat(dp->d_name, &st) < 0) {
				status += Perror(dp->d_name);
				continue;
			}
		}

		if ((st.st_mode & S_IFMT) == S_IFDIR) {
                        status += chownr(dp->d_name, uid, gid);
		}
		else if (hflag) {
			if(lchown(dp->d_name, uid, gid) < 0)
				status = ChownPerror(dp->d_name);
		}
		else {
			if(chown(dp->d_name, uid, gid) < 0)
				status = ChownPerror(dp->d_name);
		}
        }
        closedir(dirp);
        if (chdir(savedir) < 0) {
                pfmt(stderr, MM_ERROR, ":11:Cannot change back to %s: %s\n", 
                	savedir, strerror(errno));
                exit(255);
        }

        /*
         * Change what we are given after doing its contents.
         */
        if (chown(dir, uid, gid) < 0)
		return (ChownPerror(dir));

        return (0);
}

isnumber(s)
char *s;
{
	register c;

	while(c = *s++)
		if(!isdigit(c))
			return(0);
	return(1);
}

/*
 * Procedure:     atoxid
 *
 * Restrictions:
                 pfmt:	none
*/
uid_t
atoxid(s,flag)
register char *s;
register int flag;
{
	register uid_t i ;

	i = (uid_t) strtol(s,NULL,10) ;
	if(i > UID_MAX ) {
		if(flag)
			pfmt(stderr, MM_ERROR,
				":1151:Numeric group id too large\n");
		else
			pfmt(stderr, MM_ERROR,
				":27:Numeric user id too large\n");
		exit(4);
   	}
	return (i) ;
}

/*
 * Procedure:     Perror
 *
 * Restrictions:
 *               pfmt: 		none
 *               strerror: 	none
 */
Perror(s)
char *s;
{
	pfmt(stderr, MM_ERROR, ":12:%s: %s\n", s, strerror(errno));
        return(1);
}

/*
 * Procedure:     ChownPerror
 *
 * Note: This routine specifically prints out error messages for
 *	 chown(2) or lchown(2).  Depending on the error, the 
 *	 routine either returns or exits.
 *
 * Restrictions:
 *               pfmt: 		none
 *               strerror: 	none
 */
ChownPerror(s)
char *s;
{
	/*
	 * EINVAL indicates that the numeric id given is out of range.
	 * Print the id and error message.  No use repeating
	 * error for each call to chown() and thus, exit now.
	 */
	if (errno == EINVAL) {
		pfmt(stderr, MM_ERROR, ":12:%s: %s\n", owner, strerror(errno));
		exit(1);
	}
	pfmt(stderr, MM_ERROR, ":12:%s: %s\n", s, strerror(errno));
	return(1);
}
