/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)nfs_cast.c	1.2"
#ident	"$Header$"

/*
 * nfs_cast: broadcast to a specific group of NFS servers
 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#define _NSL_RPC_ABI
#include <rpc/rpc.h>
#include <syslog.h>
#include <rpc/clnt_soc.h>
#include <rpc/nettype.h>
#include <netconfig.h>
#include <netdir.h>
#include "nfs_prot.h"
#include <nfs/nfs_clnt.h>
#include <unistd.h>

#define NFSCLIENT
#include <nfs/mount.h>
#include "automount.h"


extern int verbose;
extern int trace;

void free_transports();
void calc_resp_time();
int choose_best_server();

#define PENALTY_WEIGHT		100000
#define UDPMSGSIZE_CHAR		UDPMSGSIZE*sizeof(char)

static struct tstamps {
	struct tstamps          *ts_next;
	int                     ts_penalty;
	int                     ts_inx;
	int                     ts_rcvd;
	struct timeval          ts_timeval;
};

/* A list of addresses - all belonging to the same transport */

static struct addrs {
	struct addrs		*addr_next;
	int                     addr_inx;
	struct nd_addrlist	*addr_addrs;
	struct tstamps          *addr_if_tstamps;
};

/* A list of connectionless transports */

static struct transp {
	struct transp		*tr_next;
	int			tr_fd;
	char			*tr_device;
	struct t_bind		*tr_taddr;
	struct addrs		*tr_addrs;
};

/*
 * This routine is designed to be able to "ping"
 * a list of hosts to find the host that is
 * up and available and responds fastest.
 * This must be done without any prior
 * contact with the host - therefore the "ping"
 * must be to a "well-known" address.  The outstanding
 * candidate here is the address of "rpcbind".
 *
 * A response to a ping is no guarantee that the host
 * is running NFS, has a mount daemon, or exports
 * the required filesystem.  If the subsequent
 * mount attempt fails then the host will be marked
 * "ignore" and the host list will be re-pinged
 * (sans the bad host). This process continues
 * until a successful mount is achieved or until
 * there are no hosts left to try.
 */
