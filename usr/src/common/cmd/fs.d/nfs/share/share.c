#ident	"@(#)share.c	1.2"
#ident	"$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * nfs share
 */
#include <stdio.h>
#include <pwd.h>	/* used for anon stuff */
#include <string.h>
#include <varargs.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>	/* for UID_NOBODY */
#include <sys/stat.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <nfs/export.h>
#include <netconfig.h>
#include <sys/mac.h>	/* for MAC_GET */
#include <locale.h>
#include <pfmt.h>
#include <netdir.h>
#include "sharetab.h"

/*
 * exit values 
 */
#define RET_OK		0
#define RET_ERR		33	/* usage error */
#define RET_ADD		34	/* could not add to sharetab */
#define RET_DEL		35	/* could not delete from sharetab */
#define RET_EXP		36	/* could not share */
#define RET_PAROP	37	/* error in options parsing */

void usage();
void lidload();
int set_addrmask();

extern int issubdir();
struct export ex;

main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	extern char *optarg;
	char *dir;
	char *res = "-";
	char *opts="rw";
	char *descr="";
	int c;
	level_t tmp_lid;

 	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:nfs share");

	while ((c = getopt(argc, argv, "o:d:")) != EOF) {
		switch (c) {
		case 'o':
			opts = optarg;
			break;
		case 'd':
			descr = optarg;
			break;
		default:
			usage();
			exit (RET_ERR);
		}
	}

	if (argc <= optind || argc - optind > 2) {
		usage();
		exit (RET_ERR);
	}
	dir = argv[optind];

	if (argc - optind > 1)
		res = argv[optind + 1];

	switch (shareable(dir)) {
	case 0:
		exit (RET_ERR);
	case 1:
		break;
	case 2:
		if (sharetab_del(dir) < 0)
			exit (RET_DEL);
		break;
	}

	if (parseopts(&ex, opts) < 0)
		exit (RET_PAROP);

	if ((lvlproc(MAC_GET, &tmp_lid) == 0) || (errno != ENOPKG))
		lidload();

	if (exportfs(dir, &ex) < 0) {
		pfmt(stdout, MM_ERROR, 
		     ":33:%s failed for %s: %s\n", 
		     "exportfs", dir, strerror(errno));
		exit (RET_EXP);
	}

	if (sharetab_add(dir, res, opts, descr) < 0)
		exit (RET_ADD);

	exit (RET_OK);
}

/*
 * Check the nfs share entries in sharetab file.
 * Returns:
 *	0  dir not shareable
 *	1  dir is shareable
 *	2  dir is already shared (can modify options)
 */
int
shareable(path)
	char *path;
{
	FILE *f;
	extern int errno;
	struct share *sh;
	struct stat st;
	int res;

	errno = 0;
	if (*path != '/') {
		pfmt(stderr, MM_ERROR, ":34:%s: is not a full pathname\n", path);
		return (0);
	}
	if (stat(path, &st) < 0) {	/* does it exist ? */
		pfmt(stderr, MM_ERROR, 
		     ":23:%s: cannot %s %s: %s\n",
		     "shareable", "stat", path, strerror(errno));
		return (0);
	}

	f = fopen(SHARETAB, "r");
	if (f == NULL) {
		if (errno == ENOENT)
			return (1);
		pfmt(stderr, MM_ERROR, ":20:%s: cannot open %s: %s\n",
		     "shareable", SHARETAB, strerror(errno));
		return (0);
	}

	while ((res = getshare(f, &sh)) > 0) {
		if (strcmp(sh->sh_fstype, "nfs") != 0)
			continue;

		if (direq(path, sh->sh_path)) {
			(void) fclose(f);
			return (2);
		}

		if (issubdir(sh->sh_path, path)) {
			pfmt(stderr, MM_ERROR,
			     ":35:%s: sub-directory (%s) already shared\n",
			     path, sh->sh_path);
			(void) fclose(f);
			return (0);
		}
		if (issubdir(path, sh->sh_path)) {
			pfmt(stderr, MM_ERROR,
			     ":36:%s: parent-directory (%s) already shared\n",
			     path, sh->sh_path);
			(void) fclose(f);
			return (0);
		}
	}

	if (res < 0) {
		pfmt(stderr, MM_ERROR, ":37:error reading %s\n", SHARETAB);
		(void) fclose(f);
		return (0);
	}
	(void) fclose(f);
	return (1);
}

