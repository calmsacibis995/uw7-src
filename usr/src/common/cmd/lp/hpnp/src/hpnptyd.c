/*		copyright	"%c%"	*/

#ident	"@(#)hpnptyd.c	1.3"

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

#define _NFILE 128
/*
 * FILE
 *	hpnptyd - HP Network Peripheral Pty Daemon
 *
 * DESCRIPTION
 *      The HP Network Peripheral Pty Daemon provides a connection between
 *      pseudo-ttys and networked peripherals.
 *
 */
#if	defined(sun)
#define BSD 1
#endif /* sun */

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<stdio.h>
#include	<string.h>
#include	<libgen.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<signal.h>
#if	defined(BSD)
#include	<sys/file.h>
#include	<sys/ioctl.h>
#endif	/* BSD */
#include	"fifo.h"

#include <locale.h>
#include "hpnp_msg.h"

nl_catd catd;

/* Declare external variables.
 */
extern int  errno;	    /* system error number */

/* Define local constants.
 */
#define	PORT	    9100		/* default port number */
#define	TIMEOUT	    30			/* default timeout */

/* Define message strings.
 */
#define	MSG_COULDNTFORK	    "%s: Couldn't fork daemon:\n\
    Master pty = %s\n\
    Peripheral = %s\n\
    Port       = %d\n\
    Timeout    = %u\n"

#define	DEBUG_PERROR(text)	/* debugging version of perror() */ \
	{ \
	    LogMessage(1, "%s: error %d (%s)", text, errno, \
		strerror(errno), NULL, NULL); \
	}

/* Define local variables.
 */
char	*ProgName;	/* argv[0] */
Fifo	Xmit;		/* transmit FIFO (host to peripheral) */
Fifo	Recv;		/* receive FIFO (peripheral to host) */
int	Foreground = 0;
int	KeepConnection = 0;
int	CloseConnection = 0;


/*
 * Declare local functions.
 */
static void	AbortDaemon(int);
static void	Daemon();
static void	DeadConnection(int);
static void	DoProcessing();
int	FifoFill();
int	FifoFlush();
static void    ForceClose(int);
static int	ForkDaemon();
void	LogMessage();
int	MakeConnection();
static int	OpenPty(char *);
int	OpenSocket();
void	ProcessConfig(); 
int	ResolveAddress();
static void	Snooze(unsigned int *);
void    StartLog();
void    StopLog();

