/*		copyright	"%c%" 	*/

#ident	"@(#)truss:common/cmd/truss/actions.c	1.5.9.1"
#ident  "$Header$"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "print.h"
#include "proto.h"
#include "machdep.h"

/*
 * Actions to take when process stops.
 */

/* This module is carefully coded to contain only read-only data */
/* All read/write data is defined in ramdata.c (see also ramdata.h) */

/*
 * Function prototypes for static routines in this module.
 */

static	int	ptraced(process_t *);
static	int	stopsig(process_t *);


/*
 * requested() gets called for these reasons:
 *	flag == PTRACED:	report "Continued with ..."
 *	flag == JOBSIG:		report nothing; change state to JOBSTOP
 *	flag == JOBSTOP:	report "Continued ..."
 *	default:		report sleeping system call
 *
 * It returns a new flag:  JOBSTOP or SLEEPING or 0.
 */
int
requested(process_t *Pr, int flag)
{
	int sig = Pr->why.pr_lwp.pr_cursig;
	int sys;
	int newflag = 0;

	switch (flag) {
	case JOBSIG:
		return JOBSTOP;

	case PTRACED:
	case JOBSTOP:
		if (sig > 0 && sig <= PRMAXSIG && prismember(&signals, sig)) {
			length = 0;
			(void) printf("%s    Continued with signal #%d, %s",
				pname, sig, signame(sig));
			if (Pr->why.pr_lwp.pr_action.sa_handler == SIG_DFL)
				(void) printf(" [default]");
			else if (Pr->why.pr_lwp.pr_action.sa_handler == SIG_IGN)
				(void) printf(" [ignored]");
			else
				(void) printf(" [caught]");
			(void) fputc('\n', stdout);
		}
		break;

	default:
		if (!(Pr->why.pr_lwp.pr_flags & PR_ASLEEP)
		 || (sys = Pgetsysnum(Pr)) <= 0 || sys > PRMAXSYS)
			break;

		newflag = SLEEPING;

		/* Make sure we catch sysexit even if we're not tracing it. */
		if (!prismember(&trace, sys))
			(void) Psysexit(Pr, sys, TRUE);
		else if (!cflag) {
			length = 0;
			Pr->why.pr_lwp.pr_what = sys;	/* cheating a little */
			Errno = 0;
			Rval1 = Rval2 = 0;
			(void) sysentry(Pr);
			if (pname[0])
				(void) fputs(pname, stdout);
			length += printf("%s", sys_string);
			sys_leng = 0;
			*sys_string = '\0';
			length >>= 3;
			if (length >= 4)
				(void) fputc(' ', stdout);
			for ( ; length < 4; length++)
				(void) fputc('\t', stdout);
			(void) fputs("(sleeping...)\n", stdout);
			length = 0;
		}
		break;
	}

	if ((flag = jobcontrol(Pr)) != 0)
		newflag = flag;

	return newflag;
}

static int
ptraced(process_t *Pr)
{
	register int sig = Pr->why.pr_lwp.pr_what;

	if (!(Pr->why.pr_flags & PR_PTRACE) ||
	    Pr->why.pr_lwp.pr_why != PR_SIGNALLED ||
	    sig != Pr->why.pr_lwp.pr_cursig ||
	    sig <= 0 || sig > PRMAXSIG)
		return 0;

	if (!cflag && prismember(&signals, sig)) {
		register int sys;

		length = 0;
		(void) printf("%s    Stopped on signal #%d, %s",
			pname, sig, signame(sig));
		if ((Pr->why.pr_lwp.pr_flags & PR_ASLEEP) &&
		    (sys = Pgetsysnum(Pr)) > 0 && sys <= PRMAXSYS) {
			int code;
			int nabyte;
			ulong_t ap = getargp(Pr, &nabyte);

			if (nabyte < sizeof code ||
			    Pread(Pr, ap, &code, sizeof code) != sizeof code)
				code = -1;

			(void) printf(", in %s()", sysname(sys, code));
		}
		(void) fputc('\n', stdout);
	}

	return PTRACED;
}

int
jobcontrol(process_t *Pr)
{
	register int sig = stopsig(Pr);

	if (sig == 0)
		return 0;

	if (!cflag &&				/* not just counting */
	    prismember(&signals, sig)) {	/* tracing this signal */
		register int sys;

		length = 0;
		(void) printf("%s    Stopped by signal #%d, %s",
			pname, sig, signame(sig));
		if ((Pr->why.pr_lwp.pr_flags & PR_ASLEEP) &&
		    (sys = Pgetsysnum(Pr)) > 0 &&
		    sys <= PRMAXSYS) {
			int code;
			int nabyte;
			ulong_t ap = getargp(Pr, &nabyte);

			if (nabyte < sizeof code ||
			    Pread(Pr, ap, &code, sizeof code) != sizeof code)
				code = -1;

			(void) printf(", in %s()", sysname(sys, code));
		}
		(void) fputc('\n', stdout);
	}

	return JOBSTOP;
}

