#ifndef _IO_HBA_HBA_H		/* wrapper symbol for kernel use */
#define _IO_HBA_HBA_H		/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/hba.h	1.14.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef HBA_PREFIX
#error HBA_PREFIX must be defined
#endif

/* 
 * hba.h
 *
 * Description:
 *	This file contains defines useful for maintaining source
 *	compatibility for ongoing releases of PDI.  The MACROS 
 *	provided here hide the differences made to the PDI interface 
 *	and differences between a multi-threaded vs. 
 *	non-multithreading driver.
 *
 *	The use of these MACROS and version defines, make it possible
 *	to keep a single copy of the source for a SVR4.2/UnixWare driver 
 *	and a SVR4.2MP driver.
 *
 * How to use hba.h:
 *	To use hba.h, define HBA_PREFIX in your driver.h file:
 *	For example:
 *
 *		#define HBA_PREFIX dpt
 *
 *	and order the include files in the driver as follows:
 *
 *		#include	all other includes
 *		#include	your_driver.h
 *		#include	<sys/hba.h>
 *		#include	<sys/ddi.h>
 *		#include	<sys/ddi_386at.h>  (optional)
 *
 * Assumptions are made by these macros about the names of functions
 * and variables.  In particular: 
 *
 *		XXXidata             
 * 		XXX_attach_info      
 * 		XXX_cntls            
 * 		XXXclose             
 * 		XXXdevflag           
 * 		XXXfreeblk           
 * 		XXXgetblk            
 * 		XXXgetinfo           
 * 		XXXhalt
 * 		XXXinit              
 * 		XXXintr              
 * 		XXXioctl             
 * 		XXXopen              
 * 		XXXsend              
 * 		XXXstart             
 * 		XXXxlat              
 *              XXX_load
 *              XXX_unload
 *              XXXicmd
 *
 * The following block of comments further describe the version
 * definitions, and extentions/enhancements made to PDI, making
 * it necessary for these MACROS.
 *
 *------------------------------------
 *
 *  1) Source Compatibiliity
 *	The version differences summarized below affect all SVR4.2 and
 *	UnixWare 1.1 driver source moving to SVR4.2MP.  It is recommended 
 *	that a single shared source of the driver be used for all
 *	releases, with appropriate #if PDI_VERSION directives applied.
 *
 *	The following MACROS hide the sleepflag differences:
 *		HBACLOSE, HBAFREEBLK, HBAGETBLK, HBAGETINFO, HBAICMD,
 *		HBAIOCTL, HBAOPEN, HBASEND, HBAXLAT
 *
 *		SDI_GET, SDI_GETBLK, SDI_TRANSLATE
 *	and are used as the function names.
 *
 *	The following MACROS hide the hbaxlat function declaration
 *	differences:
 *		HBAXLAT_DECL, HBAXLAT_RETURN
 *	and are used as the function return type, and function return
 *	statement, respectively.
 *
 *	HBA_IDATA(hba_cntls) MACRO expands to code that initializes
 *	the version number field of the hbaidata structure.  This
 *	MACRO should be used in the hbastart routine, prior to calling
 *	sdi_gethbano() and sdi_register().  The MACRO argument is
 *	the number of controllers configured.
 *
 *	HBA_IDATA_STRUCT macro expands to the proper version of
 *	the hba_idata structure.
 *
 *      HBA_SENSE_COPY macro expands to code to copy sense data
 *      to the SCSI block for cached sense data.
 *
 *	PDI_FOUND macro prints out the found message in a standard
 *	format.  Under 2.0 and beyond the message only goes to
 *	putbuf.
 *
 *	HBA_AUTOCONF_IDATA macro expands to the appropriate
 *	autoconfig routines
 *
 *	HBA_IDATA_DECL macro expands to the proper declaration for
 *      the driver's idata structure.  This should be used in
 *	the driver's .h file or in all of the driver's .c files.
 *
 *	HBA_IDATA_DEF macro expands to the proper definition for
 *      the driver's idata structure.  This should be used exactly
 *      once in the driver
 *
 *      SDI_ACFREE expands to the appropriate call to sdi_acfree() when
 *	appropriate.
 *
 *      MOD_DRVATTACH expands to the appropriate call to mod_drvattach()
 *      when appropriate.
 *
 *      MOD_DRVDEATTACH expands to the appropriate call to mod_drvdeattach()
 *      when appropriate.
 *
 *	There are various other defines/MACROS that help hide the
 *	difference between a multithreaded driver and non-multithreaded
 *	driver, such as: LOCK, UNLOCK, LOCK_ALLOC, LKINFO_DECL.
 *
 *  2) PDI_VERSION is a #define made available to HBA drivers through sdi.h
 *	that should be used to conditionally compile code for extending 
 *	driver capability in ongoing releases.  The preferred usage is,
 *	for example:
 *
 *		#if PDI_VERSION >= PDI_SVR42MP
 *
 *	which means SVR4.2MP or later (where PDI_SVR42MP is defined to be 3).
 *
 *	The value of PDI_VERSION is:
 *		1	SVR4.2/UnixWare 1.1 Driver
 *		2	SVR4.2 DTMT (Maintainence Release) Driver
 *		3	SVR4.2MP Driver
 *		4	UnixWare 2.0 Driver
 *		5	UnixWare 2.1 Driver
 *
 *	A PDI_VERSION 1 driver conforms to the HBA interface 
 *	defined in the SVR4.2 Portable Driver Interface
 *	(PDI) Guide.  A PDI_VERSION 3 driver conforms to 
 *	the SVR4.2MP PDI Guide, which has the following 
 *	extentions to the HBA interface:
 *
 *	- sleep flag argument extension to certain HBA and SDI
 *	  interface routines. (More on this below.)
 *	- breakup control block extension added to hbagetinfo.
 *	- DMAable memory allocated with kmem_alloc_physcontig
 *
 *	NOTE: Since SVR4.2 DTMT was not an official release, the
 *	      differences associated with this version is not included.
 *
 *  3) Sleep flag
 *	The SDI and HBA interfaces were extended in SVR4.2MP to
 *	use a flag argument for those routines that may need to
 *	sleep.  Since certain HBA/SDI routines need to be called at
 *	interrupt level, a sleep flag was necessary to indicate
 *	whether a sleep could be done.  
 *	The HBA routines that have the additional flag argument
 *	include: getblk(), icmd(), send(), and xlat().
 *	The SDI routines that have the additional flag argument
 *	include: sdi_get(), sdi_getblk(), sdi_icmd(), sdi_send,
 *	and sdi_translate().
 *  
 *  4) Breakup 
 *	The SVR4.2MP DDI/DKI has a new improved breakup interface,
 *	buf_breakup().  A PDI_VERSION 3 driver provides the target
 *	driver with the breakup parameters through the SVR4.2MP
 *	extention defined for the HBA getinfo routine.
 *	Pass-thru has also to be made dual level to use buf_breakup()
 *	with the driver's taylor made breakup parameters.
 *
 *	The breakup code differences should be ifdef:
 *
 *		#if (PDI_VERSION >= PDI_SVR42MP)
 *			... new breakup code ...
 *		#else /* !(PDI_VERSION >= PDI_SVR42MP) */ /*
 *			... old breakup code ...
 *		#endif /* !(PDI_VERSION >= PDI_SVR42MP) */ /*
 *
 *  5) Memory allocation
 *	
 *	HBA drivers may need to allocate physically contiguous DMAable
 *	memory.  Past versions of the DDI/DKI were not explicit
 *	about how drivers were to get DMAable memory.  The lack
 *	of documentation and the lack of a DDI/DKI interface for
 *	allocating physically contiguous DMAable memory lead driver
 *	writers to the conclusion that kmem_alloc was the correct
 *	interface.
 *
 *	SVR4.0, SVR4.2, and UnixWare 1.1 kmem_alloc'ed memory is
 *	in fact DMAable, but only guaranteed to be physically
 *	contiguous on allocations of 1 page or less.  This worked
 *	not by design but because of the way KMA was implemented.
 *
 *	UnixWare 2.0 and SVR4.2MP kmem_alloc'ed memory is not
 *	guaranteed to be either DMAable or physically contiguous.
 *	This is by design.  A new DDI routine, kmem_alloc_physreq
 *	(kmem_alloc_physcontig in SVR4.2MP), is provided for
 *	drivers that need DMAable memory.
 *	
 *	Drivers that currently use kmem_alloc() to allocate DMA memory
 *	must convert to use kmem_alloc_physreq() for UnixWare 2.0
 *	(kmem_alloc_physcontig() for SVR4.2MP).
 *
 *	HBA drivers that are DDI compliant are binary compatible 
 *	with SVR4.2MP, and use kmem_alloc() for DMA memory will 
 *	still work unless they use large amounts of memory.  If they
 *	use [x]phystokv() or KBASE, however, they will only work
 *	with the PHYSTOKVMEM tunable set to 1 (See description below.)
 */

