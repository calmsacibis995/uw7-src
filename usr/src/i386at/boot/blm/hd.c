#ident	"@(#)stand:i386at/boot/blm/hd.c	1.2.1.2"
#ident  "$Header$"

/*
 * Platform-specific hard disk driver.
 */

#include <boot.h>
#include <bioscall.h>
#include <sys/types.h>
#include <sys/fdisk.h>

#define BLKSZ 512
#define ECC_CORRECTED 0x11

#define OSTYP_UNUSED	0
#define OSTYP_UNIX	99
#define OSTYP_XENIX	2
#define OSTYP_DOS12	1
#define OSTYP_DOS16	4
#define OSTYP_DOS32	6
#define OSTYP_DOSEXT	5
#define OSTYP_ANYDOS	999
#define OSTYP_NT	7
#define OSTYP_OS2	7

struct ostyp {
	const char *ostyp_name;
	int ostyp_ind;
};
STATIC const struct ostyp ostypes[] = {
	{ "UNIX", OSTYP_UNIX },
	{ "XENIX", OSTYP_XENIX },
	{ "DOS_12", OSTYP_DOS12 },
	{ "DOS_16", OSTYP_DOS16 },
	{ "DOS_32", OSTYP_DOS32 },
	{ "DOS_EXT", OSTYP_DOSEXT },
	{ "DOS", OSTYP_ANYDOS },
	{ "NT", OSTYP_NT },
	{ "OS2", OSTYP_OS2 },
	{ "", OSTYP_UNIX }	/* default */
};
#define NTYPES (sizeof ostypes / sizeof ostypes[0])

STATIC void hd_read(void);
STATIC void hd_newdev(void);
STATIC void hd_flush(void);

STATIC block_driver_t hd_drv = {
	"hd", "", -1, D_VTOC, BLKSZ, 0, 0, NULL, hd_read, hd_newdev, hd_flush
};

STATIC char *hd_buffer;		/* I/O buffer, stolen from current driver */
STATIC ulong_t hd_bufsz;	/* Real buffer size, in hd blocks */
STATIC ulong_t hd_bufblk;	/* First block in buffer */
STATIC ulong_t hd_blkcnt;	/* Current number of blocks in buffer */
STATIC ulong_t spt, spc;	/* Disk geometry */
STATIC char curdrv = -1;	/* Current drive unit number */
const drive_base = 0x80;	/* To get BIOS drive code, add this to unit */

void
_start(void)
{
	/*
	 * Steal the current driver's buffer, since we know it's in real-address
	 * mode addressable memory. The caller must be sure to flush the
	 * buffer (set d_blkno to NOBLK) whenever switching between drivers.
	 */
	hd_buffer = b_driver->d_buffer;
	if (strcmp(b_driver->d_name, "hd") == 0)
		hd_bufsz = 32;	/* We "know" how big the hdboot buffer is. */
	else
		hd_bufsz = b_driver->d_blksz / BLKSZ;

	battach(&hd_drv);
}

