#ident "@(#)endpt.c	1.3"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
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

/*
 * This file contains sockets and TLI implementations of the following
 * endpoint-related functions:
 *
 * int get_endpt(u_short port)
 *	Open UDP endpoint & bind to (INADDR_ANY, port).  Returns fd
 *	if ok, or -1 if error.
 *
 * int set_endpt_opt(int endpt, int level, int option, void *value, int len)
 *	Set a socket option.  Returns 0 if ok, or -1 if error.
 *
 * int send_packet(int endpt, struct sockaddr_in *dest, void *buf, int len)
 *	Send a packet.  Returns 0 if ok, -1 if error.
 *
 * int recv_packet(int endpt, void *buf, int len, struct sockaddr_in *from,
 *   u_long *ifindex)
 *	Receive a packet.  Returns length if ok, 0 if packet was bigger than
 *	len, or -1 if an error occurred.  The index of the receiving interface
 *	is returned in *ifindex.
 *
 * int is_endpt(int fd, struct sockaddr_in *addr)
 *	Determine whether fd is a valid endpoint (socket or TLI device).
 *	This routine is called on fd 0 to determine if an endpoint has been
 *	passed from inetd or tlid.  Returns 1 if so, 0 if not.  If the fd
 *	is an endpoint, the bound address is returned in *addr.
 *
 * If USE_TLI is defined, the TLI versions are compiled.  Otherwise,
 * the sockets versions are compiled.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <netinet/in.h>
#include <syslog.h>
#include <errno.h>

#ifdef USE_TLI

#include <sys/tiuser.h>
#include <fcntl.h>
#include <paths.h>
#include <stropts.h>

extern int t_errno, t_nerr;
extern char *t_errlist[];

/*
 * t_strerror -- return a TLI error string based on the given error code.
 */

static char *
t_strerror(int err)
{
	static char buf[1024];

	if (err == TSYSERR) {
		sprintf(buf, "%s: %s", t_errlist[TSYSERR], strerror(errno));
	}
	else if (err < t_nerr) {
		strcpy(buf, t_errlist[err]);
	}
	else {
		sprintf(buf, "TLI error %d", err);
	}

	return buf;
}

int
get_endpt(u_short port)
{
	int endpt;
	struct t_bind *bind_req, *bind_ret;
	struct sockaddr_in *sin;

	if ((endpt = t_open(_PATH_UDP, O_RDWR, NULL)) == -1) {
		report(LOG_ERR, "Unable to open %s: %s",
			_PATH_UDP, t_strerror(t_errno));
		return -1;
	}

	if (!(bind_req = (struct t_bind *) t_alloc(endpt, T_BIND, T_ALL))
	    || !(bind_ret = (struct t_bind *) t_alloc(endpt, T_BIND, T_ALL))) {
		report(LOG_ERR, "t_alloc failed: %s", t_strerror(t_errno));
		t_close(endpt);
		return -1;
	}

	bind_req->addr.len = sizeof(struct sockaddr_in);
	sin = (struct sockaddr_in *) bind_req->addr.buf;
	memset(sin, 0, sizeof(*sin));
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_port = port;

	if (t_bind(endpt, bind_req, bind_ret) == -1) {
		report(LOG_ERR, "t_bind failed: %s", t_strerror(t_errno));
		t_close(endpt);
		return -1;
	}

	if (memcmp(bind_req->addr.buf, bind_ret->addr.buf, bind_req->addr.len)
	    != 0) {
		report(LOG_ERR, "Unable to bind to port %d",
			ntohs(port));
		t_close(endpt);
		return -1;
	}

	t_free((char *) bind_req, T_BIND);
	t_free((char *) bind_ret, T_BIND);

	return endpt;
}

