#ident	"@(#)decomp:gzip.h	1.1"

/* gzip.h -- common declarations for all gzip modules
 * Copyright (C) 1992-1993 Jean-loup Gailly.
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

#ifdef	USER_LEVEL
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define rmalloc malloc
#else  /* NOT USER_LEVEL */
#ifndef	NULL
#define	NULL	0
#endif /* NULL */
#endif /* USER_LEVEL */

#include <sys/types.h>

#define OF(args)  args
#define memzero(s, n)     memset ((s), 0, (n))

typedef void *voidp;

#ifndef RETSIGTYPE
#define RETSIGTYPE void
#endif

#define local static

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

#define DEFLATED   8
extern int method;         /* compression method */

#ifndef	INBUFSIZ
#define INBUFSIZ  0x8000  /* input buffer size */
#endif

#define INBUF_EXTRA  64     /* required by unlzw() */

#ifndef	OUTBUFSIZ
#define OUTBUFSIZ  16384  /* output buffer size */
#endif

#define OUTBUF_EXTRA 2048   /* required by unlzw() */

#define DIST_BUFSIZE 0x8000 /* buffer for distances, see trees.c */

#define EXTERN(type, array)  extern type array[]
#define DECLARE(type, array, size)  type array[size]
#define ALLOC(type, array, size)
#define FREE(array)

EXTERN(uch, outbuf);         /* output buffer */
EXTERN(ush, d_buf);          /* buffer for distances, see trees.c */
extern uch *inbuf;
extern uch *window;         /* Sliding window and suffix table (unlzw) */

extern uch * global_outbuf; 

#define tab_prefix prev    /* hash link (see deflate.c) */
#define head (prev+WSIZE)  /* hash head (see deflate.c) */

EXTERN(ush, tab_prefix);  /* prefix code (see unlzw.c) */

extern unsigned insize; /* valid bytes in inbuf */
extern unsigned inptr;  /* index of next byte to be processed in inbuf */
extern unsigned outcnt; /* bytes in output buffer */

#ifndef WSIZE
#define WSIZE 0x8000     /* window size--must be a power of two, and */
#endif                     /*  at least 32K for zip's deflate method */

#define get_byte()  (inptr < insize ? inbuf[inptr++] : fill_inbuf())

/* put_byte is used for the compressed output, put_char for the
 * uncompressed output. However unlzw() uses window for its
 * suffix table instead of its output buffer, so it does not use put_char.
 * (to be cleaned up).
 */
#define put_byte(c) {outbuf[outcnt++]=(uch)(c); if (outcnt==OUTBUFSIZ)\
   flush_outbuf();}
#define put_char(c) {window[outcnt++]=(uch)(c); if (outcnt==WSIZE)\
   flush_window();}

/* Output a 16 bit value, lsb first */
#define put_short(w) \
{ if (outcnt < OUTBUFSIZ-2) { \
    outbuf[outcnt++] = (uch) ((w) & 0xff); \
    outbuf[outcnt++] = (uch) ((ush)(w) >> 8); \
  } else { \
    put_byte((uch)((w) & 0xff)); \
    put_byte((uch)((ush)(w) >> 8)); \
  } \
}

/* Output a 32 bit value to the bit stream, lsb first */
#define put_long(n) { \
    put_short((n) & 0xffff); \
    put_short(((ulg)(n)) >> 16); \
}

extern void flush_mbuf  OF((void));
extern   int (*read_buf) OF((char *buf, unsigned size));

	/* in util.c: */
extern void clear_bufs    OF((void));
extern int  fill_inbuf    OF((void));
extern void flush_window  OF((void));

	/* in inflate.c */
extern int inflate OF((void));
extern void mem_decompress(char *, char *, ulg, ulg);
extern void mem_decomp_free(void);
