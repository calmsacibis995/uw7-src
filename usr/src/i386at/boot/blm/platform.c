#ident	"@(#)stand:i386at/boot/blm/platform.c	1.9"
#ident  "$Header$"

/*
 * Miscellaneous platform-specific support.
 */

#include <boot.h>
#include <bioscall.h>

STATIC ulong_t _get_time(void);
STATIC void last_chance(void);
STATIC void raw_boot(block_device_t *devp);
STATIC int _ischar(void);
STATIC int _getchar(void);

STATIC struct L_platform _L_platform = {
	_get_time, last_chance, raw_boot,
	NULL	/* buses */
};
STATIC struct L_kbd _L_kbd = {
	_ischar, _getchar
};

STATIC char bus_str[32];

extern void bioscall_start(void);
extern void memsizer_start(void);
extern void video_start(void);

STATIC void timer_start(void);
STATIC void bus_detect(void);
STATIC void a20(void);
STATIC int empty8042(void);
STATIC void gethdparms(void);

STATIC ulong_t ticks;	/* clock ticks since boot */

ulong_t * const Int1C = (ulong_t *)(4 * 0x1C);
STATIC ulong_t Orig1C;

extern char RMint1C[], int1C_end[];
extern char *_RMcode;
extern char _protcall1C[];

STATIC void (*OrigAbort)(void);
STATIC void _abort(void);

void
_start(void)
{
	char pstart[10];

	L_platform = &_L_platform;
	L_kbd = &_L_kbd;

	bioscall_start();
	memsizer_start();

	bus_detect();
	a20();

	video_start();

	timer_start();

	gethdparms();

	/*
	 * Allocate one page of "low" memory for the kernel to use as a
	 * real-address-mode trampoline. The kernel will locate it by
	 * the value of the boot parameter, "PSTART".
	 */
	sprintf(pstart, "%u", (ulong_t)malloc3(4096, 4096, boot_use));
	param_set("PSTART", pstart, PARAM_RDONLY|PARAM_HIDDEN);
}

STATIC void
timer_start(void)
{
	ulong_t patchloc;

	/* Install timer tick handler, after copying to real-mode memory */
	Orig1C = *Int1C;
	*Int1C = (ulong_t)_RMcode + 0x100 - (int1C_end - RMint1C);
	memcpy((void *)*Int1C, RMint1C, int1C_end - RMint1C);

	/* Patch call to RMprotcall in copy of tick handler */
	patchloc = *Int1C + (ulong_t)(_protcall1C - RMint1C) + 1;
	*(ulong_t *)patchloc = (ulong_t)_RMprotcall - (patchloc + 4);

	/* Intercept abort function, to restore original timer handler */
	OrigAbort = p1->p1_abortf;
	p1->p1_abortf = _abort;
}

/*
 * Asynchronous timer tick. If we're waiting for block I/O when the timer
 * fires, check for (time-related) events.
 */
void
_timer(void)
{
	if (biowait) {
		++ticks;
		check_event();
	}
}

/*
 * Get current time in centiseconds.
 */
STATIC ulong_t
_get_time(void)
{
	struct biosregs regs;
	static ulong_t prevtick, day;

	if (!biowait) {
		regs.intnum = 0x1A;	/* Time BIOS functions */
		regs.ax = 0x0000;		/* get system time */
		(void)bioscall(&regs);

		ticks = ((ulong_t)regs.cx << 16) + regs.dx;
		if (ticks < prevtick)
			day += 24*60*60*100;	/* midnight wraparound */
		prevtick = ticks;
	}

	return day + (ticks * 1000) / 182;	/* 18.2 ticks per second */
}

STATIC void
_abort(void)
{
	/* De-install timer tick handler */
	*Int1C = Orig1C;

	(*OrigAbort)();
	/* NOTREACHED */
}

STATIC int
_ischar(void)
{
	struct biosregs regs;

	regs.intnum = 0x16;	/* Keyboard BIOS functions */

	regs.ax = 0x0100;		/* check for keystroke */
	return (bioscall(&regs) & EFL_Z) ? 0 : 1;
}

STATIC int
_getchar(void)
{
	struct biosregs regs;

	regs.intnum = 0x16;	/* Keyboard BIOS functions */

	regs.ax = 0x0000;		/* get keystroke */
	(void)bioscall(&regs);

	return (regs.ax & 0xFF);
}

/*
 * Last chance to do platform-specific stuff before we jump to the kernel.
 * No block I/O will be performed after this point.
 */
STATIC void
last_chance(void)
{
	struct biosregs regs;

	/*
	 * We may have booted from an emulated floppy on a CD-ROM.
	 * Turn off any such emulation now.
	 */
	regs.intnum = 0x13;	/* Disk Functions */
	regs.ax = 0x4B00;		/* terminate disk emulation */
	regs.dx = 0x7f;				/* terminate all disks */
	regs.si = (unsigned short)biosbuffer();	/* scratch buffer */
	(void)bioscall(&regs);
}

