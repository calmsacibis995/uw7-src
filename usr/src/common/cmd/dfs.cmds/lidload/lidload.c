/*		copyright	"%c%" 	*/

#ident	"@(#)lidload.c	1.2"
#ident  "$Header$"

#define _KMEMUSER
#include <rpc/types.h>
#include <sys/privilege.h>
#undef _KMEMUSER
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <mac.h>
#include <errno.h>
#include <sys/tiuser.h>
#include <nfs/export.h>
#include <netdir.h>
#include <netconfig.h>
#include <netinet/in.h>
#include <nfs/nfs.h>
#include <nfs/export.h>
#include <nfs/nfssys.h>
#include <sys/nserve.h>
#include <sys/rf_sys.h>
#include "lidload.h"

#define LPFILE	"/etc/dfs/lid_and_priv"

#ifdef __STDC__
static int		getlpent(FILE *, struct lap *);
static struct lpgen	*load_lid_and_priv(u_int *, lid_t *, pvec_t *, int *);
static int		makelist(char *, char *, struct lpgen **);
static int		priv2pvec(char *, pvec_t *);
static void		pr_err(char *, ...);
int			rfsys(int, ...);
int			_nfssys(int, caddr_t);
void			nc_perror(char *);
int			privnum(char *);
void			rfs_uniq(struct lpgen *, u_int, rfslp_t **, u_int *);
void			usage(void);
#else
static int		getlpent();
static struct lpgen	*load_lid_and_priv();
static int		makelist();
static int		priv2pvec();
static void		pr_err();
int			rfsys();
int			_nfssys();
void			nc_perror();
int			privnum();
void			rfs_uniq();
void			usage();
#endif

#define DNFILE "/etc/rfs/domain"
char domainname[MAXDNAME];

/*ARGSUSED*/
main(argc, argv)
	int argc;
	char *argv[];
{
	struct lpgen		*lp;
	u_int			size, nfscount;
	lid_t			deflid;
	pvec_t			defpriv;
	struct nfs_loadlp_args	nfsarg;
	int			nfserr	= 0;
	rfsloadlp_t		rfsarg;
	int			rfserr	= 0;
	int			norfs	= FALSE;
	int			nonfs	= FALSE;
	struct nfs_getfh_args	getfha;
	fhandle_t		fhbuf;
	struct nfslpbuf		*nfsdata= (struct nfslpbuf *)malloc((u_int)1);
	struct nfslpbuf		*nfsq;
	struct lpgen		*p;
	int			defset	= FALSE;	
	int			i;

	if (argc != 1) {
		usage();
		exit(1);
	}

	if (rfsys(RF_GETDNAME, domainname, MAXDNAME) < 0 ||
	    domainname[0] == '\0') {
		if (errno == ENOPKG)
			norfs = TRUE;
		else {
			FILE *fp;
			if (((fp = fopen(DNFILE, "r")) == NULL) ||
			    (fgets(domainname, MAXDNAME, fp) == NULL)) {
				norfs = TRUE;
			} else {
				/* remove trailing newline if necessary */
				if (domainname[strlen(domainname)-1] == '\n')
					domainname[strlen(domainname)-1] = '\0';
				fclose(fp);
			}
		}
	}

	getfha.fname = "/";
	getfha.fhp = &fhbuf;
	if (_nfssys(NFS_GETFH, (caddr_t)&getfha) < 0 && errno == ENOPKG)
		nonfs = TRUE;

	if (nonfs && norfs){
		pr_err("neither rfs nor nfs running\n");
		exit(1);
	}

	if ((lp = load_lid_and_priv(&size, &deflid, &defpriv, &defset)) == NULL
	&&	defset == FALSE
	){
		pr_err("found no valid entries to download\n");
		exit(1);
	}

	if (nonfs)
		goto rfs;

	nfscount = 0;
	for(i = 0, p = lp; i < size / sizeof(struct lpgen); i++, p++) {
		if (p->lp_valid) {
			nfsdata = (struct nfslpbuf *)realloc(nfsdata,
					(nfscount+1) * sizeof(struct nfslpbuf));
			nfsq = nfsdata + nfscount;
			nfsq->addr  = p->lp_addr;
			nfsq->mask  = p->lp_mask;
			nfsq->lid  = p->lp_lid;
			nfsq->priv = p->lp_priv;
			
			nfscount++;
		}
	}

	nfsarg.buf = nfsdata;
	nfsarg.size = size;
	nfsarg.deflid = deflid;
	nfsarg.defpriv = defpriv;
	if ((nfserr = _nfssys(NFS_LOADLP, (caddr_t)&nfsarg)) < 0) {
		nfserr = errno;
		perror ("lidload: NFS load");
	}

	if (norfs)
		exit(nfserr);

rfs:
	/* stuff for RFS */
	rfs_uniq(lp, size, &rfsarg.rfloadlp_buf, &rfsarg.rfloadlp_size);
	rfsarg.rfloadlp_deflid	= deflid;
	rfsarg.rfloadlp_defpriv	= defpriv;

	if ((rfserr = rfsys(RF_LOADLP, &rfsarg)) < 0) {
		rfserr = errno;
		perror ("lidload: RFS load");
	}

	exit(nfserr | rfserr << 4 /* shift enough to prevent overlap?*/);
	/* NOTREACHED */
}
void
rfs_uniq(lp, size, bufpp, n)
	struct lpgen	*lp;
	u_int		size;
	rfslp_t		**bufpp;	
	u_int		*n;
{
	char		*curname = "";
	struct lpgen	*p;
	u_int		i, ucount = 0;
	rfslp_t		*rfsdata = (rfslp_t *)malloc((u_int)1), *q;

	*n = 0;
	if(!lp)
		return;

	for(i = 0, p = lp; i < size / sizeof(struct lpgen); i++, p++) {
	/*
	 * For each unique name create an rfslp_t struct to hold it,
	 * thereby removing duplicates from the original lpgen array.
	 */
		if( (strcmp(curname,p->lp_hostname) && p->lp_valid) ) {
			rfsdata = (rfslp_t *)realloc(rfsdata,
					(ucount+1)*sizeof(rfslp_t));
			q = rfsdata + ucount;
			q->rflp_hostname= p->lp_hostname;
			q->rflp_lid  	= p->lp_lid;
			q->rflp_priv 	= p->lp_priv;
			
			ucount++;
			curname = p->lp_hostname;
		}
	}
	*n 	= ucount * sizeof(rfslp_t);
	*bufpp	= ucount!=0 ? rfsdata : (rfslp_t *)0;
	return;
}

