/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)prot_free.c	1.2"
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

/*
 * prot_free.c consists of subroutines that implement the
 * DOS-compatible file sharing services for PC-NFS
 */

#include <stdio.h>
#include "prot_lock.h"

extern int debug;

/* ARGSUSED */
void *
proc_nlm_freeall(Rqstp, Transp)
	struct svc_req *Rqstp;
	SVCXPRT *Transp;
{
	nlm_notify	req;
	/*
	 * Allocate space for arguments and decode them
	 */

	req.name = NULL;
	if (!svc_getargs(Transp, xdr_nlm_notify, (char *)&req)) {
		svcerr_decode(Transp);
		return;
	}

	if (debug)
		printf("proc_nlm_freeall from %s\n", req.name);
	destroy_client_shares(req.name);

	free(req.name);
	svc_sendreply(Transp, xdr_void, NULL);
}