int
direq(dir1, dir2)
	char *dir1, *dir2;
{
	struct stat st1, st2;

	if (strcmp(dir1, dir2) == 0)
		return (1);
	if (stat(dir1, &st1) < 0 || stat(dir2, &st2) < 0)
		return (0);
	return (st1.st_ino == st2.st_ino && st1.st_dev == st2.st_dev);
}

char *optlist[] = {
#define OPT_RO		0
	SHOPT_RO,
#define OPT_RW		1
	SHOPT_RW,
#define OPT_ROOT	2
	SHOPT_ROOT,
#define OPT_SECURE	3
	SHOPT_SECURE,
#define OPT_ANON	4
	SHOPT_ANON,
#define OPT_WINDOW	5
	SHOPT_WINDOW,
#define OPT_WRASYNC	6
	SHOPT_WRASYNC,
	NULL
};

#define badnum(x)				\
	((x) == NULL ||				\
	 (!isdigit(*(x)) &&			\
	  (*(char *)(x) != '-' ||		\
	   !isdigit(*(char *)(x+1)))))

#define RO	0x1	/* ro	*/
#define RW	0x2	/* rw	*/
#define ROL	0x4	/* ro=	*/
#define RWL	0x8	/* rw=	*/

/*
 * Parse the share options from the "-o" flag.
 * The extracted data is moved into the exports
 * structure which is passed into the kernel via
 * the exportfs() system call.
 */
