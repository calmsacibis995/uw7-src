#ident	"@(#)mzip.c	15.1"

/* mzip.c -- compress files to the gzip or pkzip format
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

#include <ctype.h>
#include <stdio.h>

#ifdef HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif


/* ===========================================================================
 * Deflate in to out.
 * IN assertions: the input and output buffers are cleared.
 *   The variables time_stamp and save_orig_name are initialized.
 */
void mzip(char *ibuf, int ilen, char *obuf, int *olen)
{
    uch  flags = 0;         /* general purpose bit flags */
    ush  attr = 0;          /* ascii/binary flag */
    ush  deflate_flags = 0; /* pkzip -es, -en or -ex equivalent */
    void mem_init(char *buf, unsigned size);
    extern uch *global_wbuf;

    method = DEFLATED;
    outcnt = 0;

    global_wbuf = outbuf;
    clear_bufs();
    mem_init(ibuf,ilen);

    bi_minit(obuf);		/* Must make BI_INIT work with memory */
    ct_init(&attr, &method);	/* same with these as well */
    lm_init(level, &deflate_flags);

    (void)deflate();
    flush_mbuf();
}


static unsigned lsize;
static char *lbuf;

unsigned mem_copy(char *buf,unsigned size)
{
	if (size > lsize) size = lsize;
	if (size == 0) return size;
	memcpy(buf,lbuf,size);
	lbuf+=size;
	lsize -= size;
	return size;
}


void mem_init(char *buf, unsigned size)
{
	lbuf = buf;
	lsize = size;
}
