#ident	"@(#)decomp:munzip.c	1.1"

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

#include "gzip.h"

#ifndef USER_LEVEL
#include <sys/bootinfo.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/cram.h>

#include <boothdr/boot.h>
#include <boothdr/initprog.h>
#include <boothdr/bootlink.h>
#include <boothdr/libfm.h>

#define	free(x)

#endif /* USER_LEVEL */

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

	if (inbuf == NULL)
		inbuf = (unsigned char *)rmalloc(32768);
	if (window == NULL)
		window = (unsigned char *)rmalloc(65536);
	clear_bufs();
	mem_init(ibuf,ilen);
	global_outbuf = (unsigned char *)obuf;
	method = DEFLATED;

	res = inflate();
	if (res == 3) {
		/* fprintf(stderr,"Out of memory\n"); */
#ifndef USER_LEVEL
		bootabort();
#endif
	}
	else if (res != 0) {
		/* fprintf(stderr,"invalid compressed data--format violated"); */
#ifndef USER_LEVEL
		bootabort();
#endif
	}
}

void mem_decompress(char *ibuf, char *obuf, ulg ilen, ulg olen)
{
	munzip(ibuf, obuf, ilen, olen);
}

void mem_decomp_free(void)
{
	if (inbuf)
		free(inbuf);
	if (window)
		free(window);
}
