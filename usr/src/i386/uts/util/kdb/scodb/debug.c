#ident	"@(#)kern-i386:util/kdb/scodb/debug.c	1.1.1.2"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1995.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Modification History:
 *	L000	scol!nadeem	23apr92
 *	- implement scol!hughd's idea of a command which dumps memory
 *	  in symbol form for easy analysis of stack frames.  Command
 *	  is "dn <address>".
 *	L001	scol!nadeem	2jul92
 *	- support for highlighting frame pointers on "dn" command.  Clear
 *	  the saved frame pointers when exiting the debugger.
 *	- corrected the "[yes]" and "[no]" messages displayed during single
 *	  stepping which tell the user whether a conditional jump instruction
 *	  is going to be taken or not.  There were two bugs here:
 *		- the "jle" logic was faulty (corrected in this module),
 *		- the F_ZF, F_CF etc. macros cannot be compared together
 *		  (corrected in h/dis.h).
 *	L002	scol!hughd	16jul92
 *	- stop compiler warning with 3.2.5 DS (info.c empty translation unit):
 *	  moved #ifdef SCODBDEBUG c_info() into debug.c from retired info.c
 *	L003	scol!blf & scol!nadeem	8apr93
 *	- do not de-reference "snap_regptr" if it's NULL.  This error manifests
 *	  itself as unpredictable behaviour after a kernel panic caused by
 *	  cmn_err(CE_PANIC); either garbage registers or a double panic.
 *	L004	scol!nadeem	21jun93
 *	- added functionality for "value" breakpoints.  See bkp.c
 *	  for more details.
 *	- when quitting from single-step mode, make sure that we reset
 *	  the MODE variable immediately.  This is only becomes a problem
 *	  when debugging multiple cpus using the (scocan) MPX debugging
 *	  features.
 *	L005	scol!blf	19 Aug 1993
 *	- Don't try to use u.u_procp if we don't have a u (i.e., early
 *	  in startup) or u.u_procp is NULL.
 *	L006	scol!nadeem	3dec93
 *	- fixed bug in displaying history numbers - intermediate zeroes
 *	  were being omitted, so that the history would look like:
 *	  debug0:599>, debug0:60>, debug0:61>, ..., debug0:69>, debug0:610>.
 *	L007	scol!hughd	27jan94
 *	- updated help menu "stack" syntax to [stack_addr|-p pid|-p proc_addr]
 *	- updated help menu "r" syntax similarly, it also uses vtoregs()
 *	L008	nadeem@sco.com	6apr94
 *	- the "exact" breakpoint concept has been removed, so update the
 *	  help message for the "bp" command appropriately.  Also, there
 *	  is a new "name" keyword for the breakpoint command.
 *	L009	nadeem@sco.com	19aug94
 *	- make 'e' and 'j' aliases for <space> in single-step mode.  This means
 *	  that you can just type repeat 'j' or 'e' without stopping at call
 *	  instructions.
 *	- when single-stepping an indirect call instruction (eg: call 14(%eax)),
 *	  print out the effective address as well (eg: call 14(%eax) <htbmap> ).
 *	L010	nadeem@sco.com
 *	- added support for fast single stepping.  A new "C" command causes
 *	  instructions to be executed up until the next control transfer
 *	  instruction (ie: jump/call).
 *	L011	nadeem@sco.com	13dec94
 *	- added support for user level scodb.  Generate an alternative and
 *	  chopped down version of scodb() which only includes the command loop.
 *	- only allow the commands which make sense at user level; alias,
 *	  change memory, dump memory, declare structure, register dump,
 *	  structure dump, stack backtrace, unassemble,  define variable.
 *	- modified db_pproc() to work correctly.  This involves dereferencing
 *	  the local variable &upage instead of &u for U-page accesses, and
 *	  making use of the PROC_PTR() macro which is supplied with a kernel
 *	  proc slot address and returns a user pointer to the proc slot which
 *	  can be safely dereferenced.
 */

