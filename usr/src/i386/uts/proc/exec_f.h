#ifndef _PROC_EXEC_F_H	/* wrapper symbol for kernel use */
#define _PROC_EXEC_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/exec_f.h	1.13.1.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Architecture Family specific (i86 processor family) exec header file.
 */

/* uarg.flags definitions. */
#define RINTP	  0x1		/* A run-time interpreter is active. */
#define EMULA	  0x2		/* Invoking emulator (i286x, dosx). */

/*
 * Magic number for intp.  This is defined here to keep
 * the code in proc/obj/intp.c machine independent.
 */
#define	INTPMAGIC 0x2123	/* Lilliputians, (0x2321 for Blefuscuans) */


/* XENIX SUPPORT */

/* Enhanced Application Compatibility Support */

/* Defines for u_renv2 flags */
#define SCO_SHNSL	0x1             /* Uses static shared libnsl */
#define SCO_USES_SHNSL	(RENV2 & SCO_SHNSL)

#define ISC_POSIX	0x2
#define ISC_USES_POSIX	(u.u_procp->p_execinfo && (RENV2 & ISC_POSIX))

#define RE_ISSCO	0x4		/* the executable is a sco executable */
					/* currently set on any COFF, and */	
					/* specially marked ELFs. */
#define isSCO	(u.u_procp->p_execinfo && (RENV2 & RE_ISSCO))

		
		

/* End Enhanced Application Compatibility Support */


/*
 * defines for bits 24-31 of ex_renv and various macros for accessing
 * fields of ex_renv.  All bits not currently defined are reserved
 * for future expansion.
 */
#define RE_ISXOUT	0x0000000
#define RE_ISCOFF	0x1000000
#define RE_ISELF	0x2000000
#define	RE_RENVMASK	0x3000000	/* runtime environment bits */

#define	RE_CPUTYPE	(XC_CPU << 16)
#define	RE_IS386	(XC_386 << 16)
#define	RE_ISWSWAP	(XC_WSWAP << 16)

/* binary types */
#define	isCOFF		(u.u_procp->p_execinfo && ((RENV & RE_RENVMASK) == RE_ISCOFF))	/* 386 COFF */
#define	isXOUT		(u.u_procp->p_execinfo && ((RENV & RE_RENVMASK) == RE_ISXOUT))	/* 386 x.out */

#define	IS386()		(u.u_procp->p_execinfo && (((RENV >> 16) & XC_CPU) == XC_386))

/*
 * The following define is used to indicate that the program being
 * exec'd is one of the 286 emulators.  This bit is set in ex_renv.
 */
#define	RE_EMUL		0x4000000	/* 286 emulator */
#define	is286EMUL	(RENV & RE_EMUL)

/*
 * Defines for badvise bits of ex_renv and various macros for accessing
 * these bits.  
 */

#define UB_PRE_SV	0x8000000	/* badvise pre-System V */
#define UB_XOUT		0x10000000	/* badvise x.out */
#define UB_LOCKING	0x20000000	/* badvise locking() system call  */
					/*      (for kernel use only)     */
#define UB_FCNTL	0x40000000	/* badvise fcntl() system call    */
					/*      (for kernel use only)     */
#define UB_XSDSWTCH	0x80000000	/* badvise XENIX shared data context */
					/*	switching		     */

				/* badvise indicates x.out behavior */
#define BADVISE_XOUT	(u.u_procp->p_execinfo && ((RENV & UB_XOUT) == UB_XOUT))

				/* badvise indicates pre-System V behavior */
#define BADVISE_PRE_SV	((RENV & UB_PRE_SV) == UB_PRE_SV)
				/* badvise indicates XENIX locking() call */
#define ISLOCKING	((RENV & UB_LOCKING) == UB_LOCKING)
				/* badvise indicates fcntl() call */
#define ISFCNTL		((RENV & UB_FCNTL) == UB_FCNTL)
				/* badvise indicates XENIX shared data
				 * 	context switching is enabled
				 */
#define BADVISE_XSDSWTCH ((RENV & UB_XSDSWTCH) == UB_XSDSWTCH)
				/* x.out binary or badvise indicates x.out */
#define VIRTUAL_XOUT	(isXOUT || BADVISE_XOUT)

/* End XENIX Support */


#ifdef _KERNEL

/*
 * The execpoststack() macro is a machine dependent encapsulation of
 * postfix processing to hide the stack direction from elf.c,
 * thereby making this portion of the elf.c code machine independent.
 */
#define execpoststack(ARGS, ARRAYADDR, BYTESIZE)  \
	(copyout((ARRAYADDR), (caddr_t)(ARGS)->auxaddr, BYTESIZE) ? EFAULT : \
		(((ARGS)->auxaddr += (BYTESIZE)), 0))

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_EXEC_F_H */
