#ident	"@(#)pcintf:pkg_lockset/lsetset.c	1.1.1.2"
#include <sccs.h>

SCCSID(@(#)lsetset.c	3.2);	/* 8/20/91 14:37:38 */

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
   lsetset.c: Implementation of inter-process lock setting

   Exported functions:
	int		lsSet();
*/


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <stdio.h>

#include <lockset.h>
#include <varargs.h>

#include <internal.h>

/* 
   lsSet: Apply some locks to a lock set

   Input Parameters:
	lockSetT lockSet;		Lockset ID
	int lockOptions:		Special behaviour options
	int lockOp, ...:		List of locks to set
	0				End of lock list

   Tasks:
	(description of what's done)

   Outputs:
	Return Value: LS_SUCCESS (0) or LS_FAIL (-1)
	lsetErr: Error code if return == LS_FAIL
*/

int
/*VARARGS2*/
lsSet(lockSet, lockOptions, va_alist)
lockSetT lockSet;
int lockOptions;
va_dcl
{
r0 lockDataT FAR	*lockP;
r1 va_list		opList;		/* Operation code argument list */
int			lockOp;		/* Lock operation code */
r2 struct sembuf FAR	*opFill;	/* Fill operation list */
r3 int			nSet;		/* Number of locks set */
int			curLock;	/* Current lock number */
int			prevLock = 0;	/* Previous lock number */
short			opFlags = 0;	/* Semaphore operation flags */

	/*
	 * get the internal form of the lockset, check if locks are already
	 * set, in which case, return an error.
	 */

	lockP = (lockDataT FAR *)lockSet;
	if (lockP->nSet != 0) {
		lsetErr = LSERR_TABLE_LOCK;
		return LS_FAIL;
	}

	/* Compute semaphore operation flags from lockOptions */
	if (lockOptions & LS_NOWAIT)
		opFlags |= IPC_NOWAIT;
	if ((lockOptions & LS_CONFIDENT) == 0)
		opFlags |= SEM_UNDO;

	/* Translate each lock operation to a semaphore operation */
	va_start(opList);
	opFill = lockP->semOps;
	for (nSet = 0; lockOp = va_arg(opList, int); nSet++) {
		/* Too many locks? */
		if (nSet >= LS_MAXSETS) {
			va_end(opList);
			lsetErr = LSERR_SPACE;
			return LS_FAIL;
		}

		/* Absolute value of op code is lock number */
		curLock = abs(lockOp);

		/*
		   Check for monotonically increasing lock number
		   and for overflow of the number of locks in set
		*/
		if (curLock <= prevLock || curLock > lockP->nLocks) {
			va_end(opList);
			lsetErr = LSERR_PARAM;
			return LS_FAIL;
		}

		/* Everything OK, construct semaphore operation */
		opFill->sem_num = curLock - 1;
		opFill->sem_op = lockOp < 0 ? -LS_SEMMAX : -1;
		opFill->sem_flg = opFlags;

		/* Get ready for next iteration */
		prevLock = curLock;
		opFill++;
	}
	va_end(opList);

	/* Adjust nSet; Perform semaphore operations to set locks */
	while (semop(lockP->semDesc, lockP->semOps, nSet) == -1) {
		/* If interrupted and LS_IGNORE was given, try again */
		if (errno == EINTR && (lockOptions & LS_IGNORE))
			continue;

		lsetErr = LSERR_SYSTEM;
		return LS_FAIL;
	}

	/* Success!; Record number of locks and return */
	lockP->nSet = nSet;
	return LS_SUCCESS;
}