/*
 * Read in the contents of /etc/dfs/lid_and_priv, performing correctness and
 * consistency checks, and create a list of struct lpgen * to return.
 * Size is set to the total size of the returned array.
 *
 * Errors checked:
 * 1) Only one occurrence of a particular hostname/domainname
 * 2) All hostnames must map to at least one netbuf
 * 3) All level names and privileges must be valid
 *
 * Note: this routine eats memory and doesn't free it, but that should not
 * be a problem as the lidload command will exit soon.
 */
static struct lpgen *
load_lid_and_priv(size, deflid, defpriv, defset)
	u_int *size;
	lid_t *deflid;
	pvec_t *defpriv;
	int *defset;
{
	FILE *lapfp;
	struct lap lapent;
	struct lpgen *hlist, *retbuf = NULL, *tmpbuf;
	int retcount = 0;
	pvec_t priv;
	lid_t lid;
	int count, i;

	if ((lapfp = fopen(LPFILE, "r")) == NULL) {
		/* This printf probably shouldn't be here... */
		perror ("lidload: fopen of /etc/dfs/lid_and_priv");
		return(NULL);
	}

	*deflid = 0;
	*defpriv = 0;

	while (getlpent(lapfp, &lapent) != -1) {
		/*
		 * for each entry, do the following:
		 * 1) Generate a list of addresses for this host (as an array
		 *    of struct lpgen's
		 * 2) Translate the level name into an lid_t
		 * 3) Translate the privilege list into a pvec_t
		 * 4) Store the lid and pvec in each of the new entries,
		 *    and regenerate the final lpgen array with the new entries.
		 */
		count = makelist(lapent.domainname, lapent.hostname, &hlist);

		/*if ((count = makelist(lapent.domainname, lapent.hostname, &hlist)) == 0) {
			pr_err("load_lid_and_priv: no addresses for host %s\n",
				lapent.hostname);
			*size = 0;
			return(NULL);
		}*/

		if (strcmp(lapent.lvlname, "-")) {
			if (lvlin(lapent.lvlname, &lid) == -1) {
				pr_err("invalid level name \"%s\" for host %s\n",
					lapent.lvlname, lapent.hostname);
				*size = 0;
				return(NULL);
			}
		} else
			lid = (lid_t)-1;

		if (strcmp(lapent.privlist, "-")) {
			if (priv2pvec(lapent.privlist, &priv) < 0) {
				pr_err("invalid privilege list \"%s\" for host %s\n",
					lapent.privlist, lapent.hostname);
				*size = 0;
				return(NULL);
			}
		} else
			priv = (pvec_t)-1;
		
		if ( (hlist == NULL) && (count == 1) ) {
			/* default host: set deflid and defpriv */
			if (lid != (lid_t)-1)
				*deflid = lid;
			if (priv != (pvec_t)-1)
				*defpriv = priv;
			*defset = TRUE;
		} else {
			/* a real host: add its entries */
			if (count > 0) {
				retbuf = (struct lpgen *)realloc(retbuf,
					(retcount + count) * sizeof (struct lpgen));
				(void)memcpy(&retbuf[retcount], hlist,
				     count * sizeof(struct lpgen));
				for (tmpbuf = &retbuf[retcount], i = 0; i < count;
			   	   tmpbuf++, i++) {
					tmpbuf->lp_lid = lid;
					tmpbuf->lp_priv = priv;
					tmpbuf->lp_valid = 1;
				}
			} else {
				count = 1;
				retbuf = (struct lpgen *)realloc(retbuf,
					(retcount + count) * sizeof (struct lpgen));
				for (tmpbuf = &retbuf[retcount], i = 0; i < count;
			   	   tmpbuf++, i++) {
					tmpbuf->lp_lid = lid;
					tmpbuf->lp_priv = priv;
					tmpbuf->lp_hostname = lapent.hostname;
					tmpbuf->lp_valid = 0;
				}
			}
			retcount += count;
		}
	}
	/* fill in the defaults for entries looking for default LID or priv */
	for (tmpbuf = retbuf, i = 0; i < retcount; tmpbuf++, i++) {
		if (tmpbuf->lp_lid == -1)
			tmpbuf->lp_lid = *deflid;
		if (tmpbuf->lp_priv == -1)
			tmpbuf->lp_priv = *defpriv;
	}
	*size = retcount * sizeof(struct lpgen);
	return(retbuf);
}

