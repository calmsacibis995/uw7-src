/*
 * File client.c
 * The client
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1995.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#pragma comment(exestr, "@(#) client.c 11.1 95/05/01 SCOINC")

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "dlpiut.h"

char	clientname[1024] = "client";
char	servername[1024] = "server";

int		sfd;	/* server's mailbox */
int		cfd;	/* client's mailbox */

char	ibuf[MAXMSG];
char	obuf[MAXMSG];

static void	initipc(void);
static void	startserver(void);
static void	clientdone(int code);
static void	sendserver(int argc, const char * const *argv);

void
client(int argc, const char * const *argv)
{
	sfd = -1;
	cfd = -1;

	signal(SIGINT, clientdone);
	if (strcmp(*argv, "-n") == 0) {
		argv++;
		clientname[0] = 'C';
		strncpy(&clientname[1], *argv, sizeof(clientname)-2);
		servername[0] = 'S';
		strncpy(&servername[1], *argv, sizeof(servername)-2);
		argv++;
		argc -= 2;
	}
	initipc();
	startserver();
	sendserver(argc, argv);
}

/*
 * create our mailbox
 */

static void
initipc()
{
	cfd = mcreat(clientname);
	if (cfd < 0) {
		printf("ERROR='Unable to create client ipc q'\n");
		clientdone(1);
	}
}

/*
 * connect to and start the server if needed.
 */

static void
startserver()
{
	int i;

	sfd = -1;
	sfd = mopen(servername);
	if (sfd < 0) {
		if (fork() == 0) {
			for (i = 3; i < 20; i++)
				close(i);
			server();
		}
		/* five second timeout */
		for (i = 0; i < 50; i++) {
			nap(100);
			sfd = mopen(servername);
			if (sfd >= 0)
				break;
		}
	}
	if (sfd < 0) {
		printf("ERROR='Unable to start Server'\n");
		clientdone(1);
	}
}

/*
 * client's general purpose cleanup and exit routine
 */

static void
clientdone(int code)
{
	if (sfd >= 0)
		mclose(sfd);
	if (cfd >= 0)
		mdelete(cfd);
	exit(code);
}

/*
 * send our command line to the server for processing
 */

static void
sendserver(int argc, const char * const *argv)
{
	int i;

	i = 0;
	while (argc > 0) {
		strcpy(&ibuf[i], *argv);
		i += strlen(*argv);
		ibuf[i++] = 0;
		argc--;
		argv++;
	}
	ibuf[i++] = 0;
	msend(sfd, ibuf, i);

	ibuf[0] = 0;
	mrecv(cfd, ibuf, MAXMSG);
	if (ibuf[0])
		write(1, ibuf, strlen(ibuf));
	if (strstr(ibuf, "ERROR=") != 0)
		clientdone(1);
	else
		clientdone(0);
}