/*
 * The following defines PDI_VERSION for DTMT, and causes an
 * error for all others.
 */
#ifndef PDI_VERSION
#ifdef PDI_SVR42
#define PDI_VERSION	PDI_SVR42_DTMT
#else	/* !PDI_SVR42 */
#error PDI_VERSION not defined; unknown O/S release
#endif /* PDI_SVR42 */
#endif /* !PDI_VERSION */


/*
 * PDI_VERSION is defined as follows for the versions:
 *      UnixWare 1.1    1
 *      SVR4.2 DTMT     undefined by O/S; use 2
 *      SVR4.2 MP       3
 *      UnixWare 2.0    4
 *      UnixWare 2.1    5
 * 
 * These are defined in both sdi.h and hba.h 
 */
#ifndef PDI_UNIXWARE11
#define PDI_UNIXWARE11  1
#endif
#ifndef PDI_SVR42_DTMT
#define PDI_SVR42_DTMT  2
#endif
#ifndef PDI_SVR42MP
#define PDI_SVR42MP     3
#endif
#ifndef PDI_UNIXWARE20
#define PDI_UNIXWARE20  4
#endif
#ifndef PDI_UNIXWARE21
#define PDI_UNIXWARE21  5
#endif


/*
 * HBA_SENSE_COPY(sense, sbp)
 * Macro used to update the sense data in a struct sb.
 * The sense argument should be a (char *) to accurate sense
 * data.  The sbp argument should be the (struct sb *) of the
 * SCSI block that is to be updated.
 */

