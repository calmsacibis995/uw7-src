/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
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

#ident	"@(#)bootcntl.c	15.1"

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/bootinfo.h>
#include <sys/bootcntl.h>
#include <pfmt.h>
#include <locale.h>


#define equal(a,b)      (strcmp(a, b) == 0)

char    *bootname;              /* pointer to boot file name 	*/
char	rfs[B_STRSIZ];		/* input parameter string	*/
char 	options_string[2] = "r:";
int	bootfd;
char	*myname;

main(argc,argv)
int argc;
char *argv[];
{
        extern char     *optarg;
        extern int      optind;
        int     	c;
        int     	errflg = 0;
	struct		bootcntl hpbootcntl;
	int		i;
	int		cnt;
	int		bcsize;
	char		param[B_STRSIZ];
	char		*idxp;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxbootcntl");
	(void)setlabel("UX:bootcntl");

	myname = (char*)strrchr(argv[0], '/');
	myname = (myname != 0)? myname+1: argv[0];
        while ((c = getopt(argc, argv, options_string)) != -1) {
                switch (c) {
                case 'r':
			if ((int)strlen(optarg) >= B_STRSIZ) {
				pfmt(stderr, MM_ERROR,
				    ":1:Root file system string too long.\n");
				exit(15);
			}
			strcpy(rfs,optarg);
			break;
		case '?':
			errflg++;
		}
	}

        if (argc - optind < 1)
                ++errflg;
        if (errflg) {
                giveusage();
                exit(40);
        }

	bootname = argv[optind];
        if ((bootfd = open(bootname, O_RDWR)) == -1) {
		pfmt(stderr, MM_ERROR,
			 ":2:Unable to open specified boot program.\n");
		exit(10);
	}

/*	boot control block is at the second sector of the boot program	*/
	bcsize = sizeof(struct bootcntl);
	lseek(bootfd, 512, 0);
	if ((cnt = read(bootfd, (char *)&hpbootcntl, bcsize)) != bcsize) {
		pfmt(stderr, MM_ERROR,
			":3:Cannot read boot control block.\n");
		exit(30);
	}

/*	check for high performance boot program				*/
	if (hpbootcntl.bc_magic != BPRG_MAGIC) {
		pfmt(stderr, MM_ERROR,
			":4:Invalid boot control block.\n");
		exit(20);
	}

/*	check for current entry for root file system parameter		*/
	for (i=0; i<hpbootcntl.bc_argc; i++) {	
		strncpy(param, (char *)(hpbootcntl.bc_argv[i]), B_STRSIZ);
		if ((idxp = strchr(param, '=')) == (char *)NULL)
			continue;
		*idxp++ = '\0';
		if (equal(param, "rootfs") || equal(param, "rootfstype"))
			break;
	}

/*	if root file system has not been defined, add the specified one	*/
	if (i >= hpbootcntl.bc_argc) {
		if (hpbootcntl.bc_argc == BC_MAXARGS) {
			pfmt(stderr, MM_ERROR,
			    ":5:Total number of parameter strings exceeds maximum.\n");
			exit(25);
		}
		hpbootcntl.bc_argc++;
	}
	strcpy((char *)(hpbootcntl.bc_argv[i]), rfs);

/*	update the boot control block					*/
	lseek(bootfd, 512, 0);
	if ((cnt = write(bootfd, (char *)&hpbootcntl, bcsize)) != bcsize) {
		pfmt(stderr, MM_ERROR,
			":6:Cannot write boot control block.\n");
		exit(31);
	}
	close(bootfd);
}

giveusage()
{
        pfmt(stderr, MM_ACTION,
		":7:Usage: %s -r root-file-system boot-program\n", myname);
}
