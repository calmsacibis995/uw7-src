/*
 *	@(#) execmail.c 11.1 97/10/30 
 */
/***************************************************************************
 *
 *	Copyright (c) 1994		The Santa Cruz Operation, Inc.
 *
 *	All rights reserved.  No part of this program or publication may be
 *	reproduced, transmitted, transcribed, stored in a retrieval system,
 *	or translated into any language or computer language, in any form or
 *	by any means, electronic, mechanical, magnetic, optical, chemical,
 *	biological, or otherwise, without the prior written permission of:
 *	
 *		The Santa Cruz Operation, Inc.		(408) 425-7222
 *		400 Encinal St., Santa Cruz, California 95060 USA
 *
 **************************************************************************/
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1993 Lachman Technology, Inc.
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
/*	@(#) execmail.c 6.3 8/21/93 5.0
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 *
 *	- "Dummy" execmail->sendmail injection program
 *	Takes mail intended for the "real" execmail router, and
 *	calls sendmail for actual routing.
 */
/* Old SIDs: */
/* #ident "@(#)execmail.c	6.3 8/21/93 - STREAMware TCP/IP source" */
/* static char sccsid[] = "@(#)execmail.c	6.3 8/21/93 5.0"; */
/*
 * Modification History
 *
 * M000, 07-Oct-94, keithr
 *	- Call sendmail with -oem so it will send bounces when
 *	  it encounters errors.
 */

#include <signal.h>
#include <stdio.h>

#define ARGSIZE  1024		/* recipients plus flags */
#define RESERVE    11		/* number of arg postions to reserve
				   for added flags to sendmail */

int metoo = 0;
int hopcount = 0;
int nflag = 0;		/* no aliasing if set */
int fflag = 0;		/* -f from specified if set */
int rflag = 0;		/* remote mail (invoked from rmail) if set */
char *From;

#ifdef NEVER
extern char **sysnames();
#endif /* NEVER */


main(argc, argv)
int argc;
char **argv;
{
    char *argl[ARGSIZE], **ap;
    char **to;

    if (argc < 2)
	usage();
    if (argc > ARGSIZE-RESERVE) {
	fprintf(stderr, "%s: too many arguments, %d is maximum\n",
			argv[0], ARGSIZE-RESERVE);
	exit(1);
    }

    setuid(geteuid());		/* sendmail wants us to "really" be root
				   before it'll believe us about who
				   the mail is _really_ from */

    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);	/* let's hope we can finish in time */
    argv++;
    while (argc > 2 && ((*argv)[0]) == '-') {
        /* flags passed to sendmail by default: -oo */
        switch ((*argv)[1]) {
        case 'm':  
            metoo++;  
            break;	/* send to me, too */
            /* "-om" in sendmail */
        case 'n':  
            nflag++;  
            break;	/* no aliasing */
            /* identical in sendmail */
        case 'r':  
            rflag++;  
            break;	/* remote site mail */
            /* this flag internal to execmail -- not used in this version */
        case 'f':
            /* identical in sendmail */
            /* note that we have to run as root for
            				   sendmail to allow this flag */
            From = argv[2];
            fflag++;
            argc--;
            argv++;
            break;
        case 'h':
            /* identical in sendmail */
            if ((hopcount = atoi(argv[2]) + 1) < 0) {
                hopcount = 0;
            }
            argc--;
            argv++;
            break;
        default:
            usage();
            break;
        }
        argc--;
        argv++;
    }
    to = argv;	/* beginning of list of recipients */

#ifdef NEVER
    if ((Localnames = sysnames()) == NULL || Localnames[0] == NULL) {
        Lsysname = "???";
    } else if (rflag || Localnames[1] == NULL) {
        Lsysname = Localnames[0];		/* site name? */
    } else {
        Lsysname = Localnames[1];		/* machine name! */
    }
#endif /* NEVER */

    ap = argl;
    *ap++ = "sendmail";
    *ap++ = "-oo";
    *ap++ = "-oem";
    if (metoo)
        *ap++ = "-om";
    if (nflag)
        *ap++ = "-n";
    if (From) {
        *ap++ = "-f";
        *ap++ = From;
    }
    while (*to != NULL)
        *ap++ = *to++;
    ap = NULL;
    ap++;

    execv("/usr/lib/sendmail", argl);

    fprintf(stderr, "%s: exec of sendmail failed\n", argv[0]);
    exit(1);	/* an error if we return from the exec */
}


usage()
{
    fprintf(stderr, "usage: execmail [-mnr] [-f from] [-h hopcount] user ...\n");
    exit(1);
}