main(int argc, char *argv[])
{
    int		    option;		    /* current option */
    int		    l_flag	= 0;	    /* flag for '-l' option */
    int		    m_flag	= 0;	    /* flag for '-m' option */
    int		    x_flag	= 0;	    /* flag for '-x' option */
    int		    p_flag	= 0;	    /* flag for '-p' option */
    int		    t_flag	= 0;	    /* flag for '-t' option */
    int		    err_flag	= 0;	    /* flag for usage errors */
    char	    *log_name;		    /* logging file name */ 
    char	    *master_pty;	    /* master pty name */
    char	    *peripheral;	    /* peripheral name */
    unsigned int    port	= PORT;	    /* port number */
    unsigned int    timeout	= TIMEOUT;  /* timeout */
    int	fd;	/* temporary file descriptor */
    char            *s;
extern char *optarg;	    /* pointer to option argument */
extern int  optind;	    /* index of current option */
extern int  opterr;	    /* flag to enable error logging by getopt() */

	setlocale(LC_ALL, "");
	catd = catopen( MF_HPNP, MC_FLAGS);

	ProgName = basename(argv[0]);

	opterr = 0;
	while (!err_flag && ((option = getopt(argc, argv, "fkl:m:p:t:x:")) != EOF)) {
	switch (option) {
	case 'f':
		Foreground++;
		break;

	case 'k':
		KeepConnection++;
		break;

	case 'l':	/* logging file name */
		if (l_flag++ == 0)
			log_name = optarg;
		else
			err_flag++;
		break;

	case 'm':	/* master pty name */
		if (m_flag++ == 0)
			master_pty = optarg;
		else
			err_flag++;
		break;

	case 'p':	/* port number */
		if (p_flag++ == 0)
			port = atoi(optarg);
		else
			err_flag++;
		break;

	case 't':   /* timeout */
		if (t_flag++ == 0) {
			if (atoi(optarg) <= 0) {
				fprintf
				  (
				  stderr,
				  MSGSTR
				    (
				    HPNP_HTYD_TMOUT,
				    "%s: timeout must be greater than 0.\n"
				    ),
				  ProgName
				  );
				exit(1);
			}
			timeout = atoi(optarg);
		} else
			err_flag++;
		break;

	case 'x':   /* hostname or address */
		if (x_flag++ == 0)
			peripheral = optarg;
		else
			err_flag++;
		break;

	case '?':   /* illegal option */
		err_flag++;
		break;
	}/*switch*/

	}/*while*/

	/* Emit a usage message and exit if there were: invalid options; too
	 * many arguments; or conflicting options.
	 */
	/* We don't produce an error if a hostname or address, or master pty
	 * is not given because we may be getting it from a configuration file
	 * of pty devices to IP address to port number mappings.
	 */
	if (err_flag || (optind < argc) || !x_flag || !m_flag) {
		fprintf
		  (
		  stderr,
		  MSGSTR
		    (
		    HPNP_HTYD_USAGE,
	  "%s -m master_pty -x peripheral [-p port] [-t timeout] [-l logfile]\n"
		    ),
		  ProgName
		  );
		exit(1);
	}

	/* Close all file descriptors
	 */
	for (fd = 0; fd < _NFILE; fd++)
		close(fd);

	/* Optionally try to open the logging file.
	 */
	if (l_flag)
		OpenLog(log_name);

	/* Move the current directory off the mounted filesystem */
	chdir("/");

	/* Clear any inherited file mode creation mask */
	umask(0);

	/* Invoke a single daemon */
	if (Foreground)
		Daemon(master_pty, peripheral, port, timeout);
	else 
	if (ForkDaemon(master_pty, peripheral, port, timeout) == -1) {
		fprintf
		  (
		  stderr,
		  MSGSTR( HPNP_HTYD_FORKF, MSG_COULDNTFORK),
		  ProgName, master_pty, peripheral, port, timeout	
		  );
		exit(1);
	}

	/* Exit successfully */
	exit(0);
}

/*
 * NAME
 *	ForkDaemon - fork a daemon
 *
 * SYNOPSIS
 *	int ForkDaemon(master_pty, peripheral, port, timeout);
 *	char		*master_pty;	name of master pty
 *	char		*peripheral;	host name/IP address of peripheral
 *	unsigned int	port;		socket port number 
 *	unsigned int	timeout;	timeout (seconds)
 *
 * DESCRIPTION
 *	"ForkDaemon" invokes a daemon to handle a single pty-to-peripheral
 *	connection and does the necessary disassociation from the parent
 *	process. "ForkDaemon" does not use its input arguments and merely
 *	passes them on to the main processing routine ("Daemon").
 *
 * RETURNS
 *	If successful, "ForkDaemon" returns the process ID of the new daemon;
 *	if unsuccessful, a value of -1.
 */
static int
ForkDaemon(master_pty, peripheral, port, timeout)
char		*master_pty;	/* name of master pty */
char		*peripheral;	/* host name/IP address of peripheral */
unsigned int	port;		/* socket port number */
unsigned int	timeout;	/* timeout (seconds) */
{
	int	pid;	/* pid from fork() */
#ifdef BSD
	int	fd;	/* temporary file descriptor */
#endif

	/* Ignore terminal stop signals */
#if	defined(SIGTTOU)
	signal(SIGTTOU, SIG_IGN);
#endif
#if	defined(SIGTTIN)
	signal(SIGTTIN, SIG_IGN);
#endif
#if	defined(SIGTSTP)
	signal(SIGTSTP, SIG_IGN);
#endif

	/* Fork the daemon and return immediately for the parent process */
	if ((pid = fork()) != 0)
		return (pid);

	/* Disassociate from the controlling terminal and process group and 
	 * ensure that the process can't reacquire a new controlling terminal.
	 *
	 * This is done differently for BSD vs. System V:
	 *
	 *	BSD won't assign a new controlling terminal because the
	 *	process group is non-zero.
	 *
	 *	System V won't assign a new controlling terminal because the 
	 *	process is not a process group leader. (Must not do a subsequent
	 *	setpgrp()!)
	 */
#if	defined(BSD)

	/* Change the process group and lose the controlling terminal */
	setpgrp(0, getpid());

	if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
		ioctl(fd, TIOCNOTTY, 0);
		close(fd);
	}

