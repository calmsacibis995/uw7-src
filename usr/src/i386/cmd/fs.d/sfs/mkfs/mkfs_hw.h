/*	Copyright (c) 1990, 1991, 1992, 1993 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*              copyright       "%c%"   */

#ident	"@(#)sfs.cmds:i386/cmd/fs.d/sfs/mkfs/mkfs_hw.h	1.1"
#ident	"$Header$"

/* Hardware dependent values moved from mkfs.c */

/* The BIG parameter is machine dependent.  It should be a long integer */
/* constant that can be used by the number parser to check the validity */
/* of numeric parameters.  On 16-bit machines, it should probably be    */
/* the maximum unsigned integer, 0177777L.  On 32-bit machines where    */
/* longs are the same size as ints, the maximum signed integer is more  */
/* appropriate.  This value is 017777777777L.                           */

#define BIG     017777777777L

/*
 * The following constants set the defaults used for the number
 * of sectors (fs_nsect), and number of tracks (fs_ntrak).
 *   
 *			NSECT		NTRAK
 *	72MB CDC	18		9
 *	30MB CDC	18		5
 *	720KB Diskette	9		2
 */
#define DFLNSECT	18
#define DFLNTRAK	9

/*
 * If the pagesize is larger than MINBSIZE, the default block size will be
 * the pagesize; otherwise the default will be MINBSIZE.  If the block size 
 * is smaller than NOFRAGBSIZE, the default fragment size will equal the 
 * block size (so there will be no fragments.)  Otherwise the default
 * fragment size will equal DESFRAGSIZE.
 *
 * Both block size and fragment size must be a power of 2 and meet the 
 * following constraints:
 *	MINBSIZE <= block size <= MAXBSIZE
 *	DEV_BSIZE <= fragment size <= block size
 *	block size / fragment size <= 8
 */
#define DESFRAGSIZE	1024
#define NOFRAGBSIZE	2048

/*
 * Cylinder groups may have up to MAXCPG cylinders. The actual
 * number used depends upon how much information can be stored
 * on a single cylinder. The default is to used 16 cylinders
 * per group.
 */
#define	DESCPG		16	/* desired fs_cpg */

/*
 * ROTDELAY gives the minimum number of milliseconds to initiate
 * another disk transfer on the same clider. It is used in
 * determining the rotationally opiml layout for disk blocks
 * within a file; the default of fs_rotdelay is 4ms.
 */
#define ROTDELAY	4

/*
 * MAXCONTIG sets the default for the maximum number of blocks
 * that may be allocated sequentially. Since UNIX drivers are
 * not capable of scheduling multi-block transfers, this defaults
 * to 1 (ie no contiguous blocks are allocated).
 */
#define MAXCONTIG	1

/*
 * Disks are assumed to rotate at 60HZ, unless otherwise specified.
 */
#define	DEFHZ		60

/*
 * NOCOMPAT for architectures with no binary backward compatibility.
 */
#define NOCOMPAT	0
