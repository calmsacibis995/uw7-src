#ident	"@(#)OSRcmds:compress/lzh-decode.c	1.1"
#pragma comment(exestr, "@(#) lzh-decode.c 25.1 94/11/08 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1991-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 * lzh-decode.c
 *-----------------------------------------------------------------------------
 * From public domain code in zoo by Rahul Dhesi, who adapted it from "ar"
 * archiver written by Haruhiko Okumura.
 *=============================================================================
 */
/*
 *	L000	scol!hughd	30nov91
 *	- extracting a compressed tar volume with 2Mb of memory
 *	  takes about 25 minutes!  could this be because lzh_decode()
 *	  re-MemAllocs its buffer on every entry?  stop that, as
 *	  elsewhere check whether we've already allocated buffer
 *	L001	scol!anthonys	31 Oct 94
 *	- Corrected some compiler warnings.
 */

#include "lzh-defs.h"

FILE *arcfile;

static int j;  /* remaining bytes to copy */

static void
decode_start ()
{
	huf_decode_start();
	j = 0;
	decoded = 0;
}

/*
 * decode; returns no. of chars decoded 
 * The calling function must keep the number of bytes to be processed.  This
 * function decodes either 'count' bytes or 'DICSIZ' bytes, whichever is
 * smaller, into the array 'buffer[]' of size 'DICSIZ' or more.  Call
 * decode_start() once for each new file before calling this function.
 */
static uint					/* L001 */
decode (uint  count,
        uchar buffer [])
{
	static uint i;
	uint r, c;

	r = 0;
	while (--j >= 0) {
		buffer[r] = buffer[i];
		i = (i + 1) & (DICSIZ - 1);
		if (++r == count)
			return r;
	}
	for ( ; ; ) {
		c = decode_c();
		if (decoded)
			return r;
		if (c <= UCHAR_MAX) {
			buffer[r] = c;
			if (++r == count) 
				return r;
		} else {
			j = c - (UCHAR_MAX + 1 - THRESHOLD);
			i = (r - decode_p() - 1) & (DICSIZ - 1);
			while (--j >= 0) {
				buffer[r] = buffer[i];
				i = (i + 1) & (DICSIZ - 1);
				if (++r == count)
					return r;
			}
		}
	}
}

int
lzh_decode (FILE *infile,
            FILE *outfile)
{
	uint        n;						/* L001 */
static	uchar *     buffer = NULL;				/* L001 L000 */

	arcfile = infile;		/* stream to be decoded */
	if (buffer == NULL)					/* L000 */
		buffer = MemAlloc (DICSIZ);

	decode_start();
	while (!decoded) {
		n = decode((uint) DICSIZ, buffer);
		fwrite_crc(buffer, n, outfile);
	}
	return 0;
}