#if (PDI_VERSION < PDI_UNIXWARE20)
#define HBA_SENSE_COPY(sense, sbp)
#else
#define HBA_SENSE_COPY(sense, sbp) \
		bcopy((sense), (caddr_t)(sdi_sense_ptr(sbp)) + 1, SENSE_SZ)
#endif


#if (PDI_VERSION < PDI_UNIXWARE20)
#define PDI_FOUND(Name, Base, Slot, Interrupt) \
	cmn_err(CE_NOTE, "%s: found at I/O address 0x%x, slot %d, interrupt %d\n", \
		Name, Base, Slot, Interrupt)
#else
#define PDI_FOUND(Name, Base, Slot, Interrupt) \
	cmn_err(CE_NOTE, "!%s: found at I/O address 0x%x, slot %d, interrupt %d\n", \
		Name, Base, Slot, Interrupt)
#endif

#if (PDI_VERSION < PDI_UNIXWARE20)
#define HBA_AUTOCONF_IDATA()  LINT0FUNC()
#else
#define HBA_AUTOCONF_IDATA() \
 { \
	 glue(HBA_PREFIX, idata) = \
		sdi_hba_autoconf(HBA_STRING(HBA_PREFIX) , \
				 glue(_, glue(HBA_PREFIX, idata)), \
				 &glue(HBA_PREFIX, _cntls) ); \
         if (glue(HBA_PREFIX, idata) == NULL) return -1; \
 } 
#endif

#if (PDI_VERSION < PDI_UNIXWARE20)
#define HBA_IDATA_DECL() \
	 extern	 HBA_IDATA_STRUCT glue(HBA_PREFIX, idata)[]
#else
#define HBA_IDATA_DECL() \
	 extern	HBA_IDATA_STRUCT glue(_, glue(HBA_PREFIX, idata))[], \
		 *glue(HBA_PREFIX, idata)
#endif

#if (PDI_VERSION < PDI_UNIXWARE20)
#define HBA_IDATA_DEF() extern  HBA_IDATA_STRUCT glue(HBA_PREFIX, idata)[]
#else
#define HBA_IDATA_DEF() \
	HBA_IDATA_STRUCT *glue(HBA_PREFIX, idata) = NULL
#endif

#if (PDI_VERSION < PDI_UNIXWARE20)
#define SDI_ACFREE()  LINT0FUNC()
#else
#define SDI_ACFREE() \
		sdi_acfree( glue(HBA_PREFIX, idata), glue(HBA_PREFIX, _cntls))
