#ident	"@(#)pcintf:pkg_rlock/shm_lock.c	1.1.1.2"
#include <sccs.h>

SCCSID(@(#)shm_lock.c	3.4);	/* 10/10/91 17:22:32 */

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

/*--------------------------------------------------------------------------
 *
 *	shm_lock.c - record locking operations
 *
 *	routines included:
 *		addLock()
 *		rmvLock()
 *		ioStart()
 *		ioDone()
 *		rstLocks()
 *
 *	comments:
 *
 *--------------------------------------------------------------------------
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

#ifdef	AIX_RT
#include <values.h>
#define	LONG_MAX MAXLONG
#else	/* ! AIX_RT */
#include <limits.h>
#endif	/* AIX_RT */

#include <errno.h>

#include <lockset.h>

#include <rlock.h>

#include <internal.h>

/*
 * since the ioStart() and ioDone() function pair locks out a specific
 * record lock for the duration of the period between the two calls, only one
 * process may do the I/O.  to relieve this, it is possible to create an
 * array of available locks, and to hash the file's unique ID to index one
 * of these.  this reduces the possiblity of conflict, up to the limit
 * defined by the system.  this macro handles that hashing function.
 */

#define RL_NUM(uniqID)	(RL_LOCK0 + UNIQ_INT_HASH(uniqID, _rlNHashLocks))

/*
 * internal functions
 */

static recLockT FAR	*newLock();
static void		freeLock();
static bool		hasLocks();
static bool		doHostLock();

/* 
 *	addLock() - add a lock to an open file
 *
 *	input:	openEntry - index into the global open file table
 *		dosPID - process id of the DOS process setting the lock
 *		lockLow - first byte to lock
 *		lockCount - number of bytes to lock
 *
 *	proc:	add the lock.  we need to verify that the file is open, and
 *		that the requested lock doesn't overlap an existing one.
 *
 *	output:	(int) - RL_SUCCESS (0) or RL_FAIL (-1)
 *
 *	global:	_rlockShm - checked
 */

int
addLock(openEntry, dosPID, lockLow, lockCount)
int openEntry;
long dosPID;
r3 unsigned long lockLow;
unsigned long lockCount;
{	r0 recLockT FAR *scanLockP;
	r1 recLockT FAR *newLockP;
	r2 recLockT FAR *nextLockP;
	r4 unsigned long lockHi;
	openFileT FAR *openFileP;
	fileHdrT FAR *fileHdrP;
	indexT lockIndex, newIndex;
	int saveRlockErr;

	/*
	 * if this is old style memory, we need to use a different function.
	 * _rlAddLock() will set the rlockErr value.
	 */

	if (_rlockShm.useOldStyle)
		return _rlAddLock(openEntry, (short)dosPID,
					(long)lockLow, lockCount);

	/*
	 * validate the lock range, and convert the byte count into a high
	 * lock location.
	 */

	if (lockCount == 0)
	{
		rlockErr = RLERR_PARAM;
		return RL_FAIL;
	}
	lockHi = lockLow + lockCount - 1;

	/*
	 * get a pointer to the (currently open) file which will have the
	 * locks set, and the pointer to the corresponding file header
	 * structure.  _rlGetOFile() will set the rlockErr value.
	 */

	openFileP = _rlGetOFile(openEntry);
	if (nil(openFileP))
		return RL_FAIL;
	fileHdrP = &_rlockShm.fhdrTableP[openFileP->ofFHdrIndex];

	/*
	 * now the greedy part: exclude other programs from accessing the
	 * lock table.  _rlSetLocks() will set the rlockErr value.
	 */

	if (!_rlSetLocks(-LT_LOCK, -RL_NUM(fileHdrP->fhUniqueID)))
		return RL_FAIL;

	/*
	 * get and initialize a new record lock structure.  newLock() will set
	 * rlockErr on failure.  since _rlUnlock() may also set it, the
	 * error must be saved.
	 */

	newLockP = newLock((indexT)openEntry, lockLow, lockHi,
				openFileP->ofSessID, dosPID);
	if (nil(newLockP))
	{
		saveRlockErr = rlockErr;
		_rlUnlock();
		rlockErr = saveRlockErr;
		return RL_FAIL;
	}
	newIndex = newLockP - _rlockShm.rlockTableP;

	/*
	 * first existing lock, we just need to add the lock to the head of
	 * the lock list (signified by a NIL value for scanLockP).  if we
	 * can't get off that easy, we need to loop to look for the proper
	 * location in the list, and to verify that we don't overlap an
	 * existing lock.
	 */

	lockIndex = fileHdrP->fhLockIndex;
	if (lockIndex == INVALID_INDEX)
		scanLockP = NIL_RECLOCK;
	else
	{
		for (scanLockP = &_rlockShm.rlockTableP[lockIndex];
		     !nil(scanLockP);
		     scanLockP = nextLockP)
		{
			/*
			 * convert the next index into a pointer to the
			 * next data.
			 */

			if (scanLockP->rlNextIndex == INVALID_INDEX)
			    nextLockP = NIL_RECLOCK;
			else
			    nextLockP =
				&_rlockShm.rlockTableP[scanLockP->rlNextIndex];

			/*
			 * if the requested lock overlaps one that already
			 * exists, fail.
			 */

			if ((lockLow >= scanLockP->rlLockLow &&
			     lockLow <= scanLockP->rlLockHi) ||
			    (lockHi >= scanLockP->rlLockLow &&
			     lockHi <= scanLockP->rlLockHi) ||
			    (lockLow <= scanLockP->rlLockLow &&
			     lockHi >= scanLockP->rlLockHi))
			{
				freeLock(openFileP, newLockP);
				_rlUnlock();
				rlockErr = RLERR_INUSE;
				return RL_FAIL;
			}

			/*
			 * we don't overlap, so check if the lock can go
			 * here.  if the new lock begins after the one we are
			 * currently indicating via scanLockP, and either
			 * there is no next lock, or the next lock's region
			 * is after the new one, we can add it here, break
			 * from the loop.
			 */

			if (lockLow > scanLockP->rlLockHi &&
			    (nil(nextLockP) || lockHi < nextLockP->rlLockLow))
			{
				break;
			}
		}
	}

	/*
	 * if scanLockP is NIL, the new lock will be entered at the head of
	 * the lock list.  otherwise, scanLock is the entry we currently are
	 * looking at in the list.  if nextLock is NIL, then scanLock is at
	 * the end of the list, so the new entry goes at the end of the
	 * list.  finally, if neither is NIL, the new entry goes between them.
	 */

	if (nil(scanLockP))
	{
		newLockP->rlNextIndex = fileHdrP->fhLockIndex;
		fileHdrP->fhLockIndex = newIndex;
	}
	else
	{
		if (nil(nextLockP))
			newLockP->rlNextIndex = INVALID_INDEX;
		else
			newLockP->rlNextIndex =
				nextLockP - _rlockShm.rlockTableP;
		scanLockP->rlNextIndex = newIndex;
	}

	/*
	 * we are done with the table, so release it.  then record the state
	 * change, and return.
	 */

	_rlUnlock();
	rlockState(openEntry, RLSTATE_LOCKED);
	return RL_SUCCESS;
}

/*
 *	rmvLock() - remove a lock from a file
 *
 *	input:	openEntry - index into the global open file table
 *		dosPID - process id of the DOS process setting the lock
 *		lockLow - first byte to lock
 *		lockCount - number of bytes to lock
 *
 *	proc:	find the specified lock in the list and remove it.
 *
 *	output:	(int) - RL_SUCCESS (0) or RL_FAIL (-1)
 *
 *	global:	_rlockShm - checked
 */

int
rmvLock(openEntry, dosPID, lockLow, lockCount)
int openEntry;
long dosPID;
r1 unsigned long lockLow;
unsigned long lockCount;
{	r0 recLockT FAR	*scanLockP;
	r2 unsigned long lockHi;
	openFileT FAR *openFileP;
	fileHdrT FAR *fileHdrP;
	recLockT FAR *prevLockP;
	long sessID;
	bool isLocked;
	indexT rlockIndex;

	/*
	 * if this is old style memory, we need to use a different function.
	 * _rlRmvLock() will set the rlockErr value.
	 */

	if (_rlockShm.useOldStyle)
		return _rlRmvLock(openEntry, (short)dosPID,
					(long)lockLow, lockCount);

	/*
	 * get a pointer to the open file, plus a pointer to the associated
	 * file header, and a local version of the session ID of the open
	 * file's owner.  while we're here, compute the high byte of the
	 * lock range.  _rlGetOFile() will set the rlockErr value.
	 */

	openFileP = _rlGetOFile(openEntry);
	if (nil(openFileP))
		return RL_FAIL;
	fileHdrP = &_rlockShm.fhdrTableP[openFileP->ofFHdrIndex];
	sessID = openFileP->ofSessID;
	lockHi = lockLow + lockCount - 1;

	/*
	 * now, gain exclusive access to the lock table.  _rlSetLocks() will set
	 * the rlockErr value.
	 */

	if (!_rlSetLocks(-LT_LOCK, -RL_NUM(fileHdrP->fhUniqueID)))
		return RL_FAIL;

	/*
	 * search the table for the lock to remove.  if this loop terminates
	 * 'normally', we didn't find the lock.
	 */

	prevLockP = NIL_RECLOCK;
	for (rlockIndex = fileHdrP->fhLockIndex;
	     rlockIndex != INVALID_INDEX;
	     rlockIndex = scanLockP->rlNextIndex)
	{
		/*
		 * convert the index into a table pointer.
		 */

		scanLockP = &_rlockShm.rlockTableP[rlockIndex];

		/*
		 * does this lock match the one requested?
		 */

		if (scanLockP->rlLockLow == lockLow &&
		    scanLockP->rlLockHi == lockHi)
		{
			/*
			 * was it applied by the caller?  if not, fail.
			 */

			if (scanLockP->rlDosPID != dosPID || 
			    scanLockP->rlSessID != sessID)
			{
				break;
			}

			/*
			 * the lock matched, and was applied by the caller,
			 * so remove it from the lock list, then free the
			 * table entry.
			 */

			if (nil(prevLockP))
				fileHdrP->fhLockIndex =
						scanLockP->rlNextIndex;
			else
				prevLockP->rlNextIndex =
						scanLockP->rlNextIndex;
			freeLock(openFileP, scanLockP);

			/*
			 * determine if this is the last lock, then release
			 * the table, record the non-locked condition of the
			 * file is appropriate, and return.  the recording
			 * is done after releasing the table, so we don't
			 * tie up the shared memory.
			 */

			isLocked = hasLocks(fileHdrP, (indexT)openEntry,
						sessID, dosPID);
			_rlUnlock();
			if (!isLocked)
				rlockState(openEntry, RLSTATE_ALL_UNLOCKED);
			return RL_SUCCESS;
		}

		/*
		 * if the lock overlaps an existing one, fail.
		 */

		if ((lockLow >= scanLockP->rlLockLow &&
		     lockLow <= scanLockP->rlLockHi) ||
		    (lockHi >= scanLockP->rlLockLow &&
		     lockHi <= scanLockP->rlLockHi))
		{
			break;
		}

		/*
		 * yet another uneventful loop.  save a pointer to the current
		 * entry, so we can use it as the previous entry on the next
		 * iteration.
		 */

		prevLockP = scanLockP;
	}

	/*
	 * the loop terminated 'normally', so the requested lock wasn't found,
	 * and there were no other abnormallities.  return the appropriate
	 * failure.
	 */

	_rlUnlock();
	rlockErr = RLERR_NOUNLOCK;
	return RL_FAIL;
}

/*
 *	ioStart() - handle the record locking checks
 *
 *	input:	openEntry - index into the global open file table
 *		dosPID - process id of the DOS process setting the lock
 *		lockLow - first byte to check
 *		lockCount - number of bytes to check
 *
 *	proc:	check that the requested range within the file hasn't been
 *		locked.  if it is, the function will fail.
 *
 *	output:	(int) - RL_SUCCESS (0) or RL_FAIL (-1)
 *
 *	global:	_rlockShm - checked
 */

int
ioStart(openEntry, dosPID, lockLow, lockCount)
int openEntry;
long dosPID;
r1 unsigned long lockLow;
unsigned long lockCount;
{	r0 recLockT FAR *scanLockP;
	r2 unsigned long ioHi;
	openFileT FAR *openFileP;
	fileHdrT FAR *fileHdrP;
	long sessID;
	indexT rlockIndex;

	/*
	 * if this is old style memory, we need to use a different function.
	 * _rlIOStart() will set the rlockErr value.
	 */

	if (_rlockShm.useOldStyle)
		return _rlIOStart(openEntry, (short)dosPID,
					(long)lockLow, (long)lockCount);

	/*
	 * get a pointer to the open file, and check if the file is in DOS
	 * compatability mode.  if that's the case, we don't need to deal
	 * with record locking, so just return.  _rlGetOFile() will set the
	 * rlockErr value.
	 */

	openFileP = _rlGetOFile(openEntry);
	if (nil(openFileP))
		return RL_FAIL;
	if (openFileP->ofDenyMode == OFD_DOS_COMPAT)
		return RL_SUCCESS;

	/*
	 * we have to deal with the locks.  get a pointer to the associated
	 * file header for this open file, and the session ID.  might as well
	 * get the high byte to lock while we're here.
	 */

	fileHdrP = &_rlockShm.fhdrTableP[openFileP->ofFHdrIndex];
	sessID = openFileP->ofSessID;
	ioHi = lockLow + lockCount - 1;

	/*
	 * now, gain exclusive access to the lock table.  _rlSetLocks() will set
	 * the rlockErr value.
	 */

	if (!_rlSetLocks(RL_NUM(fileHdrP->fhUniqueID), UNUSED_LOCK))
		return RL_FAIL;

	/*
	 * see if any locks overlap the requested I/O range.
	 */

	for (rlockIndex = fileHdrP->fhLockIndex;
	     rlockIndex != INVALID_INDEX;
	     rlockIndex = scanLockP->rlNextIndex)
	{
		/*
		 * convert the index into a pointer.
		 */

		scanLockP = &_rlockShm.rlockTableP[rlockIndex];

		/*
		 * if this lock and the I/O request limits don't overlap, we
		 * can continue the loop, there isn't a conflict.  likewise, if
		 * the lock was set by the caller, there isn't a conflict,
		 * so just continue the loop.
		 */

		if (ioHi < scanLockP->rlLockLow || lockLow > scanLockP->rlLockHi)
			continue;
		if (scanLockP->rlDosPID == dosPID &&
		    scanLockP->rlSessID == sessID)
			continue;

		/*
		 * if we got to this point, there is a conflicting lock.  tell
		 * the user of his misfortune.
		 */

		_rlUnlock();
		rlockErr = RLERR_INUSE;
		return RL_FAIL;
	}

	/*
	 * we didn't have any locking conflicts, so the user can access the
	 * memory.  in order to ensure that we don't get a lock applied in
	 * the middle of the write (which may take a while), leave the lock on
	 * the record lock table.  note that this means that the user is
	 * responsible for invoking ioDone() afterwards.
	 */

	return RL_SUCCESS;
}

/*
 *	ioDone() - release the locking table lock set by ioStart()
 *
 *	input:	(none)
 *
 *	proc:	simply release the lock.  not much else to do.
 *
 *	output:	(void) - none
 *
 *	global:	_rlockShm - checked
 */

void
ioDone()
{
	/*
	 * if this is old style memory, we need to use a different function.
	 * _rlIODone() will set the rlockErr value.
	 */

	if (_rlockShm.useOldStyle)
	{
		_rlIODone();
		return;	
	}

	/*
	 * all we have to do is to release the record lock table.  note that
	 * if we never did the lock, due to being in DOS compatability mode
	 * (see ioStart()), _rlUnlock() will still work correctly.
	 */

	_rlUnlock();
}

/*
 *	rstLocks() - release all of a single user's file locks
 *
 *	input:	openEntry - index into the global open file table
 *		dosPID - process id of the DOS process setting the lock
 *
 *	proc:	scan thorough the lock list for the specified file, and
 *		release all locks on that file that were set by the specified
 *		PID.
 *
 *	output:	(int) - RL_SUCCESS (0) or RL_FAIL (-1)
 *
 *	global:	_rlockShm - checked
 */

int
rstLocks(openEntry, dosPID)
int openEntry;
long dosPID;
{	r0 recLockT FAR	*scanLockP;
	r1 recLockT FAR	*prevLockP;
	r2 recLockT FAR	*nextLockP;
	openFileT FAR *openFileP;
	fileHdrT FAR *fileHdrP;
	long sessID;

	/*
	 * if this is old style memory, we need to use a different function.
	 * _rlRstLocks() will set the rlockErr value.
	 */

	if (_rlockShm.useOldStyle)
		return _rlRstLocks(openEntry, (short)dosPID);

	/*
	 * get a pointer to the open file, plus a pointer to the associated
	 * file header, and a local version of the session ID of the open
	 * file's owner.  _rlGetOFile() will set the rlockErr value.
	 */

	openFileP = _rlGetOFile(openEntry);
	if (nil(openFileP))
		return RL_FAIL;
	fileHdrP = &_rlockShm.fhdrTableP[openFileP->ofFHdrIndex];
	sessID = openFileP->ofSessID;

	/*
	 * if there are no locks set, return now.
	 */

	if (fileHdrP->fhLockIndex == INVALID_INDEX)
		return RL_SUCCESS;

	/*
	 * now, gain exclusive access to the lock table.  _rlSetLocks() will set
	 * the rlockErr value.
	 */

	if (!_rlSetLocks(-LT_LOCK, -RL_NUM(fileHdrP->fhUniqueID)))
		return RL_FAIL;

	/*	
	 * search the table for any looks that we can remove.
	 */

	prevLockP = NIL_RECLOCK;
	for (scanLockP = &_rlockShm.rlockTableP[fileHdrP->fhLockIndex];
	     !nil(scanLockP);
	     scanLockP = nextLockP)
	{
		/*
		 * get the next lock entry while the getting is good.
		 */

		if (scanLockP->rlNextIndex == INVALID_INDEX)
			nextLockP = NIL_RECLOCK;
		else
			nextLockP =
				&_rlockShm.rlockTableP[scanLockP->rlNextIndex];

		/*
		 * if this lock wasn't applied by the caller, continue the
		 * loop.  however, save the current entry pointer, so we
		 * can use it as the previous entry pointer the next time
		 * around.
		 */

		if (scanLockP->rlDosPID != dosPID ||
		    scanLockP->rlSessID != sessID ||
		    scanLockP->rlOpenIndex != (indexT)openEntry)
		{
			prevLockP = scanLockP;
			continue;
		}

		/*
		 * remove the lock from the file header's lock list, and
		 * free the entry.  if prevLockP is NIL, we were at the
		 * head of the list.
		 */

		if (nil(prevLockP))
			fileHdrP->fhLockIndex = scanLockP->rlNextIndex;
		else
			prevLockP->rlNextIndex = scanLockP->rlNextIndex;
		freeLock(openFileP, scanLockP);
	}

	/*
	 * all done, release the table, record the state change and return.
	 */

	_rlUnlock();
	rlockState(openEntry, RLSTATE_ALL_UNLOCKED);
	return RL_SUCCESS;
}

/*
 *	STATIC	newLock() - get a lock structure from the free list
 *
 *	NOTE:	the table must be locked before this routine is called.
 *
 *	input:	openIndex - index of the open file entry setting the lock
 *		lockLow - low byte to lock
 *		lockHi - high byte to lock
 *		sessID - session ID of locker
 *		dosPID - DOS process ID of locker
 *
 *	proc:	if there are any free structures, return one, and update
 *		the free list pointers.  the indices in the returned
 *		structure are set to INVALID_INDEX.
 *
 *	output:	(recLockT *) - the structure or NIL
 *
 *	global:	_rlockShm - checked
 */

static recLockT FAR *
newLock(openIndex, lockLow, lockHi, sessID, dosPID)
indexT openIndex;
unsigned long lockLow, lockHi;
long sessID, dosPID;
{	r0 recLockT FAR	*recLockP;
	indexT rlockIndex;

	/*
	 * get the index of the free list.  if there is nothing there,
	 * return NIL.
	 */

	rlockIndex = *_rlockShm.lockFreeIndexP;
	if (rlockIndex == INVALID_INDEX)
	{
		rlockErr = RLERR_NOSPACE;
		return NIL_RECLOCK;
	}

	/*
	 * we have a free entry, can the host lock it?  doHostLock() will
	 * set rlockErr on failure.
	 */

	if (!doHostLock(&_rlockShm.openTableP[openIndex],
			lockLow, lockHi - lockLow + 1, TRUE))
		return NIL_RECLOCK;

	/*
	 * okay, we have a free entry, and the host is locked.  pull the
	 * entry off the free list.
	 */

	recLockP = &_rlockShm.rlockTableP[rlockIndex];
	*_rlockShm.lockFreeIndexP = recLockP->rlNextIndex;

	/*
	 * initialize the structure.
	 */

	recLockP->rlNextIndex	= INVALID_INDEX;
	recLockP->rlOpenIndex	= openIndex;
	recLockP->rlLockLow	= lockLow;
	recLockP->rlLockHi	= lockHi;
	recLockP->rlSessID	= sessID;
	recLockP->rlDosPID	= dosPID;
	return recLockP;
}

/*
 *	STATIC	freeLock() - return a record lock to the free list
 *
 *	NOTE:	the table must be locked before this routine is called.
 *
 *	input:	openFileP - the open file associated with the lock
 *		recLockP - pointer to the entry to free
 *
 *	proc:	unlock the host locks, then mark the entry as 'unused', and
 *		stick it on the head of the record lock table's free list.
 *
 *	output:	(void) - none
 *
 *	global:	_rlockShm - set
 */

static void
freeLock(openFileP, recLockP)
openFileT FAR *openFileP;
r0 recLockT FAR	*recLockP;
{
	/*
	 * tell the host that we are unlocking this section.
	 */

	(void)doHostLock(openFileP, recLockP->rlLockLow,
			 recLockP->rlLockHi - recLockP->rlLockLow + 1, FALSE);

	/*
	 * mark the entry as unused, and place it on the top of the free list.
	 */

	recLockP->rlLockHi = 0L;
	recLockP->rlNextIndex = *_rlockShm.lockFreeIndexP;
	*_rlockShm.lockFreeIndexP = recLockP - _rlockShm.rlockTableP;
}

/*
 *	STATIC	hasLocks() - does the specified file have any locks set?
 *
 *	WARN:	the lock table must be locked when this function is called.
 *
 *	input:	fileHdrP - pointer to the file header entry
 *		openIndex - the index of the 'open' entry
 *		sessID - the session ID
 *		dosPID - the DOS process ID
 *
 *	proc:	search through the locks on for the specified file, and find
 *		(if possible) at least one lock set by the specified process.
 *
 *	output:	(bool) - is an appropriate lock set?
 *
 *	global:	_rlockShm - checked
 */

static bool
hasLocks(fileHdrP, openIndex, sessID, dosPID)
fileHdrT FAR *fileHdrP;
r1 int openIndex; 
r2 long sessID;
r3 long dosPID;
{	r0 recLockT FAR	*scanLockP;

	/*
	 * if there are no locks set, return now.
	 */

	if (fileHdrP->fhLockIndex == INVALID_INDEX)
		return FALSE;

	/*	
	 * search the table for any locks that were set by the caller.  if
	 * one is found, return TRUE.  if none are found by the time the
	 * loop is forced to terminate, return FALSE.
	 */

	scanLockP = &_rlockShm.rlockTableP[fileHdrP->fhLockIndex];
	while (TRUE)
	{
		if (scanLockP->rlDosPID == dosPID &&
		    scanLockP->rlSessID == sessID &&
		    scanLockP->rlOpenIndex == openIndex)
		{
			return TRUE;
		}
		if (scanLockP->rlNextIndex == INVALID_INDEX)
			break;
		scanLockP = &_rlockShm.rlockTableP[scanLockP->rlNextIndex];
	}
	return FALSE;
}

/*
 *	STATIC	doHostLock() - handle the actual host locking control
 *
 *	input:	openFileP - pointer to the open file structure
 *		lockLow - base byte offset to lock/unlock
 *		lockCount - number of bytes to lock/unlock
 *		setLock - set the lock? (if not, then unlock)
 *
 *	proc:	inform the host of the lock we want to set or clear.  if
 *		unlocking, there is no failure.  otherwise honor any
 *		existing locks.
 *
 *	output:	(bool) - could we lock/unlock?
 *
 *	global:	rlockErr - set on failure
 */

static bool
doHostLock(openFileP, lockLow, lockCount, setLock)
openFileT FAR *openFileP;
unsigned long lockLow, lockCount;
bool setLock;
{	struct flock hostLock;
	unsigned long maxLockCount;
	int retCode;

	/*
	 * before doing anything else, check if we want to even deal with
	 * this.  if the area to be locked starts at a location that is
	 * larger than the maximum host's offset, forget the whole thing.
	 * it is recorded in the tables -- don't tell the system.
	 *
	 * on the other hand, if the initial offset is okay, but the length	
	 * is too long, adjust the length.  note that, for the second
	 * check, lockLow is guaranteed to be less than or equal to the
	 * maximum.  so, we can make the checks without the possibility of
	 * overflow.
	 *
	 * NOTE 1: this code presumes that MAX_LOCK_OFFSET is the *absolute*
	 * maximum offset, even though the fields in the structure to do
	 * the actual host lock, might allow a larger value.  this seems
	 * reasonably safe, since any other result would allow a file to
	 * have areas that couldn't be locked (that is, the starting offset
	 * couldn't be made large enough to reach each individual byte).
	 *
	 * NOTE 2: Since the maximum offset is the actual last value, rather
	 * than "the last value plus 1" as is more customary, the maximum
	 * legal count is actually 1 more than the difference between the
	 * maximum and the offset.  HOWEVER ... there is a problem here, since
	 * lockLow is 0-origin, whereas lockCount is generally not.  Thus,
	 * lockLow ranges from 0 to MAX_LOCK_OFFSET (inclusive), while
	 * lockCount ranges from 1 to MAX_LOCK_OFFSET+1 (inclusive).  However,
	 * UNIX doesn't seem to like a count of MAX_LOCK_OFFSET+1, so we have
	 * to special case it.
	 *
	 * NOTE 3: We don't check that lockCount is non-0.  Perhaps we should?
	 */

	if (lockLow > MAX_LOCK_OFFSET)
		return TRUE;

	if (lockLow == 0)
		maxLockCount = MAX_LOCK_OFFSET;
	else
		maxLockCount = MAX_LOCK_OFFSET - lockLow + 1;
	if (lockCount > maxLockCount)
		lockCount = maxLockCount;

	/*
	 * set the host lock structure.  on UNIX, simultaneous read and write
	 * locks aren't valid.  thus, we need to make some decisions.  for now,
	 * if the file has write access, we presume a write lock.  otherwise
	 * a read lock is set.  if we aren't setting the lock, we must be
	 * clearing it, so forget everything that I just said.
	 */

	if (!setLock)
		hostLock.l_type = F_UNLCK;
	else if (ANYSET(openFileP->ofAccMode, OFA_WRITE))
		hostLock.l_type = F_WRLCK;
	else
		hostLock.l_type = F_RDLCK;

	/*
	 * setting everything else is pretty straight-forward.  note that
	 * the start and length values must be translated into a signed value
	 * for this platform.  well, strictly, they don't *need* to be cast,
	 * but it makes the compilers happy.
	 */

	hostLock.l_start	= (long)lockLow;
	hostLock.l_len		= (long)lockCount;
	hostLock.l_whence	= 0;
	hostLock.l_pid		= 0;

	/*
	 * we now have the information, perform the lock or unlock.  if we
	 * were unlocking, ignore the return code, presume that it all
	 * worked.  otherwise the lock is relevant, and any errors have to
	 * have the global error set.
	 */

	retCode = fcntl(openFileP->ofFileDesc, F_SETLK, &hostLock);
	if (!setLock || (retCode == RL_SUCCESS))
		return TRUE;
	switch (errno)
	{
		case EACCES:	rlockErr = RLERR_LOCKED;	break;
		case EBADF:	rlockErr = RLERR_CORRUPT;	break;
		default:	rlockErr = RLERR_SYSTEM;
	}
	return FALSE;
}