int
parseopts(exp, opts)
	struct export *exp;
	char *opts;
{
	char *p, *savep, *val;
	char *rolist = NULL;
	char *rwlist = NULL;
	char *rootlist = NULL;
	struct passwd *pt;
	struct passwd *getpwnam();
	int des_window;
	int ex_access = 0;
	int r;

	exp->ex_anon = UID_NOBODY;
	exp->ex_auth = AUTH_UNIX;
	des_window = 30000;
	p = strdup(opts);
	if (p == NULL) {
		pfmt(stderr, MM_ERROR, ":3:%s: no memory for %s\n",
		     "parseopts", "p");
		return (-1);
	}

	while (*p) {
		savep = p;
		switch (getsubopt(&p, optlist, &val)) {
		case OPT_RO:
			if (val) {
				ex_access |= ROL;
				if (rolist != NULL) {
					pfmt(stderr, MM_ERROR, 
					     ":38:option %s specified more than once\n",
					     "ro");
					pfmt(stderr, MM_ACTION, 
					     ":39:share -F nfs -o %s=client[:client] ...\n",
					     "ro");
					return (-1);
				}
				rolist = strdup(val);
				if (rolist == NULL) {
					pfmt(stderr, MM_ERROR, 
					     ":3:%s: no memory for %s\n",
					     "parseopts", "rolist");
					return (-1);
				}
			} else {
				ex_access |= RO;
			}
			break;
		case OPT_RW:
			if (val) {
				ex_access |= RWL;
				if (rwlist != NULL) {
					pfmt(stderr, MM_ERROR, 
					     ":38:option %s specified more than once\n",
					     "rw");
					pfmt(stderr, MM_ACTION,
					     ":39:share -F nfs -o %s=client[:client] ...\n",
					     "rw");
					return (-1);
				}
				rwlist = strdup(val);
				if (rwlist == NULL) {
					pfmt(stderr, MM_ERROR,
					     ":3:%s: no memory for %s\n",
					     "parseopts", "rwlist");
					return (-1);
				}
			} else {
				ex_access |= RW;
			}
			break;
		case OPT_ROOT:
			if (val == NULL)
				goto badopt;
			if (rootlist != NULL) {
				pfmt(stderr, MM_ERROR, 
				     ":38:option %s specified more than once\n",
				     "root");
				pfmt(stderr, MM_ACTION,
				     ":39:share -F nfs -o %s=client[:client] ...\n",
				     "root");
				return (-1);
				}
			rootlist = strdup(val);
			if (rootlist == NULL) {
				pfmt(stderr, MM_ERROR,
				     ":3:%s: no memory for %s\n",
				     "parseopts", "rootlist");
				return (-1);
			}
			break;
		case OPT_SECURE:
			exp->ex_auth = AUTH_DES;
			break;
		case OPT_ANON:
			if (!badnum(val))
				exp->ex_anon = atoi(val);
			else {		/* we also allow real users hd */
				pt = getpwnam (val);
				if (pt == NULL)
					goto badopt;
				else
					exp->ex_anon = pt->pw_uid;
			}
			break;
		case OPT_WINDOW:
			if (badnum(val))
				goto badopt;
			des_window = atoi(val);
			break;
		case OPT_WRASYNC:
			exp->ex_flags |= EX_WR_ASYNC;
			break;
		default:
			goto badopt;
		}
	}

	r = 0;
	switch (ex_access) {
	case 0:
		exp->ex_flags |= EX_RDWR;
		break;
	case RO:
	case ROL:
		exp->ex_flags |= EX_RDONLY;
		break;
	case RW:
		exp->ex_flags |= EX_RDWR;
		break;
	case RWL:
		exp->ex_flags |= EX_RDWR | EX_EXCEPTIONS;
		r = getaddrs(&exp->ex_rwaddrs, rwlist, EXMAXADDRS);
		break;
	case RO | RWL:
	case ROL | RWL:
		exp->ex_flags |= EX_RDONLY | EX_EXCEPTIONS;
		r = getaddrs(&exp->ex_rwaddrs, rwlist, EXMAXADDRS);
		break;
	case ROL | RW:
		exp->ex_flags |= EX_RDWR | EX_EXCEPTIONS;
		r = getaddrs(&exp->ex_roaddrs, rolist, EXMAXADDRS);
		break;
	default:
		goto badopt;
	}
	if (r < 0)
		return (-1);

	switch (exp->ex_auth) {
	case AUTH_UNIX:
		if (rootlist) {
			r = getaddrs(&exp->ex_unix.rootaddrs, rootlist,
					EXMAXROOTADDRS);
			if (r < 0) {
				savep = rootlist;
				goto badopt;
			}
		}
		break;
	case AUTH_DES:
		exp->ex_des.window = des_window;
		if (rootlist) {
			if (share_des(&exp->ex_des, rootlist) < 0) {
				savep = rootlist;
				goto badopt;
			}
		}
		break;
	case AUTH_ESV:
		if (rootlist) {
			r = getaddrs(&exp->ex_esv.esvrootaddrs, rootlist,
					EXMAXROOTADDRS);
			if (r < 0) {
				savep = rootlist;
				goto badopt;
			}
		}
		break;
	}

	return (0);

badopt:
	pfmt(stderr, MM_ERROR, ":7:invalid option: %s\n", savep);
	return (-1);
}

/*
 * Convert a colon delimited list of names into
 * a list of network addresses and a count.
 * The list is passed to the kernel in the export
 * structure of the exportfs system call.
 * Lists from "ro=", "rw=" and "root=" are converted here.
 */