#endif

#if (PDI_VERSION < PDI_UNIXWARE20)
#define MOD_DRVATTACH() \
		mod_drvattach(&glue(HBA_PREFIX, _attach_info))
#else
#define MOD_DRVATTACH()   LINT0FUNC()
#endif

#if (PDI_VERSION < PDI_UNIXWARE20)
#define MOD_DRVDETACH() \
		mod_drvdetach(&glue(HBA_PREFIX, _attach_info))
#else
#define MOD_DRVDETACH()   LINT0FUNC()
#endif

/*
 * HBA_IDATA(X)
 * Macro to be called in hbastart routine (prior to call to
 * sdi_gethbano() and sdi_register()). This Macro insures that
 * the correct version number is initialized in the idata structure.
 *
 *	X	number of controllers configured
 */
#define HBA_IDATA(X) \
	{ \
	int i; \
	for(i=0; i<X; i++) \
		glue(HBA_PREFIX, idata)[i].version_num =  \
		(glue(HBA_PREFIX, idata)[i].version_num & ~HBA_VMASK) | \
		PDI_VERSION; \
	}

/*
 *	HBA_IDATA_STRUCT
 *	macro that expands to the proper hba_idata structure.
 */

#if (PDI_VERSION < PDI_UNIXWARE20)
#define HBA_IDATA_STRUCT struct hba_idata
#endif
#if (PDI_VERSION == PDI_UNIXWARE20)
#define HBA_IDATA_STRUCT struct hba_idata_v4
#endif
#if (PDI_VERSION >= PDI_UNIXWARE21)
#define HBA_IDATA_STRUCT struct hba_idata_v5
#endif

/*
 * Define for limitation to the device size 
 * when physiock is used (prior to PDI_UNIXWARE20).  This
 * define can be used when the device is this size or greater.
 * For PDI_UNIXWARE20, a value of 0 for nblocks means unlimited.
 */
#if (PDI_VERSION < PDI_UNIXWARE20)

#define HBA_MAX_PHYSIOCK 4194303
#else
#define HBA_MAX_PHYSIOCK 0
#endif

/*
 * Includes for the following MACROS
 */
#ifdef _KERNEL_HEADERS

#include <fs/buf.h>			/* REQUIRED */
#include <io/target/scsi.h>		/* REQUIRED */
#include <proc/cred.h>			/* REQUIRED */
#include <proc/proc.h>			/* REQUIRED */
#if (PDI_VERSION >= PDI_SVR42MP)
#include <util/engine.h>		/* REQUIRED */
#include <util/ksynch.h>		/* REQUIRED */
#endif	/* (PDI_VERSION >= PDI_SVR42MP) */

#elif defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/buf.h>			/* REQUIRED */
#include <sys/scsi.h>			/* REQUIRED */
#include <sys/cred.h>			/* REQUIRED */
#include <sys/proc.h>			/* REQUIRED */
#if (PDI_VERSION >= PDI_SVR42MP)
#include <sys/engine.h>			/* REQUIRED */
#include <sys/ksynch.h>			/* REQUIRED */
#endif	/* (PDI_VERSION >= PDI_SVR42MP) */

#endif /* _KERNEL || _KMEMUSER */

/*
 *
 * Drivers that are multi-threaded, but compiled for UnixWare 1.1
 * continue to work with the following MACROS. 
 * (These are already defined in ddi.h for >= SVR4.2MP)
 * (These are already defined in sdi_comm.h for == SVR4.2 DTMT)
 *
 */
#if (PDI_VERSION < PDI_SVR42_DTMT)

extern int spl5s(), splx();

#define	LKINFO_DECL(l,s,f)		int l
#define	LOCK_ALLOC(h,i,p,f)		((lock_t *)1)

#define	LOCK_DEALLOC(l)		        LINT1FUNC(l)

#define	UNLOCK(lockp, ipl)		splx(ipl)
#define	LOCK(lockp, ipl)		spl5s()
#define	_SDI_LOCKED	0x1
#define	_SDI_SLEEPING	0x2
#define	SLEEP_ALLOC(h,i,f)		(int *)kmem_zalloc(sizeof(int), f)
#define	SLEEP_LOCK(lockp, pri)		{ \
				while ( *(lockp) & _SDI_LOCKED ) { \
					*(lockp) |= _SDI_SLEEPING; \
					sleep(lockp, pri); \
				} \
				*(lockp) |= _SDI_LOCKED; }