enum clnt_stat 
nfs_cast(host_array, best_host, timeout)
	struct host_names *host_array;
	struct host_names *best_host;	/* set best host to use */
	int timeout;			/* timeout (sec) */
{
	enum clnt_stat stat;
	AUTH *sys_auth = authsys_create_default();
	XDR xdr_stream;
	register XDR *xdrs = &xdr_stream;
	int outlen;
	int if_inx;
	int flag;
	int sent, addr_cnt, rcvd, if_cnt;
	fd_set readfds, mask;
	register u_long xid;		/* xid - unique per addr */
	register int i;
	struct rpc_msg msg;
	struct timeval t, rcv_timeout;
	/* char outbuf[UDPMSGSIZE], inbuf[UDPMSGSIZE]; */
	char *outbuf, *inbuf;
	struct t_unitdata t_udata, t_rdata;
	struct nd_hostserv hs;
	struct nd_addrlist *retaddrs;
	struct transp *tr_head = NULL;
	struct transp *trans, *prev_trans = NULL;
	struct addrs *a, *prev_addr = NULL;
	struct tstamps *ts, *prev_ts = NULL;
	NCONF_HANDLE *nc = NULL;
	struct netconfig *nconf;
	struct rlimit rl;
	int dtbsize;
	extern int right_pid;

	if (trace > 1)
		fprintf(stderr, "nfs_cast: \n");

	outbuf = (char *)malloc(UDPMSGSIZE_CHAR);
	if (outbuf == NULL) {
		syslog(LOG_ERR, gettxt(":245", "%s: no memory for %s"),
		       "nfs_cast", "outbuf");
		stat = RPC_CANTSEND;
		return (stat);
	}
	inbuf = (char *)malloc(UDPMSGSIZE_CHAR);
	if (inbuf == NULL) {
		free(outbuf);
		syslog(LOG_ERR, gettxt(":245", "%s: no memory for %s"),
		       "nfs_cast", "inbuf");
		stat = RPC_CANTSEND;
		return (stat);
	}

	/*
	 * For each connectionless transport get a list of
	 * host addresses.  Any single host may have
	 * addresses on several transports.
	 */
	addr_cnt = sent = rcvd = 0;
	nc = setnetconfig();
	if (nc == NULL) {
		stat = RPC_CANTSEND;
		goto done_broad;
	}
	FD_ZERO(&mask);

	if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
		dtbsize = rl.rlim_cur;
	else
		dtbsize = FD_SETSIZE;

	while (nconf = getnetconfig(nc)) {
		if (!(nconf->nc_flag & NC_VISIBLE) ||
		    nconf->nc_semantics != NC_TPI_CLTS ||
		    (strcmp(nconf->nc_protofmly, NC_LOOPBACK) == 0))
			continue;

		trans = (struct transp *) malloc (sizeof(*trans));
		if (trans == NULL) {
			syslog(LOG_ERR, gettxt(":245", "%s: no memory for %s"),
			       "nfs_cast", "trans");
			stat = RPC_CANTSEND;
			goto done_broad;
		}
		memset (trans, 0, sizeof(*trans));
		if (tr_head == NULL)
			tr_head = trans;
		else
			prev_trans->tr_next = trans;

		trans->tr_fd = t_open(nconf->nc_device, O_RDWR, NULL);
		if (trans->tr_fd < 0) {
			syslog(LOG_ERR,
			       gettxt(":94", "%s: %s failed for %s: %m"),
			       "nfs_cast", "t_open", nconf->nc_device);
			if (tr_head == trans)
				tr_head = NULL;
			else
				prev_trans->tr_next = NULL;
			free(trans);
			continue;
		}

		prev_trans = trans;

		if (t_bind(trans->tr_fd, (struct t_bind *) NULL, 
			   (struct t_bind *) NULL) < 0) {
			syslog(LOG_ERR,
			       gettxt(":94", "%s: %s failed for %s: %m"),
			       "nfs_cast", "t_bind", nconf->nc_device);
			stat = RPC_CANTSEND;
			goto done_broad;
		}

		trans->tr_taddr = 
		   (struct t_bind *) t_alloc(trans->tr_fd, T_BIND, T_ADDR);
		if (trans->tr_taddr == (struct t_bind *) NULL) {
			syslog(LOG_ERR,
			       gettxt(":94", "%s: %s failed for %s: %m"),
			       "nfs_cast", "t_alloc", nconf->nc_device);
			stat = RPC_SYSTEMERROR;
			goto done_broad;
		}

		trans->tr_device = nconf->nc_device;
		FD_SET(trans->tr_fd, &mask);
		
		if_inx = 0;
		for (i = 0; host_array[i].host != NULL; i++) {
			hs.h_host = host_array[i].host;
			hs.h_serv = "rpcbind";
			if (trace > 1)
				fprintf(stderr,
					"nfs_cast: host_array[%d].host=%s\n",
					i, host_array[i].host);
			if (netdir_getbyname(nconf, &hs, &retaddrs) == ND_OK) {
				a = (struct addrs *) malloc (sizeof(*a));
				if (a == NULL) {
					syslog(LOG_ERR,
					       gettxt(":96", "%s: no memory"),
					       "nfs_cast");
					stat = RPC_CANTSEND;
					goto done_broad;
				}
				memset (a, 0, sizeof(*a));
				if (trans->tr_addrs == NULL)
					trans->tr_addrs = a;
				else
					prev_addr->addr_next = a;
				prev_addr = a;
				a->addr_if_tstamps = NULL;
				a->addr_inx = i;
				a->addr_addrs = retaddrs;
				if_cnt = retaddrs->n_cnt;
				while (if_cnt--) {
					ts = (struct tstamps *) malloc (sizeof(*ts));
					if (ts == NULL) {
						syslog(LOG_ERR,
						       gettxt(":96", "%s: no memory"),
						       "nfs_cast");
						stat = RPC_CANTSEND;
						goto done_broad;
					}			
					memset (ts, 0, sizeof(*ts));
					ts->ts_penalty = host_array[i].penalty;
					if (a->addr_if_tstamps == NULL)
						a->addr_if_tstamps = ts;
					else
						prev_ts->ts_next = ts;
					prev_ts = ts;
					ts->ts_inx = if_inx++;
					addr_cnt++;
				}
			} else {
				if (verbose)
					syslog(LOG_ERR, gettxt(":233",
							       "%s: %s address is not known"),
					       host_array[i].host, nconf->nc_netid);
			}
		}
	} /* end while */
	if (addr_cnt == 0) {
		syslog(LOG_ERR, gettxt(":234", "%s: could not find addresses"),
		       "nfs_cast");
		stat = RPC_CANTSEND;
		goto done_broad;
	}

	(void) gettimeofday(&t, (struct timezone *) 0);
	xid = (right_pid ^ t.tv_sec ^ t.tv_usec) & ~0xFF;
	t.tv_usec = 0;

	/* serialize the RPC header */

	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = RPCBPROG;
	msg.rm_call.cb_vers = RPCBVERS;	/* any version will do */
	msg.rm_call.cb_proc = NULLPROC;
	if (sys_auth == (AUTH *) NULL) {
		stat = RPC_SYSTEMERROR;
		goto done_broad;
	}
	msg.rm_call.cb_cred = sys_auth->ah_cred;
	msg.rm_call.cb_verf = sys_auth->ah_verf;
	xdrmem_create(xdrs, outbuf, UDPMSGSIZE_CHAR , XDR_ENCODE);
	if (! xdr_callmsg(xdrs, &msg)) {
		stat = RPC_CANTENCODEARGS;
		goto done_broad;
	}
	outlen = (int)xdr_getpos(xdrs);
	xdr_destroy(xdrs);

	t_udata.opt.len = 0;
	t_udata.udata.buf = outbuf;
	t_udata.udata.len = outlen;

	/*
	 * Basic loop: send packet to all hosts and wait for response(s).
	 * The response timeout grows larger per iteration.
	 * A unique xid is assigned to each address in order to
	 * correctly match the replies.
	 */
	for (t.tv_sec = 4; timeout > 0; t.tv_sec += 2) {
		timeout -= t.tv_sec;
		rcv_timeout = t;
		if (timeout < 0)
			t.tv_sec = timeout + t.tv_sec;
		sent = 0;
		for (trans = tr_head; trans; trans = trans->tr_next) {
			for (a = trans->tr_addrs; a; a = a->addr_next) {
				struct netbuf *if_netbuf =
					a->addr_addrs->n_addrs;
				ts = a->addr_if_tstamps;
				if_cnt = a->addr_addrs->n_cnt;		
				while (if_cnt--) {
					*((u_long *)outbuf) =
						htonl(xid + ts->ts_inx);
					(void) gettimeofday(&(ts->ts_timeval), 
							    (struct timezone *) 0);
					ts = ts->ts_next;
					t_udata.addr = *if_netbuf++;
					/* xid is the first thing in
					 * preserialized buffer
					 */
					if (t_sndudata(trans->tr_fd, &t_udata) != 0) {
						continue;
					}
					sent++;
				}
			}
		}
		if (sent == 0) {		/* no packets sent ? */
			stat = RPC_CANTSEND;
			goto done_broad;
		}

		/*
		 * Have sent all the packets.  Now collect the responses...
		 */
		rcvd = 0;
	recv_again:
		msg.acpted_rply.ar_verf = _null_auth;
		msg.acpted_rply.ar_results.proc = xdr_void;
		readfds = mask;
		switch (select(dtbsize, &readfds, 
		        (fd_set *) NULL, (fd_set *) NULL, &rcv_timeout)) {

		case 0:  /* timed out */
			if (rcvd == 0) {
				stat = RPC_TIMEDOUT;
				continue;
			} else 
				goto done_broad;

		case -1:  /* some kind of error */
			if (errno == EINTR)
				goto recv_again;
			syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
			       "nfs_cast", "select");
			if (rcvd == 0)
				stat = RPC_CANTRECV;
			goto done_broad;

		}  /* end of select results switch */

		for (trans = tr_head; trans; trans = trans->tr_next) {
			if (FD_ISSET(trans->tr_fd, &readfds))
				break;
		}
		if (trans == NULL)
			goto recv_again;

	try_again:
		t_rdata.addr = trans->tr_taddr->addr;
		t_rdata.udata.buf = inbuf;
		t_rdata.udata.maxlen = UDPMSGSIZE_CHAR;
		t_rdata.udata.len = 0;
		t_rdata.opt.len = 0;
		if (t_rcvudata(trans->tr_fd, &t_rdata, &flag) < 0) {
			if (errno == EINTR)
				goto try_again;
			syslog(LOG_ERR, gettxt(":94", "%s: %s failed for %s: %m"),
			       "nfs_cast", "t_rcvudata", trans->tr_device);
			stat = RPC_CANTRECV;
			continue;
		}
		if (t_rdata.udata.len < sizeof (u_long))
			goto recv_again;
		if (flag & T_MORE) {
			syslog(LOG_ERR,
			       gettxt(":235", "%s: %s: buffer overflow from %s"),
			       "nfs_cast", "t_rcvudata", trans->tr_device);
			goto recv_again;
		}
		/*
		 * see if reply transaction id matches sent id.
		 * If so, decode the results.
		 * Note: received addr is ignored, it could be different
		 * from the send addr if the host has more than one addr.
		 */
		xdrmem_create(xdrs, inbuf, (u_int) t_rdata.udata.len, XDR_DECODE);
		if (xdr_replymsg(xdrs, &msg)) {
			if (msg.rm_reply.rp_stat == MSG_ACCEPTED &&
			   (msg.rm_xid & ~0xFF) == xid) {		    
				struct addrs *curr_addr;
				i = msg.rm_xid & 0xFF;
				for (curr_addr = trans->tr_addrs; curr_addr; 
				     curr_addr = curr_addr->addr_next) {
					for (ts = curr_addr->addr_if_tstamps;
					     ts; ts = ts->ts_next)
						if (ts->ts_inx == i && !ts->ts_rcvd) {
							ts->ts_rcvd = 1;
							(void) calc_resp_time(&ts->ts_timeval);
							stat = RPC_SUCCESS;
							rcvd++;
							if (rcvd == 1 && 
							    (timercmp(&ts->ts_timeval, &rcv_timeout, < )))
								rcv_timeout =
									ts->ts_timeval;
							break;
						}
				}
			}					
			/* otherwise, we just ignore the errors ... */
		}
		xdrs->x_op = XDR_FREE;
		msg.acpted_rply.ar_results.proc = xdr_void;
		(void) xdr_replymsg(xdrs, &msg);
		XDR_DESTROY(xdrs);
		if (rcvd == sent)
			goto done_broad;
		else
			goto recv_again;
	}
	if (!rcvd)
		stat = RPC_TIMEDOUT;