#define EISA_SIGADDR	0xFFFD9
#define PCI_SIG		0x20494350
#define MCAISA_IDPORT   0x7f
#define   MCAISA_IDMASK	0xf0
#define   MCAISA_ID1	0xa0
#define   MCAISA_ID2	0x90

STATIC void
bus_detect(void)
{
	struct biosregs regs;
	unsigned short pciver;	/* PCI bus version number, BCD, major:minor */
	unsigned char pnpver;	/* PnP bus version number, BCD, major:minor */
	unsigned char x, *sigp;
	char *p;
	int i;

	/* Get pointer to configuration table */
	regs.intnum = 0x15;	/* System BIOS functions */
	regs.ax = 0xC000;		/* get configuration */
	(void)bioscall(&regs);
	bios_confp = (unsigned char *)((regs.di << 4) + regs.bx);
		/* really returned in es:bx, but bioscall puts es into di */

	bustypes = 0;

	/* Check for PCI bus */
	regs.intnum = 0x1A;
	regs.ax = 0xB101;		/* PCI installation check */
	if ((bioscall32(&regs) & EFL_C) == 0 && (regs.ax & 0xFF00) == 0 &&
	    regs.edx == PCI_SIG) {
		bustypes |= BUS_PCI;
		pciver = regs.bx;
	}

	if (bios_confp[CONF_FEATURE1] & CONF_MCA) {
		bustypes |= BUS_MCA;
		/*
		 * See the comment in [at386/mip/mc386.c].
		 * The MCAISA_ID checks may be redundant with CONF_DUAL.
		 */
		x = (inb(MCAISA_IDPORT) & MCAISA_IDMASK);
		if (x == MCAISA_ID1 || x == MCAISA_ID2 ||
		    (bios_confp[CONF_FEATURE1] & CONF_DUAL))
			bustypes |= BUS_ISA;
	}
	if (memcmp((char *)EISA_SIGADDR, "EISA", 4) == 0)
		bustypes |= BUS_EISA;

	/*
	 * Scan for PnP BIOS signature.
	 */
	for (sigp = (unsigned char *)0xF0000;
	      sigp < (unsigned char *)0x100000; sigp += 16) {
		if (memcmp(sigp, "$PnP", 4) == 0) {
			/* verify checksum */
			for (i = x = 0; i < sigp[5]; i++)
				x += sigp[i];
			if (x != 0)
				continue;
			/* OK, we've got a PnP BIOS */
			bustypes |= BUS_PNP;
			pnpver = sigp[4];
			break;
		}
	}

	/*
	 * Unless we found just an MCA bus, assume there's ISA capability.
	 * XXX - How do we detect PCI systems w/o an ISA compatibility bus?
	 */
	if (bustypes != BUS_MCA)
		bustypes |= BUS_ISA;

	/*
	 * Generate user-readable bus types string.
	 */
	p = L_platform->buses = bus_str;
	if (bustypes & BUS_PCI) {
		sprintf(p, "PCI%B.%B", pciver >> 8, pciver & 0xFF);
		p += strlen(p);
	}
	if (bustypes & BUS_MCA)
		stradd("MCA", bus_str, ',', &p);
	if (bustypes & BUS_EISA)
		stradd("EISA", bus_str, ',', &p);
	if (bustypes & BUS_ISA)
		stradd("ISA", bus_str, ',', &p);
	if (bustypes & BUS_PNP) {
		ASSERT(p != bus_str);
		sprintf(p, ",PnP%B.%B", pnpver >> 4, pnpver & 0x0F);
		p += strlen(p);
	}
	*p = '\0';

	/* NOTE: Old bootstrap has special cases for Olivetti and Corollary */
}

#define MCA_SYSPORT	0x92
#define   MCASYS_A20	0x02

/*
 * Keyboard controller I/O port addresses.
 */
#define KB_ICMD		0x64	/* input buffer command W/O */
#define KB_STAT		0x64	/* keyboard controller status R/O */
#define KB_IDAT		0x60	/* input buffer data W/O */
#define KB_ODAT		0x60	/* output buffer data R/O */

/*
 * Keyboard controller commands and flags.
 */
#define KB_INBF		0x02	/* input buffer full flag */
#define KB_OUTBF	0x01	/* output buffer full flag */
#define KB_WOP		0xD1	/* write output port command */
#define KB_A20ENABLE	0xDF	/* enable A20 gate command */

/*
 * Enable address line A20, which is off by default for 8086 compatibility.
 */
STATIC void
a20(void)
{
	if (bustypes & BUS_MCA /* || weirdIBM */) {
		/* Do it the MCA way, using the system control port. */
		unsigned char x;

		x = inb(MCA_SYSPORT);
		x |= MCASYS_A20;
		outb(MCA_SYSPORT, x);
	} else {
		/* Do it the PC-AT way, using the keyboard controller. */

		/* Ensure 8042 input buffer is empty */
		if (empty8042())
			return;		/* failure */

		outb(KB_ICMD, KB_WOP);	/* 8042 command to write output port */

		/* Wait for 8042 to accept command */
		if (empty8042())
			return;		/* failure */

		outb(KB_IDAT, KB_A20ENABLE);	/* address line 20 gate on */

		/* Wait for 8042 to accept command */
		if (empty8042())
			return;		/* failure */

		outb(KB_ICMD, 0xFF);	/* null cmd to ensure it's all done */

		/* Wait for 8042 to accept final command */
		(void)empty8042();
	}
}