/*
 * Return the signal the process stopped on iff process is already stopped on
 * PR_JOBCONTROL or is stopped on PR_SIGNALLED or PR_REQUESTED with a current
 * signal that will cause a JOBCONTROL stop when the process is set running.
 */
static int
stopsig(process_t *Pr)
{
	register int sig = 0;

	if (Pr->state == PS_STOP) {
		switch (Pr->why.pr_lwp.pr_why) {
		case PR_JOBCONTROL:
			sig = Pr->why.pr_lwp.pr_what;
			if (sig < 0 || sig > PRMAXSIG)
				sig = 0;
			break;
		case PR_SIGNALLED:
		case PR_REQUESTED:
			if (Pr->why.pr_lwp.pr_action.sa_handler == SIG_DFL) {
				switch (Pr->why.pr_lwp.pr_cursig) {
				case SIGSTOP:
				case SIGTSTP:
				case SIGTTIN:
				case SIGTTOU:
					sig = Pr->why.pr_lwp.pr_cursig;
					break;
				}
			}
			break;
		}
	}

	return sig;
}

int
signalled(process_t *Pr)
{
	int sig = Pr->why.pr_lwp.pr_what;
	int flag = 0;

	if (sig <= 0 || sig > PRMAXSIG)	/* check bounds */
		return 0;

	if (cflag)			/* just counting */
		Cp->sigcount[sig]++;

	if ((flag = ptraced(Pr)) == 0 &&
	    (flag = jobcontrol(Pr)) == 0 &&
	    !cflag &&
	    prismember(&signals, sig)) {
		int sys;

		length = 0;
		(void) printf("%s    Received signal #%d, %s",
			pname, sig, signame(sig));
		if ((Pr->why.pr_lwp.pr_flags & PR_ASLEEP) &&
		    (sys = Pgetsysnum(Pr)) > 0 &&
		    sys <= PRMAXSYS) {
			int code;
			int nabyte;
			ulong_t ap = getargp(Pr, &nabyte);

			if (nabyte < sizeof code ||
			    Pread(Pr, ap, &code, sizeof code) != sizeof code)
				code = -1;

			(void) printf(", in %s()", sysname(sys, code));
		}
		if (Pr->why.pr_lwp.pr_action.sa_handler == SIG_DFL)
			(void) printf(" [default]");
		else if (Pr->why.pr_lwp.pr_action.sa_handler == SIG_IGN)
			(void) printf(" [ignored]");
		else
			(void) printf(" [caught]");
		(void) fputc('\n', stdout);
		if (Pr->why.pr_lwp.pr_info.si_code != 0 ||
		    Pr->why.pr_lwp.pr_info.si_pid != 0)
			print_siginfo(&Pr->why.pr_lwp.pr_info);
	}

	if (flag == JOBSTOP)
		flag = JOBSIG;
	return flag;
}

void
faulted(process_t *Pr)
{
	int flt = Pr->why.pr_lwp.pr_what;

	if ((unsigned)flt > PRMAXFAULT)	/* check bounds */
		flt = 0;
	Cp->fltcount[flt]++;

	if (cflag)		/* just counting */
		return;
	length = 0;
	(void) printf("%s    Incurred fault #%d, %s  %%pc = 0x%.8X",
		pname, flt, fltname(flt), Pr->REG[R_PC]);
	if (flt == FLTPAGE)
		(void) printf("  addr = 0x%.8X", Pr->why.pr_lwp.pr_info.si_addr);
	(void) fputc('\n', stdout);
	if (Pr->why.pr_lwp.pr_info.si_signo != 0)
		print_siginfo(&Pr->why.pr_lwp.pr_info);
}