#include <proc/pid.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <util/engine.h>

#undef		debug

#include	"dbg.h"
#include	"dis.h"
#include	"sent.h"
#include	"histedit.h"
#include	"bkp.h"


STATIC		int	 level		= 0;
STATIC		int	 paniced	= 0;
NOTSTATIC	int	*REGP = NULL;
NOTSTATIC	int	 MODE		= KERNEL;
NOTSTATIC	char	*scodb_error;
NOTSTATIC	char	*scodb_error2;

extern int		 scodb_maxhist;
extern struct scodb_list	 scodb_history;

/* forward declarations for dbcmd */
int	c_alias();
int 	c_backtrace(),	c_bc(),		c_bl(),		c_bp();
int	c_calc(),	c_chmem();
int	c_dcl(),	c_dumpmem();
int	c_help(),	c_newdebug(),	c_ps();
#ifdef SCODBDEBUG
int	c_info();
#endif
int	c_quit(),	c_quitif();
int	c_regdisp();
int	c_singstep(),	c_step(),	c_struct();
int	c_tfe(),	c_type();
int	c_unalias(),	c_unasm(),	c_undcl(),	c_unvar();
int	c_var();

STATIC struct dbcmd calc_cmd = {
	{ "calculator", }, 0, AR_INF, c_calc
};

NOTSTATIC struct dbcmd scodb_commands[] = {
	{	{ "alias", },
		0, AR_INF,
		c_alias
	},
	{	{ "bc", },
		1, AR_INF,
		c_bc
	},
	{	{ "bl", },
		0, AR_INF,
		c_bl
	},
	{	{ "bp", },
								/* L004v */
		1, 4,						/* L004^ */
		c_bp
	},
	{	{ "c", "cb", "cs", "cl", },
		1, AR_INF,
		c_chmem
	},
	{	{ "d", "db", "ds", "dl", "dn" },		/* L000 */
		1, AR_INF,
		c_dumpmem
	},
	{	{ "declare", "dcl", },
		0, AR_INF,
		c_dcl
	},
#ifdef SCODBDEBUG
	{	{ "info", },
		0, AR_INF,
		c_info
	},
#endif
	{	{ "newdebug", },
		0, 0,
		c_newdebug
	},
	{	{ "ps", },
		0, 0,
		c_ps
	},
	{	{ "q", "Q", "quit", "return", },
		0, AR_INF,
		c_quit
	},
#ifndef USER_LEVEL						/* L011v */
	{	{ "quitif", },
		1, AR_INF,
		c_quitif
	},
#endif								/* L011^*/
	{	{ "r", "R", },
		0, AR_INF,
		c_regdisp
	},
	{	{ "s", "step", },
 		0, 1,
		c_singstep
	},
	{	{ "stack", "b", "B", },
		0, AR_INF,
		c_backtrace
	},
	{	{ "struct", },
		0, AR_INF,
		c_struct
	},
#ifdef NEVER
	{	{ "tfe", },
		0, 1,
		c_tfe
	},
#endif								/* L011^ */
	{	{ "type", },
		1, AR_INF,
		c_type
	},
	{	{ "u", "dis", },
		1, AR_INF,
		c_unasm
	},
	{	{ "unalias", },
		1, AR_INF,
		c_unalias
	},
	{	{ "undeclare", "undcl", },
		1, AR_INF,
		c_undcl
	},
	{	{ "unvar", },
		1, AR_INF,
		c_unvar
	},
	{	{ "var", },
		0, AR_INF,
		c_var
	},
	{	{ 0, "?", "help", },	/* don't list */
		0, AR_INF,
		c_help
	},
};
NOTSTATIC char ndbcmd = NMEL(scodb_commands);

