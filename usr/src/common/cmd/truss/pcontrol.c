/*		copyright	"%c%" 	*/

#ident	"@(#)truss:common/cmd/truss/pcontrol.c	1.3.9.6"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <sys/procset.h>
#include <sys/priocntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"
#include "machdep.h"

/* Process Management */

/* This module is carefully coded to contain only read-only data */
/* All read/write data is defined in ramdata.c (see also ramdata.h) */


/*
 * Function prototypes for static routines in this module.
 */

static	void	prdump(process_t *);
static	void	deadcheck(process_t *);

#include "registers.h"

/* create new controlled process */
int
Pcreate(process_t *P,		/* program table entry */
	char **args)		/* argument array, including the command name */
{
	register int i;
	register int upid;

	upid = fork();

	if (upid == 0) {		/* child process */
		(void) pause();		/* wait for PRSABORT from parent */

		/* if running setuid or setgid, reset credentials to normal */
		if ((i = getgid()) != getegid())
			(void) setgid(i);
		if ((i = getuid()) != geteuid())
			(void) setuid(i);

		(void) execvp(*args, args);	/* execute the command */
		_exit(127);
	}

	if (upid == -1) {		/* failure */
		perror("Pcreate fork()");
		return -1;
	}

	/* initialize the process structure */
	(void) memset(P, 0, sizeof *P);

	P->cntrl = TRUE;
	P->child = TRUE;
	P->state = PS_RUN;
	P->upid  = upid;

	/* Open /proc files */
	if ((P->asfd = Popen(upid, "as", O_RDWR|O_EXCL, 0)) == -1) {
		perror("Pcreate open as");
		kill(upid, SIGKILL);
		return -1;
	}
	if ((P->ctlfd = Popen(upid, "ctl", O_WRONLY, 0)) == -1) {
		perror("Pcreate open ctl");
		kill(upid, SIGKILL);
		close(P->asfd);
		return -1;
	}
	if ((P->statusfd = Popen(upid, "status", O_RDONLY, 0)) == -1) {
		perror("Pcreate open status");
		kill(upid, SIGKILL);
		close(P->asfd);
		close(P->ctlfd);
		return -1;
	}

	/* mark as run-on-last-close so it runs even if we die on a signal */
	if (Pset(P, PR_RLC) == -1)
		perror("Pcreate PCSET PR_RLC");

	for (;;) {		/* wait for process to block in pause() */
		(void) Pstop(P);	/* stop the controlled process */

#if 0
		if (debugflag) {

			register int i;
			pstatus_t status;

			/* read the process registers */
			(void) lseek(P->statusfd, 0L, 0);
			if (read(P->statusfd, &status, sizeof status) !=
			    sizeof status)
				perror("Pcreate: readstatus failure");
			else for (i = 0; i < NGREG; i++) {
				(void) fprintf(stderr,
					"  %%%s: 0x%.8X 0x%.8X\n",
					regname[i], P->REG[i],
					status.pr_reg[i]);
			}
		}
#endif
		if (P->state == PS_STOP &&
		    P->why.pr_lwp.pr_why == PR_REQUESTED &&
		    (P->why.pr_lwp.pr_flags & PR_ASLEEP) &&
		    Pgetsysnum(P) == SYS_pause)
			break;

		if (P->state != PS_STOP ||	/* interrupt or process died */
		    Psetrun(P, 0, 0) != 0) {	/* can't restart */
			u_long sig = SIGKILL;

			(void) Pctl(P, PCKILL, &sig, sizeof sig); /* kill ! */
			(void) close(P->asfd);
			(void) close(P->ctlfd);
			(void) close(P->statusfd);
			(void) kill(upid, sig);			/* kill !! */
			P->state = PS_DEAD;
			P->asfd = 0;
			P->ctlfd = 0;
			P->statusfd = 0;
			return -1;
		}

		/*
		 * Yield the processor to give the child a fighting chance
		 * to run.  This prevents us from starving the child when
		 * we're running in the fixed priority scheduling class.
		 */
		(void) priocntl(P_PID, P_MYID, PC_YIELD, (void *)0);
	}

	(void) Psysentry(P, SYS_exit, 1);	/* catch these sys calls */
	(void) Psysentry(P, SYS_exec, 1);
	(void) Psysentry(P, SYS_execve, 1);

	/* kick it off the pause() */
	if (Psetrun(P, 0, PRSABORT) == -1) {
		u_long sig = SIGKILL;

		perror("Pcreate PCRUN");
		(void) Pctl(P, PCKILL, &sig, sizeof sig);
		(void) kill(upid, sig);
		(void) close(P->asfd);
		(void) close(P->ctlfd);
		(void) close(P->statusfd);
		P->state = PS_DEAD;
		P->asfd = 0;
		P->ctlfd = 0;
		P->statusfd = 0;
		return -1;
	}

	(void) Pwait(P);	/* wait for exec() or exit() */

	return 0;
}

