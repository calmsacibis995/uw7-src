#ifndef _SVC_TRAP_H	/* wrapper symbol for kernel use */
#define _SVC_TRAP_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/trap.h	1.5"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Trap type values
 */

#define	DIVERR		0	/* divide by 0 error		*/
#define	SGLSTP		1	/* single step			*/
#define	NMIFLT		2	/* NMI				*/
#define	BPTFLT		3	/* breakpoint fault		*/
#define	INTOFLT		4	/* INTO overflow fault		*/
#define	BOUNDFLT	5	/* BOUND instruction fault	*/
#define	INVOPFLT	6	/* invalid opcode fault		*/
#define	NOEXTFLT	7	/* extension not available fault*/
#define	DBLFLT		8	/* double fault			*/
#define	EXTOVRFLT	9	/* extension overrun fault	*/
#define	INVTSSFLT	10	/* invalid TSS fault		*/
#define	SEGNPFLT	11	/* segment not present fault	*/
#define	STKFLT		12	/* stack fault			*/
#define	GPFLT		13	/* general protection fault	*/
#define	PGFLT		14	/* page fault			*/
#define	EXTERRFLT	16	/* extension error fault	*/
#define ALIGNFLT	17	/* alignment fault		*/
#define	MCEFLT		18	/* machine check exception	*/

#define USERFLT		0x100	/* value to OR if user trap	*/

#define TRP_PREEMPT	0x200	/* software redispatch		*/
#define TRP_UNUSED	0x201	/* unused trap/interrupt	*/

/*
 *  Values of error code on stack in case of page fault 
 */

#define	PF_ERR_MASK	0x01	/* Mask for error bit */
#define PF_ERR_PAGE	0	/* page not present */
#define PF_ERR_PROT	1	/* protection error */
#define PF_ERR_WRITE	2	/* fault caused by write (else read) */
#define PF_ERR_USER	4	/* processor was in user mode
					(else supervisor) */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_TRAP_H */