/* return TRUE iff syscall is being traced */
int
sysentry(process_t *Pr)
{
	int arg;
	int nabyte;
	int nargs;
	int i;
	int x;
	int len;
	char * s;
	ulong_t ap;
	register const struct systable *stp;
	int what = Pr->why.pr_lwp.pr_what;
	int subcode = -1;
	int istraced;
	int raw;
	int argprinted;

	/* protect ourself from operating system error */
	if (what <= 0 || what > PRMAXSYS)
		what = 0;

	/* get systable entry for this syscall */
	stp = subsys(what, -1);

	/* get address of argument list + number of bytes of arguments */
	ap = getargp(Pr, &nabyte);

	if (nabyte > sizeof sys_args)
		nabyte = sizeof sys_args;

	nargs = nabyte / sizeof (int);
	if (nargs > stp->nargs)
		nargs = stp->nargs;
	sys_nargs = nargs;
	nabyte = nargs * sizeof (int);

	if (stp->nargs)
		(void)memset(sys_args, 0, (int)(stp->nargs*sizeof (int)));

	if (nabyte > 0 &&
	    (i = Pread(Pr, ap, sys_args, nabyte)) != nabyte) {
		(void) printf("%s\t*** Bad argument pointer: 0x%.8X ***\n",
			pname, ap);
		if (i < 0)
			i = 0;
		sys_nargs = nargs = i / sizeof (int);
		nabyte = nargs * sizeof (int);
	}

	if (nargs > 0) {	/* interpret syscalls with sub-codes */
		subcode = sys_args[0];
		switch (what) {
		case SYS_utssys:
			/* For SYS_utssys, subcode is 3rd arg */
			if (sys_nargs > 2)
				subcode = sys_args[2];
			break;
		case SYS_xenix:
			/* For SYS_xenix, subcode is in pr_syscall */
			subcode = Pr->why.pr_lwp.pr_syscall >> 8;
			break;
		}
		stp = subsys(what, subcode);
	}
	if (nargs > stp->nargs)
		nargs = stp->nargs;
	sys_nargs = nargs;

	/* fetch and remember first argument if it's a string */
	sys_valid = FALSE;
	if (nargs > 0
	 && stp->arg[0] == STG
	 && (s = fetchstring((long)sys_args[0], 400)) != NULL) {
		sys_valid = TRUE;
		len = strlen(s);
		while (len >= sys_psize) {	/* reallocate if necessary */
			free(sys_path);
			sys_path = malloc(sys_psize *= 2);
			if (sys_path == NULL)
				abend("cannot allocate pathname buffer", 0);
		}
		(void) strcpy(sys_path, s);	/* remember pathname */
	}

	istraced = prismember(&trace, what);
	raw = prismember(&rawout, what);

	/* force tracing of read/write buffer dump syscalls */
	if (!istraced && nargs > 2) {
		int fdp1 = sys_args[0] + 1;

		switch (what) {
		case SYS_read:
		case SYS_readv:
		case SYS_pread:
			if (prismember(&readfd, fdp1))
				istraced = TRUE;
			break;
		case SYS_write:
		case SYS_writev:
		case SYS_pwrite:
			if (prismember(&writefd, fdp1))
				istraced = TRUE;
			break;
		}
	}

	/* initial argument index, skipping over hidden arguments */
	for (i = 0; !raw && i < nargs && stp->arg[i] == HID; i++)
		;

	sys_leng = 0;
	if (cflag || !istraced)		/* just counting */
		*sys_string = 0;
	else if (i >= nargs)
		sys_leng = sprintf(sys_string, "%s()",
			sysname(what, raw? -1 : subcode));
	else {
		sys_leng = sprintf(sys_string, "%s(",
			sysname(what, raw? -1 : subcode));
		argprinted = 0;
		for (;;) {
			arg = sys_args[i];
			x = stp->arg[i];

			if (x == STG && !raw &&
			    i == 0 && sys_valid) {	/* already fetched */
				outstring("\"");
				outstring(sys_path);
				outstring("\"");
				argprinted = 1;
			} else {
				if (argprinted)
					outstring(", ");
				argprinted = (*Print[x])(arg, raw);
			}

			if (++i >= nargs)
				break;
		}
		outstring(")");
	}

	return istraced;
}

