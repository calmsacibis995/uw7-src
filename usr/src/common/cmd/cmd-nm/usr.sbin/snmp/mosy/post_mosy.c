#ident	"@(#)post_mosy.c	1.3"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)post_mosy.c	1.1 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
#ifndef lint
static char SysVr3TCPID[] = "@(#)post_mosy.c	6.1 Lachman System V STREAMS TCP source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
/* pcm.c - back end to the MOSY compiler */

/*
 *      System V STREAMS TCP - Release 5.0
 *
 *  Copyright 1992 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 * 
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

#include <stdio.h>
#include <fcntl.h>
#include <snmp.h>
#include <objects.h>

static	char   *myname = "post_mosy";
char   *i_file = NULLCP;	/* input file with MOSY compiled defns. */
char   *o_file = NULLCP;	/* Output file */
int	compile ();

main (argc, argv)
int	argc;
char  **argv;
{
    arginit (argv);
    compile ();
}

int
compile ()
{
    register OT	    ot;
    FILE    *fp;

    if (o_file) {
	if ((fp = fopen (o_file, "w")) == NULL) {
	    fprintf (stderr, "unable to write");
	    return OK;
	}
    }
    else
	fp = stdout;

    for (ot = text2obj ("ccitt"); ot; ot = ot -> ot_next)
	fprintf (fp, "\"%s\" \t \"%s\"\n", ot -> ot_text, sprintoid (ot -> ot_name));

    if (o_file)
	(void) fclose (fp);

    return OK;
}

int
arginit (vec)
char    **vec;
{
    register char  *ap, *pp;

    for (vec++; ap = *vec; vec++) {
	if (*ap == '-') {
	    while (*++ap)
		switch (*ap) {
		    case 'i':
			if ((pp = *++vec) == NULL || *pp == '-') {
			    fprintf (stderr, "Usage: %s -i i_file [-o o_file] \n", myname);
			    exit(-1);
			}
			i_file = pp;
			break;

		    case 'o':
			if ((pp = *++vec) == NULL) {
			    fprintf (stderr, "Usage: %s -i i_file [-o o_file] \n", myname);
			    exit(-1);
			}
			if (*pp == '-')
			    break;
			o_file = pp;
			break;

		    default:
			fprintf (stderr, "Unknown switch -%c \n", *ap);
			fprintf (stderr, "Usage: %s -i i_file [-o o_file] \n", myname);
			exit(-1);
		}
	    continue;
	}
    }

    if (readobjects(i_file) == NOTOK) {
	fprintf(stderr, "readobjects: failed");
	exit (-1);
    }
}
