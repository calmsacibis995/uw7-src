#ident	"@(#)gtphostent.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: gtphostent.c,v 1.10 1995/02/16 05:54:28 neil Exp"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Legent Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*      SCCS IDENTIFICATION        */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/timeb.h>
#include <time.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/syslog.h>
#include <sys/conf.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#ifndef _KERNEL
#define _KERNEL
#include <netinet/in_comp.h>
#undef _KERNEL
#else
#include <netinet/in_comp.h>
#endif

#ifndef _KERNEL
#define _KERNEL
#include <sys/ksynch.h>
#undef _KERNEL
#else
#include <sys/ksynch.h>
#endif

#include <netinet/ppp_lock.h>
#include <netinet/ppp.h>
#include "pppd.h"
#include "pppu_proto.h"
#include "pathnames.h"

/* Address Pooling API */
#include "pool.h"
#include "pool_proto.h"
#include <errno.h>

#define bcmp(s1, s2, len)	memcmp(s1, s2, len)
#define bcopy(s1, s2, len)	memcpy(s2, s1, len)
#define bzero(str,n)            memset((str), (char) 0, (n))

#include <pfmt.h>
#include <locale.h>
#include <unistd.h>

char	*_ppphost_file = _PATH_PPPHOSTS;
char	*_auth_file = _PATH_PPPAUTH;
char	*_pool_file = _PATH_PPPPOOL;

#define IS_SEPARATOR(x) (((x) == '\t') || ((x) == '\n') || ((x) == ' ') || ((x) == '\0'))

/* open ppphost file */
FILE *
setppphostent()
{
	FILE *fp;

	if ((fp = fopen(_ppphost_file, "r" )) == NULL) {
		ppp_syslog(LOG_WARNING, gettxt(":35", "setppphostent: fopen %s fail"), _ppphost_file);
	}
	return (fp);
}

/* close ppphost file */
endppphostent(fp)
FILE *fp;
{
	if (fp) {
		fclose(fp);
	}
}

/* read an entry from ppphost file and put in struct ppphost
 * return value:
 *   0:	end of file
 *  -1: bad entry
 *   1: succeed
 */
int
getppphostent(fp, hp)
FILE *fp;
struct ppphostent *hp;
{
	char *p, *ps, *ptr, *ptr1, *lps;
	char *tmp_ptr;
	struct hostent *hostptr;
	char address[NAME_SIZE+1];
	char line[PPPBUFSIZ+1];
	char entry[PPPBUFSIZ+1];

	if (fp == NULL) {
		ppp_syslog(LOG_WARNING, gettxt(":36", "getppphostent: bad file descriptor"));
		return (0);
	}
again:
	if ((p = pppfgets(line, BUFSIZ, fp)) == NULL)
		return (0);
	if (*p == '\0')
		goto again;

	/* initialize the returning ppphost */ 
	hp->loginname[0] = '\0';
	hp->device[0] = '\0';
	hp->attach[0] = '\0';
	bzero(&hp->ppp_cnf.remote, sizeof(struct sockaddr_in));
	hp->ppp_cnf.remote.sin_family = AF_INET;
	bzero(&hp->ppp_cnf.local, sizeof(struct sockaddr_in));
	hp->ppp_cnf.local.sin_family = AF_INET;
	bzero(&hp->ppp_cnf.mask, sizeof(struct sockaddr_in));
	hp->ppp_cnf.mask.sin_family = AF_INET;
	hp->ppp_cnf.mask.sin_addr.s_addr = inet_addr(DEF_MASK);

	/* ps points to the first field */
	ps = strtok(p, " \t\n");
	bcopy(p, entry, strlen(p) + 1);


