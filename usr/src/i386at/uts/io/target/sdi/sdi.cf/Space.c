#ident	"@(#)kern-i386at:io/target/sdi/sdi.cf/Space.c	1.9.4.1"
#ident	"$Header$"

/*
 * sdi/space.c
 */

#include <config.h>
#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi_defs.h>
#include <sys/sdi.h>
#include <sys/scsi.h>

/* XXX for now */
#define	SDI_RTABSZ	8

int sdi_major = SDI__CMAJOR_0;
major_t sdi_pass_thru_major = SDI__CMAJOR_1;

int	sdi_hbaswsz = SDI_MAX_HBAS;
int	sdi_rtabsz = SDI_RTABSZ;
/* Delay count for sdi_register */
/* This delay is the number of seconds to wait for devices becoming ready */
int	sdi_cklu_dnr_cnt = SDI_REGISTER_DELAY;

/*
 *  Note that the order of the layers is established not by the value
 *  of precedence ( e.g. SDI_STACK_GAMMA ) but by the order of the precedence
 *  values in this array.  Thus, below, BETA is between ALPHA and VTOC, even
 *  if the numerical value for BETA is greater than or less than ALPHA.
 */

sdi_layer_list_t sdi_layer_order[] = {
		{ "HBA/Target Driver Level" , "BASE " , SDI_STACK_BASE } ,
		{ "Deflection Driver Level" , "ALPHA" , SDI_STACK_ALPHA} ,
		{ "DARDAC Driver Level    " , "BETA " , SDI_STACK_BETA } ,
		{ "MPIO Driver Level      " , "GAMMA" , SDI_STACK_GAMMA} ,
		{ "Partition Driver Level " , "DELTA" , SDI_STACK_DELTA} ,
		{ "VTOC Driver Level      " , "VTOC " , SDI_STACK_VTOC } ,
	};
int sdi_layer_entries = sizeof(sdi_layer_order) / sizeof(sdi_layer_list_t);

/*
 * Some devices may:
 *	a- show  problems when LUNs > 0 are accessed.
 *	b- have non-continous LUNs.
 *	c- respond to multiple LUNs and the show problems after a certain one.
 *	   (generalized case of a).
 *
 * Below is the Inquiry Data and the last lun supported from devices that
 * are known to have problems.  If a device is found in the list below,
 * only the number of LUNs stated will be searched.
 *
 * To add a device add an entry of the form <{ char *, char *, char *, int}>,
 * where the fields represent Vendor, Product, Revision and number of contigous
 * supported LUNs, * respectively.
 *
 * All fields must be present.
 *
 * As for the <char *> fields, null is indicated by <"">. This matches all
 * strings. As for the <int> field, <-1> instructs the search to be performed
 * on all valid luns for the SCSI level supported (for SCSI-2, 8).
 *
 * The last set of three strings in the array must be {"", "", "", -1}
 */

struct sdi_blist sdi_limit_lun[] = {
	/* "Vendor", "Product", "Revision", LUNs */
	{"CHINON","CD-ROM CDS-431","H42", 1},	/* Locks on lun != 0*/
	{"CHINON","CD-ROM CDS-535","Q14", 1}, 	/* Locks on lun != 0*/
	{"COMPAQ","CD-ROM CDU561-31", "1.8i", 1},
	{"DENON","DRD-25X","V", 1},		/* Lock on lun != 0 */
	{"HITACHI","DK312C","CM81", 1},		/* Responds to all lun */
	{"HITACHI","DK314C","CR21", 1},		/* Responds to all lun */
	{"HP", "C1750A", "3226", 1},		/* scanjet iic */
	{"HP", "C1790A", "", 1},		/* scanjet iip */
	{"HP", "C2500A", "", 1},		/* scanjet iicx */
	{"IMS", "CDD521/10","2.06", 1},		/* Locks on lun != 0*/
	{"Logical", "Drive", "", 32},		/* 32 Logical Drives support */
	{"MAXTOR","XT-3280","PR02", 1},		/* Locks on lun != 0*/
	{"MAXTOR","XT-4380S","B3C", 1},		/* Locks on lun != 0*/
	{"MAXTOR","MXT-1240S","I1.2", 1},	/* Locks on lun != 0*/
	{"MAXTOR","XT-4170S","B5A", 1},		/* Locks on lun != 0*/
	{"MAXTOR","XT-8760S","B7B", 1}, 	/* Locks on lun != 0*/ 
	{"MEDIAVIS","CDR-H93MV","1.31", 1},	/* Locks on lun !=0 */
	{"NEC","CD-ROM DRIVE:841","1.0", 1}, 	/* Locks on lun != 0*/
	{"QUANTUM","LPS525S","3110", 1},	/* Locks on lun !=0 */
	{"QUANTUM","PD1225S","3110", 1},	/* Locks on lun !=0 */
	{"RODIME","RO3000S","2.33", 1},		/* Locks on lun != 0*/
	{"SANKYO", "CP525","6.64", 1}, 		/* causes failed REQ SENSE */
	{"SEAGATE", "ST157N", "\004|j", 1},	/* causes failed REQUEST SENSE*/
	{"SEAGATE", "ST296","921", 1},   	/* Responds to all lun */
	{"SONY","CD-ROM CDU-541","4.3d", 1},
	{"SONY","CD-ROM CDU-55S","1.0i", 1},
	{"SYMBIOS","INF-01-00","",32},		/* Symbios RAID box */
	{"TANDBERG", "TDC 3600", "", 1},	/* Locks on lun != 0*/
	{"TEAC","CD-ROM","1.06", 1},		/* causes failed REQUEST SENSE*/
	{"TEXEL","CD-ROM","1.06", 1},		/* causes failed REQUEST SENSE*/
	{"", "", "", -1}			/* MUST BE LAST */
};

#ifdef RAMD_BOOT
int sdi_ramd_boot=1;
#else
int sdi_ramd_boot=0;
#endif

int pdi_timeout = PDI_TIMEOUT;
