/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)auto_look.c	1.2"
#ident	"$Header$"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <pwd.h>
#include <netinet/in.h>
#include <netdb.h>
#define _NSL_RPC_ABI
#include <rpc/types.h>
#include <syslog.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>
#include <rpc/xdr.h>
#include <sys/tiuser.h>
#include <rpc/clnt.h>
#include <unistd.h>
#include "nfs_prot.h"
#define NFSCLIENT
typedef nfs_fh fhandle_t;
#include <rpcsvc/mount.h>
#include <nfs/mount.h>
#include "automount.h"

nfsstat do_mount();
struct mapent *getmapent();
void diag();
void getword();
void unquote();
void macro_expand();
void free_mapent();

extern int trace;
extern int verbose;

/*
 * Description:
 *	Gets the entries from the map and file out the mapent structures.
 *	Calls do_mount() to check fs_q and tmpq and do mounts if needed.
 *	Make the link before returning.
 * Call From:
 *	nfsproc_lookup_2_svc, nfsproc_readlink_2_svc
 * Entry/Exit:
 *	No locks held on entry.
 * 	Only if successful, link's avnode lock is held on
 *		exit (from makelink()).
 */
nfsstat
lookup(dir, name, vpp, cred)
	struct autodir *dir;
	char *name;
	struct avnode **vpp;
	struct authunix_parms *cred;
{
	struct mapent *me;
	struct link *link;
	struct filsys *fs = NULL;
	char *linkpath = NULL;
	nfsstat status;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: lookup: name=%s\n", myid, name);

	me = getmapent(dir->dir_map, dir->dir_opts, name, cred);
	if (me == NULL) {
		if (*name == '=' && cred->aup_uid == 0)
			diag(name+1);
		return (NFSERR_NOENT);
	}

	if (trace > 1) {
		struct mapent *ms;
		struct mapfs *mfs;
	
		fprintf(stderr, "%d: lookup: %s/ %s (%s)\n",
			myid, dir->dir_name, name, me->map_root);
		for (ms = me; ms; ms = ms->map_next) {
			fprintf(stderr, "   %s \t-%s\t",
				*ms->map_mntpnt ? ms->map_mntpnt : "/",
				ms->map_mntopts);
			for (mfs = ms->map_fs; mfs; mfs = mfs->mfs_next)
				fprintf(stderr, "%d: %s:%s%s%s ",
					myid,
					mfs->mfs_host,
					mfs->mfs_dir,
					*mfs->mfs_subdir ? ":" : "",
					mfs->mfs_subdir);
			fprintf(stderr, "\n");
		}
	}

	status = do_mount(dir, me, &fs, &linkpath);
	free_mapent(me);

	if (status != NFS_OK)
		return (status);

	if (linkpath == NULL)
		syslog(LOG_ERR, gettxt(":252","%s: ERROR: %s is %s"),
		       "lookup", "linkpath", "NULL");

	link = makelink(dir, name, fs, linkpath);
	if (link == NULL)
		return (NFSERR_NOSPC);

	*vpp = &link->link_vnode;
	return (NFS_OK);
}

/*
 * This routine is really for debugging purpose only.  It is a way to
 * look at internal structures and to turn on/off tracing and verbose
 * messages.
 */
