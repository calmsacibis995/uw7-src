/*		copyright	"%c%"	*/

#ident	"@(#)hpnpf.c	1.3"

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
#include	<libgen.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<sys/stat.h>
#include	<sys/wait.h>
#include	<fcntl.h>
#include	<string.h>
#include	<errno.h>
#include	<unistd.h>
#include	<signal.h>
#include	"fifo.h"
#include	"snmp.h"

#include	<sys/ioctl.h>

#include <locale.h>
#include "hpnp_msg.h"

nl_catd catd;

extern int	errno;

#define        PORT         9100               /* default port number */
#define MAXFILES     100
#define LONGSLEEP    60
#define SHORTSLEEP   10


char		*ProgName;      /* argv[0] */
char		*community = COMMUNITY;
static char	*NL_Magic = "&k2G";
static char	*FileNames[MAXFILES];
int        Control_fd = -1;
int        Data_fd = -1;
int        Files;
int        LeftOver = 0;
int        PCL_NL = 0;
int        MapNL = 0;
int        NoRetry = 0;
int        Speed = 0;
int        Verbose = 0;
static int	PostscriptBanner = 0;
static int	ClosePostscriptBanner = 0;

static int	Process();
static int	Relay();
static void	SigPipe(int);
static void	Abort(int);



static void
usage(void)
{
	fprintf
	  (
	  stderr,
	  MSGSTR
	    (
	    HPNP_HPNF_USAGE,
	    "Usage: %s -x periph [-c port] [-nNrv] [-p port] [-l logfile]\n"
	    ),
	  ProgName
	  );
	exit(1);
	/* NOTREACHED */
}

main(int argc, char *argv[])
{
	int		option;			/* current option */
	int		err_flag    = 0;	/* flag for usage errors */
	char		*netperiph  = NULL;	/* peripheral name */
	unsigned int	dataport   = PORT;	/* port number */
	unsigned int	controlport = -1;
	char		*s;
	int		relay       = 0;
	int		ret;
	extern char	*optarg;	/* pointer to option argument */
	extern int	optind;		/* index of current option */

	setlocale(LC_ALL, "");
	catd = catopen( MF_HPNP, MC_FLAGS);

	for (Files = 0; Files < MAXFILES; Files++)
		FileNames[Files] = NULL;

	ProgName = basename( argv[0]);

	while (!err_flag && ((option = getopt(argc, argv, "c:l:nNPp:rRs:Svx:")) != EOF)) {
		switch (option) {
			case 'c':/* only used when contacting a relay process */
				controlport = atoi(optarg);
				break;

			case 'l':   /* logging file name */
				OpenLog(optarg);
				break;

			case 'n':
				PCL_NL++;
				break;

			case 'N':
				MapNL++;
				break;

			case 'p':   /* port number */
				dataport = atoi(optarg);
				break;

			case 'P':   /* Postscript banner Page */
				PostscriptBanner++;
				break;

			case 'r':
				relay++;
				break;

			case 'R':
				NoRetry++;
				break;

			case 's':
				StatusFile(optarg);
				break;

			case 'S':
				Speed++;
				break;
			case 'v':
				Verbose++;
				break;

			case 'x':
				if (netperiph == NULL)
					netperiph = optarg;
				else
					err_flag++;
				break;

			case '?':   /* illegal option */
				err_flag++;
				break;
		}
	}

	if (err_flag) {
		usage();
		/* NOTREACHED */
	}

	for (Files=0; optind < argc; optind++) {
		if (Files >= MAXFILES)
		{
			fprintf
			  (
			  stderr,
			  MSGSTR
			    (
			    HPNP_HPNF_FALLW, "%s: only %d files allowed\n"
			    ),
			  ProgName, MAXFILES
			  );
			/* make this an error condition */
			usage();
			/* NOTREACHED */
		}
		if (strcmp(argv[optind], "-") && access(argv[optind], R_OK))
		{
			fprintf
			  (
			  stderr,
			  MSGSTR( HPNP_HPNF_NACCF,"%s: cannot access file %s \n"
				),
			  ProgName, argv[optind]
			  );
			exit(1);
			/* NOTREACHED */
		}
		FileNames[Files++] = argv[optind];
	}/*for*/

	/*
	 *  If no files, input is stdin.
	 */
	if (Files == 0)
		FileNames[Files++] = "-";

	if (relay)
		ret = Relay(netperiph, dataport);
	else
		ret = Process(netperiph, dataport, controlport);

	/* Exit */
	EndLog();
	exit(ret);
}