done_broad:
	if (rcvd) {
		int index;
	        index = choose_best_server(tr_head);
		*best_host = host_array[index];
		if (trace > 1)
			fprintf(stderr, "nfs_cast: host=%s, penalty=%d\n", 
				best_host->host, best_host->penalty);
	}
	if (nc)
		endnetconfig(nc);
	if (tr_head)
		free_transports(tr_head);
	if (sys_auth)
		AUTH_DESTROY(sys_auth);
	free(outbuf);
	free(inbuf);
	return (stat);
}


choose_best_server(trans)
struct transp *trans;
{
	struct transp *t;
	struct addrs *a;
	struct tstamps *ti;
	struct timeval min_timeval;
	int inx = 0;

	if (trace > 1)
		fprintf(stderr, "choose_best_server: \n");

	min_timeval.tv_sec = 10; /* just a strart */
	for (t = trans; t; t = t->tr_next) {
		for (a = t->tr_addrs; a; a = a->addr_next)
			for (ti = a->addr_if_tstamps;
			     ti; ti = ti->ts_next) {
				ti->ts_timeval.tv_usec +=
					(ti->ts_penalty * PENALTY_WEIGHT);
				if (ti->ts_timeval.tv_usec >= 1000000) {
					ti->ts_timeval.tv_usec -= 1000000;
					ti->ts_timeval.tv_sec++;
				} 
				if (timercmp(&(ti->ts_timeval),
					     &min_timeval, < )) {
					min_timeval = ti->ts_timeval;
					inx = a->addr_inx;
				}
			}
	}
	return (inx);
}


