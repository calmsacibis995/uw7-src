/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)lockf.h	1.2"
#ident	"$Header$"

/*
 *              PROPRIETARY NOTICE (Combined)
 *
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *
 *
 *
 *              Copyright Notice
 *
 *  Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *      (c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *      (c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *                All rights reserved.
 */

int	can_lock();
int	merge_lock();
void	add_lock();
void	rem_lock();
int	lock_region();
int	unlock_region();
int	lock();
int	unlock();

#ifndef NULL
#define	NULL	0
#endif
struct lock_ref {
	struct lock_ref  *Next,		/* Link list pointers		   */
			 *Prev;
	struct data_lock *ThisLock,	/* Lock this reference refers too  */
			 *Conflict;	/* Lock in conflict with this one  */
	int		 Type;		/* BLOCKING, BLOCKED, or FREE	   */
	};

struct data_lock {
	struct data_lock *Next,		/* Next lock in the list	   */
			*Prev,		/* Previous Lock in the list	   */
			*NextProc;	/* Link list of process' locks	   */
	struct process_locks *MyProc;	/* Points to process lock list	   */
	struct lock_ref  *Is_Blocking,	/* Is Blocking list		   */
			*Blocked_By;	/* Blocked by list NULL == ACTIVE  */
	struct reclock	*req;		/* A back pointer to the reclock   */
	int		base,		/* Region description.		   */
			length,		/* Length of lock		   */
			type,		/* EXCLUSIVE or SHARED		   */
			granted,	/* The granted flag		   */
			color,		/* Used during deadlock search	   */
			LockID,		/* ID of this lock		   */
			system,		/* System process 'pid' is on..    */
			pid,		/* "Owner" of lock.		   */
			class;		/* Class of lock (FILE, IO, NET)   */
	long		rsys,		/* System process 'rpid' is on..   */
			rpid;		/* Remote process identifier	   */
	};

/* process lock structure holds locks owned by a given process */
struct process_locks {
	long		pid;
	struct process_locks *next;
	struct data_lock	*lox;
	};


/* pointed to by the vnode */
struct lock_list {
	struct data_lock *exclusive,	/* The exclusive list		   */
			 *shared,	/* The shared list		   */
			 *pending;	/* Those that want to get on	   */
	};

/* structure that contains list of locks to be granted */

#define GRANT_LOCK_FLAG		0xf	/* flag in eld.l_xxx to signify   */
					/* grant locks need to be granted */
					/* by lockmgr			  */

/* Reference 'types' also BLOCKING below is used */
#define	FREE	0
#define	BLOCKED 1

#define	EXCLUSIVE	1	/* Lock is an Exclusive lock		*/
#define	BLOCKING	2	/* Block process for this lock. 	*/

/* Lock classes, determines how they are treated when they are unlocked. */
#define	FILE_LOCK	0	/* Generic class no special treatment	*/
#define	IO_LOCK		1	/* Indicates I/O waiting to complete 	*/
#define	LOCKMGR		2	/* Indicates lock manager lock 		*/

/* These defines are used with lock structures to determine various things */
#define	LOCK_TO_EOF	-1
#define	END(l)	(((l)->length == LOCK_TO_EOF || ((l)->length == 0)) ? \
	LOCK_TO_EOF : (l)->base + (l)->length - 1)
#define	ACTIVE(l)	((l)->Blocked_By == NULL)

/*
 * Determine if locks intersect.  Lock intersection is computed
 * by determining:
 *
 *	If			Then they intersect if
 *	--			----------------------
 * 	Both not to-EOF locks	bases and ends are mutually out of range
 *	One only is EOF lock	If the EOF-lock base is within end of
 *				non-EOF lock
 *	Both are EOF locks	Always
 */
#define	TOUCHING(a, b) \
	((END(a) >= 0 && END(b) >= 0 && \
	(!((END(a) < (b)->base) || ((a)->base > END(b))))) || \
	(END(a) < 0 && END(b) >= 0 && END(b) >= (a)->base) || \
	(END(a) >= 0 && END(b) < 0 && END(a) >= (b)->base) || \
	(END(a) < 0 && END(b) < 0))

/* Is TRUE if A and B are adjacent */
#define	ADJACENT(a, b) ((END(a)+1 == (b)->base) || ((a)->base == END(b)+1))

/* Is TRUE if a is completely contained within b */
#define	WITHIN(a, b) \
	(((a)->base >= (b)->base) && \
	((END(a) >= 0 && END(b) >= 0 && END(a) <= END(b)) || (END(b) < 0) || \
	((b)->length == 0x7fffffff)))

#define	SAMEOWNER(a, b)	(((a)->pid == (b)->pid) && \
			((a)->rpid == (b)->rpid))

struct	data_lock	*free_locks;

#define	GETLOCK(l)	(l) = get_lock()
#define	FREELOCK(l)	{ if ((l)->LockID < 0) { (l)->pid = 0; \
			    (l)->Next = free_locks; \
			    (l)->NextProc = 0; \
			    free_locks = (l); \
			  }\
 			}