	/* process the first field 
	 * 1. incoming call: *login_name
	 * 2. outgoing call: remote:local 
	 */
	if (*ps == ASTERISK) { 	/* for incoming call */		
		if (*(ps+1)) {
			bcopy(ps+1, hp->loginname, sizeof(hp->loginname));
		} else {
			ppp_syslog(LOG_WARNING, gettxt(":37", "getppphostent: no login name"));
			return(-1);
		}
	}
	else {	/* for outgoing call */
		/* THIS STUFF IS INTERNET SPECIFIC */
		lps = strchr(ps, COLON);
		if (!lps) {
			ppp_syslog(LOG_WARNING, gettxt(":38", "getppphostent for '%s': no colon between remote and local address"), entry);
			return(-1);
		}
		*lps = '\0';
		lps ++;
		if (*lps == '\0') {
			ppp_syslog(LOG_WARNING,gettxt(":39", "getppphostent for '%s': no local address"), entry);
			return(-1);
		}

		if (*ps == '\0') {
			ppp_syslog(LOG_WARNING,gettxt(":40", "getppphostent for '%s': no remote address"), entry);
			return(-1);
		}

		/* set the default uucp name to be remote host name */
		bcopy(ps, hp->uucp_system, sizeof(hp->uucp_system));

		/* set the remote address */
		if (getaddr(ps, &hp->ppp_cnf.remote) < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":41", "getppphostent: get %s address fail"), ps);
			return(-1);
		}
		/* set the local address */
		if (getaddr(lps, &hp->ppp_cnf.local) < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":41", "getppphostent: get %s address fail"), lps);
			return(-1);
		}
		ps = lps;
	}

	/* process the rest fields */
	for (ptr = ps; *ptr != '\0'; ptr++);
	ptr++;

	/*
	 * Check for paramters specified without a value (e.g. accm=)
	 */

	if ((strstr(ptr, "= ") != NULL) ||
	    (strstr(ptr, "=\n") != NULL) ||
	    (strstr(ptr, "=\t") != NULL)) {
	  ppp_syslog(LOG_ERR, 
		     gettxt(":42", "Value not specified for parameter(s) in entry %s in %s\n"),
		     entry, _PATH_PPPHOSTS);
	  return(-1);
	}


	/* process "attach=" field */
	if ((ptr1 = strstr(ptr, ATTACH)) != NULL) {
		mystrcpy(hp->attach, ptr1 + strlen(ATTACH));
	}

	/* process "local=" field */

	if (hp->ppp_cnf.local.sin_addr.s_addr == 0) { 
		if ((ptr1 = strstr(ptr, LADDR)) != NULL) {
			ptr1 += strlen(LADDR);
			if (*ptr1 == PLUS) { 
				/* address pooling */
				ptr1++;
				if (strlen(ptr1) <= NAME_SIZE) 
					mystrcpy(hp->pool_tag_local, ptr1);
				else
					hp->pool_tag_local[0] = '\0';
			} else {
				hp->pool_tag_local[0] = '\0';
				mystrcpy(address, ptr1);
				getaddr(address, &hp->ppp_cnf.local);
			}
		}
	}

	/* process "remote=" field */
	if (hp->ppp_cnf.remote.sin_addr.s_addr == 0) { 
		if ((ptr1 = strstr(ptr, RADDR)) != NULL) {
			ptr1 += strlen(RADDR);
			if (*ptr1 == PLUS) { 
				/* address pooling */
				ptr1++;
				if (strlen(ptr1) <= NAME_SIZE) 
					mystrcpy(hp->pool_tag_remote, ptr1);
				else
					hp->pool_tag_remote[0] = '\0';
			} else {
				hp->pool_tag_remote[0] = '\0';
				mystrcpy(address, ptr1);
				getaddr(address, &hp->ppp_cnf.remote);
			}
		}
	}

	/* process "mask=" field */
	if ((ptr1 = strstr(ptr, MASK)) != NULL) {
		mystrcpy(address, ptr1 + strlen(MASK));
		getaddr(address, &hp->ppp_cnf.mask);
	}

	/* if this isn't a static link, process "uucp=" field */
	if ((ptr1 = strstr(ptr, gettxt(":43", "staticdev="))) == NULL) {
		if((ptr1 = strstr(ptr, UUCPNAME)) != NULL) 
			mystrcpy(hp->uucp_system, ptr1 + strlen(UUCPNAME));
	} else 
		mystrcpy(hp->device, ptr1 + strlen(STATICDEV));

	/* process "speed=" field */
	if ((ptr1 = strstr(ptr, SPEED)) != NULL) {
		mystrcpy(hp->speed, ptr1 + strlen(SPEED));
	} else
		strcpy(hp->speed, DEF_SPEED);

	/* process "filter=" field */
	if ((ptr1 = strstr(ptr, FILTER)) != NULL) {
		mystrcpy(hp->tag, ptr1 + strlen(FILTER));
	} else {
		if (hp->device[0] == '\0') 
			strcpy(hp->tag, DEF_TAG_DYN);
		else
			strcpy(hp->tag, DEF_TAG_DED);
	}

	if ((ptr1 = strstr(ptr, IDLE)) == NULL)
		hp->ppp_cnf.inactv_tmout = 0;
	else
		hp->ppp_cnf.inactv_tmout = atoi(ptr1+ strlen(IDLE));

	if ((ptr1 = strstr(ptr, REQTMOUT)) == NULL)
		hp->ppp_cnf.restart_tm = 0;
	else
		hp->ppp_cnf.restart_tm = atoi(ptr1 + strlen(REQTMOUT));
	
	if ((ptr1 = strstr(ptr, CONF)) == NULL)
		hp->ppp_cnf.max_cnf = 0;
	else
		hp->ppp_cnf.max_cnf = atoi(ptr1 + strlen(CONF));

	if ((ptr1 = strstr(ptr, TERM)) == NULL)
		hp->ppp_cnf.max_trm = 0;
	else
		hp->ppp_cnf.max_trm = atoi(ptr1 + strlen(TERM));
	
	if ((ptr1 = strstr(ptr, NAK)) == NULL)
		hp->ppp_cnf.max_failure = 0;
	else
		hp->ppp_cnf.max_failure = atoi(ptr1 + strlen(NAK));
	
	if ((ptr1 = strstr(ptr, MRU)) == NULL)
		hp->ppp_cnf.mru = DEF_MRU;
	else
		hp->ppp_cnf.mru = atoi(ptr1 + strlen(MRU));
	
	if ((ptr1 = strstr(ptr, ACCM)) == NULL)
		hp->ppp_cnf.accm = DEF_ACCM;
	else {
		hp->ppp_cnf.accm = strtoul((ptr1 + strlen(ACCM)), 
						&tmp_ptr, 16);

		if (! IS_SEPARATOR(*tmp_ptr)) {
		  ppp_syslog(LOG_ERR, gettxt(":44", "Invalid accm for entry '%s' in %s\n"),
			     entry, _PATH_PPPHOSTS);
		  return(-1);
		}
		  
	      }

	/* User can specify either PAP or CHAP but not both */ 	
	hp->ppp_cnf.pap = 0;
	hp->ppp_cnf.chap = 0;
	if ((ptr1 = strstr(ptr, AUTH))) {
		if (!strncmp(ptr1 + strlen(AUTH), AUTHPAP, strlen(AUTHPAP))) 
			hp->ppp_cnf.pap = 1;
		else {
#if defined(DOCHAP)
			if (!strncmp(ptr1 + strlen(AUTH), AUTHCHAP, strlen(AUTHCHAP))) 
				hp->ppp_cnf.chap = unique();
			else
				ppp_syslog(LOG_WARNING, gettxt(":45", "getppphostent for '%s': bad authentication protocol"), entry);
#else
			ppp_syslog(LOG_WARNING, gettxt(":45", "getppphostent for '%s': bad authentication protocol"), entry);
#endif
		}
	}
	
	if (strstr(ptr, NOMGC) == NULL) 
		hp->ppp_cnf.mgc = unique();
	else
		hp->ppp_cnf.mgc = 0;
	
	if (strstr(ptr, NOPROTCOMP) == NULL)
		hp->ppp_cnf.protcomp = 1;
	else
		hp->ppp_cnf.protcomp = 0;

	if (strstr(ptr, NOACCOMP) == NULL)
		hp->ppp_cnf.accomp = 1;
	else
		hp->ppp_cnf.accomp = 0;

	if (strstr(ptr, NOIPADDR) == NULL)
		hp->ppp_cnf.ipaddress = 1;
	else
		hp->ppp_cnf.ipaddress = 0;

	if (strstr(ptr, RFC1172ADDR) == NULL)
		hp->ppp_cnf.newaddress = 1;
	else
		hp->ppp_cnf.newaddress = 0;

	if (strstr(ptr, NOVJ) == NULL)
		hp->ppp_cnf.vjcomp = 1;
	else
		hp->ppp_cnf.vjcomp = 0;

	if (strstr(ptr, NOSLOTCOMP) == NULL)
		hp->ppp_cnf.vjc_slot_comp = 1;
	else
		hp->ppp_cnf.vjc_slot_comp = 0;

	if ((ptr1 = strstr(ptr, MAXSLOT)) == NULL)
		hp->ppp_cnf.vjc_max_slot = MAX_STATES; 
	else {
		hp->ppp_cnf.vjc_max_slot = atoi(ptr1 + strlen(MAXSLOT));
		if (hp->ppp_cnf.vjc_max_slot > MAX_STATES || 
			hp->ppp_cnf.vjc_max_slot < MIN_STATES) {
			ppp_syslog(LOG_WARNING, gettxt(":46", "getppphostent: max slot:%d out of range"), hp->ppp_cnf.vjc_max_slot);
			hp->ppp_cnf.vjc_max_slot = MAX_STATES; 
		}
	}

	if (strstr(ptr, OLD) == NULL)
		hp->ppp_cnf.old_ppp = 0;
	else
		hp->ppp_cnf.old_ppp = 1;

	if ((ptr1 = strstr(ptr, AUTHTMOUT)) == NULL)
		hp->ppp_cnf.auth_tmout = 0;
	else
		hp->ppp_cnf.auth_tmout = atoi(ptr1 + strlen(AUTHTMOUT));

	hp->flow = RTSCTS;
	if ((ptr1 = strstr(ptr, FLOW))) {
		if (!strncmp(ptr1 + strlen(FLOW), HFLOW , strlen(HFLOW))) 
			hp->flow = RTSCTS;
		else {
			if (!strncmp(ptr1 + strlen(FLOW), SFLOW , strlen(SFLOW))) 
				hp->flow = XONXOFF;
		}
	}

	if (strstr(ptr, CLOCL))
		hp->clocal = 1;
	else 
		hp->clocal = 0;

	if ((ptr1 = strstr(ptr, RETRY)) == NULL)
		hp->uucpretry = DEF_UUCPRETRY;
	else 
		hp->uucpretry = atoi(ptr1 + strlen(RETRY));
			
	if ((ptr1 = strstr(ptr, DEBUGGING)) == NULL)
		hp->ppp_cnf.debug = 0;
	else {
		hp->ppp_cnf.debug = atoi(ptr1 + strlen(DEBUGGING));
		if (hp->ppp_cnf.debug > 2 || hp->ppp_cnf.debug < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":47", "getppphostent: debug level:%d out of range"), hp->ppp_cnf.debug);
			hp->ppp_cnf.debug = 0;
		}
	}

	if ((ptr1 = strstr(ptr, PROXY)) == NULL)
		hp->proxy = 0;
	else 
		hp->proxy = 1;
	
	/* get the password for the local host */
	if ((ptr1 = strstr(ptr, NAME)) == NULL)
		hp->ppp_cnf.pid_pwd.PID[0] = ASTERISK; 
	else
		mystrcpy(hp->ppp_cnf.pid_pwd.PID, ptr1 + strlen(NAME));
	
	if (papgetpwd(hp->ppp_cnf.pid_pwd.PID, hp->ppp_cnf.pid_pwd.PWD) < 0){
	        if (hp->ppp_cnf.pid_pwd.PID[0] == '*') {
		  ppp_syslog(LOG_WARNING, 
			     gettxt(":48", "getppphostent: No default local host ID in %s"),
			     _PATH_PPPAUTH);

		  ppp_syslog(LOG_WARNING, 
		     gettxt(":49", "NOTE: using local hostname as host id and passwd for entry '%s'"),
			     entry);
		}
		else {
		  ppp_syslog(LOG_WARNING, 
			     gettxt(":50", "ERROR: No local host ID in %s that corresponds to name=%s as specified in %s"), 
			     _PATH_PPPAUTH, hp->ppp_cnf.pid_pwd.PID, _PATH_PPPHOSTS);
		  
		  return(-1);

		}
	
		/* local host ID is not set, use host name instead */ 	
		if (gethostname(hp->ppp_cnf.pid_pwd.PID, PID_SIZE) < 0)	{	
			ppp_syslog(LOG_WARNING, gettxt(":51", "getppphostent for '%s': gethostname fail: %m"), entry);
			return(-1);
		}
		if (gethostname(hp->ppp_cnf.pid_pwd.PWD, PWD_SIZE) < 0)	{	
			ppp_syslog(LOG_WARNING, gettxt(":51", "getppphostent for '%s': gethostname fail: %m"), entry);
			return(-1);
		}
		return(1);
	}

	
	return (1);
}

