#ident	"@(#)stand:i386at/boot/blm/memsizer.c	1.13.1.2"
#ident	"$Header$"

/*
 * Platform-specific memory sizing.
 */

#include <boot.h>
#include <bioscall.h>

#define RESERVED0	0x800	/* # bytes at address 0 reserved for BIOS */
#define _512K		(512 * 1024)
#define _640K		(640 * 1024)
#define _1M		(1024 * 1024)
#define MINMEM		(15 * _1M)	/* sane configurations must have
					 * some memory above MINMEM */

STATIC void memsize(void);

STATIC struct L_memsizer _L_memsizer = {
	memsize
};

ulong_t extmem_sz;	/* size of "extended memory" (1st piece at 1M) */
ulong_t extmem_limit;	/* highest address reportable as "extended memory" */

STATIC int e820_memsize(void);
STATIC int mca_memsize(void);
STATIC int eisa_memsize(void);

#ifdef MEMDEBUG
STATIC char memdbgbuf[100];
STATIC char memdebug_param[] = "MEMDEBUG";
STATIC int first_time = 1;

STATIC void
memdebug(void)
{
 if (first_time)
  param_set(memdebug_param, memdbgbuf, PARAM_HIDDEN|PARAM_RDONLY|PARAM_APPEND);
}
#endif /* MEMDEBUG */


void
memsizer_start(void)
{
	L_memsizer = &_L_memsizer;
}

STATIC void
memsize(void)
{
	struct biosregs regs;
	ulong_t basemem_sz;

	/*
	 * We MUST add a free chunk first, in order for maddmem() to be
	 * able to allocate memory segment structures, since the caller
	 * may have done an mreset().
	 *
	 * We start with base memory (memory below 1MB, contiguous from 0),
	 * since it's easiest to find.
	 */

	/* Standard BIOS function for "base memory" (0-640K) */
	regs.intnum = 0x12;	/* Get base memory size BIOS function */
	(void)bioscall(&regs);
	basemem_sz = regs.ax * 1024;

	if (basemem_sz < 512 * 1024) {
		printf("WARNING: basemem too small; setting to 512K\n");
		basemem_sz = 512 * 1024;
	}

	/*
	 * Make base memory available, except for a small reserved portion.
	 * Must come before any allocations so there's memory available to
	 * allocate.
	 */
	maddmem(RESERVED0, basemem_sz - RESERVED0, NULL);

#ifdef MEMDEBUG
/* WARNING: Must come after first maddmem(). */
sprintf(memdbgbuf, "base:%uK", regs.ax);
memdebug();
#endif

	/* Reserved: real-address mode vectors and BIOS data area. */
	maddmem(0, RESERVED0, reserved_use);

	/*
	 * Unless we only have 512K base memory, assume remainder up to 640K
	 * actually exists, but is reserved.
	 */
	if (basemem_sz < _640K && basemem_sz > _512K)
		maddmem(basemem_sz, _640K - basemem_sz, reserved_use);

	/*
	 * Next we have to find all the (possibly discontiguous) extended
	 * memory (memory above 1MB).
	 *
	 * First see if the new Query System Address MAP BIOS call is avail.
	 */
	if (e820_memsize())
		goto done;

	/* Next, try the MCA-style Return Memory-Map Information */
	if ((bios_confp[CONF_FEATURE2] & CONF_MEMORY) && mca_memsize())
		goto done;

	/*
	 * Try to get a reliable size for "extended memory"; that is,
	 * the contiguous piece starting at 1MB. First try the "E801"
	 * advanced memory size BIOS function, but don't trust it to
	 * have the whole picture.
	 *
	 * Set extmem_limit to the upper end of the range that could
	 * be reported as extended memory, so we can validate memory
	 * reported by the EISA BIOS. For E801, this limit is 64MB.
	 * For the standard AT BIOS function 8800, the limit is either
	 * 16MB or 64MB, depending on the BIOS writer's interpretation;
	 * so assume 16MB unless it actually reports memory above 16MB.
	 */
	extmem_sz = 0;
	extmem_limit = 16 * _1M;
	regs.intnum = 0x15;	/* System BIOS functions */
	regs.ax = 0xE801;
	if ((bioscall(&regs) & EFL_C) == 0) {
		ulong_t ext2_base, ext2_sz, invalid;

		/*
		 * The "ax" value from E801 is supposed to report
		 * memory between 1MB and 16MB. Don't trust E801 if this
		 * value is out of range. Take advantage of the fact that
		 * we only support systems with at least ~16M.
		 */
		invalid = (regs.ax * 1024 > 15 * _1M ||
			   regs.ax * 1024 < 14 * _1M);
#ifdef MEMDEBUG
sprintf(memdbgbuf, "e801:%s%s%uK@1M %uK@16M",
		   invalid ? "F" : "",
		   (regs.ax != regs.cx || regs.bx != regs.dx) ? "M" : "",
		   regs.ax, regs.bx * 64);
memdebug();
#endif
		if (invalid)
			goto bad_e801;

		extmem_sz = regs.ax * 1024;
		extmem_limit = 64 * _1M;
		ext2_base = 16 * _1M;
		ext2_sz = regs.bx * 0x10000;
		if (extmem_sz == 15 * _1M) {
			if ((extmem_sz += ext2_sz) > 63 * _1M) {
				extmem_sz = 63 * _1M;
				ext2_base = 64 * _1M;
				ext2_sz -= 48 * _1M;
			} else
				ext2_sz = 0;
		}
		if (ext2_sz != 0)
			maddmem(ext2_base, ext2_sz, NULL);
	}
bad_e801:

	/*
	 * Use the AT standard BIOS function for "extended memory",
	 * which reports the amount of continguous memory starting at
	 * 1MB, and see if it reports anything more than E801 did.
	 * On some machines, this only reports memory below 16MB.
	 */
	regs.ax = 0x8800;		/* get extended memory size */
	(void)bioscall(&regs);
	if (regs.ax * 1024 > extmem_sz) {
		extmem_sz = regs.ax * 1024;
		if (extmem_sz > 16 * _1M)
			extmem_limit = 64 * _1M;
	}
#ifdef MEMDEBUG
sprintf(memdbgbuf, "8800:%uK@1M", regs.ax);
memdebug();
#endif
	maddmem(0x100000, extmem_sz, NULL);

	/*
	 * Finally, try to get more info from the EISA configuration
	 * information, if avail. We do this last so we can sanity-check
	 * it with extmem_sz.
	 */
	if (bustypes & BUS_EISA)
		eisa_memsize();

done:	;
#ifdef MEMDEBUG
first_time = 0;
#endif
}

