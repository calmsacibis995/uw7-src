#ident	"@(#)ns.c	1.3"
#ident	"$Header$"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/mnttab.h>
#include <sys/systeminfo.h>
#define _NSL_RPC_ABI
#include <rpc/types.h>
#include <syslog.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>
#include <rpc/xdr.h>
#include <sys/tiuser.h>
#include <rpc/clnt.h>
#include <netinet/in.h>
#include <rpcsvc/ypclnt.h>

#include "nfs_prot.h"
#define	NFSCLIENT
#include <nfs/mount.h>
#include "automount.h"

#define	NETMASKS_FILE		"/etc/netmasks"
#define	MASK_SIZE		1024
#define	STACKSIZ		30
#define MAX_YP_RETRIES		10
#define RETRY_DELAY		2

#define SUCCESS			0
#define NOTFOUND		1
#define UNAVAIL			2

extern int verbose;
extern int trace;

void macro_expand();
void dirinit();
char *get_line();
int replace_undscr_by_dot();
int unquote();
int getword();
struct mapent *do_mapent();
struct mapent *getmapent_hosts();
struct mapent *getmapent_passwd();
void loadmaster_map();
void loaddirect_map();

static char yp_mydomain[64];

void
ns_setup()
{
	(void) getdomainname(yp_mydomain, sizeof (yp_mydomain));
}

void
getmapbyname(map, opts, map_type)
	char *map;
	char *opts;
	int map_type;
{
	thread_t myid = thr_self();

	if (trace > 1)
	  fprintf(stderr, "%d: getmapbyname: map=%s\n", myid, map);

	if (map_type == 0)
		/* master map */
		(void) loadmaster_map(map, opts);
	else
		/* direct map */
		(void) loaddirect_map(map, map, opts);

}

struct mapent *
getmapent(mapname, mapopts, key, cred)
	char *mapname;
	char *mapopts;
	char *key;
	struct authunix_parms *cred;
{
	struct mapent *me = NULL;
	struct mapent *getmapent_bykey_files();
	struct mapent *getmapent_bykey_nis();
	extern int trace;
	int err;
	thread_t myid = thr_self();

	if (trace > 1)
	  fprintf(stderr, "%d: getmapent: (%s, %s, %s)\n",
		  myid, mapname, mapopts, key);

	if (strcmp(mapname, "-hosts") == 0)
		return (getmapent_hosts(mapopts, key));
	if (strcmp(mapname, "-passwd") == 0)
		return (getmapent_passwd(mapopts, key, cred));

	if (*mapname == '/') {
		/* has to be a file, so the only name svc we can use
		 * is files. So we can short circuit and not go thru
		 * the song and dance of picking a name svc and falling
		 * back to the next ....
		 */
		if ((me = getmapent_bykey_files(mapname, mapopts, key, cred, &err)) != NULL)
			return(me);
	}

	if ((me = getmapent_bykey_nis(mapname, mapopts, key, &err)) != NULL)
		return (me);

	return ((struct mapent *) NULL);
}

/* 
 * does the map exist in any name service ?
 */
map_exists(map)
char *map;
{
	int err = 0;
	thread_t myid = thr_self();

	if (trace > 2)
	  fprintf(stderr, "%d: map_exists: \n", myid);

	if (*map == '/') {
		err = access(map, R_OK);
		if (err == 0)
			return (SUCCESS);
	}
	else {
		char buff[MAXFILENAMELEN];
		strcpy(buff, "/etc/");
		strcat(buff, map);
		err = access(buff, R_OK);
		if (err  == 0)
			return (SUCCESS);
        }
	if ((err = map_exists_yp(map)) == SUCCESS)
			return (SUCCESS);

	return (UNAVAIL);
}

