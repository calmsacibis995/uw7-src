#ident	"@(#)uue.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)uue.c	1.3 */
#include <stdio.h>
#include <config.h>

/*
 * hack to metamail to decode uuencoded bodyparts
 * Written by Keith Moore, February 1992
 */

uueget (ptr, outfp, n)
char *ptr;
FILE *outfp;
{
    unsigned char c1, c2, c3;
    unsigned char p0, p1, p2, p3;

    p0 = (ptr[0] - ' ') & 0x3F;
    p1 = (ptr[1] - ' ') & 0x3F;
    p2 = (ptr[2] - ' ') & 0x3F;
    p3 = (ptr[3] - ' ') & 0x3F;

    c1 = p0 << 2 | p1 >> 4;
    c2 = p1 << 4 | p2 >> 2;
    c3 = p2 << 6 | p3;

    if (n >= 1)
	putc (c1, outfp);
    if (n >= 2)
	putc (c2, outfp);
    if (n >= 3)
	putc (c3, outfp);
}


getline (buf, size, fp)
char *buf;
int size;
FILE *fp;
{
    int c;
    char *ptr = buf;

    for (c = 0; c < size; ++c)
	buf[c] = ' ';
    do {
	c = getc (fp);
	if (c == EOF) {
	    *ptr = '\0';
	    return (ptr == buf) ? -1 : 0;
	}
	else if (c == '\n' || c == '\r') {
	    *ptr = '\0';
	    return 0;
	}
	else if (ptr == buf && c == '>') /* ">From" line hack */
	    continue;
	else if (size > 0) {
	    *ptr++ = c;
	    size--;
	}
    } while (1);
}


fromuue (infp, outfp, boundaries, ctptr)
FILE *infp, *outfp;
char **boundaries;
int *ctptr;
{
    char buf[63];

    while (1) {
	if (getline (buf, sizeof buf, infp) < 0) {
	    pfmt (stderr, MM_ERROR, ":2:Premature EOF!\n");
	    return;
	}
	if (strncmp (buf, "begin", 5) == 0)
	    break;
	else if (buf[0] == '-' && buf[1] == '-') {
	    if (boundaries && PendingBoundary (buf, boundaries, ctptr))
		return;
	}
    }
    while (1) {
	if (getline (buf, sizeof buf, infp) < 0) {
	    pfmt (stderr, MM_ERROR, ":2:Premature EOF!\n");
	    return;
	}
	else if (strncmp (buf, "end", 5) == 0)
	    break;
	else if (buf[0] == '-' && buf[1] == '-') {
	    if (boundaries && PendingBoundary (buf, boundaries, ctptr)) {
		pfmt (stderr, MM_ERROR, ":75:premature end of x-uue body part\n");
		return;
	    }
	    else {
		pfmt (stderr, MM_ERROR, ":74:ignoring invalid boundary marker\n");
		continue;
	    }
	}
	else if (*buf == '\0') continue;
	else {
	    int length = (*buf - ' ');
	    if (*buf == '`')
		length = 0;
	    if (length < 0 || length > 63) {
		pfmt (stderr, MM_WARNING, ":66:fromuue: illegal length (%d)\n",
			 length);
	    }
/* Nathan Maman recommends commenting out the next two lines */
	    else if (length == 0)
		break;
	    else {
		char *ptr = buf + 1;
		while (length > 0) {
		    uueget (ptr, outfp, length);
		    length -= 3;
		    ptr += 4;
		}
	    }
	}
    }
}
