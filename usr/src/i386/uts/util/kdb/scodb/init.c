#ident	"@(#)kern-i386:util/kdb/scodb/init.c	1.1.1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * MODIFICATION HISTORY:
 *
 *	L000	scol!nadeem	21apr92
 *      - remove call to dbpicinit() in scodbpminit(), as it was
 *        causing the machine to reboot.  This is because it was setting
 *        up iplmask[], which is one of the per-processor variables.  These
 *        variables are held in a separate page per processor, which is
 *        only initialised after this point via a call to initmpvars().
 *	L001	scol!nadeem	23apr92
 *	- move the call to crllry_debugger() from install.d/scodb_mpx.i into
 *	  here.
 *	L002	scol!nadeem	11may92
 *	- made sane_console() into a null routine, as this was causing
 *	  u-page corruption (due to vidioctl() call), and was causing
 *	  panic's on 3.2v4 final.
 *	L003	scol!nadeem	22jun93
 *	- added support for value breakpoints.  See also bkp.c and
 *	  corollary/dbspt.c for further details.
 *	L004	scol!nadeem	11aug93
 *	- added support for displaying source code line numbers during
 *	  disassembly.  Call db_line_init() to initialise line number table.
 *	L005	scol!jamesh	25nov93
 *	- disable the watchdog timer NMI when we enter the debugger.
 *	L006	scol!jonwe	09Sep94
 *	- disable rtc clock tracking.
 *	L007	nadeem@sco.com	13dec94
 *	- mods for user level scodb.  Move the inline history initialisation
 *	  code from scodbpminit() into a new routine scodb_hist_init() in io.c.
 *	- remove compiler warnings; place '#ifdef NEVER' around sane_console(),
 *	  and define scodb_call() to be a void.
 */

#include	<proc/user.h>
#include	<util/debug.h>
#include	<util/emask.h>
#include	<util/engine.h>
#include	<util/kdb/kdebugger.h>
#include	<util/kdb/xdebug.h>
#include	<util/kdb/db_as.h>
#include	<util/kdb/db_slave.h>
#include	<util/mod/moddefs.h>
#include	<svc/errno.h>

#include	"dbg.h"
#include	"sent.h"
#include	"bkp.h"

void scodb_call();
int scodb_load(), scodb_unload();
extern void db_xprintf();


MOD_MISC_WRAPPER(scodb, scodb_load, scodb_unload, "SCODB kernel debugger");

#ifndef UNIPROC
                        
volatile int scodb_slaves_signalled;
emask_t scodb_slaves;

extern volatile int db_slave_flag[MAXNUMCPU];
extern volatile struct slave_cmd db_slave_command;
extern int db_master_cpu;
extern lock_t db_master_lock;
extern struct lwp * volatile db_lwp[MAXNUMCPU];
extern int Nengine, nonline;

#define dbactive (db_cp_active[myengnum])
extern volatile int db_cp_active[MAXNUMCPU];
                
void scodb_slave_cmd(), scodb_slave();
extern void db_invoke_slave();
#else

extern int dbactive;

#endif

extern int MODE;
extern int * volatile db_r0ptr[MAXNUMCPU];
pl_t scodb_entry_ipl[MAXNUMCPU];

struct kdebugger scodb_reg = {
	scodb_call,
#ifndef UNIPROC
	scodb_slave_cmd,
#else
	0,
#endif
	db_xprintf
};

NOTSTATIC void
scodb_call(reason, rp)
	int reason, *rp;
{
	pl_t pri;
	int eng;

#ifndef UNIPROC
	/* skip eng == 0 */
	for(eng=1;eng <= nonline; eng++)
		EMASK_SET1(&scodb_slaves, eng);
#endif

	if (reason == DR_ONLINE) {
#ifndef UNIPROC
		pl_t oldpri;

		oldpri = LOCK(&db_master_lock, PLHI);
		while (db_master_cpu != -1 && !scodb_slaves_signalled)
			;
		if (db_master_cpu != -1)
			EMASK_SET1(&scodb_slaves, myengnum);
		if (db_master_cpu == -1) {
			UNLOCK(&db_master_lock, oldpri);
			return;
		}
		reason = DR_SLAVE;
		UNLOCK(&db_master_lock, oldpri);
#else
		return;
#endif
	}

	if (!dbactive)
		scodb_entry_ipl[myengnum] = splhi();
	else
		pri = splhi();

	db_flushtlb();

	db_r0ptr[myengnum] = rp;

#ifndef UNIPROC
	db_lwp[myengnum] = u.u_lwpp;

	if (reason == DR_SLAVE) {
		if (dbactive)
			cmn_err(CE_PANIC, "SCODB: slave already active");
		dbactive = 1;

		scodb_slave();
	} else
#endif /* not UNIPROC */
	if (scodb_master(reason)) {
		/* Bail out if we had to revert to slave mode */
#ifndef UNIPROC
		db_lwp[myengnum] = NULL;
#endif
		splx(scodb_entry_ipl[myengnum]);
		return;
	}

#ifndef UNIPROC
	db_lwp[myengnum] = NULL;
#endif

	dbactive = 0;

	splx(scodb_entry_ipl[myengnum]);

}