void
diag(s)
	char *s;
{
	register int i;
	register struct autodir *dir;
	register struct avnode *avnode;
	register struct link *link;
	register struct filsys *fs;
	extern dev_t tmpdev;
	extern struct q fh_q_hash[];
	/* extern rwlock_t fh_rwlock; */
	/* extern rwlock_t fsq_rwlock; */
	/* extern rwlock_t tmpq_rwlock; */
	thread_t myid = thr_self();

	fprintf(stderr, "%d: diag(%s)\n", myid, s);

	syslog(LOG_ERR, "%d: WARNING: in debugging routine: diag(%s)", myid, s);

	if (isdigit(*s)) {
		/* set trace level */
		trace = atoi(s);
		syslog(LOG_ERR, "trace = %d", trace);
		return;
	}

	switch (*s) {
	case 'v':	/* toggle verbose */
		verbose = !verbose;
		if (verbose)
			syslog(LOG_ERR, "verbose on");
		else
			syslog(LOG_ERR, "verbose off");
		break;

	case 'n':	/* print vnodes */
		/* RW_RDLOCK(&fh_rwlock); */
		fprintf(stderr, "%d: These are the current links:\n", myid);
		for (i = 0; i < FH_HASH_SIZE; i++) {
			avnode = HEAD(struct avnode, fh_q_hash[i]);
			for (; avnode; avnode = NEXT(struct avnode, avnode)) {
				/* MUTEX_LOCK(&avnode->vn_mutex); */
				if (avnode->vn_type == VN_LINK) {
					link = (struct link *)avnode->vn_data;
					fprintf(stderr, 
						"link: (%d, %d) %s/ %s ",
						avnode->vn_valid,
						avnode->vn_count,
						link->link_dir->dir_name,
						link->link_name);
					if (link->link_path)
						fprintf(stderr, "-> \"%s\" ",
							link->link_path);
					if (link->link_fs)
						fprintf(stderr, "@ %s:%s ",
							link->link_fs->fs_host,
							link->link_fs->fs_dir);
					fprintf(stderr, "\t[%d]\n",
						link->link_death == 0 ? 0 :
						link->link_death - time_now);
			        } else {
					dir = (struct autodir *)avnode->vn_data;
					fprintf(stderr, 
						"dir : %s %s -%s\n",
						dir->dir_name, dir->dir_map,
						dir->dir_opts);
				}
				/* MUTEX_UNLOCK(&avnode->vn_mutex); */
			}
		}
		fprintf(stderr, "\n");
		/* RW_UNLOCK(&fh_rwlock); */
		break;

	case 'f' :	/* print fs's */
		/* RW_RDLOCK(&fsq_rwlock); */
		/* RW_RDLOCK(&tmpq_rwlock); */
		fprintf(stderr, "%d: This is the current fs_q:\n", myid);
		for (fs = HEAD(struct filsys, fs_q); fs; 
		     fs = NEXT(struct filsys, fs)) {
			/* MUTEX_LOCK(&fs->fs_mutex); */
			fprintf(stderr, "%s %s:%s -%s ",
				fs->fs_mntpnt, fs->fs_host, fs->fs_dir,
				fs->fs_opts);
			if (fs->fs_mine) {
				fprintf(stderr, "(%x)%x:%x ",
					tmpdev & 0xFFFF,
					fs->fs_mntpntdev & 0xFFFF,
					fs->fs_mountdev & 0xFFFF);
				fprintf(stderr, "<%d> ", fs->fs_unmounted);
				fprintf(stderr, "[%d]\n",
					fs->fs_death > time_now ?
					fs->fs_death - time_now : 0);
			} else
				fprintf(stderr, "\n");
			/* MUTEX_UNLOCK(&fs->fs_mutex); */
		}
		fprintf(stderr, "%d: This is the current tmpq:\n", myid);
		for (fs = HEAD(struct filsys, tmpq); fs; 
		     fs = NEXT(struct filsys, fs)) {
			/* MUTEX_LOCK(&fs->fs_mutex);
			fprintf(stderr, "%s %s:%s -%s ",
				fs->fs_mntpnt, fs->fs_host, fs->fs_dir,
				fs->fs_opts);
			if (fs->fs_mine) {
				fprintf(stderr, "(%x)%x:%x ",
					tmpdev & 0xFFFF,
					fs->fs_mntpntdev & 0xFFFF,
					fs->fs_mountdev & 0xFFFF);
				fprintf(stderr, "<%d> ", fs->fs_unmounted);
				fprintf(stderr, "[%d]\n",
					fs->fs_death > time_now ?
					fs->fs_death - time_now : 0);
			} else
				fprintf(stderr, "\n");
			/* MUTEX_UNLOCK(&fs->fs_mutex); */
		}
		fprintf(stderr, "\n");
		/* RW_UNLOCK(&tmpq_rwlock); */
		/* RW_UNLOCK(&fsq_rwlock); */
		break;

	case 'd' :	/* print dir_q's */
		/* RW_WRLOCK(&fh_rwlock); */
		fprintf(stderr, "%d: This is dir_q:\n", myid);
		for (dir = HEAD(struct autodir, dir_q); dir;
		     dir = NEXT(struct autodir, dir)) {
			avnode = &dir->dir_vnode;
			/* RW_WRLOCK(&dir->dir_rwlock); */
			fprintf(stderr, "%s %s (%d %d %d)\n",
				dir->dir_name, dir->dir_map,
				avnode->vn_fattr.uid, avnode->vn_fattr.gid,
				avnode->vn_fattr.size);
			/* RW_UNLOCK(&dir->dir_rwlock); */
		}
		/* RW_UNLOCK(&fh_rwlock); */
		break;
	}
}


