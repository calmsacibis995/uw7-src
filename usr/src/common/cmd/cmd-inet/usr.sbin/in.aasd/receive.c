#ident "@(#)receive.c	1.2"
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
 * This file contains code to listen for and receive connections and
 * messages from clients.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <errno.h>
#include <memory.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <aas/aas.h>
#include <aas/aas_proto.h>
#include <aas/aas_util.h>
#include "aasd.h"

/*
 * clear_passwords is set to 1 during a reconfiguration.  When this is set,
 * we clear the passwords cached for each connection.
 */

int clear_passwords = 0;

/*
 * receive -- start receiving connections/messages
 * This function takes a list of Endpoint structures which define
 * the endpoints that should be bound and listened on.  This function
 * maintains a list of active connections.  When a message is received,
 * process_request is called to process it.  receive uses select to listen
 * on all listening endpoints and receive data on all connections
 * simultaneously.  receive does not return.
 * If work_func is non-null, it is called (approximately) every intvl seconds.
 */

static fd_set read_fds, write_fds;

/*
 * do_input -- handle input on a connection
 */

static void
do_input(Connection *cp)
{
	int ret;

	ret = aas_rcv_msg(cp->fd, &cp->msg_info,
		AAS_MAX_REQ_SIZE);
	switch (ret) {
	case AAS_RCV_MSG_MORE:
		break;
	case AAS_RCV_MSG_OK:
		if (debug > 1) {
			report(LOG_INFO,
				"Received message from %s:%s: type %d, length %d",
				cp->af->name,
				(*cp->af->addr2str)(cp->from, cp->fromlen),
				cp->msg_info.header.msg_type,
				cp->msg_info.header.msg_len);
		}
		/*
		 * If a reconfiguration has happened, clear the cached
		 * password.
		 */
		if (clear_passwords) {
			cp->password = NULL;
		}
		process_request(cp);
		cp->msg_info.first = 1;
		break;
	case AAS_RCV_MSG_INCOMPLETE:
		report(LOG_WARNING, "Incomplete message received from %s:%s",
		    cp->af->name,
		    (*cp->af->addr2str)(cp->from, cp->fromlen));
		close(cp->fd);
		/*
		 * This will cause it to be removed
		 * from the list.
		 */
		cp->fd = -1;
		break;
	case AAS_RCV_MSG_CLOSED:
		close(cp->fd);
		cp->fd = -1;
		break;
	case AAS_RCV_MSG_BAD_MSG:
		report(LOG_WARNING, "Bad message received from %s:%s",
		    cp->af->name,
		    (*cp->af->addr2str)(cp->from, cp->fromlen));
		break;
	case AAS_RCV_MSG_TOO_BIG:
		send_nack(cp, AAS_MSG_TOO_BIG);
		close(cp->fd);
		cp->fd = -1;
		break;
	case AAS_RCV_MSG_ERROR:
		/*
		 * This can be an error reading from the socket or an error
		 * from malloc.  Send a server error NACK and close it.
		 */
		report(LOG_ERR,
			"An error occurred while receiving a message: %m");
		send_nack(cp, AAS_SERVER_ERROR);
		close(cp->fd);
		cp->fd = -1;
		break;
	}
}

#define AAS_WRITE_LIMIT		4096

/*
 * do_output -- handle output on a connection
 */

static void
do_output(Connection *cp)
{
	int num, ret;

	if ((num = cp->send_rem) > AAS_WRITE_LIMIT) {
		num = AAS_WRITE_LIMIT;
	}
	if ((ret = write(cp->fd, cp->send_ptr, num)) == -1) {
		report(LOG_ERR, "do_output: write failed: %m");
		free(cp->send_buf);
		cp->send_buf = NULL;
		close(cp->fd);
		cp->fd = -1;
	}
	else {
		cp->send_rem -= ret;
		if (cp->send_rem == 0) {
			free(cp->send_buf);
			cp->send_buf = NULL;
		}
		else {
			cp->send_ptr += ret;
		}
	}
}

