#ident	"@(#)mp.cmds:common/cmd/pexbind/pexbind.c	1.3"
/*
 *		Proprietary program material
 *
 * This material is proprietary to UNISYS and is not to be reproduced,
 * used or disclosed except in accordance with a written license
 * agreement with UNISYS.
 *
 * (C) Copyright 1990 UNISYS.  All rights reserved.
 *
 * UNISYS believes that the material furnished herewith is accurate and
 * reliable.  However, no responsibility, financial or otherwise, can be
 * accepted for any consequences arising out of the use of this material.
 */

#include <sys/types.h>
#include <sys/procset.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <sys/bind.h>
#include <malloc.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>


int bind = 0;
int unbind = 0;
int verbose = 0;
int query = 0;

extern char *sys_errlist[];


/*
 * Command: pexbind
 *
 *      o       Exclusively bind  process[es] to processor_id
 *      o       Remove exclusive bindings from process[es]
 *      o       display exclusive binding information
 *
 */

usage()
{
        pfmt(stderr, MM_ERROR, ":7:Incorrect Usage\n");
        pfmt(stderr, MM_ACTION, ":23:usage: pexbind -b processor pid...\n");
        pfmt(stderr, MM_ACTION, ":24:       pexbind -u pid...\n");
        pfmt(stderr, MM_ACTION, ":25:       pexbind -q [pid...]\n");

	exit(1);
}

main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;
	int t;
	id_t pid;
	processorid_t prc;
	int r;
	id_t * pidp;
	int pidcnt=0;
	int ret;

	extern char *optarg;
	extern int optind;

                                        /* setlocale with "" means use
                                         * the LC_* environment variable
                                         */
        (void) setlocale(LC_ALL, "");
        (void) setcat("uxmp");         /* set default message catalog */
        (void) setlabel("UX:pexbind");   /* set deault message label */


	while ((ch = getopt(argc, argv, "b:uqv")) != EOF)  switch (ch)
	{
	case 'b':
		bind++;
		if ((t = getint(optarg)) < 0)
		{
			pfmt(stderr, MM_ERROR,
				":16:invalid processor id %s\n",
				optarg);
			exit(1);
		}

		prc = (processorid_t) t;
		break;
	case 'u':
		unbind++;
		break;
	case 'q':
		query++;
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
		 * Must have exactly one of -b, -u, -q.
		 */
		usage();

	r = 0;
	if (optind >= argc)
	{
		/*
		 * No pid arguments: must be "pexbind -q".
		 */
		if (!query)
			usage();
		if (query_all())
			r = 1;
	} else
	{

		/* make space for pid list */

		if( (! query ) && (optind < argc)) {

			pidp=(pid_t *)
				malloc(((argc - optind) +1)*sizeof(id_t));
		}

		/*
		 * Handle each pid argument.
		 */
		for ( ;  optind < argc;  optind++)
		{
			if ((ch = getint(argv[optind])) < 0)
			{
				pfmt(stderr, MM_ERROR, 
				 ":17:invalid process id %s\n",argv[optind]);
				usage();
			}
			pid = (id_t) ch;
			if (query){
				if( query_pid(pid,1) )
					r =1;
			}
			else 
			{
				*(pidp+pidcnt) = pid;
				pidcnt++;
			}
		}

		if (bind)
		{
			/* do the actual bindings */
			ret = processor_exbind(P_PID, pidp, pidcnt, t, 	
							(processorid_t *)0);
			if( ret < 0 ) {
				pfmt(stderr, MM_ERROR, 
					":26:%s  (failed to bind)\n",
					 strerror(errno));
			}

		} else if( unbind ) {

		
			/* do the actual unbindings */
                        ret = processor_exbind(P_PID, pidp, pidcnt, 
					PEXBIND_NONE, (processorid_t *)0);
			if( ret < 0 ) {
				pfmt(stderr, MM_ERROR, 
					":27:%s failed to release bindings\n",
					 strerror(errno));
			}

		}

	}
	if( ret < 0 ) r =1;
	exit(r);
}

/*
 * Query exclusive binding status of one process.
 */
query_pid(pid,printit)
	id_t pid;
	int printit;
{
	processorid_t prc;

 	if( processor_exbind(P_PID, &pid, 1, PEXBIND_QUERY, &prc) < 0 ){

		pfmt(stderr, MM_ERROR, ":28:cannot get status for pid %d\n",pid);
		return(1);
	}

	if( prc >= 0  ) {

		pfmt(stdout, MM_INFO, 
				":29:process id %d bound to %d\n",pid,prc);
	} else {
	
		if( printit)
		pfmt(stdout, MM_INFO, 
				":30:process id %d is not bound\n",pid);
	}
	
	return(0);
}

/*
 * Query exclusive binding status of the entire system.
 */
query_all()
{
	register DIR *fd;
	register struct dirent *d;
	register id_t pid;
	int r;

	/*
	 * Read all the entries in /proc to get 
	 * all the processes in the system.
	 */
	if ((fd = opendir("/proc")) == NULL)
	{
		pfmt(stderr, MM_ERROR, ":18:cannot open /proc\n");
		return (1);
	}

	while ((d = readdir(fd)) != NULL)
	{
		if (d->d_name[0] == '.')
			continue;
		pid = atoi(d->d_name);
		if (pid == 0)
			continue;
		if( query_pid(pid,0) ) r=1;
	}

	closedir(fd);
	return (r);
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