int
set_endpt_opt(int s, int level, int optname, void *optval, int optlen)
{
	struct t_optmgmt opt_req, opt_ret;
#define OPTBUFLEN 32
	char reqbuf[OPTBUFLEN], retbuf[OPTBUFLEN];
#undef OPTBUFLEN
	struct opthdr *opt;

	opt_req.opt.buf = reqbuf;
	opt_req.opt.len = sizeof(struct opthdr) + OPTLEN(optlen);
	opt_req.flags = T_NEGOTIATE;
	opt = (struct opthdr *) reqbuf;
	opt->level = level;
	opt->name = optname;
	opt->len = OPTLEN(optlen);
	memcpy(OPTVAL(opt), optval, optlen);

	opt_ret.opt.buf = retbuf;
	opt_ret.opt.maxlen = sizeof(retbuf);

	if (t_optmgmt(s, &opt_req, &opt_ret) == -1) {
		report(LOG_ERR, "t_optmgmt failed: %s",
			t_strerror(t_errno));
		return -1;
	}

	if (opt_ret.opt.len != opt_req.opt.len
	    || memcmp(reqbuf, retbuf, opt_req.opt.len) != 0) {
		report(LOG_ERR, "t_optmgmt didn't set option correctly.");
		return -1;
	}

	return 0;
}

int
send_packet(int endpt, struct sockaddr_in *dest, void *buf, int len)
{
	struct t_unitdata udata;

	udata.addr.buf = (char *) dest;
	udata.addr.len = sizeof(struct sockaddr_in);
	udata.opt.buf = NULL;
	udata.opt.len = 0;
	udata.udata.buf = (char *) buf;
	udata.udata.len = len;

	if (t_sndudata(endpt, &udata) == -1) {
		report(LOG_ERR, "t_sndudata failed: %s",
			t_strerror(t_errno));
		return -1;
	}

	return 0;
}

/*
 * handle_tli_event -- called when an unexpected event occurs on a TLI
 * endpoint.  Calls t_look and does the appropirate thing.
 */

static void
handle_tli_event(int endpt)
{
	struct t_uderr uderr;
	struct sockaddr_in addr;

	switch (t_look(endpt)) {
	case T_ERROR:
		report(LOG_ERR, "Fatal error occurred on TLI endpoint; exiting...");
		exit(1);
	case T_UDERR:
		uderr.addr.buf = (char *) &addr;
		uderr.addr.maxlen = sizeof(addr);
		uderr.opt.buf = NULL;
		uderr.opt.maxlen = 0;
		if (t_rcvuderr(endpt, &uderr) == -1) {
			report(LOG_ERR, "t_rcvuderr failed: %s; exiting...",
				t_strerror(t_errno));
			exit(1);
		}
		errno = uderr.error;
		report(LOG_WARNING, "Error sending to %s: %s",
			inet_ntoa(addr.sin_addr),
			get_errmsg());
		break;
	case -1:
		report(LOG_ERR, "t_look failed: %s; exiting..",
			t_strerror(t_errno));
		exit(1);
	}
}

int
recv_packet(int endpt, void *buf, int len, struct sockaddr_in *from,
	u_long *ifindex)
{
	struct t_unitdata udata;
	int flags, found;
	char optbuf[64];
	struct opthdr *hdr;

	udata.addr.buf = (char *) from;
	udata.addr.maxlen = sizeof(struct sockaddr_in);
	udata.opt.buf = optbuf;
	udata.opt.maxlen = sizeof(optbuf);
	udata.udata.buf = (char *) buf;
	udata.udata.maxlen = len;

	if (t_rcvudata(endpt, &udata, &flags) == -1) {
		if (t_errno == TLOOK) {
			handle_tli_event(endpt);
			return -1;
		}
		else {
			report(LOG_ERR, "t_rcvudata failed: %s; discarding",
				t_strerror(t_errno));
			return -1;
		}
	}

	/*
	 * If the packet was too big, read the rest of it in and discard
	 * it.
	 */

	if (flags & T_MORE) {
		do {
			if (t_rcvudata(endpt, &udata, &flags) == -1) {
				if (t_errno == TLOOK) {
					handle_tli_event(endpt);
					return -1;
				}
				else {
					report(LOG_ERR, "t_rcvudata failed: %s; discarding",
						t_strerror(t_errno));
					return -1;
				}
			}
		} while (flags & T_MORE);
		return 0;
	}

	/*
	 * Get the interface index.
	 */

	found = 0;
	if (udata.opt.len > 0) {
		for (hdr = (struct opthdr *) optbuf;
		    (char *) hdr < optbuf + udata.opt.len;
		    hdr = (struct opthdr *) ((char *) hdr + hdr->len)) {
			if (hdr->level != IPPROTO_IP
			    || hdr->name != IP_RECVIFINDEX) {
				continue;
			}
			if (hdr->len != OPTLEN(sizeof(u_long))
			    + sizeof(struct opthdr)) {
				report(LOG_ERR, "Bad IF index option??; discarding...");
				return -1;
			}
			*ifindex = *((u_long *) OPTVAL(hdr));
			found = 1;
		}
	}
	if (!found) {
		report(LOG_ERR, "UDP didn't return IF index??; discarding...");
		return -1;
	}

	return udata.udata.len;
}