void
receive(Endpoint *endpoints, WorkFunc work_func, time_t intvl)
{
	int max_fd, ns, ret, flags;
	size_t fromlen, max_sockaddr_len;
	Connection *connections, *cp, *ncp, *oconnections, **clink;
	Connection dummy;
	Endpoint *ep, *nep, *oendpoints, **eplink;
	struct sockaddr *from;
	struct timeval tv, *tvp;
	time_t now, next_call;

	/*
	 * Open & bind sockets to listen on.  This loop builds a new list
	 * of endpoints, eliminating any that couldn't be opened or bound.
	 */
	
	oendpoints = endpoints;
	endpoints = NULL;
	eplink = &endpoints;
	max_sockaddr_len = 0;

	for (ep = oendpoints; ep; ep = nep) {
		nep = ep->next;
		if ((ep->fd = socket(ep->af->family, SOCK_STREAM,
		    ep->af->protocol)) == -1) {
			report(LOG_ERR, "Unable to create %s socket: %m",
				ep->af->name);
			continue;
		}
		if ((*ep->af->bind)(ep->fd, ep->addr, ep->addr_len) == -1) {
			report(LOG_ERR, "Unable to bind %s socket to %s: %m",
				ep->af->name,
				(*ep->af->addr2str)(ep->addr, ep->addr_len));
			continue;
		}
		if (listen(ep->fd, 5) == -1) {
			report(LOG_ERR, "listen failed for %s socket: %m",
				ep->af->name);
			continue;
		}

		/*
		 * OK -- put it on the list.
		 */

		ep->next = NULL;
		*eplink = ep;
		eplink = &ep->next;

		if (ep->af->sockaddr_len > max_sockaddr_len) {
			max_sockaddr_len = ep->af->sockaddr_len;
		}
	}

	if (!endpoints) {
		report(LOG_ERR, "No endpoints to listen on; exiting.");
		exit(1);
	}

	if (!(from = (struct sockaddr *) malloc(max_sockaddr_len))) {
		malloc_error("receive(1) [exiting]");
		exit(1);
	}

	connections = NULL;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

	/*
	 * Set up time at which work_func is to be called.
	 */

	if (work_func) {
		next_call = time(NULL) + intvl;
		tv.tv_usec = 0;
		tvp = &tv;
	}
	else {
		tvp = NULL;
	}

	for (;;) {
		/*
		 * Set up list of fds to select on.
		 */
		max_fd = -1;
		for (ep = endpoints; ep; ep = ep->next) {
			FD_SET(ep->fd, &read_fds);
			if (ep->fd > max_fd) {
				max_fd = ep->fd;
			}
		}
		for (cp = connections; cp; cp = cp->next) {
			/*
			 * If we're in the process of sending a response
			 * on this connection, select for writing.
			 * Otherwise, select for reading.
			 */
			if (cp->send_buf) {
				FD_SET(cp->fd, &write_fds);
				FD_CLR(cp->fd, &read_fds);
				if (cp->fd > max_fd) {
					max_fd = cp->fd;
				}
			}
			else {
				FD_SET(cp->fd, &read_fds);
				FD_CLR(cp->fd, &write_fds);
				if (cp->fd > max_fd) {
					max_fd = cp->fd;
				}
			}
		}

		/*
		 * Wait for things to happen.  If a work function was
		 * specified, set a timeout for the time remaining until
		 * the next time we're supposed to call the function.
		 */
		
		if (work_func) {
			now = time(NULL);
			if (now >= next_call) {
				tv.tv_sec = 0;
			}
			else {
				tv.tv_sec = next_call - now;
			}
		}

		if (select(max_fd + 1, &read_fds, &write_fds, NULL, tvp)
		    == -1) {
			if (errno == EINTR) {
				continue;
			}
			report(LOG_ERR, "select failed: %m; exiting.");
			exit(1);
		}

		/*
		 * Process any connections that have new data.
		 * While doing this, we build a new connection list
		 * that only contains connections that are still open.
		 */
		
		oconnections = connections;
		connections = NULL;
		clink = &connections;

		for (cp = oconnections; cp; cp = ncp) {
			ncp = cp->next;
			if (FD_ISSET(cp->fd, &write_fds)) {
				FD_CLR(cp->fd, &write_fds);
				do_output(cp);
			}
			else if (FD_ISSET(cp->fd, &read_fds)) {
				FD_CLR(cp->fd, &read_fds);
				do_input(cp);
			}

			/*
			 * If the connection was closed, free storage.
			 * Otherwise, put the connection on the new list.
			 */
			
			if (cp->fd == -1) {
				free(cp->from);
				free(cp);
			}
			else {
				cp->next = NULL;
				*clink = cp;
				clink = &cp->next;
			}
		}

		/*
		 * Clear passwords if a reconfiguration has happened.
		 */
		
		if (clear_passwords) {
			for (cp = connections; cp; cp = cp->next) {
				cp->password = NULL;
			}
			clear_passwords = 0;
		}

		/*
		 * Process any endpoints that have new connections.
		 */

		for (ep = endpoints; ep; ep = ep->next) {
			if (!FD_ISSET(ep->fd, &read_fds)) {
				continue;
			}
			fromlen = max_sockaddr_len;
			if ((ns = accept(ep->fd, from, &fromlen)) == -1) {
				report(LOG_ERR, "accept on %s socket failed: %m",
					ep->af->name);
				continue;
			}

#if 0
			/*
			 * Set non-blocking mode on the connection to avoid
			 * hanging the server.
			 */
			if (fcntl(ns, F_GETFL, &flags) == -1
			    || fcntl(ns, F_SETFL, flags | O_NDELAY) == -1) {
				report(LOG_ERR, "receive: fcntl failed: %m");
				close(ns);
				continue;
			}
#endif
			/*
			 * Connection is ok.  Set it up.
			 */
			cp = NULL;
			if (!(cp = str_alloc(Connection))
			    || !(cp->from =
			    (struct sockaddr *) malloc(fromlen))) {
				malloc_error("receive(2)");
				if (cp) {
					free(cp);
				}
				dummy.fd = ns;
				send_nack(&dummy, AAS_SERVER_ERROR);
				close(ns);
				continue;
			}
			cp->af = ep->af;
			cp->fd = ns;
			memcpy(cp->from, from, fromlen);
			cp->fromlen = fromlen;
			cp->msg_info.first = 1;
			cp->send_buf = NULL;
			cp->password = NULL;

			/*
			 * Initialize the authentication stuff and
			 * send a hello message.
			 */
			
			if (!init_auth(cp)) {
				free(cp->from);
				free(cp);
				continue;
			}
			
			/*
			 * Add it to the list of connections.
			 */

			cp->next = NULL;
			*clink = cp;
			clink = &cp->next;

			if (debug > 1) {
				report(LOG_INFO,
				    "Connection accepted from %s:%s",
				    cp->af->name,
				    (*cp->af->addr2str)(cp->from, cp->fromlen),
				    cp->fd);
			}
		}

		/*
		 * Call the work function if it's time.
		 */
		
		if (time(NULL) >= next_call) {
			(*work_func)();
			next_call = time(NULL) + intvl;
		}
	}
}