int
getaddrs(addrs, ap, maxcnt)
	struct exaddrlist *addrs;
	char *ap;
	int maxcnt;
{
	struct nd_hostserv hs;
	struct nd_addrlist *retaddrs;
	struct netconfig *nconf;
	struct netconfig *getnetpath();
	int i, j;
	int ret = 0;
	int nnames, naddrs;
	char **names = NULL;
	int *namesvalid = NULL;
	NCONF_HANDLE *nc = NULL;
	/* NCONF_HANDLE *setnetconfig(); */

	nnames = parselist(ap, &names, maxcnt);
	if (nnames <= 0)
		return (-1);
	namesvalid = (int *) calloc(nnames, sizeof(int));
	if (namesvalid == NULL) {
		pfmt(stderr, MM_ERROR, ":3:%s: no memory for %s\n",
		     "getaddrs", "namesvalid");
		ret = -1;
		goto done;
	}

	nc = setnetconfig();
	if (nc == NULL) {
		pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n",
		     "setnetconfig", nc_sperror());
		ret = -1;
		goto done;
	}

	addrs->naddrs = 0;
	addrs->addrvec = (struct netbuf *) malloc(maxcnt *
		sizeof(struct netbuf));
	if (addrs->addrvec == NULL) {
		pfmt(stderr, MM_ERROR, ":3:%s: no memory for %s\n",
		     "getaddrs", "addrvec");
		ret = -1;
		goto done;
	}
	addrs->addrmask = (struct netbuf *) malloc(maxcnt *
		sizeof(struct netbuf));
	if (addrs->addrmask == NULL) {
		pfmt(stderr, MM_ERROR, ":3:%s: no memory for %s\n",
		     "getaddrs", "addrmask");
		ret = -1;
		goto done;
	}
	naddrs = 0;

	while (nconf = getnetpath(nc)) {
		if ((nconf->nc_semantics != NC_TPI_CLTS)
		|| ((nconf->nc_flag & NC_VISIBLE) == 0))
			continue;
		hs.h_serv = ""; 
		/* hs.h_serv = NULL; */
		for (i = 0; i < nnames; i++) {
			hs.h_host = names[i];
			if (netdir_getbyname(nconf, &hs, &retaddrs) != ND_OK) {
				continue;
			}

			for (j = 0; j < retaddrs->n_cnt; j++) {
				if (naddrs >= maxcnt) {
					pfmt(stderr, MM_ERROR,
					     ":41:list is too long (maximum %d)\n",
					     maxcnt);
					ret = -1;
					goto done;
				}
				addrs->addrvec [naddrs] = retaddrs->n_addrs[j];
				addrs->addrmask[naddrs] = retaddrs->n_addrs[j];
				if (set_addrmask(nconf, 
				      &addrs->addrmask[naddrs]) < 0) {
				      ret = -1;
				      goto done;
				}
				naddrs++;
				namesvalid[i]++;
			}
		}
	}
	addrs->naddrs = naddrs;

	/* now check that all the names got resolved to addresses */

	for (i = 0 ; i < nnames ; i++) {
		if (namesvalid[i] == 0) {
			pfmt(stderr, MM_ERROR,
			     ":42:%s: network address not known\n", 
			     names[i]);
			ret = -1;
		}
	}

done:
	if (nc)
		endnetpath(nc);
	if (nnames)
		free(names);
	if (namesvalid)
		free(namesvalid);
	if (ret != 0 && addrs->addrvec) {
		free(addrs->addrvec);
		addrs->addrvec = NULL;
	}

	return (ret);
}

#include <netinet/in.h>

/* Create an address mask appropriate for the transport.
 * The mask is used to obtain the host-specific part of
 * a network address when comparing addresses.
 * For an internet address the host-specific part is just
 * the 32 bit IP address and this part of the mask is set
 * to all-ones. The port number part of the mask is zeroes.
 */
int
set_addrmask(nconf, mask)
	struct netconfig *nconf;
	struct netbuf *mask;
{
	mask->buf = (char *)malloc(mask->len);
	if (mask->buf == NULL) {
		pfmt(stderr, MM_ERROR, ":3:%s: no memory for %s\n",
		     "set_addrmask", "buf");
		return (-1);
	}
	memset(mask->buf, 0, mask->len);	/* reset all mask bits */

	if ((strcmp(nconf->nc_protofmly, NC_INET) == 0) &&
	    (strcmp(nconf->nc_proto, NC_UDP) == 0)) {
		((struct sockaddr_in *) mask->buf)->sin_addr.s_addr = ~0;
	} else {
		memset(mask->buf, 0xFF, mask->len);	/* set all mask bits */
	}
	return(0);
}