void
calc_resp_time(sendtime)
struct timeval *sendtime;
{
	struct timeval timenow;

	if (trace > 2)
		fprintf(stderr, "calc_resp_time: \n");

	(void) gettimeofday(&timenow, (struct timezone *) 0);
	if (timenow.tv_usec <  sendtime->tv_usec) {
		timenow.tv_sec--;
		timenow.tv_usec += 1000000;
	}
	sendtime->tv_sec = timenow.tv_sec - sendtime->tv_sec;
	sendtime->tv_usec = timenow.tv_usec - sendtime->tv_usec;
}

void
free_transports(trans)
	struct transp *trans;
{
	struct transp *t, *tmpt;
	struct addrs *a, *tmpa;
	struct tstamps *ts, *tmpts;

	if (trace > 2)
		fprintf(stderr, "free_transports: \n");

	for (t = trans; t; t = tmpt) {
		if (t->tr_taddr)
			(void) t_free((char *)t->tr_taddr, T_BIND);
		if (t->tr_fd > 0)
			(void) t_close(t->tr_fd);
		for (a = t->tr_addrs ; a ; a = tmpa) {
			for (ts = a->addr_if_tstamps; ts; ts = tmpts) {
				tmpts = ts->ts_next;
				free(ts);
			}				
			(void) netdir_free((char *)a->addr_addrs, ND_ADDRLIST);
			tmpa = a->addr_next;
			free(a);
		}
		tmpt = t->tr_next;
		free(t);
	}
}