struct mapent *
do_mapent(lp, lq, mapname, mapopts, key)
	char *lp, *lq, *mapname, *mapopts, *key;
{
	char w[1024], wq[1024];
	char entryopts[1024];
	struct mapent *me, *mp, *ms;
	int err, implied;
	extern char *opt_check();
	extern int syntaxok;
	char *p;
	thread_t myid = thr_self();

	macro_expand(key, lp, lq);

	if (trace > 1)
		fprintf(stderr, "%d: do_mapent: \"%s %s\"\n", myid, key, lp);

	getword(w, wq, &lp, &lq, ' ');

	if (w[0] == '-') {	/* default mount options for entry */
		if (syntaxok && (p = opt_check(w+1))) {
			syntaxok = 0;
			syslog(LOG_ERR,
			       gettxt(":193",
				      "WARNING: %s ignored for %s in %s"),
			       p, key, mapname);
		}
		(void) strcpy(entryopts, w+1);
		mapopts = entryopts;
		getword(w, wq, &lp, &lq, ' ');
	}
	implied = *w != '/';

	ms = me = NULL;
	while (*w == '/' || implied) {
		mp = me;
		me = (struct mapent *)malloc(sizeof (*me));
		if (me == NULL)
			goto alloc_failed;
		memset((char *) me, 0, sizeof (*me));
		if (ms == NULL)
			ms = me;
		else
			mp->map_next = me;
		
		if (strcmp(w, "/") == 0 || implied)
			me->map_mntpnt = strdup("");
		else
			me->map_mntpnt = strdup(w);
		if (me->map_mntpnt == NULL)
			goto alloc_failed;

		if (implied)
			implied = 0;
		else
			getword(w, wq, &lp, &lq, ' ');

		if (w[0] == '-') {	/* mount options */
			if (syntaxok && (p = opt_check(w+1))) {
				syntaxok = 0;
				syslog(LOG_ERR,
				       gettxt(":193",
					      "WARNING: %s ignored for %s in %s"),
				       p, key, mapname);
			}
			me->map_mntopts = strdup(w+1);
			getword(w, wq, &lp, &lq, ' ');
		} else
			me->map_mntopts = strdup(mapopts);
		if (me->map_mntopts == NULL)
			goto alloc_failed;
		if (w[0] == '\0') {
			syslog(LOG_ERR,
			       gettxt(":194",
				      "%s: map %s, key %s: bad"),
			       "do_mapent", mapname, key);
			goto bad_entry;
		}
		err = mfs_get(mapname, me, w, wq, &lp, &lq);
		if (err < 0)
			goto alloc_failed;
		if (err > 0)
			goto bad_entry;
		me->map_next = NULL;
	}

	if (*key == '/') {
		*w = '\0';	/* a hack for direct maps */
	} else {
		(void) strcpy(w, "/");
		(void) strcat(w, key);
	}
	ms->map_root = strdup(w);
	if (ms->map_root == NULL)
		goto alloc_failed;

	return (ms);

alloc_failed:
	syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "do_mapent");