#else	/* BSD */

	/* Lose the control terminal and change the process group */
	setpgrp();

	/* Become immune from process group leader death and become a 
	 * non process-group leader.
	 */
	signal(SIGHUP, SIG_IGN);
	if (fork() != 0)
		exit(0);

#endif

	/* Call the main daemon program */
	Daemon(master_pty, peripheral, port, timeout);
	return(0);
}

/*
 * NAME
 *	Daemon - main daemon routine
 *
 * SYNOPSIS
 *	void Daemon(master_pty, peripheral, port, timeout);
 *	char		*master_pty;	name of master pty
 *	char		*peripheral;	host name/IP address of peripheral
 *	unsigned int	port;		socket port number 
 *	unsigned int	timeout;	timeout (seconds)
 *
 * DESCRIPTION
 *	"Daemon" connects a pty to a network peripheral.  Both the
 *      pty and TCP socket are opened and a connection is made to
 *      the network peripheral.  
 *
 * RETURNS
 *      "Daemon" returns nothing.
 */
static void
Daemon(master_pty, peripheral, port, timeout)
char		*master_pty;	/* name of master pty */
char		*peripheral;	/* host name/IP address of peripheral */
unsigned int	port;		/* socket port number */
unsigned int	timeout;	/* timeout (seconds) */
{
	int		fd_pty;		    /* master pty file descriptor */
	int		fd_socket;	    /* socket file descriptor */
	unsigned int	retry;		    /* retry period */
	int 		fd_slave = -1;

	/* Trap external termination signals and broken connections */
	signal(SIGTERM, AbortDaemon);
	signal(SIGPIPE, DeadConnection);

	signal(SIGUSR1, StartLog);
	signal(SIGUSR2, StopLog);
	signal(SIGHUP, ForceClose);

	/* Emit a logging message to indicate start of the daemon */
	LogMessage(1, "Started (-m %s -x %s -p %u -t %u)", master_pty, 
					peripheral, port, timeout);

	/* Seed the random number generator using the current system time */
	srand((unsigned int)time(NULL));

	/* Open the master pty, retrying as necessary */
	retry = 1; 
	while ((fd_pty = OpenPty(master_pty)) == -1) 
		Snooze(&retry);

#ifdef BSD
	/* Open the slave side of the pty so the data written 
	 * to the slave side persists.  Otherwise, data less than
	 * 4K is lost if it is not read within 15 seconds of the last 
	 * close on the slave side because of the streams pty
	 * implementation.
	 */
	fd_slave = OpenSlavePty(master_pty);
#endif /* BSD */

	/* Loop forever (or until the daemon is aborted) */
	retry = 1;
	while (1) {
		/* Open the socket, retrying as necessary */
		while ((fd_socket = OpenSocket(0)) == -1)
			Snooze(&retry);

		/* Resolve the peripheral address, retrying as necessary */
		while (!ResolveAddress(peripheral))
			Snooze(&retry);

		/* Make the connection, retrying as necessary */
		if (!MakeConnection(fd_socket, port, 1)) {
			Snooze(&retry);
			close(fd_socket);
			continue;
		}

		LogMessage(1, "Daemon: connected");
		DoProcessing(fd_pty, &fd_socket, timeout, master_pty, fd_slave);
		if (fd_socket != -1)
			close(fd_socket);
		retry = 1;
	}/*while*/
}


/*
 * NAME
 *	Snooze - sleep for a random, increasing time period
 *
 * SYNOPSIS
 *	void Snooze(ptimer);
 *	unsigned int	*ptimer;    pointer to timer variable
 *
 * DESCRIPTION
 *	This routine puts the current process to sleep for the number of
 *	seconds specified in the variable pointed to by "ptimer". It then
 *	adjusts the timer by doubling it (to a maximum of 60 seconds) and
 *	adding a randomizing factor.
 *
 * RETURNS
 *	"Snooze" returns nothing. The updated value of the timer will be found
 *	in the variable pointed to by "ptimer".
 */
static void
Snooze(unsigned int *ptimer)
{

	/* Sleep for a while */
	sleep(*ptimer);

	/* Double the timer value and clip to a maximum of 60 seconds. Then add
	 * a radomizing factor between 0 and 3 seconds.
	 */
	(*ptimer) *= 2;
	if (*ptimer > 60)
		*ptimer = 60;
	(*ptimer) += (rand() % 3);
}