getnetmask_byaddr(netname, mask)
	char *netname;
	char **mask;
{
	FILE *f;
	char line[MASK_SIZE];
	char lastline[MASK_SIZE];
	int keylen, outsize, err;
	char *out;
	thread_t myid = thr_self();

	if (trace > 2)
	  fprintf(stderr, "%d: getnetmask_byaddr: \n", myid);

	err = SUCCESS;
	f = fopen(NETMASKS_FILE, "r");
	if (f != NULL) {
		while (fgets(line, MASK_SIZE -  1, f)) {
			out = strtok_r(line, " \t\n", (char **)&lastline);
			if (strcmp(out, netname) == 0) {
				out = strtok_r(NULL, " \t\n", (char **)&lastline);
				*mask = strdup(out);
				if (*mask == NULL) {
					syslog(LOG_ERR,
					       gettxt(":96", "%s: no memory"),
					       "getnetmask_byaddr");
					fclose(f);
					return (NOTFOUND);
				}
				fclose(f);
				return (SUCCESS);
			}
		}
		err = NOTFOUND;
		fclose(f);
	} else
		err = UNAVAIL;

	keylen = strlen(netname);
	if ((err = yp_match(yp_mydomain, "netmasks.byaddr",
			      netname, keylen, mask,
			      &outsize)) == SUCCESS)
		return (SUCCESS);

	err = yp_err(err);

	return (NOTFOUND);
}

void
loadmaster_map(mapname, defopts)
	char *mapname;
	char *defopts;
{
	thread_t myid = thr_self();

	if (trace > 1)
	  fprintf(stderr, "%d: loadmaster_map: mapname=%s\n", myid, mapname);

	if (*mapname == '/') {
		loadmaster_file(mapname, defopts);
		return;
	}
	loadmaster_yp(mapname, defopts);

	return;
}

void
loaddirect_map(mapname, localmap, defopts)
	char *mapname;
	char *localmap;
	char *defopts;
{
	thread_t myid = thr_self();

	if (trace > 1)
	  fprintf(stderr, "%d: loaddirect_map: mapname=%s\n", myid, mapname);

	if (*mapname == '/') {
		loaddirect_file(mapname, localmap, defopts);
		return;
	}
	loaddirect_yp(mapname, localmap, defopts);

	return;
}

replace_undscr_by_dot(map)
char *map;
{
	int ret_val = 0;
	thread_t myid = thr_self();

	if (trace > 2)
	  fprintf(stderr, "%d: replace_undscr_by_dot: map=%s \n", myid, map);

	while (*map) {
		if (*map == '_') {
			ret_val = 1;
			*map = '.';
		}
		map++;
	}
	return (ret_val);
}


yp_err(err)
	int err;
{
	thread_t myid = thr_self();

	if (trace > 1)
	  fprintf(stderr, "%d: yp_err: err=%d \n", myid, err);

	switch (err) {
	case 0:
		return (SUCCESS);
	case YPERR_KEY:
		return (NOTFOUND);
	case YPERR_MAP:
		return (UNAVAIL);
	default:
		return (UNAVAIL);
	}
}

loadmaster_yp(mapname, defopts)
	char *mapname;
	char *defopts;
{
	int first, err;
	char *key, *nkey, *val;
	int kl, nkl, vl;
	char dir[256], map[256], qbuff[256];
	char *p, *opts, *my_mapname;
	int count = 0;
	thread_t myid = thr_self();
	int retry;

	if (trace > 1)
	  fprintf(stderr, "%d: loadmaster_yp: mapname=%s\n", myid, mapname);

	first = 1;
	key  = NULL; kl  = 0;
	nkey = NULL; nkl = 0;
	val  = NULL; vl  = 0;

	/* need a private copy of mapname, because we may change
	 * the underscores by dots. We however do not want the
	 * orignal to be changed, as we may want to use the
	 * original name in some other name service
	 */
	my_mapname = strdup(mapname);
	if (my_mapname == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "loadmaster_yp");
		/* not the name svc's fault but ... */
		return (UNAVAIL);
	}
	for (;;) {
		if (first) {
			first = 0;
			retry = MAX_YP_RETRIES;
			do {
				if (retry < MAX_YP_RETRIES)
					sleep(RETRY_DELAY);
				err = yp_first(yp_mydomain, my_mapname,
					       &nkey, &nkl, &val, &vl);
			} while ((err == YPERR_YPBIND) && (--retry > 0));
			if ((err == YPERR_MAP) &&
			    (replace_undscr_by_dot(my_mapname)))
					err = yp_first(yp_mydomain, my_mapname,
						       &nkey, &nkl, &val, &vl);
		} else {
			err = yp_next(yp_mydomain, my_mapname, key, kl,
				      &nkey, &nkl, &val, &vl);
		}
		if (err) {
			if (err != YPERR_NOMORE && err != YPERR_MAP)
				if (verbose)
					syslog(LOG_ERR, "%s: %s",
					       my_mapname, yperr_string(err));
			break;
		}
		if (key)
			free(key);
		key = nkey;
		kl = nkl;

		if (kl >= 256 || vl >= 256)
			break;
		if (kl < 2 || vl < 1)
			break;
		if (isspace(*key) || *key == '#')
			break;
		(void) strncpy(dir, key, kl);
		dir[kl] = '\0';
		macro_expand("", dir, qbuff);
		(void) strncpy(map, val, vl);
		map[vl] = '\0';
		macro_expand(dir, map, qbuff);
		p = map;
		while (*p && !isspace(*p))
			p++;
		opts = defopts;
		if (*p) {
			*p++ = '\0';
			while (*p && isspace(*p))
				p++;
			if (*p == '-')

		opts = p+1;
		}

		dirinit(dir, map, opts, 0);
		count++;
		free(val);
	}
	if (my_mapname)
		free(my_mapname);

	/* in the context of a master map, if no entry is
	 *  found, it is like NOTFOUND
	 */
	if (count > 0 && err == YPERR_NOMORE)
		return (SUCCESS);
	else {
		if (err)
			return (yp_err(err));
		else
			/* this case will happen if map is empty
			 *  or none of the entries is valid
			 */
			return (NOTFOUND);
	}
}

