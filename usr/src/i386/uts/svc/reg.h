#ifndef	_SVC_REG_H	/* wrapper symbol for kernel use */
#define	_SVC_REG_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/reg.h	1.16"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif
#ifndef _KERNEL

#include <sys/regset.h> /* SVR4.0COMPAT */

#endif

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Defines location of the users' stored registers relative to EAX.
 * Usage is u.u_ar0[XX].
 *
 *
 * All kernel entries are thru a trap/interrupt stack frame.
 * This is SW extended to save/restore registers, etc.
 *
 * The error code pushed on some traps is popped into l.trap_err_code
 * to make the stack frame consistent.
 *
 * A push-all instruction is used to push all user general registers,
 * thus syscall() and trap() handler can have arbitrary call frames.
 *
 * Once in syscall/trap handler, stack looks like:
 *
 *	old SS, padded		only if inter-segment (user->kernel)
 *	old SP			only if inter-segment (user->kernel)
 *	flags
 *	CS, padded		sense user-mode entry from RPL field
 *	EIP			return context
 *	DS, padded		user DS segment register
 *	ES, padded		user ES segment register
 *	EAX			scratch registers
 *	ECX			ditto
 *	EDX			more such
 *	EBX			register variable
 *	ESP			kernel ESP during trap (from push-all)
 *	EBP			of interrupted frame
 *	ESI			register variable
 *	EDI			register variable
 *	trap-type		(trap only)
 *
 *	EIP			return address
 *	XXX			syscall or trap call frame
 */

/*
 * The namespace defined here has the "T_" prefix to avoid the collision
 * with SVR4.0 regset namespace.  These "T_" symbols may be used by some
 * (non-driver) binary kernel modules (e.g. for MERGE386) and must thus
 * have their values preserved for binary compatibility.
 */

#define	T_SS	(7)
#define	T_UESP	(6)
#define	T_EFL	(5)
#define	T_CS	(4)
#define	T_EIP	(3)
#define T_DS	(2)
#define T_ES	(1)
#define	T_EAX	(0)
#define	T_ECX	(-1)
#define	T_EDX	(-2)
#define	T_EBX	(-3)
/*		(-4)	unused  (from ESP in push-all) */
#define	T_EBP	(-5)
#define	T_ESI	(-6)
#define	T_EDI	(-7)
#define T_TRAPNO (-8)

/*
 * Offsets from SP to registers that must be accessed in assembly code.
 * Assumes SP points to EDI in the above trap frame.
 */

#define	SP_EIP		(T_EIP-T_EDI)
#define	SP_CS		(T_CS-T_EDI)
#define	SP_EAX		(T_EAX-T_EDI)
#define	SP_EFL		(T_EFL-T_EDI)

/*
 * Offset from the bottom of the kernel stack to EAX
 * (used by the SET_U_AR0() macro).
 */
#define	U_EAX		(T_SS-T_EAX+1)
/*
 * Offset from the bottom of the kernel stack to the top of the syscall
 * trap frame.
 */
#define U_EDI           (T_SS-T_EDI+1)

/*
 * Interrupts save scratch registers in a consistent order with a push-all;
 * save entry PL below this.  Thus, after interrupt entry, stack looks like:
 *
 *	old SS, padded		only if inter-segment (user->kernel)
 *	old SP			only if inter-segment (user->kernel)
 *	flags
 *	CS, padded		sense user-mode entry from RPL field
 *	IP			return context
 *	DS, padded		user DS segment register
 *	ES, padded		user ES segment register
 *	EAX			scratch registers
 *	ECX			ditto
 *	EDX			more such
 *	old PL			PL value on entry  (esp points here)
 */

/*
 * Offsets from SP to registers that must be accessed in assembly code.
 * Assumes SP points to the base of the above interrupt frame (old PL).
 */
#define	INTR_SP_CS	(T_CS - (T_EDX - 1))
#define INTR_SP_IP	(T_EIP - (T_EDX - 1))

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
        }
#endif
#endif /* _SVC_REG_H */