/* open a /proc file */
int
Popen(int pid,			/* process id */
      char *file,		/* file name */
      int mode,			/* open mode */
      int oldfd)		/* oldfd to close (0 if initial open) */
{
	char procname[100];
	int fd, dupit = 0;
	extern int errno;

	(void) sprintf(procname, "%s/%d/%s", procdir, pid, file);
	if ((fd = open(procname, mode)) < 0)
		return -1;

	/* on initial open, make sure it's not one of 0, 1, or 2. */
	/* this allows truss to work when spawned by init(1m). */
	/* on a re-open, close the original file descriptor and try */
	/* to retain that number. */
	if (oldfd == 0) {
		 if (0 <= fd && fd <= 2) {
			dupit = 1;
			oldfd = 3;
		}
	} else {
		if (close(oldfd) != 0)
			oldfd = 3;
		dupit = (fd != oldfd);
	}
	if (dupit) {
		int dfd = fcntl(fd, F_DUPFD, oldfd);

		(void) close(fd);
		if (dfd < 0)
			return -1;
		fd = dfd;
	}
	/* mark it close-on-exec so any created process doesn't inherit it */
	(void) fcntl(fd, F_SETFD, 1);
	errno = 0;
	return fd;
}

/* This is for the !?!*%! call to sleep() in execvp() */
unsigned int
sleep(unsigned int n)
{
	if (n)
		mysleep(n);
	return 0;
}

void
mysleep(unsigned int n)
{
	(void) sighold(SIGALRM);	/* avoid race condition */
	(void) alarm(n);
	(void) sigpause(SIGALRM);	/* don't care if awoken prematurely */
	(void) alarm(0);		/* cancel outstanding alarm from intr */
	timeout = FALSE;
}

#if 0
static void
fddump(int fd)	/* debugging code -- dump fstat(fd) */
{
	struct stat statb;
	const char * s;

	(void) fprintf(stderr, "fd = %d", fd);
	if (fstat(fd, &statb) == -1)
		goto out;
	switch (statb.st_mode & S_IFMT) {
	case S_IFDIR: s="S_IFDIR"; break;
	case S_IFCHR: s="S_IFCHR"; break;
	case S_IFBLK: s="S_IFBLK"; break;
	case S_IFREG: s="S_IFREG"; break;
	case S_IFIFO: s="S_IFIFO"; break;
	default:      s="???"; break;
	}
	(void) fprintf(stderr, "  %s  mode = 0%o  dev = 0x%.4X  ino = %d",
		s,
		statb.st_mode & ~S_IFMT,
		statb.st_dev,
		statb.st_ino);
	(void) fprintf(stderr, "  uid = %d,  gid = %d,  size = %ld\n",
		statb.st_uid,
		statb.st_gid,
		statb.st_size);

out:
	(void) fputc('\n', stderr);
}
#endif /* 0 */

