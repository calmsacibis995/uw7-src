#ident	"@(#)pcintf:pkg_lockset/lsetnew.c	1.1.1.2"
#include <sccs.h>

SCCSID(@(#)lsetnew.c	3.2);	/* 8/20/91 14:35:56 */

/*
   (c) Copyright 1985 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.  No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/


/*
   lsetnew.c: Implementation of lock set creation function

   Exported functions:
	lockSet		*lsNew();
*/


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include <lockset.h>

#include <internal.h>

extern char FAR *malloc();

/* 
   lsNew: Create a new lock set

   Input Parameters:
	lockSetKey:		Key identifier for lock set
	permission:		Requested permissions
	minLocks:		Minimum acceptable number of locks in set
	maxLocks:		Maximum useful number of locks in set

   Tasks:
	- Create and initialize underlying lock structure
	  (file or semaphore set)
	- Initialize lockSet structure and return pointer to caller

   Outputs:
	Return Value: lockset ID for the new lock set
	lsetErr: Holds error code if return value is INVALID_LOCKSET
*/

lockSetT
lsNew(lockSetKey, permission, minLocks, maxLocks)
ulong			lockSetKey;
int			permission;
int			minLocks;
int			maxLocks;
{
key_t			semKey;		/* Semaphore set identifier */
int			semPerm;	/* Semaphore set permission code */
int			semDesc;	/* Semaphore set descriptor */
int			nLocks;		/* Number of locks in set */
int			initSem;	/* Initialize semaphores */
lockDataT FAR		*lockP;		/* Newly allocated lockSet */

	/* Validate arguments */
	if (minLocks <= 0 ||
	    maxLocks <= 0 ||
	    minLocks > maxLocks ||
	    lockSetKey == IPC_PRIVATE)
	{
		lsetErr = LSERR_PARAM;
		return INVALID_LOCKSET;
	}

	/*
	 * compute the semaphore identifier and permission code.
	 */

	semPerm = IPC_CREAT | IPC_EXCL;
	if (ANYSET(permission, LS_PRIVATE)) {
#ifdef	TBD
		/*
		 * check for conflicting permission bits
		 */

		if ((permission ^ LS_PRIVATE) != 0)
		{
			lsetErr = LSERR_PARAM;
			return INVALID_LOCKSET;
		}
#endif
		semKey = IPC_PRIVATE;
	} else {
		semKey	= (key_t)lockSetKey;

		if (ANYSET(permission, LS_O_WRITE)) semPerm |= UP_OW;
		if (ANYSET(permission, LS_G_WRITE)) semPerm |= UP_GW;
		if (ANYSET(permission, LS_W_WRITE)) semPerm |= UP_WW;
		if (ANYSET(permission, LS_O_READ)) semPerm |= UP_OR;
		if (ANYSET(permission, LS_G_READ)) semPerm |= UP_GR;
		if (ANYSET(permission, LS_W_READ)) semPerm |= UP_WR;
	}

	/* Try to get as many of maxLocks semaphores as available */
	for (nLocks = maxLocks; nLocks >= minLocks; nLocks--) {
		/* Attempt to create a new semaphore set */
		semDesc = semget(semKey, nLocks, semPerm);

		/* Break from loop on success */
		if (semDesc != LS_FAIL)
			break;

		/* If semaphore set exists, return INVALID_LOCKSET */
		if (errno == EEXIST || errno == EINVAL)
		{
			lsetErr = LSERR_EXIST;
			return INVALID_LOCKSET;
		}
	}

	/* Check to see if semaphore set was successfully created */
	if (semDesc == LS_FAIL) {
		if (errno == EACCES)
			lsetErr = LSERR_PERM;
		else
			lsetErr = LSERR_SPACE;
		return INVALID_LOCKSET;
	}

	/* Initialize all semaphores to maximum value */
	for (initSem = 0; initSem < nLocks; initSem++)
		if (semctl(semDesc, initSem, SETVAL, LS_SEMMAX) == LS_FAIL) {
			/* "Can't happen" */
			/* Remove semaphore set and return INVALID_LOCKSET */
			(void)semctl(semDesc, 0, IPC_RMID, 0);
			lsetErr = LSERR_SYSTEM;
			return INVALID_LOCKSET;
		}

	/* Allocate and initialize new lock set descriptor */
	lockP = (lockDataT FAR *)malloc(sizeof(lockDataT));
	if (nil(lockP))
	{
		lsetErr = LSERR_SPACE;
		return INVALID_LOCKSET;
	}
	lockP->semDesc = semDesc;
	lockP->nLocks = nLocks;
	lockP->nSet = 0;

	/* Return pointer to new lock set descriptor */
	return (lockSetT)lockP;
}