#define	SLEEP_UNLOCK(lockp)		{ \
				*(lockp) &= ~_SDI_LOCKED; \
				if (*(lockp) & _SDI_SLEEPING) { \
					*(lockp) &= ~_SDI_SLEEPING; \
					wakeup(lockp); \
				} }

#define	RW_WRLOCK(lockp, ipl)		LOCK(lockp, ipl)
#define	RW_RDLOCK(lockp, ipl)		LOCK(lockp, ipl)
#define	RW_UNLOCK(lockp, ipl)		UNLOCK(lockp, ipl)
#define	TRYLOCK(lockp, ipl)		LOCK(lockp, ipl)
#define	SV_ALLOC(f)			(int *)kmem_zalloc(1, f)
#define	SV_DEALLOC(svp)			kmem_free((void *)svp, 1)

#define	SV_WAIT(svp, pri, lockp)	{ (void)spl0(); \
				          (void)sleep((caddr_t)svp, pri); }
#define	SV_BROADCAST(svp, flags)	wakeup((caddr_t)svp)

#define lock_t		int
#define rwlock_t	int
#define sleep_t		int
#define sv_t		int
#define	pl_t		int
#define bcb_t		char
#define	spldisk()	spl5()

#ifndef PLDISK
#define	PLDISK	5
#endif

#define	ITIMEOUT(f, a, t, p)	timeout(f, a, t)

#define F_DMA_24	F_DMA		/* Device supports 24-bit DMA */
#define F_DMA_32	0x010		/* Device supports 32-bit DMA */

#endif	/* (PDI_VERSION < PDI_SVR42MP) */

#if (PDI_VERSION <= PDI_SVR42MP)
/*
 * Additional SCSI defines/structures that should be part of scsi.h
 * (These are defined in scsi.h for > SVR4.2MP)
 */
#define	SS_RDBLKLEN	0X05		/* Read block length limits	   */

/*
 * Block length limit data
 */
struct blklen {
	unsigned res1:8;	   /* Reserved			*/
	unsigned max_blen:24;	   /* maximum block length	*/
	unsigned min_blen:16;	   /* minimum block length	*/
};

#define RDBLKLEN_SZ       6 
#define RDBLKLEN_AD(x)     ((char *)(x))

/*
 * Mode data structure
 */
struct mode {
	unsigned md_len   :8;		/* Sense data length		*/
	unsigned md_media :8;		/* Medium type			*/
	unsigned md_speed :4;		/* Tape speed			*/
	unsigned md_bm    :3;		/* Buffering mode		*/
	unsigned md_wp    :1;		/* Write protected		*/
	unsigned md_bdl   :8;		/* Block descriptor length	*/
	unsigned md_dens  :8;		/* Density code			*/
	unsigned md_nblks :24;		/* Number of blocks		*/
	unsigned md_res   :8;		/* Reserved field		*/
	unsigned md_bsize :24;		/* Block size			*/
};
#endif /* (PDI_VERSION <= PDI_SVR42MP) */

#if (PDI_VERSION >= PDI_SVR42MP)
#define	ITIMEOUT(f, a, t, p) itimeout(f, a, t, p)
#endif	/* (PDI_VERSION >= PDI_SVR42MP) */

/*
 * MACROS for concatenation to generate 'HBA_PREFIX' func name
 * As an example, here's what's going on below:
 *	GETBLK -> glue(PREFIX, getblk) -> xglue(PREFIX, getblk) -> dptgetblk
 */
#define xglue(a, b) a ## b
#define glue(a,b) xglue (a, b)

/*
 * MACROS for puting double quotes about hopefully anything.
 * for example, if HBA_PREFIX is dpt then
 * HBA_STRING(HBA_PREFIX) -> XHBA_STRING(dpt) -> "dpt"
 */
#define HBA_STRING(X)   XHBA_STRING(X) 
#define XHBA_STRING(X)   #X