bad_entry:
	free_mapent(ms);
	return ((struct mapent *) NULL);
}


char *
get_line(fp, line, linesz)
	FILE *fp;
	char *line;
	int linesz;
{
	register char *p;
	register int len;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: get_line: \n", myid);

	p = line;

	for (;;) {
		if (fgets(p, linesz - (p-line), fp) == NULL)
			return (NULL);
trim:
		len = strlen(line);
		if (len <= 0) {
			p = line;
			continue;
		}
		/* trim trailing white space */
		p = &line[len - 1];
		while (p > line && isspace(*(u_char *)p))
			*p-- = '\0';
		if (p == line)
			continue;
		/* if continued, get next line */
		if (*p == '\\')
			continue;
		/* ignore # and beyond */
		if (p = strchr(line, '#')) {
			*p = '\0';
			goto trim;
		}
		return (line);
	}
}


mfs_get(mapname, me, w, wq, lp, lq)
	struct mapent *me;
	char *mapname, *w, *wq, **lp, **lq;
{
	struct mapfs *mfs, **mfsp;
	char *wlp, *wlq;
	char *hl, hostlist[1024], *hlq, hostlistq[1024];
	char hostname_and_penalty[MXHOSTNAMELEN+5];
	char *hn, *hnq, hostname[MXHOSTNAMELEN+1];
	char dirname[MAXPATHLEN+1], subdir[MAXPATHLEN+1];
	char qbuff[MAXPATHLEN+1], qbuff1[MAXPATHLEN+1];
	char pbuff[10], pbuffq[10];
	int penalty;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: mfs_get: mapname=%s\n", myid, mapname);

	mfsp = &me->map_fs;
	*mfsp = NULL;

	while (*w && *w != '/') {
		wlp = w; wlq = wq;
		getword(hostlist, hostlistq, &wlp, &wlq, ':');
		if (!*hostlist)
			goto bad_entry;
		getword(dirname, qbuff, &wlp, &wlq, ':');
		if (*dirname != '/')
			goto bad_entry;
		*subdir = '/'; *qbuff = ' ';
		getword(subdir+1, qbuff+1, &wlp, &wlq, ':');

		hl = hostlist; hlq = hostlistq;
		for (;;) {
			getword(hostname_and_penalty, qbuff, &hl, &hlq, ',');
			if (!*hostname_and_penalty)
				break;
			hn = hostname_and_penalty;
			hnq = qbuff;
			getword(hostname, qbuff1, &hn, &hnq, '(');
			
			if (strcmp(hostname, hostname_and_penalty) == 0) {
				penalty = 0;
			} else {			
				hn++; hnq++;
				getword(pbuff, pbuffq, &hn, &hnq, ')');
				if (!*pbuff)
					penalty = 0;
				else
					penalty = atoi(pbuff);
			}
			mfs = (struct mapfs *)malloc(sizeof *mfs);
			if (mfs == NULL)
				return (-1);
			memset(mfs, 0, sizeof *mfs);
			*mfsp = mfs;
			mfsp = &mfs->mfs_next;
	
			mfs->mfs_host = strdup(hostname);
			mfs->mfs_penalty = penalty;
			if (mfs->mfs_host == NULL)
				return (-1);
			mfs->mfs_dir = strdup(dirname);
			if (mfs->mfs_dir == NULL)
				return (-1);
			mfs->mfs_subdir = strdup( *(subdir+1) ? subdir : "");
			if (mfs->mfs_subdir == NULL)
				return (-1);
		}
		getword(w, wq, lp, lq, ' ');
	}
	return (0);

bad_entry:
	syslog(LOG_ERR, gettxt(":195", "%s: bad entry %s in map %s"),
	       "mfs_get", w, mapname);
	return (1);
}