/*
*	Breakpoints are only "really set" (i.e., have the int3
*	instruction written to the code space or DR7 modified)
*	if at level 1, so that no breakpoints are hit while
*	still in the debugger.
*
*	Certain things are done by the debugger for safety:
*		o  Debugger executes on separate stack (see call.s)
*		   so that no stack overflows will occur.
*
*		o  debugkey is saved and set to 0 so that attempts
*		   to enter the debugger from within it will fail.
*
*	Other values are saved and reset upon exit of the debugger:
*		o  REGP, the register pointer
*/
NOTSTATIC
scodb(regp)
	int *regp;
{
	int argc, c, r, doret = 0, doerr = 0, i, col;
	static int first = 0;
	static int wascall = 0;
	static int wasret = 0;
	static int faststep = 0;				/* L010 */
	char **argv;
	struct dbcmd *getcmd(), *cmd;
	struct instr inst;
	int *oREGP;
	extern int umode;

#define		RETURN	goto retn

	scodb_enter();

	/* any level save stuff */
	oREGP = REGP;

	REGP = regp;	/* remember user's registers */

	/*
	 * Setup the REGP values corresponding to the kernel stack.
	 * T_UESP is the place corresponding to the user stack pointer, if
	 * an inter-segment transition took place.  There is no corresponding
	 * place on the stack where the value of the kernel stack pointer at
	 * the time of the trap is stored.  T_ESP is the place on the stack
	 * where the pusha instruction placed the value of the kernel
	 * stack pointer before the pusha instruction was executed.  This
	 * value is not used by the corresponding popa instruction.  We use
	 * this as a place to store the kernel ESP value.  We assume that the
	 * kernel ESP was at the point on the stack before EFL (EFL-1 == UESP).
	 */

	REGP[T_ESP] = &REGP[T_UESP];

	/* clear high words of segment registers */
	REGP[T_CS] &= 0x0FFFF;
	REGP[T_DS] &= 0x0FFFF;
	REGP[T_ES] &= 0x0FFFF;

	/*
	REGP[FS] &= 0x0FFFF;
	REGP[GS] &= 0x0FFFF;
	REGP[SS] &= 0x0FFFF;
	*/


	++level;
	plev();


#ifndef USER_LEVEL
	switch (databp()) {	/* may print something */
		case DB_RETURN:
			++doret;
			break;

		case DB_CONTINUE:
			break;

		case DB_USAGE:
		case DB_ERROR:
			++doerr;
			break;
	}
#endif

	if (REGP[T_TRAPNO] == BPTFLT) {	/* breakpoint */
		--REGP[T_EIP];	/* back to instruction */
		if (!bp_specisset()) {
			switch (bp_print(REGP[T_CS], REGP[T_EIP])) {
				case DB_RETURN:
					++doret;
					break;

				case DB_CONTINUE:
					break;

				case DB_USAGE:
				case DB_ERROR:
					++doerr;
					break;
			}
		}
	}

	if (doret && !doerr) {
		/*
		*	databp() and bp_print() return
		*	DB_RETURN if no errors occurred _and_
		*	we were asked to quit; otherwise
		*	we just continue.
		*/
		bp_unsetall();
		goto getout;
	}

step:	switch (MODE) {
		case GOTORET:
			bp_unsetspec();	/* just in case */
			if (wasret) {
				wasret = 0;
				MODE = SINGLESTEP;
				goto gostep;
			}
			inst.in_seg = REGP[T_CS];
			inst.in_off = REGP[T_EIP];
			if (!disi(&inst)) {
				badaddr(inst.in_seg, inst.in_off);
				break;
			}
		gtr:	if (inst.in_flag & I_CALL) {
				/* skip calls */
				setspec(&inst);	/* may change MODE */
				if (bp_setspec()) /* may change MODE */
					REGP[T_EFL] &= ~PS_T;
				else
					REGP[T_EFL] |= PS_T;
				MODE = GOTORET;
				RETURN;
			}
			wasret = !strcmp(inst.in_opcn, "ret") ||
					!strcmp(inst.in_opcn, "iret");
			REGP[T_EFL] |= PS_T;
			RETURN;

		case TRACEFUNCE:
			/*
			*	if entry to a function, see if
			*	user wants to stop or continue;
			*	if bad address always stop.
			*/
			r = check_tfe();
			if (r < 0)	/* error? */
				break;
			if (r) {
				c = getrchr();
				if (quit(c))
					break;
			}
			else
				REGP[T_EFL] |= PS_T;
			RETURN;

		case SINGLESTEP:
			if (wascall && bp_specisset()) {
				char *s, *symname();

				/*
				*	really came from a call,
				*	print return value
				*/
				s = symname(REGP[T_EAX], 1);
				if (umode & UM_BINARY) {
					if (first++)
						putchar('\n');
					printf("\t\tcall ");
				}
				else
					putchar('\t');
				printf("returns %x", REGP[T_EAX]);
				if (s)
					printf(" (%s)", s);
				bp_unsetspec();
			}
			/* disassemble this instruction */
		gostep:	inst.in_seg = REGP[T_CS];
			inst.in_off = REGP[T_EIP];
			if (first++)
				putchar('\n');
			if (!disi(&inst)) {
				badaddr(inst.in_seg, inst.in_off);
				break;
			}
			pinst(REGP[T_CS], REGP[T_EIP], &inst, 0, 0);
			wascall = (inst.in_flag & I_CALL) ? 1 : 0;

#define	_C_PR	" [ejr]? "
#define	C_PR()	printf(_C_PR)
#define	C_UPR()	{						\
			int i;					\
								\
			for (i = 1;i < sizeof(_C_PR);i++) {	\
				putchar('\b');			\
				putchar(' ');			\
				putchar('\b');			\
			}					\
		}

			if (inst.in_flag & I_JUMP) {
				int n, r;
				struct opmap *getopm();

				n = getopm(&inst)->op_opnds[2];
				if (n != 0 && n < 18) {
					switch (n) {
					case  1:	/* jo	*/
						r = F_OF;
						break;
					case  2:	/* jno	*/
						r = !F_OF;
						break;
					case  3:	/* jb	*/
						r = F_CF;
						break;
					case  4:	/* jnb	*/
						r = !F_CF;
						break;
					case  5:	/* je	*/
						r = F_ZF;
						break;
					case  6:	/* jne	*/
						r = !F_ZF;
						break;
					case  7:	/* jbe	*/
						r = F_CF || F_ZF;
						break;
					case  8:	/* jnbe	*/
						r = !F_CF && !F_ZF;
						break;
					case  9:	/* js	*/
						r = F_SF;
						break;
					case 10:	/* jns	*/
						r = !F_SF;
						break;
					case 11:	/* jp	*/
						r = F_PF;
						break;
					case 12:	/* jnp	*/
						r = !F_PF;
						break;
					case 13:	/* jl	*/
						r = F_SF != F_OF;
						break;
					case 14:	/* jnl	*/
						r = F_SF == F_OF;
						break;
					case 15:	/* jle	*/
						r = F_ZF || (F_SF != F_OF);
								/* L001 */
						break;
					case 16:	/* jnle	*/
						r = !F_ZF && (F_SF == F_OF);
						break;
					case 17:	/* jcxz	*/
						r = F_CXZ;
						break;
					}
					if (r)
						printf(" [yes]");
					else
						printf(" [no]");
				}
				else if (n > 17)
					printf(" [unknown]");
			}
			else if (wascall) {
				print_call_eaddr(&inst);	/* L009 */
				C_PR();
			}

			if (faststep) {				/* L010v */
				if ((inst.in_flag & (I_CALL|I_JUMP)) ||
				    !strcmp(inst.in_opcn, "ret") ||
				    !strcmp(inst.in_opcn, "iret"))
					faststep = 0;
				else
					RETURN;
			}					/* L010^ */

			col = p_col();
			for (;;) {
				p_ssregs(col);
				c = getrchr();
				if (quit(c)) {
					MODE = KERNEL;		/* L004 */
					e_ssregs(col);
					putchar('\n');
					break;
				}
				if (c == 'r' || c == 'R') {
					e_ssregs(col);
					MODE = GOTORET;
					goto gtr;
				}
				if (wascall) {
					if (c == 'e' || c == 'E') {
						/* enter */
						if (level == 1)
							REGP[T_EFL] |= PS_T;
						bp_unsetspec();
					}
					else if (c == 'j' || c == 'J') {
						/* jump over */
						if (level == 1) {
							setspec(&inst);
							if (bp_setspec())
								REGP[T_EFL] &= ~PS_T;
							else
								REGP[T_EFL] |= PS_T;
						}
					}
					else {
						dobell();
						continue;
					}
				}
				else {
					switch (c) {		/* L010v */
					case 'C':
						faststep = 1;
						/* fall through */
					case ' ':
					case 'j':
					case 'e':
						if (level == 1)
							REGP[T_EFL] |= PS_T;
						break;
					default:
						dobell();
						continue;
					}			/* L010^ */
				}
				e_ssregs(col);
				if (wascall)
					C_UPR();
				RETURN;
			}
			wascall = 0;
			break;

		case KERNEL:
			putchar('\n');
			if (REGP[T_TRAPNO] == 0 && !paniced)
				db_pproc();
			bp_unsetall();
			break;

		case FROMBP:
			/*
			*	just from a breakpoint at our previous
			*	cs:eip, so we unset and reset the bps
			*	to force the old one.
			*/
			bp_unsetall();
			MODE = KERNEL;
			REGP[T_EFL] &= ~PS_T;
			if (level == 1) {
				bp_setall(1);	/* force it to be set */
			}
			RETURN;
	}

	first = 0;
	REGP[T_EFL] &= ~PS_T;
	for (;;) {
		scodb_error2 = scodb_error = NULL;
		cmd = getcmd(&argc, &argv, &scodb_history);
		switch (docmd(cmd, argc, argv)) {
			case DB_CONTINUE:	/* ignore */
			case DB_ERROR:		/* ignore */
				break;

			case DB_USAGE:		/* bitch and ignore */
				usage(cmd);
				break;

			case DB_RETURN:		/* bail out */
				goto getout;
		}
	}