/*
 * NAME
 *	OpenPty - open the master pty and set options
 *
 * SYNOPSIS
 *	int OpenPty(master_pty);
 *	char	*master_pty;	name of master pty
 *
 * DESCRIPTION
 *	This routine attempts to open the pty specified by "master_pty". 
 *	Once opened, it sets various options for the pty.
 *
 *	In case of errors, messages will be logged.
 *
 * RETURNS
 *	If successful, "OpenPty" returns a valid file descriptor. If
 *	unsuccessful, the value -1 is returned. A fatal error will cause
 *	the daemon to be aborted via "AbortDaemon()".
 */
static int
OpenPty(char *master_pty)
{
	static int	prev_errno  = 0;	/* previous error number */
	int		fd;		/* file descriptor to be returned */

	LogMessage(1, "OpenPty: entered");

	/* Try to open the master pty for non-blocking reading and writing */
	if ((fd = open(master_pty, O_RDWR /*| O_NDELAY*/)) != -1) {
		/* We succeeded. Clear the previous error number, enable
		 * trapping of open(), close(), and ioctl(), and return the
		 * master pty file descriptor.
		 */
		prev_errno = 0;
		return (fd);
	}

	/* We failed. Optionally emit an error message */
	if (prev_errno != errno) {
		prev_errno = errno;
		LogMessage(1, "Error (open): %s", strerror(errno));
	}

	/* Return a non-fatal error or abort */
	if (errno != EBUSY)
		AbortDaemon(0);

	return(-1);
}


/*
 * NAME
 *	OpenSlavePty - Open the slave side of the pty.
 *
 * SYNOPSIS
 *      OpenSlavePty(master_pty)
 *      char *master_pty;
 *
 * DESCRIPTION
 *      On a Sun system, after the last writer closes the slave side, the
 *      data only persists for 15 seconds.  This is because ptys are
 *      implemented with streams.  Have the pty daemon open
 *      the slave side and keep it open so that the data is not flushed
 *      if the pty daemon doesn't get to it right away.  
 *
 * RETURNS
 *	"LogMessage" returns nothing.
 */

#if     defined(BSD)
static int
OpenSlavePty(master_pty)
char *master_pty;
{
	char        slave_pty[80];
	char        *s;
	int 	fd;

	LogMessage(1, "OpenSlavePty: entered");
	strcpy(slave_pty, master_pty);
	if ((s = strrchr(slave_pty, '/')) != NULL) {
		*++s = 't';
		if((fd=open(slave_pty, O_WRONLY, 0664)) < 0)
			LogMessage(1, "OpenSlavePty: failed open of %s %s", 
		slave_pty, strerror(errno));
	}

	return(fd);
}
#endif  /* BSD */

/*
 * NAME
 *	DoProcessing - do daemon processing
 *
 * SYNOPSIS
 *      void DoProcessing(fd_pty, fd_socket, timeout, master_pty, fd_slave)
 *	int		fd_pty;	    file descriptor for master pty
 *	int		fd_socket;  file descriptor for LAN socket
 *	unsigned int	timeout;    timeout (seconds)
 *      char            *master_pty;  name of master pty
 *      int             fd_slave    descriptor for slave pty
 *
 * DESCRIPTION
 *	This routine performs the bi-directional communication between the
 *	master pty specified by "fd_pty" and the socket specified by 
 *	"fd_socket". If "timeout" is non-zero, it specifies how long the
 *      connection can be idle before the connection is closed.  Closing
 *      the connection allows the peripheral to be power cycled without
 *      hpnptyd losing printjob data.  If the connection is left up, hpnptyd
 *      won't find out that the peripheral has been power cycled until
 *      it writes to the socket and then data will be lost.
 *
 * RETURNS
 *	"DoProcessing" returns nothing. It will exit only when the connection
 *	times out or is broken.
 */
