#ifndef _IO_TARGET_SDI_SDI_EDT_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_SDI_EDT_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sdi/sdi_edt.h	1.18.7.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/* 		SDI Equipped Device Table information		*/

#ifdef _KERNEL_HEADERS
#include	<util/types.h>		/* REQUIRED */
#include	<io/autoconf/resmgr/resmgr.h>		/* REQUIRED */
#else
#include	<sys/types.h>		/* REQUIRED */
#include	<sys/resmgr.h>		/* REQUIRED */
#endif	/* _KERNEL_HEADERS */

/* driver ioctl commands */

#define BIOC		('B'<<8)	/* For BUS ioctl() commands 	*/
#define	B_GETTYPE	(BIOC|0x1)	/* Get bus and driver name 	*/
#define	B_GETDEV	(BIOC|0x2)	/* Get device for pass through 	*/
#define B_REDT		(BIOC|0x4)	/* Read EDT	 		*/
#define B_HA_CNT	(BIOC|0x5)	/* get # of HA boards configured*/
#define B_OBSOLETE	(BIOC|0x6)	/* Get # of subdevices per LU (obsolete in gemini) */
#define B_RXEDT		(BIOC|0x7)	/* Read Extended EDT 		*/
#define B_EDT_CNT	(BIOC|0x8)	/* get # of EDT entries 	*/
#define B_GET_MAP	(BIOC|0x9)	/* Get sdi_gethbano mapping     */
#define B_MAP_CNT	(BIOC|0xa)	/* Get size of gethbano mapping */
#define B_ADD_DEV	(BIOC|0xb)      /* Hot add a device */
#define B_RM_DEV	(BIOC|0xc)      /* Hot remove a device */
#define B_PAUSE		(BIOC|0xd)      /* Pause the SCSI bus */
#define B_CONTINUE	(BIOC|0xe)      /* continue normal operation of SCSI bus */
#define B_RESERVED	(BIOC|0xf)	/* reserved for future expansion */
#define B_NEW_TIMEOUT_VALUES (BIOC|0x10) /* supply new timeout values */

#define B_CAPABILITIES	(BIOC|0x11)	/* ioctl for hba info */
#define B_TIMEOUT_SUPPORT (BIOC|0x12)	/* turn timeout/reset support on/off */
#define B_RXEDT_VOL (BIOC|0x13)	/* special REDT ioctl just for vxvm */

/*
#define	SDI_DEV(x)	((((x)->ha_slot) << 3) | ((x)->tc_id))
 *
 * NOTE: HAMINOR MACRO SHOULD NO LONGER BE USED.
 *	SDI_MINOR REPLACES IT, AND EXTENDS THE DEFINITION OF 
 *	THE CONTROLLER/TARGET/LUN.
 */

#define HAMINOR(c,t,l)	((c << 5) | (t << 2) | l)

/*
 * Minor numbers are now an 18-bit field. Since binary compatibility
 * must be maintained with the old style, the low order 8 bits remain
 * defined as above.  The remaining 10 bits extend the existing
 * controller, target, and lun fields, and add the bus number.
 * The pass-through minor number is now defined as:
 *               +------------------------+
 * minor number: |bbb lll tt cc ccc ttt ll|
 *               +------------------------+
 * 	bbb	bus number  		3 bits (15-17)
 * 	lll	extended lun		3 bits (12-14)
 * 	tt	extended target		2 bits (10-11)
 * 	cc	extended controller	2 bits (8-9)
 * 	ccc	controller number	3 bits (5-7)
 * 	ttt	target number		3 bits (2-4)
 * 	ll	lun         		2 bits (0-1)
 *
 * NOTE: for SDI_MINOR definition below...
 *       shifting of extended lun and target are properly
 *	 adjusted for upper part of value.  For example, extended
 *	 lun is in the 12th bit field, but the value being shifted
 *	 is already shifted 2 bits.  Therefore, shift value is 10.
 *	 Similarly, extended target is in 10th bit field, but the
 *	 value is already shifted 3 bits.  Therefore shift value is 7.
 */
#define SDI_MINOR(c,t,l,b) ((((b)&0x07) <<15) | \
			    (((l)&0x1C) <<10) | \
			    (((t)&0x18) <<07) | \
			    (((c)&0x1F) <<05) | \
			    (((t)&0x07) <<02) | \
			    ((l)&0x03))


#define SCSI_SUBDEVS	16	/* Number of subdevices per minor 	*/

#define	NAME_LEN	10

#ifndef VID_LEN
#define VID_LEN		8
#define PID_LEN		16
#define REV_LEN		4
#endif

#define INQ_LEN		VID_LEN+PID_LEN+1 /* inquiry data length including */
					  /* vendor id and product id */
#define INQ_EXLEN	INQ_LEN+REV_LEN	  /* INQ_LEN plus revision id */

/*
 * Stamp structure to map the serial field of pdinfo structure into
 * software simulated 64-bits quantity.
 */
struct pd_stamp {
	ulong_t ps_word0;		/* First word of the stamp */
	ulong_t ps_word1;		/* Second word of the stamp */
	ulong_t ps_word2;		/* Third word of the stamp */
};

#define PD_STAMP_DFLT	0x20202020	/* Default value for stamp component
					 * (integer value of four blank
					 * characters)
					 */
