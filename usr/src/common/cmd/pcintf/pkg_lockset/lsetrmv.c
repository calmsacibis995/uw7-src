#ident	"@(#)pcintf:pkg_lockset/lsetrmv.c	1.1.1.2"
#include <sccs.h>

SCCSID(@(#)lsetrmv.c	3.2);	/* 8/20/91 14:36:54 */

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
   lsetuse.c: Implementation of lock set removal function.

   Exported functions:
	lockSet		*lsRmv();
*/


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include <lockset.h>

#include <internal.h>

/*
 * this is a nil pointer for the semid_ds structure.  by setting it here, we
 */

#define NIL_SEMID_DS	((struct semid_ds FAR *)0)

/* 
   lsRmv: Remove a lock set from the system

   Input Parameters:
	ulong lockSetKey;	Key identifier for the lockset to remove.

   Tasks:
	"Open" underlying lock structure (file or semaphore set)
	Remove it

   Outputs:
	Return Value: LS_SUCCESS (0) or LS_FAIL (-1)
	lsetErr: Holds error code if return value is LS_FAIL
*/

int
lsRmv(lockSetKey)
ulong lockSetKey;
{	int semDesc;

	/* Get a semaphore descriptor for the semaphore set */
	if ((semDesc = semget((key_t)lockSetKey, 0, UP_ORW)) == LS_FAIL) {
		lsetErr = LSERR_SYSTEM;
		return LS_FAIL;
	}

	/* Remove semaphore set from system */
	if (semctl(semDesc, 0, IPC_RMID, NIL_SEMID_DS) != LS_FAIL)
		return LS_SUCCESS;
	lsetErr = LSERR_SYSTEM;
	return LS_FAIL;
}
