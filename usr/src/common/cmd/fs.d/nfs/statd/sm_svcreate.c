/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)sm_svcreate.c	1.2"
#ident	"$Header$"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <errno.h>
#ifdef SYSLOG
#include <sys/syslog.h>
#else
#define LOG_ERR 3
#endif				/* SYSLOG */
#include <rpc/nettype.h>
#include <netconfig.h>
#include <netdir.h>
#include <unistd.h>

extern int t_errno;

extern char *strdup();

/*
 * It creates a link list of all the handles it could create.
 * If svc_create_statd() is called multiple times, it uses the handle
 * created earlier instead of creating a new handle every time.
 */
SVCXPRT *
svc_create_statd(dispatch, prognum, versnum, nettype, servname)
	void (*dispatch) ();		/* Dispatch function */
	u_long prognum;			/* Program number */
	u_long versnum;			/* Version number */
	char *nettype;			/* Networktype token */
	char *servname;
{
	struct xlist {
		SVCXPRT *xprt;	/* Server handle */
		struct xlist *next;	/* Next item */
	}    *l;
	static struct xlist *xprtlist;	/* A link list of all the handles */
	int num = 0;
	SVCXPRT *xprt;
	struct netconfig *nconf;
	struct t_bind *bind_addr;
	int fd;
	struct nd_hostserv ns;
	struct nd_addrlist *nas;
	struct t_info tinfo;
	void *net;


	if ((net = _rpc_setconf(nettype)) == NULL) {
		syslog(LOG_ERR, 
		       gettxt(":141", "%s: %s protocol is not known"),
		       "svc_create_statd", nettype);
		return ((SVCXPRT *)NULL);
	}
	while (nconf = _rpc_getconf(net)) {
		if (strcmp(nconf->nc_protofmly, NC_LOOPBACK)
#ifndef LOCAL_ONLY
		    && strcmp(nconf->nc_protofmly, NC_INET)
#endif
		    )
			continue;

		for (l = xprtlist; l; l = l->next) {
			if (strcmp(l->xprt->xp_netid, nconf->nc_netid) == 0) {
				/* Found an  old  one,  use  it
				*/
				(void) rpcb_unset(prognum, versnum, nconf);
				if (svc_reg(l->xprt, prognum, versnum,
					dispatch, nconf) == FALSE)
					syslog(LOG_ERR,
					       gettxt(":142", "%s: could not register (program %d, version %d) on %s"),
					       "svc_create_statd", prognum,
					       versnum, nconf->nc_netid);
				else
					num++;
				break;
			}
		}
		/* It was not found.
		 * Now create a new one */
		if (l == (struct xlist *) NULL) {
			if ((fd = t_open(nconf->nc_device, O_RDWR, &tinfo))
			    < 0) {
				syslog(LOG_ERR,
				       gettxt(":143", "%s: %s cannot open connection: %s"),
				       "svc_create_statd", nconf->nc_netid,
				       t_strerror(t_errno));
				continue;
			}
			bind_addr = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR);
			if ((bind_addr == (struct t_bind *)NULL)) {
				syslog(LOG_ERR, 
				       gettxt(":114", "%s: %s failed"),
				       "svc_create_statd", "t_alloc");
				continue;
			}
			ns.h_host = HOST_SELF;
			ns.h_serv = servname;
			if (!netdir_getbyname(nconf, &ns, &nas)) {
				/* Copy the address */
				bind_addr->addr.len =
				    nas->n_addrs->len;
				memcpy(bind_addr->addr.buf,
				    nas->n_addrs->buf, (int) nas->n_addrs->len);
				netdir_free((char *) nas, ND_ADDRLIST);
			} else {
				/* not an error */
				(void) t_free((char *)bind_addr, T_BIND);
				bind_addr = (struct t_bind *)NULL;
			}


			xprt = svc_tli_create(fd, nconf, bind_addr, 0, 0);
			if (bind_addr)
				(void) t_free((char *)bind_addr, T_BIND);
			if (xprt) {

				/* Made  a  new  one,  use  it */
				(void) rpcb_unset(prognum, versnum, nconf);
				if (svc_reg(xprt, prognum, versnum,
					dispatch, nconf) == FALSE)
					syslog(LOG_ERR,
					       gettxt(":142", "%s: could not register (program %d, version %d) on %s"),
					       "svc_create_statd", prognum,
					       versnum, nconf->nc_netid);
				l = (struct xlist *) malloc(sizeof(struct xlist));
				if (l == (struct xlist *) NULL) {
					syslog(LOG_ERR,
					       gettxt(":96", "%s: no memory"),
					       "svc_create_statd");
					return ((SVCXPRT *)NULL);
				}
				l->xprt = xprt;
				l->next = xprtlist;
				xprtlist = l;
				num++;
				if (strcmp(nconf->nc_protofmly, NC_LOOPBACK)
				    == 0)
					_rpc_negotiate_uid(xprt->xp_fd);
			}
		}
	}
	_rpc_endconf(net);
	/* In case of num == 0; the
	 * error messages are generated
	 * by the underlying layers;
	 * and hence not needed here. */
	return (xprt);
}