getout:	if (level == 1) {	/* really leaving. */
		switch (MODE) {
			case SINGLESTEP:
			case TRACEFUNCE:
				goto step;

			case KERNEL:
				bp_setall(0);
				/* may change MODE to FROMBP */
				break;
		}
	}
retn:	--level;
	if (level == 0) {	/* really leaving debugger */
	}
	else {
		plev();
#ifndef USER_LEVEL
		REGP = oREGP;
#endif
	}
	scodb_exit();
	db_clear_frames();					/* L001 */

	scodb_end_io();

	return;
}

#ifdef OLD_USER_LEVEL

NOTSTATIC
scodb()
{
	int argc;
	char **argv;
	int quitflag = 0;
	struct dbcmd *getcmd(), *cmd;

	/* clear high words of segment registers */

	REGP[CS] &= 0x0FFFF;
	REGP[DS] &= 0x0FFFF;
	REGP[ES] &= 0x0FFFF;
	REGP[FS] &= 0x0FFFF;
	REGP[GS] &= 0x0FFFF;

	plev();

	db_pproc();

	while (!quitflag) {
		scodb_error2 = scodb_error = NULL;
		cmd = getcmd(&argc, &argv, &scodb_history);
		switch (docmd(cmd, argc, argv)) {
			case DB_CONTINUE:	/* ignore */
			case DB_ERROR:		/* ignore */
				break;

			case DB_USAGE:		/* bitch and ignore */
				usage(cmd);
				break;

			case DB_RETURN:		/* bail out */
				quitflag = 1;
		}
	}
}