void
free_mapent(me)
	struct mapent *me;
{
	struct mapfs *mfs;
	struct mapent *m;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: free_mapent: \n", myid);

	while (me) {
		while (me->map_fs) {
			mfs = me->map_fs;
			if (mfs->mfs_host)
				free(mfs->mfs_host);
			if (mfs->mfs_dir)
				free(mfs->mfs_dir);
			if (mfs->mfs_subdir)
				free(mfs->mfs_subdir);
			me->map_fs = mfs->mfs_next;
			free((char *)mfs);
		}
		if (me->map_root)
			free(me->map_root);
		if (me->map_mntpnt)
			free(me->map_mntpnt);
		if (me->map_mntopts)
			free(me->map_mntopts);
		m = me->map_next;
		free((char *)me);	/* from all this misery */
		me = m;
	}
}

/*
 * Gets the next token from the string "p" and copies
 * it into "w".  Both "wq" and "w" are quote vectors
 * for "w" and "p".  Delim is the character to be used
 * as a delimiter for the scan.  A space means "whitespace".
 */
void
getword(w, wq, p, pq, delim)
	char *w, *wq, **p, **pq, delim;
{
	while ((delim == ' ' ? isspace(**p) : **p == delim) && **pq == ' ')
		(*p)++, (*pq)++;

	while (**p &&
	     !((delim == ' ' ? isspace(**p) : **p == delim) && **pq == ' ')) {
		*w++  = *(*p)++;
		*wq++ = *(*pq)++;
	}
	*w  = '\0';
	*wq = '\0';
}

/*
 * Performs text expansions in the string "pline".
 * "plineq" is the quote vector for "pline".
 * An identifier prefixed by "$" is replaced by the
 * corresponding environment variable string.  A "&"
 * is replaced by the key string for the map entry.
 */
void
macro_expand(key, pline, plineq)
	char *key, *pline, *plineq;
{
	register char *p,  *q;
	register char *bp, *bq;
	register char *s;
	char buffp[2048], buffq[2048];
	char envbuf[64], *pe;
	int expand = 0;
	char *getenv();
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: macro_expand: key=%s\n", myid, key);

	p = pline ; q = plineq;
	bp = buffp ; bq = buffq;
	while (*p) {
		if (*p == '&' && *q == ' ') {	/* insert key */
			for (s = key ; *s ; s++) {
				*bp++ = *s;
				*bq++ = ' ';
			}
			expand++;
			p++; q++;
			continue;
		}

		if (*p == '$' && *q == ' ') {	/* insert env var */
			p++; q++;
			pe = envbuf;
			if (*p == '{') {
				p++ ; q++;
				while (*p && *p != '}') {
					*pe++ = *p++;
					q++;
				}
				if (*p) {
					p++ ; q++;
				}
			} else {
				while (*p && isalnum(*p)) {
					*pe++ = *p++;
					q++;
				}
			}
			*pe = '\0';
			s = getenv(envbuf);
			if (s) {
				while (*s) {
					*bp++ = *s++;
					*bq++ = ' ';
				}
				expand++;
			}
			continue;
		}
		*bp++ = *p++;
		*bq++ = *q++;

	}
	if (!expand)
		return;
	*bp = '\0';
	*bq = '\0';
	(void) strcpy(pline , buffp);
	(void) strcpy(plineq, buffq);
}


/*
 * Removes quotes from the string "str" and returns
 * the quoting information in "qbuf". e.g.
 * original str: 'the "quick brown" f\ox'
 * unquoted str: 'the quick brown fox'
 *         qbuf: '    ^^^^^^^^^^^  ^ '
 */
