/* @(#)cmd.vxvm:common/raid5/volraid5.h	1.1 1/24/97 21:18:25 - cmd.vxvm:common/raid5/volraid5.h */
#ident	"@(#)cmd.vxvm:common/raid5/volraid5.h	1.1"

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

#ifndef _VOLRAID5_H
#define _VOLRAID5_H

/*
 * volraid5.h - gen usage type specific header
 *
 * This file contains data specific to the raid5 usage type volume
 * manager utilities.
 */

/* plex states that are used by the FCF VM gen utilities */
#define	RAID5_PL_CLEAN	"CLEAN"   /* plex is up to date, volume was shutdown */
#define	RAID5_PL_ACTIVE	"ACTIVE"  /* plex is active no vol shutdown was done */
#define	RAID5_PL_STALE	"STALE"   /* plex needs to be revived */
#define	RAID5_PL_EMPTY	"EMPTY"   /* plex has no useful data */
#define	RAID5_PL_OFFLINE "OFFLINE" /* plex is associated but not online */
#define RAID5_PL_TEMP	 "TEMP"	   /* disassociate on volume start or stop */
#define RAID5_PL_TEMPRM	 "TEMPRM"  /* disassociate and rm on start or stop */
#define RAID5_PL_SNAPATT "SNAPATT" /* snapshot states */
#define RAID5_PL_SNAPDONE "SNAPDONE"  
#define RAID5_PL_TEMPRMSD "TEMPRMSD"	/* disassoc and rm hier on failure */
#define	RAID5_PL_SNAPTMP "SNAPTMP" /* snapshot in progress, dissoc on start */
#define	RAID5_PL_SNAPDIS "SNAPDIS" /* snapshot done, dissoc on start */
#define RAID5_PL_NODEV	"NODEVICE" /* no disk access record */
#define RAID5_PL_REMOVED	"REMOVED"      	/* disk device for plex is removed */
#define RAID5_PL_IOFAIL	"IOFAIL"	/* I/O fail caused plex detach */
#define RAID5_PL_SYNC	"SYNC"	/* In read-writeback mode */
#define RAID5_PL_NEEDSYNC "NEEDSYNC"	/* Requires read-writeback mode */
#define	RAID5_PL_LOG "LOG"		/* Plex is valid raid5 log */
#define	RAID5_PL_BADLOG	"BADLOG"	/* Plex is invalid raid5 log */

#define	RAID5_VOL_REPLAY "REPLAY"	/* Replaying of logs in progress */

/* exit codes specific to the raid5 usage type */
#define VEX_RAID5_BADCNFG	32	/* configuration is inconsistent */
#define VEX_RAID5_STATE		33	/* plex state precludes operation */
#define VEX_RAID5_GEOM		34	/* bad or non-matching plex geometry */
#define VEX_RAID5_SPARSE	35	/* a sparse plex prevents operation */
#define VEX_RAID5_BADLOGS	36	/* can't reconcile log areas */
#define VEX_RAID5_BADIDEA	37	/* raid5 version of -f required */

#endif	/* _VOLRAID5_H */