#endif

/*
*	put debug level in history's prompt
*/
plev() {
	int l = level - 1;
	register char *s;

	s = scodb_history.li_prompt + 5;
	if (l >= 0) {						/* L011 */
		if (l > 10)
			*s++ = dtoc(l / 10);
		*s++ = dtoc(l % 10);
	}


#ifndef UNIPROC
	/*
	 * Add the processor number to the debug prompt.
	 */

	*s++ = ':';
	*s++ = dtoc(myengnum % 10);
#endif

	*s++ = '>';

	scodb_history.li_pp = s;
	LF_PLX(&scodb_history, scodb_history.li_pp - scodb_history.li_prompt);
}

/*
*	print the process id and name of the current process
*/

#ifndef USER_LEVEL						/* L011 */

NOTSTATIC
db_pproc() {
	proc_t *pp;
	lwp_t *lwpp;
	struct execinfo *ei;
	
	pp = practive;
	if (pp) {
		lwpp = pp->p_lwpp;
		ei = pp->p_execinfo;

		if (pp->p_pidp)
			printf("PID 0x%x: ", pp->p_pidp->pid_id);
		if (ei)
			printf("%s", ei->ei_comm);
		if (lwpp == NULL)
			printf(" <zombie>");
		printf("\n");
	}
}

