#ifndef _IO_TARGET_VTOC_VTOCDEF_H
#define _IO_TARGET_VTOC_VTOCDEF_H

#ident	"@(#)kern-pdi:io/layer/vtoc/vtocdef.h	1.3.4.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#include <io/vtoc.h>

/*
 *  NOTE: the next 2 numbers must follow the relationship
 *
 *  log2(VTOC_MAX_DISKS) + log2(VTOC_MAX_MINOR) <= 18
 *
 *  Also, be careful.  The value of VTOC_FIRST_SPEC_PART
 *  is hard-coded in boot_mkdev.c and mkdev.d/disk1
 */
#define VTOC_MAX_DISKS	1024
#define VTOC_MAX_MINOR	256

/*
 * Reserve the minors 184-188 for the special partitions used to access the
 * FDISK partitions and the whole disk at once.  VTOC_NUM_SPEC_PART needs to be
 * increased for future support of any other kind of special partitions (e.g.
 * extended DOS logical partitions)
 */
#define	VTOC_FIRST_SPEC_PART	(V_NUMSLICES)
#define	VTOC_NUM_SPEC_PART	(FD_NUMPART + 1)

/*
 * Reserve the minors 189-204 for the clone channels of each VTOC instance.
 * These channels ALWAYS refer to a slice on the BOOT disk as defined by
 * the BOOTSTAMP boot parameter.
 */
#define	VTOC_FIRST_CLONE_CHANNEL	(VTOC_FIRST_SPEC_PART + VTOC_NUM_SPEC_PART)
#define	VTOC_NUM_CLONE_CHANNEL	(V_NUMPAR)

#define VTOC_LAST_USED_MINOR	(VTOC_FIRST_CLONE_CHANNEL + VTOC_NUM_CLONE_CHANNEL - 1)

#define VTOC_SIZE	((VE_MAXSECTS+1)*VTOC_SMALLEST_SECTOR)

typedef struct vtoc_data {
	/* Structure related stuff */
	int vtoc_state;	
	lock_t *vtoc_lock;
	pl_t	vtoc_opri;
	sv_t *vtoc_sv;

	/* Fdisk related stuff */
	int vtoc_active;
	int vtoc_fdiskFlag[VTOC_NUM_SPEC_PART];
	struct xpartition vtoc_fdiskTab[VTOC_NUM_SPEC_PART];
	uchar_t vtoc_heads;	/* Cylinder size as reported by the fdisk */
	uchar_t vtoc_sectors;	/* Sector size as reported by the fdisk */
	uchar_t vtoc_dptype;	/* Disk type */

	/* Underlying disk parameters */
	daddr_t	vtoc_unixst;
	ulong_t vtoc_blksz;	/* Size of block */
	daddr_t vtoc_nblk;	/* Number of blocks */
	bcb_t	*vtoc_bcbp; /* breakup control block */

	/* Vtoc related stuff */
	int vtoc_sanity;
	int vtoc_version;
	int vtoc_nslices;
	int *vtoc_vtocFlag;
	struct partition *vtoc_part;

	/* Layer related stuff */
	uint_t vtoc_offset;
	sdi_device_t *vtoc_export;
	sdi_device_t vtoc_device;
} vtoc_t;

#define VTOC_INVALID_FDISK	-1

/*
 * Possible values of vtoc_state.
 */
#define VTOC_SINGLETHREAD	1

/*
 * Locking macros
 */
#define VTOC_LOCK(vtoc) (vtoc->vtoc_opri = LOCK(vtoc->vtoc_lock, pldisk))
#define VTOC_UNLOCK(vtoc) UNLOCK(vtoc->vtoc_lock, vtoc->vtoc_opri)

extern struct dev_cfg VTOC_dev_cfg[];
extern int VTOC_dev_cfg_size;

#if defined(__cplusplus)
        }
#endif

#endif /* _IO_TARGET_VTOC_VTOCDEF_H */
