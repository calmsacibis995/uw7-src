#ident	"@(#)mp.cmds:common/cmd/pbind/pbind.c	1.3"
/*
 *	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
 *	  All Rights Reserved
 *
 *	This is UNPUBLISHED PROPRIETARY SOURCE CODE OF
 *	UNIX System Laboratories, Inc.
 *	The copyright notice above does not evidence any actual
 *	or intended publication of such source code.
 * 
 *	Copyright (c) 1990 Intel Corporation
 *	Portions Copyright (c) 1990 Ing.C.Olivetti & C., S.p.A.
 *	Portions Copyright (c) 1990 NCR Corporation
 *	Portions Copyright (c) 1990 Oki Electric Industry Co., Ltd.
 *	Portions Copyright (c) 1990 Unisys Corporation
 *	All rights reserved.
 *
 *		INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied to USL under the terms of a license
 *	agreement with Intel Corp. and may not be copied or disclosed
 *	except in accordance with the terms of that agreement.
 */

#include <sys/types.h>
#include <sys/procset.h>
#include <sys/processor.h>
#include <sys/bind.h>
#include <dirent.h>
#include <stdio.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>

int bind = 0;
int unbind = 0;
int verbose = 0;
int query = 0;

/*
 * Command: pbind
 *
 *	o	Bind  process[es] to processor_id
 *	o	Remove bindings from process[es]
 *	o	display binding information
 *
 */


usage()
{
	pfmt(stderr, MM_ERROR, ":7:Incorrect Usage\n");
	pfmt(stderr, MM_ACTION, ":13:usage: pbind -b processor pid...\n");
	pfmt(stderr, MM_ACTION, ":14:       pbind -u pid...\n");
	pfmt(stderr, MM_ACTION, ":15:       pbind -q [pid...]\n");
	exit(1);
}

main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;
	int t;
	processorid_t prc;
	int r;

	extern char *optarg;
	extern int optind;
                                        /* setlocale with "" means use
                                         * the LC_* environment variable
                                         */
        (void) setlocale(LC_ALL, "");
        (void) setcat("uxmp");         /* set default message catalog */
        (void) setlabel("UX:pbind");   /* set deault message label */


	while ((ch = getopt(argc, argv, "b:uqv")) != EOF)  switch (ch)
	{
	case 'b':
		bind++;
		if ((t = getint(optarg)) < 0)
		{
			pfmt(stderr, MM_ERROR,
			   ":16:invalid processor id %s\n", optarg);
			exit(1);
		}
		prc = (processorid_t) t;
		break;
	case 'u':
		unbind++;
		prc = PBIND_NONE;
		break;
	case 'q':
		query++;
		prc = PBIND_QUERY;
		verbose++;
		break;
	case 'v':
		verbose++;
		break;
	default:
		usage();
	}

	if (bind + unbind + query != 1)
		/*
		 * Must specify exactly one of -b, -u, -q.
		 */
		usage();

	r = 0;
	if (optind >= argc)
	{
		/*
		 * No pid arguments: must be "pbind -q".
		 */
		if (!query)
			usage();
		if (query_all())
			r = 1;
	} else
	{
		/*
		 * Run thru the list of pids.
		 */
		for ( ;  optind < argc;  optind++)
		{
			if ((ch = getint(argv[optind])) < 0)
			{
				pfmt(stderr,MM_ERROR, ":17:invalid process id %s\n",
					argv[optind]);
				usage();
			}
			if (pbind((id_t)ch, prc,1))
				r = 1;
		}
	}
	exit(r);
}

/*
 * Query all the processes in the system.
 */
query_all()
{
	register DIR *fd;
	register struct dirent *d;
	register id_t pid;

	/*
	 * Run thru the /proc directory to find all the pids.
	 */
	if ((fd = opendir("/proc")) == NULL)
	{
		pfmt(stderr, MM_INFO, ":18:cannot open /proc\n");
		return (1);
	}

	while ((d = readdir(fd)) != NULL)
	{
		if (d->d_name[0] == '.')
			continue;
		pid = atoi(d->d_name);
		if (pid == 0)
			/*
			 * Skip names which aren't pids, like "." and "..".
			 * (Also skips process 0.)
			 */
			continue;
		pbind(pid, PBIND_QUERY,0);
	}

	closedir(fd);
	return (0);
}

/*
 * Return a string describing a binding state.
 */
	char *
binding(prc)
	processorid_t prc;
{
	static char buf[30];

	if (prc == PBIND_NONE)
		return ("unbound");
	sprintf(buf, "bound to %d", prc);
	return (buf);
}

/*
 * Bind (or unbind) a process to a processor.
 * Or just query, if the processor is PBIND_QUERY.
 */
	int
pbind(pid, prc, printit)
	id_t pid;
	int printit;
	processorid_t prc;
{
	processorid_t oprc;

	if (processor_bind(P_PID, pid, prc, &oprc) < 0)
	{
		pfmt(stdout, MM_ERROR, ":19:%s failed for pid %d\n",
				strerror(errno),pid);
		return (1);
	} 
	if (prc == PBIND_QUERY || prc == oprc) {
		if( printit || (oprc != PBIND_NONE) )
		pfmt(stdout, MM_INFO, ":20:process id %d %s\n", pid, 
								binding(oprc));
	} else {
		pfmt(stdout, MM_INFO, ":21:process id %d was %s,", 
							pid, binding(oprc));
		pfmt(stdout, MM_NOSTD, ":22: now %s\n", binding(prc));
	}
	return (0);
}

int
getint(s)
	char *s;
{
	char *es;
	long n;
	extern long strtol();

	n = strtol(s, &es, 0);
	if (es == s || *es != '\0')
		return (-1);
	return (n);
}