static void
DoProcessing(fd_pty, fd_socket, timeout, master_pty, fd_slave)
int		fd_pty;	    /* file descriptor for master pty */
int		*fd_socket;  /* file descriptor for LAN socket */
unsigned int	timeout;    /* timeout (seconds) */
char            *master_pty;
int             fd_slave;
{
	int		nfds;	    /* maximum number of file descriptors */
	fd_set		readfds;    /* select mask for reading */
	fd_set		writefds;   /* select mask for writing */
	fd_set		exceptfds;  /* select mask for exceptions */
	struct timeval	*ptimer;    /* pointer to select timer */
	struct timeval	timer;	    /* timer for select */ 
	int			n;	    /* count returned */

	LogMessage(1, "DoProcessing: entered");

	FifoInit(&Xmit);
	FifoInit(&Recv);

	/* Initialize the maximum number of file descriptors, the select masks, 
	 * and the select timer.
	 */
	nfds = ((fd_pty > *fd_socket) ? (fd_pty + 1) : (*fd_socket + 1));
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	if (KeepConnection && !CloseConnection)
		ptimer = 0;
	else {
		ptimer = &timer;
		timer.tv_sec = timeout;
		timer.tv_usec = 0;
	}

	/* Loop forever */
	for (;;) {
		/* If the appropriate FIFOs are not full, set up the select
		 * read masks for the pty and socket.
		 */
		if (FifoBytesFree(&Xmit) > 0)
			FD_SET(fd_pty, &readfds);
		else
			FD_CLR(fd_pty, &readfds);

		if (*fd_socket != -1) {
			if (FifoBytesFree(&Recv) > 0)
				FD_SET(*fd_socket, &readfds);
			else
				FD_CLR(*fd_socket, &readfds);
		}

		/* If the appropriate FIFOs are not empty, set up the select
		 * write masks for the socket and pty.
		 */
		if (*fd_socket != -1) {
			if (FifoBytesUsed(&Xmit) > 0)
				FD_SET(*fd_socket, &writefds);
			else
				FD_CLR(*fd_socket, &writefds);
		}

		if (FifoBytesUsed(&Recv) > 0)
			FD_SET(fd_pty, &writefds);
		else
			FD_CLR(fd_pty, &writefds);

		/* Wait for something to happen */
		if ((n = select(nfds, &readfds, &writefds, &exceptfds, ptimer)) == -1) {
			if (errno == EINTR) {
				if (CloseConnection) {
					ptimer = &timer;
					timer.tv_sec = timeout;
					timer.tv_usec = 0;
				}
				continue;
			}

			DEBUG_PERROR("DoProcessing: select()");
			return;
		}

		/* Check for a timeout */
		if (n == 0) {
			if ((!KeepConnection || CloseConnection) 
			    && (FifoBytesUsed(&Xmit) == 0)
			    && (FifoBytesUsed(&Recv) == 0)) {

				FD_CLR(*fd_socket, &readfds);
				FD_CLR(*fd_socket, &writefds);
				FifoInit(&Xmit);
				FifoInit(&Recv);
				close(*fd_socket);
				*fd_socket = -1;
				ptimer = 0;
				CloseConnection = 0;
				LogMessage(1, "DoProcessing: socket closed - idle %d seconds", timeout);
#ifdef BSD
				/* Close and open the slave side of the pty to avoid
				 * EBUSY errors.
				 */
				if (fd_slave != -1)
					close(fd_slave);

				/* Open the slave side of the pty until so the
				 * data written to the slave side persists.
				 * Otherwise, data less than 4K is lost if it
				 * is not read within 15 seconds of the last
				 * close on the slave side because of the
				 * streams pty implementation.
				 */
				fd_slave = OpenSlavePty(master_pty);
#endif /* BSD */
			} else {
				timer.tv_sec = timeout;
				timer.tv_usec = 0;
			}

			continue;
		}

		/* Check for data to be read from the pty */
		if (FD_ISSET(fd_pty, &readfds)) {
			/* If the socket was closed, return to establish a
			 * new connection.
			 */
			if (*fd_socket == -1)
				return;

			if ((n = FifoFill(&Xmit, fd_pty, 0)) == -1) {
				DEBUG_PERROR("DoProcessing: read from pty");
#if	defined(hpux)
				return;
#endif	

#if     defined(BSD)

		/* In BSD - when the process, inputting data into the slave side
		   of the pty, exits, it appears to the master side (this
		   process) as if an IO error happened.

		   If the slave side of the pty is kept open, this won't happen.
		*/
				if (errno == EIO) {
					int retry;

					FD_CLR(fd_pty, &readfds);
					FD_CLR(fd_pty, &writefds);
					close(fd_pty);
					FifoInit(&Recv);
					retry = 1;
					while((fd_pty = OpenPty(master_pty)) == -1)
						Snooze(&retry);
    			        	nfds = ((fd_pty > *fd_socket) ? (fd_pty + 1) : (*fd_socket + 1));
				} else {
					DEBUG_PERROR("DoProcessing: read from pty");
					return;
				}
#endif  /* BSD */
			} else {
				LogMessage(1, "DoProcessing: %d bytes read from pty",n );
			}

		}

		/* Check for data to be written to the socket */
		if ((*fd_socket != -1) && FD_ISSET(*fd_socket, &writefds)) {
			if ((n = FifoFlush(&Xmit, *fd_socket)) == -1) {
				if (errno != EWOULDBLOCK) {
					DEBUG_PERROR("DoProcessing: write to socket");
					return;
				}
			} else
				LogMessage(1, "DoProcessing: %d bytes written to socket", n);
		}

		/* Check for data to be read from the socket */
		if ((*fd_socket != -1) && FD_ISSET(*fd_socket, &readfds)) {
			if ((n = FifoFill(&Recv, *fd_socket, 0)) == -1) {
				DEBUG_PERROR("DoProcessing: read from socket");
				return;
			}
			/* The network peripheral shouldn't close the
			 * the connection on us.  Return to reconnect
			 * if it does.
			 */
			if (n == 0) {
				LogMessage(1, "Connection closed by peripheral");
				return;
			}
			LogMessage(1, "DoProcessing: %d bytes read from socket", n);
		}

		/* Check for data to be written to the pty */
		if (FD_ISSET(fd_pty, &writefds)) {
			if ((n = FifoFlush(&Recv, fd_pty)) == -1) {
				DEBUG_PERROR("DoProcessing: write to pty");
				return;
			}
			LogMessage(1, "DoProcessing: %d bytes written to pty", n);
		}

		/* Check to see if the FIFOs are exhausted; if so, initialize the
		 * pointers.
		 */

		if (FifoBytesUsed(&Xmit) == 0)
		    FifoInit(&Xmit);

		if (FifoBytesUsed(&Recv) == 0)
		    FifoInit(&Recv);

		/* Now that network data has left the local
		 * buffer, close the connection if no new
		 * data comes in before the timeout.
		 */
		if ((!KeepConnection || CloseConnection) 
       	                    && (FifoBytesUsed(&Xmit) == 0)) {
			ptimer = &timer;
			timer.tv_sec = timeout;
			timer.tv_usec = 0;
		} else
			ptimer = 0;
	}/*for*/
}