#define SMAP_SIG	0x534D4150	/* 'SMAP' */
#define SMAP_BASE	0
#define SMAP_BASE_HI	4
#define SMAP_SIZE	8
#define SMAP_SIZE_HI	12
#define SMAP_TYPE	16
#define   SMAP_MAINSTORE	1
#define   SMAP_RESERVED		2

STATIC int
e820_memsize(void)
{
	struct biosregs regs;
	char *membuf;
	ulong_t base, baseHI, size, sizeHI;
	int any_above_min = 0, any = 0;
	int failret;

	regs.intnum = 0x15;
	regs.ebx = 0;
	membuf = biosbuffer();

	do {
		regs.ax = 0xE820;
		regs.edx = SMAP_SIG;
		regs.di = (ulong_t)membuf;
		regs.ecx = 20;
		failret = ((bioscall32(&regs) & EFL_C) != 0);

		/*
		 * Check for failure from the BIOS call. Also make sure the
		 * signature and size match, but only check the first time
		 * around, in case BIOS only sets them the first time.
		 */
#ifdef MEMDEBUG
if (failret ||  regs.eax != SMAP_SIG || regs.ecx != 20) {
    sprintf(memdbgbuf, "e820:%s%s%sF", regs.eax != SMAP_SIG ? "S" : "",
			failret ? "C" : "", regs.ecx != 20 ? "20" : "");
    memdebug();
}
#endif
		if (failret ||
		    (!any && (regs.eax != SMAP_SIG || regs.ecx != 20)))
			break;

		any = 1;

		base = *(ulong_t *)&membuf[SMAP_BASE];
		baseHI = *(ulong_t *)&membuf[SMAP_BASE_HI];
		size = *(ulong_t *)&membuf[SMAP_SIZE];
		sizeHI = *(ulong_t *)&membuf[SMAP_SIZE_HI];
#ifdef MEMDEBUG
sprintf(memdbgbuf, "e820:base %u baseHI %u size %u sizeHI %u type %d",
	base, baseHI, size, sizeHI, *(ulong_t *)&membuf[SMAP_TYPE]);
memdebug();
#endif

		if (baseHI != 0 || sizeHI != 0 || size == 0 ||
		    *(ulong_t *)&membuf[SMAP_TYPE] != SMAP_MAINSTORE)
			continue;

		if (base < _1M) {
			/*
			 * Ignore any memory reported below 1MB. We know
			 * what memory below 1MB looks like, and some BIOSes
			 * report incorrect memory here. (In particular,
			 * the HP Vectra XU 5/90 with BIOS Version 1.07.03
			 * reports non-existent memory at 0xE0000-0xEFFFF.)
			 */
			continue;
		}

		if (base != 0 && size > -base)
			size = -base;

		maddmem(base, size, NULL);

		if (base + size - 1 > MINMEM)
			any_above_min = 1;

	} while (regs.ebx != 0);

	return any_above_min;
}

