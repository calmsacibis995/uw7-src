/*		copyright	"%c%"	*/

#ident	"@(#)network.c	1.3"

/*
 * (c) Copyright 1991 Hewlett-Packard Company.  All Rights Reserved.
 *
 * This source is for demonstration purposes only.
 *
 * Since this source has not been debugged for all situations, it will not be
 * supported by Hewlett-Packard in any manner.
 *
 * This material is provided "as is".  Hewlett-Packard makes no warranty of
 * any kind with regard to this material.  Hewlett-Packard shall not be liable
 * for errors contained herein for incidental or consequential damages in 
 * connection with the furnishing, performance, or use of this material.
 * Hewlett-Packard assumes no responsibility for the use or reliability of 
 * this software.
 */

#include	<stdio.h>
#include	<sys/time.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/ioctl.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<sys/fcntl.h>
#include	<errno.h>

#include <locale.h>
#include "hpnp_msg.h"

extern nl_catd catd;

extern int	errno;
extern char 	*ProgName;

/* Increase the size of the send buffer size for better performance */
#define SOCKETSIZE    16 * 1024

struct sockaddr_in  Paddr;		/* address information structure */

/*
 * NAME
 *	PrepListen - open 2 sockets to communicate
 *
 * SYNOPSIS
 *	void PrepListen(control, data)
 *      int *control, *data;
 *
 * DESCRIPTION
 *	This routine opens a socket for a control line and a socket
 *      for a data line.  The numbers for the sockets are left in a
 *      file in the current directory called .port.  For instance,
 *      .port might contain "-c 12000 -p 12001".  A later instance
 *      of hpnpf uses these options to talk to the "relay" hpnpf that
 *      created the .port file.  The printjob data is transferred over
 *      the data line (-p).  After the data line is closed, a single
 *      byte is transferred back over the control line (-c) indicating
 *      success or failure.
 *
 * RETURNS
 *	"PrepListen" returns nothing, but initializes the control
 *      and data variables.
 */
void
PrepListen(control, data)
int *control, *data;
{
	int s;
	struct sockaddr_in saddr;
	FILE *portfile;

	LogMessage(1, "PrepListen: entered");
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = (getpid() % 5000) + 10000;
	if ((portfile = fopen(".port", "w")) == NULL) {
		LogMessage(1, "Error (fopen .port): %s", strerror(errno));
		fprintf
		  (
		  stderr,
		  MSGSTR( HPNP_NETW_ERROR, "Error (fopen .port): %s"),
		  strerror(errno)
		  );
		exit(1);
	}
	*control = -1;

	while(1) {
		if ((s = OpenSocket(0)) < 0)
			exit(1);
		if (bind(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
			LogMessage(1, "Error (bind): %s", strerror(errno));
			close(s);
			saddr.sin_port++;
			continue;
		}
		if (*control == -1) {
			*control = s;
			LogMessage(1, "PrepListen: will listen for IF control on port %d", saddr.sin_port);
			fprintf(portfile, "-c %d ", saddr.sin_port);
			saddr.sin_port++;
		} else {
			*data = s;
			LogMessage(1, "PrepListen: will listen for IF data on port %d", saddr.sin_port);
			fprintf(portfile, "-p %d\n", saddr.sin_port);
			fclose(portfile);
			return;
		}
	}
}


/*
 * NAME
 *	MeetIF - accept a single connection from an input filter
 *
 * SYNOPSIS
 *      int MeetIF(relayfd)
 *      int relayfd;
 *
 * DESCRIPTION
 *      This routine accepts a single connection from an /etc/printcap
 *      input filter.  This routine will be called twice - once for the
 *      control line and once for the data line.  Relayfd is the a 
 *      descriptor that is listened on for the connection. 
 *
 * RETURNS
 *	"MeetIF" returns the descriptor from the connection.
 */
int
MeetIF(int relayfd)
{
	int rfd;
	struct sockaddr_in addr;
	size_t len;

	LogMessage(1, "MeetIF: entered");
	len = sizeof(addr);
	rfd = accept(relayfd, (struct sockaddr *)&addr, &len);
	if (rfd < 0) {
		LogMessage(1, "Error (accept): %s", strerror(errno));
		return(-1);
	}
	LogMessage(1, "MeetIF: connection from %s", 
					inet_ntoa(addr.sin_addr));
	return(rfd);
}


/*
 * NAME
 *	DoNonBlocking - turn nonblocking on or off on the socket
 *
 * SYNOPSIS
 *      void DoNonBlocking(fd, on)
 *      int fd;
 *      int on;
 *
 * DESCRIPTION
 *      This routine turns on or off blocking on the socket fd passed in.
 *
 * RETURNS
 *	"DoNonBlocking" returns nothing.
 */
void
DoNonBlocking(int fd, int on)
{
	int fdstat;

	LogMessage(1, "DoNonBlocking: non-blocking %s", on ? "on" : "off");
#ifdef hpux
	if (ioctl(fd, FIOSNBIO, &on) == -1)
		LogMessage(1, "Error (ioctl): %s", strerror(errno));
#else
	if (on == 1) {
		if (fcntl(fd, F_SETFL, O_NDELAY) == -1)
			LogMessage(1, "Error (fcntl): %s", strerror(errno));
	} else {
		fdstat = fcntl(fd, F_GETFL, 0);
		if (fdstat != -1)
			if (fcntl(fd, F_SETFL, fdstat & ~O_NDELAY) == -1)
				LogMessage(1, "Error (fcntl): %s", strerror(errno));
	}
#endif
}


/*
 * NAME
 *	OpenSocket - open a socket and set socket options
 *
 * SYNOPSIS
 *	int OpenSocket(large);
 *      int large;
 *
 * DESCRIPTION
 *	This routine attempts to open a stream socket. Once opened, it 
 *	sets various options for the socket.  If large is nonzero, the
 *      send buffer size is increased to SOCKETSIZE.
 *
 * RETURNS
 *	If successful, "OpenSocket" returns a valid file descriptor. If
 *	unsuccessful, the value -1 is returned. 
 */
int
OpenSocket(large)
int large;
{
	int	on	    = 1;    /* "on" flag */
	int	size	    = SOCKETSIZE ;
	int	fd;		    /* file descriptor to be returned */

	LogMessage(1, "OpenSocket: entered");

	/* Try to open an Internet stream socket */
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) != -1) {
		if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)) == -1)
			LogMessage(1, "Error (setsockopt - keepalive): %s", strerror(errno));