/*
 * NAME
 *     Process - connect to peripheral or relay and send files
 *
 * SYNOPSIS
 *      void
 *      Process(netperiph, dataport, controlport)
 *      char *netperiph;           
 *      int  dataport;
 *      int  controlport;
 *
 * DESCRIPTION
 *      When sending directly to the peripheral, make the printjob connection
 *      and send each file.  Continue retrying the connection indefinitely
 *      unless retrying is turned off.  After a couple minutes of refused
 *      connections, check that this host is on the peripheral's access list.
 *      After sending all the data, close the sending side of the connection
 *      and wait for the peripheral to complete the close.  All data read
 *      from the peripheral is written to the stdout even after hpnpf has
 *      closed the sending side of the connection.
 *
 *      On Sun systems, we may be sending throught a relay process if hpnpf
 *      is started as an input filter.  If so, establish 2 connections to
 *      the relay process - a control connection and a data connection.
 *      After sending the file on the data connection, wait for a response
 *      on the control connection.  A successful response on the control
 *      connection means that the relay has at least written all of the
 *      data to its network connection.  When setting up the control and
 *      data connections, give up after a certain period of time.  Since
 *      the relay is on the same machine, the connection ought to succeed
 *      very quickly.
 *
 * RETURNS
 *     "Process" returns nothing.
 */