/*
 *
 * HBA interface MACROS
 *
 * The following MACROS are used for the HBA interface routines
 * When expanded, they provide the correct interface for the
 * given PDI_VERSION.
 * Extentions that were added to SVR4.2MP which are accounted for
 * in these MACROS include:
 *	1) Several routines have one additional argument, sleepflag.
 *	2) xlat has a return value, and the declaration changed from 
 *		void to int
 *
 * These MACROS require that the following define:
 *
 *	#define HBA_PREFIX your_driver_prefix
 *
 * be put in your_driver.h header file, and that the order of
 * include files be:
 *
 *	#include	all other includes
 *	#include	your_driver.h
 *	#include	<sys/hba.h>
 *	#include	<sys/ddi.h>
 *
 */

#if (PDI_VERSION <= PDI_UNIXWARE11)

#define HBACLOSE(dev, flags, otype, credp) glue(HBA_PREFIX, close)(dev, flags, otype, credp)
#define HBAFREEBLK(hbap) glue(HBA_PREFIX, freeblk)(hbap)
#define HBAGETBLK(sleepflag) glue(HBA_PREFIX, getblk)()
#define HBAGETINFO(addr, getinfop) glue(HBA_PREFIX, getinfo)(addr, getinfop)
#define HBAICMD(hbap, sleepflag) glue(HBA_PREFIX, icmd)(hbap)
#define HBAIOCTL(dev, cmd, arg, mode, credp, rvalp) glue(HBA_PREFIX, ioctl)(dev, cmd, arg, mode, credp, rvalp)
#define HBAOPEN(dev, flags, otype, credp) glue(HBA_PREFIX, open)(dev, flags, otype, credp)
#define HBASEND(hbap, sleepflag) glue(HBA_PREFIX, send)(hbap)
#define HBAXLAT(hbap, flag, procp, sleepflag) glue(HBA_PREFIX, xlat)(hbap, flag, procp)

#define HBAXLAT_DECL void
#define HBAXLAT_RETURN(x) return

#else /* (PDI_VERSION > PDI_UNIXWARE11) */

#define HBACLOSE(dev, flags, otype, credp) glue(HBA_PREFIX, close)(dev, flags, otype, credp)
#define HBAFREEBLK(hbap) glue(HBA_PREFIX, freeblk)(hbap)
#define HBAGETBLK(sleepflag) glue(HBA_PREFIX, getblk)(sleepflag)
#define HBAGETINFO(addr, getinfop) glue(HBA_PREFIX, getinfo)(addr, getinfop)
#define HBAICMD(hbap, sleepflag) glue(HBA_PREFIX, icmd)(hbap, sleepflag)
#define HBAIOCTL(dev, cmd, arg, mode, credp, rvalp) glue(HBA_PREFIX, ioctl)(dev, cmd, arg, mode, credp, rvalp)
#define HBAOPEN(dev, flags, otype, credp) glue(HBA_PREFIX, open)(dev, flags, otype, credp)
#define HBASEND(hbap, sleepflag) glue(HBA_PREFIX, send)(hbap, sleepflag)
#define HBAXLAT(hbap, flag, procp, sleepflag) glue(HBA_PREFIX, xlat)(hbap, flag, procp, sleepflag)

#define HBAXLAT_DECL int
#define HBAXLAT_RETURN(x) return (x)

#endif /* (PDI_VERSION > PDI_UNIXWARE11) */

/*
 * hba_idata version number definitions
 */
#ifndef HBA_IDATA_INC_PTR
#define HBA_SVR4_2      1       /* SVR4.2 driver release number */
#define HBA_SVR4_2_2    2       /* SVR4.2.2 driver release number */
#define HBA_SVR4_2MP    3       /* SVR4.2MP driver release number */
#define HBA_UW21_IDATA  5       /* Unixware 2.1 idata (ver. 5)  */

#define HBA_VMASK       0xffff  /* mask out flags to get version number */

/*
 * HBA_IDATA_INC_PTR
 * Macro that bumps the idata struct pointer depending on
 * the version number. This provides compatability with
 * drivers using a version 4 idata struct in Unixware 2.1.
 */
#define HBA_IDATA_INC_PTR(x) \
        (x) = (HBA_IDATA_STRUCT *)((char *)(x) +                \
              (((x)->version_num & HBA_VMASK) == HBA_UW21_IDATA ? \
              sizeof(struct hba_idata_v5) : sizeof(struct hba_idata_v4)))
#endif