/* grab existing process */
int
Pgrab(process_t *P,		/* program table entry */
      pid_t upid,		/* UNIX process ID */
      int force)		/* if TRUE, grab regardless */
{
	register int asfd = -1, ctlfd = -1, statusfd = -1;
	int nmappings;
	int ruid;
	int rc;
	struct prcred prcred;

again:	/* Come back here if we lose it in the Window of Vulnerability */
	if (asfd >= 0 || ctlfd >= 0 || statusfd >= 0) {
		(void) close(asfd);
		(void) close(ctlfd);
		(void) close(statusfd);
		asfd = ctlfd = statusfd = -1;
	}

	(void) memset(P, 0, sizeof *P);

	if (upid <= 0)
		return -1;

	/* Request exclusive open to avoid grabbing someone else's	*/
	/* process and to prevent others from interfering afterwards.	*/
	/* If this fails and the 'force' flag is set, attempt to	*/
	/* open non-exclusively (effective only for the super-user).	*/
	/* If the address-space file doesn't exist, this indicates a	*/
	/* system process that we can't trace anyway.			*/
	if ((asfd = Popen(upid, "as", O_RDWR|O_EXCL, 0)) < 0) {
		if (errno == ENOENT)
			return -1;	/* System process */
		if ((asfd = (force ? Popen(upid, "as", O_RDWR, 0) : -1)) < 0) {
			if (errno == EBUSY && !force)
				return 1;
			if (debugflag)
				perror("Pgrab open()");
			return -1;
		}
	}

	/* Now open other /proc files */
	if ((ctlfd = Popen(upid, "ctl", O_WRONLY, 0)) == -1) {
		close(asfd);
		return -1;
	}
	if ((statusfd = Popen(upid, "status", O_RDONLY, 0)) == -1) {
		close(asfd);
		close(ctlfd);
		return -1;
	}
	P->asfd = asfd;
	P->ctlfd = ctlfd;
	P->statusfd = statusfd;


	/* ---------------------------------------------------- */
	/* We are now in the Window of Vulnerability (WoV).	*/
	/* The process may exec() a setuid/setgid or unreadable	*/
	/* object file between now and the time it stops.	*/
	/* We will get EBADF in this case and must start over.	*/
	/* ---------------------------------------------------- */

	/* Verify process credentials in case we are running setuid root.   */
	/* We only verify that our real uid matches the process's real uid. */
	/* This means that the user really did create the process, even     */
	/* if using a different group id (via newgrp(1) for example).       */
	if ((rc = Pcred(upid, &prcred)) == -1) {
		if (errno == EBADF)	/* WoV */
			goto again;
		if (rc == -1 && errno != ENOENT)
			/* Don't complain about zombies */
			perror("Pgrab Pcred");
		(void) close(asfd);
		(void) close(ctlfd);
		(void) close(statusfd);
		return -1;
	}

	/* mark as run-on-last-close so it runs even if we die from SIGKILL */
	if (Pset(P, PR_RLC) == -1) {
		if (errno == EBADF)	/* WoV */
			goto again;
		if (errno != ENOENT)	/* Don't complain about zombies */
			perror("Pgrab PCSET PR_RLC");
	}

	P->cntrl = TRUE;
	P->child = FALSE;
	P->state = PS_RUN;
	P->upid  = upid;

	/* before stopping the process, make sure it's not ourself */
	if (upid == getpid()) {
		/* write a magic number, read it through /proc file */
		/* and see if the results match. */
		long magic1 = 0;
		long magic2 = 2;

		errno = 0;

		if (Pread(P, (ulong_t)&magic1, &magic2, sizeof magic2)
		    == sizeof magic2 &&
		    magic2 == 0 &&
		    (magic1 = 0xfeedbeef) &&
		    Pread(P, (ulong_t)&magic1, &magic2, sizeof magic2)
		    == sizeof magic2 &&
		    magic2 == 0xfeedbeef) {
			(void) close(asfd);
			(void) close(ctlfd);
			(void) close(statusfd);
			(void) fprintf(stderr,
			"Pgrab(): process attempted to grab itself\n");
			return -1;
		}
	}

	/* Stop the process, get its status and its signal/syscall masks. */
	if (Pstatus(P, PCSTOP, 2) != 0) {
		if (P->state == PS_LOST)	/* WoV */
			goto again;
		if (errno != EINTR ||
		    (P->state != PS_STOP &&
		     !(P->why.pr_lwp.pr_flags & PR_DSTOP))) {
			if (P->state != PS_RUN) {
				if (errno != ENOENT)
					perror("Pgrab PCSTOP");
#if 0
				fddump(fd);
#endif
			}
			(void) close(asfd);
			(void) close(ctlfd);
			(void) close(statusfd);
			return -1;
		}
	}

	P->sigmask = P->why.pr_sigtrace;
	P->faultmask = P->why.pr_flttrace;
	P->sysentry = P->why.pr_sysentry;
	P->sysexit = P->why.pr_sysexit;

	return 0;
}

/* reopen the /proc file (after PS_LOST) */
int
Preopen(process_t *P,
	int force)			/* if TRUE, grab regardless */
{
	register int asfd, ctlfd, statusfd;
	char procname[100];

	/* reopen the /proc files */
	if ((asfd = Popen(P->upid, "as", O_RDWR, P->asfd)) < 0 &&
	    (asfd = (force? Popen(P->upid, "as", O_RDWR, P->asfd) : -1)) < 0) {
		if (debugflag)
			perror("Preopen open as");
		return -1;
	}
	if ((ctlfd = Popen(P->upid, "ctl", O_WRONLY, P->ctlfd)) < 0) {
		if (debugflag)
			perror("Preopen open ctl");
		close(asfd);
		return -1;
	}
	if ((statusfd = Popen(P->upid, "status", O_RDONLY, P->statusfd)) < 0) {
		if (debugflag)
			perror("Preopen open status");
		close(asfd);
		close(ctlfd);
		return -1;
	}

	P->state	= PS_RUN;
	P->asfd		= asfd;
	P->ctlfd	= ctlfd;
	P->statusfd	= statusfd;

	/* set run-on-last-close so it runs even if we die from SIGKILL */
	if (Pset(P, PR_RLC) == -1)
		perror("Preopen PCSET PR_RLC");

	/* process should be stopped on exec (REQUESTED) */
	/* or else should be stopped on exit from exec() (SYSEXIT) */
	if (Pwait(P) == 0 &&
	    P->state == PS_STOP &&
	    (P->why.pr_lwp.pr_why == PR_REQUESTED ||
	     (P->why.pr_lwp.pr_why == PR_SYSEXIT &&
	      (P->why.pr_lwp.pr_what == SYS_exec ||
	       P->why.pr_lwp.pr_what == SYS_execve)))) {
		/* fake up stop-on-exit-from-execve */
		if (P->why.pr_lwp.pr_why == PR_REQUESTED) {
			P->why.pr_lwp.pr_why = PR_SYSEXIT;
			P->why.pr_lwp.pr_what = SYS_execve;
		}
	} else {
		(void) fprintf(stderr,
		"Preopen: expected REQUESTED or SYSEXIT(SYS_execve) stop\n");
	}

	return 0;
}