static int
Process(netperiph, dataport, controlport)
char *netperiph;
int  dataport;
int  controlport;
{
	int fd, cfd, curfile, n, sleeptime;
	char buff[BUFSIZ];

	LogMessage(1, "Process: entered");

	signal(SIGPIPE, SigPipe);
	signal(SIGINT, Abort);

	if (!ResolveAddress(netperiph))
		return 1;

	curfile = 0;
	cfd = -1;
	sleeptime = 0;

	if (Verbose && (controlport != -1))
		fprintf
		  (
		  stderr,
		  MSGSTR
		    ( 
		    HPNP_HPNF_CPORT, "Connecting to control port on %s ... "
		    ),
		  netperiph
		  );

	if (controlport != -1) {
		while (cfd == -1) {
			if ((cfd = OpenSocket(0)) < 0)
				return 1;
			if (!MakeConnection(cfd, controlport, 0)) {
				/* Since we should only be using the control
				 * port when contacting the relay process on
				 * the local machine, there is no reason to
				 * try connecting too many times.  If the relay
				 * process is not there for 5 minutes,
				 * something went wrong.
				 */
				if (NoRetry || (sleeptime == 300)) {
					if(Verbose)
						fprintf
						  (
						  stderr,
						  MSGSTR
						    ( 
						    HPNP_HPNF_FAILD, "failed\n"
						    )
						  );
					LogMessage(1, "Process: failed to connect to relay process");
					return 1;
				}
				sleep(SHORTSLEEP);
				sleeptime += SHORTSLEEP;
				close(cfd);
				cfd = -1;
			}
		}/*while*/
		if (Verbose)
			fprintf
			  (
			  stderr,
			  MSGSTR( HPNP_HPNF_CNTED, "connected\n")
			  );
	}/*if*/

	Data_fd = fd = -1;
	sleeptime = 0;

	/* Update the status file if one is used */
	if (cfd == -1)
		ConnStatus(netperiph, 1, 0);

	if (Verbose) {
		if(cfd == -1)
			fprintf
			  (
			  stderr,
			  MSGSTR( HPNP_HPNF_CONNT, "Connecting to %s ... "),
			  netperiph
			  );
		else
			fprintf
			  (
			  stderr,
			  MSGSTR( HPNP_HPNF_DPORT, "Connecting to data port on %s ... "),
			  netperiph
			  );
	}

	while (Data_fd == -1) {
		/* Use a large send buffer size if contacting
		 * the peripheral directly.  Use a default send buffer
		 * size if talking with a relay process as indicated
		 * by using a control connection.
		 */
		if ((Data_fd = OpenSocket((cfd == -1) ? 1 : 0)) < 0)
			return 1;
		if (!MakeConnection(Data_fd, dataport, 1)) {
			if (NoRetry) {
				if (Verbose)
					fprintf
					  (
					  stderr,
					  MSGSTR( HPNP_HPNF_FAILD, "failed\n")
					  );
				return 1;
			}
			if (errno == ECONNREFUSED) {

				/* First check to see whether we're on the
				 * printer's access list. If we are not then
				 * there is no point in carrying on.
				 *
				 * Don't check if we have are being used as a
				 * relay because we are not talking directly to
				 * the printer.
				 */
				if (cfd == -1 && !check_access(netperiph)) {
					fprintf
					  (
					  stderr,
					  MSGSTR
					    (
					  HPNP_HPNF_NACCS,"Do not have access\n"
					    )
					  );
					return 1;
				}

				/* If we are on the printer's access list then
				 * maybe the printer is off-line due to some
				 * problem.
				 */
				if (CheckPrinter(netperiph))
					return 1;

				/* Since we should only be using the control
				 * port when contacting the relay process on
				 * the local machine, there is no reason to
				 * try connecting too many times.  If the relay
				 * process is not there for 5 minutes,
				 * sonething went wrong.
				 */
				if ((cfd != -1) && (sleeptime == 300)) {
					if (Verbose)
						fprintf
						  (
						  stderr,
						  MSGSTR
						    ( 
						    HPNP_HPNF_FAILD, "failed\n"
						    )
						  );
					LogMessage(1, "Process: failed to connect to relay process");
					return 1;
				}

				if (sleeptime < 10) {
					/* for the first 10 seconds, sleep 2 */
					sleep(SHORTSLEEP/5);
					sleeptime += SHORTSLEEP/5;
				} else if (sleeptime < 60) {
					/* for the next 50 seconds, sleep 5 */
					sleep(SHORTSLEEP/2);
					sleeptime += SHORTSLEEP/2;
				} else {
					/* after 1 minute, sleep 10  */
					sleep(SHORTSLEEP);
					sleeptime += SHORTSLEEP;
				}

				/* Update the status file if one is used.
				 * Wait for a few refusals first.
				 */
				if ((cfd == -1) && (sleeptime >= 20))
					ConnStatus(netperiph, 0, 1);
			} else 
				sleep(LONGSLEEP);
				close(Data_fd);
				Data_fd = -1;
		}/*if*/
	}/*while*/

	if (cfd == -1)
		RestoreStatus();

	if (Verbose)
		fprintf
		  (
		  stderr, MSGSTR( HPNP_HPNF_CNTED, "connected\n")
		  );

#ifdef hpux
       /*
        * Don't ignore this signal until after the
        * connection has been made.  This allows a printjob
        * to be cancelled and hpnpf killed if hpnpf would 
        * never have succeeded in connecting.
        */
       signal(SIGTERM, SIG_IGN);
#endif

	/* check whether the printer is okay to print on
	 * ie is there paper there, etc.
	 */
	if (CheckPrinter(netperiph)) {
		fprintf
		  (
		  stderr, MSGSTR(HPNP_HPNF_PRTNR, "%s: Printer not ready...\n"),
		  ProgName
		  );
		return 1;
	}

	if (PCL_NL)
		write(Data_fd, NL_Magic, strlen(NL_Magic));

	/* Send each file in the list
	 */
	while (curfile < Files) {
		char	*filename = FileNames[curfile];

		if (strcmp(filename, "-"))
			fd = open(filename, O_RDONLY);
		else {
			fd = fileno(stdin);
			filename = "(stdin)";
		}
		/*
		 * Skip files that can't be opened.  This
		 * shouldn't happen since we earlier check
		 * that the file was readable.
		 */
		if (fd == -1) {
			fprintf(stderr, "%s: %s - %s\n", ProgName, filename, strerror(errno));
			curfile++;
			continue;
		}
		LogMessage(1, "Process: sending file %s", filename);
		if (Verbose)
			fprintf
			  (
			  stderr, 
			  MSGSTR( HPNP_HPNF_SENDF, "Sending file %s\n"),
			  filename
			  );

		if (!SendFile(fd, fileno(stdout), Data_fd)) {
			close(Data_fd);
			close(fd);
			EndLog();
			if(Verbose)
				fprintf(stderr,"\n");
			return 1;
		}
		close(fd);
		curfile++;
		if(Verbose)
			fprintf(stderr,"\n");
	}/*while*/

	if (cfd == -1) {
		/* Turn off blocking mode */
		DoNonBlocking(Data_fd, 0);

		/* Indicate that there is no more data to be sent.
		 */
		shutdown(Data_fd, 1);
		/* Wait for the rest of the data buffered in the kernel to 
		 * be received by the peripheral.  When the peripheral closes
		 * the connection, read() will return 0.  If the connection is
		 * reset, read() returns -1.  Write data sent back to stdout.
		 */
		while ((n = read(Data_fd, buff, sizeof(buff))) > 0) {
			LogMessage(1, "Process: %d bytes read from socket", n);
			write(fileno(stdout), buff, n);
		}

		if (n == -1) {
			LogMessage(1, "Process: %s", strerror(errno));
			fprintf(stderr,"%s: %s\n", ProgName, strerror(errno));
			close(Data_fd);
			return 1;
		}
		LogMessage(1, "Process: read EOF on socket");
		close(Data_fd);
	} else {
		char returncode;

		close(Data_fd);
		/* Wait for relay process to indicate that
		 * its buffers have been flushed.
		 */
		LogMessage(1, "Process: waiting for relay process");
		if ((n = read(cfd, &returncode, 1)) != 1) {
			LogMessage(1, "Process: read on control socket: %s", n, strerror(errno));
			returncode='\1';
		}
		LogMessage(1, "Process: exiting with %d", (int)returncode);
		EndLog();
		return (int) returncode;
	}

	return 0;
}

