/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)rpc.c	1.2"
#ident  "$Header$"

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
 * this file consists of routines to support call_rpc();
 * client handles are cached in a hash table;
 * clntudp_create is only called if (site, prog#, vers#) cannot
 * be found in the hash table;
 * a cached entry is destroyed, when remote site crashes
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/time.h>
#include <syslog.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_HASHSIZE 100
#define MAX_HASHRETRIES 10
#define MAX_HASHAGE 20

extern char *xmalloc();
extern int debug;
extern int HASH_SIZE;

struct cache {
	char *host;
	int prognum;
	int versnum;
	int lastproc;
	int retries;
	int age;
	int sock;
	CLIENT *client;
	struct cache *nxt;
};

struct cache *table[MAX_HASHSIZE];
int cache_len = sizeof (struct cache);

hash(name)
	unsigned char *name;
{
	int len;
	int i, c;

	c = 0;
	len = strlen(name);
	for (i = 0; i< len; i++) {
		c = c +(int) name[i];
	}
	c = c %HASH_SIZE;
	return (c);
}

void
delete_hash(host)
	char *host;
{
	struct cache *cp;
	struct cache *cp_prev = NULL;
	struct cache *next;
	int h;

	/*
	 * if there is more than one entry with same host name;
	 * delete has to be recurrsively called
	 */

	h = hash((unsigned char *) host);
	next = table[h];
	while ((cp = next) != NULL) {
		next = cp->nxt;
		if (strcmp(cp->host, host) == 0) {
			if (cp_prev == NULL) {
				table[h] = cp->nxt;
			}
			else {
				cp_prev->nxt = cp->nxt;
			}
			if (debug)
				printf("delete hash entry (%x), %s \n", cp, host);
			if (cp->client)
				clnt_destroy(cp->client);
			if (cp->host)
				free(cp->host);
			if (cp)
				free((char *) cp);
		}
		else {
			cp_prev = cp;
		}
	}
}

/*
 * find_hash returns the cached entry;
 * it returns NULL if not found;
 */
struct cache *
find_hash(host, prognum, versnum, procnum)
	char *host;
	int prognum, versnum, procnum;
{
	struct cache *cp;

	cp = table[hash((unsigned char *) host)];
	while ( cp != NULL) {
		if (strcmp(cp->host, host) == 0 &&
		 cp->prognum == prognum && cp->versnum == versnum) {

			if ( (cp->lastproc == procnum && 
			      ++cp->retries > MAX_HASHRETRIES)
			    || (++cp->age > MAX_HASHAGE)) {
				if (debug) printf("deleting hash host %s lastproc %d procnum %d retries %d age %d\n",host,cp->lastproc,procnum,cp->retries,cp->age);
				delete_hash(host);
				return (NULL);
			}
			cp->lastproc = procnum;
			cp->retries = 0;
			return (cp);
		}
		cp = cp->nxt;
	}
	return (NULL);
}
struct cache *
make_hash_entry(char *host)
{
	struct cache *cp;
	if ((cp = (struct cache *) xmalloc((u_int) cache_len)) == NULL ) {
		return (NULL);	/* malloc error */
	}
	if ((cp->host = xmalloc((u_int) (strlen(host)+1))) == NULL ) {
		if (cp)
			free((char *) cp);
		return (NULL);	/* malloc error */
	}
	return cp;
}

/* Given hash entry, cp, initialize and add to hash table */
void
add_hash(host, prognum, versnum, procnum, clientp, cp)
	char *host;
	int prognum, versnum, procnum;
	struct cache *cp;
	CLIENT *clientp;
{
	int h;

	(void) strcpy(cp->host, host);
	cp->client  = clientp;
	cp->prognum = prognum;
	cp->versnum = versnum;
	cp->lastproc = procnum;
	cp->retries = 0;
	cp->age = 0;

	h = hash((unsigned char *) host);
	cp->nxt = table[h];
	table[h] = cp;
}

call_rpc(host, prognum, versnum, procnum, inproc, in, outproc, out, valid_in, t)
	char *host;
	u_long prognum, versnum;
	int	procnum;
	xdrproc_t inproc, outproc;
	char *in, *out;
	int valid_in;
	int t;
{
        enum clnt_stat clnt_stat;
        struct timeval timeout, tottimeout;
        struct cache *cp;
	struct t_info tinfo;
        int fd;
	extern int t_errno;
	CLIENT *clientp;

	if (debug)
		printf("enter call_rpc() ...\n");

	if (valid_in == 0) delete_hash(host);

	if ((cp = find_hash(host, (int) prognum, (int) versnum, (int) procnum)) == (struct cache *)NULL) {
		if( (cp = (struct cache *)make_hash_entry(host)) == (struct cache *)NULL){
			syslog(LOG_ERR, 
			       gettxt(":137", "%s: cannot send due to out of cache"), 
			       "call_rpc");
			return (-1);
		}
	       /*
	        * we MUST use datagram_v as connection oriented transports
	        * will cause deadlock. both the client and server will be
	        * trying to connect to each other when creating the client
	        * handle. the client while sending a request, and the server
	        * while creating a client handle to send the results of an
	        * earlier request back.
	        */
		if( (clientp = clnt_create(host, prognum, versnum, "datagram_v")) == (CLIENT *)NULL){
			syslog(LOG_ERR, 
			       gettxt(":136", "%s: server not responding for %s"),
			       host, clnt_spcreateerror("clnt_create"));
			return -1;
		}

		add_hash(host, (int) prognum, (int) versnum, (int) procnum, clientp, cp);

		if (debug)
			printf("(%x):[%s, %d, %d] is a new connection\n",
				cp, host, prognum, versnum);

		(void) CLNT_CONTROL(cp->client, CLGET_FD, (char *)&fd);
		if (t_getinfo(fd, &tinfo) != -1) {
			if (tinfo.servtype == T_CLTS) {
				/*
				 * Set time outs for connectionless case
				 */
				timeout.tv_usec = 0;
				timeout.tv_sec = 15;
				(void) CLNT_CONTROL(cp->client,
					CLSET_RETRY_TIMEOUT, (char *)&timeout);
			}
		} else {
			rpc_createerr.cf_stat = RPC_TLIERROR;
			rpc_createerr.cf_error.re_terrno = t_errno;
			delete_hash(host);
			return (rpc_createerr.cf_stat);
		}
	} else {
		if (valid_in == 0) { /* cannot use cache */
			if (debug)
				printf("(%x):[%s, %d, %d] is a new conn\n",
					cp, host, prognum, versnum);

			if (cp->client == (CLIENT *)NULL) {
                               /*
                                * we MUST use datagram_v as connection
                                * oriented transports will cause deadlock.
                                * both the client and server will be
                                * trying to connect to each other when
                                * creating the client handle. the client
                                * while sending a request, and the server
                                * while creating a client handle to send
                                * the results of an earlier request back.
                                */
				cp->client = clnt_create(host, prognum,
					versnum, "datagram_v");
			}

                	if (cp->client == (CLIENT *)NULL) {
                        	return (rpc_createerr.cf_stat);
                	}
                	(void) CLNT_CONTROL(cp->client, CLGET_FD, (char *)&fd);
                	if (t_getinfo(fd, &tinfo) != -1) {
                        	if (tinfo.servtype == T_CLTS) {
                                	/*
                                 	 * Set time outs for connectionless case
                                 	 */
                                	timeout.tv_usec = 0;
                                	timeout.tv_sec = 15;
                                	(void) CLNT_CONTROL(cp->client,
                                       	CLSET_RETRY_TIMEOUT, (char *)&timeout);
                        	}                          
                	} else { 
                        	rpc_createerr.cf_stat = RPC_TLIERROR;
                        	rpc_createerr.cf_error.re_terrno = t_errno;
				delete_hash(host);
                        	return (rpc_createerr.cf_stat);
                	}
		}
	}

	tottimeout.tv_sec = t;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(cp->client, procnum, inproc, in,
	    outproc, out, tottimeout);
	if (debug) {
		printf("clnt_stat=%d\n", (int) clnt_stat);
	}

	return ((int) clnt_stat);
}
