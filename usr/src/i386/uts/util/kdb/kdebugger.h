#ifndef _UTIL_KDB_KDEBUGGER_H	/* wrapper symbol for kernel use */
#define _UTIL_KDB_KDEBUGGER_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/kdb/kdebugger.h	1.26"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*	Used by kernel debuggers */

#ifdef _KERNEL_HEADERS

#include <io/conssw.h>		/* REQUIRED */
#include <proc/lwp.h>		/* REQUIRED */
#include <util/cmn_err.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/cmn_err.h>	/* REQUIRED */
#include <sys/conssw.h>		/* REQUIRED */
#include <sys/lwp.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

struct kdebugger {
	void	(*kdb_entry)(int reason, int *r0ptr);
	void	(*kdb_slave_cmd)(int cpu, int cmd);
	void	(*kdb_printf)(const char *fmt, VA_LIST ap);
	int	kdb_reserved[3];
	struct kdebugger *kdb_next;
	struct kdebugger *kdb_prev;
};

extern struct kdebugger *debuggers;

#define KDB_SLAVE_CMD(cpu, cmd)	(*debuggers->kdb_slave_cmd)(cpu, cmd)
#define KDB_PRINTF(fmt, ap)	(*debuggers->kdb_printf)(fmt, ap)

/*
 * dbg_init_ioswpp, dbg_init_iominorp, and dbg_init_paramp point to variables
 * containing the initial conssw pointer, minor number, and parameter string
 * to use for the debugger until (and if) a debugger command is used to switch
 * to a different device.
 *
 * dbg_iochan is the currently open channel to the current device.
 */
extern conssw_t **dbg_init_ioswpp;
extern minor_t *dbg_init_iominorp;
extern char **dbg_init_paramp;
extern conschan_t dbg_iochan;

extern volatile unsigned dbg_putc_count;

#define DBG_GETCHAR()	console_getc(&dbg_iochan)
#define DBG_PUTCHAR(c)	{ \
		while (console_putc(&dbg_iochan, c) == 0) \
			; \
		++dbg_putc_count; \
	}

extern boolean_t kdb_output_aborted;

/*
 * As a security feature, the kdb_security flag (set by the KDBSECURITY
 * tuneable) is provided.  If it is non-zero, the debugger should ignore
 * attempts to enter from a console key sequence.
 */
extern int kdb_security;

extern char *debugger_init;	/* Optional initial command string */

#define NREGSET	8
extern int *regset[NREGSET];
extern enum regset_type {
	RS_FULL, RS_INTR
} regset_type[NREGSET];
extern unsigned regset_next;

extern int db_master_cpu;

extern volatile ulong_t db_st_startpc;
extern volatile ulong_t db_st_startsp;
extern volatile ulong_t db_st_startfp;
extern ulong_t db_st_offset;
extern int db_st_cpu;
extern int *db_st_r0ptr;

#define UNKNOWN	((ulong_t)-1)	/* unknown register value for stack trace */

/* Some useful 386 opcodes */

#define OPC_INT3	0xCC
#define OPC_PUSHFL	0x9C
#define OPC_CALL_REL	0xE8
#define OPC_CALL_DIR	0x9A
#define OPC_CALL_IND	0xFF
#define OPC_PUSH_EBP	0x55
#define OPC_MOV_RTOM	0x89
#define OPC_ESP_EBP	0xE5
#define OPC_MOV_MTOR	0x8B
#define OPC_EBP_ESP	0xEC

/* This inline function gets the register values needed to start
   a stack trace.  Must not be called from a routine which returns before
   db_stacktrace() is called. */

asm void db_get_stack(void)
{
%lab	push_eip
	call	push_eip
push_eip:
	popl	db_st_startpc
	movl	%esp, db_st_startsp
	movl	%ebp, db_st_startfp
}

/* Other inline functions.  These may be similiar to some standard kernel
   functions, but define them here so we don't have dependencies on the
   kernel code. */

asm ushort_t db_fs(void)
{
	movw	%fs, %ax
}
#pragma asm partial_optimization db_fs

asm ushort_t db_gs(void)
{
	movw	%gs, %ax
}
#pragma asm partial_optimization db_gs

asm void db_wfs(ulong_t val)
{
%mem val;
	movl	val, %eax
	movw	%ax, %fs
}
#pragma asm partial_optimization db_wfs

asm void db_wgs(ulong_t val)
{
%mem val;
	movl	val, %eax
	movw	%ax, %gs
}
#pragma asm partial_optimization db_wgs

asm ulong_t db_cr0(void)
{
	movl	%cr0, %eax
}
#pragma asm partial_optimization db_cr0