/*
 * NAME
 *     Relay - relay data from input filters to the peripheral
 *
 * SYNOPSIS
 *      void
 *      Relay(netperiph, port)
 *      char *netperiph;
 *      int  port;
 *
 * DESCRIPTION
 *      Relay is run when hpnpf is started as an output filter with
 *      the BSD spooler.  Relay first connects to the peripheral.
 *      After the connection is made, Relay accepts data (a banner page)
 *      until \031\1.  At this point, Relay forks a child that will
 *      handle one input filter.  The parent suspends itself like lpd
 *      expects.  The child accepts a control and a data connection
 *      and forwards all the data sent to the data connection.  The
 *      child then sends a single byte exit status to the control connection
 *      and exits.  After the input filter exits, lpd sends a SIGCONT
 *      to the Relay process.  The parent waits for the child to exit
 *      (which it should have already done) and then prepares to 
 *      accept another banner page.
 *
 *      The reason behind having a relay process is to send the banner
 *      page and all the files to the network peripheral over a single
 *      TCP connection.  Lpd starts up one output filter for a queue
 *      run (which becomes the relay) and it starts up one input filter
 *      for each file.  While the input filter is running, the output
 *      filter must be suspended.  To do this, the a child is forked
 *      to relay the data while the parent suspends itself.
 *
 * RETURNS
 *     "Relay" returns nothing.
 */

static int
Relay(char *netperiph, int port)
{
	int iffd, controlfd, relayfd, child, sleeptime;
	char buff[BUFSIZ];

	LogMessage(1, "Relay: entered");

	signal(SIGPIPE, SigPipe);
	signal(SIGINT, Abort);

	if (!ResolveAddress(netperiph))
		return 1;

	/* PrepListen writes the port numbers to a file */
	PrepListen(&controlfd, &relayfd);

	/* Update the status file if one is used */
	ConnStatus(netperiph, 1, 0);

	Data_fd = -1;
	sleeptime = 0;

	if (Verbose)
		fprintf
		  (
		  stderr, 
		  MSGSTR( HPNP_HPNF_CONNT,  "Connecting to %s ... "),
		  netperiph
		  );

	while (Data_fd == -1) {
		if ((Data_fd = OpenSocket(1)) < 0){
			RestoreStatus();
			return 1;
		}
		if (!MakeConnection(Data_fd, port, 1)) {
			if (NoRetry) {
				if (Verbose)
					fprintf
					  (
					  stderr,
					  MSGSTR( HPNP_HPNF_FAILD, "failed\n")
					  );
				return 1;
			}
			if (errno == ECONNREFUSED) {
				/* After a certain number of refusals,
				 * check to see if we are on the access list!
				 * If we are not on the access list,
				 * CheckAccess exits.
				 */
				if (sleeptime == 120) {
					if (check_access(netperiph)) {
						return 1;
					}
				}

				if (sleeptime < 10) {
					/* for the first 10 seconds, sleep 2 */
					sleep(SHORTSLEEP/5);
					sleeptime += SHORTSLEEP/5;
				} else if (sleeptime < 60){  
					/* for the next 50 seconds, sleep 5 */
					sleep(SHORTSLEEP/2);
					sleeptime += SHORTSLEEP/2;
				} else {                     
					/* after 1 minute, sleep 10  */
					sleep(SHORTSLEEP);
					sleeptime += SHORTSLEEP;
				}

				/* 
				 * Update the status file if one is used.
				 * Wait for a few refusals first.
				 */
				if(sleeptime >= 20)
					ConnStatus(netperiph, 0, 1);
			} else 
				sleep(LONGSLEEP);
			close(Data_fd);
			Data_fd = -1;
		}/*if*/
	}/*while*/

	RestoreStatus();
	if (Verbose)
		fprintf
		  (
		  stderr, MSGSTR( HPNP_HPNF_CNTED, "connected\n")
		  );

	while (DoBanner(Data_fd)) {
		FlushLog();
           	if ((child = fork()) == -1)
			return 1;
		if (child != 0) {
			LogMessage(1, "Relay (parent): suspending");
			/* Parent suspends itself as lpd expects.  The
			 * parent keeps the connection to the network
			 * peripheral open but doesn't write to it while
			 * a child lives.
			 *
			 * The child only lives to service one input filter
			 * connection (one file.)  Between each file, the parent
			 * is restarted and suspended again.
			 */
			kill(getpid(), SIGSTOP);
			/* Child exits when it is done writing to the socket */
			LogMessage(0, "Relay (parent): resuming & waiting for child exit");
			wait((int *)0);
			FlushLog();
		} else {
			/* The child waits for connections from input
			 * filters.  Data sent from the input filters
			 * is passed to the network peripheral.  The parent
			 * process is suspended while the child is writing
			 * to the socket.
			 */
			char returncode = '\0';

			listen(relayfd, 1);
			listen(controlfd, 1);
			if((Control_fd = MeetIF(controlfd)) < 0)
				exit(1);
			if((iffd = MeetIF(relayfd)) < 0)
				exit(1);
			if(!SendFile(iffd, iffd, Data_fd))
				returncode = '\1';

			LogMessage(1, "Relay (child): writing %d to control port", returncode);
			if(write(Control_fd, &returncode, 1) != 1)
				LogMessage(1, "Relay (child): write failed %s", strerror(errno));
			close(Control_fd);
			close(iffd);
			return 0;
		}
	}

	EndLog();
	/* Turn off blocking mode */
	DoNonBlocking(Data_fd, 0);
	/* Indicate that there is no more data to be sent */
	shutdown(Data_fd, 1);

	/* Wait for the rest of the data buffered in the kernel to 
	 * be received by the peripheral.  When the peripheral closes
	 * the connection, read() will return 0.  If the connection is
	 * reset, read() returns -1.  Discard any data sent back.
	 *
	 * Don't log any more messages.  Lpd doesn't wait for the output
	 * filter to exit and, in fact, may start a new one before this
	 * one exits.  If logging is done to the same file, the two
	 * relay processes would overwrite each others log messages.
	 */
	while(read(Data_fd, buff, sizeof(buff)) > 0) 
		/* do nothing with the data sent back */ ;

	unlink(".port");
 	close(Data_fd);

	return 0;
}