void
sysexit(process_t *Pr)
{
	int r0;
	int r1;
	int ps;
	int what = Pr->why.pr_lwp.pr_what;
	register const struct systable *stp;
	int arg0;
	int istraced;
	int raw;

	/* protect ourself from operating system error */
	if (what <= 0 || what > PRMAXSYS)
		what = 0;

	/*
	 * If we aren't supposed to be tracing this one, then
	 * delete it from the traced signal set.  We got here
	 * because the process was sleeping in an untraced syscall.
	 */
	if (!prismember(&traceeven, what)) {
		(void) Psysexit(Pr, what, FALSE);
		return;
	}

	/* get systable entry for this syscall */
	stp = subsys(what, -1);

	/* pick up registers & set Errno before anything else */

	(void) Pgetareg(Pr, R_0); r0 = Pr->REG[R_0];
	(void) Pgetareg(Pr, R_1); r1 = Pr->REG[R_1];
	(void) Pgetareg(Pr, R_PS); ps = Pr->REG[R_PS];

	Errno = (ps & ERRBIT)? r0 : 0;
	Rval1 = r0;
	Rval2 = r1;

	switch (what) {
	case SYS_exit:		/* these are traced on entry */
	case SYS_exec:
	case SYS_execve:
	case SYS_evtrapret:
	case SYS_context:
		istraced = prismember(&trace, what);
		break;
	default:
		istraced = sysentry(Pr);
		length = 0;
		if (!cflag && istraced) {
			if (pname[0])
				(void) fputs(pname, stdout);
			length += printf("%s", sys_string);
		}
		sys_leng = 0;
		*sys_string = '\0';
		break;
	}

	if (istraced) {
		Cp->syscount[what]++;
		accumulate(&Cp->systime[what], &Pr->why.pr_stime, &syslast);
	}
	syslast = Pr->why.pr_stime;
	usrlast = Pr->why.pr_utime;

	arg0 = sys_args[0];

	if (!cflag && istraced) {
		if ((what == SYS_fork || what == SYS_vfork ||
		     what == SYS_fork1 || what == SYS_forkall)
		 && Errno == 0 && r1 != 0) {
			length &= ~07;
			length += 14 + printf("\t\t(returning as child ...)");
		}
		if (Errno != 0 || (what != SYS_exec && what != SYS_execve)) {
			/* prepare to print the return code */
			length >>= 3;
			if (length >= 6)
				(void) fputc(' ', stdout);
			for ( ; length < 6; length++)
				(void) fputc('\t', stdout);
		}
	}
	length = 0;

	if (sys_nargs > 0) {		/* interpret syscalls with sub-codes */
		int subcode = arg0;
		switch (what) {
		case SYS_utssys:
			/* For SYS_utssys, subcode is 3rd arg */
			if (sys_nargs > 2)
				subcode = sys_args[2];
			break;
		case SYS_xenix:
			/* For SYS_xenix, subcode is in pr_syscall */
			subcode = Pr->why.pr_lwp.pr_syscall >> 8;
			break;
		}
		stp = subsys(what, subcode);
	}

	raw = prismember(&rawout, what);

	if (Errno != 0) {		/* error in syscall */
		if (istraced) {
			Cp->syserror[what]++;
			if (!cflag) {
				const char * ename = errname(r0);

				(void) printf("Err#%d", r0);
				if (ename != NULL) {
					if (r0 < 10)
						(void) fputc(' ', stdout);
					(void) fputc(' ', stdout);
					(void) fputs(ename, stdout);
				}
				(void) fputc('\n', stdout);
			}
		}
	}
	else {
		/* show arguments on successful exec */
		if (what == SYS_exec || what == SYS_execve) {
			if (!cflag && istraced)
				showargs(Pr, raw);
		}
		else if (!cflag && istraced) {
			outstring("= ");
			(void) (*Print[stp->rval[0]])(r0, raw);
			(void) (*Print[stp->rval[1]])(r1, raw);
			(void) printf("%s\n", sys_string);
			sys_leng = 0;
			*sys_string = '\0';
		}

		if (what == SYS_fork || what == SYS_vfork ||
		    what == SYS_fork1 || what == SYS_forkall) {
			if (r1 == 0)		/* child was created */
				child = r0;
			else if (istraced)	/* this is the child */
				Cp->syscount[what]--;
		}
	}

	if (!cflag && istraced) {
		int fdp1 = arg0+1;	/* read()/write() filedescriptor + 1 */

		if (raw) {
			showpaths(stp);
			if ((what == SYS_read || what == SYS_pread ||
			     what == SYS_write || what == SYS_pwrite)
			 && iob_buf[0] != '\0')
				(void) printf("%s     0x%.8X: %s\n",
					pname, sys_args[1], iob_buf);
		}

		/*
		 * Show buffer contents for read() or write().
		 * IOBSIZE bytes have already been shown;
		 * don't show them again unless there's more.
		 */
		if (((what==SYS_read || what==SYS_pread) &&
		     Errno==0 && prismember(&readfd,fdp1)) ||
		    ((what==SYS_write || what==SYS_pread) &&
		     prismember(&writefd,fdp1))) {

			int nb = (what==SYS_write || what==SYS_pwrite) ?
				sys_args[2] : r0;

			if (nb > IOBSIZE) {
				/* enter region of lengthy output */
				if (nb > BUFSIZ/4)
					Eserialize();

				showbuffer(Pr, (long)sys_args[1], nb);

				/* exit region of lengthy output */
				if (nb > BUFSIZ/4)
					Xserialize();
			}
		}

		/*
		 * Do verbose interpretation if requested.
		 * If buffer contents for read or write have been requested and
		 * this is a readv() or writev(), force verbose interpretation.
		 */
		if (prismember(&verbose, what) ||
		    (what==SYS_readv && Errno==0 && prismember(&readfd,fdp1)) ||
		    (what==SYS_writev && prismember(&writefd,fdp1)))
		    expound(Pr, r0, raw);
	}
}

