#ident	"@(#)pcintf:bridge/semfunc.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)semfunc.c	6.5	LCC);	/* Modified: 11/19/91 11:03:48 */

/*****************************************************************************

	Copyright (c) 1986 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#ifndef RS232PCI

#include "sysconfig.h"

#include <sys/types.h>
#include <errno.h>

#if defined(RD3)		/* ipc include files for Xenix */
#	include <ipc.h>
#	include <sd.h>
#else				/* ipc for Sys V */
#	include <sys/ipc.h>
#	include <sys/sem.h>
#	include <sys/shm.h>
#endif

#include <lmf.h>

#include "pci_types.h"
#include "common.h"
#include "log.h"

/*
 * Semaphore functions for reliable delivery.
 *
 *	Reliable delivery will be useing semaphores for synchronization
 *	of ACK and DATA packets to the PC.
 *
 *	This module contains the definitions and functions for the
 *	current implemetation using semaphores.
 *
 *	Author: Richard W. Patterson	3/24/86
 *
 *  Currently this is only for SCO Xenix System V (using System III IPC)
 *			   and ATT Unix  System V (using System  V  IPC)
 *
 */

extern int rd_flag;

#ifdef	RD3                 /* XENIX Sys III */
#define sem_template	"/tmp/%06dRDrp.SEM"
static char rdsem_name[20]; 		/* semaphore name */
extern char *sdget();
#else
#define	PCI_KEY	(key_t) (((long)'P' << 16) | (long)(('C' << 8) | 'I'))

		/* ipc for Sys V */
        	/* system V sem structures */
struct	sembuf decr[] = {
			 {0, -1, SEM_UNDO & IPC_NOWAIT}
			};

struct	sembuf incr[] = {
			 {0, 1, SEM_UNDO & IPC_NOWAIT}
			};

union	semnum {
		int	val;
		struct	semid_ds *buf;
		ushort	array[1];
	       };


static	union semnum ctl_arg;		/* semctl argument union */

extern int	semget		PROTO((key_t, int, int));
extern int	semctl		PROTO((int, int, int, .../* union semun * */));
extern int	semop		PROTO((int, struct sembuf *, size_t));
extern int	shmget		PROTO((key_t, int, int));
#if defined(SCO_ODT)
extern char	*shmat		PROTO((int, char *, int));
extern int	shmdt		PROTO((char *));
extern int	shmctl		PROTO((int, int, struct shmid_ds *));
#else
extern void	*shmat		PROTO((int, void *, int));
extern int	shmdt		PROTO((void *));
extern int	shmctl		PROTO((int, int, .../* struct shmid_ds * */));
#endif

#endif


/*
 * RDSEM_INIT		Initializes a reliable delivery semaphore in
 *			the form xxxxxxRDrpSEM.  Where xxxxxx is the PID
 *			of the creating process.  For SCO Xenix.
 *
 *	Entry: None
 *
 *	Returns:
 *		-1	Failure
 *		 0	Success
 *
 */
rdsem_init()
{
	int	pid,		/* process id */
		rdsem;		/* semaphore number for reliable delivery */
	int	err;		/* saves value of errno */

	pid = getpid();

#ifdef	RD3                 /* XENIX Sys III */
	sprintf(rdsem_name, sem_template, pid);	/* create the name */
	if ((rdsem = creatsem(rdsem_name, 0777)) != -1) {
		log("rdsem_init: init of semaphore completed.\n");
		return(rdsem);
	}
	log("rdsem_init: init of semaphore failed.\n");

#else /* RD5 */
	if ((rdsem = semget(PCI_KEY + pid, 1, IPC_CREAT|0777)) != -1) {
		ctl_arg.val = 1;		/* init sem to 1 */
		if (semctl(rdsem, 0, SETVAL, ctl_arg) < 0) {
			log("semctl: errno: %d\n", errno);
			return(-1);		/* call failed */
		}
		return(rdsem);
	}
	err = errno;
	log("semget: errno: %d\n", errno);
	serious(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("SEMFUNC1",
		"Cannot create semaphore, errno = %1\n"),
		"%d", err));
#endif  /* RD5 */
	return(-1);
}


