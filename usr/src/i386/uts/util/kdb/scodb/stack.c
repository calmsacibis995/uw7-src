#ident	"@(#)kern-i386:util/kdb/scodb/stack.c	1.1.1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1995.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <proc/lwp.h>
#include <proc/user.h>
#include <util/kdb/db_as.h>
#include <util/engine.h>
#include "dbg.h"
#include "dis.h"
#include "sent.h"

extern int *REGP;
extern int * volatile db_r0ptr[MAXNUMCPU];
extern void dbprintf();
extern void scodb_call();

extern volatile int db_cp_active[MAXNUMCPU];

#define	 NFRAMES	25
STATIC int *saved_frames[NFRAMES];
STATIC int nsaved_frames = 0;

lwp_t *
scodb_ulwpp()
{
	as_addr_t	addr;
	user_t		*up;
	lwp_t		*ulwpp;

#ifndef UNIPROC
	if (!db_cp_active[myengnum])
		return NULL;
#endif

	SET_KVIRT_ADDR_CPU(addr, (u_long)&upointer, myengnum);

	if (db_read(addr, (char *)&up, sizeof(user_t *)) == -1)
		return NULL;

	SET_KVIRT_ADDR_CPU(addr, (u_long)&up->u_lwpp, myengnum);

	if (db_read(addr, (char *)&ulwpp, sizeof(lwp_t *)) == -1)
		return NULL;

	return ulwpp;
}

scodb_print_entryframe(ulong_t esp, ulong_t nexteip, void (*prf)())
{}

NOTSTATIC
c_backtrace(c, av)
	int c;
	char **av;
{
	lwp_t *lwp;
	int ret, pp, lp, ap;

	ret = parse_stack_args(c, av, &pp, &lp,&ap);

	switch (ret) {
	case 1:					/* stack -p <proc> */
		db_pstack(pp, 0, dbprintf);
		return DB_CONTINUE;
	case 2:					/* stack -l <lwp> */
		db_lstack(lp, dbprintf);
		return DB_CONTINUE;
	case 3:					/* stack -p <proc> -l <lwp> */
		db_pstack(pp, lp, dbprintf);
		return DB_CONTINUE;
	case 4:					/* stack -a <addr>  */
		db_tstack(ap,  dbprintf);
		return DB_CONTINUE;
	case -1:
		return DB_ERROR;
	default:
		break;
	}

	lwp = scodb_ulwpp();

	if (lwp == NULL)
		printf("(idle stack)\n");
	else
		printf("LWP %x:\n", lwp);

#ifdef NOTNOW
	stacktrace(printf, REGP[T_ESP], REGP[T_EBP], REGP[T_EIP], db_r0ptr[myengnum],
		   scodb_call, scodb_print_entryframe, lwp);
#else
	stacktrace(printf, REGP[T_ESP], -1,-1,db_r0ptr[myengnum],
		   scodb_call, scodb_print_entryframe, lwp);

#endif

	return DB_CONTINUE;
}

/*
 * Check whether the address passed is one of the frame pointers
 * displayed with the last stack backtrace command.  Used by the
 * "dn" command to highlight frame pointers.
 */

is_frame_pointer(addr)						/* L000 v */
register int	*addr;
{
	register int i;

	for (i = 0 ; i < nsaved_frames ; i++)
		if (saved_frames[i] == addr)
			return(1);
	return(0);
}

/*
 * Clear the array containing saved frame pointers.  Called when
 * exiting the debugger.
 */

db_clear_frames()
{
	nsaved_frames = 0;
}

db_save_frame(off)
int *off;
{
	if (nsaved_frames < NFRAMES)
		saved_frames[nsaved_frames++] = off;
}								/* L000 ^ */

static int
parse_stack_args(int c, char **av, int *pp, int *lp, int *ap)
{
	int ret, i, seg;

	ret = 0;
	for (i = 1 ; i < c ; i++) {
		if (!strcmp(av[i], "-p")) {
			if (ret & 1) {
				printf("Duplicate -p options\n");
				return(-1);
			}

			if (i + 1 >= c) {
				printf("Not enough arguments to -p option\n");
				return(-1);
			}
			if (!getaddr(av[i + 1], &seg, pp)) {
				printf("Bad argument to -p option\n");
				return(-1);
			}
			i++;			/* skip next argument */
			ret |= 1;
		} else
		if (!strcmp(av[i], "-l")) {
			if (ret & 2) {
				printf("Duplicate -l options\n");
				return(-1);
			}

			if (i + 1 >= c) {
				printf("Not enough arguments to -l option\n");
				return(-1);
			}
			if (!getaddr(av[i + 1], &seg, lp)) {
				printf("Bad argument to -l option\n");
				return(-1);
			}

			i++;			/* skip next argument */
			ret |= 2;
		}
		if (!strcmp(av[i], "-a")) {
			if (ret & 4) {
				printf("Duplicate -a options\n");
				return(-1);
			}

			if (i + 1 >= c) {
				printf("Not enough arguments to -a option\n");
				return(-1);
			}
			if (!getaddr(av[i + 1], &seg, ap)) {
				printf("Bad argument to -a option\n");
				return(-1);
			}

			i++;			/* skip next argument */
			ret |= 4;
		}
		
	}
	return(ret);
}