/* release process to run freely */
int
Prelease(process_t *P)
{
	if (P->asfd == 0)
		return -1;

	if (debugflag)
		(void) fprintf(stderr, "Prelease: releasing pid # %d\n",
			P->upid);

	/* attempt to stop it if we have to reset its registers */
	if (P->sethold || P->setregs) {
		register int count;
		for (count = 10;
		     count > 0 && (P->state == PS_RUN || P->state == PS_STEP);
		     count--) {
			(void) Pstop(P);
		}
	}

	/* if we lost control, all we can do is close the file */
	if (P->state == PS_STOP) {
		if (P->sethold &&
		    Pctl(P, PCSHOLD, &P->why.pr_lwp.pr_context.uc_sigmask,
			 sizeof (sigset_t)) != 0)
			perror("Prelease PCSHOLD");
		if (P->setregs &&
		    Pctl(P, PCSREG, &P->REG[0], sizeof (gregset_t)) != 0)
			perror("Prelease PCSREG");
	}

	(void) Preset(P, PR_FORK);
	(void) Pset(P, PR_RLC);
	(void) close(P->statusfd);
	(void) close(P->ctlfd);
	(void) close(P->asfd);	/* this sets the process running */

	/* zap the process structure */
	(void) memset(P, 0, sizeof *P);

	return 0;
}

/* debugging */
static void
prdump(process_t *P)
{
	long bits = *((long *)&P->why.pr_sigpend);

	if (P->why.pr_lwp.pr_cursig)
		(void) fprintf(stderr, "  p_cursig  = %d",
			       P->why.pr_lwp.pr_cursig);
	if (bits)
		(void) fprintf(stderr, "  p_sigpend = 0x%.8X", bits);
	(void) fputc('\n', stderr);
}

#if 0
/* debugging */
static void
dumpwhy(process_t *P, char *str)
{
	register int i;

	if (str)
		(void) fprintf(stderr, "%s\n", str);
	(void) fprintf(stderr, "pr_flags  = 0x%.8X\n", P->why.pr_flags);
	(void) fprintf(stderr, "pr_why    = 0x%.8X\n", P->why.pr_why);
	(void) fprintf(stderr, "pr_what   = 0x%.8X\n", P->why.pr_what);
	(void) fprintf(stderr, "pr_pid    = 0x%.8X\n", P->why.pr_pid);
	(void) fprintf(stderr, "pr_ppid   = 0x%.8X\n", P->why.pr_ppid);
	(void) fprintf(stderr, "pr_pgrp   = 0x%.8X\n", P->why.pr_pgrp);
	(void) fprintf(stderr, "pr_cursig = 0x%.8X\n", P->why.pr_cursig);
	(void) fprintf(stderr, "pr_sigpend= 0x%.8X\n", P->why.pr_sigpend.bits[0]);
	(void) fprintf(stderr, "pr_lwphold= 0x%.8X\n", P->why.pr_lwphold.bits[0]);
	(void) fprintf(stderr, "pr_instr  = 0x%.8X\n", P->why.pr_instr);
	(void) fprintf(stderr, "pr_utime  = 0x%.8X\n", P->why.pr_utime);
	(void) fprintf(stderr, "pr_stime  = 0x%.8X\n", P->why.pr_stime);
	for (i = 0; i < NGREG; i++)
		(void) fprintf(stderr, "%%%s = %.8x\n",
			regname[i], P->REG[i]);
}
#endif

/* wait for process to stop for any reason */
int
Pwait(process_t *P)
{
	return Pstatus(P, PCWSTOP, 0);
}

/* direct process to stop; wait for it to stop */
int
Pstop(process_t *P)
{
	return Pstatus(P, PCSTOP, 0);
}

