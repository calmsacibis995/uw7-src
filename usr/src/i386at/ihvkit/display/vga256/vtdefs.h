#ident	"@(#)ihvkit:display/vga256/vtdefs.h	1.1"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/************
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 * All rights reserved.
 ***********/

#ifndef NULL
#define NULL 	0
#endif

#define PG_SIZE		4096	/* page size in 386/ix */
#define SLBYTES 2048	/* max number of bytes in a scanline for V256 */

/*
 * Stuff that's been removed from kd.h in 2.0
 */
#ifdef	SEQ_RESET
#undef	SEQ_RESET
#endif
#define	SEQ_RESET	0x01	/* sychronously reset sequencer */

/*
 *  Temporary change. Sequencer timings changed 1/5  for processor access.
 *
#ifdef	SEQ_RUN
#undef	SEQ_RUN
#endif
#define	SEQ_RUN	0x02	 
* sychronously reset sequencer */

/*
 *  End of change.
 */

#ifdef	V256_BASE
#undef	V256_BASE
#endif
#define	V256_BASE	0xa0000	/* location of enhanced display memory */

/*
 * Define MCOUNT to be nothing so we can successfully build without profiling
 */
#define	MCOUNT