/*
 * NAME
 *     SendFile - send the file
 *
 * SYNOPSIS
 *    int
 *    SendFile(fd_hostin, fd_hostout, fd_socket)
 *    int      fd_hostin;  
 *    int      fd_hostout;
 *    int      fd_socket;
 *
 * DESCRIPTION
 *     SendFile reads from fd_hostin (a file, a pipe, a socket) and passes
 *      the data to fd_socket (a network peripheral.)  SendFile reads
 *      from fd_socket and passes the data to fd_hostout (stdout or a socket.)
 *      The FIFO data structures and routines are used to manage the
 *      transmit (to peripheral) and receive (from peripheral) FIFOs.
 *      Optionally, the FIFO read routine will convert NL to CR/LF.
 *
 *      Data is read until EOF on fd_hostin.  
 *
 * RETURNS
 *      After EOF on fd_hostin and all the data has been written to 
 *      fd_socket, return 1.  If some sort of error occurs, return 0.  If 
 *      the peripheral closes the socket, exit 1.
 */
static int
SendFile(fd_hostin, fd_hostout, fd_socket)
int            fd_hostin;  /* file descriptor for host input */
int            fd_hostout; /* file descriptor for host output */
int            fd_socket;  /* file descriptor for LAN socket */
{
	int	nfds;		/* maximum number of file descriptors */
	fd_set	readfds;	/* select mask for reading */
	fd_set	writefds;	/* select mask for writing */
	fd_set	exceptfds;	/* select mask for exceptions */
	int	n;		/* count returned */
	int	hosteof;

	LogMessage(1, "SendFile: entered");

	hosteof = 0;

	/* Initialize the maximum number of file descriptors and
	 * the select masks.
	 */

#define MAX(a,b) (a) > (b) ? (a) : (b)

	nfds = MAX(fd_hostin, fd_hostout);
	nfds = MAX(nfds, fd_socket);
	nfds++;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	FifoInit(&Xmit);
	FifoInit(&Recv);

	/* Loop forever */
	for (;;) {
		/* if the appropriate FIFOs are not full, set up the select
		 * read masks for the host and socket.
		 */
		if (!hosteof && 
		    /* if not mapping newlines, fill the FIFO 
		     */
		    ((!MapNL && (FifoBytesFree(&Xmit) >  0)) ||
		    /* if mapping newlines, empty partial buffer before filling 
		     * since we can't accurately fill the buffer - the
		     * mapping expands the data read.
		     */
		    (MapNL && (FifoBytesUsed(&Xmit) == 0))) )
			FD_SET(fd_hostin, &readfds);
		else
			FD_CLR(fd_hostin, &readfds);

		if (FifoBytesFree(&Recv) > 0)
			FD_SET(fd_socket, &readfds);
		else
			FD_CLR(fd_socket, &readfds);

		if (FifoBytesUsed(&Recv) > 0)
			FD_SET(fd_hostout, &writefds);
		else
			FD_CLR(fd_hostout, &writefds);


		/* If the appropriate FIFOs are not empty, set up the select
		 * write masks for the socket and host.
		 */
		if (FifoBytesUsed(&Xmit) > 0)
			FD_SET(fd_socket, &writefds);
		else
			FD_CLR(fd_socket, &writefds);

		/* Wait for something to happen */
		if ((n = select(nfds, &readfds, &writefds, &exceptfds, 0)) == -1) {
			if (errno == EINTR)
				continue;
			LogMessage(1, "SendFile: error (select): %s", strerror(errno));
			return(0);
		}

		/* Check for a timeout -- shouldn't happen */
		if (n == 0)
			continue;


		/* Check for data to be read from the host */
		if (FD_ISSET(fd_hostin, &readfds)) {
			if ((n = FifoFill(&Xmit, fd_hostin, MapNL)) == -1) {
				LogMessage(1, "SendFile: error (FifoFill fd_hostin): %s", strerror(errno));
				fprintf(stderr, "%s: %s\n", ProgName, strerror(errno));
				return(0);
			}
			LogMessage(0, "SendFile: %d bytes read from host",n);

			if(n == 0)
				hosteof = 1;
		}


		/* Check for data to be written to the socket */
		if (FD_ISSET(fd_socket, &writefds)) {
			if ((n = FifoFlush(&Xmit, fd_socket)) == -1) {
				if (errno != EWOULDBLOCK) {
					LogMessage(1, "SendFile: error (FifoFlush fd_socket): %s", strerror(errno));
					fprintf(stderr, "%s: %s\n", ProgName, strerror(errno));
					return(0);
				}
			} else {
				LogMessage(0, "SendFile: %d bytes written to socket", n);
				if (Verbose) {
					for (LeftOver += n; LeftOver >= 1024; LeftOver -= 1024)
						fprintf(stderr, "#");
				}
			}
		}

		/* Check for data to be read from the socket */
		if (FD_ISSET(fd_socket, &readfds)) {
			if ((n = FifoFill(&Recv, fd_socket, 0)) == -1) {
				LogMessage(1, "SendFile: error (FifoFill fd_socket): %s", strerror(errno));
				fprintf(stderr, "%s: %s\n", ProgName, strerror(errno));
				return(0);
			}

			LogMessage(0, "SendFile: %d bytes read from socket", n);

			/* The network peripheral shouldn't close the
		 	* the connection on us.  Exit with value 1 so hpnp.model
		 	* restarts the job or lpd restarts the job if we are an 
		 	* input filter.
		 	*/
			if (n == 0) {
				LogMessage(1, "SendFile: connection closed by peripheral");
				fprintf
				  (
				  stderr,
				  MSGSTR
				    ( 
				    HPNP_HPNF_CCLOS,
				    "%s: connection closed by peripheral\n"
				    ),
				  ProgName
				  );
				exit(1);
			}
		}

		/* Check for data to be written to the host */
		if (FD_ISSET(fd_hostout, &writefds))
		{
			if ((n = FifoFlush(&Recv, fd_hostout)) == -1) {
				if (errno != EWOULDBLOCK) {
					LogMessage(1, "SendFile: error (FifoFlush fd_hostout): %s", strerror(errno));
					fprintf(stderr, "%s: %s\n", ProgName, strerror(errno));
					return(0);
				}
			} else
				LogMessage(0, "SendFile: %d bytes written to host", n);
		}

		/* Check to see if the FIFOs are exhausted; if so, initialize
		 * the pointers */
		if (FifoBytesUsed(&Xmit) == 0)
			FifoInit(&Xmit);

		if (FifoBytesUsed(&Recv) == 0)
			FifoInit(&Recv);

		if (hosteof && (FifoBytesUsed(&Xmit) == 0) && (FifoBytesUsed(&Recv) == 0))
			return(1);
	}/*for*/
}