/*
 * NAME
 *	AbortDaemon - issue a logging message and abort the current daemon
 *
 * SYSNOPSIS
 *	void AbortDaemon();
 *
 * DESCRIPTION
 *	This routine is called to terminate an invocation of the daemon
 *	program. It issues an logging message, closes all file descriptors,
 *	and exits to the operating system. 
 *
 * RETURNS
 *	"AbortDaemon" returns directly to the operating system.
 */
static void
AbortDaemon(int sig)
{
	int    fd;	    /* current file descriptor */

	/* Emit a logging message */
	LogMessage(1, "Terminated");

	/* Close all file descriptors */
	for (fd = 0; fd < _NFILE; fd++)
		close(fd);

	/* Return to the operating system */
	exit(1);
}


/*
 * NAME
 *	DeadConnection - signal handler for broken connection
 *
 * SYNOPSIS
 *	void DeadConnection();
 *
 * DESCRIPTION
 *	This routine is called when the connection to the network peripheral is
 *	broken (this is normally signalled by SIGPIPE). A logging message
 *	will be emitted.
 *
 * RETURNS
 *	"DeadConnection" returns nothing.
 */
static void
DeadConnection(int sig /* unused */)
{
	LogMessage(1, "DeadConnection: entered");

	/* Emit a logging message */
	LogMessage(1, "Connection broken");
	signal(SIGPIPE, DeadConnection);
}


/*
 * NAME
 *	ForceClose - indicate that connection should be closed
 *
 * SYNOPSIS
 *	void ForceClose();
 *
 * DESCRIPTION
 *      Indicate that the connection should be closed.  This routine
 *      is called when hpnptyd receives a SIGHUP.
 *
 * RETURNS
 *	"ForceClose" returns nothing.
 */

static void
ForceClose(int sig /* unused */)
{
	LogMessage(1, "ForceClose: marking that connection is to be closed");
	CloseConnection++;
	signal(SIGHUP, ForceClose);
}