loaddirect_yp(ypmap, localmap, opts)
	char *ypmap, *localmap, *opts;
{
	int first, err, count;
	char *key, *nkey, *val, *my_ypmap;
	int kl, nkl, vl;
	char dir[100];
	thread_t myid = thr_self();

	first = 1;
	key  = NULL; kl  = 0;
	nkey = NULL; nkl = 0;
	val  = NULL; vl  = 0;
	count = 0;
	my_ypmap = NULL;

	if (trace > 1)
	  fprintf(stderr, "%d: loaddirect_yp: ypmap=%s, localmap=%s\n",
		  myid, ypmap, localmap);

	my_ypmap = strdup(ypmap);
	if (my_ypmap == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"),
		       "loaddirect_yp");
		return (UNAVAIL);
	}
	for (;;) {
		if (first) {
			first = 0;
			err = yp_first(yp_mydomain, my_ypmap, &nkey, &nkl,
				       &val, &vl);
			if ((err == YPERR_MAP) &&
			    (replace_undscr_by_dot(my_ypmap)))
				err = yp_first(yp_mydomain, my_ypmap,
					       &nkey, &nkl, &val, &vl);

		} else {
			err = yp_next(yp_mydomain, my_ypmap, key, kl,
				      &nkey, &nkl, &val, &vl);
		}
		if (err) {
			if (err != YPERR_NOMORE && err != YPERR_MAP)
				syslog(LOG_ERR, "%s: %s",
					my_ypmap, yperr_string(err));
			break;
		}
		if (key)
			free(key);
		key = nkey;
		kl = nkl;

		if (kl < 2 || kl >= 100)
			continue;
		if (isspace(*key) || *key == '#')
			continue;
		(void) strncpy(dir, key, kl);
		dir[kl] = '\0';

		dirinit(dir, localmap, opts, 1);
		count++;
		free(val);
	}

	if (my_ypmap)
		free(my_ypmap);

	if (count > 0 && err == YPERR_NOMORE)
			return (SUCCESS);
	else
		return (yp_err(err));

}