int
is_endpt(int fd, struct sockaddr_in *addr)
{
	struct strioctl sioc;

	if (t_sync(fd) == -1) {
		if (t_errno != TBADF
		    && !(t_errno == TSYSERR && errno == EBADF)) {
			report(LOG_ERR, "t_sync failed: %s",
				t_strerror(t_errno));
		}
		return 0;
	}

	/*
	 * Issue an ioctl to get the address we're bound to.
	 */

	sioc.ic_cmd = SIOCGETNAME;
	sioc.ic_timout = -1;
	sioc.ic_len = sizeof(struct sockaddr_in);
	sioc.ic_dp = (char *) addr;

	if (ioctl(fd, I_STR, &sioc) == -1) {
		report(LOG_ERR, "SIOCGETNAME failed: %s", get_errmsg());
		return 0;
	}

	return 1;
}

#else

#include <sys/uio.h>

int
get_endpt(u_short port)
{
	int endpt;
	struct sockaddr_in sin;

	if ((endpt = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		report(LOG_ERR, "Unable to create UDP socket: %s",
			get_errmsg());
		return -1;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = port;

	if (bind(endpt, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
		report(LOG_ERR, "Unable to bind to port %d\n",
			ntohs(port));
		return -1;
	}

	return endpt;
}

int
set_endpt_opt(int endpt, int level, int optname, void *optval, int optlen)
{
	return setsockopt(endpt, level, optname, optval, optlen);
}

int
send_packet(int endpt, struct sockaddr_in *dest, void *buf, int len)
{
	if (sendto(endpt, buf, len, 0, (struct sockaddr *) dest,
	    sizeof(struct sockaddr_in)) == -1) {
		report(LOG_ERR, "sendto failed: %s", get_errmsg());
		return -1;
	}
	else {
		return 0;
	}
}

int
recv_packet(int endpt, void *buf, int len, struct sockaddr_in *from,
	u_long *ifindex)
{
	struct msghdr mh;
	struct iovec iov;
	struct cmsghdr *cmh;
	char ctlbuf[64];
	int rlen, found;

	iov.iov_base = buf;
	iov.iov_len = len;
	mh.msg_name = (char *) from;
	mh.msg_namelen = sizeof(struct sockaddr_in);
	mh.msg_iov = &iov;
	mh.msg_iovlen = 1;
	mh.msg_control = ctlbuf;
	mh.msg_controllen = sizeof(ctlbuf);

	if ((rlen = recvmsg(endpt, &mh, 0)) < 0) {
		report(LOG_ERR, "recvmsg failed: %s", get_errmsg());
		return -1;
	}

	if (mh.msg_flags & MSG_TRUNC) {
		return 0;
	}

	/*
	 * Get the interface number.  There may be other options so we
	 * have to look for IP_RECVIFINDEX.
	 */
	
	found = 0;
	if (mh.msg_controllen > 0) {
		for (cmh = CMSG_FIRSTHDR(&mh); cmh;
		    cmh = CMSG_NXTHDR(&mh, cmh)) {
			if (cmh->cmsg_level != IPPROTO_IP
			    || cmh->cmsg_type != IP_RECVIFINDEX) {
				continue;
			}
			if (cmh->cmsg_len != sizeof(struct cmsghdr)
			    + OPTLEN(sizeof(u_long))) {
				report(LOG_ERR, "Bad IF index option??; discarding...");
				return -1;
			}
			*ifindex = *((u_long *) CMSG_DATA(cmh));
			found = 1;
		}
	}
	if (!found) {
		report(LOG_ERR, "UDP didn't return IF index??; discarding...");
		return -1;
	}
	return rlen;
}

int
is_endpt(int fd, struct sockaddr_in *addr)
{
	size_t len;

	len = sizeof(struct sockaddr_in);
	if (getsockname(fd, (struct sockaddr *) addr, &len) == -1) {
		if (errno != EBADF && errno != ENOTSOCK) {
			report(LOG_ERR, "getsockname failed: %s", get_errmsg());
		}
		return 0;
	}

	return 1;
}

#endif