#ifdef SO_SNDBUF
		if (large) {
			if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) == -1)
				LogMessage(1, "Error (setsockopt - send buffer size): %s", 
							strerror(errno));
			else
				LogMessage(1, "OpenSocket: using %dK send buffer", SOCKETSIZE/1024);
		}
#endif
	} else
		LogMessage(1, "Error (socket): %s", strerror(errno));

	return(fd);
}


/*
 * NAME
 *	ResolveAddress - resolve the network peripheral IP address
 *
 * SYNOPSIS
 *	int ResolveAddress(netperiph);
 *	char		    *netperiph;	host name/IP address of network periph
 *
 * DESCRIPTION
 *	This routine attempts to resolve the IP address for the network periph
 *	specified by "netperiph" (which may be given as a host name or in
 *	dotted decimal notation). The corresponding Internet address 
 *	information is stored in the global structure pointed to by "Paddr".  
 *      The assumption here is that only one host will be contacted and so 
 *      Paddr is not known outside of this file.
 *
 * RETURNS
 *	The routine returns 1 for success and 0 for failure.
 */
int
ResolveAddress(netperiph)
char *netperiph;	/* host name or IP address of network periph */
{
	struct hostent	*h_info;	    /* host information */

	LogMessage(1, "ResolveAddress: %s", netperiph);

	/* Check for dotted decimal or hostname */
	if ((Paddr.sin_addr.s_addr = inet_addr(netperiph)) == -1) {
		/* The IP address is not in dotted decimal notation. Try to
		 * get the network peripheral IP address by host name.
		*/
		if ((h_info = gethostbyname(netperiph)) != NULL) {
			/* Copy the IP address to the address structure */
			memcpy(&(Paddr.sin_addr.s_addr), h_info->h_addr, h_info->h_length);
		} else {
			LogMessage(1, "Error: unknown host %s", netperiph);
			fprintf
			  (
			  stderr,
			  MSGSTR( HPNP_NETW_UHOST, "%s: unknown host %s\n"),
			  ProgName, netperiph
			  );
			return (0);
		}
	}

	Paddr.sin_family = AF_INET;
	return (1);
}


/*
 * NAME
 *	MakeConnection - make a connection to the socket
 *
 * SYNOPSIS
 *      int MakeConnection(fd, port, nonblocking)
 *      int fd;
 *      unsigned int port;
 *      int nonblocking;
 *
 * DESCRIPTION
 *	This routine tries to make a connection via the socket specified by
 *	"fd" to the destination specified in structure pointed to by "Paddr".
 *      The port number to connect to is passed in.  Nonblocking indicates
 *      whether the socket should be made nonblocking or not.
 *
 * RETURNS
 *	The routine returns 1 for success and 0 for failure.
 */
int
MakeConnection(fd, port, nonblocking)
int		    fd;		/* file descriptor of socket */
unsigned int	  port;
int nonblocking;
{
	LogMessage(1, "MakeConnection: %s, port %d", inet_ntoa(Paddr.sin_addr), port);

	Paddr.sin_port = htons(port);
	/* Try to make the connection */
	if (connect(fd, (struct sockaddr *) &Paddr, sizeof(struct sockaddr_in)) == 0) {
		/* We succeeded. Clear the previous error number, enable
		 * non-blocking mode and return successfully.
		 */
		if (nonblocking)
			DoNonBlocking(fd, 1);
		return (1);
	}

	LogMessage(1, "Error (connect): %s", strerror(errno));
	return (0);
}