/*
 * Get password for a host
 *	if pid[0] == '*'
 *		return password for default local host 
 *	return 0: fail to find the password
 *	return 1: succeed
 */ 
int
papgetpwd(pid, pwd)
	char pid[];
	char pwd[];
{
	char *p ,*ps;
 	char	line[PPPBUFSIZ+1];
	FILE	*authf;

	pwd[0] = '\0';

	if ((authf = fopen(_auth_file, "r")) == NULL) { 
		ppp_syslog(LOG_WARNING, gettxt(":52", "can't open %s:%m"), _auth_file);
		return(-1);
	}

again: if ((p = pppfgets(line, PPPBUFSIZ, authf)) == NULL) {
		fclose(authf); 
		if (pid[0] == ASTERISK)
			ppp_syslog(LOG_WARNING, gettxt(":53", "can't get passwd for local host"));
		else	
			ppp_syslog(LOG_WARNING, gettxt(":54", "can't get passwd for host %s"), pid);
		return(-1);
	}
	if (*p == '\0')
		goto again;

	ps = strtok(p, " \t");
	if (pid[0] == ASTERISK) {
		if (*ps == ASTERISK) {
			strncpy(pid, ps + 1, PID_SIZE - 1);
			pid[PID_SIZE - 1] = '\0';
			ps = strtok(NULL, " \t");
			strncpy(pwd, ps, PWD_SIZE - 1);
			pwd[PWD_SIZE - 1] = '\0';
			fclose(authf); 
		} else
			goto again;
	} else {
		if (*ps == ASTERISK)
			ps++; 
		if ((strlen(ps) == strlen(pid)) 
			&& (memcmp(ps, pid, strlen(ps)) == 0)) {
			ps = strtok(NULL, " \t");
			strncpy(pwd, ps, PWD_SIZE - 1);
			pwd[PWD_SIZE - 1] = '\0';
			fclose(authf); 
		} else
			goto again;
	}
	return(0);
}

