#ifndef _SVC_SYSI86_H	/* wrapper symbol for kernel use */
#define _SVC_SYSI86_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/sysi86.h	1.17.1.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/ioctl.h>		/* REQUIRED */

#elif !defined(_KERNEL)

#include <sys/p_sysi86.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#ifndef DIRSIZ
#define	DIRSIZ	14
#endif

/*
 * Commands for sysi86 system call (1-?)
 */

#define SI86NULLPTR	15	/* Enable/disable null-pointer workaround */
#define SI86FPHW        40      /* what (if any?) floating-point hardware */
#define SI86GCON	46	/* get real console device number	  */

/* 54 reserved for i386at platform */

#define SETNAME		56	/* rename the system */

/* 60 reserved for i386at platform */

#define SI86MEM         65      /* return the memory size of system */

#define SI86TODEMON     66      /* Transfer control to kernel debugger. */
#define SI86CCDEMON     67      /* Control character access to debugger. */

/* 71, 73 and 74 reserved for VPIX */

/* 72 reserved for i386at platform */

#define SI86DSCR        75      /* Set a segment or gate descriptor     */

/* 77 reserved for NFA */

#define SI86LIMUSER	91	/* obsolete */

/* 92 through 94 reserved for i386at platform */

/* XENIX Binary Support */
#define SI86SHFIL	100	/* map a file into addr space of a proc */
#define SI86PCHRGN	101	/* make globally visible change to a region */
#define SI86BADVISE	102	/* badvise subcommand - see below for   */
				/*	badvise subfunction definitions */
#define SI86SHRGN	103	/* enable/disable XENIX small model shared */
				/*	data context switching		   */ 
#define	SI86CHIDT	104	/* set user level int 0xf0, ... 0xff handlers */
#define	SI86EMULRDA	105	/* remove special emulator read access	*/
/* End XENIX Binary Support */

/*
 *	NOTE: Numbers 106 - 110 have been registered and are reserved
 *	for future use for AT&T hardware.
 */

#define SI86IOPL	112	/* change I/O privilege level */

#define SI86GETFEATURES 114     /* get os features vector (OSR5) */


/*
 *	Numbers 200-299 reserved for platform-specific functions.
 */


/*
 *  The SI86DSCR subcommand of the sysi86() system call
 *  sets a segment or gate descriptor in the kernel.
 *  The following descriptor types are accepted:
 *    - executable and data segments in the LDT at DPL 3
 *  The request structure declared below is used to pass the values
 *  to be placed in the descriptor.  A pointer to the structure is
 *  passed as the second argument of the system call.
 *  If acc1 is zero, the descriptor is cleared.
 */

/* request structure passed by user */
struct ssd {
	unsigned int sel;   /* descriptor selector */
	unsigned int bo;    /* segment base or gate offset */
	unsigned int ls;    /* segment limit or gate selector */
	unsigned int acc1;  /* access byte 5 */
	unsigned int acc2;  /* access bits in byte 6 or gate count */
};

/* XENIX Binary Support */
/*
 *  The SI86SHFIL subcommand of the sysi86() system call
 *  maps a file into a region in user address space.
 *  The request structure declared below is used to pass the
 *  system call parameters.  A pointer to the structure is
 *  passed as the second argument of the system call.
 */
struct mmf {
	char *	mf_filename;	/* path name of file */
	long	mf_filesz;	/* Size in bytes of section of file	*/
				/* from which this region is mapped.	*/
	long	mf_regsz;	/* Size of region in bytes */
	short	mf_flags;	/* Either 0 or RG_NOSHARE */
};

/*
 *  The SI86PCHRGN subcommand of the sysi86() system call
 *  change the memory mapped image of a file.
 *  The request structure declared below is used to pass the values
 *  system call parameters.  A pointer to the structure is
 *  passed as the second argument of the system call.
 */
struct cmf {
	char *	cf_srcva;	/* modified image address */
	char *	cf_dstva;	/* location to patch */
	long	cf_count;	/* size of patch */
};

/*
 * The SI86BADVISE subcommand of the sysi86() system call specifies
 * XENIX variant behavior for certain system calls and kernel routines.
 * The 'arg' argument of sysi86() for SI86BADVISE is an integer.  Bits
 * 8..15 specify SI86B_SET or SI86B_GET.  Bits 0..7 contain
 * SI86B_PRE_SV, SI86B_XOUT, or SI86B_XSDSWTCH.  All these constants are 
 * defined below.  The 'arg' argument thus consists of either SI86B_SET 
 * OR'ed with zero or more of SI86B_PRE_SV, SI86B_XOUT, and SI86B_XSDSWTCH,
 * or of SI86B_GET. 
 */

#define SI86B_SET		0x0100	/* set badvise bits      */
#define SI86B_GET		0x0200	/* retrieve badvise bits */	

#define SI86B_PRE_SV		0x0008	/* follow pre-System V x.out behavior */
#define SI86B_XOUT		0x0010 	/* follow XENIX x.out behavior */
#define SI86B_XSDSWTCH		0x0080	/* XENIX small model shared data    */
					/*	context switching enabled   */
/*
 *   The request structure declared below is used by the XENIX 286 emulator
 *   (/bin/x286emul) in conjunction with the SI86SHRGN subcommand of sysi86().
 *   The SI86SHRGN subcommand is used by the XENIX 286 emulator to support
 *   XENIX shared data.  The second argument passed to sysi86() is a 
 *   pointer to an xsdbuf struct.
 *
 *   If the 'xsd_cmd' field of xsdbuf is SI86SHR_CP, the XENIX 286 emulator is 
 *   using the SI86SHRGN subcommand to set up XENIX small model shared data 
 *   context switching support for a given XENIX shared data segment.  In this 
 *   case, the xsdbuf struct contains the start addr for the shared data in 
 *   386 space, followed by the start addr for the shared data in the 286 
 *   executable's private data.  
 *
 *   If the 'xsd_cmd' field is SI86SHR_SZ, the XENIX 286 emulator is using the 
 *   SI86SHRGN subcommand to retrieve the size of an existing XENIX shared 
 *   data segment.  In this case, the xsdbuf struct contains the start addr
 *   for the shared data in 386 space.
 *   The size of the shared data segment starting at 'xsd_386vaddr' will
 *   be returned in the 'xsd_size' field by sysi86().
 */

#define SI86SHR_CP	0x1	/* SI86SHRGN used for XENIX sd context switch */
#define SI86SHR_SZ	0x2	/* SI86SHRGN used to get XENIX sd seg size */

struct xsdbuf {
	unsigned xsd_cmd;	/* SI86SHRGN subcommand, either SI86SHR_CP 
				 * 	or SI86SHR_SZ.
				 */ 	
	char	*xsd_386vaddr;	/* Addr of "real" XENIX shared data seg in 
				 * 	the emulator.
				 */
	union {
	   char	*xsd_286vaddr;	/* Addr of XENIX shared data seg in the
				 * 	286 data portion of the emulator.
				 */
	   unsigned long xsd_size;/* Size of XENIX shared data seg */
	} xsd_un;
};
/* End XENIX Binary Support */


#if defined(_KERNEL)

/*
 * sysi86 system call arguments
 */

struct sysi86a {
	short    cmd;           /* these can be just about anything */
	union ioctl_arg arg;    /* use this for all new commands!   */
	long     arg2, arg3;    /* backward compatibility           */
};

#else

/* user level interface */
#if defined(__STDC__)
extern int sysi86(int, ...);
#else
extern int sysi86();
#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_SYSI86_H */