/* get status and possibly wait for specified process to stop or terminate */
int
Pstatus(register process_t *P,	/* program table entry */
	register int request,	/* PCSTOP, PCWSTOP, or 0 */
	unsigned sec)		/* if non-zero, alarm timeout in seconds */
{
	register int status = 0;
	int err = 0;

	switch (P->state) {
	case PS_NULL:
	case PS_LOST:
	case PS_DEAD:
		return -1;
	case PS_STOP:
		if (request != 0)
			return 0;
	}

	switch (request) {
	case 0:
	case PCSTOP:
	case PCWSTOP:
		break;
	default:
		/* programming error */
		(void) fprintf(stderr, "Pstatus: illegal request\n");
		return -1;
	}

	timeout = FALSE;
	if (sec)
		(void) alarm(sec);
	if (request != 0 && Pctl(P, request, 0, 0) != 0) {
		err = errno;
		if (sec)
 			(void) alarm(0);
		if (err == EINTR) {
			(void) lseek(P->statusfd, 0L, 0);
			if (read(P->statusfd, &P->why, sizeof P->why) !=
			    sizeof P->why)
				err = errno;
		}
	} else {
		(void) lseek(P->statusfd, 0L, 0);
		if (read(P->statusfd, &P->why, sizeof P->why) != sizeof P->why)
			err = errno;
	}

	if (sec)
		(void) alarm(0);

	if (err) {
		switch (err) {
		case EINTR:		/* timeout or user typed DEL */
			if (debugflag)
				(void) fprintf(stderr, "Pstatus: EINTR\n");
			break;
		case EBADF:	       /* we lost control of the the process */
			if (debugflag)
				(void) fprintf(stderr, "Pstatus: EBADF\n");
			P->state = PS_LOST;
			break;
		default:		/* check for dead process */
			if (debugflag || err != ENOENT) {
				const char * errstr;

				switch (request) {
				case 0:
					errstr = "Pstatus status"; break;
				case PCSTOP:
					errstr = "Pstatus PCSTOP"; break;
				case PCWSTOP:
					errstr = "Pstatus PCWSTOP"; break;
				default:
					errstr = "Pstatus PC???"; break;
				}
				perror(errstr);
			}
			deadcheck(P);
			break;
		}
		if (!timeout || err != EINTR) {
			errno = err;
			return -1;
		}
	}

	if (!(P->why.pr_lwp.pr_flags & PR_STOPPED)) {
		if (request == 0 || timeout) {
			timeout = FALSE;
			return 0;
		}
		(void) fprintf(stderr, "Pstatus: process is not stopped\n");
		return -1;
	}

	P->state = PS_STOP;
	timeout = FALSE;

#if 0	/* debugging */
    {
	register int i;
	pstatus_t status;

	/* read the process registers */
	(void) lseek(P->statusfd, 0L, 0);
	if (read(P->statusfd, &status, sizeof(status)) != sizeof(status))
		perror("Pstatus: readstatus failure");
	else for (i = 0; i < NGREG; i++) {
		if (debugflag && P->REG[i] != status.pr_reg[i])
			(void) fprintf(stderr,
				"  %%%s: 0x%.8X 0x%.8X\n",
				regname[i], P->REG[i], status.pr_reg[i]);
		P->REG[i] = status.pr_reg[i];
	}
    }
#endif

	switch (P->why.pr_lwp.pr_why) {
	case PR_REQUESTED:
		if (debugflag) {
			(void) fprintf(stderr, "Pstatus: why: REQUESTED");
			prdump(P);
		}
		break;
	case PR_SIGNALLED:
		if (debugflag) {
			(void) fprintf(stderr, "Pstatus: why: SIGNALLED %s",
				signame(P->why.pr_lwp.pr_what));
			prdump(P);
		}
		break;
	case PR_FAULTED:
		if (debugflag) {
			(void) fprintf(stderr, "Pstatus: why: FAULTED %s",
				fltname(P->why.pr_lwp.pr_what));
			prdump(P);
		}
		break;
	case PR_SYSENTRY:
		/* remember syscall address */
		P->sysaddr = P->REG[R_PC]-SYSCALL_OFF;

		if (debugflag) {
			(void) fprintf(stderr, "Pstatus: why: SYSENTRY %s",
				sysname(P->why.pr_lwp.pr_what, -1));
			prdump(P);
		}
		break;
	case PR_SYSEXIT:
		/* remember syscall address */
		P->sysaddr = P->REG[R_PC]-SYSCALL_OFF;

		if (debugflag) {
			(void) fprintf(stderr, "Pstatus: why: SYSEXIT %s",
				sysname(P->why.pr_lwp.pr_what, -1));
			prdump(P);
		}
		break;
	case PR_JOBCONTROL:
		if (debugflag) {
			(void) fprintf(stderr, "Pstatus: why: JOBCONTROL %s",
				signame(P->why.pr_lwp.pr_what));
			prdump(P);
		}
		break;
	default:
		if (debugflag) {
			(void) fprintf(stderr, "Pstatus: why: Unknown");
			prdump(P);
		}
		status = -1;
		break;
	}

#if 0	/* debugging */
	if (debugflag)
		dumpwhy(P, 0);
#endif

	return status;
}