/* Macros for setting and matching stamps */
#define PD_SETSTAMP(S,W0,W1,W2)	{ \
					(S)->ps_word0 = (W0); \
					(S)->ps_word1 = (W1); \
					(S)->ps_word2 = (W2); \
				}
#define PD_STAMPMATCH(X,Y) \
	((X)->ps_word0 == (Y)->ps_word0 && \
	 (X)->ps_word1 == (Y)->ps_word1 && \
	 (X)->ps_word2 == (Y)->ps_word2)

/*
 * Previous system limits for number of hba's, targets, and luns.
 * Also, used as default maximum for targets and luns, unless HBA
 * specifies otherwise.
 */
#define MAX_HAS		8		/* The max HA's in a system.	*/
#define MAX_TCS		8
#define MAX_LUS		8

/*
 * Current system limits for number of hba's, targets, and luns.
 * HBA's can specify number of targets and luns supported in the idata
 * structure, up to these maximums.
 */
#define MAX_EXHAS	32
#define MAX_EXTCS	32
#define MAX_EXLUS	32
#define MAX_BUS		8

struct bus_type {
	char	bus_name[NAME_LEN];	/* Name of the driver's bus */
	char	drv_name[NAME_LEN];	/* Driver prefix */
};


/* This structure is stored in the space file by target	*/
/* drivers  on a per TC type which that driver supports.*/

struct tc_data
{
	uchar_t	tc_inquiry[INQ_LEN];	/* TC inquiry data	*/
	uchar_t	max_lus;	/* max LUs supported by TC	*/
};


/* This structure is used by target drivers to determine	*/
/* how many controllers are configured in the system		*/

struct tc_edt
{
	uchar_t	ha_slot;
	uchar_t	tc_id;
	uchar_t	n_lus;
	uchar_t	lu_id[MAX_LUS];
};


/* These defines are used to extract the HA occurence and the type (single ended
 * or differential) out of the "ha_slot" of the edt structure.
 */

/* SCSI Version Flags */
#define SINGLE_ENDED    1
#define DIFFERENTIAL    2

#define BUS_TYPE(x)	((((x)>>7) & 0x1) ? DIFFERENTIAL : SINGLE_ENDED)
#define BUS_OCCUR(x)	((x) & 0x7)

struct scsi_edt		/* SCSI bus equipped device table. One per HA */
{
	short	c_maj;		/* Target drv. character major number */
	short	b_maj;		/* Target drv. block major number     */
	uchar_t	pdtype;		/* Target controller SCSI device type */
	uchar_t	tc_equip;	/* one if TC is equipped	      */
	uchar_t	ha_slot;	/* Host Adaptor controller slot number*/
	uchar_t	n_lus;		/* number of equipped LUS on TC	      */
	uchar_t	lu_id[MAX_LUS];	/* one if LU is equipped      */
	char	drv_name[NAME_LEN];	/* target driver ASCII name   */
	uchar_t	tc_inquiry[INQ_LEN];	/* TC vendor and product name */
};

struct scsi_xedt       /* SCSI bus extended equipped device table. One per lun */
{
	major_t		xedt_cmaj;	/* Target drv. character major number */
	major_t		xedt_bmaj;	/* Target drv. block major number     */
	ulong_t		xedt_minors_per;/* number of minors for each unit     */
	uchar_t		xedt_pdtype;	/* Target controller SCSI device type */
	uchar_t		xedt_lun;	/* what lun is this device */
	ushort_t	xedt_ctl;	/* Host Adaptor controller number     */
	ushort_t	xedt_target;	/* target number		      */
	uchar_t		xedt_bus;	/* bus number			      */
	char		xedt_drvname[NAME_LEN];	/* target driver ASCII name   */
	uchar_t	xedt_tcinquiry[INQ_EXLEN];      /* TC vendor and product name */
	ulong_t		xedt_memaddr;	/* Configured memory (ROM BIOS) addr  */
					/*   for controller xedt_ctl.	      */
	uchar_t		xedt_ctlorder;	/* Controller order (if specified) by */
					/*   controller xedt_ctl.	      */
	ulong_t		xedt_ordinal;	/* ordinal numbering of all devices  */
	minor_t		xedt_first_minor; /* the first minor for this device */
	uchar_t		xedt_ha_id;		/* HBA target number */
	uchar_t		xedt_fill[3];	/* future expansion  */
	rm_key_t	xedt_rmkey;	/* resmgr DB key ( if any ) */
	struct pd_stamp	xedt_stamp;	/* Device stamp (if any) */
};

struct drv_majors {
	major_t b_maj;		/* Block major number		*/
	major_t c_maj;		/* Character major number	*/
	ulong_t minors_per;	/* number of minors per device instance	*/
	minor_t first_minor;	/* the first minor for this device */
};

struct scsi_adr {			/* SCSI Address 	        */
	int	        scsi_ctl;	/* Controller			*/
	int	        scsi_target;	/* Target			*/
	int	        scsi_lun;	/* Lun				*/
	int		scsi_bus;	/* Bus				*/
};

/*
 * HBA_map slot values
 */
#define SDI_UNUSED_MAP	-2
#define SDI_NEW_MAP	-1

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SDI_SDI_EDT_H */
