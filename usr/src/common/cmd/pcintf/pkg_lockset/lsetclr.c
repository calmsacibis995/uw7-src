#ident	"@(#)pcintf:pkg_lockset/lsetclr.c	1.1.1.2"
#include <sccs.h>

SCCSID(@(#)lsetclr.c	3.2);	/* 8/20/91 14:35:14 */

/*
   (c) Copyright 1985 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.	 No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/


/*
   lsetclr.c: Implementation of inter-process lock clearing

   Exported functions:
	int		lsClr();
*/


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include <lockset.h>

#include <internal.h>

/* 
   lsClr: Remove locks previously applied to a lock set

   Input Parameters:
	lockSetT lockSet:	Lockset ID

   Tasks:
	(description of what's done)

   Outputs:
	Return Value: LS_SUCCESS (0) or LS_FAIL (-1)
	rLockErr: Holds error code if return value == LS_FAIL
*/

int
lsClr(lockSet)
lockSetT lockSet;
{	r0 lockDataT FAR *lockP;
	r1 struct sembuf FAR *opScan;
	r2 int nSet;

	/*
	 * get the internal format of the lockset data, return an error if
	 * no locks are set.
	 */

	lockP = (lockDataT FAR *)lockSet;
	if ((nSet = lockP->nSet) == 0) {
		lsetErr = LSERR_TABLE_LOCK;
		return LS_FAIL;
	}

	/* Translate list of lock setting semaphore ops to their inverses */
	opScan = lockP->semOps;
	for (nSet = lockP->nSet; nSet-- > 0; opScan++)
		/* Negate semaphore op code to invert locking action */
		opScan->sem_op = -opScan->sem_op;

	/* Remember and clear count of locks set */
	nSet = lockP->nSet;
	lockP->nSet = 0;

	/* Release locks by incrementing semaphores */
	if (semop(lockP->semDesc, lockP->semOps, nSet) != LS_SUCCESS) {
		lsetErr = LSERR_SYSTEM;
		return LS_FAIL;
	}

	/* Success */
	return LS_SUCCESS;
}
