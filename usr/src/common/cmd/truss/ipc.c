/*		copyright	"%c%" 	*/

#ident	"@(#)truss:common/cmd/truss/ipc.c	1.2.10.1"
#ident  "$Header$"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include <signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"
#include "machdep.h"

/*
 * Routines related to interprocess communication
 * among the truss processes which are controlling
 * multiple traced processes.
 */

/* This module is carefully coded to contain only read-only data */
/* All read/write data is defined in ramdata.c (see also ramdata.h) */

extern	void	perror();
extern	void	errmsg();
extern	long	strtol();
extern	unsigned alarm();

/*
 * Function prototypes for static routines in this module.
 */

static	void	Ecritical( int );
static	void	Xcritical( int );
#if 0
static	void	UnFlush(void);
#endif


/*
 * Ensure everyone keeps out of each other's way
 * while writing lines of trace output
 */
void
Flush(void)
{

	/* except for regions bounded by Eserialize()/Xserialize(), */
	/* this is the only place anywhere in the program */
	/* where a write() to the trace output file takes place */
	/* so here is where we detect errors writing to the output */

	register FILE * fp = stdout;

	if (fp->__ptr == fp->__base)
		return;

	errno = 0;

	if (Cp->serialize == 0 || semid == -1)
	{
		increment(&Cp->nonserial);
		if (interrupt)
#if 0
			UnFlush();
#endif
			funflush(fp);
		else
			(void) fflush(fp);
		decrement(&Cp->nonserial);
	}
	else {
		Ecritical(0);
		if (interrupt)
#if 0
			UnFlush();
#endif
			funflush(fp);
		else
			(void) fflush(fp);
		Xcritical(0);
	}

	if (ferror(fp) && errno)	/* error on write(), probably EPIPE */
		interrupt = TRUE;		/* post an interrupt */
}

#if 0
static void
UnFlush(void)	/* avoid writing what is in the stdout buffer */
{
	register FILE * fp = stdout;

	fp->__cnt -= (fp->__ptr - fp->__base);	/* this is filthy */
	fp->__ptr = fp->__base;
}
#endif

/* Eserialize() and Xserialize() are used to bracket */
/* a region which may produce large amounts of output, */
/* such as showargs()/dumpargs() */

void
Eserialize(void)
{
	/* tell everyone to serialize output */
	increment(&Cp->serialize);

	Ecritical(0);

	/* wait for everyone to synchronize */
	while (Cp->nonserial && !interrupt)
		mysleep(1);
}

void
Xserialize(void)
{
	(void) fflush(stdout);

	Xcritical(0);

	/* OK not to serialize now */
	decrement(&Cp->serialize);
}

/*
 * Enter critical region ---
 * wait on semaphore, lock out other processes
 */
static void
Ecritical(int sem)	/* which semaphore */
{
	struct sembuf sembuf;
	register struct sembuf *sops;

	if (semid == -1)	/* there is no semaphore */
		return;

	sops = &sembuf;
	sops->sem_num = sem;		/* semaphore # */
	sops->sem_op = -1;		/* decrement semaphore */

/* Using SEM_UNDO too easily exhausts the system's */
/* semadj structures when truss follows many processes. */
/* It is only needed as a precaution against a truss process dying */
/* without having properly adjusted one of its semaphores. */
/* So long as truss works correctly, there will be no problem regardless */
#if 0
	sops->sem_flg = SEM_UNDO;	/* wait for semaphore to exceed 0 */
#else
	sops->sem_flg = 0;		/* wait for semaphore to exceed 0 */
#endif

	while (semop(semid, sops, 1) == -1) {
		if (errno != EINTR) {
			char semnum[2];
			semnum[0] = '0' + sem;
			semnum[1] = '\0';

			perror(command);
			errmsg("cannot decrement semaphore #", semnum);
			semid = -1;
			break;
		}
	}
}

/*
 * Exit critical region ---
 * release other processes waiting on semaphore
 */
static void
Xcritical(int sem)	/* which semaphore */
{
	struct sembuf sembuf;
	register struct sembuf *sops;

	if (semid == -1)	/* there is no semaphore */
		return;

	sops = &sembuf;
	sops->sem_num = sem;		/* semaphore # */
	sops->sem_op = 1;		/* increment semaphore */

/* See the comment above about using SEM_UNDO */
#if 0
	sops->sem_flg = SEM_UNDO;
#else
	sops->sem_flg = 0;
#endif

	while (semop(semid, sops, 1) == -1) {
		if (errno != EINTR) {
			char semnum[2];
			semnum[0] = '0' + sem;
			semnum[1] = '\0';

			perror(command);
			errmsg("cannot increment semaphore #", semnum);
			semid = -1;
			break;
		}
	}
}

