#ident	"@(#)OSRcmds:compress/lzh-defs.h	1.1"
#pragma comment(exestr, "@(#) lzh-defs.h 25.1 94/11/08 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1991-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 * lzh-defs.h
 *-----------------------------------------------------------------------------
 * From public domain code in zoo by Rahul Dhesi, who adapted it from "ar"
 * archiver written by Haruhiko Okumura.
 *=============================================================================
 *
 *	L000	scol!anthonys	31 Oct 94
 *	- Corrected some compiler warnings.
 */

/*
 * Standard system includes.
 */
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>				/* L000 */

/*
 * Basic datatypes.
 *    Note:  t_uint16 must be exactly 16 bits.
 */

typedef unsigned char  uchar;    /*  8 bits or more */
typedef unsigned short t_uint16; /* exactly 16 bits */

/*
 * For lzh modules to use in defining buffer size.
 */
#define DICBIT    13    /* 12(-lh4-) or 13(-lh5-) */
#define DICSIZ ((unsigned) 1 << DICBIT)

/* 
 * Defines used in multiple modules
 */
#define NC (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)
	/* alphabet = {0, 1, 2, ..., NC - 1} */
#define CBIT 9  /* $\lfloor \log_2 NC \rfloor + 1$ */
#define CODE_BIT  16  /* codeword length */

#define BITBUFSIZ (CHAR_BIT * sizeof bitbuf)
#define MAXMATCH 256    /* formerly F (not more than UCHAR_MAX + 1) */
#define THRESHOLD  3    /* choose optimal value */

/*
 * huf-coding.c externals.
 */
extern int    decoded;
extern ushort left[], right[];

void
HufOutput (uint c,
           uint p);

void
huf_encode_start();

void
huf_encode_end ();

uint
decode_c ();

uint
decode_p ();

void
huf_decode_start ();

/*
 * huf-table.c externals.
 */
void 
make_table (int    nchar,
            uchar  bitlen [],
            int    tablebits,
            ushort table []);

/*
 * huf-tree.c externals.
 */
int 
make_tree (int    nparm,
           ushort freqparm [],
           uchar  lenparm [],
           ushort codeparm []);

/*
 * lzh-crc.c externals.
 */
void
addbfcrc (register char *buffer,
          register int count);

/*
 * lzh-decode.c externals.
 */
extern FILE *arcfile;

int
lzh_decode (FILE *infile,
            FILE *outfile);

/*
 * lzh-encode.c externals.
 */
extern FILE *lzh_infile;
extern FILE *lzh_outfile;

void
lzh_encode (FILE          *infile,
            FILE          *outfile,
            unsigned long *originalSize,
            unsigned long *compressSize);

/*
 * lzh-io.c externals.
 */
extern t_uint16 bitbuf;
extern int      unpackable;
extern ulong    compsize, origsize;

void
fillbuf (int n);

uint
getbits (int n);

void
putbits (int  n,
         uint x);

int 
fread_crc (uchar *p,
           int    n,
           FILE  *f);

void
fwrite_crc (uchar *p,
            uint   n,				/* L000 */
            FILE  *f);

void
init_getbits ();

void
init_putbits ();

/*
 * lzhutil.c externals.
 */
void
FatalError (char *errorMsg);

void
FatalUnixError (char *errorMsg);

void *
MemAlloc (size_t size);