#else								/* L011v */

NOTSTATIC
db_pproc() {
#ifdef NOTYET
	int i;
	char *s;
	struct proc *p;
	struct user *up = &upage;

	if (up->u_procp == (struct proc *)0)
		return;

	p = PROC_PTR(up->u_procp);

	printf("\nPID ");
	pnz(p->p_pid, 4);

	s = p->p_pid ? up->u_psargs : up->u_comm;

	if (*s) {
		putchar(':');
		putchar(' ');
		prst(s, 50);
	}
	putchar('\n');
#endif
	printf("db_pproc()\n");
}

#endif								/* L011^ */

NOTSTATIC
docmd(cmd, argc, argv)
	struct dbcmd *cmd;
	int argc;
	char *argv[];
{
	if ((cmd->dc_minargs >= argc) || (cmd->dc_maxargs != AR_INF && cmd->dc_maxargs < (argc - 1)))  	/* arg count */
		return DB_USAGE;
	return (*cmd->dc_func)(argc, argv);
}

/*
*	it is very important that nothing mess with the
*	argument vector (*av) or its contents, as the
*	vectors point into the history list...
*/
NOTSTATIC
struct dbcmd *
getcmd(ac, av, list)
	int *ac;
	char ***av;
	register struct scodb_list *list;
{
	char *s, c;
	register struct dbcmd *cmd;
	register int n, cn;
	extern struct dbcmd scodb_ocmds[];
	extern int scodb_nocmds;
	char **v;

tryagain:
	if (list) {
		list->li_curnum = list->li_mxnum;
#if 0
		phn(list);
#endif
		printf(list->li_prompt);
		if (getivec(ac, av, list, 0, 0) == 0)
			goto tryagain;
	}
	if (!getalias(ac, av)) {
		if (list && list->li_flag & LF_CANERR) {
			/* even so, keep this line: */
			inc_hist(list);
			goto tryagain;
		}
		else
			return 0;	/* error */
	}
	c = (*av)[0][0];
	for (n = 0;n < NMEL(scodb_commands);n++) {
		v = scodb_commands[n].dc_name;
		if (!*v)
			++v;
		while (*v) {
			if (!strcmp(*v, (*av)[0])) {
				/*	found command	*/
				cmd = &scodb_commands[n];
				goto retcmd;
			}
			++v;
		}
	}
	/*
	*	check "other" commands
	*/
	for (n = 0;n < scodb_nocmds;n++) {
		v = scodb_ocmds[n].dc_name;
		if (!*v)
			++v;
		while (*v) {
			if (!strcmp(*v, (*av)[0])) {
				/*	found command	*/
				cmd = &scodb_ocmds[n];
				goto retcmd;
			}
			++v;
		}
	}
	/*
	*	must be a calculator line
	*/
	cmd = &calc_cmd;

retcmd:
	/*
	*	got a command, may have argument problems
	*/
	if ((cmd->dc_minargs >= *ac) || (cmd->dc_maxargs != AR_INF && cmd->dc_maxargs < (*ac - 1))) {	/* arg count */
		usage(cmd);
		if (list && list->li_flag & LF_CANERR) {
			inc_hist(list);
			goto tryagain;
		}
		else
			return 0;
	}

	/*
	*	seems ok
	*/
	inc_hist(list);
	return cmd;
}