int
share_des(des, list)
	struct desexport *des;
	char *list;
{
	char netname[MAXNETNAMELEN + 1];
	char **names;
	int cnt, i;

	cnt = parselist(list, &names, EXMAXROOTNAMES);
	if (cnt <= 0)
		return (-1);

	des->rootnames = (char **) malloc(cnt * sizeof(char *));
	if (des->rootnames == NULL) {
		pfmt(stderr, MM_ERROR, ":28:%s: no memory\n", 
		     "share_des");
		free(names);
		return (-1);
	}

	for (i = 0 ; i < cnt ; i++) {
		if (!host2netname(netname, names[i], NULL)) {
			pfmt(stderr, MM_ERROR, ":8:%s failed: %s is not known\n",
			     "host2netname", names[i]);
			continue;
		}

		des->rootnames[i] = strdup(netname);
		if (des->rootnames[i] == NULL) {
			pfmt(stderr, MM_ERROR, ":28:%s: no memory\n", 
			     "share_des");
			free(names);
			while (--i)
				free(des->rootnames[i]);
			return (-1);
		}
	}
	des->nnames = i;
	return (0);
}

/*
 * Parse a string of colon-delimited names.
 * Return a count of the names in "list" or
 * an error if the count exceeds "maxcnt".
 * Point "alist" at a malloc'ed string vector.
 */
int
parselist(list, alist, maxcnt)
	char *list, ***alist;
	int maxcnt;
{
	char **a;
	char *p;
	register int i;

	a = (char **) malloc(maxcnt * sizeof(char *));
	if (a == NULL) {
		pfmt(stderr, MM_ERROR, ":28:%s: no memory\n", "parselist");
		return (0);
	}
	*alist = a;

	for (i = 0 ; i < maxcnt ; i++) {
		a[i] = strtok(list, ":");
		if (a[i] == NULL)
			return (i);
		list = NULL;
	}
	
	pfmt(stderr, MM_ERROR, ":41:list is too long (maximum %d)\n", maxcnt);
	free(a);
	return (0);
}

/*
 * Append an entry to the sharetab file
 */
int
sharetab_add(dir, res, opts, descr)
	char *dir, *res, *opts, *descr;
{
	FILE *f;
	struct share sh;

	f = fopen(SHARETAB, "a");
	if (f == NULL) {
		pfmt(stderr, MM_ERROR, ":20:%s: cannot open %s: %s\n", 
		     "sharetab_add", SHARETAB, strerror(errno));
		return (-1);
	}

	if (lockf(fileno(f), F_LOCK, 0L) < 0) {
		pfmt(stderr, MM_ERROR, ":21:%s: cannot lock %s: %s\n",
		     "sharetab_add", SHARETAB, strerror(errno));
		(void) fclose(f);
		return (-1);
	}
	sh.sh_path = dir;
	sh.sh_res = res;
	sh.sh_fstype = "nfs";
	sh.sh_opts = opts;
	sh.sh_descr = descr;

	if (putshare(f, &sh) < 0) {
		pfmt(stderr, MM_ERROR, ":43:could not add %s to %s\n", 
		     dir, SHARETAB);
		(void) fclose(f);
		return (-1);
	}
	(void) fclose(f);
	return (0);
}

/*
 * Remove an entry from the sharetab file.
 */
int
sharetab_del(dir)
	char *dir;
{
	FILE *f;

	f = fopen(SHARETAB, "r+");
	if (f == NULL) {
		pfmt(stderr, MM_ERROR, ":20:%s: cannot open %s: %s\n", 
		     "sharetab_del", SHARETAB, strerror(errno));
		return (-1);
	}
	if (lockf(fileno(f), F_LOCK, 0L) < 0) {
		pfmt(stderr, MM_ERROR, ":21:%s: cannot lock %s: %s\n",
		     "sharetab_del", SHARETAB, strerror(errno));
		(void) fclose(f);
		return (-1);
	}
	if (remshare(f, dir) < 0) {
		pfmt(stderr, MM_ERROR, ":33:%s failed for %s: %s\n",
		     "remshare", dir, strerror(errno));
		return (-1);
	}
	(void) fclose(f);
	return (0);
}

void
usage()
{
	pfmt(stderr, MM_ACTION,
	     ":32:Usage: share -F nfs [-o options] [-d description] pathname\n");
}

void
lidload()
{
	if (fork() == 0) {
		execl("/usr/sbin/lidload", "lidload", 0);
		exit(0);
	}
}