static void
deadcheck(register process_t *P)
{
	if (P->asfd == 0)
		P->state = PS_DEAD;
	else {
		(void) lseek(P->statusfd, 0L, 0);
		while (read(P->statusfd, (char *)&P->why, sizeof(pstatus_t))
		  ==-1) {
			switch (errno) {
			default:
				/* process is dead */
				if (P->asfd != 0) {
					(void) close(P->asfd);
					(void) close(P->ctlfd);
					(void) close(P->statusfd);
				}
				P->asfd = 0;
				P->ctlfd = 0;
				P->statusfd = 0;
				P->state = PS_DEAD;
				break;
			case EINTR:
				continue;
			case EBADF:
				P->state = PS_LOST;
				break;
			}
			break;
		}
	}
}

/* get values of registers from stopped process */
int
Pgetregs(process_t *P)
{
	if (P->state != PS_STOP)
		return -1;
	return 0;		/* registers are always available */
}

/* get the value of one register from stopped process */
int
Pgetareg(process_t *P,
	 int reg)		/* register number */
{
	if (reg < 0 || reg >= NGREG) {
		(void) fprintf(stderr,
			"Pgetareg(): invalid register number, %d\n", reg);
		return -1;
	}
	if (P->state != PS_STOP)
		return -1;
	return 0;		/* registers are always available */
}

/* put values of registers into stopped process */
int
Pputregs(process_t *P)
{
	if (P->state != PS_STOP)
		return -1;
	P->setregs = TRUE;	/* set registers before continuing */
	return 0;
}

/* put value of one register into stopped process */
int
Pputareg(process_t *P,
	 int reg)		/* register number */
{
	if (reg < 0 || reg >= NGREG) {
		(void) fprintf(stderr,
			"Pputareg(): invalid register number, %d\n", reg);
		return -1;
	}
	if (P->state != PS_STOP)
		return -1;
	P->setregs = TRUE;	/* set registers before continuing */
	return 0;
}

int
Psetrun(register process_t *P,
	int sig,		/* signal to pass to process */
	register int flags)	/* flags: PRCSIG|PRSTEP|PRSABORT|PRSTOP */
{
	register int request;		/* for setting signal */
	register int why = P->why.pr_lwp.pr_why;
	siginfo_t info;
	u_long uflags = flags;
	u_long usig = sig;

	if (sig < 0 || sig > PRMAXSIG ||
	    P->state != PS_STOP)
		return -1;

	if (sig) {
		if (flags & PRCSIG)
			request = PCKILL;
		else {
			switch (why) {
			case PR_REQUESTED:
			case PR_SIGNALLED:
				request = PCSSIG;
				break;
			default:
				request = PCKILL;
				break;
			}
		}
	}

	/* must be initialized to zero */
	(void) memset(&info, 0, sizeof info);
	info.si_signo = sig;

	if ((P->setsig &&
	     Pctl(P, PCSTRACE, &P->sigmask, sizeof (sigset_t)) == -1) ||
	    (P->sethold &&
	     Pctl(P, PCSHOLD, &P->why.pr_lwp.pr_context.uc_sigmask,
		  sizeof (sigset_t)) == -1) ||
	    (P->setfault &&
	     Pctl(P, PCSFAULT, &P->faultmask, sizeof (fltset_t)) == -1) ||
	    (P->setentry &&
	     Pctl(P, PCSENTRY, &P->sysentry, sizeof (sysset_t)) == -1) ||
	    (P->setexit &&
	     Pctl(P, PCSEXIT, &P->sysexit, sizeof(sysset_t)) == -1) ||
	    (P->setregs &&
	     Pctl(P, PCSREG, &P->REG[0], sizeof (gregset_t)) == -1) ||
	    (sig && request == PCKILL &&
	     Pctl(P, request, &usig, sizeof usig) == -1) ||
	    (sig && request == PCSSIG &&
	     Pctl(P, request, &info, sizeof info) == -1)) {
	bad:
		switch (errno) {
		case ENOENT:
			P->state = PS_DEAD;
			break;
		case EBADF:
			P->state = PS_LOST;
			break;
		default:
			perror("Psetrun");
			break;
		}
		return -1;
	}
	P->setentry = FALSE;
	P->setexit  = FALSE;
	P->setregs  = FALSE;

	if (Pctl(P, PCRUN, &uflags, sizeof uflags) == -1) {
		if ((why != PR_SIGNALLED && why != PR_JOBCONTROL) ||
		    errno != EBUSY)
			goto bad;
		goto out;	/* ptrace()ed or jobcontrol stop -- back off */
	}

	P->setsig   = FALSE;
	P->sethold  = FALSE;
	P->setfault = FALSE;
out:
	P->state    = (flags&PRSTEP)? PS_STEP : PS_RUN;
	return 0;
}

int
Pstart(process_t *P,
       int sig)				/* signal to pass to process */
{
	return Psetrun(P, sig, 0);
}