/*
 *
 * SDI interface MACROS
 *
 * The following MACROS are used for the SDI interface routines
 * When expanded, they provide the correct interface for the
 * given PDI_VERSION.  The interface for SVR4.2MP include
 * a sleepflag to some of the routines.
 *
 * These MACROS require that the following define:
 *
 *	#define HBA_PREFIX your_driver_prefix
 *
 * be put in your_driver.h header file, and that the order of
 * include files be:
 *
 *	#include	all other includes
 *	#include	your_driver.h
 *	#include	<io/hba/hba.h>
 *	#include	<io/ddi.h>
 */
#if (PDI_VERSION <= PDI_UNIXWARE11)

#define SDI_GET(head, sleepflag) sdi_get(head, KM_SLEEP)
#define SDI_GETBLK(sleepflag) sdi_getblk()
#define SDI_TRANSLATE(sp, flag, procp, sleepflag) sdi_translate(sp, flag, procp)
#define SDI_INTR_ATTACH() 
#define SDI_ICMD(sp, sleepflag) sdi_icmd(sp)

#else /* (PDI_VERSION > PDI_UNIXWARE11) */

#define SDI_GETBLK(sleepflag) sdi_getblk(sleepflag)
#define SDI_TRANSLATE(sp, flag, procp, sleepflag) sdi_translate(sp, flag, procp, sleepflag)
#define SDI_INTR_ATTACH() \
	 sdi_intr_attach( \
			 glue(HBA_PREFIX, idata),\
			 glue(HBA_PREFIX, _cntls),\
			 glue(HBA_PREFIX, intr),\
			 glue(HBA_PREFIX, devflag))
#define SDI_ICMD(sp, sleepflag) sdi_icmd(sp, sleepflag)

#endif /* (PDI_VERSION > PDI_UNIXWARE11) */

#if (PDI_VERSION < PDI_UNIXWARE20)
/*
 * Multiple SCSI bus support added in UnixWare 2.0
 * Set new scsi_ad macros to the old ones for pre-UnixWare 2.0
 */
#define SDI_EXHAN SDI_HAN
#define SDI_EXTCN SDI_TCN
#ifdef SDI_EXLUN
#undef SDI_EXLUN
#endif
#define SDI_EXLUN SDI_LUN
#define SDI_BUS(X) 0

#define SC_EXHAN SC_HAN
#define SC_EXTCN SC_TCN
#define SC_EXLUN SC_LUN
#define SC_BUS(X)  0

#define sa_ct sa_fill
#define SDI_SA_CT(c,t)	(((c)<<3) | ((t) & 0x07))
#define SDI_CONTROL(x)  (((x)>>3) & 0x1f)
#define SDI_EXILLEGAL(c,t,l,b) (!sdi_redt(c,t,l))

#define SDI_REDT(hba, bus, target, lun) sdi_redt(hba, target, lun)
#else /* (PDI_VERSION >= PDI_UNIXWARE20) */
#define SDI_REDT(hba, bus, target, lun) sdi_rxedt(hba, bus, target, lun)
#endif /* (PDI_VERSION >= PDI_UNIXWARE20) */

#ifndef SDI_MAX_HBAS
#define SDI_MAX_HBAS MAX_HAS
#endif
/*
 * Prototypes for HBA driver entry points.
 * All versions supplied.
 */
#if (PDI_VERSION <= PDI_UNIXWARE11)

int			glue(HBA_PREFIX, init)(void);
int			glue(HBA_PREFIX, start)(void);
void			glue(HBA_PREFIX, intr)(unsigned int);
void			glue(HBA_PREFIX, halt)(void);
long			glue(HBA_PREFIX, send)(register struct hbadata *);
long			glue(HBA_PREFIX, icmd)(register struct hbadata *);
struct hbadata  *	glue(HBA_PREFIX, getblk)(void);
long			glue(HBA_PREFIX, freeblk)(register struct hbadata *);
void			glue(HBA_PREFIX, getinfo)(struct scsi_ad *,struct hbagetinfo *);
int			glue(HBA_PREFIX, open)(dev_t *, int, int, cred_t *);
int			glue(HBA_PREFIX, close)(dev_t, int, int, cred_t *);
int			glue(HBA_PREFIX, ioctl)(dev_t, int, caddr_t, int, cred_t *, int *);
HBAXLAT_DECL		glue(HBA_PREFIX, xlat)(register struct hbadata *, int, struct proc *);
int 			glue(HBA_PREFIX, _load)(void);
int			glue(HBA_PREFIX, _unload)(void);

