#ident	"@(#)OSRcmds:compress/lzh-io.c	1.1"
#pragma comment(exestr, "@(#) lzh-io.c 25.3 94/11/08 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1991-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 * lzh-io.c -- input/output
 *-----------------------------------------------------------------------------
 * From public domain code in zoo by Rahul Dhesi, who adapted it from "ar"
 * archiver written by Haruhiko Okumura.
 *=============================================================================
 */
/*
 *	Modification History
 *	L001	4 Nov 92	scol!johnfa
 *	For XPG4 Conformance;
 *	- if appending .Z to filename would make name exceed NAME_MAX bytes
 *	  the command will now fail with an error status.
 *	- converted to use getopt() to follow Utility Syntax Guidelines.
 *	- Added message catalogue functionality.
 *	L002	20 Jan 94	scol!ianw
 *	- speeded up by removing unnecessary calls to addbfcrc(), they should
 *	  have been removed when the code was ported from zoo.
 *	L003	31 Oct 94	scol!anthonys
 *	- Corrected some compiler warnings.
 */

#include "lzh-defs.h"
/* #include <errormsg.h> */						/* L001 Start */
#include <errno.h>
#ifdef INTL
#  include <locale.h>
#  include "compress_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num,str)	(str)
#  define catclose(d)		/*void*/
#endif /* INTL */						/* L001 Stop */


t_uint16 bitbuf;
int unpackable;

ulong compsize, origsize;

static uint  subbitbuf;
static int   bitcount;

/*
 * Shift bitbuf n bits left, read n bits.
 */
void
fillbuf (int n)
{
	bitbuf <<= n;
	while (n > bitcount) {
		bitbuf |= subbitbuf << (n -= bitcount);
		if (feof(arcfile))
			subbitbuf = 0;
		else
			subbitbuf = (uchar) getc(arcfile);
		bitcount = CHAR_BIT;
	}
	bitbuf |= subbitbuf >> (bitcount -= n);
}

uint
getbits (int n)
{
	uint x;

	x = bitbuf >> (BITBUFSIZ - n);  fillbuf(n);
	return x;
}

/*
 * Write rightmost n bits of x.
 */
void
putbits (int  n,
         uint x)
{
	if (n < bitcount) {
		subbitbuf |= x << (bitcount -= n);
	} else {
		(void) putc((int) (subbitbuf | (x >> (n -= bitcount))), lzh_outfile);
		compsize++;

		if (n < CHAR_BIT) {
			subbitbuf = x << (bitcount = CHAR_BIT - n);
		} else {
			(void) putc((int) (x >> (n - CHAR_BIT)), lzh_outfile);
			compsize++;
			subbitbuf = x << (bitcount = 2 * CHAR_BIT - n);
		}
	}
}

int 
fread_crc (uchar *p,
           int    n,
           FILE  *f)
{
	int i;

	i = n = fread((char *) p, 1, n, f);  origsize += n;
	return n;
}

void
fwrite_crc (uchar *p,
            uint    n,						/* L003 */
            FILE  *f)
{
	if (fwrite((char *) p, 1, n, f) < n) 
		FatalUnixError (MSGSTR(COMPRESS_IOERR,
			"I/O error or disk full."));		/* L001 */
}

void
init_getbits ()
{
	bitbuf = 0;  subbitbuf = 0;  bitcount = 0;
	fillbuf(BITBUFSIZ);
}

void
init_putbits ()
{
	bitcount = CHAR_BIT;  subbitbuf = 0;
}
