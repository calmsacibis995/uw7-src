#ident	"@(#)truss:i386/cmd/truss/args.c	1.1.2.1"
#ident	"$Header$"

#include <stdio.h>

#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"
#include "machdep.h"

/* display arguments to successful exec() */
void
showargs(process_t *Pr, int raw)
{
	int nargs;
	ulong_t ap;
	ulong_t sp;

	length = 0;

	Pgetareg(Pr, R_SP); ap = Pr->REG[R_SP]; /* UESP */

	if (Pread(Pr, ap, &nargs, sizeof nargs) != sizeof nargs) {
		printf("\n%s\t*** Bad argument list? ***\n", pname);
		return;
	}
#if 0
	if (debugflag) {
		int i, n, stack[256];
		n = 0x7fffffff - ap;
		if ( n > 1024 ) n = 1024;
		fprintf(stderr, "ap = 0x%x, nargs = %d, stacksize = %d\n",
						ap, nargs, n);
		Pread(Pr, ap, stack, n);
		for ( i = 0 ; i < 256 ; i++ ) {
			if ( (n -= 4) < 0 )
				break;
			fprintf(stderr, "%08x:	%8x\n", ap + 4 * i, stack[i]);
		}
	}
#endif

	(void) printf("  argc = %d\n", nargs);
	if (raw)
		showpaths(&systable[SYS_exec]);

	show_cred(Pr, FALSE);

	if (aflag || eflag) {		/* dump args or environment */

		/* enter region of (potentially) lengthy output */
		Eserialize();

		ap += sizeof (int);

		if (aflag)		/* dump the argument list */
			dumpargs(Pr, ap, "argv:");

		ap += (nargs+1) * sizeof (char *);

		if (eflag)		/* dump the environment */
			dumpargs(Pr, ap, "envp:");

		/* exit region of lengthy output */
		Xserialize();
	}
}

/* get address of arguments to syscall */
/* also return # of bytes of arguments */
ulong_t
getargp(process_t *Pr, int *nbp)
{
	ulong_t ap, sp;
	int nabyte;

	(void) Pgetareg(Pr, R_SP); sp = Pr->REG[R_SP];
	ap = sp + sizeof (int);
	nabyte = 512;
	if (nbp != NULL)
		*nbp = nabyte;
	return ap;
}
