#ident	"@(#)kern-i386:util/kdb/kdb_util/kdb.c	1.45.3.2"
#ident	"$Header$"

#include <io/conf.h>
#include <io/conssw.h>
#include <mem/hatstatic.h>
#include <mem/vmparam.h>
#include <proc/lwp.h>
#include <proc/tss.h>
#include <proc/user.h>
#include <proc/cg.h>
#include <svc/cpu.h>
#include <svc/creg.h>
#include <svc/reg.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/kdb/db_as.h>
#include <util/kdb/kdebugger.h>
#include <util/kdb/xdebug.h>
#include <util/mod/mod_obj.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

MOD_MISC_WRAPPER(kdb, NULL, NULL, "kernel debugger utilities");

char db_xdigit[] = "0123456789abcdef";

extern void (*kdb_inittbl[])();

/* Initial empty init commands; replaced by unixsyms */
static char empty_commands[2];
char *kdbcommands = empty_commands;
int kdbcommsz = sizeof(empty_commands);

int db_master_cpu = -1;

char *debugger_init = "";
volatile unsigned dbg_putc_count;
conschan_t dbg_iochan;
int dbg_cons_suspended;

boolean_t kdb_output_aborted;

int *regset[NREGSET];
enum regset_type regset_type[NREGSET];
unsigned regset_next;

char *kdb_stack0, *kdb_stacks;

STATIC void kdb_alloc_stacks(void);

#define ALIGN(p)  ((char *)(((ulong)(p) + sizeof(ulong) - 1) & \
						~(sizeof(ulong) - 1)))

/*
 * For machines with an interrupt button.
 */
void
kdbintr()
{
	(*cdebugger) (DR_USER, NO_FRAME);
}


/*
 * First-time initialization.  Called from kernel sysinit().
 * This handles the static module case.  When KDB is dynamically loaded
 * the stub version of kdb_init() will be called at sysinit time to
 * handle initialization of the debug_count_mutex; kdb_register will then
 * be called from the interface module(s) load routines.
 */
void
kdb_init(void)
{
	void	(**initf)();

	FSPIN_INIT(&debug_count_mutex);

	/* Make sure debug registers are initially cleared */
	db_wdr7(0);
	db_wdr6(0);

	/* If this processor supports I/O breakpoints, enable them */
	if (l.cpu_features[0] & CPUFEAT_DE) {
		/* enable debug extensions */
		ulong_t cr4 = db_cr4();
		db_wcr4(cr4 | CR4_DE);
	}

	/* Allocate space for cpu 0's private stack */
	ASSERT(kdb_stacks == NULL);
	kdb_stacks = kdb_stack0 = calloc(mmu_ptob(1));
	if (kdb_stacks != NULL) {
		l.dftss.t_esp0 = l.dftss.t_esp =
				(uint_t)kdb_stacks + mmu_ptob(1);
	}

	/* Initialize each statically-installed interface module. */
	for (initf = kdb_inittbl; *initf != (void (*)())0;)
		(*(*initf++)) ();

	/* Get pointer to command strings at end of symbol table */
	debugger_init = kdbcommands;

	/* Call debugger to execute init-time commands, if any */
	if (*debugger_init && cdebugger != nullsys) {
		extern int demon_call_type;

		/*
		 * Set a flag and generate a trap into the debugger.
		 * This is done, rather than calling the debugger
		 * directly, to get a trap frame saved.
		 */
		demon_call_type = DR_INIT;
		asm(" int $3"); /* Force a debug trap */
	}
}


/*
 * Circular list of all currently-registered debuggers.
 * Points into the circle at the currently-active debugger.
 */
struct kdebugger *debuggers = NULL;

static fspin_t kdb_list_lock;	/* mutex for debuggers list */

void
kdb_register(struct kdebugger *debugger)
{
	/* Lock all the symbol tables in memory. */
	mod_obj_symlock(SYMLOCK);

	if (dbg_iochan.cnc_consswp == NULL || dbg_iochan.cnc_dev == NODEV) {
		/* Initialize console I/O */
		if (!console_openchan(&dbg_iochan, *dbg_init_ioswpp,
				      *dbg_init_iominorp, *dbg_init_paramp,
				      B_FALSE)) {
			/*
			 *+ The console device configured for KDB either does
			 *+ not exist or cannot be used as a console.  To
			 *+ correct, change the KDB device configuration or
			 *+ add the indicated device.
			 */
			cmn_err(CE_NOTE, "Specified KDB console unit %d "
					 "unavailable.", *dbg_init_iominorp);
		}
	}

	FSPIN_LOCK(&kdb_list_lock);

	if ((debugger->kdb_next = debuggers) == NULL) {
		debuggers = debugger->kdb_next = debugger;
		cdebugger = debugger->kdb_entry;
	} else
		(debugger->kdb_prev = debuggers->kdb_prev)->kdb_next = debugger;

	debuggers->kdb_prev = debugger;

	FSPIN_UNLOCK(&kdb_list_lock);

	/* Allocate space for private stack(s) if not done already */
	if (kdb_stacks == NULL)
		kdb_alloc_stacks();
}