struct mapent *
getmapent_bykey_nis(map, mapopts, key, err)
	char *map;
	char *mapopts;
	char *key;
	int  *err;
{
	char *ypline = NULL;
	char *my_map = NULL;
	struct mapent *me = NULL;
	char linebuf[2048], linebufq[2048];
	char *lp, *lq;
	int yplen, len;
	int reason;
	thread_t myid = thr_self();

	if (trace > 1)
	  fprintf(stderr, "%d: getmapent_bykey_nis: (%s, %s)\n",
		  myid, map, key);

	reason = yp_match(yp_mydomain, map, key, strlen(key), &ypline, &yplen);
	if (reason == YPERR_MAP) {
		my_map = strdup(map);
		if (my_map == NULL) {
			syslog(LOG_ERR, gettxt(":96", "%s: no memory"),
			       "getmapent_bykey_nis");
			*err = UNAVAIL;
			return ((struct mapent *) NULL);
		}
		if (replace_undscr_by_dot(my_map))
			 reason = yp_match(yp_mydomain, my_map, key,
					   strlen(key), &ypline, &yplen);
	}

	if (reason) {
		if (reason == YPERR_KEY) {
			/* Try the default entry "*"
			 */
			if (my_map == NULL)
				reason = yp_match(yp_mydomain, map, "*", 1,
					       &ypline, &yplen);
			else
				reason = yp_match(yp_mydomain, my_map, "*", 1,
					       &ypline, &yplen);
		} else {
			if (verbose)
				syslog(LOG_ERR, "%s: %s",
				       map, yperr_string(reason));
			reason = 1;
		}
	}
	if (my_map != NULL)
		free(my_map);

	*err = yp_err(reason);
	if (reason) {
		me = NULL;
		goto done;
	}

	/*
	 * at this point we are sure that yp_match succeeded
	 * so massage the entry by
	 * 1. ignoring # and beyond
	 * 2. trim the trailing whitespace
	 */
	if (lp = strchr(ypline, '#'))
		*lp = '\0';
	len = strlen(ypline);
	if (len == 0)
		goto done;
	lp = &ypline[len - 1];
	while (lp > ypline && isspace(*lp))
		*lp-- = '\0';
	if (lp == ypline)
		goto done;
	(void) strcpy(linebuf, ypline);
	lp = linebuf;
	lq = linebufq;
	unquote(lp, lq);
	/* now we have the correct line */

	me = do_mapent(lp, lq, map, mapopts, key);
done:
	if (ypline)
		free((char *) ypline);
	return (me);

}

map_exists_yp(map)
	char *map;
{
	int err, len;
	char *val, *test_map;
	thread_t myid = thr_self();

	if (trace > 1)
	  fprintf(stderr, "%d: map_exists_yp: map=%s\n", myid, map);

	test_map = NULL;
	if ((err = yp_match(yp_mydomain, map,
			     "x", 1, &val, &len)) == YPERR_MAP) {
		/* map name not found. If map name contains "_"
		 * flip them to "." and try again.
		 */
		test_map = strdup(map);
		if (test_map == NULL) {
			syslog(LOG_ERR, gettxt(":96", "%s: no memory"),
			       "map_exists_yp");
			return (UNAVAIL);
		}
		if (replace_undscr_by_dot(test_map))
			err = yp_match(yp_mydomain,
					     test_map, "x", 1, &val, &len);

		free(test_map);
	}
	if (err == 0 || err == YPERR_KEY)
		return (SUCCESS);
	else
		return (UNAVAIL);
}


FILE *
file_open(fname)
	char *fname;
{
	FILE *fp;
	char buff[MAXFILENAMELEN];
	thread_t myid = thr_self();

	if (trace > 1)
	  fprintf(stderr, "%d: file_open: fname=%s\n", myid, fname);
	
	if (*fname != '/') {
		/* prepend an "/etc" */
		(void) strcpy(buff, "/etc/");
		(void) strcat(buff, fname);
	}  else
		(void) strcpy(buff, fname);

	fp = fopen(buff, "r");
	if ((fp == NULL) && (replace_undscr_by_dot(buff))) {
		/* could not open file and name service is
		 * based on files and filename has an
		 * underscore : then replace underscore '_'
		 * by dot '.' and try to open again
		 */
		fp = fopen(buff, "r");
	}
	return (fp);
}

loadmaster_file(mastermap, defopts)
	char *mastermap;
	char *defopts;
{
	FILE *fp;
	int done = 0;
	char *line, *dir, *map, *opts;
	char linebuf[1024];
	char lineq[1024];
	thread_t myid = thr_self();

	if (trace > 1)
	  fprintf(stderr, "%d: loadmaster_file: mastermap=%s\n",
		  myid, mastermap);

	if ((fp = file_open(mastermap)) == NULL)
		return (UNAVAIL);

	while ((line = get_line(fp, linebuf, sizeof (linebuf))) != NULL) {
		macro_expand("", line, lineq);
		dir = line;
		while (*dir && isspace(*dir)) dir++;
		if (*dir == '\0')
			continue;
		map = dir;
		while (*map && !isspace(*map)) map++;
		if (*map)
			*map++ = '\0';
		if (*dir == '+') {
			opts = map;
			while (*opts && isspace(*opts)) opts++;
			if (*opts != '-')
				opts = defopts;
			else
				opts++;

			dir++;
			(void) loadmaster_map(dir, opts);
		} else {
			while (*map && isspace(*map)) map++;
			if (*map == '\0')
				continue;
			opts = map;
			while (*opts && !isspace(*opts)) opts++;
			if (*opts) {
				*opts++ = '\0';
				while (*opts && isspace(*opts)) opts++;
			}
			if (*opts != '-')
				opts = defopts;
			else
				opts++;

			dirinit(dir, map, opts, 0);
		}
		done++;
	}
	(void) fclose(fp);
	if (done > 0)
		return (SUCCESS);
	else
		return (NOTFOUND);
}

