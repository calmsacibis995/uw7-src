#ifndef _IO_TARGET_TAPE_TAPE_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_TAPE_TAPE_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/st01/tape.h	1.7.4.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*	Copyright (c) 1986, 1987  Intel Corporation	*/
/*	All Rights Reserved	*/


/* 
 * Standard tape ioctl commands  
 */

#define T_BASE		('[' << 8)
#define T_RETENSION	(T_BASE | 001) 	/* Retension Tape 		*/
#define T_RWD		(T_BASE | 002)	/* Rewind Tape 			*/
#define T_ERASE		(T_BASE | 003)	/* Erase Tape 			*/
#define T_WRFILEM	(T_BASE | 004)	/* Write Filemarks		*/
#define T_RST		(T_BASE | 005)	/* Reset Tape Drive 		*/
#define T_RDSTAT	(T_BASE | 006)	/* Read Tape Drive Status 	*/
#define T_SFF		(T_BASE | 007)	/* Space Filemarks Forward 	*/
#define T_SBF		(T_BASE | 010)	/* Space Blocks Forward		*/
#define T_LOAD		(T_BASE | 011)	/* Load Tape		 	*/
#define T_UNLOAD	(T_BASE | 012)	/* Unload Tape 			*/
/* The two following ioctls are for Kennedy drives which are now not supported */
#define T_SFREC		(T_BASE | 013)	/* Seek Forward a Record 	*/
#define T_SBREC 	(T_BASE | 014)	/* Seek Backward a Record 	*/
/* The following ioctl is for older controllers which are now not supported */
#define T_TINIT 	(T_BASE | 015)	/* Initialize Tape Interface 	*/

/*	Additional tape ioctls		*/

#define	T_RDBLKLEN	(T_BASE | 016)	/* Read Block Size		*/
#define	T_WRBLKLEN	(T_BASE | 017)	/* Set Block Size		*/
#define	T_PREVMV	(T_BASE | 020)	/* Prevent Media Removal	*/
#define	T_ALLOMV	(T_BASE | 021)	/* Allow Media Removal		*/
#define T_SBB		(T_BASE | 022)	/* Space Blocks Backwards 	*/
#define T_SFB		(T_BASE | 023)	/* Space Filemarks Backwards 	*/
#define T_EOD		(T_BASE | 024)	/* Space to End Of Data		*/
#define T_STS		(T_BASE | 027)	/* Set Tape Speed (1600/6250 bpi etc.)	*/
#define T_STD		(T_BASE | 030)	/* Set Tape Density (QIC-120/150 etc.)	*/
#define	T_SETCOMP	(T_BASE | 031)	/* Set Compression/Decompression*/
#define	T_GETCOMP	(T_BASE | 032)	/* Get Compression status	*/

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_TARGET_TAPE_TAPE_H */
