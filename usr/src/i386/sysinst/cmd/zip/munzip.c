#ident	"@(#)munzip.c	15.1"

/*
 * munzip.c -- decompress files in zip compression, block format
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * You may contact UNIX System Laboratories by writing to
 * UNIX System Laboratories, 190 River Road, Summit, NJ 07901, USA
 */

#include "tailor.h"
#include "gzip.h"
#include "crypt.h"

#include <stdio.h>

/* PKZIP header definitions */
#define LOCSIG 0x04034b50L      /* four-byte lead-in (lsb first) */
#define LOCFLG 6                /* offset of bit flag */
#define  CRPFLG 1               /*  bit for encrypted entry */
#define  EXTFLG 8               /*  bit for extended local header */
#define LOCHOW 8                /* offset of compression method */
#define LOCTIM 10               /* file mod time (for decryption) */
#define LOCCRC 14               /* offset of crc */
#define LOCSIZ 18               /* offset of compressed size */
#define LOCLEN 22               /* offset of uncompressed length */
#define LOCFIL 26               /* offset of file name field length */
#define LOCEXT 28               /* offset of extra field length */
#define LOCHDR 30               /* size of local header, including sig */
#define EXTHDR 16               /* size of extended local header, inc sig */


/* ===========================================================================
 * Unzip in to out.  This routine works on both gzip and pkzip files.
 *
 * IN assertions: the buffer inbuf contains already the beginning of
 *   the compressed data, from offsets inptr to insize-1 included.
 *   The magic header has already been checked. The output buffer is cleared.
 */
void munzip(char *ibuf, char *obuf, ulg ilen, ulg olen)
{
	int n;
	int res;
	extern unsigned char *global_outbuf;

	clear_bufs();
	mem_init(ibuf,ilen);
	global_outbuf = (unsigned char *)obuf;
	method = DEFLATED;

	
	res = inflate();

	if (res == 3) {
		error("out of memory");
	}
	else if (res != 0) {
		error("invalid compressed data--format violated");
	}
}
