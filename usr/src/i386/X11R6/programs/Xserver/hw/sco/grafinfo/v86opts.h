/*
 *	@(#) v86opts.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	SCO MODIFICATION HISTORY
 *
 * S000, 16-Feb-94, staceyc
 * 	created
 */

#ifndef	V86OPTS_H
#define	V86OPTS_H

/*
 * Option flags
 */
#define	OPT_DEBUG	0x0001	/* enable some debug printfs */
#define	OPT_IOPRINT	0x0002	/* illegal i/o causes printf */
#define	OPT_IOERROR	0x0004	/* illegal i/o causes error */
#define	OPT_LDSYSROM	0x0008	/* load system rom */
#define	OPT_IOTRACE	0x0010	/* print all i/o */
#define	OPT_TICKPRINT	0x0020	/* print v86 pc at clock ticks */
#define OPT_LDPCIROM    0x0040  /* load PCI bios */
#define OPT_INTERP_DEBUG    0x0080 /* debug interpreter, show all instrs */
#define OPT_DISABLE_VM86    0x0100 /* don't use v86 kernel driver */

#endif