STATIC void
hd_newdev(void)
{
	struct biosregs regs;
	struct ipart *parts, *partp[FD_NUMPART];
	int ostyp;
	int i, j, k;
	int partlen;
	char *partname, *new_params;

	ASSERT(hd_drv.d_params != NULL);

	if (hd_drv.d_unit != curdrv) {
		/*
		 * Determine disk geometry.
		 */
		regs.intnum = 0x13;		/* Disk I/O BIOS functions */
		regs.ax = 0x800;		/* Get disk parameters */
		regs.dx = hd_drv.d_unit + drive_base;

		if ((bioscall(&regs) & EFL_C) != 0) {
			printf("Can't get disk geometry\n");
			hd_drv.d_unit = -1;
			return;
		}

		spt = regs.cx & 0x3F;
		spc = ((regs.dx >> 8) + 1) * spt;
		curdrv = hd_drv.d_unit;
#ifdef DEBUG
printf("hd(%d): spt %u spc %u\n", curdrv, spt, spc);
#endif
	}

	/*
	 * Read partition table, so we can determine starting sector.
	 */
	hd_drv.d_blkno = hd_blkcnt = 0;
	hd_read();		/* read fdisk partition table (sector 0) */
	parts = (struct ipart *)&hd_drv.d_buffer[BOOTSZ];

	/*
	 * Sort partition table by starting sector. This causes partition
	 * numbers to be in disk order rather than order within the table.
	 */
	for (i = 0; i < FD_NUMPART; i++) {
		for (j = 0; j < i; j++) {
			if (parts[i].systid != OSTYP_UNUSED &&
			    (parts[i].relsect < partp[j]->relsect ||
			     partp[j]->systid == OSTYP_UNUSED)) {
				for (k = i; k-- > j;)
					partp[k + 1] = partp[k];
				break;
			}
		}
		partp[j] = &parts[i];
	}

	/*
	 * Now let's start parsing the d_params
	 */
	partlen = strcspn(hd_drv.d_params, ",");
	partname = strndup(hd_drv.d_params, partlen);

	/*
	 * If partition is specified as an OS type name, convert it.
	 */
	for (ostyp = 0, i = 0; i < NTYPES; i++) {
		if (strcmp(partname,  ostypes[i].ostyp_name) == 0) {
			ostyp = ostypes[i].ostyp_ind;
			break;
		}
	}

	if (ostyp) {
		/*
		 * Locate partition by OS type. Considering only partitions
		 * of the desired OS type, pick the one that is marked active
		 * or else the first one.
		 */
		for (i = -1, j = FD_NUMPART; j-- != 0;) {
			if (ostyp == OSTYP_ANYDOS) {
				if (partp[j]->systid != OSTYP_DOS12 &&
				    partp[j]->systid != OSTYP_DOS16 &&
				    partp[j]->systid != OSTYP_DOS32 &&
				    partp[j]->systid != OSTYP_DOSEXT)
					continue;
			} else if (partp[j]->systid != ostyp)
				continue;
			if (partp[j]->bootid == ACTIVE) {
				i = j;
				break;
			}
			if (i == -1)
				i = j;
		}

		if (i != -1) {
			/*
			 * Save logical partition number in a place we can
			 * export it from.
			 */
			new_params = malloc(strlen(hd_drv.d_params) -
					     partlen + 2);
			new_params[0] = i + '1';

			/*
			 * Now copy the tail of the parameter string to the
			 * export string.
			 */
			strcpy(new_params + 1, hd_drv.d_params + partlen);

			/* Point to the export string. */
			hd_drv.d_params = new_params;
		}
	} else {
		/*
		 * Partition is specified as a numeric parameter.
		 */
		if (partname[1] != '\0' ||
		    (i = partname[0] - '1') < 0 ||
		    i >= FD_NUMPART ||
		    partp[i]->systid == OSTYP_UNUSED)
			i = -1;
	}

	if (i == -1) {
		printf("No such partition\n");
		hd_drv.d_unit = -1;
		return;
	}

	/* Save physical partition number for raw boot. */
	rawpart = partp[i] - parts;

	hd_drv.d_base = partp[i]->relsect * 512;

#ifdef DEBUG
printf("partition base: %u\n", hd_drv.d_base);
#endif
}

STATIC void
hd_read(void)
{
	struct biosregs regs;
	int cyl, head, sector;
	ulong_t blkno = hd_drv.d_blkno;
	unsigned char drive = curdrv + drive_base;
	int retry = 0;

	if (blkno >= hd_bufblk && blkno < hd_bufblk + hd_blkcnt) {
		hd_drv.d_buffer = hd_buffer + (blkno - hd_bufblk) * BLKSZ;
		return;
	}

	hd_drv.d_buffer = hd_buffer;
	hd_bufblk = blkno;
	hd_blkcnt = hd_bufsz;

	cyl = blkno / spc;
	head = (blkno % spc) / spt;
	sector = (blkno % spt) + 1;

	regs.intnum = 0x13;		/* Disk I/O BIOS functions */

	do {
		regs.ax = 0x200 | hd_blkcnt;		/* Read sectors */
		regs.bx = (unsigned short)hd_buffer;
		regs.cx = (cyl << 8) | ((cyl >> 2) & 0xC0) | sector;
		regs.dx = (head << 8) | drive;

		if ((bioscall(&regs) & EFL_C) == 0 ||
		    ((regs.ax >> 8) & 0xFF) == ECC_CORRECTED)
			return;

		/* if disk read error, reset and retry */
		regs.ax = 0;
		(void)bioscall(&regs);

	} while (++retry % 5 != 0 || --hd_blkcnt != 0);

	hd_drv.d_blkno = NOBLK;	/* indicate failure */
}

STATIC void
hd_flush(void)
{
	hd_blkcnt = 0;
}
