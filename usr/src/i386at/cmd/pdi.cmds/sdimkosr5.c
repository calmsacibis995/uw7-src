#ident	"@(#)pdi.cmds:sdimkosr5.c	1.1"

#include <sys/vtocos5.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/fdisk.h>
#include <sys/vtoc.h>
#include <sys/alttbl.h>
#include <sys/altsctr.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <pfmt.h>
#include <string.h>
#include <locale.h>

#define BLKSZ	512
#define OSR5_FIRST_SLICE	1
#define OSR5_LAST_SLICE		7

extern int errno;

/* Global variables */
static struct partable divvytab;	/* Divvy table */
static struct vtoc_ioctl vtoc;		/* Vtoc table */
static uchar_t *buffer;		/* Scratch buffer to do I/O. */
static struct SCSIbadtab badtrkTab;	/* OSR5 Badtrak table */
static int devfd;					/* file descriptor to disk */
static char *devname;				/* name of the disk */
static int unix_base;				/* where does Unix start */
static struct disk_parms dp;		/* disk parameters/geometry */

/* Forward declarations */
static int getParam (int argc, char *argv[]);
static struct alts_ent *getBadtrk(int , struct SCSIbadtab *);
static int getDivvy(int, struct partable *, uchar_t *);
static struct alts_ent *getBadtrk(int , struct SCSIbadtab *);
static void divToVtoc(struct partable *, struct vtoc_ioctl *, int, int, int);
static void prtDivvy(struct partable *);
static int getUnixPartition(int , uchar_t *, int *, int *, int *);

main(int argc, char *argv[])
{
	struct alts_ent *alttab;
	int sectsz, hdsz;

	/* Deal with calling parameters. */
	if (getParam(argc, argv)) {
		/* We must exit. All messages produced by the called funtion. */
		exit(1);
	}

	/* Find out where does the Unix paritiont start */
	if (!getUnixPartition(devfd, buffer, &unix_base, &sectsz, &hdsz)) {
		pfmt(stderr, MM_ERROR, ":1: Invalid Unix partition on %s\n", devname);
	}

	/* Get the divvy */
	if (getDivvy(devfd, &divvytab, buffer)) {
		pfmt(stderr, MM_ERROR, ":2: Invalid OSR5 divvy table on %s\n", devname);
		exit(1);
	}

	/* Translate divvy to vtoc.*/
	divToVtoc(&divvytab, &vtoc, unix_base, sectsz, hdsz);

	/* Write the pdinfo + vtoc on disk */
	writeVtoc(devfd, &vtoc);

	/* Clean up */
	free(alttab);
	free(buffer);

	return 0;
}

/*
 * static int
 * getParam (int argc, char *argv[])
 *
 * Get parameters and other global information and resources for the program.
 */
static int
getParam (int argc, char *argv[])
{
	extern int optind;
	char label[64];

	setlocale(LC_ALL,"");
	setcat("sdimkosr5");
	sprintf(label,"UX:%s",argv[0]);
	setlabel(label);

	/* Open the disk */
	if ((devfd = open(argv[optind], O_RDWR)) == -1) {
		pfmt(stderr, MM_ERROR, ":3: Cannot open file: %s\n", strerror(errno));
		return -1;
	}

	/* Get the disk parameters/geometry */
	if (ioctl(devfd, V_GETPARMS, &dp) == -1) {
		pfmt(stderr, MM_ERROR, ":4: GETPARMS on %s failed\n", devname);
		return -1;
	}

	/* Allocate a buffer that matches the underlying block size */
	if (!(buffer = (uchar_t *) malloc(dp.dp_secsiz))) {
		pfmt(stderr, MM_ERROR, ":5: Out of memory\n", devname);
		return -1;
	}

	devname = argv[optind];

	return 0;
}


/*
 * static int
 * getDivvy(int devfd, struct partable *divvytab, uchar_t *buff)
 *
 * Read the divvy into struct partable.
 */
static int
getDivvy(int devfd, struct partable *divvytab, uchar_t *buff)
{
	ushort_t *p = (ushort_t *) buff;
	struct absio absio;

	absio.abs_sec = TWO2PARLOC + unix_base;
	absio.abs_buf = (char *) buff;
	if (ioctl(devfd, V_RDABS, &absio) != 0) {
		return -1;
	}

	if (((struct partable *) p)->p_magic != PAMAGIC) {
		return -1;
	}

	divvytab->p_magic = *p++;
	bcopy((uchar_t *)p, (uchar_t *) divvytab->p,
	      sizeof(struct parts) * MAXPARTS);

#ifdef DEBUG
	prtDivvy(divvytab);
#endif

	return 0;
}


/*
 * static void
 * divToVtoc(struct partable *divvytab, struct vtoc_ioctl *vtoc)
 *
 * Convert a divvy into a vtoc.
 */
