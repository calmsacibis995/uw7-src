/* @(#)cmd.vxvm:common/gen/volgen.h	1.1 1/24/97 21:11:14 - cmd.vxvm:common/gen/volgen.h */
#ident	"@(#)cmd.vxvm:common/gen/volgen.h	1.1"

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

#ifndef _VOLGEN_H
#define _VOLGEN_H

/*
 * volgen.h - gen usage type specific header
 *
 * This file contains data specific to the gen usage type volume
 * manager utilities.
 */

/* plex states that are used by the FCF VM gen utilities */
#define	GEN_PL_CLEAN	 "CLEAN"   /* plex is up to date, volume was shutdown */
#define	GEN_PL_ACTIVE	 "ACTIVE"  /* plex is active no vol shutdown was done */
#define	GEN_PL_STALE	 "STALE"   /* plex needs to be revived */
#define	GEN_PL_EMPTY	 "EMPTY"   /* plex has no useful data */
#define	GEN_PL_OFFLINE	 "OFFLINE" /* plex is associated but not online */
#define GEN_PL_TEMP	 "TEMP"	   /* disassociate on volume start or stop */
#define GEN_PL_TEMPRM	 "TEMPRM"  /* disassociate and rm on start or stop */
#define GEN_PL_SNAPATT	 "SNAPATT" /* snapshot states */
#define GEN_PL_SNAPDONE	 "SNAPDONE"  
#define GEN_PL_TEMPRMSD	 "TEMPRMSD"	/* disassoc and rm hier on failure */
#define	GEN_PL_SNAPTMP	 "SNAPTMP" /* snapshot in progress, dissoc on start */
#define	GEN_PL_SNAPDIS	 "SNAPDIS" /* snapshot done, dissoc on start */
#define GEN_PL_NODEV	"NODEVICE" /* no disk access record */
#define GEN_PL_REMOVED	"REMOVED"      	/* disk device for plex is removed */
#define GEN_PL_IOFAIL	"IOFAIL"	/* I/O fail caused plex detach */
#define GEN_PL_SYNC	"SYNC"	/* In read-writeback mode */
#define GEN_PL_NEEDSYNC	"NEEDSYNC"	/* Requires read-writeback mode */

/* exit codes specific to the gen usage type */
#define VEX_GEN_BADCNFG	32	/* configuration is inconsistent */
#define VEX_GEN_STATE	33	/* plex state precludes operation */
#define VEX_GEN_GEOM	34	/* bad or non-matching plex geometry */
#define VEX_GEN_SPARSE	35	/* a sparse plex prevents operation */
#define VEX_GEN_BADLOGS	36	/* can't reconcile log areas */

#endif /* _VOLGEN_H */