/*
 * RDSEM_OPEN		This function is called by other processes
 *			requiring access to a semaphore.
 *
 *	Entry:
 *		None
 *
 *	Returns:
 *		-1	Failure
 *		 0	Success
 *
 */
rdsem_open()
{
	int	semn;
	int	ppid;

	ppid = getppid();

#ifdef	RD3                 /* XENIX Sys III */
	sprintf(rdsem_name, sem_template, getppid());	/* create the name */
	if ((semn = opensem(rdsem_name)) != -1) {
#else	/* RD5 */
	if ((semn = semget(PCI_KEY + ppid, 1, 0777)) != -1) {
#endif 	/* RD3 */
		log("rdsem_open: success\n");
		return(semn);
	}
	log("rdsem_open: failure\n");
}
	

/*
 * RDSEM_ACCESS		Returns whether a process can/has gained access
 *			to a particular semaphore.
 *
 *	Entry:
 *		sem_id		The desired semaphore to gain access to.
 *
 *	Return:
 *		-1		Semaphore is busy or non-existent
 *
 *
 */
rdsem_access(sem_id)
register int sem_id;
{
#ifdef	RD3                 /* XENIX Sys III */
	if (nbwaitsem(sem_id) != -1) {
		log("rdsem_access: gained access.\n");
		return(0);
	}
	log("rdsem_access: access denied.\n");

	switch (errno) {
		case ENAVAIL : log("rdsem_access: sem not available.\n");
			       break;

		default : log("rdsem_access: errno: %d\n", errno);
	}

	return(-1);
#else
	if (semop(sem_id, decr, 1) < 0) {
		log("rdsem_access: semop errno: %d\n", errno);
		return(-1);
	}

	log("rdsem_access: gained access.\n");
	return(0);
#endif
}



/*
 * RDSEM_RELINQ		Relinquishes control of the passed semaphore.
 *
 *	Entry:
 *		sem_num		The desired semaphore to relinquish control.
 *
 *	Return:
 *		-1		Semaphore is busy or non-existent
 *
 */
rdsem_relinq(sem_num)
register int sem_num;
{

#ifdef	RD3                 /* XENIX Sys III */
	return(sigsem(sem_num));

#else
	if (semop(sem_num, incr, 1) < 0) {
		log("rdsem_relinq: semop errno: %d\n", errno);
		return(-1);
	}

	log("rdsem_relinq: released.\n");
	return(0);
#endif
}


/*
 * RDSEM_UNLNK		Deletes the previously created reliable delivery
 *			semaphore.
 *
 *	Entry:
 *		None
 *
 *	Returns:
 *		Nothing
 *
 */
void rdsem_unlnk(sem_id)
register int sem_id;
{
	if (!rd_flag)
		return;

#ifdef	RD3                 /* XENIX Sys III */
	unlink(rdsem_name);
#else
	if (semctl(sem_id, 0, IPC_RMID, 0L) < 0)
		log("rdsem_unlnk: semctl errno: %d\n", errno);
#endif
}


/*
 * Shared memory functions for reliable delivery.
 *
 *	This module contains the definitions and functions for the
 *	current implemetation using shared memory.
 *
 *	Author: Richard W. Patterson	4/16/86
 *
 *
 */

#ifdef	RD3                 /* XENIX Sys III */
#define shm_template	"/tmp/%06dRDrp.SHM"
static char	RDshmpath[20];		/* for shared memory file. */
#endif

/*
 * RD_SHMINIT		This procedure initializes the shared memory for
 *			reliable delivery.  It sets up the necessary
 *			structures so other processes can access it.
 *
 *	Entry:	None
 *
 *	Returns:
 *		shmid	Returns a shared memory descriptor
 *		   -1	Could not create the shared memory
 *
 */
shmid_t rd_sdinit()
{
	shmid_t	shmid;		/* shared memory descriptor */

#ifdef	RD3                 /* XENIX Sys III */
	sprintf(RDshmpath, shm_template, getpid());
	if ((shmid = (char *) sdget(RDshmpath, SD_CREAT|SD_UNLOCK|SD_WRITE,
			((long) sizeof(struct rd_shared_mem)), 0777)) < 0)
		return (shmid_t)-1;
#else				/* ipc for Sys V */
	key_t	key;		/* key for shared memory */
	int	err;		/* saves value of errno */

	key = (key_t) getpid();	/* use process ID for key */
	if ((shmid = shmget(key, sizeof(struct rd_shared_mem), 
					IPC_CREAT|SHM_R|SHM_W)) < 0) {
		err = errno;
		log("shmget: errno: %d\n", errno);
		serious(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("SEMFUNC2",
			"Cannot create shared mem segment, errno = %1\n"),
			"%d", err));
		return (shmid_t)-1;
	}
#endif
	return shmid;
}


