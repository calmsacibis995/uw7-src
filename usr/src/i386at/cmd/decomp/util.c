#ident	"@(#)decomp:util.c	1.1"

/* util.c -- utility functions for gzip support
 * Copyright (C) 1992-1993 Jean-loup Gailly
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License, see the file COPYING.
 */

#include "gzip.h"

#ifndef USER_LEVEL
#include <sys/param.h>
#include <sys/bootinfo.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/cram.h>

#include <boothdr/boot.h>
#include <boothdr/initprog.h>
#include <boothdr/libfm.h>

#endif /* USER_LEVEL */

#ifndef	EOF
#define	EOF -1
#endif

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

/* ===========================================================================
 * Clear input and output buffers
 */
void clear_bufs()
{
    outcnt = 0;
    insize = inptr = 0;
}

/* ===========================================================================
 * Fill the input buffer. This is called only when the buffer is empty
 * and at least one byte is really needed.
 */
int fill_inbuf()
{
    int len;

    /* Read as much as possible */
    insize = 0;
    do {
		len = mem_copy((char *)inbuf+insize,INBUFSIZ-insize);
        if (len == 0 || len == EOF) break;
	insize += len;
    } while (insize < INBUFSIZ);

    inptr = 1;
    return inbuf[0];
}

/* ===========================================================================
 * Write the output window window[0..outcnt-1] and update crc and bytes_out.
 * (Used for the decompressed data only.)
 */
void flush_window()
{
	extern uch *global_wbuf;
	unsigned char *ob;

	if (outcnt == 0) return;
	ob = global_wbuf;
	global_wbuf = window;
	flush_mbuf();
	global_wbuf = ob;
}


/* ===========================================================================
 * Write the output buffer outbuf[0..outcnt-1] and update bytes_out.
 * (used for the compressed data only)
 */
void flush_mbuf()
{
    extern uch *global_wbuf;
    if (outcnt == 0) return;

    memcpy(global_outbuf,(char *)global_wbuf,outcnt);
    global_outbuf+=(ulg)outcnt;
    outcnt = 0;
}

/*
 * INTERFACE
 *      void bi_minit (char *outblock)
 *          Initialize the bit string routines.
 *
 */

void bi_minit(char *outb)
{
    global_outbuf = (uch *)outb;
}