/*
 * NAME
 *     DoBanner - handle an lpd banner page
 *
 * SYNOPSIS
 *      DoBanner(sfd)
 *      int sfd;
 *
 * DESCRIPTION
 *      DoBanner accepts a banner page from lpd and sends it over
 *      the socket.  The sequence \031\1 ends the banner page.  DoBanner
 *      sees EOF when lpd exits.  Optionally, the sequence to set the
 *      printer line terminator to newline preceeds the banner page.
 *
 * RETURNS
 *     "DoBanner" returns 0 on EOF; 1 otherwise.
 */
static int
DoBanner(int sfd)
{
	int cc = 0;
	int c, d;
	char buff[BUFSIZ];
	int did_pcl_nl = 0;

	LogMessage(1, "DoBanner: entered");

	while ((c = getc(stdin)) != EOF) {
		switch(c) {

		case '\031':
			if ((d = getc(stdin)) == '\1') {
				if (!cc)
					return 1;
				if (PCL_NL && !did_pcl_nl) {
					LogMessage(1, "DoBanner: %d bytes written to socket", strlen(NL_Magic));
					write(sfd, NL_Magic, strlen(NL_Magic));
					did_pcl_nl = 1;
				}

				LogMessage(1, "DoBanner: %d bytes written to socket", cc);
				write(sfd, buff, cc);

				/* Send out the Closing Postscript commands to 
				 * Print out the Banner.
				 */
				if (ClosePostscriptBanner) {
					strncpy(&buff[0],") show \n showpage\n  ",20);
					cc = 20;

					LogMessage(1, "DoBanner: %d bytes written to socket", cc);
					write(sfd, buff, cc);

					PostscriptBanner++;
					ClosePostscriptBanner = 0;
				}

				return(1);
			}/*if*/
			ungetc(d, stdin);
			/* fall through */

		default: 
		/*
		 * This code should only be entered once per connection if 
		 * we are outputing a postscript banner page. 
		 */
		if (PostscriptBanner) {
			cc = 0;
			strncpy(&buff[cc],"/TopOfPage 11.00 72 mul def \n",29);
			cc += 29;
			strncpy(&buff[cc],"/LeftMargin .375 72 mul def \n",29);
			cc += 29;
			strncpy(&buff[cc],"/Courier findfont \n",19);
			cc += 19;
			strncpy(&buff[cc],"14 scalefont \n",14);
			cc += 14;
			strncpy(&buff[cc],"setfont \n",9);
			cc += 9;
			strncpy(&buff[cc],"LeftMargin TopOfPage 100 sub moveto \n",37);
			cc += 37;
			strncpy(&buff[cc],"(",1);
			cc += 1;

			LogMessage(1, "DoBanner: %d bytes written to socket", cc);
			write(sfd, buff, cc);

			cc = 0;
			PostscriptBanner = 0;
			ClosePostscriptBanner++;
		}

		if (MapNL && (c == '\n'))
			buff[cc++] = '\r';
		buff[cc++] = c;
		if (cc == BUFSIZ) {
			if (PCL_NL && !did_pcl_nl) {
				LogMessage(1,"DoBanner: %d bytes written to socket", strlen(NL_Magic));
				write(sfd, NL_Magic, strlen(NL_Magic));
				did_pcl_nl = 1;
			}
			LogMessage(0, "DoBanner: %d bytes written to socket", cc);
			write(sfd, buff, cc);
			cc = 0;
		}
		break;
		}/*switch*/

	}/*while*/

	if (cc != 0) {
		if (PCL_NL && !did_pcl_nl) {
			LogMessage(1,"DoBanner: %d bytes written to socket", strlen(NL_Magic));
			write(sfd, NL_Magic, strlen(NL_Magic));
			did_pcl_nl = 1;
		}
		LogMessage(0, "DoBanner: %d bytes written to socket", cc);
		write(sfd, buff, cc);

		/* Send out the Closing Postscript commands to 
		* Print out the Banner.
		*/
		if (ClosePostscriptBanner) {
			strncpy(&buff[0],") show \n showpage\n  ",20);
			cc = 20;

			LogMessage(1, "DoBanner: %d bytes written to socket", cc);
			write(sfd, buff, cc);

			PostscriptBanner++;
			ClosePostscriptBanner = 0;
		}

	}

	LogMessage(1, "DoBanner: EOF");
	return(0);
}

