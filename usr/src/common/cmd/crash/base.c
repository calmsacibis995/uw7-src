#ident	"@(#)crash:common/cmd/crash/base.c	1.1.1.1"
/* based on blf's OS5 crash/base.c 25.1 93/10/25 */

/*
 * This file contains code for the crash functions base and hexmode.
 */

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <sys/param.h>
#include "crash.h"

/*forward*/ static void	prnum(char *);

/* get arguments for function base */
getbase(void)
{
	int	c;

	optind = 1;
	while ((c = getopt(argcnt,args,"w:")) != EOF) {
		switch(c) {
		case 'w':
			redirect();
			break;
		default:
			longjmp(syn,0);
			/* NOTREACHED */
		}
	}
	if (optind >= argcnt) {
		longjmp(syn, 0);
		/* NOTREACHED */
	}
	for (;;) {
		prnum(args[optind]);
		if (++optind >= argcnt)
			return;
		putc('\n', fp);
	}
}

#define FIELD(T,x,n)	(*(((T *)&x)+(n)))
#define SWORD(x,n)	FIELD(short,(x),(n))
#define UWORD(x,n)	FIELD(unsigned short,(x),(n))
#define SBYTE(x,n)	FIELD(char,(x),(n))
#define UBYTE(x,n)	FIELD(unsigned char,(x),(n))

/* print results of function base */
static void
prnum(char *string)
{
	register int	i;
	long		num, t;
	static long	lbolt0time;

	num = strcon(string, 'd');

	(void) fputs(
		"RADIX:          LONGWORD    WORDS (LSW MSW)      BYTES (LSB .. MSB)\n",
		fp
	);

	(void) raw_fprintf(fp,
		"hex:        %#12lx    %#7hx %#7hx     %#4x %#4x %#4x %#4x\n",
		num,
		UWORD(num,0), UWORD(num,1),
		UBYTE(num,0), UBYTE(num,1), UBYTE(num,2), UBYTE(num,3)
	);
	(void) raw_fprintf(fp,
		"decimal:    %12ld    %7hd %7hd     %4d %4d %4d %4d\n",
		num,
		SWORD(num,0), SWORD(num,1),
		SBYTE(num,0), SBYTE(num,1), SBYTE(num,2), SBYTE(num,3)
	);
	(void) raw_fprintf(fp,
		"(unsigned)  %12lu    %7hu %7hu     %4u %4u %4u %4u\n",
		num,
		UWORD(num,0), UWORD(num,1),
		UBYTE(num,0), UBYTE(num,1), UBYTE(num,2), UBYTE(num,3)
	);
	(void) raw_fprintf(fp,
		"octal:      %#12lo    %#7ho %#7ho     %#4o %#4o %#4o %#4o\n",
		num,
		UWORD(num,0), UWORD(num,1),
		UBYTE(num,0), UBYTE(num,1), UBYTE(num,2), UBYTE(num,3)
	);

	(void) fputs("binary:    ",fp);
	for(i=0,t=num; i<32; i++,t<<=1)
		(void) putc('0'+(t<0), fp);

	(void) fputs("  ch:  ",fp);
	for (i = 0; i < 4; i++) {
		unsigned char ch = UBYTE(num,i);
		if (i)
			fputs("  ",fp);
		putch(ch);
	}
	(void) putc('\n', fp);

	if (lbolt0time == 0) {
		vaddr_t Time = symfindval("time");
		vaddr_t Lbolt = symfindval("lbolt");
		long time, lbolt;

		readmem(Time, 1, -1, &time, sizeof(time), "time");
		readmem(Lbolt, 1, -1, &lbolt, sizeof(lbolt), "lbolt");
		lbolt0time = time - ((unsigned long)lbolt)/HZ;
	}

	(void) raw_fprintf(fp, "date/time: %s  ", date_time(num));
	(void) raw_fprintf(fp, "lbolt: %s\n", date_time(lbolt0time +
			+ ((unsigned long)num)/HZ));
}

/* get arguments for function hexmode */
gethexmode(void)
{
	int	c;

	optind = 1;
	while ((c = getopt(argcnt,args,"w:")) != EOF) {
		switch(c) {
		case 'w':
			redirect();
			break;
		default:
			longjmp(syn,0);
		}
	}

	if (args[optind]) {
		if (args[optind+1])
			longjmp(syn,0);
		if (strcmp(args[optind], "on") == 0)
			hexmode = 1;
		else if (strcmp(args[optind], "off") == 0)
			hexmode = 0;
		else
			longjmp(syn,0);
	}

	raw_fprintf(fp, "hexmode is %s\n", hexmode?
	"on: commands expect and display hexadecimal":
	"off: commands expect and display decimal or octal or hexadecimal");
}