/*
 * Wait for the keyboard input buffer to be empty, and flush anything
 * that comes in from the output buffer. Returns non-zero on error.
 */
STATIC int
empty8042(void)
{
	int i, stat;

	for (i = 0; i < 10000; i++) {
		if ((stat = inb(KB_STAT)) & KB_OUTBF) {
			(void)inb(KB_ODAT);
			continue;
		}
		if ((stat & KB_INBF) == 0)
			return 0;
	}
	return 1;	/* timed out */
}


#define BOOTSECSZ 512
#define MBOOT	0x600
#define PARTTBL 0x7BE
#define PT_SIZE	0x10

STATIC void
raw_boot(block_device_t *devp)
{
	extern char rawboot[], rawboot_end[];
	static char *bootsec;
	struct biosregs regs;
	ulong_t orig_base;
	int status;

	if (bootsec == NULL)
		bootsec = malloc(BOOTSECSZ);

	/*
	 * Read the fdisk table from the masterboot sector into memory at its
	 * standard location, so the loaded bootstrap can find its fdisk entry.
	 * This needs to be an absolute read.
	 */
	orig_base = b_driver->d_base;
	b_driver->d_base = 0;
	status = bread((void *)MBOOT, 0, BOOTSECSZ);
	b_driver->d_base = orig_base;

	if (status == -1) {
		printf("Error loading fdisk table\n");
		return;
	}

	/* Load first sector from partition. */
	if (bread(bootsec, 0, BOOTSECSZ) == -1) {
		printf("Error loading boot sector\n");
		return;
	}

	/* Check for valid signature, if appropriate. */
	if (!(b_driver->d_flags & D_REMOVABLE) &&
	    *(unsigned short *)&bootsec[BOOTSECSZ - 2] != 0xAA55U) {
		printf("Missing operating system\n");
		return;
	}

	/* De-install timer tick handler */
	*Int1C = Orig1C;

	/*
	 * Copy the boot sector somewhere that rawboot can find it (must be
	 * a real-mode addressable location). Use the "BIOS buffer". Can't
	 * copy it to the real location yet, since the code to do bioscall32()
	 * lives there.
	 */
	regs.ebx = (ulong_t)biosbuffer(); /* rawboot expects address in %ebx */
	memcpy((void *)regs.ebx, bootsec, BOOTSECSZ);

	/*
	 * Set up the remaining regs for rawboot, to be passed on to the
	 * bootstrap.
	 */
	regs.esi = PARTTBL + rawpart * PT_SIZE;
	regs.dx = devp->unit + 0x80;

	/*
	 * For the rest, we need some helper code to run in real-address mode.
	 * Execute it with rmcall().
	 * This never returns, since it jumps to the new bootstrap.
	 */
	(void)rmcall(&regs, rawboot, rawboot_end - rawboot);
	/* NOTREACHED */
}

STATIC void
gethdparms(void)
{
	static char hdparm_name[] = "HDPARMx";
	char val[80];
	struct biosregs regs;
	int unit, ncyls, nheads, nsects;
	unsigned efl;

	/*
	 * Fetch drive parameters using BIOS calls, for HD_PARMS.
	 */
	for (unit = 0; unit <= 1; unit++) {
		regs.intnum = 0x13;
		regs.ax = 0x0800;
		regs.dx = unit + 0x80;
	
		efl = bioscall(&regs);

		/* If BIOS call failed or drive # too large, leave all zeros */
		if ((efl & EFL_C) || (regs.ax & 0xFF00) != 0 ||
		    unit >= (regs.dx & 0xFF))
			continue;

		ncyls = (regs.cx >> 8) + ((regs.cx & 0xC0) << 2) + 1;
		nheads = (regs.dx >> 8) + 1;
		nsects = (regs.cx & 0x3F);

		/*
		 * Note: original version (in old bootstrap) had the side
		 * effect of allowing the INT 41/46 Disk Parameter Table
		 * to override the number of cylinders reported by the BIOS
		 * call. Is this behavior important? Was it even correct?
		 * As best as I can tell, it resulted in use of one extra
		 * cylinder that the BIOS had reserved for a landing zone.
		 *
		 * Original version also checked a complex and improbable
		 * set of conditions and if all were met, then ncyls
		 * would be set to zero (i.e. no drive). Is this needed?
		 */

		hdparm_name[6] = unit + '0';
		sprintf(val, "%u,%u,%u", ncyls, nheads, nsects);
		param_set(hdparm_name, val, PARAM_GEN);
	}
}