void
unquote(str, qbuf)
	char *str, *qbuf;
{
	register int escaped, inquote, quoted;
	register char *ip, *bp, *qp;
	char buf[2048];

	escaped = inquote = quoted = 0;

	for (ip = str, bp = buf, qp = qbuf ; *ip ; ip++) {
		if (!escaped) {
			if (*ip == '\\') {
				escaped = 1;
				quoted++;
				continue;
			} else
			if (*ip == '"') {
				inquote = !inquote;
				quoted++;
				continue;
			}
		}

		*bp++ = *ip;
		*qp++ = (inquote || escaped) ? '^' : ' ';
		escaped = 0;
	}
	*bp = '\0';
	*qp = '\0';
	if (quoted)
		(void) strcpy(str, buf);
}


struct mapent *
getmapent_passwd(mapopts, login, cred)
	char *mapopts, *login;
	struct authunix_parms *cred;
{
	struct mapent *me;
	struct mapfs *mfs;
	struct passwd *pw;
	char buf[64];
	char *p;
	int c;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: getmapent_passwd: \n", myid);

	if (login[0] == '~' && login[1] == 0) {
		pw = getpwuid(cred->aup_uid);
		if (pw)
			login = pw->pw_name;
	}
	else
		pw = getpwnam(login);
	if (pw == NULL)
		return ((struct mapent *) NULL);
	for (c = 0, p = pw->pw_dir ; *p ; p++)
		if (*p == '/')
			c++;
	if (c != 3)     /* expect "/dir/host/user" */
		return ((struct mapent *) NULL);

	me = (struct mapent *)malloc(sizeof *me);
	if (me == NULL)
		goto alloc_failed;
	memset((char *) me, 0, sizeof *me);
	me->map_mntopts = strdup(mapopts);
	if (me->map_mntopts == NULL)
		goto alloc_failed;
	mfs = (struct mapfs *)malloc(sizeof *mfs);
	if (mfs == NULL)
		goto alloc_failed;
	memset((char *) mfs, 0, sizeof *mfs);
	me->map_fs = mfs;
	(void) strcpy(buf, "/");
	(void) strcat(buf, login);
	mfs->mfs_subdir = strdup(buf);
	p = strrchr(pw->pw_dir, '/');
	*p = '\0';
	p = strrchr(pw->pw_dir, '/');
	mfs->mfs_host = strdup(p+1);
	if (mfs->mfs_host == NULL)
		goto alloc_failed;
	me->map_root = strdup(p);
	if (me->map_root == NULL)
		goto alloc_failed;
	me->map_mntpnt = strdup("");
	if (me->map_mntpnt == NULL)
		goto alloc_failed;
	mfs->mfs_dir = strdup(pw->pw_dir);
	if (mfs->mfs_dir == NULL)
		goto alloc_failed;
	(void) endpwent();
	return (me);

alloc_failed:
	syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "getmapent_passwd");
	free_mapent(me);
	return ((struct mapent *) NULL);
}