#define MCA_SYS1M	0x0A
#define MCA_SYS16M	0x0E

STATIC int
mca_memsize(void)
{
	struct biosregs regs;
	char *membuf;
	ulong_t nK;
	int any_above_min = 0;

	/* Do Return Memory-Map Information BIOS call */
	regs.intnum = 0x15;
	regs.ax = 0xC700;
	membuf = biosbuffer();
	regs.si = (ulong_t)membuf;
	if ((bioscall(&regs) & EFL_C) != 0)
		return 0;

	/* Register system memory between 1MB and 16 MB (in 1K units) */
	nK = *((ulong_t *)&membuf[MCA_SYS1M]);
	maddmem(0x100000, nK * 1024, NULL);
#ifdef MEMDEBUG
sprintf(memdbgbuf, "mca:%uK@1M", nK);
#endif
	if (0x100000 + nK * 1024 - 1 > MINMEM)
		any_above_min = 1;

	/* Register system memory between 16MB and 4GB (in 1K units) */
	nK = *((ulong_t *)&membuf[MCA_SYS16M]);
	maddmem(0x1000000, nK * 1024, NULL);
#ifdef MEMDEBUG
sprintf(memdbgbuf + strlen(memdbgbuf), " %uK@16M", nK);
memdebug();
#endif
	if (0x1000000 + nK * 1024 - 1 > MINMEM)
		any_above_min = 1;

	return any_above_min;
}

#define	EISA_NSLOTS		20	/* only check 20 slots */

/*
 * Bit masks defining DL register contents upon return from
 * EISA_CONFIG_BRIEF BIOS call.  Only one of interest for
 * the moment is whether the slot defines any memory entries.
 */
#define	EISA_HAS_MEMS		2

/* Bit masks defining memory info in EISA config structure below */
#define	EISA_MEM_WRITE		0x0001	/* bit 0 for writeable */
#define	EISA_MEM_TYPE		0x00018	/* bits 3-4 for mem type */
#define	EISA_MORE_LIST		0x00080 /* bit 7 for more-of-list indicator */
/* if this is set, then memory continues */ 

/* memory types masked out by EISA_MEM_TYPE above */
#define	EISA_MEM_SYS		0

/*
 * The config structure returned by the EXTENDED BIOS call contains
 * an array of structures describing memory usage on the slot.
 * There are up to 9, 7 bytes each, in this format.
 */
