#ident	"@(#)mmencode.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)mmencode.c	1.4 */
/*
Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)

Permission to use, copy, modify, and distribute this material
for any purpose and without fee is hereby granted, provided
that the above copyright notice and this permission notice
appear in all copies, and that the name of Bellcore not be
used in advertising or publicity pertaining to this
material without the specific, prior written permission
of an authorized representative of Bellcore.  BELLCORE
MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY
OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS",
WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
*/
#include <stdio.h>
#include <config.h>
#ifdef MSDOS
#include <fcntl.h>
#endif

#define BASE64 1
#define QP 2 /* quoted-printable */

main(argc, argv)
int argc;
char **argv;
{
    int encode = 1, which = BASE64, i, portablenewlines = 0;
    FILE *fp = stdin;

    (void) setlocale(LC_ALL, "");
    (void) setcat("uxmetamail");
    (void) setlabel("UX:mimencode");

    for (i=1; i<argc; ++i) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
		case 'u':
		    encode = 0;
		    break;
		case 'q':
		    which = QP;
		    break;
		case 'p':
		    portablenewlines = 1;
		    break;
		case 'b':
		    which = BASE64;
		    break;
		default:
		    pfmt(stderr, MM_ACTION, ":205:Usage: mimencode [-u] [-q] [-b] [-p] [file name]\n");
		    exit(-1);
	    }
	} else {
#ifdef MSDOS
	    if (encode)
		fp = fopen(argv[i], "rb");
	    else
	    {
		fp = fopen(argv[i], "rt");
		setmode(fileno(stdout), O_BINARY);
	    } /* else */
#else
	    fp = fopen(argv[i], "r");
#endif /* MSDOS */
	    if (!fp) {
		perror(argv[i]);
		exit(-1);
	    }
	}
    }
#ifdef MSDOS
    if (fp == stdin) setmode(fileno(fp), O_BINARY);
#endif /* MSDOS */
    if (which == BASE64) {
	if (encode) {
	    to64(fp, stdout, portablenewlines);
	} else {
	    from64(fp,stdout, (char **) NULL, (int *) 0, portablenewlines);
	}
    } else {
	if (encode) toqp(fp, stdout); else fromqp(fp, stdout, NULL, 0);
    }
    return(0);
}