loaddirect_file(map, local_map, opts)
	char *map, *local_map, *opts;
{
	FILE *fp;
	int done = 0;
	char *line, *p1, *p2;
	char linebuf[1024];
	thread_t myid = thr_self();

	if (trace > 1)
	  fprintf(stderr, "%d: loaddirect_file: map=%s\n", myid, map);

	if ((fp = file_open(map)) == NULL)
		return (UNAVAIL);

	while ((line = get_line(fp, linebuf, sizeof (linebuf))) != NULL) {
		p1 = line;
		while (*p1 && isspace(*p1)) p1++;
		if (*p1 == '\0')
			continue;
		p2 = p1;
		while (*p2 && !isspace(*p2)) p2++;
		*p2 = '\0';
		if (*p1 == '+') {
			p1++;
			(void) loaddirect_map(p1, local_map, opts);
		} else {
			dirinit(p1, local_map, opts, 1);
		}
		done++;
	}

	(void) fclose(fp);
	if (done > 0)
		return (SUCCESS);
	else
		return (NOTFOUND);
}

struct mapent *
getmapent_bykey_files(mapname, mapopts, key, cred, err)
	char *mapname;
	char *mapopts;
	char *key;
	struct authunix_parms *cred;
	int  *err;
{
	extern int syntaxok;
	FILE *fp = NULL;
	struct mapent *me = NULL;
	char word[128], wordq[128], obuff[128];
	char *lp, *lq, linebuf[2048], linebufq[2048];
	thread_t myid = thr_self();

	if (trace > 1)
	  fprintf(stderr, "%d: getmapent_bykey_files: (%s, %s)\n",
		  myid, mapname, key);

	*err = SUCCESS;
	if ((fp = file_open(mapname)) == NULL) {
		*err = UNAVAIL;
		return ((struct mapent *) NULL);
	}
	for (;;) {
		lp = get_line(fp, linebuf, sizeof (linebuf));
		if (lp == NULL) {
			(void) fclose(fp);
			*err = NOTFOUND;
			return ((struct mapent *) NULL);
		}
		if (verbose && syntaxok && isspace(*(u_char *)lp)) {
			syntaxok = 0;
			syslog(LOG_ERR,
			       gettxt(":236", "%s: leading space in map entry %s in %s"),
			       "getmapent_bykey_files", lp, mapname);
		}
		lq = linebufq;
		unquote(lp, lq);
		getword(word, wordq, &lp, &lq, ' ');
		if (strcmp(word, key) == 0)
			break;
		if (word[0] == '*' && word[1] == '\0')
			break;
		if (word[0] == '+') {
			syslog(LOG_ERR,
			       gettxt(":250", "%s: recursively call %s with %s"),
			       "getmapent_bykey_files", "getmapent", word);
			getword(obuff, wordq, &lp, &lq, ' ');
			if (obuff[0] == '-')
				mapopts = obuff;
			me = getmapent(word+1, mapopts, key, cred);
			if (me != NULL) {
				(void) fclose(fp);
				return (me);
			}
			continue;
		}
		/*
		 * sanity check each map entry key against
		 * the lookup key as the map is searched.
		 */
		if (verbose && syntaxok) { /* sanity check entry */
			if (*key == '/') {
				if (*word != '/') {
					syntaxok = 0;
					syslog(LOG_ERR,
					       gettxt(":237", "%s: bad key %s in direct map %s"),
					       "getmapent_bykey_files",
					       word, mapname);
				}
			} else {
				if (strchr(word, '/')) {
					syntaxok = 0;
					syslog(LOG_ERR,
					       gettxt(":238", "%s: bad key %s in indirect map %s"),
					       "getmapent_bykey_files",
					       word, mapname);
				}
			}
		}
	}
	(void) fclose(fp);

	/* if we fall thru here => we have the entry */
	me = do_mapent(lp, lq, mapname, mapopts, key);
	return (me);
}