typedef struct {
	unsigned char memflgs;		/* see bit masks above */
	unsigned char sizeflags;	/* type of access -- unimportant */
	unsigned char startLO;
	unsigned char startMED;
	unsigned char startHIGH;	/* start address divided by 0x100 */
	unsigned char sizeLO;
	unsigned char sizeHIGH;		/* size in bytes divided by 0x400.
					 * A 0 value means a 64MB size.
					 */
} eisa_mem_info_t;

/*
 * This is the layout of the structure filled in by the EXTENDED
 * CONFIG bios call.
 */
typedef struct {
	char pad1[0x22];
	unsigned char flags;
	char pad2[0x50];
	eisa_mem_info_t memlist[9];
	char pad3[156];			/* total structure size is 320 bytes */
} eisa_config_t;

STATIC int
eisa_memsize(void)
{
	struct biosregs regs;
	char *membuf;
	eisa_config_t *eisa;
	eisa_mem_info_t *emp;
	int i, j;
	ulong_t nK, base, end, badext;
	int any_above_min = 0;

	badext = extmem_sz + _1M;
	membuf = biosbuffer();
	eisa = (eisa_config_t *)membuf;

	for (i = 0; i < EISA_NSLOTS; i++) {
		/*
		 * This many EISA BIOS calls can take a while;
		 * check for events so animation doesn't stall.
		 */
		check_event();

		regs.intnum = 0x15;	/* EISA BIOS call */
		regs.ax = 0xD800;	/* Read slot configuration info */
		regs.cx = i;		/* slot number */
		if ((bioscall(&regs) & EFL_C) != 0 ||
		    !(regs.dx & EISA_HAS_MEMS))
			continue;

		for (j = 0; j < (int)(regs.dx >> 8); j++) {
			/*
			 * This many EISA BIOS calls can take a while;
			 * check for events so animation doesn't stall.
			 */
			check_event();

			regs.intnum = 0x15;	/* EISA BIOS call */
			regs.ax = 0xD801; /* Read function configuration info */
			regs.cx = (j << 8) | i;	/* function, slot number */
			regs.si = (ulong_t)membuf;
			if ((bioscall(&regs) & EFL_C) != 0 ||
			    !(eisa->flags & EISA_HAS_MEMS))
				continue;	/* try next function */

			emp = &eisa->memlist[0];
			do {
				if ((emp->memflgs & EISA_MEM_TYPE) !=
							EISA_MEM_SYS ||
				    !(emp->memflgs & EISA_MEM_WRITE))
					continue;

				base = ((emp->startHIGH << 16) +
					(emp->startMED << 8) +
					 emp->startLO) * 0x100;

				if (base < _1M)
					continue;

				nK = (emp->sizeHIGH << 8) + emp->sizeLO;
				/* A value of zero means 64MB (in 1K units) */
				if (nK == 0)
					nK = 0x10000;

#ifdef MEMDEBUG
sprintf(memdbgbuf, "eisa:%uK@%uM", nK, base / _1M);
memdebug();
#endif
				/*
				 * Unfortunately, we can't completely trust
				 * memory reported here, since many ECUs allow
				 * users to manually specify incorrect memory
				 * sizes. For memory that could have been
				 * reported by extmem_sz, assume extmem_sz
				 * knows better. To get this completely right,
				 * though, we'd need to do a full sort of the
				 * EISA memory segments, so we make a further
				 * assumption: that the BIOS will not split
				 * contiguous memory into multiple segments
				 * except when forced by the 64MB size
				 * limitation, or that if it does, it will
				 * report them in ascending address order.
				 * This lets us handle each segment
				 * independently, and only special-case a
				 * segment which overlaps or abuts extmem_sz.
				 */
				end = base + nK * 1024 - 1;
				if (base <= badext && badext < extmem_limit) {
					if (end >= badext)
						badext = end + 1;
					continue;
				}

				if (end > MINMEM) {
					any_above_min = 1;
					if (badext > extmem_sz + _1M) {
						maddmem(_1M, badext - _1M,
							NULL);
					}
					badext = 0;
				}

				maddmem(base, nK * 1024, NULL);

			} while (((emp++)->memflgs & EISA_MORE_LIST) &&
				 emp < &eisa->memlist[9]);
		}
	}

	return any_above_min;
}
