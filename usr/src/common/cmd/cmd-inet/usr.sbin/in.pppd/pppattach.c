#ident	"@(#)pppattach.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: pppattach.c,v 1.4 1994/11/17 04:52:43 neil Exp"
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
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/syslog.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#include <pfmt.h>
#include <locale.h>
#include <unistd.h>

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

#define USAGE()  pfmt(stderr, MM_ERROR, ":90:usage: %s [-t <timeout>] [-v] attach_name\n", progname)
#define TIMEOUT  2*60 

char *progname, *attachname;
int timeout = TIMEOUT;
int vflag = 0;

/*
 * sig_usr - SIGUSR1 signal handler
 *		pppd sends a SIGUSR1 if the attachname is illegal.
 */
sig_usr(sig)
	int	sig;
{
	pfmt(stderr, MM_ERROR, ":91:%s: bad attachname: %s\n", progname, attachname);
	exit(2);
}

/*
 * sig_alm - SIGALM signal handler
 */
sig_alm()
{
	pfmt(stderr, MM_ERROR, 
 	  ":92: Connection not established within timeout period (timeout = %d seconds)\n", 
		timeout);
	exit(3);
}

main (argc,argv)
	int	argc;
	char	*argv[];
{
	msg_t	msg;
	int	s, pid, rval;
        char c;
        struct conn_made_s cm;

        (void)setlocale(LC_ALL,"");     
        (void)setcat("uxppp");
        (void)setlabel("UX:pppattach");     

	sigset(SIGUSR1, sig_usr);
	sigset(SIGALRM, sig_alm);
	
	progname = argv[0];

	if (argc == 1) {
                USAGE();
                exit(1);
        }


	while ((c = getopt(argc, argv, "t:v")) != EOF) {
	  switch(c) {
	        case 't':
                        timeout = atoi(optarg);
                        break;
                case 'v':
                        vflag++;
                        break;
                default:
                        USAGE();
                        exit(1);
                }
        }

        if (optind > argc) {
                USAGE();
                exit(1);
        }

	attachname = argv[optind];

	pid = getpid();
	memset((char *)&msg, 0, sizeof(msg));
	msg.m_type = MATTACH;
	msg.m_pid = pid;
	strncpy(msg.m_attach, attachname, NAME_SIZE);

	s = ppp_sockinit();
	if (s < 0) {
		pfmt(stderr, MM_ERROR, ":93:%s: can't connect to pppd\n", argv[0]);
		exit(1);
	}
		
	/* Send a MATTACH message to pppd */
	rval = write(s, (char *)&msg, sizeof(msg));
	if (rval < 0) {
		pfmt(stderr, MM_ERROR, ":94:%s write to socket fail\n", argv[0]);
		exit(1);
	}

	alarm(TIMEOUT);

        /*
         * Wait for daemon to notify us of success or failure.
         */

	if (vflag)
                printf(gettxt(":95", "Waiting for connection establishment ...\n"));

        if ((rval = read(s, &msg, sizeof(msg))) == 0)
                pfmt(stderr, MM_ERROR, ":96:Lost connection to PPP deamon\n");
	else {
                if (msg.m_type == MDATTACH ) {
                        if (vflag)
                                printf(gettxt(":97", "Attach to %s successful\n"), attachname);

                        if ((rval = read(s, &cm, sizeof(struct conn_made_s))) ==
 0) {
                                pfmt(stderr, MM_ERROR, ":98:Can't read connection info");
                                exit(1);
                        }
                        if (vflag) {
                                printf(gettxt(":99", "Interface %s created\n"), cm.ifname);
                                printf(gettxt(":100", "Remote Address: %s\n"),
                                        inet_ntoa(cm.dst.sin_addr));
                                printf(gettxt(":101", "Local Address:  %s\n"),
                                        inet_ntoa(cm.src.sin_addr));
                        }
                }
                else if (msg.m_type == MDATTACH_FAIL) {
                        if (vflag)
                                printf(gettxt(":102", "Attach to %s failed\n"), attachname);
                        exit(4);
                }
                else if (msg.m_type == MDATTACH_BUSY) {
                        if (vflag)
                                printf(gettxt(":103", "Already attached to %s\n"), attachname);
                        exit(5);
                }
                else {
                        pfmt(stderr, MM_ERROR, 
				":104:recieved unknown response from PPP deamon\n");
			exit(1);
		}
        }

	close(s);

	exit(0);
}

