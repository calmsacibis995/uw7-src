/*	copyright	"%c%"	*/

/*      Portions Copyright (c) 1988, Sun Microsystems, Inc.     */
/*      All Rights Reserved.                                    */

#ident	"@(#)id:id.c	1.8.3.2"

#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/param.h>
#include <pfmt.h>
#include <locale.h>
#include <sys/types.h>	/* getuid(), geteuid(), getgid(), getegid() */
#include <limits.h>	/* LOGNAME_MAX */
#include <unistd.h>	/* getuid(), geteuid(), getgid(), getegid() */
#include <errno.h>	/* errno */
#include <stdlib.h>	/* getopt() */
#include <string.h>	/* strerror(), strncmp(), strncpy() */

static void	pgroups_posix(char *);
static void	all_gids(char *, gid_t, gid_t);
static void	all_gnames(char *, char *, char *);
static void	prt_usage(int);

static int	userid_wflag = 0;
static int	groupid_wflag = 0;

static char	uid_label[] =
	":1169";
static char	uid_dftfmt[] =
	"uid";
static char	gid_label[] =
	":1170";
static char	gid_dftfmt[] =
	" gid";
static char	euid_label[] =
	":1171";
static char	euid_dftfmt[] =
	" euid";
static char	egid_label[] =
	":1172";
static char	egid_dftfmt[] =
	" egid";
static char	no_user[] =
	":1173:%s : No such user\n";
static char	notfound_uid[] =
	":1174:userid not found in user database\n";
static char	notfound_gid[] =
	":1175:groupid not found in group database\n";
static char	usage[] =
	":1176:Usage:\n"
	"\tid [-a]\n"
	"\tid user\n"
	"\tid -G [-n] [user]\n"
	"\tid -g [-nr] [user]\n"
	"\tid -u [-nr] [user]\n";

static char	incr_usage[] =
	":8:Incorrect usage\n";
static char	groups_title[] =
	":1014: groups=";

static char	name_fmt[]	= "%s";
static char	name_spcfmt[]	= " %s";
static char	name_nlfmt[]	= "%s\n";
static char	name_parfmt[]	= "(%s)";
static char	id_fmt[]	= "%u";
static char	id_spcfmt[]	= " %u";
static char	id_nlfmt[]	= "%u\n";
static char	id_name_fmt[]	= "%s=%u";