struct mapent *
getmapent_hosts(mapopts, host)
	char *mapopts, *host;
{
	CLIENT *cl;
	struct mapent *me, *ms, *mp;
	struct mapfs *mfs;
	struct exports *ex = NULL;
	struct exports *exlist, *texlist, **texp, *exnext;
	struct timeval timeout;
	enum clnt_stat pingmount();
	char name[MAXPATHLEN];
	int elen;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: getmapent_hosts: host=%s\n", myid, host);

	/* check for special case: host is me */

	if (strcmp(host, self) == 0) {
		ms = (struct mapent *)malloc(sizeof *ms);
		if (ms == NULL)
			goto alloc_failed;
		memset((char *) ms, 0, sizeof *ms);
		ms->map_root = strdup("");
		if (ms->map_root == NULL)
			goto alloc_failed;
		ms->map_mntpnt = strdup("");
		if (ms->map_mntpnt == NULL)
			goto alloc_failed;
		ms->map_mntopts = strdup("");
		if (ms->map_mntopts == NULL)
			goto alloc_failed;
		mfs = (struct mapfs *)malloc(sizeof *mfs);
		if (mfs == NULL)
			goto alloc_failed;
		memset((char *) mfs, 0, sizeof *mfs);
		ms->map_fs = mfs;
		mfs->mfs_host = strdup(self);
		if (mfs->mfs_host == NULL)
			goto alloc_failed;
		mfs->mfs_dir  = strdup("/");
		if (mfs->mfs_dir == NULL)
			goto alloc_failed;
		mfs->mfs_subdir  = strdup("");
		if (mfs->mfs_subdir == NULL)
			goto alloc_failed;
		return (ms);
	}

	if (pingmount(host) != RPC_SUCCESS)
		return ((struct mapent *) NULL);

	/* get export list of host */
	cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "circuit_v");
	if (cl == NULL) {
		cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "datagram_v");
		if (cl == NULL) {
			syslog(LOG_ERR,
			       gettxt(":218",
				      "%s: %s server not responding for %s"),
			       "getmapent_hosts", host,
			       clnt_spcreateerror("clnt_create"));
			return((struct mapent *) NULL);
		}
	      
	}

	timeout.tv_usec = 0;
	timeout.tv_sec  = 25;
	if (clnt_call(cl, MOUNTPROC_EXPORT, xdr_void, 0,
		      xdr_exports, (caddr_t)&ex, timeout)) {
		syslog(LOG_ERR,
		       gettxt(":218", "%s: %s server not responding for %s"),
		       "getmapent_hosts", host, clnt_sperror(cl, "clnt_call"));
		clnt_destroy(cl);
		return((struct mapent *) NULL);
		
	}

	clnt_destroy (cl);
        
	if (ex == NULL) {
		if (trace > 1)
			fprintf(stderr,
				"%d: getmapent_hosts: null export list\n",
				myid);
		return ((struct mapent *) NULL);
	}

	/* now sort by length of names - to get mount order right */
	exlist = ex;
	texlist = NULL;
	for (ex = exlist; ex; ex = exnext) {
		exnext = ex->ex_next;
		ex->ex_next = 0;
		elen = strlen(ex->ex_name);

		for (texp = &texlist; *texp; texp = &((*texp)->ex_next))
			if (elen < (int) strlen((*texp)->ex_name))
				break;
		ex->ex_next = *texp;
		*texp = ex;
	}
	exlist = texlist;

	/* Now create a mapent from the export list */
	ms = NULL;
	me = NULL;
	for (ex = exlist; ex; ex = ex->ex_next) {
		mp = me;
		me = (struct mapent *)malloc(sizeof *me);
		if (me == NULL)
			goto alloc_failed;
		memset((char *) me, 0, sizeof *me);

		if (ms == NULL)
			ms = me;
		else
			mp->map_next = me;

		(void) strcpy(name, "/");
		(void) strcat(name, host);
		me->map_root = strdup(name);
		if (me->map_root == NULL)
			goto alloc_failed;
		if (strcmp(ex->ex_name, "/") == 0)
			me->map_mntpnt = strdup("");
		else
			me->map_mntpnt = strdup(ex->ex_name);
		if (me->map_mntpnt == NULL)
			goto alloc_failed;
		me->map_mntopts = strdup(mapopts);
		if (me->map_mntopts == NULL)
			goto alloc_failed;
		mfs = (struct mapfs *)malloc(sizeof *mfs);
		if (mfs == NULL)
			goto alloc_failed;
		memset((char *) mfs, 0, sizeof *mfs);
		me->map_fs = mfs;
		mfs->mfs_host = strdup(host);
		if (mfs->mfs_host == NULL)
			goto alloc_failed;
		mfs->mfs_dir  = strdup(ex->ex_name);
		if (mfs->mfs_dir == NULL)
			goto alloc_failed;
		mfs->mfs_subdir = strdup("");
		if (mfs->mfs_subdir == NULL)
			goto alloc_failed;
	}
	freeex(exlist);
	return (ms);

alloc_failed:
	syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "getmapent_hosts");
	free_mapent(ms);
	freeex(exlist);
	return ((struct mapent *) NULL);
}

