#ident "@(#)bootpef.c	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
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
#ifndef _BLURB_
#define _BLURB_
/************************************************************************
          Copyright 1988, 1991 by Carnegie Mellon University

                          All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of Carnegie Mellon University not be used
in advertising or publicity pertaining to distribution of the software
without specific, written prior permission.

CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
************************************************************************/
#endif /* _BLURB_ */


#ifndef lint
static char rcsid[] = "/proj/tcp/6.0/lcvs/usr/common/usr/src/cmd/net/bootpd/bootpef.c,v 6.2 1994/02/08 16:09:00 stevea Exp";
#endif


/*
 * bootpef - BOOTP Extension File generator
 *	Makes an "Extension File" for each host entry that
 *	defines an and Extension File. (See RFC1497, tag 18.)
 *
 * HISTORY
 *	See ./Changes
 *
 * BUGS
 *	See ./ToDo
 */



#ifdef	__STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <sys/types.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>	/* inet_ntoa */

#ifndef	NO_UNISTD
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <syslog.h>


#include <arpa/bootp.h>
#include "hash.h"
#include "bootpd.h"
#include "dovend.h"
#include "hwaddr.h"
#include "readfile.h"
#include "report.h"
#include "tzone.h"
#include "patchlevel.h"
#include "pathnames.h"

#define	BUFFERSIZE   		0x4000



/*
 * Externals, forward declarations, and global variables
 */

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

static void dovend_rfc1048 P((struct bootp *, struct host *, long));
static void mktagfile P((struct host *));
static void usage P((void));

#undef P


/*
 * General
 */

char *progname;
char *chdir_path;
int debug = 0;			/* Debugging flag (level) */
byte *buffer;

/*
 * Globals below are associated with the bootp database file (bootptab).
 */

char *bootptab = _PATH_CONFIG;


/*
 * Print "usage" message and exit
 */
static void
usage()
{
	fprintf(stderr,
			"usage:  bootpef [ -c dir ] [-D level] [-d] [-f configfile] [host...]\n");
	fprintf(stderr, "\t -c dir\tset current directory\n");
	fprintf(stderr, "\t -p dir\tset current directory (obsolete)\n");
	fprintf(stderr, "\t -d\tincrement debug level (obsolete)\n");
	fprintf(stderr, "\t -D n\tset debug level\n");
	fprintf(stderr, "\t -f n\tconfig file name\n");
	exit(1);
}


/*
 * Initialization such as command-line processing is done and then the
 * main server loop is started.
 */
void
main(argc, argv)
	int argc;
	char **argv;
{
    struct host *hp;
	int n;
	int c;
	extern int optind;
	extern char *optarg;

	progname = strrchr(argv[0],'/');
	if (progname) progname++;
	else progname = argv[0];

	/* Get work space for making tag 18 files. */
	buffer = (byte *) malloc(BUFFERSIZE);
	if (!buffer) {
		report(LOG_ERR, "malloc failed");
		exit(1);
	}
	
	/*
	 * Set defaults that might be changed by option switches.
	 */
	
	/*
	 * Read switches.
	 */
	while ((c = getopt(argc, argv, "c:p:dD:f:")) != EOF) {
		switch (c) {
			
		case 'p': /* chdir_path */
		case 'c': /* chdir_path */
			if (*optarg != '/') {
				fprintf(stderr,
				"bootpef: invalid chdir specification: %s\n",
					optarg);
				break;
			}
			chdir_path = optarg;
			break;
			
		case 'd': /* debug */
			/*
			 * Backwards-compatible behavior:
			 * no parameter, so just increment the debug flag.
			 */
			debug++;
			break;
		case 'D': /* debug */
			if ((sscanf(optarg, "%d", &n) != 1) || (n < 0)) {
				fprintf(stderr,
				"bootpef: invalid debug level: %s\n",
					optarg);
				break;
			}
			debug = n;
			break;
			
		case 'f': /* config file */
			bootptab = optarg;
			break;
			
		default:
			usage();
			break;
		}
	}

	argv += optind;
	argc -= optind;

	/* Get the timezone. */
	tzone_init();

	/* Allocate hash tables. */
	rdtab_init();

	/*
	 * Read the bootptab file.
	 */
	readtab(1); /* force read */
	
	/* Set the cwd (i.e. to /tftpboot) */
	if (chdir_path) {
		if (chdir(chdir_path) < 0)
			report(LOG_ERR, "%s: chdir failed", chdir_path);
	}

	/* If there are host names on the command line, do only those. */
	if (argc > 0) {
		unsigned int tlen, hashcode;

		while (argc) {
			tlen = strlen(argv[0]);
			hashcode = hash_HashFunction((u_char*)argv[0], tlen);
			hp = (struct host *) hash_Lookup(nmhashtable,
											 hashcode,
											 nmcmp, argv[0]);
			if (!hp) {
				printf("%s: no matching entry\n", argv[0]);
				exit(1);
			}
			if (!hp->flags.exten_file) {
				printf("%s: no extension file\n", argv[0]);
				exit(1);
			}
			mktagfile(hp);
			argv++;
			argc--;
		}
		exit(0);
	}
		
	
    hp = (struct host *) hash_FirstEntry(nmhashtable);
	while (hp != NULL) {
	    mktagfile(hp);
		hp = (struct host *) hash_NextEntry(nmhashtable);
    }

}



/*
 * Make a "TAG 18" file for this host.
 * (Insert the RFC1497 options.)
 */

static void
mktagfile(hp)
	struct host *hp;
{
	FILE *fp;
	int bytesleft, len;
	byte *vp;
	char *tmpstr;

    if (!hp->flags.exten_file)
		return;

	vp = buffer;
	bytesleft = BUFFERSIZE;
	bcopy(vm_rfc1048, vp, 4);		/* Copy in the magic cookie */
	vp += 4;
	bytesleft -= 4;

	/*
	 * The "extension file" options are appended by the following
	 * function (which is shared with bootpd.c).
	 */
	len = dovend_rfc1497(hp, vp, bytesleft);
	vp += len;
	bytesleft -= len;

	if (bytesleft < 1) {
		report(LOG_ERR, "%s: too much option data",
			   hp->exten_file->string);
		return;
	}
	*vp++ = TAG_END;	
	bytesleft--;

	/* Write the buffer to the extension file. */
	printf("Updating \"%s\"\n", hp->exten_file->string);
	if ((fp = fopen(hp->exten_file->string, "w")) == NULL) {
		report(LOG_ERR, "error opening \"%s\": %s",
			   hp->exten_file->string, get_errmsg());
		return;
	}
	len = vp - buffer;
	if (len != fwrite(buffer, 1, len, fp)) {
		report(LOG_ERR, "write failed on \"%s\" : %s",
			   hp->exten_file->string, get_errmsg());
	}
	fclose(fp);

} /* dovend_rfc1048 */


/*
 * Local Variables:
 * tab-width: 4
 * End:
 */
