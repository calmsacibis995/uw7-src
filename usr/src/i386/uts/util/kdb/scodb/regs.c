#ident	"@(#)kern-i386:util/kdb/scodb/regs.c	1.1.1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 * Modification History
 *
 *	L000	scol!nadeem	19may92
 *	- use crllry_index() instead of scodb_procnum().  Also removed
 *	  definition of scodb_procnum() from install.d/scodb_mpx.i.
 *	L001	scol!nadeem	2jul92
 *	- fix bug whereby the special registers were not being displayed
 *	  correctly on different cpus.  The ldsregs() routine was
 *	  not bothering to reload the values for gdtr, ldtr, idtr, cr0,
 *	  cr1, and cr3 after having loaded these once.  This means that
 *	  the first time you do an 'r' command, those registers will
 *	  always be displayed with the same values, no matter what cpu
 *	  you are subsequently running on.
 *	L002	nadeem		19oct92
 *	- fix the display of the 'gdtr' and 'idtr' registers (see also sregs.s).
 *	L003	nadeem		22dec94
 *	- support for user level scodb (under #ifdef USER_LEVEL):
 *	- replace dereferences on "u." with "upage.", which a global copy of the
 *	  U-page from the dump image,
 *	- when displaying the registers in c_regdisp(), leave out the special
 *	  registers (ie: CR0 etc) as these cannot be read from user mode,
 *	- when the dump image is /dev/mem, then only the "r -p <pid_or_proc>"
 *	  form of register display command is valid.  Reading the registers
 *	  of the current process when not in kernel mode does not make sense.
 *	  On the other hand, the registers of any other sleeping processes can
 *	  be read from their TSS's,
 *	- if the dump image is a crash dump, and scodb was not able to find
 *	  the registers in it, then fail the register dump command,
 */

/*
#include	"sys/types.h"
#include	"sys/seg.h"
#include	"sys/param.h"
#include	"sys/signal.h"
#include	"sys/user.h"
#include	"sys/xdebug.h"
*/

#include	"sys/reg.h"
#include 	"proc/tss.h"
#include	"dbg.h"
#include	"sent.h"
#include	"histedit.h"

#define		NRPL	5

/*
*	Field width:
*		__		two spaces
*		rrrr		four characters of register name
*		=		an equal sign
*		vvvvvvvv	value of register
*/
#define		FLDW	(2+4+1)

STATIC int *regp;
NOTSTATIC int sregs[NSPECREG];
STATIC int *sregp = sregs;
int scodb_nregl;		/* number of lines for register display */
int scodb_regdisp = 0;		/* type of register display */

extern char *regnames[];
extern char *sregnames[];
extern char *symname();

struct regd {
	int	  rd_r;
	int	  rd_w;
	int	**rd_p;
	char	**rd_s;
	int	toffset;	/* whether rd_r is a reg trap frame offset */
};

#define		R(rg)	{ rg, 8, &regp,  regnames,  1 },
#define		RS(rg)	{ rg, 4, &regp,  regnames,  1 },
#define		S(rg)	{ rg, 8, &sregp, sregnames, 0 },
#define		NONE	{ 0,  8, 0,      0,         0 },
#define		NONES	{ 0,  4, 0,      0,         0 },

/*
*	can't use the REGP[SS], cuz it's not necessarily on the stack!
*/
#define		X_SS		T_DS
/*
*	the difference in printing between the user and kernel
*	registers is that the user ones has a U_SP, the kernel a
*	K_SP
*/
#define		SP_L	10
#define		K_SP	T_ESP
#define		U_SP	T_UESP

#ifdef USER_LEVEL						/* L003v */

extern struct user upage;
extern int regp_valid;		/* does the REGP array contain the contents
				   of the kernel registers? */
extern int k_processor_index;	/* kernel "processor_index" variable */
extern int devmem_flag;		/* /dev/mem used as dump image? */

/*
 * Can't display special registers from user level scodb.
 */

STATIC struct regd regd[] = {
	R(EIP)	R(EAX)	RS(CS)	NONE	NONE
	R(EBP)	R(EBX)	RS(DS)	NONE	NONE
	R(0)	R(ECX)	RS(ES)	NONE	NONE
	R(ESI)	R(EDX)	RS(FS)	NONE	NONE
	R(EDI)	NONE	RS(GS)	NONE	NONE
	R(EFL)	NONE	RS(SS)	S(PROC)	R(TRAPNO)
};

#else								/* L003^ */

/* TBD - FS, GS, SS */

STATIC struct regd regd[] = {
	R(T_EIP)  R(T_EAX)  RS(T_CS)	S(LDTR)	S(CR0)
	R(T_EBP)  R(T_EBX)  RS(T_DS)	S(GDTR)	S(CR1)
	R(0)	  R(T_ECX)  RS(T_ES)	S(IDTR)	S(CR2)
	R(T_ESI)  R(T_EDX)  NONES	S(TR)	S(CR3)
	R(T_EDI)  NONE	    NONES	NONE	NONE
	R(T_EFL)  NONE	    NONES	S(PROC)	R(T_TRAPNO)
};

#endif								/* L003 */

/*
*	usage:
*		[rR]			dump registers for cur proc
*		[rR] addr		dump registers at `addr'
*		[rR] -p pid		dump registers for pid
*/
NOTSTATIC
c_regdisp(c, v)
	int c;
	char **v;
{
	int *rp, isk;
	long seg, pid;
	int prregs[NSYSREG];
	extern int *REGP;
	extern char *regnames[], *scodb_error;

	if (**v == 'R') {	/* use user regs */
		/* TBD */
		return DB_ERROR;
	} else {			/* use kernel regs */
		rp = REGP;
		isk = 1;
	}

	if (c == 3 && !strcmp(v[1], "-p")) {
		/* TBD */
		return DB_ERROR;
	} else
	if (c == 2 && !strcmp(v[1], "-t")) {
		scodb_regdisp = !scodb_regdisp;
	}
	else if (c > 1) {
		seg = KDSSEL;
		if (!getaddrv(v + 1, &seg, &rp)) {
			perr();
			return DB_ERROR;
		} 
		if (!validaddr(getlinear(seg, rp))) {
			badaddr(seg, rp);
			return DB_ERROR;
		}
	}

#ifdef USER_LEVEL						/* L003v */
	if (c != 3 && !regp_valid) {
		printf("Could not retrieve the kernel registers.\n");
		return DB_ERROR;
	}
#endif								/* L003^ */

	if (rp == 0 && !isk) {
		printf("not a user process.\n");
		return DB_ERROR;
	}
	dpregs(rp, isk);
	return DB_CONTINUE;
}

NOTSTATIC
ldsregs() {
#ifndef USER_LEVEL						/* L003 */
	sregs[CR2]	= _cr2();
	sregs[TR]	= get_tr();
	sregs[PROC]	= crllry_index();			/* L000 */
	sregs[LDTR]	= gldt();
	sregs[GDTR]	= ggdt();				/* L002 */
	sregs[IDTR]	= gidt();				/* L002 */
	sregs[CR0]	= _cr0();
	sregs[CR1]	= 0;	/* cr1 doesn't really exist to us */
	sregs[CR3]	= _cr3();
#else								/* L003v */

	/*
	 * The user level scodb cannot read the special registers from
	 * either the dump image or /dev/mem.
	 */

	sregs[PROC]	= 0;
#endif								/* L003^ */
}

NOTSTATIC
dpregs(rp, isk)
	int *rp, isk;
{
	regp = rp;
	if (isk)
		regd[SP_L].rd_r = K_SP;
	else
		regd[SP_L].rd_r = U_SP;

	switch (scodb_regdisp) {
	case 0:
		dregs0();
		break;
	case 1:
		dregs();
		break;
	}
}

dregs0()
{
	int i, col, val;
	char *s;

	scodb_nregl = 5;

	printf("  eip=");
	pnz(regp[T_EIP], 8);
	printf("   eax=");
	pnz(regp[T_EAX], 8);
	printf("   edx=");
	pnz(regp[T_EDX], 8);
	printf("   efl=");
	pnz(regp[T_EFL], 8);
	printf(" ");
	decode_flags(regp[T_EFL]);

	printf("\n  ebp=");
	pnz(regp[T_EBP], 8);
	printf("   ebx=");
	pnz(regp[T_EBX], 8);
	printf("   esi=");
	pnz(regp[T_ESI], 8);

	printf("\n  esp=");
	pnz(regp[T_ESP], 8);
	printf("   ecx=");
	pnz(regp[T_ECX], 8);
	printf("   edi=");
	pnz(regp[T_EDI], 8);

	printf("\n\nesp->");
	col = 6;		/* current column (starting from one) */

	/*
	 * Display as much of the stack as will fit on the line
	 */

	for (i = 0 ; ; i++) {
		db_getlong(KDSSEL, (char *) regp[T_ESP] + (i*4), &val);
		if ((s = symname(val, 2))) {
			col += strlen(s) + 2;
			if (col >= MAXCOL)
				break;
			printf("  %s", s);
		} else {
			col += 8 + 2;
			if (col >= MAXCOL)
				break;
			printf("  ");
			pnz(val, 8);
		}
	}

	/*
	 * Blank out the remainder of the line
	 */

	clrtoeol();

	printf("\n");
}

STATIC
decode_flags(unsigned flags)
{
	char *flag_names = "SZ-A-P-C";
	unsigned bit;
	int i;

	bit = 0x80;
	for (i = 0 ; i < 8 ; i++, bit >>= 1) {
		if (flags & bit)
			putchar(flag_names[i]);
		else
			putchar('-');
	}
}

/*
*	string length, to a max of 4
*/
#define		SL(s)	(s[1] ? (s[2] ? (s[3] ? 4 : 3) : 2) : 1)
STATIC
dregs() {
	register int i, l;
	register char *s;
	register struct regd *rd = regd;

	scodb_nregl = 6;
	ldsregs();
	for (i = 0;i < NMEL(regd);i++, rd++) {
		if ((i % NRPL) == 0)
			printf("    ");
		if (rd->rd_s) {
			putchar(' ');
			putchar(' ');
			if (rd->toffset)
				s = rd->rd_s[T_REGNAME(rd->rd_r)];
			else
				s = rd->rd_s[rd->rd_r];

			/*
			*	make field width 4 characters.
			*/
			switch (SL(s)) {
				case 1:
					putchar(' ');
				case 2:
					putchar(' ');
				case 3:
					putchar(' ');
				case 4:
					;
			}
			while (*s)
				putchar(*s++);
			putchar('=');
			pnz((*rd->rd_p)[rd->rd_r == T_SS ? X_SS : rd->rd_r], rd->rd_w);
		}
		else {
			for (l = 0;l < (FLDW+rd->rd_w);l++)
				putchar(' ');
		}
		if (((i + 1) % NRPL) == 0)
			putchar('\n');
	}
}

vtoregs()
{
	return(0);
}