int
scodb_master(int reason)
{
	int was_branch = 0;
	union dbr6 dr6;

#ifndef UNIPROC
	/*
	 * Stop the world (i.e. put other CPUs in slave mode),
	 * unless we were single-stepping, in which case it's already stopped.
	 */

	if (!dbactive && db_master_cpu != myengnum) {
		/* Negotiate for the right to be the master. */
		if (TRYLOCK(&db_master_lock, PLHI) == INVPL) {
			/*
			 * Someone else got to be master first.
			 * Return so we can reenter in slave mode.
			 */
			return 1;
		} else if (db_master_cpu != -1) {
			UNLOCK(&db_master_lock, PLHI);
			/*
			 * Someone else got to be master first.
			 * Return so we can reenter in slave mode.
			 */
			return 1;
		}
		db_master_cpu = myengnum;
		scodb_slaves_signalled = 0;
		UNLOCK(&db_master_lock, PLHI);

		/* Now signal the other CPUs to stop. */
		{
			int	cpu;

			for (cpu = 0; cpu < Nengine; cpu++) {
				if (cpu == myengnum)
					continue;
				db_lwp[cpu] = NULL;
				db_cp_active[cpu] = 0;
			}
			xcall_all(&scodb_slaves, B_TRUE, db_invoke_slave, NULL);
			scodb_slaves_signalled = 1;
		}

		/* Wait for them all to stop. */
		scodb_slave_cmd(-1, DBSCMD_SYNC);
	}
#endif /* not UNIPROC */
	
	dbactive = 1;

	dr6.d6_val = gdbr(6);

	if (reason == DR_BPT1 && scodb_chkbrkpt() == 0)
		goto exit;

	if (db_r0ptr[myengnum] == NULL)
		debugsetup();	/* see entry.s */
	else
		scodb(db_r0ptr[myengnum]);

exit:

#ifndef UNIPROC
	if (MODE == KERNEL) {
		/* Release master ownership */
		db_master_cpu = -1;

		/* Tell all the slaves to exit */
		scodb_slave_cmd(-1, DBSCMD_EXIT);
	}
#endif /* not UNIPROC */

	return 0;
}

#ifndef UNIPROC

void
scodb_slave(void)
{
	u_int	cmd;

	/* Process commands from the master */
	do {
		/* Wait for a command */
		while (db_slave_flag[myengnum] == 0)
			xcall_intr();
		/* Process the command */
		switch (cmd = db_slave_command.cmd) {
		case DBSCMD_SYNC:
		case DBSCMD_EXIT:
			break;
		case DBSCMD_GET_STACK:
			db_get_stack();
			break;
		case DBSCMD_AS_READ:
			db_slave_command.rval =
				db_read(db_slave_command.addr,
					db_slave_command.buf,
					db_slave_command.n);
			break;
		case DBSCMD_AS_WRITE:
			db_slave_command.rval =
				db_write(db_slave_command.addr,
					 db_slave_command.buf,
					 db_slave_command.n);
			break;
		case DBSCMD_CR0:
			db_slave_command.rval = db_cr0();
			break;
		case DBSCMD_CR2:
			db_slave_command.rval = db_cr2();
			break;
		case DBSCMD_CR3:
			db_slave_command.rval = db_cr3();
			break;
		case DBSCMD_CR4:
			db_slave_command.rval = db_safe_cr4();
			break;
		}
		/* Tell the master we're done */
		ASSERT(db_slave_flag[myengnum] != 0);
		db_slave_flag[myengnum] = 0;

	} while (cmd != DBSCMD_EXIT);
}

/*	int	slave;		Slave CPU #, or -1 for all */

void
scodb_slave_cmd(int slave, int cmd)
{
	int	cpu;


	db_slave_command.cmd = cmd;

	if (slave == -1) {
		for (cpu = 0; cpu < Nengine; ++cpu) {
			if (!EMASK_TEST1(&scodb_slaves, cpu))
				continue;
			ASSERT(db_slave_flag[cpu] == 0);
			db_slave_flag[cpu] = 1;
		}
		for (cpu = 0; cpu < Nengine; ++cpu) {
			if(cpu == myengnum) {
				if(db_slave_flag[cpu] != 0) {
					printf("scodb_slave_cmd: cannot wait for self\n");
					db_slave_flag[cpu] == 0;
					continue;
				}
			}
			while (db_slave_flag[cpu] != 0)
				;
		}
	} else {
		ASSERT(slave != myengnum);
		ASSERT(db_cp_active[slave]);
		ASSERT(db_slave_flag[slave] == 0);
		db_slave_flag[slave] = 1;
		while (db_slave_flag[slave] != 0)
			;
	}
}

#endif

NOTSTATIC
scodb_kdb_init()
{
	scodbinit();
}

NOTSTATIC
scodbinit()
{
	kdb_register(&scodb_reg);
	scodb_unlock_symtab();

	db_bp_init();
	db_dv_init();
	db_dcl_init();
	db_sym_init();
	db_stv_init();
	db_line_init();						/* L004 */

	db_hist_init();						/* L007 */
}

STATIC int
scodb_load()
{
	scodbinit();
	return 0;
}

STATIC int
scodb_unload()
{
	extern struct kdebugger *debuggers;
	struct kdebugger *debugger = &scodb_reg;

	return (EBUSY);

#if 0
	/* FSPIN_LOCK(&kdb_list_lock); */

	debugger->kdb_next->kdb_prev = debugger->kdb_prev;
	debugger->kdb_prev->kdb_next = debugger->kdb_next;
	if (debuggers == debugger) {
		if ((debuggers = debugger->kdb_next) == debugger) {
			debuggers = NULL;
			cdebugger = nullsys;
		} else
			cdebugger = debuggers->kdb_entry;
	}

	/* FSPIN_UNLOCK(&kdb_list_lock); */

	return (0);
#endif
}
