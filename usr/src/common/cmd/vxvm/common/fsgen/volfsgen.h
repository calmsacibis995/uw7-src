/* @(#)cmd.vxvm:common/fsgen/volfsgen.h	1.1 1/24/97 21:08:24 - cmd.vxvm:common/fsgen/volfsgen.h */
#ident	"@(#)cmd.vxvm:common/fsgen/volfsgen.h	1.1"

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

#ifndef _VOLFSGEN_H
#define _VOLFSGEN_H

/*
 * volfsgen.h - fsgen usage type specific header
 *
 * This file contains data specific to the fsgen usage type volume
 * manager utilities.
 */

/* plex states that are used by the FCF VM fsgen utilities */
#define	FSGEN_PL_CLEAN	 "CLEAN"   /* plex is up to date, volume was shutdown */
#define	FSGEN_PL_ACTIVE	 "ACTIVE"  /* plex is active no vol shutdown was done */
#define	FSGEN_PL_STALE	 "STALE"   /* plex needs to be revived */
#define	FSGEN_PL_EMPTY	 "EMPTY"   /* plex has no useful data */
#define	FSGEN_PL_OFFLINE "OFFLINE" /* plex is associated but not online */
#define FSGEN_PL_TEMP	 "TEMP"	   /* disassociate on volume start or stop */
#define FSGEN_PL_TEMPRM	 "TEMPRM"  /* disassociate and rm on start or stop */
#define FSGEN_PL_SNAPATT	 "SNAPATT"
#define FSGEN_PL_SNAPDONE	 "SNAPDONE"
#define FSGEN_PL_TEMPRMSD	 "TEMPRMSD"
#define FSGEN_PL_SNAPTMP "SNAPTMP" /* snapshot progress, dis on vol start */
#define FSGEN_PL_SNAPDIS "SNAPDIS" /* snapshot done, dis on vol start */
#define FSGEN_PL_NODEV	"NODEVICE" /* no disk access record */
#define FSGEN_PL_REMOVED "REMOVED"     	/* disk device for plex is removed */
#define FSGEN_PL_IOFAIL	"IOFAIL"	/* I/O fail caused plex detach */

/* exit codes specific to the fsgen usage type */
#define VEX_FSGEN_BADCNFG	32	/* configuration is inconsistent */
#define VEX_FSGEN_STATE		33	/* plex state precludes operation */
#define VEX_FSGEN_GEOM		34	/* bad or non-matching plex geometry */
#define VEX_FSGEN_SPARSE	35	/* a sparse plex prevents operation */
#define VEX_FSGEN_BADLOGS	36	/* can't reconcile log areas */
#define VEX_FSGEN_NO_FSTYPE	40	/* no filesystem type for volume */
#define VEX_FSGEN_BAD_FSTYPE	41	/* no such filesystem type */

#endif /* _VOLFSGEN_H */