/* add process to list of those being traced */
void
procadd(pid_t spid)
{
	register int i;
	register int j = -1;

	if (Cp == NULL)
		return;

	Ecritical(1);
	for (i = 0; i < sizeof(Cp->tpid)/sizeof(Cp->tpid[0]); i++) {
		if (Cp->tpid[i] == 0) {
			if (j == -1)	/* remember first vacant slot */
				j = i;
			if (Cp->spid[i] == 0)	/* this slot is better */
				break;
		}
	}
	if (i < sizeof(Cp->tpid)/sizeof(Cp->tpid[0]))
		j = i;
	if (j >= 0) {
		Cp->tpid[j] = getpid();
		Cp->spid[j] = spid;
	}
	Xcritical(1);
}

/* delete process from list of those being traced */
void
procdel(void)
{
	register int i;
	register pid_t tpid;

	if (Cp == NULL)
		return;

	tpid = getpid();

	Ecritical(1);
	for (i = 0; i < sizeof(Cp->tpid)/sizeof(Cp->tpid[0]); i++) {
		if (Cp->tpid[i] == tpid) {
			Cp->tpid[i] = 0;
			break;
		}
	}
	Xcritical(1);
}

/*
 * Check for open of /proc/nnnnn or /proc/nnnnn/... file.
 *
 * This is used when a traced process tries to open a file for writing
 * -- we want to detect the process opening any /proc file of a process
 * we are tracing so that we can try not to interfere with whatever it
 * is doing.
 *
 * If the process opens one of its own /proc files we just let go of it.
 * If it opens some other process's /proc file we try to track down
 * whether that other process is being traced by one of our truss clone
 * processes.  If it is, we tell our clone to let go.  If the process's
 * open attempt resulted in a EBUSY error, we have already interfered
 * with it, so before we let go of the process we try to back it up to
 * before it tried the open.  When it resumes (now free of our
 * interference) it should get better results.
 *
 * Return TRUE iff process opened its own
 * else inform controlling truss process.
 *
 * This implementation makes many assumptions -- most importantly that
 * the pathname will contain the substring "proc/" if it refers to a
 * /proc file.  This means that if a traced process chdir's to /proc and
 * uses relative pathnames, it will have to fight with truss for
 * control.  Too bad.
 */
int
checkproc(process_t *Pr, char *path, int err)
{
	register int pid;
	register int i;
	char * next;
	char * sp;
	int rc = FALSE;		/* assume not self-open */

	if ((sp = strstr(path, "proc/")) == NULL) /* find proc directory */
		return FALSE;

	sp += 4;			/* Point to trailing / */
	*sp = '\0';			/* Remove / temporarily */

	if ((pid = strtol(sp+1, &next, 10)) < 0 || /* filename not a number */
	    (*next != '\0' && *next != '/') || /* filename not a number */
	    !isprocdir(Pr, path) ||	/* file not in a /proc directory */
	    pid == getpid() ||		/* process opened truss's /proc file */
	    pid == 0) {			/* process opened process 0 */
		*sp = '/';		/* restore the embedded '/' */
		return FALSE;
	}

	*sp = '/';		/* restore the embedded '/' */

	/* process did open a /proc file --- */

	if (pid == Pr->upid)	/* process opened its own /proc file */
		rc = TRUE;
	else {			/* send signal to controlling truss process */
		for (i = 0; i < sizeof(Cp->tpid)/sizeof(Cp->tpid[0]); i++) {
			if (Cp->spid[i] == pid) {
				pid = Cp->tpid[i];
				break;
			}
		}
		if (i >= sizeof(Cp->tpid)/sizeof(Cp->tpid[0]))
			err = 0;	/* don't attempt retry of open() */
		else {	/* wait for controlling process to terminate */
			while (pid && Cp->tpid[i] == pid) {
				if (kill(pid, SIGUSR1) == -1)
					break;
				mysleep(1);
			}
			Ecritical(1);
			if (Cp->tpid[i] == 0)
				Cp->spid[i] = 0;
			Xcritical(1);
		}
	}

	if (err) {	/* prepare to reissue the open() system call */
#if 0
		UnFlush();	/* don't print the failed open() */
#endif
		funflush(stdout);
		if (rc && !cflag && prismember(&trace,SYS_open)) { /* last gasp */
			(void) sysentry(Pr);
			(void) printf("%s%s\n", pname, sys_string);
			sys_leng = 0;
			*sys_string = '\0';
		}
		(void) Psetsysnum(Pr, SYS_open);

		Pr->REG[R_PC] -= SYSCALL_OFF;	/* sizeof syscall instruction */

		(void) Pputregs(Pr);
	}

	return rc;
}