STATIC void
kdb_alloc_stacks(void)
{
	uint_t i;
#ifdef CCNUMA
	cgnum_t cgnum;
#endif /* CCNUMA */
	struct tss386 *tssp;

	kdb_stacks = kmem_alloc(mmu_ptob(Nengine), KM_NOSLEEP);
	if (kdb_stacks != NULL) {
		for (i = Nengine; i-- != 0;) {
#ifdef CCNUMA
			/*
			 * Skip those cg(s) which are not yet online.
			 */
			cgnum = cpu_to_cg[i];
			if (cgnum == CG_NONE || !IsCGOnline(cgnum))
				continue;
#endif /* CCNUMA */
			tssp = &ENGINE_PLOCAL_PTR(i)->dftss;
			tssp->t_esp0 = tssp->t_esp =
				(uint_t)kdb_stacks + mmu_ptob(i + 1);
		}
	}
}

void
kdb_online(void)
{
	ASSERT(kdb_stacks != NULL);

	if (Nengine > 1) {
		if (kdb_stacks == kdb_stack0) {
			/* still using single stack; allocate full set now */
			kdb_alloc_stacks();
		}
		if (kdb_stacks != NULL && kdb_stacks != kdb_stack0) {
			l.dftss.t_esp0 = l.dftss.t_esp =
				(uint_t)kdb_stacks + mmu_ptob(myengnum + 1);
		}
		/* Make sure debug registers are initially cleared */
		db_wdr7(0);
		db_wdr6(0);

		/* If this processor supports I/O breakpoints, enable them */
		if (l.cpu_features[0] & CPUFEAT_DE) {
			/* enable debug extensions */
			ulong_t cr4 = db_cr4();
			db_wcr4(cr4 | CR4_DE);
		}
	}
}

void
kdb_unregister(struct kdebugger *debugger)
{
	FSPIN_LOCK(&kdb_list_lock);

	debugger->kdb_next->kdb_prev = debugger->kdb_prev;
	debugger->kdb_prev->kdb_next = debugger->kdb_next;
	if (debuggers == debugger) {
		if ((debuggers = debugger->kdb_next) == debugger) {
			debuggers = NULL;
			cdebugger = nullsys;
		} else
			cdebugger = debuggers->kdb_entry;
	}

	FSPIN_UNLOCK(&kdb_list_lock);

	/* Unlock symbol tables. */
	mod_obj_symlock(SYMUNLOCK);
}

/*
 * Hooks for debug_printf() from cmn_err.h.
 */
void
kdb_printf(const char *fmt, VA_LIST ap)
{
	KDB_PRINTF(fmt, ap);
}

boolean_t
kdb_check_aborted(void)
{
	return kdb_output_aborted;
}


void
kdb_next_debugger(void)
{
	debuggers = debuggers->kdb_next;
	cdebugger = debuggers->kdb_entry;
}

boolean_t
kdb_select_io(char *devname, minor_t minor)
{
	static conschan_t newchan;
	int idx;

	for (idx = conscnt; idx-- > 0;) {
		if (strcmp(devname, constab[idx].cn_name) == 0) {
			struct_zero(&newchan, sizeof newchan);
			if (!console_openchan(&newchan,
					      constab[idx].cn_consswp,
					      minor, NULL, B_FALSE))
				return B_FALSE;
			if (dbg_iochan.cnc_dev != NODEV) {
				kdb_end_io();
				console_closechan(&dbg_iochan);
			}
			dbg_iochan = newchan;
			kdb_start_io();
			return B_TRUE;
		}
	}
	return B_FALSE;
}

#ifdef USE_GDB
boolean_t
kdb_gdb_select_io(char *devname, minor_t minor)
{
	extern conschan_t gdb_ioc;
	static conschan_t newchan;
	int idx;

	for (idx = conscnt; idx-- > 0;) {
		if (strcmp(devname, constab[idx].cn_name) == 0) {
			struct_zero(&newchan, sizeof newchan);
			if (!console_openchan(&newchan,
					      constab[idx].cn_consswp,
					      minor, NULL, B_FALSE))
				return B_FALSE;
			gdb_ioc = newchan;
			return B_TRUE;
		}
	}
	return B_FALSE;
}
#endif

