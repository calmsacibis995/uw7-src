#ident	"@(#)kern-i386:kernel.cf/Space.c	1.14.5.3"
#ident	"$Header$"

#include <config.h>	/* to collect tunable parameters */

#include <sys/types.h>
#include <sys/var.h>
#include <sys/param.h>

/* Enhanced Application Binary Compatibility */

#include <sys/sockmod.h>
#include <sys/osocket.h>

/* Enhanced Application Binary Compatibility */

#ifndef NUMSXT
#define NUMSXT	0
#endif
#ifndef XSDSEGS
#define XSDSEGS	0
#endif
#ifndef XSDSLOTS
#define XSDSLOTS 0
#endif

struct var	v = {
	NBUF,
	0,	/* NCALL is auto-tuned */
	0,	/* NPROC is auto-tuned */
	MAXCLSYSPRI,
	0,	/* MAXUP is auto-tuned */
	NHBUF,
	NHBUF-1,
	NPBUF,
	NAUTOUP,
	BUFHWM,
	/* XENIX Support */
	NSCRN,
	NEMAP,
	NUMSXT,
	XSDSEGS,
	XSDSLOTS,
	/* End XENIX Support */
	MAXULWP,
	/* XXX - make into real tunables: */
	1,	/* v_nonexclusive */
	100,	/* v_max_proc_exbind: a guess only */
	128,	/* v_static_sq */
};

boolean_t nullptr_default = NULLPTR;
#if NULLPTR == 2
boolean_t nullptr_log = B_TRUE;
#else
boolean_t nullptr_log = B_FALSE;
#endif

/*
 * This array lists all kernel routines which return structures.
 * This is needed to properly handle stack traces for the i386 family,
 * since the calling convention for such routines involves a net change
 * of the stack pointer.
 */
#include <sys/dl.h>
vaddr_t structret_funcs[] = {
	(vaddr_t)ladd,
	(vaddr_t)lsub,
	(vaddr_t)lmul,
	(vaddr_t)ldivide,
	(vaddr_t)lmod,
	(vaddr_t)lshiftl,
	(vaddr_t)lsign
};
size_t structret_funcs_size = sizeof structret_funcs;

/*
 * Misc parameter variables.
 */

#ifndef	CPURATE			
#define	CPURATE		16	/* minimum cpu rate in Mhz */
#endif

#ifndef	CPUSPEED		/* approximate speed of cpu in VAX MIPS */
#define	CPUSPEED	25	/* used for spin loops in various places */
				/* normalised to 100 Mhz                 */
#endif

#ifndef	I486_CPUSPEED		/* ditto for 25 Mhz 486 */
#define	I486_CPUSPEED	50
#endif

int	cpurate		= CPURATE;
int	lcpuspeed	= CPUSPEED;
int	i486_lcpuspeed	= I486_CPUSPEED;


/* Enhanced Application Binary Compatibility */

/* 
 * SCO socket emulation protocol translation structure.
 * It is used by osocket, a loadable driver.
 * This is here because it is initialized once only by /usr/eac/bin/initsock,
 * when the system goes multi-user.
 */
struct odomain *osoc_family = 0; 

/* end Enhanced Application Binary Compatibility */


/*
 * USER_RDTSC controls permission to use the RDTSC instruction to read
 * the Time Stamp Counter from user mode.  If non-zero, the user may
 * execute RDTSC on processors which support it (e.g. Pentium).
 */
boolean_t user_rdtsc = USER_RDTSC;

/*
 * USER_RDPMC controls permission to use the RDPMC instruction to read
 * the Performance Monitoring Counters from user mode.  If non-zero,
 * the user may execute RDPMC on processors which support it
 *  (currently P6 only).
 */
boolean_t user_rdpmc = USER_RDPMC;

/*
 * Define .extuvirt section - first section in kernel, used to "float"
 * the beginning of the kernel to make room for extended user virtual,
 * if enabled.  Note that .extuvirt must begin and end on a page table
 * boundary.
 */

#define	str(s)	#s
#define	xstr(s)	str(s)

/*
 * Needs to be on a page table boundary
 */
asm(".section .extuvirt, \"a\", \"nobits\"");
asm(".zero " xstr(UVIRT_EXTENSION) " << " xstr(PAGESHIFT));