/*
 * NAME
 *     CheckPrinter - check printer for paper, toner, etc.
 *
 * SYNOPSIS
 *     int
 *     CheckPrinter(netperiph)
 *     char *netperiph;
 *
 * DESCRIPTION
 *     Uses the getone SNMP utility to obtain peripheral specific information
 *     such as the paper status, toner status, etc.
 *     If the status indicates that the printer cannot print then exit with 1.
 *     Otherwise return 0.
 *
 * RETURNS
 *     Returns 0 if the printer can print a request.
 *     Returns 1 if the printer is unable to print
 *     The reason for the printer being unable to print is printed on the
 *     stderr.
 */

static int
CheckPrinter(char *netperiph)
{
	char pbuf[BUFSZ], line[BUFSZ], *val, *eval, *epbuf;
	FILE *pfd;

	if (access(GETONE, X_OK)) {
		/* no-way of contacting the printer..... */
		return 1;
	}

	/* Construct the first part of the command argument for popen */
	sprintf(pbuf, "%s %s %s ", GETONE, netperiph, COMMUNITY);

	/* Mark the end of this first part of the command argument */
	epbuf = strrchr(pbuf, ' ');
	epbuf++;

	/* First of all check that the printer is online:
	 * HP.2.3.9.1.1.2.1.0 refers to the printer status in the
	 * printer's MIB.
	 */
	strcat(epbuf, SNMP_ONLINE);
	pfd = popen(pbuf, "r");

	/* Read the output of the getone command  */
	val = NULL;
	while (fgets(line, BUFSZ, pfd) != NULL) {
		/* Skip lines that are not 'Value: ' */
		if (line[0] != 'V')
			continue;

		/* Assume line we have got is a "Value: ...." line
		 */
		val = strchr(line, ' ');
		val++;
		break;
	}/*while*/
	if (val == NULL)
		return 1;

	/*
	 * Strip off trailing newline.
	 */
	eval = strrchr(line, '\n');
	*eval = '\0';

	switch (*val) {
		case '1': /* Printer is off-line, maybe a reason for this
			   * which we shall check later.
			   */
			fprintf
			  (
			  stderr,
			  MSGSTR( HPNP_HPNF_OFFLN, "%s is off-line\n"),
			  netperiph
			  );
			pclose(pfd);
			break;
		case '0': /* Printer is on-line. */
			pclose(pfd);
			return(0);
			break;
		default: /* Some other value returned, not good */
			pclose(pfd);
			break;
	}/*switch*/

	/* Deal with unknown status first.
	 */
	if (*val != '1')
		fprintf
		  (
		  stderr,
		  MSGSTR
		    ( 
		    HPNP_HPNF_USTAT, "Unknown status returned by %s - %s\n\n"
		    ),
		  netperiph, val
		  );

	/* Find out why the printer is off-line.
	 */
	fprintf
	  (
	  stderr,
	  MSGSTR( HPNP_HPNF_STATP, "Current status of printer is:\n\n\t")
	  );

	/* Point to the last space in the command buffer for popen. This
	 * enables us to overwrite the previous argument.
	 */
	 epbuf = strrchr(pbuf, ' ');

	/* Replace the previous argument with HP.2.4.3.1.2.0
	 * This part of the printer's MIB tells us whether the printer is
	 * printing, ready to print, etc.
	 */
	strcpy(epbuf, SNMP_PRINTING);
	pfd = popen(pbuf, "r");

	/*
	 * Read the output of the getone command.
	 */
	while (fgets(line, BUFSZ, pfd) != NULL) {
		if (line[0] != 'V')
			continue;
		break;
	}


	/* Assume this line is "Value: ...." and print out the value.
	 */
	val = strchr(line, ' ');
	val++;
	fprintf(stderr, "%s\n\t", val);

	/* Now check that there is paper:
	 * HP.2.3.9.1.1.2.2.0 is the paper status.
	 */
	epbuf = strrchr(pbuf, ' ');

	strcpy(epbuf, SNMP_PAPER);
	pfd = popen(pbuf, "r");

	/*
	 * Read the output of getone.
	 */
	val = NULL;
	while (fgets(line, BUFSZ, pfd) != NULL) {
		/*
		* Skip lines beginning "Name: ...." or blank lines
		*/
		if (line[0] != 'V')
			continue;
		/*
		 * Assume this line is "Value: ...."
		 */
		val = strchr(line, ' ');
		val++;
		break;
	}
	if (!val)
		return 1;

	pclose(pfd);
	if (*val != '0') {
		/* Paper is not okay. */
		fprintf
		  (
		  stderr,
		  MSGSTR( HPNP_HPNF_PAPER, "%s has a paper problem\n\t"),
		  netperiph
		  );
		pclose(pfd);
		return 1;
	}

	/* Now check for some other error that requires human intervention:
	 * HP.2.3.9.1.1.2.3.0 is the intervention status of the printer.
	 */
	epbuf = strrchr(pbuf, ' ');
	strcpy(epbuf, SNMP_INTERVENTION);
	pfd = popen(pbuf, "r");

	/*
	 * Read the output of getone.
	 */
	val = NULL;
	while (fgets(line, BUFSZ, pfd) != NULL) {
		if (line[0] != 'V')
			continue;
		/*
		 * Assume this line is "Value: ...."
		 */
		val = strchr(line, ' ');
		val++;
		break;
	}
	if (val)
		return 1;

	pclose(pfd);
	if (*val != '0') 
	{
		/* Intervention required. */
		fprintf
		  (
		  stderr,
		  MSGSTR( HPNP_HPNF_HUMNI, "%s requires human intervention\n"),
		  netperiph
		  );
		return(1);
	}

	fprintf(stderr, "\n");
	return(0);
}

