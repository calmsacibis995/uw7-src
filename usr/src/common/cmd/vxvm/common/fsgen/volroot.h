/* @(#)cmd.vxvm:common/fsgen/volroot.h	1.1 1/24/97 21:08:35 - cmd.vxvm:common/fsgen/volroot.h */
#ident	"@(#)cmd.vxvm:common/fsgen/volroot.h	1.1"

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
#ifndef _VOLROOT_H
#define _VOLROOT_H

/*
 * volroot.h - root usage type specific header
 *
 * This file contains data specific to the root usage type volume
 * manager utilities.
 */

/* plex states that are used by the VxVM root usage type utilities */
#define	ROOT_PL_CLEAN	 "CLEAN"   /* plex is up to date, volume was shutdown */
#define	ROOT_PL_ACTIVE	 "ACTIVE"  /* plex is active no vol shutdown was done */
#define	ROOT_PL_STALE	 "STALE"   /* plex needs to be revived */
#define	ROOT_PL_EMPTY	 "EMPTY"   /* plex has no useful data */
#define	ROOT_PL_OFFLINE "OFFLINE" /* plex is associated but not online */
#define ROOT_PL_TEMP	 "TEMP"	   /* disassociate on volume start or stop */
#define ROOT_PL_TEMPRM	 "TEMPRM"  /* disassociate and rm on start or stop */
#define ROOT_PL_SNAPATT "SNAPATT" /* remove plex & subdisks on volume start */
#define ROOT_PL_SNAPDONE "SNAPDONE"/* remove plex & subdisks on volume start */
#define ROOT_PL_SNAPTMP "SNAPTMP" /* snapshot dissociate on volume start */
#define ROOT_PL_SNAPDIS "SNAPDIS" /* dissociate on volume start */
#define ROOT_PL_NODEV	"NODEVICE" /* no disk access record */
#define ROOT_PL_REMOVED	"REMOVED"      	/* disk device for plex is removed */
#define ROOT_PL_IOFAIL	"IOFAIL"	/* I/O fail caused plex detach */


/* exit codes specific to the root usage type */
#define VEX_ROOT_BADCNFG	32	/* configuration is inconsistent */
#define VEX_ROOT_STATE		33	/* plex state precludes operation */
#define VEX_ROOT_GEOM		34	/* bad or non-matching plex geometry */
#define VEX_ROOT_SPARSE		35	/* a sparse plex prevents operation */
#define VEX_ROOT_BADLOGS	36	/* can't reconcile log areas */
#define VEX_ROOT_NO_FSTYPE	40	/* no filesystem type for volume */
#define VEX_ROOT_BAD_FSTYPE	41	/* no such filesystem type */
#define	VEX_ROOT_RESTRICTED	50	/* operation violates restrictions */

/*
 * Determine if a volume is of the root usage type
 */
#define VOL_ROOT_UTYPE(v)	(strcmp((v)->v_perm.v_use_type, "root") == 0)

#endif /* _VOLROOT_H */
