#ident	"@(#)kern-i386at:svc/panic.c	1.25.8.1"
#ident	"$Header$"

#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/conf.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/bootinfo.h>
#include <svc/fp.h>
#include <svc/memory.h>
#include <svc/psm.h>
#include <svc/reg.h>
#include <svc/systm.h>
#include <svc/uadmin.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/kcontext.h>
#include <util/kdb/xdebug.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

extern int upyet;

/*PRINTFLIKE1*/
extern void panic_printf(const char *fmt, ...);

struct panic_data panic_data;
int dblpanic;

/*
 * value to pass to ms_shutdown().  Configurable panic (via uadmin())
 * allows the system to stay down or reboot.
 *
 *  zero        -> no auto reboot
 *  non-zero    -> automatic reboot
 */
extern int bootpanic;

extern void _powerdown_hook(void);

STATIC void printprocregs(void);
STATIC void panic_print_stack(kcontext_t *);
STATIC boolean_t panic_print_regs;

void mdboot(int, int);
void mdboot2(int);

/*
 * void
 * panic_start(kcontext_t *kcp)
 *	First half of platform-dependent panic processing.
 *
 * Calling/Exit State:
 *	Called with a pointer to the saved context, as saved by saveregs().
 *	The caller is responsible for mutexing other panic and cmn_err users.
 *
 * Description:
 *	Saves this engine's state in well-known places for dump analysis.
 *	Prints machine state (using panic_printf()).
 */

void
panic_start(kcontext_t *kcp)
{
	panic_data.pd_rp = kcp;

	if (u.u_procp != NULL)
		panic_data.pd_lwp = u.u_procp->p_lwpp;

	if (upyet) {
		panic_data.pd_engine = l.eng;
		l.panic_pt = READ_PTROOT();
		if (using_fpu)
			save_fpu();
	} else
		panic_data.pd_engine = engine;

	printprocregs();
	ms_show_state();
}

/*
 * void
 * panic_shutdown(void)
 *	Second half of platform-dependent panic processing.
 *
 * Calling/Exit State:
 *	The caller is responsible for mutexing other panic and cmn_err users.
 *	The caller will already have printed the regular PANIC message.
 *
 * Description:
 *	Stops all running engines, then shuts down or reboots.
 *	If available, may invoke a kernel debugger.
 */
void
panic_shutdown(kcontext_t *kcp)
{
#ifdef NODEBUGGER
	panic_print_stack(kcp);
#else
	if (*cdebugger == nullsys)
		panic_print_stack(kcp);
	(*cdebugger)(DR_PANIC, NO_FRAME);
#endif

	mdboot(bootpanic ? AD_BOOT : AD_HALT, 0);
	/* NOTREACHED */
}

/*
 * MACRO char
 * digtohex(int dig)
 *	Return hex character code representing integer value from 0 to 15
 */
#define	digtohex(dig)	(((0 <= (dig)) && ((dig) <= 9)) ? \
				'0' + (dig) : 'A' + ((dig) - 10))

#define MAXHEXDIG	8

/*
 * STATIC void
 * panic_printhex(uint_t, width)
 *	Print an unsigned integer in hex in an emergency.
 *
 * Calling/Exit State:
 *	Called from panic.
 */
STATIC void
panic_printhex(uint_t x, int width)
{
	char buf[MAXHEXDIG + 1];
	int i;
	uint_t val;

	val = x;
	for (i = 0 ; i < width ; ++i) {
		buf[(width - 1) - i] = digtohex(val & 0x0F);
		val >>= 4;
	}
	buf[width] = 0;
	panic_printf("%s", buf);
}

void
panic_me(void)
{
	*((char *) 0x40000000) = 0;
}

/*
 * STATIC void
 * panic_print_stack(kcontext_t *kcp)
 *	Print out some stack 
 *
 * Calling/Exit State:
 *	Called from panic.
 */
STATIC void
panic_print_stack(kcontext_t *kcp)
{
	ulong_t *esp = (ulong_t *) &kcp;
	int i, v;

	if (u.u_kpgflt_stack)
		esp = (ulong_t *) &u.u_kpgflt_stack[T_SS + 1];
	else if (kcp)
		esp = (ulong_t *) kcp->kctx_esp;
	else
		esp = (ulong_t *) &kcp;
	if (esp >= (ulong_t *) upointer)
		return;
	panic_printf("Raw stack dump begins at 0x%x:\n", esp);
	for (i = 0; i < 10; ++i) {
		panic_printhex((uint_t) esp, 3);
		for (v = 0; v < 8 && esp < (ulong_t *) upointer; ++v) {
			panic_printf(" ");
			panic_printhex(*esp, 8);
			++esp;
		}
		panic_printf("\n");
		if (esp >= (ulong_t *) upointer)
			return;
	}
}

/*
 * void
 * double_panic(kcontext_t *kcp)
 *	Platform-dependent double-panic processing.
 *
 * Calling/Exit State:
 *	Called with a pointer to the saved context, as saved by saveregs().
 *	The caller is responsible for mutexing other panic users.
 *	The caller will already have printed the regular DOUBLE PANIC message.
 *
 * Description:
 *	Called when a panic occurs during another panic.
 *	Saves this engine's state in well-known places for dump analysis,
 *	and directly shuts down without printing anything.
 */