#else /* (PDI_VERSION > PDI_UNIXWARE11) */

int			glue(HBA_PREFIX, init)(void);
int			glue(HBA_PREFIX, start)(void);
void			glue(HBA_PREFIX, intr)(unsigned int);
void			glue(HBA_PREFIX, halt)(void);
long			glue(HBA_PREFIX, send)(register struct hbadata *, int);
long			glue(HBA_PREFIX, icmd)(register struct hbadata *, int);
struct hbadata  *	glue(HBA_PREFIX, getblk)(int);
long			glue(HBA_PREFIX, freeblk)(register struct hbadata *);
void			glue(HBA_PREFIX, getinfo)(struct scsi_ad *,struct hbagetinfo *);
int			glue(HBA_PREFIX, open)(dev_t *, int, int, cred_t *);
int			glue(HBA_PREFIX, close)(dev_t, int, int, cred_t *);
int			glue(HBA_PREFIX, ioctl)(dev_t, int, caddr_t, int, cred_t *, int *);
HBAXLAT_DECL		glue(HBA_PREFIX, xlat)(register struct hbadata *, int, struct proc *, int);
int 			glue(HBA_PREFIX, _load)(void);
int			glue(HBA_PREFIX, _unload)(void);

#endif /* (PDI_VERSION > PDI_UNIXWARE11) */


/*
 * SDI prototypes provided for UnixWare 1.1 drivers
 * (These are defined in sdi.h for > UnixWare 1.1)
 */
#if (PDI_VERSION <= PDI_UNIXWARE11)
#if defined(_KERNEL) 

extern int 		sdi_access(struct sdi_edt *, int, struct owner *);
extern void 		sdi_blkio(buf_t *, unsigned int, void (*)());
extern void 		sdi_callback(struct sb *);
extern void 		sdi_clrconfig(struct owner *, int, void (*)());
extern struct owner *	sdi_doconfig(struct dev_cfg[], int, char *, 
				struct drv_majors *, void (*)());
extern void		sdi_errmsg(char *, struct scsi_ad *, struct sb *,
				struct sense *, int, int);
extern struct dev_spec * sdi_findspec(struct sdi_edt *, struct dev_spec *[]);
extern void		sdi_free(struct head *, struct jpool *);
extern long		sdi_freeblk(struct sb *);
extern struct jpool	*sdi_get(struct head *, int);
extern struct sb	*sdi_getblk();
extern void		sdi_getdev(struct scsi_ad *, dev_t *);
extern int		sdi_gethbano(int);
extern int		sdi_register(void *, void *);
extern int		sdi_icmd(struct sb *);
extern int		sdi_send(struct sb *);
extern short		sdi_swap16(uint);
extern int		sdi_swap24(uint);
extern long		sdi_swap32(ulong);
extern struct sdi_edt	*sdi_redt(int, int, int);
extern void		sdi_name(struct scsi_ad *, char *);
extern void		sdi_translate(struct sb *, int, proc_t *);
extern int		sdi_wedt(struct sdi_edt *, int, char *);

extern int		sdi_open(dev_t *, int, int, cred_t *);
extern int		sdi_close(dev_t , int, int, cred_t *);
extern int		sdi_ioctl(dev_t, int, caddr_t, int, cred_t *, int *);

#endif /* _KERNEL */
#endif /* (PDI_VERSION <= PDI_UNIXWARE11) */

/* fake functions to shut lint up */
#ifdef lint
#define LINT0FUNC() (void)lint0func()
#define LINT1FUNC(x1) (void)lint1func((int)x1)
#define LINT2FUNC(x1, x2) (void)lint2func((int)x1, (int)x2)

static int lint0func();
static int lint1func(int x);
static int lint2func(int x1, int x2);

static int lint0func()
{
    return lint0func();
}
static int lint1func(int x)
{
    return lint1func(x);
}
static int lint2func(int x1, int x2)
{
    return lint2func(x1, x2);
}
#else

#define LINT0FUNC()
#define LINT1FUNC(x1)
#define LINT2FUNC(x1, x2)

#endif

#if defined(__cplusplus)
	}
#endif


#endif /* _IO_HBA_HBA_H */		/* wrapper symbol for kernel use */
