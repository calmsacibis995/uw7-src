#ident	"@(#)OSRcmds:compress/huf-table.c	1.1"
#pragma comment(exestr, "@(#) huf-table.c 25.1 92/12/01 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1991-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 * huf-table.c -- make table for decoding
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


void 
make_table (int    nchar,
            uchar  bitlen [],
            int    tablebits,
            ushort table [])
{
	ushort count[17], weight[17], start[18], *p;
	uint i, k, len, ch, jutbits, avail, nextcode, mask;

	for (i = 1; i <= 16; i++) count[i] = 0;
	for (i = 0; i < nchar; i++) count[bitlen[i]]++;

	start[1] = 0;
	for (i = 1; i <= 16; i++)
		start[i + 1] = start[i] + (count[i] << (16 - i));
	if (start[17] != (ushort)((unsigned) 1 << 16))
		FatalError (MSGSTR(COMPRESS_BADDECO,"Bad decode table"));
								/* L001 */

	jutbits = 16 - tablebits;
	for (i = 1; i <= tablebits; i++) {
		start[i] >>= jutbits;
		weight[i] = (unsigned) 1 << (tablebits - i);
	}
	while (i <= 16) {
	   weight[i] = (unsigned) 1 << (16 - i);
	   i++;
        }

	i = start[tablebits + 1] >> jutbits;
	if (i != (ushort)((unsigned) 1 << 16)) {
		k = 1 << tablebits;
		while (i != k) table[i++] = 0;
	}

	avail = nchar;
	mask = (unsigned) 1 << (15 - tablebits);
	for (ch = 0; ch < nchar; ch++) {
		if ((len = bitlen[ch]) == 0) continue;
		nextcode = start[len] + weight[len];
		if (len <= tablebits) {
			for (i = start[len]; i < nextcode; i++) table[i] = ch;
		} else {
			k = start[len];
			p = &table[k >> jutbits];
			i = len - tablebits;
			while (i != 0) {
				if (*p == 0) {
					right[avail] = left[avail] = 0;
					*p = avail++;
				}
				if (k & mask) p = &right[*p];
				else          p = &left[*p];
				k <<= 1;  i--;
			}
			*p = ch;
		}
		start[len] = nextcode;
	}
}