/*
 * RD_SDOPEN		This procedure essentially "opens" an already
 *			existing shared memory segment.  It returns a
 *			descriptor to the calling process.  The calling
 *			process will pass its parent's PID.  The parent's
 *			PID is used as a watermark.
 *
 *	Entry:  None
 *
 *	Returns:
 *		shmid		This processes shared mem ID
 *		   -1		Could not access/open this segment
 *
 */

shmid_t rd_sdopen()
{
	shmid_t	shmid;		/* shared memory descriptor */
#ifdef	RD3                 /* XENIX Sys III */
	int	pid;

	pid = getppid();	/* use parent's process id */
	sprintf(RDshmpath, shm_template, pid);
	if ((shmid = (char *) sdget(RDshmpath, SD_WRITE,
			((long) sizeof(struct rd_shared_mem)), 0777)) < 0)
		return (shmid_t)-1;
#else				/* ipc for Sys V */
	key_t	key;		/* key for shared memory */

	key = (key_t) getppid();	/* use parent's process ID */
	if ((shmid = shmget(key, sizeof(struct rd_shared_mem), 
				SHM_R|SHM_W)) < 0) {
		log("shmget: errno: %d\n", errno);
		return (shmid_t)-1;
	}
#endif
	return shmid;
}


/*
 * RD_SDENTER		This procedure requests that the system attach the
 *			desinated shared memory to the process' addr space.
 *
 *	Entry:
 *		shmdesc 	Desciptor for the shared memory
 *
 *	Returns:
 *		>0		Ptr to shared memory segment. (Unix Sys V)
 *		 0		Attach failed.
 *
 */
char *
rd_sdenter(shmdesc)
	shmid_t	shmdesc;
{
#ifdef	RD3                 /* XENIX Sys III */
	if (sdenter(shmdesc, SD_WRITE) < 0)
		return NULL;
	return shmdesc;
#else				/* ipc for Sys V */
	char	*ptr;
	if ((ptr = (char *) shmat(shmdesc, 0, SHM_R|SHM_W)) == (char *)-1) {
		log("shmat: errno: %d\n", errno);
		return NULL;
	}
	return ptr;
#endif
}


/*
 * RD_SDLEAVE		This procedures requests that the system deatach
 *			the specified shared memory from the calling
 *			process' address space.
 *
 *	Entry:
 *		shmaddr		Descriptor (segment addr) for the shared memory.
 *
 *	Returns:
 *		 0		The memory was detached.
 *		-1		Error in detaching the memory.
 *
 */
rd_sdleave(shmaddr)
char *shmaddr;
{

#ifdef	RD3                 /* XENIX Sys III */
	if (sdleave(shmaddr) < 0)
		return -1;
#else				/* ipc for Sys V */
	if (shmdt(shmaddr) < 0) {
		log("shmdt: errno: %d\n", errno);
		return -1;
	}
#endif
	return 0;
}

/*
 * RD_SHMDEL			This procedure removes the shared memory
 *				segment from the system.  Never to be found
 *				again.  This is called ONLY by the process
 *				that created the shared memory segment.
 *
 *	Entry:
 *		shmid		Desciptor of the shared memory
 *
 *	Returns:
 *		Nothing
 *
 */
void rd_shmdel(shmid)
	shmid_t	shmid;
{
	if (!rd_flag)
		return;

#ifdef	RD3                 /* XENIX Sys III */
	if (sdfree(shmid) < 0)
		log("rd_shmdel: couldn't release shared memory.\n");
	sprintf(RDshmpath, shm_template, getpid());
	unlink(RDshmpath);
#else				/* ipc for Sys V */
	if (shmctl(shmid, IPC_RMID, 0L) < 0)
		log("rd_shmdel: shmctl errno: %d\n", errno);
#endif
}
#endif /* RS232PCI */