void
double_panic(kcontext_t *kcp)
{
	/*
	 * Set the dblpanic flag so that the firmware routine will not
	 * attempt flushing dirty memory to disk, but instead will reboot
	 * immediately.
	 */
	dblpanic = 1;
	panic_data.pd_dblrp = kcp;

	if (u.u_procp != NULL)
		panic_data.pd_lwp = u.u_procp->p_lwpp;

	/*
	 * At panic level 2 we dump 10 lines of stack in raw hex.
	 * For maximal density, we dump 8 longs per line, together
	 * with a one line header which announces where the dump
	 * begins.
	 */
	if (l.panic_level == 2) {
		printprocregs();
		panic_print_stack(kcp);
	}

	panic_data.pd_engine = l.eng;
	mdboot(bootpanic ? AD_BOOT : AD_HALT, 0);
	/* NOTREACHED */
}

/*
 * void
 * mdboot(int fcn, int mdep)
 *
 *      Halt or Reboot the processor/s
 *
 * Description
 *	Take a system dump if requested. Then cross interrupt all active cpus
 *	and cause all cpus to go to mdboot2 which will complete the shutdown
 *	process. This is needed because some of the shutdown process must be
 *	done on the boot engine and this function may be running on a non-boot
 *	engine.
 *
 * Calling/Exit State:
 *	Intended use for fcn values:
 *		flag == 1	automatic reboot, no user interaction required
 *		flag != 1	halt the system and wait for user interaction
 *      This routine will not return.
 */

/* ARGSUSED */
void
mdboot(int fcn, int mdep)
{
	emask_t			resp;

	if (l.panic_level && os_soft_sysdump)
		sysdump();
/* Enhanced Application Compatibility Support */
/* End Enhanced Application Compatibility Support */
	if ((fcn == AD_HALT) || (fcn == AD_SCO_PWRDOWN))
		_powerdown_hook();

	xcall_all(&resp, B_FALSE, mdboot2, (void*)fcn);

	mdboot2(fcn);
}


/*
 * void
 * mdboot2(int fcn)
 *
 *      Halt or Reboot the processor/s
 *
 * Description:
 *	Complete the shutdown process. Non boot engines just go offline.
 *	The boot engine is responsible for final system shutdown.
 *
 * Calling/Exit State:
 *      This routine will not return.
 */
STATIC void
mdboot2(int fcn)
{
	int	action;
	uint_t	bustype;

	if (myengnum != BOOTENG) {
		DISABLE();
		ms_offline_prep();
		ms_offline_self(); 		/* does not return. */

	} else {

		/* 
		 * If sanity timer in use, turn off to allow clean soft reboot.
		 */
		if (cm_bustypes() & CM_BUS_EISA)
			eisa_sanity_halt();

		DISABLE();
		splhi();	/* Some drivers are assuming this ... */

		reboot_prompt(fcn);
		ms_time_spin(100);

		switch(fcn) {
		case AD_HALT:
/* Enhanced Application Compatibility Support */
			action = MS_SD_AUTOBOOT;
			break;
		case AD_SCO_PWRDOWN:
/* End Enhanced Application Compatibility Support */
			action = (os_shutdown_caps&MS_SD_POWEROFF) ? MS_SD_POWEROFF : MS_SD_HALT;
			break;
		case AD_IBOOT:
			action = MS_SD_BOOTPROMPT;
			break;
		case AD_BOOT:
			action = MS_SD_AUTOBOOT;
			break;
		default:
			action= MS_SD_HALT;
		}

		if (action & os_shutdown_caps)
			ms_shutdown(action);
		for(;;)
			asm("    hlt    ");
	}
	/* NOTREACHED */
}


/*
 * STATIC void
 * printprocregs(void)
 *	Prints all the processor registers.
 *
 * Calling/Exit State:
 *	None.
 */

STATIC void
printprocregs(void)
{
	/*
	 * Try to print out information from a kernel mode address
	 * fault to indicate something about the faulting function,
	 * not xcmn_panic. This can give false information, since
	 * the panic may have resulted from an illegal action within
	 * a legal fault. However, the registers printed from
	 * kcp aren't very useful (so we don't lose much in the
	 * case of a false hit on u.u_kpgflt_stack).
	 */
	if (u.u_kpgflt_stack && !panic_print_regs) {
		ulong_t *tcp = u.u_kpgflt_stack;

		panic_printf("Kernel Page Fault from (cs:eip) = (%x:%x):\n",
			     tcp[T_CS], tcp[T_EIP]);
		panic_printf(" eax=%x ebx=%x ecx=%x edx=%x\n",
			     tcp[T_EAX], tcp[T_EBX],
			     tcp[T_ECX], tcp[T_EDX]);
		panic_printf(" esi=%x edi=%x ebp=%x esp=%x\n",
			     tcp[T_ESI], tcp[T_EDI],
			     tcp[T_EBP], &tcp[T_SS + 1]);
		panic_print_regs = B_TRUE;
	}
}


/*
 * void
 * concurrent_panic(kcontext_t *kcp)
 *	Platform-dependent concurrent-panic processing.
 *
 * Calling/Exit State:
 *	Called with a pointer to the saved context, as saved by saveregs().
 *	The caller will already have printed the regular CONCURRENT PANIC
 *	message.
 *
 * Description:
 *	Called when this engine has panicked, but another engine has already
 *	panicked first.
 *	Saves this engine's state in well-known places for dump analysis,
 *	and directly shuts down without printing anything.
 *
 * Remarks:
 *	Should never happen on a uniprocessor i386at.
 */
/* ARGSUSED */
void
concurrent_panic(kcontext_t *kcp)
{
}