/*
 * makelist: given a hostname, create an lpgen structure
 * list appropriate to that operation.
 * Returns the number of entries, or 0 if an error or no entries.
 */
static int
makelist(domainname, hostname, lpbret)
	char *domainname;
	char *hostname;
	struct lpgen **lpbret;
{
	struct netconfig *nconf;
	_VOID *nch;
	int i, count = 0;
	struct nd_hostserv hostserv;
	struct nd_addrlist *addrlist;
	struct netbuf *buflist = NULL, *tmpbuf, *masklist = NULL, *tmpmask;
	struct lpgen *lpb, *tmplp;

	/* special case : wild card host: return 1 but no lpgen struct */
	if (!strcmp(hostname, "*") &&
	    !strcmp(domainname, "*")) {
		*lpbret = NULL;
		return(1);
	}

	if ((nch = setnetconfig()) == NULL) {
		nc_perror("makelist: setnetconfig");
		*lpbret = NULL;
		return(-1);
	}

	/*
	 * The following loop will obtain successive netconfig entries
	 * and for each one generate an address list.
	 * Each (hostname, netconfig_entry) pair can generate several
	 * address netbufs, and for each netbuf we create an appropriate
	 * mask (or at least we try).
	 * At the loop termination 'buflist' is an array of netbufs with
	 * host addresses and 'masklist' is a corresponding array of netbufs
	 * with address masks. 'Count' is the number of entries in each list.
	 */
	while (nconf = getnetconfig(nch)) {
		if (!(nconf->nc_flag & NC_VISIBLE))
			continue;

		hostserv.h_host = hostname;
		hostserv.h_serv = "rpcbind";	/* XXX - very likely to exist */
		if (netdir_getbyname(nconf, &hostserv, &addrlist) != 0)
			continue;

		if (addrlist->n_cnt <= 0)
			continue;

		buflist = (struct netbuf *)realloc(buflist,
			(count + addrlist->n_cnt) * sizeof(struct netbuf));
		masklist = (struct netbuf *)realloc(masklist,
			(count + addrlist->n_cnt) * sizeof(struct netbuf));
		(void)memcpy(&buflist[count],
			addrlist->n_addrs,
			addrlist->n_cnt * sizeof (struct netbuf));
		(void)memcpy(&masklist[count],
			addrlist->n_addrs,
			addrlist->n_cnt * sizeof (struct netbuf));
		for (i=0, tmpmask = &masklist[count];
		     i < addrlist->n_cnt;
		     i++, tmpmask++) {
			tmpmask->buf = (char *)malloc(tmpmask->maxlen);
			if (!strcmp(nconf->nc_protofmly, NC_INET)) {
				(void)memset(tmpmask->buf, 0, tmpmask->maxlen);
				/*LINTED pointer alignment*/
				((struct sockaddr_in *)tmpmask->buf)->sin_addr.s_addr = (u_long)0xffffffff;
			} else
				(void)memset(tmpmask->buf, 0xff, tmpmask->maxlen);
		}
		count += addrlist->n_cnt;
	}

