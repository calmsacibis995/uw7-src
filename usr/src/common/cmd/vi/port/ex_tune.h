/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* Copyright (c) 1981 Regents of the University of California */
#ident	"@(#)vi:port/ex_tune.h	1.12.2.4"
#ident  "$Header$"

#include <sys/param.h>
/*
 * Definitions of editor parameters and limits
 */

/*
 * Pathnames.
 */
#define	EXRECOVER	"/usr/lib/exrecover"
#define	EXPRESERVE	"/usr/lib/expreserve"
#define USRPRESERVE	"/var/preserve/"
#define	TMPDIR	"/var/preserve"

/*
 * If your system believes that tabs expand to a width other than
 * 8 then your makefile should cc with -DTABS=whatever, otherwise we use 8.
 */
#ifndef TABS
#define	TABS	8
#endif

/*
 * Maximums
 *
 * The definition of CRSIZE should be the same as BUFSIZ
 * Most other definitions are quite generous.
 */

#define LBSIZE		(LINE_MAX+1)	/* Line buffer size (including NUL) */
#define CRSIZE		BUFSIZ		/* Crypt block size */

#define ESIZE		1024
#define	FNSIZE		MAXPATHLEN	/* Max file name size */
#define	RHSSIZE		512		/* Size of rhs of substitute */
#define	NBRA		9		/* Number of re \( \) pairs */
#define	TAGSIZE		32		/* Tag length */
#define	ONMSZ		128		/* Option name size */
#define	GBSIZE		256		/* Buffer size */
#define	UXBSIZE		128		/* Unix command buffer size */
#define	VBSIZE		128		/* Partial line max size in visual */
/* LBLKS is also defined in expreserve.c */
#ifndef VMUNIX
#define	LBLKS		125		/* Line pointer blocks in temp file */
#define	HBLKS		1		/* struct header fits in BUFSIZ*HBLKS */
#else
#define	LBLKS		900
#define	HBLKS		3
#endif
#define	MAXDIRT		12		/* Max dirtcnt before sync tfile */
#define TCBUFSIZE	1024		/* Max entry size in termcap, see
					   also termlib and termcap */

/*
 * Except on VMUNIX, these are a ridiculously small due to the
 * poor arglist processing implementation which fixes core
 * proportional to them.  Argv (and hence NARGS) is really unnecessary,
 * and argument character space not needed except when
 * arguments exist.  Argument lists should be saved before the "zero"
 * of the incore line information and could then
 * be reasonably large.
 */
#define	NCARGS	5120
#define	NARGS	(NCARGS/6)

/*
 * If you have no terminals which are larger than 24 * 80 you may well 
 * want to make TUBESIZE smaller.  
 */
#define	TUBELINES	107	/* Number of screen lines for visual */
#define	TUBECOLS	220	/* Number of screen columns for visual */
#define	TUBESIZE	23540	/* Maximum screen size for visual */

/*
 * Output column (and line) are set to this value on cursor addressable
 * terminals when we lose track of the cursor to force cursor
 * addressing to occur.
 */
#define	UKCOL		-20	/* Prototype unknown column */

/*
 * Attention is the interrupt character (normally 0177 -- delete).
 * Quit is the quit signal (normally fs -- control-\) and quits open/visual.
 */
#define	ATTN	(-2)
#define	QUIT	('\\' & 037)