extern	int	ipfd;

/* 
 * get an unused ip address pair from the pool
 * return
 *	-1: fail
 *	0: succeed
 */
int
getaddress(tag, address)
	char tag[];
	struct sockaddr_in	*address;
{
  int r;

  r = pool_addr_alloc(tag, POOL_IP, address, 
		      sizeof(struct sockaddr_in));
  if (r == 0)
    return(0);

  if (errno == EFAULT)
    ppp_syslog(LOG_ERR, gettxt(":55", "%s is not a valid address pool tag"), 
	       tag);
  else if (errno == EAGAIN)
    ppp_syslog(LOG_ERR, 
	       gettxt(":56", "There are no addresses currently available in pool %s"),
	       tag);
  else
    ppp_syslog(LOG_ERR, 
	       gettxt(":57", "Error allocating address from  pool %s (errno = %d)") ,
	       tag, errno);

  return(-1);

}

/*
 * turn an IP address string (either dotted decimal or name) into
 * a sockaddr_in.
 */
getaddr(s, sin)
	char	*s;
	struct sockaddr_in	*sin;
{
	struct hostent	*hp;
	int val;
	int r = 0;
	unsigned long 	l;

	bzero(sin, sizeof(struct sockaddr_in));

	l = inet_addr(s);
	
	if (l == INADDR_NONE) {  /* not dot format */ 
		hp = gethostbyname(s);
		if (hp) {
			sin->sin_family = hp->h_addrtype;
			bcopy(hp->h_addr, (char *)&sin->sin_addr, hp->h_length);
		} else {
			ppp_syslog(LOG_WARNING, gettxt(":58", "gethostbyname(%s) fail: %m"),s);
			return(-1);
		}
	} else {	/* dot format */
		sin->sin_family = AF_INET;
		r = inet_aton(s, &val);
		if (r != -1) {
			sin->sin_addr.s_addr = val;
		} else {
			ppp_syslog(LOG_WARNING,gettxt(":59", "inet_aton(%s) fail: %m"),s);
			return(-1);
		}
	}
	return(0);
}

void
mystrcpy(s,t)
char *s, *t;
{
	int size;

	size = strcspn(t, " \t\n");
	strncpy(s,t,size);
	*(s + size) = '\0';
}	

/* Generate a unique number */
int
unique()
{
	struct	tm *t;
	long	tloc;
	int	number, namelen, i;
	char	name[MAXHOSTNAMELEN];

	time(&tloc);
	t = localtime(&tloc);
	number = t->tm_sec * t->tm_mon + t->tm_min * t->tm_hour - t->tm_mday;
	
	if (gethostname(name, sizeof(namelen)) == 0)
		for (i = 0 ; i < MAXHOSTNAMELEN & name[i] != '\0'; i++)
			number += name[i];

	/* Make sure it's not zero */  
	if (number == 0) 
		number = 33;

	return(number);
}
