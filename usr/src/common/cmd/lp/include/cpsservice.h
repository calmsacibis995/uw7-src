#ident	"@(#)cpsservice.h	1.2"
#ident	"$Header$"

/*
 *    Copyright Novell Inc. 1991
 *    (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *    No part of this file may be duplicated, revised, translated, localized
 *    or modified in any manner or compiled, linked or uploaded or
 *    downloaded to or from any computer system without the prior written
 *    consent of Novell, Inc.
 *
 *
 *  NetWare Unix Client 
 *        Author: Gary B. Tomlinson
 *       Created: 5-3-91
 *
 *       SCCS ID: 1.3
 *         delta: 2/14/92  16:41:39
 *
 *  MODULE:
 *	cpsservice.h	- The NUC PCLIENT independent print service (CPS) 
 *			  service context object.  Member of the NUC
 *			  PCLEINT Services.
 *
 *  ABSTRACT:
 *	The cpsservice.h is included with PCLIENT indepdent Client Print
 *	Services operations to define the qmsService focal object.
 *
 */

#ifndef	_CPS_SERVICE_H
#define	_CPS_SERVICE_H

/*
 * QMS Service Constants
 */
#define	ANY_PSERVER		0xFFFFFFFF
#define	SCHEDULE_ASAP		0xFF
#define	NUCPS_CLIENT_NAME	"NUC: "
#define NUCPS_UNKNOWN_FILE_NAME	"Unknown File Name"

/*
 * NAME
 *	qmsService	- The NUC PCLEINT qmsService focal object.
 *
 * DESCRIPTION
 *	This data structure defines the Client Print Service QMS context 
 *	object, which is the focal object of the CPS layer itself.  This
 *	object defines the context of a service attachment between the
 *	UNIX client process and QMS.
 *
 * MEMBERS
 *	connID		- Internal Name of QMS Server Platform, represented
 *			  as a connection identifier.
 *	queueID		- Internal Name of QMS Queue, represented as a 
 *			  integer object identifier.
 *	pServerID	- Internal Name of Print Server, represented as a
 *			  integer object identifier.  A value of ANY_PSERVER
 *			  specifies any PSERVER that services queueID.
 *	cachedStatuses	- Array of Cached Job Statuses.  The
 *			  NWcpsGetQueueInfo(3) caches the status of all jobs
 *			  on the queue in order to support NEXT_QUEUED_JOB, 
 *			  and NEXT_USER_JOB requests.
 *	numberCached	- Number of elements in cachedStatuses[].
 */
typedef	struct	qmsService {
	NWCONN_HANDLE	connID;
	nuint32			queueID;
	nuint32			pServerID;
	GENERIC_STATUS_T	*cachedStatuses;
	nuint32			numberCached;
} QMS_SERVICE_T;

#endif	/* _CPS_SERVICE_H	*/


