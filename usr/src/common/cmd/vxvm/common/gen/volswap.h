/* @(#)cmd.vxvm:common/gen/volswap.h	1.1 1/24/97 21:12:23 - cmd.vxvm:common/gen/volswap.h */
#ident	"@(#)cmd.vxvm:common/gen/volswap.h	1.1"

/*
 * Copyright(C)1996 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 * 
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 * 
 *               RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *               VERITAS SOFTWARE
 * 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043
 */
#ifndef _VOLSWAP_H
#define _VOLSWAP_H

/*
 * volgen.h - swap usage type specific header
 *
 * This file contains data specific to the swap usage type volume
 * manager utilities.
 */

/* plex states that are used by the FCF VM swap utilities */
#define	SWAP_PL_CLEAN	 "CLEAN"   /* plex is up to date, volume was shutdown */
#define	SWAP_PL_ACTIVE	 "ACTIVE"  /* plex is active no vol shutdown was done */
#define	SWAP_PL_STALE	 "STALE"   /* plex needs to be revived */
#define	SWAP_PL_EMPTY	 "EMPTY"   /* plex has no useful data */
#define	SWAP_PL_OFFLINE	 "OFFLINE" /* plex is associated but not online */
#define SWAP_PL_TEMP	 "TEMP"	   /* disassociate on volume start or stop */
#define SWAP_PL_TEMPRM	 "TEMPRM"  /* disassociate and rm on start or stop */
#define SWAP_PL_SNAPATT "SNAPATT" /* remove plex & subdisks on volume start */
#define SWAP_PL_SNAPDONE "SNAPDONE"/* remove plex & subdisks on volume start */
#define SWAP_PL_SNAPTMP "SNAPTMP" /* snapshot dissociate on volume start */
#define SWAP_PL_SNAPDIS "SNAPDIS" /* dissociate on volume start */
#define SWAP_PL_NODEV	"NODEVICE" /* no disk access record */
#define SWAP_PL_REMOVED	"REMOVED"      	/* disk device for plex is removed */
#define SWAP_PL_IOFAIL	"IOFAIL"	/* I/O fail caused plex detach */

/* exit codes specific to the swap usage type */
#define VEX_SWAP_BADCNFG	32	/* configuration is inconsistent */
#define VEX_SWAP_STATE		33	/* plex state precludes operation */
#define VEX_SWAP_GEOM		34	/* bad or non-matching plex geometry */
#define VEX_SWAP_SPARSE		35	/* a sparse plex prevents operation */
#define VEX_SWAP_BADLOGS	36	/* can't reconcile log areas */
#define	VEX_SWAP_RESTRICTED	50	/* operation violates restrictions */

/*
 * Determine if a volume is of the swap usage type
 */
#define VOL_SWAP_UTYPE(v)	(strcmp((v)->v_perm.v_use_type, "swap") == 0)

#endif /* _VOLSWAP_H */