int
Pterm(register process_t *P)
{
	int sig = SIGKILL;

	if (debugflag)
		(void) fprintf(stderr,
			"Pterm: terminating pid # %d\n", P->upid);
	if (P->state == PS_STOP)
		(void) Pstart(P, SIGKILL);
	(void) Pctl(P, PCKILL, &sig, sizeof sig);	/* make sure */
	(void) kill((int)P->upid, SIGKILL);		/* make double sure */

	if (P->asfd != 0) {
		(void) close(P->asfd);
		(void) close(P->ctlfd);
		(void) close(P->statusfd);
	}

	/* zap the process structure */
	(void) memset(P, 0, sizeof *P);

	return 0;
}

int
Pread(process_t *P,
      ulong_t address,			/* address in process */
      void *buf,			/* caller's buffer */
      size_t nbyte)			/* number of bytes to read */
{
	if (nbyte <= 0)
		return 0;

	if (lseek(P->asfd, (long)address, 0) == address)
		return read(P->asfd, buf, nbyte);

	return -1;
}

int
Pwrite(process_t *P,
       ulong_t address,			/* address in process */
       const void *buf,			/* caller's buffer */
       size_t nbyte)			/* number of bytes to write */
{
	register int rc = -1;

	if (nbyte <= 0)
		return 0;

	if (lseek(P->asfd, (long)address, 0) != address ||
	    (rc = write(P->asfd, buf, nbyte)) == -1)
		perror("Pwrite");

	return rc;
}

/* action on specified signal */
int
Psignal(process_t *P,		/* program table exit */
	int which,		/* signal number */
	int stop)		/* if TRUE, stop process; else let it go */
{
	register int oldval;

	if (which <= 0 || which > PRMAXSIG || (which == SIGKILL && stop))
		return -1;

	oldval = prismember(&P->sigmask, which)? TRUE : FALSE;

	if (stop) {	/* stop process on receipt of signal */
		if (!oldval) {
			praddset(&P->sigmask, which);
			P->setsig = TRUE;
		}
	} else {		/* let process continue on receipt of signal */
		if (oldval) {
			prdelset(&P->sigmask, which);
			P->setsig = TRUE;
		}
	}

	return oldval;
}

/* action on specified fault */
int
Pfault(process_t *P,		/* program table exit */
       int which,		/* fault number */
       int stop)		/* if TRUE, stop process; else let it go */
{
	register int oldval;

	if (which <= 0 || which > PRMAXFAULT)
		return -1;

	oldval = prismember(&P->faultmask, which)? TRUE : FALSE;

	if (stop) {	/* stop process on receipt of fault */
		if (!oldval) {
			praddset(&P->faultmask, which);
			P->setfault = TRUE;
		}
	} else {		/* let process continue on receipt of fault */
		if (oldval) {
			prdelset(&P->faultmask, which);
			P->setfault = TRUE;
		}
	}

	return oldval;
}

/* action on specified system call entry */
int
Psysentry(process_t *P,		/* program table entry */
	  int which,		/* system call number */
	  int stop)		/* if TRUE, stop process; else let it go */
{
	register int oldval;

	if (which <= 0 || which > PRMAXSYS)
		return -1;

	oldval = prismember(&P->sysentry, which)? TRUE : FALSE;

	if (stop) {	/* stop process on sys call */
		if (!oldval) {
			praddset(&P->sysentry, which);
			P->setentry = TRUE;
		}
	} else {		/* don't stop process on sys call */
		if (oldval) {
			prdelset(&P->sysentry, which);
			P->setentry = TRUE;
		}
	}

	return oldval;
}

/* action on specified system call exit */
int
Psysexit(process_t *P,		/* program table exit */
	 int which,		/* system call number */
	 int stop)		/* if TRUE, stop process; else let it go */
{
	register int oldval;

	if (which <= 0 || which > PRMAXSYS)
		return -1;

	oldval = prismember(&P->sysexit, which)? TRUE : FALSE;

	if (stop) {	/* stop process on sys call exit */
		if (!oldval) {
			praddset(&P->sysexit, which);
			P->setexit = TRUE;
		}
	} else {		/* don't stop process on sys call exit */
		if (oldval) {
			prdelset(&P->sysexit, which);
			P->setexit = TRUE;
		}
	}

	return oldval;
}