main(argc, argv)
int argc;
char **argv;
{
	uid_t uid, euid;
	gid_t gid, egid;
	static char stdbuf[BUFSIZ];
	int c, aflag=0;
	int Gflag=0;
	int gflag=0;
	int nflag=0;
	int rflag=0;
	int uflag=0;
	struct passwd *pwdp = (struct passwd *)NULL;
	struct group *grpp = (struct group *)NULL;
	char username[LOGNAME_MAX + 1];
	char groupname[LOGNAME_MAX + 1];
	char egroupname[LOGNAME_MAX + 1];

	setlocale(LC_ALL, "");
	setcat("uxcore.abi");
	setlabel("UX:id");
	
	while ((c = getopt(argc, argv, "Gagnru")) != EOF) {
		switch(c) {
			case 'G':
				if (aflag || gflag || rflag || uflag) {
					prt_usage(1);
				}
				Gflag++;
				break;
			case 'a': 
				if (Gflag || gflag || nflag
					  || rflag || uflag) {
					prt_usage(1);
				}
				aflag++;
				break;
			case 'g':
				if (Gflag || aflag || uflag) {
					prt_usage(1);
				}
				gflag++;
				break;
			case 'n':
				if (aflag) {
					prt_usage(1);
				}
				nflag++;
				break;
			case 'r':
				if (Gflag || aflag) {
					prt_usage(1);
				}
				rflag++;
				break;
			case 'u':
				if (Gflag || aflag || gflag) {
					prt_usage(1);
				}
				uflag++;
				break;
			default: 
				prt_usage(0);
				break;
		}
	}
	if (!Gflag && !gflag && !uflag && (nflag || rflag)) {
		prt_usage(1);
	}

	setbuf (stdout, stdbuf);

	if (optind < argc) {
		if ((optind + 1) != argc) { /* too many arguments */
			prt_usage(1);
		}
		if (aflag) {
			prt_usage(1);
		}
		setpwent();
		if ((pwdp = getpwnam(argv[optind]))
				== (struct passwd *)NULL) {
			pfmt(stderr, MM_ERROR, no_user, argv[optind]);
			exit(1);
		}
		uid = euid = pwdp->pw_uid;
		gid = egid = pwdp->pw_gid;
	} else {
		uid = getuid();
		gid = getgid();
		euid = geteuid();
		egid = getegid();
	}

	username[LOGNAME_MAX] = groupname[LOGNAME_MAX]
		= egroupname[LOGNAME_MAX] = '\0';

	if (Gflag) {
		setpwent();
		if ((pwdp = getpwuid(uid)) != (struct passwd *)NULL) {
			(void)strncpy(username, pwdp->pw_name,
				(size_t)LOGNAME_MAX);
		} else {
			username[0] = '\0';
			userid_wflag++;
		}

		if (nflag) {
			setgrent();
			if ((grpp = getgrgid(gid))
					!= (struct group *)NULL) {
				(void)strncpy(groupname, grpp->gr_name,
					(size_t)LOGNAME_MAX);
			} else {
				groupname[0] = '\0';
				groupid_wflag++;
			}

			setgrent();
			if ((grpp = getgrgid(egid))
					!= (struct group *)NULL) {
				(void)strncpy(egroupname, grpp->gr_name,
					(size_t)LOGNAME_MAX);
			} else {
				egroupname[0] = '\0';
				groupid_wflag++;
			}

			if ((username[0] != '\0') &&
			    (groupname[0] != '\0') &&
			    (egroupname[0] != '\0')) {
				all_gnames(username, groupname,
					egroupname);
			}
		} else {
			if (username[0] != '\0') {
				all_gids(username, gid, egid);
			}
		}
	} else if (gflag) {
		if (nflag) {
			setgrent();
			grpp = rflag ? getgrgid(gid) : getgrgid(egid);
			if (grpp == (struct group *)NULL) {
				groupid_wflag++;
				printf(id_nlfmt,
					(rflag
					? (unsigned)gid
					: (unsigned)egid));
			} else {
				printf(name_nlfmt, grpp->gr_name);
			}
		} else {
			if (rflag) {
				printf(id_nlfmt, (unsigned)gid);
			} else {
				printf(id_nlfmt, (unsigned)egid);
			}
		}
	} else if (uflag) {
		if (nflag) {
			setpwent();
			pwdp = rflag ? getpwuid(uid) : getpwuid(euid);
			if (pwdp == (struct passwd *)NULL) {
				userid_wflag++;
				printf(id_nlfmt,
					(rflag
					? (unsigned)uid
					: (unsigned)euid));
			} else {
				printf(name_nlfmt, pwdp->pw_name);
			}
		} else {
			if (rflag) {
				printf(id_nlfmt, (unsigned)uid);
			} else {
				printf(id_nlfmt, (unsigned)euid);
			}
		}
	} else {
		puid (gettxt(uid_label, uid_dftfmt), uid);
		pgid (gettxt(gid_label, gid_dftfmt), gid);
		if (uid != euid)
			puid (gettxt(euid_label, euid_dftfmt), euid);
		if (gid != egid)
			pgid (gettxt(egid_label, egid_dftfmt), egid);

		if (aflag)
			pgroups ();
		else {
			setpwent();
			pwdp = getpwuid(uid);
			if (pwdp != (struct passwd *)NULL) {
				(void)strncpy(username, pwdp->pw_name,
					(size_t)LOGNAME_MAX);
				pgroups_posix(username);
			}
		}
		putchar ('\n');
	}

	if (userid_wflag) {
		pfmt(stderr, MM_WARNING, notfound_uid);
	}
	if (groupid_wflag) {
		pfmt(stderr, MM_WARNING, notfound_gid);
	}

	exit(0);
}