asm ulong_t db_cr2(void)
{
	movl	%cr2, %eax
}
#pragma asm partial_optimization db_cr2

asm ulong_t db_cr3(void)
{
	movl	%cr3, %eax
}
#pragma asm partial_optimization db_cr3

asm ulong_t db_cr4(void)
{
	movl	%cr4, %eax
}
#pragma asm partial_optimization db_cr4

asm void db_wcr4(int val)
{
%reg val;
	movl	val, %cr4
%mem val;
	movl	val, %eax
	movl	%eax, %cr4
}
#pragma asm partial_optimization db_wcr4

asm ulong_t db_dr0(void)
{
	movl	%dr0, %eax
}
#pragma asm partial_optimization db_dr0

asm ulong_t db_dr1(void)
{
	movl	%dr1, %eax
}
#pragma asm partial_optimization db_dr1

asm ulong_t db_dr2(void)
{
	movl	%dr2, %eax
}
#pragma asm partial_optimization db_dr2

asm ulong_t db_dr3(void)
{
	movl	%dr3, %eax
}
#pragma asm partial_optimization db_dr3

asm ulong_t db_dr6(void)
{
	movl	%dr6, %eax
}
#pragma asm partial_optimization db_dr6

asm ulong_t db_dr7(void)
{
	movl	%dr7, %eax
}
#pragma asm partial_optimization db_dr7

asm void db_wdr0(ulong_t val)
{
%mem	val;
	movl	val, %eax
	movl	%eax, %dr0
}
#pragma asm partial_optimization db_wdr0

asm void db_wdr1(ulong_t val)
{
%mem	val;
	movl	val, %eax
	movl	%eax, %dr1
}
#pragma asm partial_optimization db_wdr1

asm void db_wdr2(ulong_t val)
{
%mem	val;
	movl	val, %eax
	movl	%eax, %dr2
}
#pragma asm partial_optimization db_wdr2

asm void db_wdr3(ulong_t val)
{
%mem	val;
	movl	val, %eax
	movl	%eax, %dr3
}
#pragma asm partial_optimization db_wdr3

asm void db_wdr6(ulong_t val)
{
%mem	val;
	movl	val, %eax
	movl	%eax, %dr6
}
#pragma asm partial_optimization db_wdr6

asm void db_wdr7(ulong_t val)
{
%mem	val;
	movl	val, %eax
	movl	%eax, %dr7
}
#pragma asm partial_optimization db_wdr7

asm void db_flushtlb(void)
{
	movl	%cr3, %eax
	movl	%eax, %cr3
}
#pragma asm partial_optimization db_flushtlb

asm void db_outl(ulong_t port, ulong_t val)
{
%mem	port,val;
	movl	port, %edx
	movl    val, %eax
	outl	(%dx)
}
#pragma asm partial_optimization db_outl

asm void db_outw(ulong_t port, ulong_t val)
{
%mem	port,val;
	movl	port, %edx
	movl    val, %eax
	outw	(%dx)
}
#pragma asm partial_optimization db_outw

asm void db_outb(ulong_t port, ulong_t val)
{
%mem	port,val;
	movl	port, %edx
	movl    val, %eax
	outb	(%dx)
}
#pragma asm partial_optimization db_outb

asm int db_inl(ulong_t port)
{
%mem	port;
	movl	port, %edx
	inl	(%dx)
}
#pragma asm partial_optimization db_inl

asm int db_inw(ulong_t port)
{
%mem	port;
	subl    %eax, %eax
	movl	port, %edx
	inw	(%dx)
}
#pragma asm partial_optimization db_inw

asm int db_inb(ulong_t port)
{
%mem	port;
	subl    %eax, %eax
	movl	port, %edx
	inb	(%dx)
}
#pragma asm partial_optimization db_inb


extern void kdb_printf(const char *fmt, VA_LIST ap);
extern boolean_t kdb_check_aborted(void);
extern void kdb_register(struct kdebugger *);
extern void kdb_unregister(struct kdebugger *);
extern void kdb_online(void);
extern void kdb_next_debugger(void);
extern boolean_t kdb_select_io(char *devname, minor_t minor);
extern void kdb_start_io(void);
extern void kdb_end_io(void);
extern void db_stacktrace(void (*)(), ulong_t, boolean_t, lwp_t *);
extern char *findsyminfo(vaddr_t, vaddr_t *, int *);
extern char *findsymname(vaddr_t, void (*)());
extern ulong_t findsymaddr(char *);
extern vaddr_t findsymval(vaddr_t);
extern void db_sym_and_off(vaddr_t, void (*)());
extern int adrtoext(vaddr_t);
extern void prassym(void (*)());

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KDB_KDEBUGGER_H */