STATIC
inc_hist(list)
	struct scodb_list *list;
{
	if (list) {
		++list->li_mxnum;
		if (list->li_flag & LF_WRAPS) {
			if ((list->li_mxnum % list->li_mod) == (list->li_minum % list->li_mod))
				++list->li_minum;
		}
	}
}

/*
*	put a history number in list's prompt
*/
STATIC
phn(list)
	register struct scodb_list *list;
{
	list->li_pp = list->li_prompt + LF_PL(list);

	pnd(list->li_pp, list->li_curnum);

	list->li_pp += strlen(list->li_pp);

	*list->li_pp++ = '>';
	*list->li_pp++ = ' ';
	*list->li_pp = 0;
}

NOTSTATIC
usage(f)
	struct dbcmd *f;
{
	printf("Incorrect command usage - type 'help' for help\n");
}

NOTSTATIC
cleariline(n)
	register int n;
{
	while (n--)
		printf("\b \b");
}

NOTSTATIC
perr() {
	printf("Error");
	if (scodb_error) {
		putchar(':');
		putchar(' ');
		printf(scodb_error, scodb_error2);
	}
	else
		putchar('.');
	putchar('\n');
}

/*
 * newdebug command - change to next debugger.
 */

c_newdebug(int c, char **v)
{
	kdb_next_debugger();
}

/*
 * ps command - display process status.
 */

c_ps(int c, char **v)
{
	db_ps();
	return DB_CONTINUE;
}

#ifdef SCODBDEBUG					/* L002 begin */
# undef		C_REG
# include	<syms.h>
# include	"stunv.h"
# include	"val.h"

NOTSTATIC
c_info(c, v)
	int c;
	char **v;
{
	int t_basic, t_derived;
	struct value va;
	char buf[128], *s, *ptyp();
	extern struct cstun *stun;
	extern int nstun;
	extern struct btype btype[];
	extern int *REGP;

	va.va_seg = REGP[T_DS];
	if (!valuev(v + 1, &va, 1, 0)) {
		perr();
		return DB_ERROR;
	}
	t_basic = BTYPE(va.va_cvari.cv_type);
	t_derived = va.va_cvari.cv_type >> N_BTSHFT;

	if (va.va_flags & V_VALUE) {
		printf("segment:      ");
			pnz(va.va_seg, 4);
			putchar('\n');
		printf("value:        ");
			pnz(va.va_value, 8);
			putchar('\n');
	}
	else {
		printf("not a value\n");
	}
	printf("address:      ");
		pnz(va.va_address, 8);
		putchar('\n');
	printf("size:         %d\n", f_sizeof(0, &va.va_cvari, 0));
	printf("flags:        ");
		pnz(va.va_flags, 4);
		putchar('\n');
	printf("basic type:   ");
		pnz(t_basic, 2);
		printf(" = %s", btype[t_basic].bt_nm);
		if (IS_STUN(t_basic)) {
			if (va.va_cvari.cv_index >= 0)
				printf(" %s", stun[va.va_cvari.cv_index].cs_names);
			else
				printf(" unknown");
		}
		putchar('\n');
	printf("derived type: ");
		pnz(t_derived, 6);
		if (t_derived) {
			s = ptyp("", t_derived, buf, sizeof buf, va.va_cvari.cv_dim);
			printf(" = %s", s);
		}
		putchar('\n');
	return DB_CONTINUE;
}
#endif							/* L002 end */