/* execute the syscall instruction */
int
execute(process_t *P,		/* process control structure */
	int sysindex)		/* system call index */
{
	sigset_t hold;		/* mask of held signals */
	int sentry;		/* old value of stop-on-syscall-entry */
	u_long sig;

	/* move current signal back to pending */
	if (P->why.pr_lwp.pr_cursig) {
		sig = 0;
		(void) Pctl(P, PCSSIG, &sig, sizeof sig);
		sig = P->why.pr_lwp.pr_cursig;
		(void) Pctl(P, PCKILL, &sig, sizeof sig);
		P->why.pr_lwp.pr_cursig = 0;
	}

	sentry = Psysentry(P, sysindex, TRUE);	/* set stop-on-syscall-entry */
	hold = P->why.pr_lwp.pr_context.uc_sigmask; /* remember signal mask */
	prfillset(&P->why.pr_lwp.pr_context.uc_sigmask); /* hold all signals */
	P->sethold = TRUE;

	if (Psetrun(P, 0, PRCSIG) == -1)
		goto bad;
	while (P->state == PS_RUN)
		(void) Pwait(P);

	if (P->state != PS_STOP)
		goto bad;
	P->why.pr_lwp.pr_context.uc_sigmask = hold; /* restore hold mask */
	P->sethold = TRUE;
	(void) Psysentry(P, sysindex, sentry);	/* restore sysentry stop */
	if (P->why.pr_lwp.pr_why  == PR_SYSENTRY &&
	    P->why.pr_lwp.pr_what == sysindex)
		return 0;
bad:
	return -1;
}

/* check syscall instruction in process */
int
checksyscall(process_t *P)
{
	/* this should always succeed--we always have a good syscall address */
	syscall_t instr;		/* holds one syscall instruction */

	return (Pread(P, P->sysaddr, &instr, sizeof instr) == sizeof instr &&
		instr == SYSCALL) ?
			0 : -1;
}

char *opname[] = {
	"",		"PCSTOP",	"PCDSTOP",	"PCWSTOP",
	"PCRUN",	"PCSTRACE",	"PCCSIG",	"PCKILL",
	"PCUNKILL",	"PCSHOLD",	"PCSFAULT",	"PCCFAULT",
	"PCSENTRY",	"PCSEXIT",	"PCSET",	"PCRESET",
	"PCSREG",	"PCSFPREG",	"PCNICE"
};

int nops = sizeof(opname)/sizeof(opname[0]);

/* Issue a control operation */
int
Pctl(process_t *P,			/* process */
     ulong_t op,			/* operation */
     void *argp,			/* pointer to operands */
     size_t arglen)			/* size in bytes of operands */
{
	int rc;

	if (debugflag) {
		int i, n;

		fprintf(stderr, "Pctl: ");
		if (0 < op && op < nops)
			fprintf(stderr, "%s", opname[op]);
		else
			fprintf(stderr, "op#%d", op);
		if (arglen) {
			fprintf(stderr, " (");
			n = arglen/sizeof(u_long);
			for (i = 0; i < n; i++)
				fprintf(stderr, "%s0x%08lX%s",
					i == 0 ? "" : " ",
					((ulong_t *)argp)[i],
					i == n-1 ? "" : ",");
			fprintf(stderr, ")");
		}
		fflush(stderr);
	}

	if (arglen > 0) {
		struct iovec vec[2];
		vec[0].iov_base = (void *)&op;
		vec[0].iov_len = sizeof op;
		vec[1].iov_base = argp;
		vec[1].iov_len = arglen;
		rc = (writev(P->ctlfd, vec, 2) == sizeof op + arglen) ? 0 : -1;
	} else
		rc = (write(P->ctlfd, &op, sizeof op) == sizeof op) ? 0 : -1;
	if (debugflag)
		fprintf(stderr, " returns %s\n",
			rc == 0 ? "SUCCESS" : "FAILURE");
	return rc;
}

int
Pset(process_t *P, ulong_t flags)	/* Set mode flags */
{
	ulong_t buf[2];

	buf[0] = PCSET;
	buf[1] = flags;
	return (write(P->ctlfd, buf, sizeof buf) == sizeof buf) ? 0 : -1;
}

int
Preset(process_t *P, ulong_t flags)	/* Reset mode flags */
{
	ulong_t buf[2];

	buf[0] = PCRESET;
	buf[1] = flags;
	return (write(P->ctlfd, buf, sizeof buf) == sizeof buf) ? 0 : -1;
}

int
Pcred(int pid, prcred_t *prcredp)
{
	int fd, rc, saverrno;
	extern int errno;
	char procname[100];

	(void) sprintf(procname, "%s/%d/cred", procdir, pid);
	if ((fd = open(procname, O_RDONLY)) == -1) {
		perror("Pcred open()");
		return -1;
	}
	saverrno = errno;
	rc = read(fd, (char *)prcredp, sizeof(*prcredp));
	close(fd);
	errno = saverrno;
	return rc;
}

/* test for empty set */
/* support routine used by isemptyset() macro */
int
is_empty(register const long * sp,	/* pointer to set (array of longs) */
	 register unsigned n)		/* number of longs in set */
{
	if (n) {
		do {
			if (*sp++)
				return FALSE;
		} while (--n);
	}

	return TRUE;
}