static void
divToVtoc(struct partable *divvytab, struct vtoc_ioctl *vtoc, int unix_base, int sectsz, int hdsz)
{
	struct partition *slice = vtoc->v_slices;
	struct parts *divpart = divvytab->p;
	int cylsz = sectsz * hdsz;
	int i;

	/* We will always make 10 slices, that is 0 through 9 */
	vtoc->v_nslices = 10;

	/*
	 * Set-up partition 0.
	 */
	slice->p_tag = V_BACKUP;
	slice->p_flag = V_VALID | V_UNMNT;
	slice->p_start = unix_base;
	slice->p_size = divpart[7].p_siz * 2;
	slice++;

	/*
	 * Set-up the data slices up between 1 and 7.
	 */
	for (i = OSR5_FIRST_SLICE; i <= OSR5_LAST_SLICE; i++, slice++, divpart++) {
		if (divpart->p_siz > 0) {
			slice->p_tag = V_USR;
			slice->p_flag = V_VALID;
			slice->p_start = divpart->p_off * 2 + cylsz + unix_base/cylsz*cylsz;
			slice->p_size = divpart->p_siz * 2;
		} else {
			slice->p_tag = V_UNUSED;
			slice->p_flag = slice->p_start = slice->p_size = 0;
		}
	}

	/*
	 * Set-up alternate sector slice on slice 8
	 */
	slice->p_tag = V_ALTSOSR5;
	slice->p_flag = V_VALID | V_UNMNT;
	slice->p_start = TWO2BADLOC + unix_base;
	slice->p_size = sectsz * hdsz - TWO2BADLOC + 1;
	slice++;

	/*
	 * Set-up boot slice on slice 9
	 */
	slice->p_tag = V_BOOT;
	slice->p_flag = V_VALID | V_UNMNT;
	slice->p_start = unix_base;
	slice->p_size = TWO2BADLOC;
}

/*
 * static int
 * writeVtoc(int devfd, struct vtoc_ioctl *vtoc)
 *
 * Put a pdinfo and vtoc on the disk. This will overwrite whatever is on
 * sector 29!
 */
static int
writeVtoc(int devfd, struct vtoc_ioctl *vtoc)
{
	struct pdinfo pdinfo;

	/* update pdinfo struct to match GETPARMS info */
	pdinfo.sanity = VALID_PD;
	pdinfo.cyls = dp.dp_cyls;
	pdinfo.tracks = dp.dp_heads;
	pdinfo.sectors = dp.dp_sectors;
	pdinfo.bytes = dp.dp_secsiz;
	pdinfo.logicalst = dp.dp_pstartsec;
	pdinfo.vtoc_ptr = dp.dp_secsiz * VTOC_SEC + sizeof(pdinfo);
	pdinfo.vtoc_len =  sizeof(struct vtoc);
	pdinfo.alt_ptr = TWO2BADLOC;	/* Make it point to alternate slice */
	pdinfo.alt_len = sizeof(struct alt_info);

	if (ioctl(devfd, V_WRITE_PDINFO, &pdinfo) == -1) {
		(void)pfmt(stderr, MM_ERROR, ":6:Writing pdinfo failed: %s\n", strerror(errno));
	}

	if (ioctl(devfd, V_WRITE_VTOC, vtoc) == -1) {
		(void)pfmt(stderr, MM_ERROR, ":7:Writing vtoc failed: %s\n", strerror(errno));
	}
}

/*
 * static int
 * getUnixPartition(int devfd, char *buff, int *start, int *sectsz, int *hdsz)
 *
 * Read fdisk and find the ACTIVE UNIX partition. Return the block where this
 * starts.
 */
static int
getUnixPartition(int devfd, uchar_t *buff, int *start, int *sectsz, int *hdsz)
{
	int i;
	struct mboot *mboot;
	int ret_val = 0;
	struct absio absio;
	struct ipart *unix_part, *fdp;

	/* Read fdisk table */
	absio.abs_sec = 0;
	absio.abs_buf = (char *) buff;
	if (ioctl(devfd, V_RDABS, &absio) < 0) {
		(void) pfmt(stderr, MM_ERROR,
		  ":8:Disksetup unable to read partition table from %s: %s\n",
			devname, strerror(errno));
		return 0;
	}
	mboot = (struct mboot *) buff;

	/* find an active UNIX System partition */
	unix_part = NULL;
	fdp = (struct ipart *) mboot->parts;
	for (i = FD_NUMPART; i-- > 0; ++fdp) {
		if ((fdp->systid == UNIXOS) && (fdp->bootid == ACTIVE))
				unix_part = fdp;
	}

	if (unix_part == NULL) {
		(void) pfmt(stderr, MM_ERROR,
		  ":9:No active UNIX System partition in partition table!\n");
		return 0;
	}

	*start = unix_part->relsect;
	*sectsz = unix_part->endsect & 0x3F ;
	*hdsz = unix_part->endhead + 1;

#ifdef DEBUG
	printf("Unix base is %d (0x%x)\n", *start, *start);
	printf("\tsectorsz %d (0x%x) headsz %d (0x%x)\n", *sectsz, *sectsz, *hdsz, *hdsz);
#endif

	return 1;
}

#ifdef DEBUG
static void
prtDivvy(struct partable *divvytab)
{
	int i;
	struct parts *divpart = divvytab->p;

	printf("Sanity=0x%x\n", divvytab->p_magic);
	for (i = 0; i < MAXPARTS; i++, divpart++) {
		printf("off=%d (0x%x) sz=%d (0x%x)\n",
		divpart->p_off, divpart->p_off, divpart->p_siz, divpart->p_siz);
	}
}
#endif