puid (s, id)
	char *s;
	uid_t id;
{
	struct passwd *pw;

	printf(id_name_fmt, s, (unsigned)id);
	setpwent();
	pw = getpwuid(id);
	if (pw)
		printf (name_parfmt, pw->pw_name);
	else
		userid_wflag++;
}

pgid (s, id)
	char *s;
	gid_t id;
{
	struct group *gr;

	printf(id_name_fmt, s, (unsigned)id);
	setgrent();
	gr = getgrgid(id);
	if (gr)
		printf (name_parfmt, gr->gr_name);
	else
		groupid_wflag++;
}

pgroups ()
{
	gid_t groupids[NGROUPS_UMAX];
	gid_t *idp;
	struct group *gr;
	int i;

	i = getgroups(NGROUPS_UMAX, groupids);
	if (i > 0) {
		pfmt (stdout, MM_NOSTD, groups_title);
		for (idp = groupids; i--; idp++) {
			printf (id_fmt, (unsigned)(*idp));
			setgrent();
			gr = getgrgid(*idp);
			if (gr)
				printf (name_parfmt, gr->gr_name);
			if (i)
				putchar (',');
		}
	}
}

static void
pgroups_posix(name)
	char		*name;
{
	struct group	*grpp;
	char		**cp;
	int		ngroups = 1;
	int		first = 1;

	setgrent();
	while ((grpp = getgrent()) != (struct group *)NULL) {
		for (cp = grpp->gr_mem; ngroups < NGROUPS_UMAX &&
				cp && *cp;
				cp++) {
			if (strncmp(*cp, name, (size_t)LOGNAME_MAX)
							== 0) {
				if (first) {
					pfmt (stdout, MM_NOSTD,
						groups_title);
					first = 0;
				} else {
					putchar (',');
				}
				printf(id_fmt, (unsigned)grpp->gr_gid);
				printf(name_parfmt,
					(unsigned)grpp->gr_name);
				ngroups++;
				break;
			}
		}
	}
}

static void
all_gids(name, gid, egid)
	char	*name;
	gid_t	gid;
	gid_t	egid;
{
	struct group	*grpp;
	char		**cp;
	int		ngroups = 1;

	printf(id_fmt, (unsigned)gid);
	if (gid != egid) {
		printf(id_spcfmt, (unsigned)egid);
	}

	setgrent();
	while ((grpp = getgrent()) != (struct group *)NULL) {
		for (cp = grpp->gr_mem; ngroups < NGROUPS_UMAX &&
				cp && *cp;
				cp++) {
			if ((strncmp(*cp, name, (size_t)LOGNAME_MAX)
				    == 0) &&
				    (grpp->gr_gid != gid) &&
				    (grpp->gr_gid != egid)) {
				printf(id_spcfmt,
					(unsigned)grpp->gr_gid);
				ngroups++;
				break;
			}
		}
	}
	putchar('\n');
}

static void
all_gnames(username, groupname, egroupname)
	char		*username;
	char		*groupname;
	char		*egroupname;
{
	struct group	*grpp;
	char		**cp;
	int		ngroups = 1;

	printf(name_fmt, groupname);
	if (strncmp(groupname, egroupname, (size_t)LOGNAME_MAX) != 0) {
		printf(name_spcfmt, egroupname);
	}

	setgrent();
	while ((grpp = getgrent()) != (struct group *)NULL) {
		for (cp = grpp->gr_mem; ngroups < NGROUPS_UMAX &&
				cp && *cp;
				cp++) {
			if ((strncmp(*cp, username,
				(size_t)LOGNAME_MAX) == 0) &&
				(strncmp(grpp->gr_name, groupname,
				(size_t)LOGNAME_MAX) != 0) &&
				(strncmp(grpp->gr_name, egroupname,
					(size_t)LOGNAME_MAX) != 0)) {
				printf(name_spcfmt,
					(unsigned)grpp->gr_name);
				ngroups++;
				break;
			}
		}
	}
	putchar('\n');
}

static void
prt_usage(complain)
	int	complain;
{
	if (complain) {
		pfmt(stderr, MM_ERROR, incr_usage);
	}
	pfmt(stderr, MM_ACTION, usage);
	exit(1);
}