/*
 * NAME
 *     SigPipe - handle SIGPIPE
 *
 * SYNOPSIS
 *      void
 *      SigPipe()
 *
 * DESCRIPTION
 *      Handle SIGPIPE.  Log to the log file.  Write a message to 
 *      stderr.  Send an exit value over the control line.
 *
 * RETURNS
 *     "SigPipe" returns nothing.
 */

static void
SigPipe(int sig)
{
	LogMessage(1, "SigPipe: received SIGPIPE");
	fprintf
	  (
	  stderr,
	  MSGSTR( HPNP_HPNF_RPEER, "%s: connection reset by peer\n"),
	  ProgName
	  );

	if (Control_fd != -1) {
		LogMessage(1, "SigPipe: writing 1 to control port");
		write(Control_fd, "\1", 1);
	}
	EndLog();
	exit(1);
}

/*
 * NAME
 *     Abort - handle SIGINT
 *
 * SYNOPSIS
 *      void
 *      Abort()
 *
 * DESCRIPTION
 *      Handle SIGINT.  Restore the status file to the old status
 *      if hpnpf had put something else into the status file.
 *
 * RETURNS
 *     "Abort" returns nothing.
 */

static void
Abort(int sig)
{
	LogMessage(1, "Abort: received SIGINT");
	EndLog();
	RestoreStatus();
	exit(1);
}
