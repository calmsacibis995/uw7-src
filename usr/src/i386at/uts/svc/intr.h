#ifndef _SVC_INTR_H	/* wrapper symbol for kernel use */
#define _SVC_INTR_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/intr.h	1.6"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL

/*
 * Table for constructing interrupt descriptor table (IDT).
 */
struct idt_init {
	int	idt_desc;		/* fault index */
	int	idt_type;		/* gate type */
	void	(*idt_addr)();		/* address */
	int	idt_priv;		/* privilege */
}; 

/*
 * Interrupt vector and pseudo-trap numbers.
 */

#define	DEVINTS		64	/* start of device interrupt	*/

extern void (*nmi_handler)();

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_INTR_H */
