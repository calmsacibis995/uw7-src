#ifndef _IO_TARGET_ALTSCTR_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_ALTSCTR_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/altsctr.h	1.5"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifdef _KERNEL_HEADERS

#include <io/vtoc.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/vtoc.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * 	alternate sector partition definitions
 */

/*	alternate sector partition information table			*/
struct	alts_parttbl {
	long	alts_sanity;	/* to validate correctness		*/
	long  	alts_version;	/* version number			*/
	daddr_t	alts_map_base;	/* disk offset of alts_partmap		*/
	long	alts_map_len;	/* byte length of alts_partmap		*/
	daddr_t	alts_ent_base;	/* disk offset of alts_entry		*/
	long	alts_ent_used;	/* number of alternate entries used	*/
	long	alts_ent_end;	/* disk offset of top of alts_entry	*/
	daddr_t	alts_resv_base;	/* disk offset of alts_reserved		*/
	long 	alts_pad[5];	/* reserved fields			*/
};

/*	alternate sector remap entry table				*/
struct	alts_ent {
	daddr_t	bad_start;	/* starting bad sector number		*/
	daddr_t	bad_end;	/* ending bad sector number		*/
	daddr_t	good_start;	/* starting alternate sector to use	*/
};

/*	size of alternate partition table structure			*/
#define	ALTS_PARTTBL_SIZE	sizeof(struct alts_parttbl) 
/*	size of alternate entry table structure				*/
#define	ALTS_ENT_SIZE	sizeof(struct alts_ent) 

/*	definition for alternate sector partition sector map		*/
#define	ALTS_GOOD	0	/* good alternate sectors		*/
#define	ALTS_BAD	1	/* bad alternate sectors		*/

/*	definition for alternate sector partition id			*/
#define	ALTS_SANITY	0xaabbccdd /* magic number to validate alts_part*/
#define ALTS_VERSION1	0x01	/* version of alts_parttbl		*/

#define ALTS_ENT_EMPTY	-1	/* empty alternate entry		*/
#define ALTS_MAP_UP	1	/* search forward with increasing sect# */
#define ALTS_MAP_DOWN	-1	/* search backward with decreasing sect#*/

#define ALTS_ADDPART	0x1	/* add alternate partition		*/
struct	alts_mempart {			/* incore alts partition info	*/
	ushort_t	ap_flag;	/* flag for alternate partition	*/
	ushort_t	ap_rcnt;	/* alts table reference count	*/
	struct	alts_parttbl *ap_tblp;	/* alts partition table		*/
	int	ap_tbl_secsiz;		/* alts parttbl sector size	*/
	uchar_t	*ap_memmapp;		/* incore alternate sector map	*/
	uchar_t	*ap_mapp;		/* alternate sector map		*/
	int	ap_map_secsiz;		/* alts partmap sector size	*/
	int	ap_map_sectot;		/* alts partmap # sector 	*/
	struct  alts_ent *ap_entp;	/* alternate sector entry table */
	int	ap_ent_secsiz;		/* alts entry sector size	*/
	struct	alts_ent *ap_gbadp;	/* growing badsec entry table	*/
	int	ap_gbadcnt;		/* growing bad sector count	*/
	struct	partition part;		/* alts partition configuration */
} ;
/*	ap_flag definitions						*/
#define	ALT_OBSOLETE	0x0001		/* Alternate table is obsolete	*/

/*	size of incore alternate partition memory structure		*/
#define	ALTS_MEMPART_SIZE	sizeof(struct alts_mempart) 

struct	altsectbl {			/* working alts info		*/
	struct  alts_ent *ast_entp;	/* alternate sector entry table */
	int	ast_entused;		/* entry used			*/
	struct	alt_info *ast_alttblp;	/* alts info			*/
	int	ast_altsiz;		/* size of alts info		*/
	struct  alts_ent *ast_gbadp;	/* growing bad sector entry ptr */
	int	ast_gbadcnt;		/* growing bad sector entry cnt */
};
/*	size of incore alternate partition memory structure		*/
#define	ALTSECTBL_SIZE	sizeof(struct altsectbl) 

/*	macro definitions						*/
#define	byte_to_secsiz(APSIZE, DPBSP)	(daddr_t) \
					((((APSIZE) + (DPBSP)->dp_secsiz - 1) \
					 / (uint)(DPBSP)->dp_secsiz) \
					 * (DPBSP)->dp_secsiz)


#define	byte_to_dsksec(APSIZE, DPBPTR)	(daddr_t) \
					((((APSIZE) +(DPBPTR)->dpb_secsiz - 1) \
					 / (DPBPTR)->dpb_secsiz) \
					 * (DPBPTR)->dpb_secsiz)

#define	byte_to_blksz(APSIZE, DSKSECSZ)	(daddr_t) \
					((((APSIZE) +(DSKSECSZ) -1) \
					 / (uint)(DSKSECSZ)) * (DSKSECSZ))

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_TARGET_ALTSCTR_H */