void
kdb_start_io(void)
{
	/*
	 * The console for KDB may or may not be the same as the system
	 * console device.  If they are the same, input may already be
	 * suspended through the console channel.  Suspend input now
	 * if it has not already been suspended.
	 */
	if (dbg_iochan.cnc_consswp != conschan.cnc_consswp ||
	    dbg_iochan.cnc_minor != conschan.cnc_minor)
		console_suspend(&dbg_iochan);
	else {
		dbg_cons_suspended = (conschan.cnc_flags & CNF_SUSPENDED);
		CONSOLE_SUSPEND();
		dbg_iochan.cnc_flags |= CNF_SUSPENDED;
	}
}

void
kdb_end_io(void)
{
	/*
	 * If we suspended input in kdb_start_io, resume it now.
	 */
	if (dbg_iochan.cnc_consswp != conschan.cnc_consswp ||
	    dbg_iochan.cnc_minor != conschan.cnc_minor)
		console_resume(&dbg_iochan);
	else {
		if (!dbg_cons_suspended) {
			CONSOLE_RESUME();
			dbg_cons_suspended = 0;
		}
		dbg_iochan.cnc_flags &= ~CNF_SUSPENDED;
	}
}

/*
 * findsyminfo looks thru symtable to find the routine name which begins
 * closest to value.
 */

char *
findsyminfo(vaddr_t value, vaddr_t *loc_p, int *valid_p)
{
	char *name;
	ulong_t offset;

	name = mod_obj_getsymname((ulong_t)value, &offset, NOSLEEP, NULL);
	*loc_p = value - offset;
	if (name == NULL || *loc_p < kvbase) {
		*valid_p = 0;
		*loc_p = 0;
		return "ZERO";
	}
	*valid_p = 1;
	return name;
}

char *
findsymname(vaddr_t value, void (*tell)())
{
	char *p;
	vaddr_t loc;
	int valid;

	p = findsyminfo(value, &loc, &valid);
	if (tell)
		(*tell) (p, value, loc);
	return (valid ? p : NULL);
}

void
db_sym_and_off(vaddr_t addr, void (*prf)())
{
	char *p;
	vaddr_t sym_addr;
	int valid;

	p = findsyminfo(addr, &sym_addr, &valid);
	if (!valid) {
		(*prf) ("?0x%x?", addr);
		return;
	}
	(*prf) ("%s", p);
	if (addr != sym_addr)
		(*prf) ("+0x%lx", addr - sym_addr);
}

vaddr_t
findsymval(vaddr_t value)
{
	vaddr_t	loc;
	int	valid;

	(void) findsyminfo(value, &loc, &valid);
	return valid? loc : (ulong)0;
}

/*
 * findsymaddr looks thru symtable to find the address of name.
 */

ulong
findsymaddr(char *name)
{
	return mod_obj_getsymvalue(name, B_FALSE, NOSLEEP);
}


volatile ulong_t db_st_startsp;
volatile ulong_t db_st_startpc;
volatile ulong_t db_st_startfp;
ulong_t db_st_offset;
int db_st_cpu;
int *db_st_r0ptr;

as_addr_t st_addr;

extern void stacktrace(void (*)(), ulong_t, ulong_t, ulong_t, ulong_t, ulong_t,
		       void (*)(), lwp_t *);

static void db_print_entry(ulong_t, ulong_t, void (*)());

void
db_stacktrace(void (*prf)(), ulong_t dbg_entry, boolean_t local, lwp_t *lwp)
{
	if (local) {
		db_get_stack();
		db_st_offset = 0;
		db_st_cpu = l.eng_num;
	}
	SET_KVIRT_ADDR_CPU(st_addr, 0, db_st_cpu);

	for (regset_next = NREGSET; regset_next != 0;)
		regset[--regset_next] = NULL;

	stacktrace(prf, db_st_startsp, db_st_startfp, db_st_startpc,
		   (ulong_t)db_st_r0ptr, dbg_entry, db_print_entry, lwp);
	db_st_r0ptr = NULL;
}


static void
db_print_entry(ulong_t esp, ulong_t nexteip, void (*prf)())
{
	int reason;

	ASSERT(prf);

	(*prf) ("DEBUGGER ENTERED FROM ");
	st_addr.a_addr = esp + db_st_offset;
	if (db_read(st_addr, &reason, sizeof(int)) == -1)
		return;
	switch (reason) {
	case DR_USER:
	case DR_SECURE_USER:
		(*prf) ("USER REQUEST");
		break;
	case DR_BPT1:
	case DR_BPT3:
		(*prf) ("BREAKPOINT");
		break;
	case DR_STEP:
		(*prf) ("SINGLE-STEP");
		break;
	case DR_PANIC:
		(*prf) ("PANIC");
		break;
	case DR_SLAVE:
		(*prf) ("ANOTHER CPU");
		break;
	default:
		db_sym_and_off(nexteip, prf);
		break;
	}
	(*prf) ("\n");
}