void
showpaths(const struct systable * stp)
{
	int i;

	for (i = 0; i < sys_nargs; i++) {
		if ((stp->arg[i] == STG) ||
		    (stp->arg[i] == RST && !Errno) ||
		    (stp->arg[i] == RLK && !Errno && Rval1 > 0)) {
			long addr = (long)sys_args[i];
			int maxleng = (stp->arg[i]==RLK)? Rval1 : 400;
			char * s;

			if (i == 0 && sys_valid)	/* already fetched */
				s = sys_path;
			else
				s = fetchstring(addr, maxleng>400?400:maxleng);

			if (s)
				(void)printf("%s     0x%.8X: \"%s\"\n",
					     pname, addr, s);
		}
	}
}

void
dumpargs(process_t *Pr, ulong_t ap, const char * str)
{
	char * string;
	register int leng = 0;
	char * arg;
	char badaddr[32];

	if (interrupt)
		return;

	if (pname[0])
		(void) fputs(pname, stdout);
	(void) fputc(' ', stdout);
	(void) fputs(str , stdout);
	leng += 1 + strlen(str);
	while (!interrupt) {
		if (Pread(Pr, ap, &arg, sizeof arg) != sizeof arg) {
			(void) printf("\n%s\t*** Bad argument list? ***\n",
				      pname);
			return;
		}
		ap += sizeof arg;
		if (arg == (char *)NULL)
			break;
		string = fetchstring((long)arg, 400);
		if (string == NULL) {
			(void) sprintf(badaddr, "BadAddress:0x%.8X", arg);
			string = badaddr;
		}
		if ((leng += (int)strlen(string)) < 71) {
			(void) fputc(' ', stdout);
			leng++;
		}
		else {
			(void) fputc('\n', stdout);
			leng = 0;
			if (pname[0])
				(void) fputs(pname, stdout);
			(void) fputs("  ", stdout);
			leng += 2 + strlen(string);
		}
		(void) fputs(string, stdout);
	}
	(void) fputc('\n', stdout);
}

/* display contents of read() or write() buffer */
void
showbuffer(process_t *Pr, long offset, int count)
{
	char buffer[320];
	int nbytes;
	register char * buf;
	register int n;

	while (count > 0 && !interrupt) {
		nbytes = (count < sizeof buffer) ? count : sizeof buffer;
		if ((nbytes = Pread(Pr, offset, buffer, nbytes)) <= 0)
			break;
		count -= nbytes;
		offset += nbytes;
		buf = buffer;
		while (nbytes > 0 && !interrupt) {
			char obuf[65];

			n = (nbytes < 32)? nbytes : 32;
			showbytes(buf, n, obuf);

			if (pname[0])
				(void) fputs(pname, stdout);
			(void) fputs("  ", stdout);
			(void) fputs(obuf, stdout);
			(void) fputc('\n', stdout);
			nbytes -= n;
			buf += n;
		}
	}
}

void
showbytes(const char * buf, int n, char * obuf)
{
	register int c;

	while (--n >= 0) {
		register int c1 = '\\';
		register int c2;

		switch (c = (*buf++ & 0xff)) {
		case '\0':
			c2 = '0';
			break;
		case '\b':
			c2 = 'b';
			break;
		case '\t':
			c2 = 't';
			break;
		case '\n':
			c2 = 'n';
			break;
		case '\v':
			c2 = 'v';
			break;
		case '\f':
			c2 = 'f';
			break;
		case '\r':
			c2 = 'r';
			break;
		default:
			if (isprint(c)) {
				c1 = ' ';
				c2 = c;
			} else {
				c1 = c>>4;
				c1 += (c1 < 10)? '0' : 'A'-10;
				c2 = c&0xf;
				c2 += (c2 < 10)? '0' : 'A'-10;
			}
			break;
		}
		*obuf++ = c1;
		*obuf++ = c2;
	}

	*obuf = '\0';
}
