#ident "@(#)main.c	1.7"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Lachman Technology, Inc.
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
#include <sys/types.h>
#include <sys/signal.h>
#include <fcntl.h>
#include "nbdnld.h"
extern int errno;
#include <locale.h>
#include "../nb_msg.h"
nl_catd catd;

#ifndef MSGSTR
#define MSGSTR(num,str) catgets(catd, MS_NB, (num), (str))
#endif
#ifndef MC_FLAGS
#define MC_FLAGS NL_CAT_LOCALE
#endif



#define DEFAULT_HOSTFILE	"/etc/lmhosts"
#define _PATH_PID "/etc/inet/nbd.pid"

extern void show_astat_ip (int nb_fd, unsigned long ipaddr);
extern void show_cache (int nb_fd);
extern void clear_cache (int nb_fd);
extern void init_arrays();
extern void init_neterrlist();

char *nbfile = DEFAULT_HOSTFILE;
char *nbdev  = "/dev/nb";

void
usage (char *prog)
{
/* printf("Usage: %s\t%s", prog, "[-a hostname] [-A ipaddr] [-c] [-n]\n");
 * printf("\t\t[-r] [-R [-f filename]] [-s] [S] [-t interval]\n\n");
 * printf("\t-a          List a remote host's name table given its name.\n");
 * printf("\t-A          List a remote host's name table given its IP address\n");
 * printf("\t-c          List the local name cache including the IP addresses\n");
 * printf("\t-f          Specify an IP address to NetBIOS host name mapping file\n");
 * printf("\t            to copy to the local name cache (default: /etc/lmhosts)\n");
 * printf("\t-n          List the locally registered NetBIOS names.\n");
 * printf("\t-r          List the names resolved by broadcast or name server\n");
 * printf("\t-R          Purge and reload the local name cache\n");
 * printf("\t-S          List session table showing remote IP addresses\n");
 * printf("\t-s          List session table showing remote NetBIOS host names\n");
 * printf("\t-t          Periodically display selected statistics\n");
 * printf("\thostname    Remote system's NetBIOS host name.\n");
 * printf("\tipaddr      Remote system's IP address in decimal dot notation.\n");
 * printf("\tinterval    Display update interval in seconds\n");
 * printf("\tfilename    Path name of a file containing IP address to NetBIOS\n");
 * printf("\t            host name mappings\n");
 * printf("Usage: %s\t%s", prog, "[-a hostname] [-R [-f filename]] [-s]\n\n");
 */

 printf(MSGSTR(MSG_NB_USE_USAGE1,
    "Usage: %s "), prog);
 printf(MSGSTR(MSG_NB_USE_USAGE2,
    "[-a hostname] [-c] [-s] [-R [-f filename]] \n\n"));
 printf(MSGSTR(MSG_NB_USE_A,
    "\t-a          List a remote host's name table given its name.\n"));
 printf(MSGSTR(MSG_NB_USE_C,
    "\t-c          List the local name cache including the IP addresses\n"));
 printf(MSGSTR(MSG_NB_USE_F,
    "\t-f          Specify an IP address to NetBIOS host name mapping file\n"));
 printf(MSGSTR(MSG_NB_USE_F1,
    "\t            to copy to the local name cache (default: /etc/lmhosts)\n"));
 printf(MSGSTR(MSG_NB_USE_I,
    "\t-i          Force reconfiguration over all network interfaces\n"));
 printf(MSGSTR(MSG_NB_USE_R,
    "\t-R          Purge and reload the local name cache\n"));
 printf(MSGSTR(MSG_NB_USE_S,
    "\t-s          List session table showing remote NetBIOS host names\n"));

 printf(MSGSTR(MSG_NB_USE_H,
    "\thostname    Remote system's NetBIOS host name. The host name is\n"));
 printf(MSGSTR(MSG_NB_USE_H1,
    "\t            case-sensitive and usually needs to be upper-case.\n"));
 printf(MSGSTR(MSG_NB_USE_FN,
    "\tfilename    Path name of a file containing IP address to NetBIOS\n"));
 printf(MSGSTR(MSG_NB_USE_FN1,
    "\t            host name mappings\n"));
	exit(1);
}


/* 
 * reload_intf()
 *
 * reloads the netbios interface information into the kernel.
 * - this is actually done by kicking the netBIOS daemon
 */
 
reload_intf()
{
	FILE *fp;
	pid_t nbdpid;
	
	fp = fopen(_PATH_PID, "r");
	if (fp == NULL)
		exit(0);	/* netbios not up */

	if (fscanf(fp, "%d", &nbdpid) != 1)
		exit(1);	/* file corrupt */

	kill(nbdpid, SIGUSR1);
	
}

/* Main */

main(int argc, char *argv[])
{
	char	*nbname;
	char	*nbfile = DEFAULT_HOSTFILE;
	char	*prog;
	int	aflag = 0;
	int	fflag = 0;
	int	sflag = 0;
	int	Rflag = 0;
	int	iflag = 0;
	int	cflag = 0;
	int	Aflag = 0;
	unsigned long	ipaddr;

	int	nb_fd;

	nbname = NULL;

	/* Parse the command line arguments */
			
	prog = argv[0];

        setlocale(LC_ALL,"");
        catd = catopen(MF_NB, MC_FLAGS);

	if (argc == 1)
		aflag = sflag = 1;
	else {
		while (++argv, --argc) {
			if (*argv[0] == '-') {
				switch (argv[0][1]) {
				case 'a':
					aflag = 1;
					argc--; argv++;
					if (argc == 0)
						usage(prog);
					else
						if (*argv[0] != '\0')
							nbname = argv[0];
					break;
				case 'f':
					fflag = 1;
					argc--; argv++;
					if (argc == 0)
						usage(prog);
					else
						if (*argv[0] != '\0')
							nbfile = argv[0];
					break;
				case 's':
					sflag = 1;
					break;
				case 'R':
					Rflag = 1;
					break;
				case 'i':
					iflag = 1;
					break;

				case 'c':
					/*
 					 * List the local name cache including
					 * IP addresses
					 */
					cflag = 1;
					break;
				case 'A':
					/*
					 * List remote machine's adapter status
					 * given it's IP address
					 */
					Aflag = 1;
					argc--; argv++;
					if (argc == 0)
						usage(prog);
					else
						if (*argv[0] != '\0')
							if ((ipaddr = inet_addr(argv[0])) == -1)
								usage(prog);
					break;

				default:
					usage(prog);
					break;
				}
			}
			else if (*argv[0] == '\0')
				usage(prog);
		}
	}

	init_arrays();

	if (fflag && !Rflag)
		usage(prog);

	if ((nb_fd = open(nbdev, O_RDWR)) < 0) {
		if(errno == -1)
			printf(MSGSTR(MSG_NB_NOTUP,"NetBIOS is not running\n"));
		else
			perror(nbdev);
		exit(1);
	}

	if (aflag)
		show_astat(nb_fd, nbname);

	if (sflag)
		show_sstat(nb_fd);

	if (Aflag)
		show_astat_ip(nb_fd, ipaddr);
	if (cflag)
		show_cache(nb_fd);
	if (Rflag)
	{
		/*
		 * Clear the local name cache before reloading it
		 * from  the lmhosts file
		 */
		clear_cache(nb_fd);
		lmhosts(nb_fd, nbfile);
	}

	if(iflag)
		reload_intf();

	close(nb_fd);
}