	(void)endnetconfig(nch);

	if (count == 0)
		return (count);

	/*
	 * Now we have the full set of addresses for this hostname over all
	 * transports. Now create an array of struct lpgen's and fill in
	 * the hostname, addr, and mask fields in each entry.
	 */
	lpb = (struct lpgen *)malloc(count * sizeof (struct lpgen));
	for (i = 0, tmpbuf = buflist, tmpmask = masklist, tmplp = lpb;
	     i < count;
	     i++, tmpbuf++, tmpmask++, tmplp++) {
		/* XXX - need special handling for domain "-" */
		tmplp->lp_hostname = (char *)malloc(strlen(domainname) +
						    strlen(hostname) + 2);
		(void)strcpy(tmplp->lp_hostname, domainname);
		(void)strcat(tmplp->lp_hostname, ".");
		(void)strcat(tmplp->lp_hostname, hostname);
		tmplp->lp_addr = tmpbuf;
		tmplp->lp_mask = tmpmask;
	}

	*lpbret = lpb;
	return (count);
}

static int
getlpent(lpfp, lpent)
	FILE *lpfp;
	struct lap *lpent;
{
	static struct {
		char *domain;
		char *host;
	} foundhosts[1024];
	char line[1024], junk;
	static int numhosts=0;
	int i, ret;


	do {
		if (fgets(line, 1024, lpfp) == NULL)
			return(-1);

	} while (line[0] == '#' || line[0] == '\0');

	if ( (ret = sscanf(line, "%s %s %s %s %s", lpent->domainname, lpent->hostname,
	   lpent->lvlname, lpent->privlist, &junk)) != 4 ) {
		if (ret < 4) {
			pr_err("/etc/dfs/lid_and_priv: line has too few entries\n");
		}
		else {
			pr_err("/etc/dfs/lid_and_priv: line has too many entries\n");
		}
		return(-1);
	}

	/* both domainname and hostname must be wildcard */
	if ( (!strcmp(lpent->domainname, "*") && strcmp(lpent->hostname, "*"))
		|| (!strcmp(lpent->hostname, "*") && strcmp(lpent->domainname, "*")) ) {
		pr_err("/etc/dfs/lid_and_priv: both domainname and hostname must be *\n");
			return(-1);
	}


	/* if domain is "-" we want the local domain */
	if (!strcmp(lpent->domainname, "-"))
		strcpy(lpent->domainname, domainname);
	
	/* Make sure this host hasn't already been found */
	for (i = 0; i < numhosts; i++)
		if (!strcmp(foundhosts[i].host, lpent->hostname) &&
		    !strcmp(foundhosts[i].domain, lpent->domainname)) {
			pr_err("/etc/dfs/lid_and_priv: duplicate host %s in domain %s\n",
				lpent->hostname, lpent->domainname);
			return (-1);
		}
	
	foundhosts[numhosts].host =
			strcpy((char *)malloc(strlen(lpent->hostname) + 1),
				lpent->hostname);
	foundhosts[numhosts++].domain =
			strcpy((char *)malloc(strlen(lpent->domainname) + 1),
				lpent->domainname);
	return(0);
}

static int
priv2pvec(priv, pvec)
	char *priv;
	pvec_t *pvec;
{
	char *c;
	char *p = strdup(priv);
	int ret;

	*pvec = (pvec_t)0;
	if ((c = strtok(p, ",")) == NULL) {
		free(p);
		return (-1);
	}
	while (c != NULL) {
		ret = privnum(c);
		if (ret >= 0) {
			pm_setbits(privnum(c), *pvec);
			c = strtok(NULL, ",");
		} else {
			return (-1);
		}
	}
	return(0);
}

static
#ifdef __STDC__
void
pr_err(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void) fprintf(stderr, "lidload: ");
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
}
#else
void
pr_err(fmt, va_alist)
	char *fmt;
	va_dcl
{
	va_list ap;

	va_start(ap);
	(void) fprintf(stderr, "lidload: ");
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
}
#endif

void
usage()
{
	pr_err("does not take arguments\n");
	(void) fprintf(stderr, "usage: lidload\n");
}
