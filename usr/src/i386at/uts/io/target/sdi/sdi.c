#ident	"@(#)kern-pdi:io/target/sdi/sdi.c	1.125.28.1"
#ident	"$Header$"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*
 *  SDI interface module.
 */

#include <fs/buf.h>
#include <io/conf.h>
#include <io/target/scsi.h>
#include <io/target/sdi/dynstructs.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi_hier.h>
#include <io/target/sdi/sdi_layer.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>

#include <util/debug.h>	

#include <util/engine.h>
#include <util/ipl.h>
#include <util/ksynch.h>

#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>	/* must come last */

extern	struct	shad_hbatbl	*sdi_shad_hbatbl;
extern struct	hba_info	sdi_hbabind_info;
void	sdi_breakup1(), sdi_breakup2(), sdi_breakup3(), sdi_breakup4();

#define ONE_SEC 1000
/*
#define SDI_DEBUG	1
*/

int	sdi_devflag	= D_MP | D_BLKOFF;

#ifdef SDI_DEBUG
STATIC int sdi_debug = 	0;
#endif

int     sdi_docmd();
int     sdi_dofunc(struct scsi_adr *, ulong_t, int);
int     sdi_add_dev();
int     sdi_hot_rm();
int     sdi_hot_add();

int	sdi_started;	/* required for backwards compat */

int	sdi_sleepflag = KM_NOSLEEP;
int	sdi_in_rinits;
int	sdi_ctlorder;

/* sdi_hacnt, HBA_tbl and sdi_rinits are protected by sdi_rinit_lock */
int sdi_hacnt = 0;			/* number of HBAs */
int HBA_map[SDI_MAX_HBAS];	/* mapping used by sdi_gethbano */
struct hba_cfg	*HBA_tbl;
void		(**sdi_rinits)();

/* sdi event lock stuff */
LKINFO_DECL( sdi_event_lkinfo, "IO:sdi:sdi_event_lkinfo", 0);
pl_t	 	sdi_event_pl;
lock_t		*sdi_event_mutex;
sv_t		*sdi_event_sv;
STATIC int	sdi_event_usecnt;

/*
 * macros to access the edt hash table.
 */
#define EDT_HASH_LEN	31		/* edt hash table size;  use a prime */

#define EDT_HASH(X) edt_hash[EDT_HASH_INDEX(X)]

#define EDT_HASH_INDEX(X) EDTHASHVAL((X)->scsi_ctl, (X)->scsi_target,  \
				     (X)->scsi_lun, (X)->scsi_bus)
#define EDTHASHVAL(hba, scsi_id, lun, bus)		\
		( ( (unsigned int)(((bus)<<15) | ((hba)<<10) | \
			((scsi_id)<<5) | (lun)) % (unsigned int)EDT_HASH_LEN ))

#define EDT_MATCH(X1, X2) \
	(X1->scsi_adr.scsi_ctl    == X2->scsi_ctl && \
	 X1->scsi_adr.scsi_target == X2->scsi_target && \
	 X1->scsi_adr.scsi_lun    == X2->scsi_lun && \
	 X1->scsi_adr.scsi_bus    == X2->scsi_bus)

struct sdi_edt	*edt_hash[EDT_HASH_LEN];	/* edt hash table */


/*
 * Events Table, one list per device type, protected by sdi_event_mutex,
 * sdi_event_usecnt
 */
STATIC struct sdi_event_list	*sdi_events[ID_NODEV];


/*
 * the pools & freelists are only used in 4.2 and earlier;
 * we have to keep the poolheads around for compatiblity.
 */
struct head	sm_poolhead;		/* head of small structs for hba drvs */
struct head	lg_poolhead;		/* head for job structs for targ drvs */

/*
 *	Error Message Defines for SCSI and SDI
 *
 *	NOTE:  These lists are currently sorted in ascending order
 *	       on the sense or completion codes in the arrays.
 *	       Maintain this ordering or sdi_err will fail.
 */

STATIC char *sdi_ext_sense_str(uchar_t ,uchar_t);
STATIC char *sdi_comp_str(ulong);
STATIC char *sdi_status_str(uchar_t);
STATIC char *sdi_sense_str(uchar_t);
STATIC void  sdi_gauntlet_init(void);

struct CompCodeMsg {
	ulong_t		CompCode; 		/* SDI Completion Code */
	char		*Msg;			/* Message String */
};

STATIC struct CompCodeMsg CompCodeTable[] = {
	{SDI_NOALLOC,	"This block is not allocated"},
	{SDI_ASW,   	"Job completed normally"},
	{SDI_LINKF0,	"Linked command done without flag"},
	{SDI_LINKF1,	"Linked command done with flag"},
	{SDI_QFLUSH,	"Job was flushed"},
	{SDI_ABORT,   	"Command was aborted"},
	{SDI_RESET,   	"Reset was detected on the bus"},
	{SDI_CRESET,	"Reset was caused by this unit"},
	{SDI_V2PERR,	"Vtop failed"},
	{SDI_TIME,   	"Job timed out"},
	{SDI_NOTEQ,   	"Addressed device not present"},
	{SDI_HAERR,   	"Host adapter error"},
	{SDI_MEMERR,	"Memory fault"},
	{SDI_SBUSER,	"SCSI bus error"},
	{SDI_CKSTAT,	"Check the status byte"},
	{SDI_SCBERR,	"SCB error"},
	{SDI_OOS,   	"Device is out of service"},
	{SDI_NOSELE,	"The SCSI bus select failed"},
	{SDI_MISMAT,	"Parameter mismatch"},
	{SDI_PROGRES,	"Job in progress"},
	{SDI_UNUSED,	"Job not in use"},
	{SDI_ONEIC,   	"More than one immediate request"},
	{SDI_SFBERR,	"SFB error"},
	{SDI_TCERR,   	"Target protocol error detected"},
};

#define	COMP_CODE_MSG_COUNT	(sizeof(CompCodeTable)/sizeof(struct CompCodeMsg))

STATIC struct CompCodeMsg CompCodeUnk =
	{ 18, "Completion Code 0xXXXXXXXX"};

struct SenseKeyMsg {
	uchar_t		Key;			/* Sense Key */
	char		*Msg;			/* Message String */
};

STATIC struct SenseKeyMsg	SenseKeyTable[] = {
	{SD_NOSENSE, "NO SENSE INFORMATION AVAILABLE"},
	{SD_RECOVER, "RECOVERED ERROR"},
	{SD_NREADY, "NOT READY"},
	{SD_MEDIUM, "MEDIUM ERROR"},
	{SD_HARDERR, "HARDWARE ERROR"},
	{SD_ILLREQ, "ILLEGAL REQUEST"},
	{SD_UNATTEN, "UNIT ATTENTION"},
	{SD_PROTECT, "WRITE PROTECTED"},
	{SD_BLANKCK, "BLANK CHECK"},
	{SD_VENUNI, "VENDOR SPECIFIC"},
	{SD_COPYAB, "COPY/COMPARE ABORTED"},
	{SD_ABORT, "ABORTED COMMAND"},
	{SD_EQUAL, "EQUAL"},
	{SD_VOLOVR, "VOLUME OVERFLOW"},
	{SD_MISCOMP, "MISCOMPARE"},
	{SD_RESERV, "RESERVED"},
};

#define	SENSE_KEY_MSG_COUNT	(sizeof(SenseKeyTable)/sizeof(struct SenseKeyMsg))

STATIC struct SenseKeyMsg SenseKeyUnk = { 12, "Sense Key 0xXX"};

struct ExtSenseKeyMsg {
	uchar_t		Key;		/* Extended Sense Key */
	uchar_t		Qual;		/* Extended Sense Key Qualifier */
	char		*Msg;		/* Message String */
};

STATIC struct ExtSenseKeyMsg	ExtSenseKeyTable[] = {
	{SC_NOSENSE, SC_NOSENSE, "NO ADDITIONAL SENSE INFORMATION AVAILABLE"},
	{SC_NOSENSE, 0x01, "FILEMARK DETECTED"},
	{SC_NOSENSE, 0x02, "END-OF-PARTITION/MEDIUM DETECTED"},
	{SC_NOSENSE, 0x03, "SETMARK DETECTED"},
	{SC_NOSENSE, 0x04, "BEGINNING-OF-PARTITION/MEDIUM DETECTED"},
	{SC_NOSENSE, 0x05, "END-OF-DATA DETECTED"},
	{SC_NOSENSE, 0x06, "I/O PROCESS TERMINATED"},
	{SC_NOSENSE, 0x11, "AUDIO PLAY OPERATION IN PROGRESS"},
	{SC_NOSENSE, 0x12, "AUDIO PLAY OPERATION PAUSED"},
	{SC_NOSENSE, 0x13, "AUDIO PLAY OPERATION SUCCESSFULLY COMPLETED"},
	{SC_NOSENSE, 0x14, "AUDIO PLAY OPERATION STOPPED DUE TO ERROR"},
	{SC_NOSENSE, 0x15, "NO CURRENT AUDIO STATUS TO RETURN"},
	{SC_NOSGNL, 0x00, "NO INDEX/SECTOR SIGNAL"},
	{SC_NOSEEK, 0x00, "NO SEEK COMPLETE"},
	{SC_WRFLT, 0x00, "PERIPHERAL DEVICE WRITE FAULT"},
	{SC_WRFLT, 0x01, "NO WRITE CURRENT"},
	{SC_WRFLT, 0x02, "EXCESSIVE WRITE ERRORS"},
	{SC_DRVNTRDY, 0x00, "LOGICAL UNIT NOT READY, CAUSE NOT REPORTABLE"},
	{SC_DRVNTRDY, 0x01, "LOGICAL UNIT IS IN PROCESS OF BECOMING READY"},
	{SC_DRVNTRDY, 0x02, "LOGICAL UNIT NOT READY, INITIALIZING COMMAND REQUIRED"},
	{SC_DRVNTRDY, 0x03, "LOGICAL UNIT NOT READY, MANUAL INTERVENTION REQUIRED"},
	{SC_DRVNTRDY, 0x04, "LOGICAL UNIT NOT READY, FORMAT IN PROGRESS"},
	{SC_DRVNTSEL, 0x00, "LOGICAL UNIT DOES NOT RESPOND TO SELECTION"},
	{SC_NOTRKZERO, 0x00, "NO REFERENCE POSITION FOUND"},
	{SC_MULTDRV, 0x00, "MULTIPLE PERIPHERAL DEVICES SELECTED"},
	{SC_LUCOMM, 0x00, "LOGICAL UNIT COMMUNICATION FAILURE"},
	{SC_LUCOMM, 0x01, "LOGICAL UNIT COMMUNICATION TIME-OUT"},
	{SC_LUCOMM, 0x02, "LOGICAL UNIT COMMUNICATION PARITY ERROR"},
	{SC_TRACKERR, 0x00, "TRACK FOLLOWING ERROR"},
	{SC_TRACKERR, 0x01, "TRACKING SERVO FAILURE"},
	{SC_TRACKERR, 0x02, "FOCUS SERVO FAILURE"},
	{SC_TRACKERR, 0x03, "SPINDLE SERVO FAILURE"},
	{0x0A, 0x00, "ERROR LOG OVERFLOW"},
	{0x0C, 0x00, "WRITE ERROR"},
	{0x0C, 0x01, "WRITE ERROR RECOVERED WITH AUTO REALLOCATION"},
	{0x0C, 0x02, "WRITE ERROR - AUTO REALLOCATION FAILED"},
	{SC_IDERR, 0x00, "ID CRC OR ECC ERROR"},
	{SC_UNRECOVRRD, 0x00, "UNRECOVERED READ ERROR"},
	{SC_UNRECOVRRD, 0x01, "READ RETRIES EXHAUSTED"},
	{SC_UNRECOVRRD, 0x02, "ERROR TOO LONG TO CORRECT"},
	{SC_UNRECOVRRD, 0x03, "MULTIPLE READ ERRORS"},
	{SC_UNRECOVRRD, 0x04, "UNRECOVERED READ ERROR - AUTO REALLOCATE FAILED"},
	{SC_UNRECOVRRD, 0x05, "L-EC UNCORRECTABLE ERROR"},
	{SC_UNRECOVRRD, 0x06, "CIRC UNRECOVERED ERROR"},
	{SC_UNRECOVRRD, 0x07, "DATA RESYNCHRONIZATION ERROR"},
	{SC_UNRECOVRRD, 0x08, "IMCOMPLETE BLOCK READ"},
	{SC_UNRECOVRRD, 0x09, "NO GAP FOUND"},
	{SC_UNRECOVRRD, 0x0A, "MISCORRECTED ERROR"},
	{SC_UNRECOVRRD, 0x0B, "UNRECOVERED READ ERROR - RECOMMEND REASSIGNMENT"},
	{SC_UNRECOVRRD, 0x0C, "UNRECOVERED READ ERROR - RECOMMEND REWRITE THE DATA"},
	{SC_NOADDRID, 0x00, "ADDRESS MARK NOT FOUND FOR ID FIELD"},
	{SC_NOADDRDATA, 0x00, "ADDRESS MARK NOT FOUND FOR DATA"},
	{SC_NORECORD, 0x00, "RECORDED ENTITY NOT FOUND"},
	{SC_NORECORD, 0x01, "RECORD NOT FOUND"},
	{SC_NORECORD, 0x02, "FILEMARK OR SETMARK NOT FOUND"},
	{SC_NORECORD, 0x03, "END-OF-DATA NOT FOUND"},
	{SC_NORECORD, 0x04, "BLOCK SEQUENCE ERROR"},
	{SC_SEEKERR, 0x00, "RANDOM POSITIONING ERROR"},
	{SC_SEEKERR, 0x01, "MECHANICAL POSITIONING ERROR"},
	{SC_SEEKERR, 0x02, "POSITIONING ERROR DETECTED BY READ OF MEDIUM"},
	{SC_DATASYNCMK, 0x00, "DATA SYNCHRONIZATION MARK ERROR"}, 
	{SC_RECOVRRD, 0x00, "RECOVERED DATA WITH NO ERROR CORRECTION APPLIED"},
	{SC_RECOVRRD, 0x01, "RECOVERED DATA WITH RETRIES"},
	{SC_RECOVRRD, 0x02, "RECOVERED DATA WITH POSITIVE HEAD OFFSET"},
	{SC_RECOVRRD, 0x03, "RECOVERED DATA WITH NEGATIVE HEAD OFFSET"},
	{SC_RECOVRRD, 0x04, "RECOVERED DATA WITH RETRIES AND/OR CIRC APPLIED"},
	{SC_RECOVRRD, 0x05, "RECOVERED DATA USING PREVIOUS SECTOR ID"},
	{SC_RECOVRRD, 0x06, "RECOVERED DATA WITHOUT ECC - DATA AUTO-REALLOCATED"},
	{SC_RECOVRRD, 0x07, "RECOVERED DATA WITHOUT ECC - RECOMMENDED REASSIGNMENT"},
	{SC_RECOVRRDECC, 0x00, "RECOVERED DATA WITH ERROR CORRECTION APPLIED"},
	{SC_RECOVRRDECC, 0x01, "RECOVERED DATA WITH ERROR CORRECTION AND RETRIES APPLIED"},
	{SC_RECOVRRDECC, 0x02, "RECOVERED DATA - DATA AUTO-REALLOCATED"},
	{SC_RECOVRRDECC, 0x03, "RECOVERED DATA WITH CIRC"},
	{SC_RECOVRRDECC, 0x04, "RECOVERED DATA WITH LEC"},
	{SC_RECOVRRDECC, 0x05, "RECOVERED DATA - RECOMMEND REASSIGNMENT"},
	{SC_DFCTLSTERR, 0x00, "DEFECT LIST ERROR"},
	{SC_DFCTLSTERR, 0x01, "DEFECT LIST NOT AVAILABLE"},
	{SC_DFCTLSTERR, 0x02, "DEFECT LIST ERROR IN PRIMARY LIST"},
	{SC_DFCTLSTERR, 0x03, "DEFECT LIST ERROR IN GROWN LIST"},
	{SC_PARAMOVER, 0x00, "PARAMETER LIST LENGTH ERROR"},
	{SC_SYNCTRAN, 0x00, "SYNCHRONOUS DATA TRANSFER ERROR"},
	{SC_NODFCTLST, 0x00, "DEFECT LIST NOT FOUND"},
	{SC_NODFCTLST, 0x01, "PRIMARY DEFECT LIST NOT FOUND"},
	{SC_NODFCTLST, 0x02, "GROWN DEFECT LIST NOT FOUND"},
	{SC_CMPERR, 0x00, "MISCOMPARE DURING VERIFY OPERATION"},
	{SC_RECOVRIDECC, 0x00, "RECOVERED ID WITH ECC CORRECTION"},
	{SC_INVOPCODE, 0x00, "INVALID COMMAND OPERATION CODE"},
	{SC_ILLBLCK, 0x00, "LOGICAL BLOCK ADDRESS OUT OF RANGE"},
	{SC_ILLBLCK, 0x01, "INVALID ELEMENT ADDRESS"},
	{SC_ILLFUNC, 0x00, "ILLEGAL FUNCTION FOR DEVICE TYPE"},
	{SC_ILLCDB, 0x00, "INVALID FIELD IN CDB"},
	{SC_INVLUN, 0x00, "LOGICAL UNIT NOT SUPPORTED"},
	{SC_INVPARAM, 0x00, "INVALID FIELD IN PARAMETER LIST"},
	{SC_INVPARAM, 0x01, "PARAMETER NOT SUPPORTED"},
	{SC_INVPARAM, 0x02, "PARAMETER VALUE INVALID"},
	{SC_INVPARAM, 0x03, "THRESHOLD PARAMETERS NOT SUPPORTED"},
	{SC_WRPROT, 0x00, "WRITE PROTECTED"},
	{SC_MEDCHNG, 0x00, "NOT READY TO READY TRANSITION (MEDIUM MAY HAVE CHANGED)"},
	{SC_MEDCHNG, 0x01, "IMPORT OR EXPORT ELEMENT ACCESSED"},
	{SC_RESET, 0x00, "POWER ON, RESET, OR BUS DEVICE RESET OCCURRED"},
	{SC_MDSELCHNG, 0x00, "PARAMETERS CHANGED"},
	{SC_MDSELCHNG, 0x01, "MODE PARAMETERS CHANGED"},
	{SC_MDSELCHNG, 0x02, "LOG PARAMETERS CHANGED"},
	{0x2B, 0x00, "COPY CANNOT EXECUTE SINCE HOST CANNOT DISCONNECT"},
	{0x2C, 0x00, "COMMAND SEQUENCE ERROR"},
	{0x2C, 0x01, "TOO MANY WINDOWS SPECIFIED"},
	{0x2C, 0x02, "INVALID COMBINATION OF WINDOWS SPECIFIED"},
	{0x2D, 0x00, "OVERWRITE ERROR ON UPDATE IN PLACE"},
	{0x2F, 0x00, "COMMANDS CLEARED BY ANOTHER INITIATOR"},
	{SC_INCOMP, 0x00, "INCOMPATIBLE MEDIUM INSTALLED"},
	{SC_INCOMP, 0x01, "CANNOT READ MEDIUM - UNKNOWN FORMAT"},
	{SC_INCOMP, 0x02, "CANNOT READ MEDIUM - INCOMPATIBLE FORMAT"},
	{SC_INCOMP, 0x03, "CLEANING CARTRIDGE INSTALLED"},
	{SC_FMTFAIL, 0x00, "MEDIUM FORMAT CORRUPTED"},
	{SC_FMTFAIL, 0x01, "FORMAT COMMAND FAILED"},
	{SC_NODFCT, 0x00, "NO DEFECT SPARE LOCATION AVAILABLE"},
	{SC_NODFCT, 0x01, "DEFECT LIST UPDATE FAILURE"},
	{0x33, 0x00, "TAPE LENGTH ERROR"},
	{0x36, 0x00, "RIBBON, INK, OR TONER FAILURE"},
	{0x37, 0x00, "ROUNDED PARAMETER"},
	{0x38, 0x00, "SEQUENTIAL POSITIONING ERROR"},
	{0x38, 0x0D, "MEDIUM DESTINATION ELEMENT FULL"},
	{0x39, 0x00, "SAVING PARAMETERS NOT SUPPORTED"},
	{0x3A, 0x00, "MEDIUM NOT PRESENT"},
	{0x3B, 0x01, "TAPE POSITION ERROR AT BEGINNING-OF-MEDIUM"},
	{0x3B, 0x02, "TAPE POSITION ERROR AT END-OF-MEDIUM"},
	{0x3B, 0x03, "TAPE OR ELECTRONIC VERTICAL FORMS UNIT NOT READY"},
	{0x3B, 0x04, "SLEW FAILURE"},
	{0x3B, 0x05, "PAPER JAM"},
	{0x3B, 0x06, "FAILED TO SENSE TOP-OF-FORM"},
	{0x3B, 0x07, "FAILED TO SENSE BOTTOM-OF-FORM"},
	{0x3B, 0x08, "REPOSITION ERROR"},
	{0x3B, 0x09, "READ PAST END OF MEDIUM"},
	{0x3B, 0x0A, "READ PAST BEGINNING OF MEDIUM"},
	{0x3B, 0x0B, "POSITION PAST END OF MEDIUM"},
	{0x3B, 0x0C, "POSITION PAST BEGINNING OF MEDIUM"},
	{0x3B, 0x0D, "MEDIUM DESTINATION ELEMENT FULL"},
	{0x3B, 0x0E, "MEDIUM SOURCE ELEMENT EMPTY"},
	{0x3D, 0x00, "INVALID BITS IN IDENTIFY MESSAGE"},
	{0x3E, 0x00, "LOGICAL UNIT HAS NOT SELF-CONFIGURED YET"},
	{0x3F, 0x00, "TARGET OPERATING CONDITIONS HAVE CHANGED"},
	{0x3F, 0x01, "MICROCODE HAS BEEN CHANGED"},
	{0x3F, 0x02, "CHANGED OPERATING DEFINITION"},
	{0x3F, 0x03, "INQUIRY DATA HAS CHANGED"},
	{SC_RAMFAIL, 0x00, "VENDOR UNIQUE"},
	{SC_DATADIAG, 0x00, "DATA PATH FAILURE"},
	{SC_POWFAIL, 0x00, "POWER-ON OR SELF-TEST FAILURE"},
	{SC_MSGREJCT, 0x00, "MESSAGE ERROR"},
	{SC_CONTRERR, 0x00, "INTERNAL TARGET FAILURE"},
	{SC_SELFAIL, 0x00, "SELECT OR RESELECT FAILURE"},
	{SC_SOFTRESET, 0x00, "UNSUCCESSFUL SOFT RESET"},
	{SC_PARITY, 0x00, "SCSI PARITY ERROR"},
	{SC_INITERR, 0x00, "INITIATOR DETECTED ERROR MESSAGE RECEIVED"},
	{SC_ILLMSG, 0x00, "INVALID MESSAGE ERROR"},
	{0x4A, 0x00, "COMMAND PHASE ERROR"},
	{0x4B, 0x00, "DATA PHASE ERROR"},
	{0x4C, 0x00, "LOGICAL UNIT FAILED SELF-CONFIGURATION"},
	{0x4E, 0x00, "OVERLAPPED COMMANDS ATTEMPTED"},
	{0x50, 0x00, "WRITE APPEND ERROR"},
	{0x50, 0x01, "WRITE APPEND POSITION ERROR"},
	{0x50, 0x02, "POSITION ERROR RELATED TO TIMING"},
	{0x51, 0x00, "ERASE FAILURE"},
	{0x52, 0x00, "CARTRIDGE FAULT"},
	{0x53, 0x00, "MEDIA LOAD OR EJECT FAILED"},
	{0x53, 0x01, "UNLOAD TAPE FAILURE"},
	{0x53, 0x02, "MEDIUM REMOVAL PREVENTED"},
	{0x54, 0x00, "SCSI TO HOST SYSTEM INTERFACE FAILURE"},
	{0x55, 0x00, "SYSTEM RESOURCE FAILURE"},
	{0x57, 0x00, "UNABLE TO RECOVER TABLE-OF-CONTENTS"},
	{0x58, 0x00, "GENERATION DOES NOT EXIST"},
	{0x59, 0x00, "UPDATED BLOCK READ"},
	{0x5A, 0x00, "OPERATOR REQUEST OR STATE CHANGE INPUT (UNSPECIFIED)"},
	{0x5A, 0x01, "OPERATOR MEDIUM REMOVAL REQUEST"},
	{0x5A, 0x02, "OPERATOR SELECTED WRITE PROTECT"},
	{0x5A, 0x03, "OPERATOR SELECTED WRITE PERMIT"},
	{0x5B, 0x00, "LOG EXEPTION"},
	{0x5B, 0x01, "THRESHOLD CONDITION MET"},
	{0x5B, 0x02, "LOG COUNTER AT MAXIMUM"},
	{0x5B, 0x03, "LOG LIST CODES EXHAUSTED"},
	{0x5C, 0x00, "RPL STATUS CHANGE"},
	{0x5C, 0x01, "SPINDLES SYNCHRONIZED"},
	{0x5C, 0x02, "SPINDLES NOT SYNCHRONIZED"},
	{0x60, 0x00, "LAMP FAILURE"},
	{0x61, 0x00, "VIDEO ACQUISITION ERROR"},
	{0x61, 0x01, "UNABLE TO ACQUIRE VIDEO"},
	{0x61, 0x02, "OUT OF FOCUS"},
	{0x62, 0x00, "SCAN HEAD POSITIONING ERROR"},
	{0x63, 0x00, "END OF USER AREA ENCOUNTERED ON THIS TRACK"},
	{0x64, 0x00, "ILLEGAL MODE FOR THIS TRACK"}
	/*
	 * Vender Unique Sense Key's (0x80->0xFF) and Vender Unique
	 * Qualifiers (0x80->0xFF) need to be checked explicitely.
	 */ 
};

#define EXT_SENSE_KEY_MSG_COUNT	(sizeof(ExtSenseKeyTable)/sizeof(struct ExtSenseKeyMsg))

STATIC struct ExtSenseKeyMsg ExtSenseKeyUnk =
	{ 24, 43, "Additional Sense Code 0xXX and Qualifier 0xXX"};

struct StatusMsg {
	uchar_t		Status; 	/* SCSI Status Byte from Target */
	char		*Msg;		/* Message String */
};

STATIC struct StatusMsg StatusTable[] = {
	{S_GOOD, "GOOD"},
	{S_CKCON, "CHECK CONDITION"},
	{S_METGD, "CONDITION MET"},
	{S_BUSY, "BUSY"},
	{S_INGD, "INTERMEDIATE"},
	{S_INMET, "INTERMEDIATE-CONDITION MET"},
	{S_RESER, "RESERVATION CONFLICT"},
	{0x22, "COMMAND TERMINATED"},
	{0x28, "QUEUE FULL"},
};

#define	STATUS_MSG_COUNT	(sizeof(StatusTable)/sizeof(struct StatusMsg))

STATIC struct StatusMsg StatusUnk = { 14, "SCSI Status 0xXX"};

char *sdi_devicenames;

char *sdi_lunsearch;

char *sdi_scsi_pdt[] = {	/* SCSI Peripherial Device types  */
        "HBA          ",	/* the HBA */
	"DISK         ",	/* ID_RANDOM		*/
	"TAPE         ",	/* ID_TAPE		*/
	"PRINTER      ",	/* ID_PRINTER		*/
	"PROCESSOR    ",	/* ID_PROCESSOR		*/
	"WORM         ",	/* ID_WORM		*/
	"CDROM        ",	/* ID_ROM		*/
	"SCANNER      ",	/* ID_SCANNER		*/
	"OPTICAL      ",	/* ID_OPTICAL		*/
	"CHANGER      ",	/* ID_CHANGER		*/
	"COMMUNICATION",	/* ID_COMMUNICATION	*/
};
extern int sdi_rtabsz;		/* Number of slots in rinit table */
extern int sdi_hbaswsz;		/* Number of slots in HBA table */
extern major_t sdi_pass_thru_major;	/* major number for sdi_only pass-thru */
extern struct sdi_blist sdi_limit_lun[];
extern sdi_cklu_dnr_cnt; /* Delay count for sdi_cklu */

sleep_t	*sdi_rinit_lock;	/* also used in sdi/conf.c */
sleep_t *sdi_hot_lock;		/* lock access in sdi_hot_rm and sdi_hot_add */
lock_t	*sdi_edt_mutex;		/* also used in sdi/conf.c */
pl_t	 sdi_edt_pl;		/* also used in sdi/conf.c */

/* the lkinfo for sdi rinit sleep lock */
LKINFO_DECL( sdi_rinit_lkinfo, "IO:sdi:sdi_rinit_lkinfo", 0);

/* the lkinfo for the hot sleep lock */
LKINFO_DECL( sdi_hot_lkinfo, "IO:sdi:sdi_hot_lkinfo", 0);

/* the lkinfo for sdi edt lock */
LKINFO_DECL( sdi_edt_lkinfo, "IO:sdi:sdi_edt_lkinfo", 0);

struct sdi_edt *sdi_rxedt_l(struct scsi_adr *);

STATIC void (*sdi_physio_start(buf_t *bp, void (*strat)(buf_t *)))(buf_t *);

/*
 * int sdi_lun_invalidate(char *str, struct scsi_adr sap)
 *
 * takes an string of the form: (c,b,t,l),... or (c:b,t,l)...
 * all elements are optional and if omitted become wildcards
 * the ()'s must always be present
 * The c,b,t,l represent controller, bus, target, lun.
 * The c,b,t,l must be positive integers.
 * Whitespace is not legal.
 *
 * The argument sap is the scsi address that is to be validated.
 * If it is within the values specified by str then the set is invalid.
 * Wildcards within sap are not valid; sap must be fully qualified.
 * 
 * If the scsi address in sap is invalid, 0 is returned
 * otherwise 1 is returned.
 */
#define IS_NUM(c) (c >= '0' && (c <= '9'))
int
sdi_lun_validate(char *names, struct scsi_adr *sap)
{
	int c, b, t, l;
	int cmax, bmax, tmax, lmax;
	int cmin, bmin, tmin, lmin;

	if (names == NULL) return 1;

	while (*names != '\0')
	{
		c = b = t = l = -1;

		if (*names == ',') names++;

		if (*names != '(') return 1; /* error */

		names++;
		/* see if the controller is specified */
		if (IS_NUM(*names))
		{
			c = 0;
			while (IS_NUM(*names))
			{
				c = c * 10 + *names - '0';
				names++;
			}
		}
		if ((*names == ':') || (*names == ',')) names++;

		if (*names == '\0') return 1; /* error */
		
		/* see if the bus is specified */
		if (IS_NUM(*names))
		{
			b = 0;
			while (IS_NUM(*names))
			{
				b = b * 10 + *names - '0';
				names++;
			}
		}

		if ((*names == ':') || (*names == ',')) names++;

		if (*names == '\0') return 1; /* error */

		/* see if the target is specified */
		if (IS_NUM(*names))
		{
			t = 0;
			while (IS_NUM(*names))
			{
				t = t * 10 + *names - '0';
				names++;
			}
		}

		if ((*names == ':') || (*names == ',')) names++;

		if (*names == '\0') return 1; /* error */

		/* see if the lun is specified */
		if (IS_NUM(*names))
		{
			l = 0;
			while (IS_NUM(*names))
			{
				l = l * 10 + *names - '0';
				names++;
			}
		}

		if (*names != ')') return 1; /* error */

		names++;

		if (c == -1) cmin = 0, cmax = MAX_EXHAS;
		else         cmin = c, cmax = c + 1;
		if (cmax > MAX_EXHAS) return 1;

		if (b == -1) bmin = 0, bmax = MAX_BUS;
		else         bmin = b, bmax = b + 1;
		if (bmax > MAX_BUS) return 1;

		if (t == -1) tmin = 0, tmax = MAX_EXTCS;
		else         tmin = t, tmax = t + 1;
		if (tmax > MAX_EXTCS) return 1;

		if (l == -1) lmin = 1, lmax = MAX_EXLUS;
		else         lmin = l, lmax = l + 1;
		if (lmax > MAX_EXLUS) return 1;

		if ( (cmin <= sap->scsi_ctl) && (sap->scsi_ctl < cmax) &&
		     (bmin <= sap->scsi_bus) && (sap->scsi_bus < bmax) &&
		     (tmin <= sap->scsi_target) && (sap->scsi_target < tmax) &&
		     (lmin <= sap->scsi_lun) && (sap->scsi_lun < lmax) )
			return 0;
	}
	return 1;
}

/*
 * int sdi_device_validate(struct ident *idp, struct scsi_adr *sap)
 * returns how many contigous luns may be probed according to the sdi_limit_lun
 * entry. Leading spaces in sdi_limit_lun and idp are ignored.  The compare
 * is limited to the length of the sdi_limit_lun[] string. The last set of
 * 3 elements must all be "".
 *
 * Returns 1 if the device is valid, 0 if not.
 */
STATIC int
sdi_device_validate(struct ident *idp)
{
	int i;
	char *s1, *s2;

	for (i = 0; 1; i++) {
		if ((sdi_limit_lun[i].vendor[0] == '\0') &&
		    (sdi_limit_lun[i].prod[0] == '\0') &&
		    (sdi_limit_lun[i].rev[0] == '\0'))
			return -1;

		s1 = idp->id_vendor;
		while (*s1 == ' ') s1++;
		s2 = sdi_limit_lun[i].vendor;
		while (*s2 == ' ') s2++;
		if (strncmp(s1, s2, strlen(s2)))
			continue;

		s1 = idp->id_prod;
		while (*s1 == ' ') s1++;
		s2 = sdi_limit_lun[i].prod;
		while (*s2 == ' ') s2++;
		if (strncmp(s1, s2, strlen(s2)))
			continue;

		s1 = idp->id_revnum;
		while (*s1 == ' ') s1++;
		s2 = sdi_limit_lun[i].rev;
		while (*s2 == ' ') s2++;
		if (strncmp(s1, s2, strlen(s2)))
			continue;

		return sdi_limit_lun[i].lun;
	}
}

/*
 * void
 * sdi_init(void)
 *	allocate & init sdi structs
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_init(void)
{
	register int i;

	if ( (HBA_tbl = (struct hba_cfg *)
	      kmem_zalloc((sdi_hbaswsz*sizeof(struct hba_cfg)), KM_NOSLEEP))
	    == NULL ||
	    (sdi_rinits = (void (**)())
	     kmem_zalloc((sdi_rtabsz*sizeof(void (*)())), KM_NOSLEEP))
	    == NULL  )
		/*
		 *+ Allocation of sdi data structures failed at
		 *+ boot time, when there should be plenty of
		 *+ memory.  Check system configuration.
		 */
		cmn_err(CE_PANIC,
			"sdi_init: cannot allocate data structures\n");
	
	if (!(sdi_rinit_lock = SLEEP_ALLOC(SDI_HIER_BASE,
					   &sdi_rinit_lkinfo, KM_NOSLEEP)) ||
	    !(sdi_hot_lock = SLEEP_ALLOC(SDI_HIER_BASE,
					 &sdi_hot_lkinfo, KM_NOSLEEP)) ||
	    !(sdi_event_mutex = LOCK_ALLOC(SDI_HIER_BASE, pldisk,
					 &sdi_event_lkinfo, KM_NOSLEEP)) ||
	    !(sdi_event_sv = SV_ALLOC(KM_NOSLEEP)) ||
	    !(sdi_edt_mutex = LOCK_ALLOC(SDI_HIER_BASE+1,
					 pldisk, &sdi_edt_lkinfo, KM_NOSLEEP)) )
		/*
		 *+ Allocation of sdi data structures failed at
		 *+ boot time, when there should be plenty of
		 *+ memory.  Check system configuration.
		 */
		cmn_err(CE_PANIC,
			"sdi_init: cannot allocate data structures\n");

	sdi_gauntlet_init();

	sdi_layers_init();
	

#ifdef SDI_DEBUG
	if(sdi_debug >= 1){
		cmn_err(CE_CONT,"edt_hash = %x\n", edt_hash);
	}
#endif

	/*
	 * Init the edt hash table.  These are also free
	 * edt structures, which are used before diving
	 * into the free pool, and are the head of each
	 * hash queue.
	 */
	for (i=0; i <  EDT_HASH_LEN; i++) {
		edt_hash[i] = (struct sdi_edt *)NULL;
	}

	/* for backwards compat only */
	sm_poolhead.f_isize = SM_POOLSIZE;
	lg_poolhead.f_isize = LG_POOLSIZE;

	if (sdi_devicenames != NULL)
	{
		char *p1, *p2, *src, *dst;
		int last=0;
		int length;

		p1 = sdi_devicenames;

		for (i = 0; !last && i <= ID_COMMUNICATION + 1 ; i++)
		{
			for (p2 = p1;
			     *p2 != '\0' && *p2 != ',';
			     p2++);

			if (*p2 == '\0')
			{
				last=1;
			}
			else
			{
				*p2 = '\0';
				p2++;
			}

			if (strlen(sdi_scsi_pdt[i]) < strlen(p1))
				length=strlen(sdi_scsi_pdt[i]);
			else
				length=strlen(p1);
			
			for (dst=sdi_scsi_pdt[i];
			     *dst != '\0';
			     dst++)
				*dst = ' ';

			for (src=p1, dst=sdi_scsi_pdt[i];
			     length;
			     length--, dst++, src++)
			{
				*dst = *src;
			}
			p1 = p2;
		}
	}
		
	for (i=0; i < (sizeof(HBA_map) / sizeof(int)); i++)
		HBA_map[i] = SDI_UNUSED_MAP;
	
}

/*
 * void
 * sdi_postroot(void)
 *	set sdi_sleepflag to KM_SLEEP -- indicates we've passed
 *	init/start.   *postroot() is not a ddi/dki interface, and
 *	is not to be used by target or hba drivers; sdi got special
 *	dispensation since it's "driver glue" rather than a driver itself.
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_postroot(void)
{
	sdi_sleepflag = KM_SLEEP;
}

/*
 * int
 * sdi_cklu(struct scsi_adr *sap, char *inquire,
 *	unsigned char *pd_type, struct ident *inq_datap, struct sense *sensep)
 *	check logical unit number
 *
 * both reads and sets pd_type
 * sets inquire if LUN==0
 * reads inquire if LUN!=0
 * reads inq_datap but will set if it does a START
 * reads sap
 *
 * Calling/Exit State:
 *	sdi_rinit_lock held across call, unless is called at init/start.
 */
int
sdi_cklu(struct scsi_adr *sap, char *inquire,
	unsigned char *pd_type, struct ident *inq_datap, struct sense *sensep)
{
	struct scs	cdb;
	int	i, retcode;

	ASSERT(sap);
	ASSERT(inquire);
	ASSERT(pd_type);
	ASSERT(inq_datap);

	/*
	 * If the target we are currently looking at is the
	 * host adapter, it is good by definition.
	 */
	if((IDP(HBA_tbl[sap->scsi_ctl].idata)->version_num & HBA_VMASK) >=
		HBA_UW21_IDATA) {
		if (sap->scsi_target ==
			IDP(HBA_tbl[sap->scsi_ctl].idata)->ha_chan_id[sap->scsi_bus])
			return B_TRUE;
	} else {
		if (sap->scsi_target == IDP(HBA_tbl[sap->scsi_ctl].idata)->ha_id)
			return B_TRUE;
	}

	/*
	 * If the id_type of the device is ID_NODEV, it is not a valid device.
	 */
	if (inq_datap->id_type == ID_NODEV)
		return B_FALSE;

	/*
	 *	We will now check the peripheral qualifier, id_pqual,
	 *	returned by the device.
	 */
	if (inq_datap->id_pqual == ID_QNOLU)
		return B_FALSE;

	/*
	 * If the device we just inquired is type ID_RANDOM and it doesn't have
	 * removable media, do an SS_TEST on it to see if it really exists.
	 *
	 * Some devices answer the SS_INQUIRE at a bunch of the lun's for the
	 * same target.  This test allows us to eliminate some of these.
	 *
	 * We check for removable media since if the device is capable of
	 * removable media, it won't test ready unless the media is inserted
	 * and we have no way to guarentee this.
	 */
	if ((inq_datap->id_type == ID_RANDOM) && (!(inq_datap->id_rmb))) {

		cdb.ss_op = SS_TEST; /* test unit ready cdb */
		cdb.ss_lun = sap->scsi_lun;
		cdb.ss_addr = NULL;
		cdb.ss_addr1 = NULL;
		cdb.ss_len = NULL;
		cdb.ss_cont = NULL;

		if (sdi_docmd(sap, &cdb, SCS_SZ, NULL, NULL, B_READ) 
		    == SDI_ASW) {
			return B_TRUE;
		}
		
		/* Something is wrong.  Could be a Unit Attention,
		 * Not Ready, or error.  
		 */
		cdb.ss_op = SS_REQSEN;
		cdb.ss_lun = sap->scsi_lun;
		cdb.ss_addr = NULL;
		cdb.ss_addr1 = NULL;
		cdb.ss_len = SENSE_SZ;
		cdb.ss_cont = NULL;
		(void)sdi_docmd(sap, &cdb, SCS_SZ, SENSE_AD(sensep), SENSE_SZ, B_READ);

		cdb.ss_op = SS_TEST; /* test unit ready cdb */
		cdb.ss_lun = sap->scsi_lun;
		cdb.ss_addr = NULL;
		cdb.ss_addr1 = NULL;
		cdb.ss_len = NULL;
		cdb.ss_cont = NULL;

		if (((retcode = sdi_docmd(sap, &cdb, SCS_SZ, NULL, NULL, 
			B_READ)) == SDI_ASW) || (retcode == SDI_SBRESC)) {
			/* The problem was a Unit Attention.  */
			/*
			** If we fail it is still possible that the device is
			** OK and that the target is reserved
			** by the other initiator. We check to see if
			** the TUR failed with a reservation
			** conflict. If it did we return true so the target
			** is recognized by SDI.
			*/
			return B_TRUE;
		}

		/* Somthing is still wrong.  Could be Not Ready or error.
		 */
		cdb.ss_op = SS_REQSEN;
		cdb.ss_lun = sap->scsi_lun;
		cdb.ss_addr = NULL;
		cdb.ss_addr1 = NULL;
		cdb.ss_len = SENSE_SZ;
		cdb.ss_cont = NULL;
		(void)sdi_docmd(sap, &cdb, SCS_SZ, SENSE_AD(sensep), SENSE_SZ, B_READ);

		if (sensep->sd_key == SD_NREADY)
		{
			/* The error is a not ready.
			 * This means the device is not ready for medium
			 * access commands.  To fix this spin up the drive.
			 */

			/* START UNIT command  to spin up disk */
			cdb.ss_op = SS_LOAD;
			cdb.ss_lun = sap->scsi_lun;
			cdb.ss_addr = NULL;
			cdb.ss_addr1 = NULL;
			cdb.ss_len = 1;
			cdb.ss_cont = NULL;
			(void)sdi_docmd(sap, &cdb, SCS_SZ, NULL,
					0, B_READ);
			
			for(i = 0; i < sdi_cklu_dnr_cnt; i++) {
				/* TUR to make sure everything is good
				*/
				cdb.ss_op = SS_TEST;
				cdb.ss_lun = sap->scsi_lun;
				cdb.ss_addr = NULL;
				cdb.ss_addr1 = NULL;
				cdb.ss_len = NULL;
				cdb.ss_cont = NULL;

				if (sdi_docmd(sap, &cdb, SCS_SZ, NULL,
					0, B_READ) != SDI_ASW) {
					/*
					** Check the addtional sense code 
					** and sense code qualifier
					** to see if the device is in the 
					** process of becoming
					** ready. If it is delay then 
					** TUR again until the device
					** becomes ready or until we get 
					** a different status.
					*/
					cdb.ss_op = SS_REQSEN;
					cdb.ss_lun = sap->scsi_lun;
					cdb.ss_addr = NULL;
					cdb.ss_addr1 = NULL;
					cdb.ss_len = SENSE_SZ;
					cdb.ss_cont = NULL;
					(void)sdi_docmd(sap, &cdb, SCS_SZ, 
						SENSE_AD(sensep), SENSE_SZ, B_READ);
					if((sensep->sd_sencode == SC_DRVNTRDY) &&
						(sensep->sd_qualifier == 0x01)) {
						/* Driver becoming ready...
						** wait then TUR again */
						drv_usecwait(SDI_CKLU_DELAY);
						continue;
					}
					return B_FALSE;
				} else {
					break;
				}
			}
			
			/* An INQUIRE is sent to update the
			 * inquire string which may be wrong if the
			 * drive wasn't spun up.
			 */
			cdb.ss_op = SS_INQUIR;
			cdb.ss_lun = sap->scsi_lun;
			cdb.ss_addr = 0;
			cdb.ss_addr1 = 0;
			cdb.ss_len = IDENT_SZ;
			cdb.ss_cont = 0;
			
			if (sdi_docmd(sap, &cdb, SCS_SZ, inq_datap,
				      IDENT_SZ, B_READ) != SDI_ASW) {
				return B_FALSE;
			}
			return B_TRUE;
		}
		/* it's an error */
		return B_FALSE;
	}

	if (inq_datap->id_type == ID_TAPE) {
		/*
		 * Some ill behaved tapes return a valid ident string
		 * on luns for which no tapes exist, so if the ident
		 * string for luns>0 match the lun 0 exactly, then
 		 * we treat this lun as un-equiped.  If the
 		 * inquires do not match, we assume the device
		 * really exists.
		 */
		if(sap->scsi_lun == 0) {
               		(void)strncpy(inquire, inq_datap->id_vendor, INQ_EXLEN);
               		inquire[INQ_EXLEN-1] = NULL;
               		*pd_type = inq_datap->id_type;
		} else {
       			if ((inq_datap->id_type == *pd_type) &&
               		(!strncmp(inq_datap->id_vendor, inquire, (INQ_EXLEN-1)))) {
               			return B_FALSE;
			}
		}
	}
	return B_TRUE;
}

/*
 * struct sdi_edt *
 * sdi_redt(int hba, int scsi_id, int lun)
 *	search the EDT for the specified entry.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Holds sdi_edt_mutex across the call to sdi_rxedt_l()
 */
struct sdi_edt *
sdi_redt(int hba, int scsi_id, int lun)
{
	struct sdi_edt *edtp;
	struct scsi_adr scsi_adr;

	sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk);

	scsi_adr.scsi_ctl = hba;
	scsi_adr.scsi_target = scsi_id;
	scsi_adr.scsi_lun = lun;
	scsi_adr.scsi_bus = 0;

	edtp = sdi_rxedt_l(&scsi_adr);

	UNLOCK(sdi_edt_mutex, sdi_edt_pl);
	return edtp;
}

/*
 * struct sdi_edt *
 * sdi_rxedt(int hba, int bus, int scsi_id, int lun)
 *	search the EDT for the specified entry.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Holds sdi_edt_mutex across the call to sdi_rxedt_l()
 */
struct sdi_edt *
sdi_rxedt(int hba, int bus, int scsi_id, int lun)
{
	struct sdi_edt *edtp;
	struct scsi_adr scsi_adr;

	sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk);

	scsi_adr.scsi_ctl = hba;
	scsi_adr.scsi_target = scsi_id;
	scsi_adr.scsi_lun = lun;
	scsi_adr.scsi_bus = bus;

	edtp = sdi_rxedt_l(&scsi_adr);

	UNLOCK(sdi_edt_mutex, sdi_edt_pl);
	return edtp;
}

/*
 * struct sdi_edt *
 * sdi_rxedt_l(struct scsi_adr *sap)
 *	search the EDT for the specified entry.
 *
 * Calling/Exit State:
 *	sdi_edt_mutex is held across the call
 */
struct sdi_edt *
sdi_rxedt_l(struct scsi_adr *sap)
{
	struct sdi_edt *edtp;

	ASSERT(sap);
	edtp = EDT_HASH(sap);
#ifdef SDI_DEBUG
	if(sdi_debug > 1) {
		cmn_err(CE_CONT,"sdi_rxedt_l(%x, %x, %x, %x)\n",
			sap->scsi_ctl, sap->scsi_target,
			sap->scsi_lun, sap->scsi_bus);
		cmn_err(CE_CONT,"edtp = %x\n", edtp);
	}
#endif
	for ( ; edtp && edtp->scsi_adr.scsi_ctl != -1; edtp = edtp->hash_p)
		if (EDT_MATCH(edtp, sap))
			return edtp;
	return (struct sdi_edt *)NULL;
}

/************************************************************
 *
 * Allocates an struct edt 
 *
 ************************************************************/
struct sdi_edt
*sdi_alloc_edt(void)
{
	struct sdi_edt *edtp;

	edtp = (struct sdi_edt *)kmem_zalloc(sizeof(struct sdi_edt),
					     sdi_sleepflag);
	ASSERT(edtp);
	return edtp;
}

/************************************************************
 *
 * Deallocates the space used by the provided edtp.
 *
 ************************************************************/
void 
sdi_free_edt(struct sdi_edt *edtp)
{
	ASSERT(edtp);
	kmem_free(edtp->timeout, SDI_TIMEOUT_VALUES * sizeof(ushort_t));
	kmem_free(edtp, sizeof(struct sdi_edt));
}

/************************************************************
 * Deletes from the edt hash table and frees the passed edtp.
 * returns SDI_RET_OK on success, SDI_RET_ERR if edtp could
 * not be found in the edt.
 *************************************************************/
int
sdi_delete_edt(struct sdi_edt *edtp)
{
	int hash_index;
	struct sdi_edt *hash;
	
	sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk);

	ASSERT(edtp);

	hash_index = EDT_HASH_INDEX(&(edtp->scsi_adr));

	hash = edt_hash[hash_index];

	if ( hash == (struct sdi_edt *)NULL)
	{
		UNLOCK(sdi_edt_mutex, sdi_edt_pl);
		return SDI_RET_ERR;
	}
	
	if (hash == edtp)
	{
		/* delete head */
		edtp = hash->hash_p;
		sdi_free_edt(hash);
		edt_hash[hash_index] = edtp;
		UNLOCK(sdi_edt_mutex, sdi_edt_pl);
		return SDI_RET_OK;
	}

	for (; hash->hash_p != (struct sdi_edt *)NULL; hash = hash->hash_p)
	{
		if (hash->hash_p == edtp)
		{
			/* delete from the middle or end */
			edtp = hash->hash_p->hash_p;
			sdi_free_edt(hash->hash_p);
			hash->hash_p = edtp;
			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			return SDI_RET_OK;
		}
	}
	
	UNLOCK(sdi_edt_mutex, sdi_edt_pl);
	return SDI_RET_ERR;
}

/************************************************************
 *
 * Inserts the passed edtp into the edt hash table
 *
 ************************************************************/
void 
sdi_insert_edt(struct sdi_edt *edtp)
{
	int hash_index;
	struct sdi_edt *hash;

	sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk);

	ASSERT(edtp);

	hash_index = EDT_HASH_INDEX(&(edtp->scsi_adr));

	hash = edt_hash[hash_index];

	edt_hash[hash_index] = edtp;
	
	edtp->hash_p = hash;

	UNLOCK(sdi_edt_mutex, sdi_edt_pl);
	return;
}


/*
 * int
 * sdi_access(struct sdi_edt *edtp, int access, struct owner *owner)
 *	change the access to the device.  usually called by sdi_doconfig()
 *	(via addrmatch() and getowner()), but could be called by
 *	target drivers that share devices.
 *
 * Calling/Exit State:
 *	If called from get_owner(), sdi_rinit_lock is held;
 *		else, no locks held on entry or exit.
 *	Acquires sdi_edt_mutex
 */
int
sdi_access(struct sdi_edt *edtp, int access, struct owner *owner)
{
	struct sdi_edt *edtp2;
	struct owner *op, **opp;

	if (!edtp || !owner ) {
		return SDI_RET_ERR;
	}

	sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk);
	edtp2 = sdi_rxedt_l(&edtp->scsi_adr);

	if ( !edtp2 ) {
		/*
		 * no such device.
		 * grant the request if reasonable.
		 */
		switch (access) {
		case SDI_ADD|SDI_CLAIM:	/* add and claim */
		case SDI_ADD:		/* add to list */
			break;
		case 0:			/* remove */
		case SDI_CLAIM:		/* remove claim */
		default:
			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			return SDI_RET_ERR;
		}

		edtp2 = sdi_alloc_edt();

		if ( !edtp2 )
		{
			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			return SDI_RET_ERR;
		}

		*edtp2 = *edtp;	/* copy the entire (struct edt) */
		edtp2->owner_list = owner;
		owner->next = (struct owner *)NULL;
		sdi_insert_edt(edtp2);
	} else {
		/*
		 * Device is already in EDT.  Validate access, perform
		 * request if possible.
		 */
		switch (access) {
		case SDI_ADD|SDI_CLAIM:	/* add and claim */
			if (edtp2->curdrv) {
				UNLOCK(sdi_edt_mutex, sdi_edt_pl);
				return SDI_RET_ERR;
			}
			/* FALLTHRU */
		case SDI_ADD:		/* add to list */
			break;
		case 0:
		case SDI_REMOVE:
		case SDI_REMOVE|SDI_DISCLAIM:

			/***
			** Find the owner block on the EDT's list.
			***/
			for ( opp = &edtp2->owner_list, op = *opp; op;
			opp = &op->next, op = *opp ) {
				if (op->maj.b_maj == owner->maj.b_maj &&
					op->maj.c_maj == owner->maj.c_maj) {
					break;
				}
			}

			if (!op) {
				/***
				** Owner block not on EDT's list!
				**/
				UNLOCK(sdi_edt_mutex, sdi_edt_pl);
				return( SDI_RET_ERR );
			}

			if (edtp2->curdrv == op) {
				edtp2->curdrv = (struct owner *)NULL;
			}

			*opp = op->next;

			if ( (access & SDI_REMOVE) ) {
				kmem_free(owner->name, strlen(owner->name)+1);
				kmem_free(owner, sizeof(struct owner));
			}

			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			return( SDI_RET_OK );

		case SDI_DISCLAIM:

			if (edtp2->curdrv == owner) {
				edtp2->curdrv = (struct owner *)NULL;
			}

			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			return( SDI_RET_OK );

		case SDI_CLAIM:

			if (edtp2->curdrv) {
				UNLOCK(sdi_edt_mutex, sdi_edt_pl);
				return( SDI_RET_ERR );
			}
			edtp2->curdrv = owner;
			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			return( SDI_RET_OK );

		default:
			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			return SDI_RET_ERR;
		}
		if (edtp2->curdrv && SDI_CLAIM & access) {
			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			return SDI_RET_ERR;
		}
		owner->next = edtp2->owner_list;
		edtp2->owner_list = owner;
	}

	owner->edtp = edtp2;
	if (access & SDI_CLAIM) {
		edtp2->curdrv = owner;
	}
	UNLOCK(sdi_edt_mutex, sdi_edt_pl);
	return SDI_RET_OK;
}

/*
 * int
 * sdi_wedt(struct sdi_edt *edtp, int devtype, char *inq)
 *      write edt entry
 *
 * Calling/Exit State:
 *      No locks held on entry or exit.
 *      Acquires sdi_edt_mutex.
 */
int
sdi_wedt(struct sdi_edt *edtp, int devtype, char *inq)
{
	struct sdi_edt *link;

	if (!edtp || !edtp->curdrv)
		return SDI_RET_ERR;
	sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk);
	link = sdi_rxedt_l(&edtp->scsi_adr);
	if (link != edtp) {
		UNLOCK(sdi_edt_mutex, sdi_edt_pl);
		return SDI_RET_ERR;
	}
	edtp->pdtype = devtype;
	strncpy(edtp->inquiry, inq, INQ_EXLEN);
	UNLOCK(sdi_edt_mutex, sdi_edt_pl);
	return SDI_RET_OK;
}

/*
 * int
 * sdi_setstamp(struct sdi_edt *edtp, struct pd_stamp *pdstamp)
 *      Set the stamp for the device in EDT to pdstamp
 *
 * Calling/Exit State:
 *      No locks held on entry or exit.
 *      Acquires sdi_edt_mutex.
 */
int
sdi_setstamp(struct sdi_edt *edtp, struct pd_stamp *stamp)
{
	if (!edtp || !edtp->curdrv)
		return SDI_RET_ERR;

	sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk);
	edtp->stamp = *stamp;
	UNLOCK(sdi_edt_mutex, sdi_edt_pl);
	return SDI_RET_OK;
}

/*
 * int
 * sdi_open(dev_t *devp, int flags, int otype, cred_t *cred_p)
 *
 * Calling/Exit State:
 *	None.
 */
int
sdi_open(dev_t *devp, int flags, int otype, cred_t *cred_p)
{
	dev_t dev = *devp;

	if (drv_priv(cred_p))
		return(EPERM);

	/*
	 *  Allow sdi_ioctl-only pass_thru using second major number
	 */
	if (getemajor(dev) == sdi_pass_thru_major)
		return 0;

	if ( !HBA_tbl[SC_EXHAN(dev)].info )
		return ENXIO;

	return (*HIP(HBA_tbl[SC_EXHAN(dev)].info)->hba_open)
				(devp, flags, otype, cred_p);
}

/*
 * int
 * sdi_close(dev_t dev, int flags, int otype, cred_t *cred_p)
 *
 * Calling/Exit State:
 *	None.
 */
int
sdi_close(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	/*
	 *  close sdi_ioctl-only pass_thru using second major number.
	 *  Notice that close always works on this dev.
	 */
	if (getemajor(dev) == sdi_pass_thru_major)
		return 0;

	/*
	 *  pass close request through to HBA driver
	 */
	return (*HIP(HBA_tbl[SC_EXHAN(dev)].info)->hba_close)(dev, flags, otype, cred_p);
}



/*
 * int
 * sdi_ioctl(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p,
 * 	int *rval_p)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires sdi_rinit_lock and sdi_edt_mutex
 */
int
sdi_ioctl(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p,
	int *rval_p)
{
	int 	hba_no=0;
	int	tc=0;
	int	lun=0;
	int 	edtsz=0;
	int	hba_active=0;
	struct 	scsi_edt *sc_edt;
	struct 	scsi_xedt *sc_xedt;
	struct 	scsi_edt *sep;
	struct 	scsi_xedt *sxep;
	struct 	sdi_edt *edtp;
	char	*ep, *sp;
	int	ret;
	HBA_IDATA_STRUCT *idatap;
	int	bus, nbus, ntargets, nluns;
	int 	length, value, func;
	struct	scsi_adr scsi_adr;
	int	size_of_copy;
	rm_key_t	rmkey;
	ushort_t values[SDI_TIMEOUT_VALUES];
	sdi_device_t	*devicep;

	switch (cmd) {
	case B_HA_CNT:
		SLEEP_LOCK(sdi_rinit_lock, pridisk);
		for (hba_no = 0, hba_active=0; hba_no < sdi_hacnt; hba_no++) {
			if (HBA_tbl[hba_no].active)
				hba_active++;
		}
                if (copyout((caddr_t)&hba_active, arg, sizeof(hba_active))) {
			SLEEP_UNLOCK(sdi_rinit_lock);
                        return(EFAULT);
		}
		SLEEP_UNLOCK(sdi_rinit_lock);
                break;

	case B_REDT:
		edtsz = sdi_hacnt * MAX_TCS * sizeof(struct scsi_edt);
		sc_edt=(struct scsi_edt *)kmem_zalloc(edtsz,KM_SLEEP);
		sep = sc_edt;
		SLEEP_LOCK(sdi_rinit_lock, pridisk);

		for (hba_no = 0, hba_active=0; hba_no < sdi_hacnt; hba_no++) {

		  scsi_adr.scsi_ctl = hba_no;
		  if (!HBA_tbl[hba_no].active)
			continue;
		  else
			hba_active++;

		  for (tc = 0; tc < MAX_TCS; tc++, sep++) {
		    scsi_adr.scsi_target = tc;
		    for (lun = 0; lun < MAX_LUS; lun++) {
		      scsi_adr.scsi_lun = lun;
		      scsi_adr.scsi_bus = 0;
		      sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk);

		      if (edtp = sdi_rxedt_l(&scsi_adr)) {

			sep->tc_equip = 1;
			sep->ha_slot = (unsigned char) hba_active;
		        sep->n_lus++;
		        sep->lu_id[lun] = 1;
			sep->pdtype = edtp->pdtype;
			strncpy((char *)sep->tc_inquiry,
				edtp->inquiry, INQ_LEN-1);
			sep->tc_inquiry[INQ_LEN] = '\0';
		        if (edtp->curdrv) {
                                sep->c_maj = edtp->curdrv->maj.c_maj;
                                sep->b_maj = edtp->curdrv->maj.b_maj;
                                ep = edtp->curdrv->name;
                                sp = sep->drv_name;
                                while (*ep != ' ' && *ep != '\t' && *ep != '\0')      
					*sp++ = *ep++;
				*sp = '\0';
		        } else {
                          strcpy(sep->drv_name, "VOID");
                          sep->drv_name[4] = '\0';
		        }
		      }
		      UNLOCK(sdi_edt_mutex, sdi_edt_pl);
		    }
		  }
		}
		SLEEP_UNLOCK(sdi_rinit_lock);

                if (copyout((caddr_t)sc_edt, arg, edtsz))
                        return(EFAULT);

		kmem_free(sc_edt, edtsz);
		sc_edt = NULL;

                break;
	case B_RXEDT_VOL:
	case B_RXEDT:
		edtsz = MAX_EXTCS * sizeof(struct scsi_xedt);
		sc_xedt=(struct scsi_xedt *)kmem_alloc(edtsz,KM_SLEEP);
		SLEEP_LOCK(sdi_rinit_lock, pridisk);

		for (hba_no = 0, hba_active=0; hba_no < sdi_hacnt; hba_no++) {

			scsi_adr.scsi_ctl = hba_no;
			if (!HBA_tbl[hba_no].active)
				continue;
			else
				hba_active++;

			idatap = IDP(HBA_tbl[hba_no].idata);
			if ((idatap->version_num & HBA_VMASK) >= PDI_UNIXWARE20) {
				nbus = idatap->idata_nbus;
				ntargets = idatap->idata_ntargets;
				nluns = idatap->idata_nluns;
				rmkey = idatap->idata_rmkey;
			} else {
				nbus = 1;
				ntargets = MAX_TCS;
				nluns = MAX_LUS;
				rmkey = RM_NULL_KEY;
			}
			for (bus = 0; bus < nbus; bus++) {
				scsi_adr.scsi_bus = bus;

				sxep = sc_xedt;
				bzero(sxep, edtsz);

				for (tc = 0; tc < ntargets; tc++) {
					scsi_adr.scsi_target = tc;
					for (lun = 0; lun < nluns; lun++) {
						sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk);
						scsi_adr.scsi_lun = lun;

						if ((edtp = sdi_rxedt_l(&scsi_adr)) &&
						!(edtp->se_devicep && (edtp->se_devicep->sdv_flags & SDI_SDV_CLAIMED))) {
							sxep->xedt_ctl = (ushort_t)idatap->cntlr;
							if ((idatap->version_num & HBA_VMASK) >= HBA_UW21_IDATA)
								sxep->xedt_ha_id = idatap->ha_chan_id[bus];
							else
								sxep->xedt_ha_id = idatap->ha_id;

							sxep->xedt_bus = (uchar_t)bus;
							sxep->xedt_target = (ushort_t) tc;
							sxep->xedt_lun = (ushort_t) lun;
							sxep->xedt_pdtype = edtp->pdtype;
							sxep->xedt_memaddr = edtp->memaddr;
							sxep->xedt_ctlorder = edtp->ctlorder;
							sxep->xedt_rmkey = rmkey;
							strncpy((char *)sxep->xedt_tcinquiry, edtp->inquiry, INQ_EXLEN);
							sxep->xedt_tcinquiry[INQ_EXLEN-1] = '\0';
							sxep->xedt_stamp = edtp->stamp;

							if (edtp->curdrv) {
								sxep->xedt_cmaj = edtp->curdrv->maj.c_maj;
								sxep->xedt_bmaj = edtp->curdrv->maj.b_maj;
								sxep->xedt_minors_per = edtp->curdrv->maj.minors_per;
								sxep->xedt_first_minor = edtp->curdrv->maj.first_minor;
								ep = edtp->curdrv->name;
								sp = sxep->xedt_drvname;
								while (*ep != ' ' && *ep != '\t' && *ep != '\0')      
									*sp++ = *ep++;

								*sp = '\0';
							} else {
								strcpy(sxep->xedt_drvname, "VOID");
								sxep->xedt_drvname[4] = '\0';
							}
							sxep++;
						} else if ((devicep = sdi_addr_lookup(&scsi_adr))) {
							sxep->xedt_ctl = (ushort_t)idatap->cntlr;
							if ((idatap->version_num & HBA_VMASK) >= HBA_UW21_IDATA)
								sxep->xedt_ha_id = idatap->ha_chan_id[bus];
							else
								sxep->xedt_ha_id = idatap->ha_id;

							sxep->xedt_bus = (uchar_t)bus;
							sxep->xedt_target = (ushort_t) tc;
							sxep->xedt_lun = (ushort_t) lun;
							sxep->xedt_pdtype = devicep->sdv_devtype;
							if ((idatap->version_num & HBA_VMASK) >= PDI_UNIXWARE20) {
								sxep->xedt_memaddr = idatap->idata_memaddr;
								sxep->xedt_ctlorder = idatap->idata_ctlorder;
							} else {
								sxep->xedt_memaddr = 0;
								sxep->xedt_ctlorder = 0;
							}
							sxep->xedt_rmkey = rmkey;
							strncpy((char *)sxep->xedt_tcinquiry,
								devicep->sdv_inquiry, INQ_EXLEN);
							sxep->xedt_tcinquiry[INQ_EXLEN-1] = '\0';
							sxep->xedt_stamp = edtp->stamp;

							sxep->xedt_cmaj = 0;
							sxep->xedt_bmaj = 0;
							sxep->xedt_minors_per = devicep->sdv_driver->sdd_minors_per;
							(void)find_basedev_on_rmkey(devicep->sdv_parent_handle,
								&(sxep->xedt_first_minor));

							if ( cmd == B_RXEDT_VOL ) {
								sxep->xedt_cmaj = getmajor(sxep->xedt_first_minor);
								sxep->xedt_bmaj = getmajor(sxep->xedt_first_minor);
								sxep->xedt_first_minor = getminor(sxep->xedt_first_minor);
							}

							ep = devicep->sdv_driver->sdd_modname;
							sp = sxep->xedt_drvname;
							while (*ep != ' ' && *ep != '\t' && *ep != '\0')      
								*sp++ = *ep++;

							*sp = '\0';

							sxep++;
						}
						UNLOCK(sdi_edt_mutex, sdi_edt_pl);
					}
				}
				size_of_copy = (char *)sxep-(char *)sc_xedt;
				if(!size_of_copy)
					continue;
				if(copyout((caddr_t)sc_xedt, arg, size_of_copy)) {
					SLEEP_UNLOCK(sdi_rinit_lock);
					kmem_free(sc_xedt, edtsz);
					sc_xedt = NULL;
					return(EFAULT);
				}
				arg += size_of_copy;
			}
		}
		SLEEP_UNLOCK(sdi_rinit_lock);

		kmem_free(sc_xedt, edtsz);
		sc_xedt = NULL;

		break;
	case B_EDT_CNT:
		SLEEP_LOCK(sdi_rinit_lock, pridisk);
		edtsz = 0;
		for (hba_no = 0, hba_active=0; hba_no < sdi_hacnt; hba_no++) {

		  scsi_adr.scsi_ctl = hba_no;
		  if (!HBA_tbl[hba_no].active)
			continue;
		  else
			hba_active++;

		  idatap = IDP(HBA_tbl[hba_no].idata);
		  if ((idatap->version_num & HBA_VMASK) >= PDI_UNIXWARE20) {
			nbus = idatap->idata_nbus;
			ntargets = idatap->idata_ntargets;
			nluns = idatap->idata_nluns;
		  } else {
			nbus = 1;
			ntargets = MAX_TCS;
			nluns = MAX_LUS;
		  }
		  for (bus = 0; bus < nbus; bus++) {
		    scsi_adr.scsi_bus = bus;
		    for (tc = 0; tc < ntargets; tc++) {
		      scsi_adr.scsi_target = tc;
		      for (lun = 0; lun < nluns; lun++) {
			scsi_adr.scsi_lun = lun;
			sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk);
		        if ((edtp = sdi_rxedt_l(&scsi_adr)) &&
			   !(edtp->se_devicep &&
			     (edtp->se_devicep->sdv_flags & SDI_SDV_CLAIMED))) {
				edtsz++;
		        } else if ((devicep = sdi_addr_lookup(&scsi_adr))) {
				edtsz++;
		        }
		        UNLOCK(sdi_edt_mutex, sdi_edt_pl);
		      }
		    }
		  }
		}
		SLEEP_UNLOCK(sdi_rinit_lock);

                if (copyout((caddr_t)(&edtsz), arg, sizeof(edtsz)))
                        return(EFAULT);
                break;

	case B_GET_MAP:
		SLEEP_LOCK(sdi_rinit_lock, pridisk);
                if (copyout((caddr_t)&HBA_map[0], arg,
			    sdi_hacnt * sizeof(HBA_map[0]))) {
			SLEEP_UNLOCK(sdi_rinit_lock);
                        return(EFAULT);
		}
		SLEEP_UNLOCK(sdi_rinit_lock);
                break;

	case B_RESERVED:  {
		struct putmapargs {
			int	a_cnt;
			caddr_t	a_hbamap;
		} putmapargs;

		SLEEP_LOCK(sdi_rinit_lock, pridisk);
                if (copyin(arg, &putmapargs, sizeof(struct putmapargs))) {
			SLEEP_UNLOCK(sdi_rinit_lock);
                        return(EFAULT);
		}
		if (putmapargs.a_cnt > SDI_MAX_HBAS) {
			SLEEP_UNLOCK(sdi_rinit_lock);
                        return(EINVAL);
		}
                if (copyin(putmapargs.a_hbamap, (caddr_t)&HBA_map[0],
			    putmapargs.a_cnt * sizeof(HBA_map[0]))) {
			SLEEP_UNLOCK(sdi_rinit_lock);
                        return(EFAULT);
		}
		SLEEP_UNLOCK(sdi_rinit_lock);
                break;
		}

	case B_MAP_CNT:
		SLEEP_LOCK(sdi_rinit_lock, pridisk);
                if (copyout((caddr_t)&sdi_hacnt, arg, sizeof(sdi_hacnt))) {
			SLEEP_UNLOCK(sdi_rinit_lock);
                        return(EFAULT);
		}
		SLEEP_UNLOCK(sdi_rinit_lock);
                break;
	case B_ADD_DEV:
	{
		struct scsi_adr sa;

		if (copyin(arg, (caddr_t)&sa, sizeof(struct scsi_adr))) {
			return EFAULT;
		}
		if (sdi_hot_add(&sa) > 0) {
			sdi_enable_all();
			return 0;
		}
		else
			return EINVAL;
	}
		/*NOTREACHED*/
		break;
	case B_RM_DEV:
	{
		struct scsi_adr sa;

		if (copyin(arg, (caddr_t)&sa, sizeof(struct scsi_adr))) {
			return EFAULT;
		}
		if (sdi_hot_rm(&sa) == SDI_RET_OK)
			return 0;
		else
			return EINVAL;
	}
		/*NOTREACHED*/
		break;
	case B_PAUSE:
	{
		struct scsi_adr sa;

		if (copyin(arg, (caddr_t)&sa, sizeof(struct scsi_adr))) {
			return EFAULT;
		}
		sa.scsi_lun = 0;
		sa.scsi_target = 0;
		if (sdi_dofunc(&sa, SFB_PAUSE, KM_SLEEP) == SDI_ASW)
			return 0;
		else
			return EINVAL;
	}
		/*NOTREACHED*/
		break;
	case B_CONTINUE:
	{
		struct scsi_adr sa;

		if (copyin(arg, (caddr_t)&sa, sizeof(struct scsi_adr))) {
			return EFAULT;
		}
		sa.scsi_lun = 0;
		sa.scsi_target = 0;
		if (sdi_dofunc(&sa, SFB_CONTINUE, KM_SLEEP) == SDI_ASW)
			return 0;
		else
			return EINVAL;
	}
		/*NOTREACHED*/
		break;

	case B_NEW_TIMEOUT_VALUES:

		if (copyin(arg, &scsi_adr, sizeof(struct scsi_adr)))
			return EFAULT;

		sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk);
		if ((edtp = sdi_rxedt_l(&scsi_adr)) == NULL) {
			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			return EINVAL;
		}

		arg += sizeof(struct scsi_adr);

		if (copyin(arg, &length, sizeof(int))) {
			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			return EFAULT;
		}

		arg += sizeof(int);

		if (copyin(arg, values, length*sizeof(unsigned short))) {
			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			return EFAULT;
                }

		/* We must be careful copying the new values on top of
		 * existing values.  If we use bcopy(), only one byte of
		 * the value may have been written when a job reads a
		 * timeout value.
		 *
		 * The assumption is that space is allocated in the edt
		 * for timeout values.  The space is allocated when a lun
		 * is determined to be equipped.
		 */

		if (!edtp->timeout) {
			UNLOCK(sdi_edt_mutex, sdi_edt_pl);
			cmn_err(CE_WARN, "timeout array not allocated");
			break;
		}
		while (length--)
			edtp->timeout[length] = values[length];
		UNLOCK(sdi_edt_mutex, sdi_edt_pl);
		break;

	case B_TIMEOUT_SUPPORT:

		if (copyin(arg, &value, sizeof(int)))
			return EFAULT;

		if (value != 0 && value != 1) {
			copyout(&pdi_timeout, arg, sizeof(int));
			break;
		}

		pdi_timeout = value;

		func = pdi_timeout ? SFB_TIMEOUT_ON : SFB_TIMEOUT_OFF;
		for (hba_no = 0, hba_active=0; hba_no < sdi_hacnt; hba_no++) {
			struct hba_cfg *hbap = &HBA_tbl[hba_no];
			static struct scsi_adr sa;
			if (!hbap->active ||
			    !hbap->info->hba_flag ||
			    !(*hbap->info->hba_flag & HBA_TIMEOUT_RESET))
					continue;

			sa.scsi_ctl = hba_no;
			sdi_dofunc(&sa, func, KM_SLEEP);
		}
		break;

	case B_CAPABILITIES:
		{
		int hba_num;
		int flag = 0;

		if (copyin(arg, &hba_num, sizeof(int)))
			return EFAULT;
		if (hba_num >= 0 && hba_num < sdi_hacnt)
			if (HBA_tbl[hba_num].info->hba_flag)
				flag = *(HBA_tbl[hba_num].info->hba_flag);
		if (copyout(&flag, arg, sizeof(int)))
			return EFAULT;
		}
		break;

	default:
		/*
		 *  fail request if sdi_ioctl-only pass_thru major is being used
		 */
		if (getemajor(dev) == sdi_pass_thru_major)
			return EINVAL;

		hba_no = SC_EXHAN(dev);
		if ((IDP(HBA_tbl[hba_no].idata)->version_num & HBA_VMASK) ==
							HBA_SVR4_2) {
			/* enable special physio handling */
			u.u_physio_start = sdi_physio_start;
			ret = (*HIP(HBA_tbl[hba_no].info)->hba_ioctl)
					(dev, cmd, arg, mode, cred_p, rval_p);
			/* disable special physio handling */
			u.u_physio_start = NULL;
		} else
			ret = (*HIP(HBA_tbl[hba_no].info)->hba_ioctl)
					(dev, cmd, arg, mode, cred_p, rval_p);
		return (ret);
	}
	return(0);
}


/*
 * int
 * sdi_config(char *drv_name, int  c_maj, int  b_maj, struct tc_data *tc_data,
 * 				int  tc_size, struct tc_edt *tc_edtp)
 *	4.0 COMPAT ONLY -- use sdi_doconfig()
 *	The target drivers pass to this function a pointer to their
 *	tc_data structure which contains the inquiry strings of the
 *	devices they support. This routine walks through the EDT data
 *	searching for the inquiry strings that match. It returns the
 *	number of TCs found, and for each TC a tc_edt structure is
 *	populated.
 *  Note: we don't know how many tcedts we can fill up, so if we overflow it
 *        we're hosed
 *
 * Calling/Exit State:
 *	None. 	(only called from init() by 4.0 drivers)
 */
/*ARGSUSED*/
int
_Compat_sdi_config(
char		*drv_name,	/* driver ASCII name	*/
int		 c_maj,		/* character major num 	*/
int		 b_maj,		/* block major num   	*/
struct tc_data	*tc_data,	/* TC inquiry data	*/
int		 tc_size,	/* TC data size 	*/
struct tc_edt	*tc_edtp)	/* pointer to TC edt	*/
{

	register int	 i;
	struct tc_data	*tc_p;
	register int lun, scsi_id;
	register short 	 hba_no;
	int		 tc_count = 0;
	char		 found;
	struct sdi_edt *hash;
	struct owner *ownerp;
	HBA_IDATA_STRUCT *idatap;
	int bus, nbus, ntargets, nluns;

#ifdef SDI_DEBUG
	if(sdi_debug > 2)
	{
		cmn_err(CE_CONT,"sdi_config(%s, %x, %x, %x, %x ,%x)\n",
			drv_name, c_maj, b_maj, tc_data, tc_size, tc_edtp);
		cmn_err(CE_CONT,"sdi_hacnt = %x\n", sdi_hacnt);
	}
#endif
	for (hba_no = 0; hba_no < sdi_hacnt; hba_no++) {
	    if (!(idatap = IDP(HBA_tbl[hba_no].idata)))
		continue;
	    if ((idatap->version_num & HBA_VMASK) >= PDI_UNIXWARE20) {
		nbus = idatap->idata_nbus;
		ntargets = idatap->idata_ntargets;
		nluns = idatap->idata_nluns;
	    } else {
		nbus = 1;
		ntargets = MAX_TCS;
		nluns = MAX_LUS;
	    }
	    for (bus = 0; bus < nbus; bus++) {
		for (scsi_id = 0; scsi_id < ntargets; scsi_id++) {
		    if((idatap->version_num & HBA_VMASK) == 
			HBA_UW21_IDATA) {
			if (scsi_id == idatap->ha_chan_id[bus])
				continue;
		    } else {
			if (scsi_id == idatap->ha_id)
				continue;
		    }

		    for(lun = 0; lun < nluns; lun++) {

			hash = sdi_rxedt(hba_no, bus, scsi_id, lun);
			if(hash->scsi_adr.scsi_ctl != hba_no || 
			   hash->scsi_adr.scsi_target != scsi_id ||
			   hash->scsi_adr.scsi_lun != lun ||
			   hash->scsi_adr.scsi_bus != bus) {
#ifdef SDI_DEBUG
				if(sdi_debug > 3)
					cmn_err(CE_CONT,
						"sdi_config: no match\n" );
#endif	
				continue; /* for */
			}

			found = B_FALSE;
			for (i = 0, tc_p = tc_data; i < tc_size; i++, tc_p++)
			{
#ifdef SDI_DEBUG
				if(sdi_debug > 3)
					cmn_err(CE_CONT,
						"sdi_config: compare %s %s\n", 
						hash->inquiry,
						tc_p->tc_inquiry);
#endif	
				if (strncmp(hash->inquiry, 
					(char *)tc_p->tc_inquiry, INQ_LEN) == 0)
				{
					/* if someone else doesn't own it */
					if(!hash->curdrv){
						found = B_TRUE;
						break;
					}
				}
			}

			if (found)
			{
				/*
				 * Get an owner structure.
				 */
				ownerp = kmem_zalloc(sizeof(struct owner),
							sdi_sleepflag);
				if ( !ownerp ) {
					/*
					 *+ Failed to allocate data structure
					 *+ at init time, when there should
					 *+ be plenty of memory.
					 */
					cmn_err(CE_WARN,
						"sdi_config: failed to allocate owner structure.\n");
				}

				hash->curdrv = ownerp;
				hash->curdrv->name = drv_name;
				
				ownerp->edtp = hash;
				
				tc_edtp->ha_slot = hash->scsi_adr.scsi_ctl;
				tc_edtp->tc_id = hash->scsi_adr.scsi_target;
				tc_edtp->n_lus = 1;
			        tc_edtp->lu_id[lun] = 1;

#ifdef SDI_DEBUG
				if(sdi_debug > 1){
					cmn_err(CE_CONT,
					"sdi_config: ha_slot %x tc_id %x : %s", 
					tc_edtp->ha_slot, tc_edtp->tc_id,
					tc_p->tc_inquiry);
				}
#endif	
				tc_count++;
				tc_edtp++;
			}
		    }
		}
	    }
	}

#ifdef SDI_DEBUG
	if(sdi_debug > 1){
		cmn_err(CE_CONT,"sdi_config: returning %x\n", tc_count);
	}
#endif	
	return (tc_count);
}

#define	SDI_INITBLK(sbp)	{	\
				(sbp)->sb.SCB.sc_comp_code = SDI_UNUSED; \
				(sbp)->sb.SCB.sc_extra = &(sbp)->extra; \
				}

/*
 * used by bio to size buffers allocated through getrbuf
 */
int sdi_blksize = 2 * LG_POOLSIZE;

/*
 * struct sb *
 * sdi_getblk(int flag)
 *	return a scsi command block
 *
 * Calling/Exit State:
 *	None.
 */
struct sb *
sdi_getblk(int flag)
{
	register struct xsb *sbp;

	if ( (sbp = (struct xsb *)SDI_GET(&lg_poolhead, flag)) )
	{
		SDI_INITBLK(sbp);
	}
	return (struct sb *)sbp;
}

/*
 * void
 * sdi_initblk(struct sb *sbp)
 *	Initialize a scsi command block
 *
 * Calling/Exit State:
 *	On entry, sbp points to space for a SCSI command block
 *		which has been initialized to 0.
 *	On exit, the fields of the command block have been initialized.
 */
void
sdi_initblk(struct xsb *sbp)
{

	SDI_INITBLK(sbp);
}

/*
 * struct sb *
 * sdi_xgetblk(int hbaflag, int flag)
 *	return a scsi command block
 *
 * Calling/Exit State:
 *	None.
 */
struct sb *
sdi_xgetblk(int hbaflag, int flag)
{
	register struct xsb *sbp;

	if ( (sbp = (struct xsb *)SDI_GET(&lg_poolhead, flag)) )
	{
		sbp->sb.SCB.sc_dev.pdi_adr_version = hbaflag;
		sbp->sb.SCB.sc_dev.extended_address =
			(struct sdi_extended_adr *)
				kmem_zalloc(
					 sizeof(struct sdi_extended_adr), 
						flag);
		if ( ! sbp->sb.SCB.sc_dev.extended_address ) {
			cmn_err(CE_WARN,
				"sdi_xgetblk: insufficient memory"
                                 " to allocate extended address");
			sdi_freeblk((struct sb *)sbp);
			return(struct sb *)NULL;
		}
		sbp->sb.SCB.sc_comp_code = SDI_UNUSED;
		sbp->sb.SCB.sc_link = NULL;
		sbp->hbadata_p = NULL;
		sbp->owner_p = NULL;
		sbp->extra.sb_sense.sd_key = SD_NOSENSE;
		sbp->sb.SCB.sc_extra = &sbp->extra;
	}
	return (struct sb *)sbp;
}

/*
 * long
 * sdi_freeblk(struct sb *sbp)
 *	free scsi command block 
 *
 * Calling/Exit State:
 *	None.
 * Returns: SDI_RET_OK or SDI_RET_ERR
 */
long
sdi_freeblk(struct sb *sbp)
{

	if (sdi_hba_freeblk(sbp) != SDI_RET_OK)
		return (SDI_RET_ERR);
	SDI_FREE(&lg_poolhead, (jpool_t *)sbp);
	return (SDI_RET_OK);
}

/*
 * void *
 * sdi_getpair(int flag)
 *      Allocate a pair of adjacent scsi command blocks.  Initialize the
 *      second one.
 *
 * Calling/Exit State:
 *      None.
 */
struct sb *
sdi_getpair(int flag)
{
        register struct xsb *sbp;

        sbp = kmem_zalloc(2 * lg_poolhead.f_isize, flag);
        if (sbp != NULL) {
                SDI_INITBLK(sbp);
                SDI_INITBLK(sbp + 1);
        }
        return (struct sb *)sbp;
}

/*
 * long
 * sdi_freepair(struct sb *sbp)
 *      Free a pair of scsi command blocks and any hba data associated
 *      with the second one.
 *
 * Calling/Exit State:
 *      None.
 * Returns: SDI_RET_OK or SDI_RET_ERR
 */
long
sdi_freepair(struct sb *sbp)
{
	struct sb *jp;

	jp = (struct sb *)(((char *)sbp) + lg_poolhead.f_isize);

        if (sdi_hba_xfreeblk(jp->SCB.sc_dev.pdi_adr_version,
			(struct sb *)(((char *)sbp) + lg_poolhead.f_isize))
                        	!= SDI_RET_OK)
                return (SDI_RET_ERR);
        kmem_free(sbp, 2 * lg_poolhead.f_isize);
        return (SDI_RET_OK);
}

/*
 * long
 * sdi_xfreeblk(int hbaflag, struct sb *sbp)
 *	free scsi command block 
 *
 * Calling/Exit State:
 *	None.
 * Returns: SDI_RET_OK or SDI_RET_ERR
 */
long
sdi_xfreeblk(int hbaflag, struct sb *sbp)
{
	if (sdi_hba_xfreeblk(hbaflag, sbp) != SDI_RET_OK)
		return (SDI_RET_ERR);
	SDI_FREE(&lg_poolhead, (jpool_t *)sbp);
	return (SDI_RET_OK);
}

/*
 * void
 * sdi_getdev(struct scsi_ad *addr, dev_t *dev)
 *	get major & minor of sdi driver; used for pass-thru
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_getdev(struct scsi_ad *addr, dev_t *dev)
{
	struct scsi_adr scsi_adr;
#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		cmn_err(CE_CONT,"sdi_getdev(%x, %x)\n", addr, dev);
#endif

	if ( ! addr->pdi_adr_version  ) {
		scsi_adr.scsi_ctl = SDI_EXHAN(addr);
		scsi_adr.scsi_target = SDI_EXTCN(addr);
		scsi_adr.scsi_lun = SDI_EXLUN(addr); 
		scsi_adr.scsi_bus = SDI_BUS(addr);
	}
	else {
		scsi_adr.scsi_ctl = SDI_HAN_32(addr);
		scsi_adr.scsi_target = SDI_TCN_32(addr);
		scsi_adr.scsi_lun = SDI_LUN_32(addr);
		scsi_adr.scsi_bus = SDI_BUS_32(addr);
	}

	if (SDI_EXILLEGAL(scsi_adr.scsi_ctl, scsi_adr.scsi_target,
			  scsi_adr.scsi_lun, scsi_adr.scsi_bus)) {
		return;
	}
	*dev = makedevice(sdi_major, SDI_MINOR(scsi_adr.scsi_ctl,
		scsi_adr.scsi_target, scsi_adr.scsi_lun, scsi_adr.scsi_bus));
}

/*
 * void
 * sdi_name(struct scsi_ad *addr, char *name)
 *	get the name of the HBA device
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_name(struct scsi_ad *addr, char *name)
{
	sdi_xname(0, addr, name);
	return;
}

/*
 * void
 * sdi_xname(int hbaflag, struct scsi_ad *addr, char *name)
 *	get the name of the HBA device
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_xname(int hbaflag, struct scsi_ad *addr, char *name)
{
	int i;
	struct hbagetinfo hbagetinfo;
#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		cmn_err(CE_CONT,"sdi_xname(%x, %s)\n", addr, name);
#endif
	
	if ( ! hbaflag )
		i = SDI_CONTROL(addr->sa_ct);
	else
		i = SDI_HAN_32(addr);
	hbagetinfo.name = name;
	hbagetinfo.bcbp = NULL;
#ifdef DEBUG
	if (!HIP(HBA_tbl[i].info)->hba_getinfo)
		cmn_err(CE_WARN, "SDI: HBA driver has no name()\n");
#endif
	if (HIP(HBA_tbl[i].info)->hba_getinfo)
		(*HIP(HBA_tbl[i].info)->hba_getinfo)( addr, &hbagetinfo);
}

/*
 * bcb_t *
 * sdi_getbcb(struct scsi_ad *addr, int sleepflag)
 *	get the breakup control block for the HBA device
 *
 * Return: 0 if allocation fails, otherwise, a pointer to a bcb.
 *
 * Calling/Exit State:
 *	None.
 */
bcb_t *
sdi_getbcb(struct scsi_ad *addr, int sleepflag)
{
	return(sdi_xgetbcb(0, addr, sleepflag));
}

/*
 * bcb_t *
 * sdi_xgetbcb(int hbaflag, struct scsi_ad *addr, int sleepflag)
 *	get the breakup control block for the HBA device
 *
 * Return: 0 if allocation fails, otherwise, a pointer to a bcb.
 *
 * Calling/Exit State:
 *	None.
 */
bcb_t *
sdi_xgetbcb(int hbaflag, struct scsi_ad *addr, int sleepflag)
{
	int i;
	struct hbagetinfo hbagetinfo = {0, 0, 0};
	char name[SDI_NAMESZ];
	physreq_t *preqp;

#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		cmn_err(CE_CONT,"sdi_xgetbcb(%x)\n", addr);
#endif
	
	if ( ! hbaflag ) {
		i = SDI_CONTROL(addr->sa_ct);
		addr->pdi_adr_version = 0;
	}
	else {
		addr->pdi_adr_version = hbaflag;
		i = SDI_HAN_32(addr);
	}
	hbagetinfo.name = name;

	if ((hbagetinfo.bcbp = bcb_alloc(sleepflag)) == NULL)
		return NULL;
	if ((preqp = physreq_alloc(sleepflag)) == NULL) {
		bcb_free(hbagetinfo.bcbp);
		return NULL;
	}

	hbagetinfo.bcbp->bcb_physreqp = preqp;

#ifdef DEBUG
	if (!HIP(HBA_tbl[i].info)->hba_getinfo)
		cmn_err(CE_WARN, "SDI: HBA driver has no bcb()\n");
#endif
	if (HIP(HBA_tbl[i].info)->hba_getinfo)
		(*HIP(HBA_tbl[i].info)->hba_getinfo)(addr, &hbagetinfo);

	if ((IDP(HBA_tbl[i].idata)->version_num & HBA_VMASK) == HBA_SVR4_2) {
		/*
		 * default zero max_xfer HBAs to DEFAULT_MAX_XFER
		 */
		if (HIP(HBA_tbl[i].info)->max_xfer == 0)
			HIP(HBA_tbl[i].info)->max_xfer = DEFAULT_MAX_XFER;

		/*
		 * Decrement the max_xfer by one page to count for non 
		 * page-aligned requests
		 */
		hbagetinfo.bcbp->bcb_max_xfer = HIP(HBA_tbl[i].info)->max_xfer;
		if (hbagetinfo.bcbp->bcb_max_xfer > ptob(1))  
			hbagetinfo.bcbp->bcb_max_xfer -= ptob(1);

		hbagetinfo.bcbp->bcb_addrtypes = BA_KVIRT;
		if (!(hbagetinfo.iotype & F_PIO)) {
			preqp->phys_align = 512;
			preqp->phys_boundary = 0;
			if (!(hbagetinfo.iotype & F_SCGTH)) {
				preqp->phys_align = 2;
				preqp->phys_boundary = ptob(1);
			}
			preqp->phys_dmasize = 24;
			if (hbagetinfo.iotype & F_DMA_32)
				preqp->phys_dmasize = 32;
		}
	}

	if (!physreq_prep(preqp, sleepflag)) {
		physreq_free(preqp);
		bcb_free(hbagetinfo.bcbp);
		return NULL;
	}

	return hbagetinfo.bcbp;
}
/*
 * void
 * sdi_freebcb(bcb_t *bcbp)
 *	free the breakup control block for the HBA device
 *
 * Return: None.
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_freebcb(bcb_t *bcbp)
{

#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		cmn_err(CE_CONT,"sdi_freebcb(%x)\n", bcbp);
#endif
	
	if (bcbp) {
		if (bcbp->bcb_physreqp)
			physreq_free(bcbp->bcb_physreqp);
		bcb_free(bcbp);
	}

}

/*
 * void
 * sdi_iotype(struct scsi_ad *addr, 
		uchar_t *iotype, unsigned int *ex_iotype,
 *	 struct ident *inq_dp)
 *	get HBA iotype
 *	
 * Calling/Exit State:
 *	None.
 */
void
sdi_iotype(struct scsi_ad *addr, uchar_t *iotype, unsigned int *ex_iotype, 
		struct ident *inq_dp)
{
	int i;
	struct hbagetinfo hbagetinfo;
	char name[SDI_NAMESZ];
#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		cmn_err(CE_CONT,"sdi_iotype(%x, %d)\n", addr, iotype);
#endif
	
	if ( ! addr->pdi_adr_version ) 
		i = SDI_CONTROL(addr->sa_ct);
	else
		i = SDI_HAN_32(addr);
	hbagetinfo.name = name;
	hbagetinfo.bcbp = NULL;
#ifdef DEBUG
	if (!HIP(HBA_tbl[i].info)->hba_getinfo)
		cmn_err(CE_WARN, "SDI: HBA driver has no iotype()\n");
#endif
	if (HIP(HBA_tbl[i].info)->hba_getinfo)
		(*HIP(HBA_tbl[i].info)->hba_getinfo)( addr, &hbagetinfo);

	*iotype = hbagetinfo.iotype;
	if (hbagetinfo.iotype & F_EXIOTYPE)
		*ex_iotype = hbagetinfo.ex_iotype;
	else
		*ex_iotype = 0;

	if(inq_dp->id_rmb)	{
		*iotype |= F_RMB;
	}
}

/*
 * int
 * sdi_translate(struct sb *sb, int bflags, proc_t *procp)
 *	Perform virtual to physical translation of the SCB
 *	data pointer.
 *
 * Calling/Exit State:
 *	None.
 */
int
sdi_translate(struct sb *sb, int bflags, proc_t *procp, int sleepflag)
{

#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		cmn_err(CE_CONT,"sdi_translate(%x, %x, %x)\n", sb, bflags, procp);
#endif
	return(sdi_xtranslate(0, sb, bflags, procp, sleepflag));
}

/*
 * int
 * sdi_xtranslate(int hbaflag, struct sb *sb, int bflags, proc_t *procp)
 *      Perform virtual to physical translation of the SCB
 *      data pointer.
 *
 * Calling/Exit State:
 *      None.
 */
int
sdi_xtranslate(int hbaflag, struct sb *sb, int bflags, 
		proc_t *procp, int sleepflag)
{
        register struct xsb *xsb = (struct xsb *)sb;
        int c, ret;

#ifdef SDI_DEBUG
        if(sdi_debug >= 1)
		cmn_err(CE_CONT,
			"sdi_xtranslate(%x, %x, %x)\n", sb, bflags, procp);
#endif
	if ( ! hbaflag )
		if (sb->sb_type == SFB_TYPE) {
                	c = SDI_CONTROL(sb->SFB.sf_dev.sa_ct);
			sb->SFB.sf_dev.pdi_adr_version = 0;
		}
        	else {
                	c = SDI_CONTROL(sb->SCB.sc_dev.sa_ct);
			sb->SCB.sc_dev.pdi_adr_version = 0;
		}
	else {
        	if (sb->sb_type == SFB_TYPE)
                	c = SDI_HAN_32(&sb->SFB.sf_dev);
        	else
                	c = SDI_HAN_32(&sb->SCB.sc_dev);
	}

        /* Check to see if we have allocated the hba specific  portion
         * of the xsb data structure.  If we have not, then allocate
         * one now.
         */
        if(! xsb->hbadata_p ) {
                xsb->hbadata_p = HIP(HBA_tbl[c].info)->hba_getblk(sleepflag, c);
                if (xsb->hbadata_p == NULL)
                        return (SDI_RET_RETRY);
                xsb->hbadata_p->sb = xsb;
        }
        ret = (*HIP(HBA_tbl[c].info)->hba_xlat)( xsb->hbadata_p, bflags, procp,
                sleepflag);

        if (((IDP(HBA_tbl[c].idata)->version_num & HBA_VMASK) == HBA_SVR4_2) ||
            (ret == SDI_RET_OK)) {
                return (SDI_RET_OK);
        } else {
                return (SDI_RET_RETRY);
        }
}

/*
 * int
 * sdi_icmd(struct sb *pt)
 *	Send SCB to HBA for immediate processing.
 *
 * Calling/Exit State:
 *	None.
 */
int
sdi_icmd(struct sb *pt, int sleepflag)
{

#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		cmn_err(CE_CONT,"sdi_icmd(%x)\n", pt);
#endif

	return(sdi_xicmd(0, pt, sleepflag));
}

/*
 * int
 * sdi_xicmd(int hbaflag, struct sb *pt, int sleepflag)
 *      Send SCB to HBA for immediate processing.
 *
 * Calling/Exit State:
 *      None.
 */
int
sdi_xicmd(int hbaflag, struct sb *pt, int sleepflag)
{
        int c;
        struct xsb *xsb = (struct xsb *)pt;

#ifdef SDI_DEBUG
        if(sdi_debug >= 1)
                cmn_err(CE_CONT,"sdi_xicmd(%x)\n", pt);
#endif

	if (! hbaflag )
                if (pt->sb_type == SFB_TYPE) {
                        c = SDI_CONTROL(pt->SFB.sf_dev.sa_ct);
			pt->SFB.sf_dev.pdi_adr_version = 0;
		}
                else {
                        c = SDI_CONTROL(pt->SCB.sc_dev.sa_ct);
			pt->SCB.sc_dev.pdi_adr_version = 0;
		}
	else
                if (pt->sb_type == SFB_TYPE)
                        c = SDI_HAN_32(&pt->SFB.sf_dev);
                else
                        c = SDI_HAN_32(&pt->SCB.sc_dev);

	/* Check to see if we have allocated the hba specific  portion
	 * of the xsb data structure.  If we have not, then allocate
	 * one now.  We have probably already allocated the hbadata_p
	 * if the target driver called sdi_translate.
	 */
	if( xsb->hbadata_p == NULL)
	{
		xsb->hbadata_p = HIP(HBA_tbl[c].info)->hba_getblk(sleepflag, c);
		if (xsb->hbadata_p == NULL)
			return (SDI_RET_RETRY);
		xsb->hbadata_p->sb = xsb;
	}
	return (*HIP(HBA_tbl[c].info)->hba_icmd)(xsb->hbadata_p, sleepflag);
}

/*
 * int
 * sdi_send(struct sb *pt, int sleepflag)
 *	Send SCB to HBA in FIFO order.
 *
 * Calling/Exit State:
 *	None.
 */
int
sdi_send(struct sb *pt, int sleepflag)
{
#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		cmn_err(CE_CONT,"sdi_send(%x)\n", pt);
#endif
	return(sdi_xsend(0, pt, sleepflag));
}

/*
 * int
 * sdi_xsend(int hbaflag, struct sb *pt, int sleepflag)
 *	Send SCB to HBA in FIFO order.
 *
 * Calling/Exit State:
 *	None.
 */
int
sdi_xsend(int hbaflag, struct sb *pt, int sleepflag)
{
	register struct xsb *xsb = (struct xsb *)pt;
	int c;
	
#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		cmn_err(CE_CONT,"sdi_send(%x)\n", pt);
#endif

	if (! hbaflag)
                if (pt->sb_type == SFB_TYPE) {
                        c = SDI_CONTROL(pt->SFB.sf_dev.sa_ct);
			pt->SFB.sf_dev.pdi_adr_version = 0;
		}
                else {
                        c = SDI_CONTROL(pt->SCB.sc_dev.sa_ct);
			pt->SCB.sc_dev.pdi_adr_version = 0;
		}
	else
                if (pt->sb_type == SFB_TYPE)
                        c = SDI_HAN_32(&pt->SFB.sf_dev);
                else
                        c = SDI_HAN_32(&pt->SCB.sc_dev);

	/* Check to see if we have allocated the hba specific  portion
	 * of the xsb data structure.  If we have not, then allocate
	 * one now.  We have probably already allocated the hbadata_p
	 * if the target driver called sdi_translate.
	 */
	if( xsb->hbadata_p == NULL) {
		xsb->hbadata_p = HIP(HBA_tbl[c].info)->hba_getblk(sleepflag, c);
		if (xsb->hbadata_p == NULL)
			return (SDI_RET_RETRY);
		xsb->hbadata_p->sb = xsb;
	}
	return (*HIP(HBA_tbl[c].info)->hba_send)(xsb->hbadata_p, sleepflag);
}

/*
 * int
 * sdi_timeout(void (*fn)(), void *arg, long ticks, pl_t pl,
 *		struct scsi_ad *scsi_adp);
 *	Perform dtimeout for a job request to the given device.
 *
 * Calling/Exit State:
 *	None.
 */
int
sdi_timeout(void (*fn)(), void *arg, long ticks, pl_t pl,
	struct scsi_ad *scsi_adp)
{
	int c, cpu;

#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		cmn_err(CE_CONT,"sdi_timeout(%x, %x, %x)\n", fn, arg, ticks);
#endif
	if (! scsi_adp->pdi_adr_version )
		c = SDI_CONTROL(scsi_adp->sa_ct);
	else
		c = SDI_HAN_32(scsi_adp);

	cpu = IDP(HBA_tbl[c].idata)->idata_cpubind;

	if(cpu >= 0)
		return (dtimeout(fn, arg, ticks, pl, cpu));
	else
		return (itimeout(fn, arg, ticks, pl));
}

/*
 * void
 * sdi_callback(struct sb *sbp)
 *	call the target driver interrupt routine to signal completion
 *	of a request.
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_callback(struct sb *sbp)
{
#ifdef SDI_DEBUG
	if(sdi_debug > 2)
		cmn_err(CE_CONT,"sdi_callback(%x)\n", sbp);
#endif

	if (sbp->sb_type == SCB_TYPE || sbp->sb_type == ISCB_TYPE) {
		if(sbp->SCB.sc_int)
			(sbp->SCB.sc_int)(sbp);
	} else {
		if(sbp->SFB.sf_int)
			(sbp->SFB.sf_int)(sbp);
	}
}

/*
 * void
 * sdi_aen(int event, int hba, int scsi_id, int lun)
 *	The sdi_aen routine is used to notify target drivers of asynchronous events
 *	occuring on ther devices and/or the devices' bus.  It takes four parameters:
 *	an event code indicating what happened, the Host Bus Adapter Number,
 *	the SCSI id and the logical unit number of the device that produced it.
 *	If a target controller caused the event, the LUN value will be -1.  If a
 *	BUS RESET occured, both the SCSI id and the LUN will be -1.  The event
 *	code names are prefixed by SDI_FLT_ and reside in sys/sdi.h.
 *	Sdi_aen scans the EDT to find the device(s) affected, and calls through the
 *	fault routine in the owner block indicated by curdrv, passing the
 *	flt_parm as an argument.
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_aen(int event, int hba, int scsi_id, int lun)
{
	struct sdi_edt *edtp;
	void sdi_fltcallbk();

	if(scsi_id == -1)	/* do all */
	{
	    for (scsi_id = 0; scsi_id < MAX_TCS; scsi_id++)
	    {
		if(lun == -1)	/* do all */
		{
			for(lun = 0; lun < MAX_LUS; lun++)
			{
				edtp = sdi_redt(hba, scsi_id, lun);
				sdi_fltcallbk(event, edtp, NULL);
			}
		}else{
			edtp = sdi_redt(hba, scsi_id, lun);
			sdi_fltcallbk(event, edtp, NULL);
		}
	    }
	} else{
		edtp = sdi_redt(hba, scsi_id, lun);
		sdi_fltcallbk(event, edtp, NULL);
	}
	
}

/*
 * void
 * sdi_xaen(int event, int hba, int scsi_id, int lun, int bus, unsigned long var)
 *
 * The sdi_xaen routine is used to notify target drivers of asynchronous events
 * occuring on the devices and/or the devices' bus.  It takes six parameters:
 * an event code indicating what happened, the Host Bus Adapter Number,
 * the SCSI id, the logical unit number, and bus of the device that produced it.
 * The 'var' parameter is an optional variable passed by the caller.
 * If a target controller caused the event, the LUN value will be -1.  If a
 * BUS RESET occured, both the SCSI id and the LUN will be -1.  The event
 * code names are prefixed by SDI_FLT_ and reside in sys/sdi.h.
 * Sdi_aen scans the EDT to find the device(s) affected, and calls through the
 * fault routine in the owner block indicated by curdrv, passing the
 * flt_parm as an argument.
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_xaen(int event, int hba, int scsi_id, int lun, int bus, unsigned long var)
{
	struct sdi_edt *edtp;
	void sdi_fltcallbk();
	int nbus, ntargets, nluns;
	int busno, scsiid, i;
	HBA_IDATA_STRUCT *idatap;

	idatap = IDP(HBA_tbl[hba].idata);
	if ((idatap->version_num & HBA_VMASK) >= PDI_UNIXWARE20) {
		nbus = idatap->idata_nbus;
		ntargets = idatap->idata_ntargets;
		nluns = idatap->idata_nluns;
	} else {
		nbus = 1;
		ntargets = MAX_TCS;
		nluns = MAX_LUS;
	}

	for (busno = 0; busno < nbus; busno++) {
	    if((bus != -1) && (bus != busno))
			continue;
	    for (scsiid = 0; scsiid < ntargets; scsiid++) {
	    	if ((scsi_id != -1) && (scsi_id != scsiid))
				continue;
			if(lun == -1) {	/* do all */
				for(i = 0; i < nluns; i++) {
					edtp = sdi_rxedt(hba, busno, scsiid, i);
					sdi_fltcallbk(event, edtp, var);
				}
			} else {
				edtp = sdi_rxedt(hba, busno, scsiid, lun);
				sdi_fltcallbk(event, edtp, var);
			}
	    }
	}
}

/* void
 * sdi_fltcallbk(int event, struct sdi_edt *edtp, unsigned long var)
 *	Call target driver's fault routine.
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_fltcallbk(int event, struct sdi_edt *edtp, unsigned long var)
{
	if(edtp == NULL)
		return;
	if (!edtp->curdrv) 
		return ;
	
	if(!edtp->curdrv->fault)
		return;
	edtp->curdrv->fault(edtp->curdrv->flt_parm, event, var);
}

/*
 * STATIC int
 * sdi_hba_freeblk(struct sb *sbp)
 *
 * Calling/Exit State:
 *	None.
 *
 * Returns: SDI_RET_OK or SDI_RET_ERR
 */
STATIC int
sdi_hba_freeblk(struct sb *sbp)
{
	return(sdi_hba_xfreeblk(0, sbp));
}

/*
 * int
 * sdi_hba_xfreeblk(int hbaflag, struct sb *sbp)
 *
 * Calling/Exit State:
 *	None.
 *
 * Returns: SDI_RET_OK or SDI_RET_ERR
 */
int
sdi_hba_xfreeblk(int hbaflag, struct sb *sbp)
{
	int hba_no, ret;

	if (! hbaflag ) {
                if (sbp->sb_type == SFB_TYPE)
                        hba_no = SDI_CONTROL(sbp->SFB.sf_dev.sa_ct);
                else
                        hba_no = SDI_CONTROL(sbp->SCB.sc_dev.sa_ct);
	}
	else {
                if (sbp->sb_type == SFB_TYPE) {
			ASSERT(sbp->SFB.sf_dev.pdi_adr_version == hbaflag);
                	ASSERT(sbp->SFB.sf_dev.extended_address);
                        hba_no = SDI_HAN_32(&sbp->SFB.sf_dev);
		}
                else {
			ASSERT(sbp->SCB.sc_dev.pdi_adr_version == hbaflag);
                	ASSERT(sbp->SCB.sc_dev.extended_address);
                        hba_no = SDI_HAN_32(&sbp->SCB.sc_dev);
		}
	}

#ifdef SDI_DEBUG
	if(sdi_debug > 2)
		cmn_err(CE_CONT,"sdi_hba_xfreeblk(%x, %x)\n", sbp);
#endif

	ret = SDI_RET_ERR;

	if (((struct xsb *)sbp)->hbadata_p) {
		ret = (*HIP(HBA_tbl[hba_no].info)->hba_freeblk)
				( ((struct xsb *)sbp)->hbadata_p, hba_no );
		((struct xsb *)sbp)->hbadata_p = (struct hbadata *)NULL;
	} 

	if ( hbaflag ) {
                if (sbp->sb_type == SFB_TYPE) {
			if (sbp->SFB.sf_dev.extended_address->version
                                			!= SET_IN_TARGET)
                        	kmem_free( sbp->SFB.sf_dev.extended_address,
                        		sizeof(struct sdi_extended_adr) );
		}
		else {
			if (sbp->SCB.sc_dev.extended_address->version
                                			!= SET_IN_TARGET)
                        	kmem_free( sbp->SCB.sc_dev.extended_address,
                                	sizeof(struct sdi_extended_adr) );
		}
	}
	return ret;
}

/*
 * int
 *sdi_HotAdd(struct scsi_adr *sa, struct ident *inq_datap, struct sense *sensep)
 *
 * Utility function that prepares the HBA for an INQUIRY on a particular LUN.
 */
STATIC int
sdi_HotAdd(HBA_IDATA_STRUCT *idatap,struct scsi_adr *sa,struct ident *inq_datap,
	struct sense *sensep)
{
	/*
	 * Prepare the HBA driver to claim a device at this SCSI address
	 */
	if (sdi_dofunc(sa, SFB_ADD_DEV, KM_SLEEP) != SDI_ASW) {
		/*
		 * This can only fail due to internal corruption.
		 */
		return SDI_RET_ERR;
	}

	/* Add the device to the EDT
	 */
	if (sdi_add_dev(idatap, sa, inq_datap, sensep) == SDI_RET_OK) {
		return SDI_RET_OK;
	} else {
		/*
		 * No device at this address so tell the HBA to forget it.
		 */
		(void) sdi_dofunc(sa, SFB_RM_DEV, KM_SLEEP);
		return SDI_RET_ERR;
	}
}

/*
 * int
 * sdi_hot_add()
 *
 * returns the # of devices found
 */
int
sdi_hot_add(struct scsi_adr *sap)
{
	int hba, bus, target, lun;
	int nhba, nbus, ntarget, nlun;
	int count=0;
	int i;
	HBA_IDATA_STRUCT *idatap;
	struct ident *inq_datap;
	struct scsi_adr sa;
	struct scsi_ad addr;
	int ctl,tgt;
	bcb_t	*bcbp;
	struct sense	*sensep;
	int	pdi_adr_version;
	ulong paddr_size;


	SLEEP_LOCK(sdi_hot_lock, pridisk);

	if (sap == NULL)
	{
		/* the scsi address is NULL.
		 * this should never happen.
		 */
		ASSERT(sap);
		SLEEP_UNLOCK(sdi_hot_lock);
		return 0;
	}

	bzero (&addr, sizeof(struct scsi_ad));
	ctl = sap->scsi_ctl == -1 ? 0: sap->scsi_ctl;
	tgt = sap->scsi_target == -1 ? 0: sap->scsi_target;
	
	addr.extended_address = 
		(struct sdi_extended_adr *)
		  kmem_zalloc(sizeof(struct sdi_extended_adr), sdi_sleepflag);

        if ( ! addr.extended_address ) {
                cmn_err(CE_WARN, "sdi_hot_add: insufficient memory"
                                        "to allocate extended address");
		SLEEP_UNLOCK(sdi_hot_lock);
                return(SDI_RET_ERR);
        }

	pdi_adr_version = sdi_ext_address(ctl);

	addr.pdi_adr_version = pdi_adr_version;
	addr.extended_address->scsi_adr.scsi_ctl = ctl;
	addr.extended_address->scsi_adr.scsi_target = tgt;

	if ( !(bcbp = sdi_xgetbcb(pdi_adr_version, &addr, sdi_sleepflag)) ||
	     !(sensep = 
		(struct sense *)sdi_kmem_alloc_phys(sizeof(struct sense),
		bcbp->bcb_physreqp, 0)) ||
	     !(inq_datap = 
		(struct ident *)sdi_kmem_alloc_phys(sizeof(struct ident),
		bcbp->bcb_physreqp, 0)) )	{

		if (bcbp)
			sdi_freebcb(bcbp);
		ASSERT(sensep);
		if (sensep)
			kmem_free(sensep, sizeof(struct sense) + paddr_size);
		if (inq_datap)
			kmem_free(inq_datap, sizeof(struct ident) + paddr_size);
	}
	paddr_size = SDI_PADDR_SIZE(bcbp->bcb_physreqp);
	/*
	 * Ungodly sense hack.
	 * The sense pointer is used multiple times in sdi_add_edt, so we must
	 * prep the hack.
	 */
	if (paddr_size)
		*((char *) sensep + sizeof(struct sense)) += 1;

	sdi_freebcb(bcbp);
	kmem_free(addr.extended_address,
			 sizeof( struct sdi_extended_adr ));

	if (sap->scsi_ctl < 0 || sap->scsi_ctl >= sdi_hacnt)
	{
		nhba = sdi_hacnt;
		hba=0;
	}
	else
	{
		hba = sap->scsi_ctl;
		nhba = hba + 1;
		
	}
	for (; hba < nhba; hba++)
	{
		if (!HBA_tbl[hba].active ||
		    (HBA_tbl[hba].info == NULL) ||
		    (HBA_tbl[hba].idata == NULL))
		{
			/* no controller here */
			continue;
		}
		    

		idatap = HBA_tbl[hba].idata;
		ASSERT(idatap);

		if ((HBA_tbl[hba].info->hba_flag == NULL) || 
		    !(*(HBA_tbl[hba].info->hba_flag) & HBA_HOT))
		{
			/* HBA controller doesn't support hot insertion/removal
			 */
			continue;
		}
		
		if (sap->scsi_bus == -1)
		{
			if ((idatap->version_num & HBA_VMASK) >= PDI_UNIXWARE20)
				nbus = idatap->idata_nbus;
			else
				nbus = 1;
			bus=0;
		}
		else
		{
			bus = sap->scsi_bus;
			nbus = bus + 1;
		}
		for (; bus < nbus; bus++)
		{
			if (sap->scsi_target == -1)
			{
				if ((idatap->version_num & HBA_VMASK)
				    >= PDI_UNIXWARE20)
					ntarget = idatap->idata_ntargets;
				else
					ntarget = MAX_TCS;
				target = 0;
			}
			else
			{
				target = sap->scsi_target;
				ntarget = target + 1;
			}
			for (; target < ntarget; target++) {
				sa.scsi_ctl = hba;
				sa.scsi_bus = bus;
				sa.scsi_target = target;

				if (sap->scsi_lun == -1) {
					int tmplun;
					struct sdi_edt *edtp;
					struct ident *validInq;

					sa.scsi_lun = 0;

					/*
					 * Decide where are we going to get the
					 * INQUIRY string from. If we can't get
					 * one, there is no device here.
					 */
					if (edtp = sdi_rxedt(sa.scsi_ctl,
					    sa.scsi_bus, sa.scsi_target,
					    sa.scsi_lun)) {
						validInq = &edtp->edt_ident;
					} else if (sdi_HotAdd(idatap, &sa,
					    inq_datap, sensep) == SDI_RET_OK) {
						validInq = inq_datap;
					} else {
						continue;
					}

					count++;	/* There is something */

					if ((tmplun = sdi_device_validate
					    (validInq)) != SDI_RET_ERR) {
						nlun = tmplun;
					} else if (validInq->id_ver & 0x02) {
						nlun = MAX_LUS;
					} else {
						nlun = MAX_LUS;
					}

					/*
					 * lun=0 was added in sdi_HotAdd above.
					 */
					lun = 1;
				} else {
					lun = sap->scsi_lun;
					nlun = lun + 1;
				}

				for (; lun < nlun; lun++) {
					sa.scsi_lun = lun;

					if (sdi_rxedt(sa.scsi_ctl, sa.scsi_bus,
					      sa.scsi_target, sa.scsi_lun)) {
						/* device is in the EDT */
						continue;
					}

					if (!sdi_lun_validate(sdi_lunsearch,
					    &sa))
						break;

					if (sdi_HotAdd(idatap, &sa, inq_datap,
						sensep) == SDI_RET_OK) {
						count++;
					}
				}
			}
		}
	}
	SLEEP_LOCK(sdi_rinit_lock, pridisk);
	sdi_in_rinits = 1;
        for (i=0; i < sdi_rtabsz; i++) {
                if (sdi_rinits[i] != NULL)
                        (*sdi_rinits[i])();
        }
        sdi_in_rinits = 0;
	SLEEP_UNLOCK(sdi_rinit_lock);
 	SLEEP_UNLOCK(sdi_hot_lock);
	kmem_free(inq_datap, sizeof(struct ident) + paddr_size);
	kmem_free(sensep, sizeof(struct sense) + paddr_size);

	return count;
}

/*
 * int
 * sdi_hot_rm()
 *
 * hot remove a device from the SCSI system
 *
 * sap specifies a specific SCSI address [hba, bus, target, lun]
 *
 * if a target driver has claimed this device then the target
 * driver will disclaim the device
 *
 * the HBA driver will disclaim the device
 *
 * the device will be removed from the edt
 *
 * returns SDI_RET_OK on success
 * returns SDI_RET_ERR on failure
 *
 * Reasons for failure: device not in EDT
 *                      HBA driver doesn't support hot insertion/removal
 *                      target driver doesn't support hot insertion/removal
 */
int
sdi_hot_rm(struct scsi_adr *sap)
{
	struct sdi_edt *edtp;

	SLEEP_LOCK(sdi_hot_lock, pridisk);

	if (sap == NULL)
	{
		/* The scsi address is NULL.
		 * This shouldn't ever happen.
		 */
		ASSERT(sap);
		SLEEP_UNLOCK(sdi_hot_lock);
		return SDI_RET_ERR;
	}

	edtp = sdi_rxedt(sap->scsi_ctl, sap->scsi_bus,
			 sap->scsi_target, sap->scsi_lun);

	if (edtp == NULL)
	{
		/* the SCSI device specified in sap doesn't exit
		 */
		SLEEP_UNLOCK(sdi_hot_lock);
		return SDI_RET_ERR;
	}

	ASSERT(HBA_tbl[edtp->hba_no].active);
	ASSERT(HBA_tbl[edtp->hba_no].info);
	ASSERT(HBA_tbl[edtp->hba_no].idata);

	if ((HBA_tbl[edtp->hba_no].info->hba_flag == NULL) || 
	    !(*(HBA_tbl[edtp->hba_no].info->hba_flag) & HBA_HOT))
	{
		/* HBA driver doesn't support hot insertion/removal
		 */
		SLEEP_UNLOCK(sdi_hot_lock);
		return SDI_RET_ERR;
	}

	if ((HBA_tbl[edtp->hba_no].idata)->version_num >= HBA_UW21_IDATA) {
		if ((HBA_tbl[edtp->hba_no].idata)->ha_chan_id[sap->scsi_bus] ==
			sap->scsi_target) {
			/* Can't hot remove the controller */
			SLEEP_UNLOCK(sdi_hot_lock);
			return SDI_RET_ERR;
		}
	} else {
		if ((HBA_tbl[edtp->hba_no].idata)->ha_id == sap->scsi_target) {
			/* Can't hot remove the controller */
			SLEEP_UNLOCK(sdi_hot_lock);
			return SDI_RET_ERR;
		}
	}

	if (edtp->curdrv != NULL)
	{
		if (edtp->curdrv->target_rm_dev == NULL)
		{
			/* target driver doesn't support
			 * hot insertion/removal
			 */
			SLEEP_UNLOCK(sdi_hot_lock);
			return SDI_RET_ERR;
		}
		if (edtp->curdrv->target_rm_dev(sap) != SDI_RET_OK)
		{
			/* target driver can't free device.  The
			 * device is probably open but it could be
			 * that the target driver doesn't know about
			 * the device which would indicate an internal
			 * error in hot insertion/removal.
			 */
			
			SLEEP_UNLOCK(sdi_hot_lock);
			return SDI_RET_ERR;
		}
	}

	if (sdi_dofunc(sap, SFB_RM_DEV, KM_SLEEP) != SDI_ASW)
	{
		/* The HBA driver couldn't remove the device.
		 * This would happen if there are pending jobs
		 * on the controller.
		 * Add the removed device back to the target
		 * driver.
		 */
		int i;

		SLEEP_LOCK(sdi_rinit_lock, pridisk);
		sdi_in_rinits = 1;
		for (i=0; i < sdi_rtabsz; i++) {
			if (sdi_rinits[i] != NULL)
				(*sdi_rinits[i])();
		}
		sdi_in_rinits = 0;
		SLEEP_UNLOCK(sdi_rinit_lock);
		SLEEP_UNLOCK(sdi_hot_lock);
		return SDI_RET_ERR;
	}

	if (sdi_delete_edt(edtp) != SDI_RET_OK)
	{
		/* The edt couldn't be deleted.  This indicates
		 * internal corruption in the EDT.
		 */
		ASSERT(sap == NULL);
		SLEEP_UNLOCK(sdi_hot_lock);
		return SDI_RET_ERR;
	}
	SLEEP_UNLOCK(sdi_hot_lock);
	return SDI_RET_OK;
}

/*
 * int
 * sdi_dofunc()
 *
 * send a PDI SCSI function to a HBA driver via the HBA's XXXicmd()
 * entry point.
 *
 * sap - the SCSI address of the HBA driver that this function is
 *       to be sent to.  Does not necessarily have to be completely
 *       defined if the bus/target/lun is not needed.
 *
 * sf_func - what PDI SCSI function is to be executed
 *
 * returns the completion code filled in the by XXXicmd().  Values
 * are in sdi_comm.h.
 *
 */
int 
sdi_dofunc(struct scsi_adr *sap, ulong_t sf_func, int sleepflag)
{
	struct sb *sp;
	int retcode;
	int pdi_adr_version;

	if ((sap->scsi_ctl < 0) ||
	    (sap->scsi_ctl >= sdi_hacnt) ||
	    !HBA_tbl[sap->scsi_ctl].active ||
	    (HBA_tbl[sap->scsi_ctl].info == NULL)  ||
	    (HBA_tbl[sap->scsi_ctl].idata == NULL))
	{
		return SDI_SFBERR;
	}

	pdi_adr_version = sdi_ext_address(sap->scsi_ctl);
	sp = sdi_xgetblk(pdi_adr_version, sleepflag);

	ASSERT(sp);

	sp->sb_type = SFB_TYPE;
	sp->SFB.sf_int = NULL;
	sp->SFB.sf_func = sf_func;
	sp->SFB.sf_dev.extended_address =
		sp->SCB.sc_dev.extended_address;
	sp->SFB.sf_dev.pdi_adr_version = pdi_adr_version;
	if (! pdi_adr_version ) {
		sp->SFB.sf_dev.sa_ct =
			 SDI_SA_CT(sap->scsi_ctl, sap->scsi_target);
		sp->SFB.sf_dev.sa_exta = sap->scsi_target;
		sp->SFB.sf_dev.sa_lun  = sap->scsi_lun;
		sp->SFB.sf_dev.sa_bus  = sap->scsi_bus;
	} else
		sp->SFB.sf_dev.extended_address->scsi_adr = *sap;

#ifdef SDI_DEBUG
        if(sdi_debug > 1)
                cmn_err(CE_CONT,"sdi_docmd: calling sdi_xicmd\n");
#endif
        sdi_xicmd(pdi_adr_version, sp, sleepflag);
        retcode = sp->SCB.sc_comp_code;

#ifdef SDI_DEBUG
        if(sdi_debug > 1) {
                if ( retcode != SDI_ASW )
                        cmn_err(CE_CONT,"sdi_docmd: retcode = %x\n", retcode);
		cmn_err(CE_CONT,"sdi_docmd: calling sdi_xfreeblk\n");
        }
#endif
	sdi_xfreeblk(pdi_adr_version, sp);
        return retcode;
}

void 
sdi_int(struct sb *sp)
{
	ASSERT(sp);
	biodone((struct buf *)(sp->SCB.sc_wd));
}

/*
 * int
 * sdi_docmd(struct scsi_adr *sap, caddr_t cdb_p, long cdbsz,
 * 			caddr_t data_p, long datasz, int rw_flag)
 *	Create and send an SCB associated SCSI command. 
 *
 * Calling/Exit State:
 *	None.
 */
int
sdi_docmd (
	   struct scsi_adr *sap,	/* SCSI address ptr	*/
	   caddr_t	cdb_p,		/* pointer to cdb 	*/
	   long	cdbsz,			/* size of cdb		*/
	   caddr_t	data_p,		/* command data area 	*/
	   long	datasz,			/* size of buffer	*/
	   int	rw_flag)		/* read/write flag	*/
{
	register struct sb   *sp;
	int		     retcode;
	int	pdi_adr_version, *flagp;
	struct xsb *xsb;
	struct buf *bp;
	struct scs *cmdp = (struct scs *)cdb_p;
#ifdef SDI_DEBUG
	if(sdi_debug > 2)
		cmn_err(CE_CONT,"sdi_docmd (%x, %x, %x, %x, %x, %x, %x %x)\n",
		    hba, scsi_id, lun, cdb_p, cdbsz, data_p, datasz, rw_flag);
#endif

	pdi_adr_version = sdi_ext_address(sap->scsi_ctl);

	sp = sdi_xgetblk(pdi_adr_version, sdi_sleepflag);
	xsb = (struct xsb *)sp;
	sp->sb_type       = ISCB_TYPE;

	/*
	 * Some HBA drivers don't call sdi_callback in all instances.
	 * It is a requirement that target drivers that support
	 * hot insertion/removal fix this bug.  Otherwise
	 * this code doesn't work very well in certain cases.
	 */
	flagp = HBA_tbl[sap->scsi_ctl].info->hba_flag;
	if (flagp && (((*flagp & HBA_HOT) && (sdi_sleepflag == KM_SLEEP)) || 
			(*flagp & HBA_CALLBACK)))
	{
		bp = getrbuf(sdi_sleepflag);
		ASSERT(bp);
		sp->SCB.sc_int    = sdi_int;
		sp->SCB.sc_wd     = (long)bp;
	}
	else
	{
		sp->SCB.sc_int    = NULL;
		sp->SCB.sc_wd     = NULL;
	}
	sp->SCB.sc_cmdpt  = cdb_p;
	sp->SCB.sc_cmdsz  = cdbsz;
	sp->SCB.sc_datapt = data_p;

	if (data_p) {
		xsb->extra.sb_data_type = SDI_PHYS_ADDR;
		if (cmdp->ss_op != SS_REQSEN)
			xsb->extra.sb_datapt = 
				(paddr32_t *) ((char *)data_p + datasz);
		else {
			xsb->extra.sb_datapt =
				(paddr32_t *) ( ((char *)data_p - 1) + 
						sizeof(struct sense) );
#ifdef OUT
			/*
			 * Ungodly sense hack.
			 * The problem is that the sense buffer is allocated
			 * in sdi_register and used multiple times through
			 * sdi_add_edt. Hence this adjustment is bogus. Instead
			 * we make this adjustment at allocation time in
			 * sdi_register.
			 */
			*(xsb->extra.sb_datapt) += 1;
#endif
		}
	}

	sp->SCB.sc_datasz = datasz;
	sp->SCB.sc_time   = (10 * ONE_SEC);

	if (! pdi_adr_version ) {
		sp->SCB.sc_dev.sa_ct =
			 SDI_SA_CT(sap->scsi_ctl, sap->scsi_target);
		sp->SCB.sc_dev.sa_exta = sap->scsi_target;
		sp->SCB.sc_dev.sa_lun  = sap->scsi_lun;
		sp->SCB.sc_dev.sa_bus  = sap->scsi_bus;
	}
	else
		sp->SCB.sc_dev.extended_address->scsi_adr = *sap;

        if (rw_flag & B_READ)
           sp->SCB.sc_mode = SCB_READ;
        else
           sp->SCB.sc_mode = SCB_WRITE;

	sdi_xtranslate (pdi_adr_version, sp, rw_flag, NULL, sdi_sleepflag);

	/* we assume that with interrupts off, icmd will complete the i/o
	 * and call back to sdi_int before returning here
   	 */
#ifdef SDI_DEBUG
	if(sdi_debug > 1)
		cmn_err(CE_CONT,"sdi_docmd: calling sdi_xicmd\n");
#endif
	sdi_xicmd(pdi_adr_version, sp, sdi_sleepflag);
	if (sp->SCB.sc_int)
	{
		(void)biowait(bp);
		freerbuf(bp);
	}
	retcode = sp->SCB.sc_comp_code;
#ifdef SDI_DEBUG
	if(sdi_debug > 1) {
		if ( retcode != SDI_ASW )
			cmn_err(CE_CONT,"sdi_docmd: retcode = %x\n", retcode);
		cmn_err(CE_CONT,"sdi_docmd: calling sdi_freeblk\n");
	}
#endif
	sdi_xfreeblk(pdi_adr_version, sp);
	return retcode;

}

/*
 * unsigned short
 * sdi_swap16(uint x)
 *	swaps bytes in a 16 bit data type.  The result in unsigned.
 *
 * Calling/Exit State:
 *	None.
 */
unsigned short
sdi_swap16(uint x)
{
	unsigned short rval = ((x & 0x00ff) << 8)  | ((x & 0xff00) >> 8);
	return rval;
}


/*
 * int
 * sdi_swap24(uint x)
 *	swaps bytes in a 24 bit data type.
 * Note: even though this routine takes an unsigned input, the return
 *	is signed because of interface documentation issues and current usage.
 *
 * Calling/Exit State:
 *	None.
 */
int
sdi_swap24(uint x)
{
	unsigned int rval;

	rval =  (x & 0x0000ff) << 16;
	rval |= (x & 0x00ff00);
	rval |= (x & 0xff0000) >> 16;
	return ((int)rval);
}


/*
 * long
 * sdi_swap32(ulong x)
 *	swaps bytes in a 32 bit data type.
 * Note: even though this routine takes an unsigned input, the return
 *	is signed because of interface documentation issues and current usage.
 *
 * Calling/Exit State:
 *	None.
 */
long
sdi_swap32(ulong x)
{
	unsigned long rval;

	rval =  (x & 0x000000ff) << 24;
	rval |= (x & 0x0000ff00) << 8;
	rval |= (x & 0x00ff0000) >> 8;
	rval |= (x & 0xff000000) >> 24;
	return ((long)rval);
}

/*
 * int
 * sdi_gethbano(int n)
 *
 * Calling/Exit State:
 *	None.
 */
int
sdi_gethbano(int n)
{
	int	cntlr;

	if(n < 0)	{
		cntlr = sdi_hacnt;
	}
	else	{
		cntlr = n;
		if(HBA_tbl[cntlr].info != NULL)	
			cntlr = sdi_hacnt;
	}

	if(cntlr >= sdi_hbaswsz)	{
		return(-ECONFIG);
	}

	HBA_map[cntlr] = n;

	return(cntlr);
}

/*
 * void
 * sdi_target_hotregister()
 *     register the target driver's XXX_rm_dev() function entry
 *     for the devices in the list of ownerp.  This function must
 *     be called with valid arguments if a target driver supports
 *     hot insertion/removal.
 *
 * target_rm_dev - the function that is to be registered.  It will
 *                 be called whenever a device that is owned by this
 *                 target driver has been hot removed.
 *
 * ownerp - a list of struct owner, one for each device owned by this
 *          target driver.  The list is linked by the target_link field
 *          and is NULL terminated.
 *               
 */
void
sdi_target_hotregister(int (*target_rm_dev)(), struct owner *ownerp)
{
	for (;ownerp != NULL; ownerp = ownerp->target_link)
	{
		ownerp->target_rm_dev = target_rm_dev;
	}
}


/*
 * int
 * sdi_register(void *infop, void *idatap)
 *	Register an HBA
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	If is not called at init/start, acquires sdi_rinit_lock.
 */
int
sdi_register(void *infop, void *idatap)
{
	struct	hba_cfg	*hbap;
	int	i, cntlr, onext_hba;
	struct	ident *inq_datap;
	int	sdi_add_edt();
	int	ctlorder;
	int ret_val;
	extern int default_bindcpu;
	struct scsi_ad addr;
	bcb_t	*bcbp;
	struct sense *sensep;
	int	*flagp;
	int	dflag;
	ulong   paddr_size;

	boolean_t		need_bind = B_FALSE;
	struct hba_info		*bind_infop;
	HBA_IDATA_STRUCT	*idp, *newidp = NULL;
	struct hba_ext_info	*eip;
	int			bcpu;
	struct shad_hbatbl	*shbap;

	if((cntlr = IDP(idatap)->cntlr) < 0)	{
		return(-ENXIO);
	}

	if(cntlr >= sdi_hbaswsz)	{
		return(-ECONFIG);
	}

	if ( sdi_sleepflag == KM_SLEEP )
		SLEEP_LOCK(sdi_rinit_lock, pridisk);

	onext_hba = sdi_hacnt;
	if((cntlr + 1) > sdi_hacnt)	{
		sdi_hacnt = cntlr + 1;
	}

	hbap = &HBA_tbl[cntlr];

	if(hbap->info != NULL)	{
		ret_val = (-ECONFIG);
		goto error_exit;
	}

	flagp = HIP(infop)->hba_flag;

	idp = IDP(idatap);
	bcpu = idp->idata_cpubind;

	if (bcpu == -1)
		bcpu = default_bindcpu;

	if (bcpu >= 0 || !flagp || !(*flagp & HBA_MP)) {
		if(sdi_shad_hbatbl == NULL)	{
			sdi_shad_hbatbl = (struct shad_hbatbl *)
				kmem_zalloc(sizeof(struct shad_hbatbl)*sdi_hbaswsz,
					sdi_sleepflag);

			if(sdi_shad_hbatbl == NULL)	{
				ret_val = (-ENOMEM);
				goto error_exit;
			}
		}

		bind_infop = (struct hba_info *)kmem_alloc(sizeof(struct hba_info),
							   sdi_sleepflag);
		if (bind_infop == NULL)	{
			ret_val = (-ENOMEM);
			goto error_exit;
		}

		*bind_infop = sdi_hbabind_info;
		bind_infop->hba_flag = HIP(infop)->hba_flag;
		bind_infop->max_xfer = HIP(infop)->max_xfer;

		if (bcpu < 0)	{
			bcpu = 0;
		}

		shbap = &sdi_shad_hbatbl[cntlr];

		shbap->info = HIP(infop);
		shbap->enginep = &engine[bcpu];
		infop = bind_infop;

		need_bind = B_TRUE;

#ifdef DEBUG
		/*
		 *+ The HBA driver in question is not MP, it
		 *+ is being bound to the indicated CPU.
		 */
		cmn_err(CE_NOTE, "Binding HBA driver %s to CPU %d",
			IDP(idatap)->name, bcpu);
#endif

	}
	/*
	 * If the driver is a previous version, allocate a new
	 * idata structure and copy the original structure over.
	 * All new idata fields become zero by the allocation.
	 */
	dflag = 0;
	if ((IDP(idatap)->version_num & HBA_VMASK) < PDI_UNIXWARE20) {
		newidp=(HBA_IDATA_STRUCT *)kmem_zalloc(sizeof(HBA_IDATA_STRUCT),
			sdi_sleepflag);
		if (newidp == NULL) {
			ret_val = (-ENOMEM);
			goto error_exit;
		}
		bcopy(IDP(idatap), newidp, sizeof(struct hba_idata));
		idatap = (void *)newidp;
		dflag++;
	}

	hbap->info	= HIP(infop);
	hbap->idata	= IDP(idatap);
	hbap->active	= 1;
#ifdef DEBUG
	cmn_err(CE_NOTE,
		"!(1)EXTENDED ADDRESS FLAG: 0x%x", (hbap->idata->version_num & HBA_EXT_ADDRESS));
#endif

	/*
	 * Check if this is a 4.2 HBA driver
	 */
	if ((hbap->idata->version_num & HBA_VMASK) == HBA_SVR4_2) {
		if (need_bind)	{
			kmem_free(infop, sizeof(struct hba_info));
		}
		hbap->info = NULL;
		hbap->idata = NULL;
		hbap->active = 0;
		/*
		 *+ sdi_register of SVR4.2 driver failed since
		 *+ PHYSTOKVMEM tunable is not set.  Rebuild
		 *+ kernel with PHYSTOKVMEM.
		 */
		cmn_err (CE_WARN, "sdi_register of SVR4.2 driver failed because PHYSTOKVMEM");
		cmn_err (CE_CONT, "support is unavailable in this kernel.");
		ret_val = (-ECONFIG);
		goto error_exit;
	}
	if(dflag)
		newidp->version_num = HBA_UW21_IDATA;

	/*
	 * Fields that are not filled in by drivers, or that
	 * are not known to previous releases, are initialized
	 * to default values.
	 */
	if (IDP(idatap)->idata_nbus == 0)
		IDP(idatap)->idata_nbus = 1;
	if (IDP(idatap)->idata_ntargets == 0)
		IDP(idatap)->idata_ntargets = MAX_TCS;
	if (IDP(idatap)->idata_nluns == 0)
		IDP(idatap)->idata_nluns = MAX_LUS;
	ctlorder = IDP(idatap)->idata_ctlorder;
	IDP(idatap)->idata_cpubind = bcpu;

	bzero (&addr, sizeof(struct scsi_ad));
	addr.extended_address =
                (struct sdi_extended_adr *)
                  kmem_zalloc(sizeof(struct sdi_extended_adr), sdi_sleepflag);

        if ( ! addr.extended_address ) {
                cmn_err(CE_WARN, "sdi_hot_add: insufficient memory"
                                        "to allocate extended address");
		ret_val = (-ENOMEM);
		goto error_exit;
        }

	addr.pdi_adr_version = sdi_ext_address(cntlr);
	if ( ! addr.pdi_adr_version ) {
		addr.sa_ct = SDI_SA_CT(cntlr, 0);
	}
	else {
        	addr.extended_address->scsi_adr.scsi_ctl = cntlr;
	}

	if (!(bcbp = sdi_xgetbcb(addr.pdi_adr_version, &addr, sdi_sleepflag)) ||
	    !(sensep = (struct sense *)sdi_kmem_alloc_phys(sizeof(struct sense), bcbp->bcb_physreqp, 0)) ||
	    !(inq_datap = (struct ident *)sdi_kmem_alloc_phys(sizeof(struct ident), bcbp->bcb_physreqp, 0)) )	{
		if (bcbp) {
			paddr_size = SDI_PADDR_SIZE(bcbp->bcb_physreqp);
			sdi_freebcb(bcbp);
		}
		ASSERT(sensep);
		if (sensep)
			kmem_free(sensep, sizeof(struct sense) + paddr_size);
		ASSERT(inq_datap);
		if (inq_datap)
			kmem_free(inq_datap, sizeof(struct ident) + paddr_size);
		hbap->info = NULL;
		hbap->idata = NULL;
		hbap->active = 0;
		ret_val = (-ENOMEM);
		goto error_exit;
	}

	paddr_size = SDI_PADDR_SIZE(bcbp->bcb_physreqp);
	/*
	 * Ungodly sense hack.
	 * The sense pointer is used multiple times in sdi_add_edt, so we must
	 * prep the hack.
	 */
	if (paddr_size)
		*((char *) sensep + sizeof(struct sense)) += 1;

	sdi_freebcb(bcbp);
	kmem_free(addr.extended_address, sizeof( struct sdi_extended_adr ));

	idp = IDP(idatap);
	if(sdi_add_edt(idp, inq_datap, sensep, HIP(infop)) == 0)	{

		if (need_bind)	{
			kmem_free(infop, sizeof(struct hba_info));
		}

		hbap->info = NULL;
		hbap->idata = NULL;
		hbap->active = 0;
		ret_val = (-ECONFIG);
		goto error_exit;
	}

	/*
	 * Keep track of the lowest order controller (idata_ctlorder)
	 * to determine (later) if an HBA is setting itself as the 
	 * boot device.
	 */
	if (ctlorder > 0) {
		if (!sdi_ctlorder || (ctlorder < sdi_ctlorder))
			sdi_ctlorder = ctlorder;
	}

	/* Now call the rinit functions currently registered.
	 * The sdi_rinits[] routines call sdi_doconfig;
	 * set sdi_in_rinits flag so that sdi_doconfig() knows
	 * that we already hold sdi_rinit_lock.
	 */
	sdi_in_rinits = 1;
	for (i=0; i < sdi_rtabsz; i++) {
		if (sdi_rinits[i] != NULL)
			(*sdi_rinits[i])();
	}
	sdi_in_rinits = 0;

	ret_val = cntlr;

	if (pdi_timeout && flagp && (*flagp & HBA_TIMEOUT_RESET)) {
		static struct scsi_adr sa;
		sa.scsi_ctl = cntlr;
		sdi_dofunc(&sa, SFB_TIMEOUT_ON, KM_SLEEP);
	}
	goto normal_exit;

error_exit:
	sdi_hacnt = onext_hba;
	HBA_map[cntlr] = SDI_UNUSED_MAP;
	if(newidp)
		kmem_free(newidp, sizeof(HBA_IDATA_STRUCT));

normal_exit:
	if ( sdi_sleepflag == KM_SLEEP )
		SLEEP_UNLOCK(sdi_rinit_lock);
	kmem_free(inq_datap, sizeof(struct ident) + paddr_size);
	kmem_free(sensep, sizeof(struct sense) + paddr_size);
	return(ret_val);
}


/*
 * int
 * sdi_deregister(void *idp)
 *	De-Register an HBA
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
sdi_deregister(HBA_IDATA_STRUCT *idp)
{

	struct scsi_adr	sa;
	int     nbus, ncntls, ntargets, nluns;
	int	b,c,t,l;
	int	cntlid;

	if (idp == NULL) {
		return 0;
	}

	if (!HBA_tbl[idp->cntlr].active)
		return 0;
	/*
	 * stop all the devices 
	 */

	sa.scsi_ctl = idp->cntlr;

	if ((idp->version_num & HBA_VMASK) >= PDI_UNIXWARE20) {
		nbus = idp->idata_nbus;
		ntargets = idp->idata_ntargets;
		nluns = idp->idata_nluns;
	} else {
		nbus = 1;
		ntargets = MAX_TCS;
		nluns = MAX_LUS;
	}

	for (b = 0; b < nbus; b++) {
		sa.scsi_bus = b;

		if((idp->version_num & HBA_VMASK) >= HBA_UW21_IDATA) { 
			cntlid = (int)(idp->ha_chan_id[b]);
		}
		else {
			cntlid = (int)(idp->ha_id);
		}

		for (t = 0; t <ntargets; t++) {
			/* skip the controller */
			if (t == cntlid)
				continue;	
			sa.scsi_target = t;

			for (l = 0; l < nluns; l++) {
				sa.scsi_lun = l;
				
				/* check if the device exists */
				if (sdi_rxedt(sa.scsi_ctl, sa.scsi_bus,
						sa.scsi_target, 
						sa.scsi_lun) == NULL) {
					/* no such device */
					continue;
				}
				if (sdi_hot_rm(&sa) != SDI_RET_OK)
					return EINVAL;
			}
		}

		/*
		 * do the controller now
		 */
		sa.scsi_target = cntlid;
		sa.scsi_lun = 0;

		if (sdi_delete_edt(sdi_rxedt(sa.scsi_ctl, sa.scsi_bus,
					sa.scsi_target, sa.scsi_lun))
						!= SDI_RET_OK) {
			return EINVAL;
		}
	}

	HBA_tbl[idp->cntlr].info = NULL;
	HBA_tbl[idp->cntlr].idata = NULL;
	HBA_tbl[idp->cntlr].active = 0;
	return 0;
}

/*
 * Checks to see if a device is at the SCSI address
 * specified in sap.
 *
 * returns SDI_INQ_CONNECTED
 * returns SDI_INQ_DISCONNECTED
 * returns SDI_INQ_UNSUPPORTED
 *
 * NOTE: assumptions are made that when a range of luns
 * are being inserted into the EDT that the insertions
 * are done in increasing order.
 */
int
sdi_check_dev(struct scsi_adr *sap, struct ident *inq_datap, 
		struct sense *sensep)
{
	struct scs	inq_cdb;
	char inquire[INQ_EXLEN], *inqp;
	struct sdi_edt *edtp;
	unsigned char pdtype;

	ASSERT(sap);
	ASSERT(inq_datap);
	bzero (inq_datap, sizeof(struct ident));

	/* send an inquiry to see if there is a device */
	inq_cdb.ss_op = SS_INQUIR;      /* inquiry cdb */
        inq_cdb.ss_addr1 = 0;
        inq_cdb.ss_addr = 0;
        inq_cdb.ss_len = IDENT_SZ;
        inq_cdb.ss_cont = 0;
	inq_cdb.ss_lun = sap->scsi_lun;

	if (sdi_docmd(sap, 
		      (caddr_t)&inq_cdb, (long)SCS_SZ,
		      (caddr_t)inq_datap, (long) IDENT_SZ,
		      B_READ) != SDI_ASW)
	{
#ifdef SDI_DEBUG
		cmn_err(CE_CONT,"target %d, lun %d: not ASW\n", scsiid, lun);
#endif
		return SDI_INQ_UNSUPPORTED;
 	}

	/* Check the inquiry returned */
	if (inq_datap->id_pqual == SCSI_INQ_TNC) {
		/*
		 * This physical device is not supported.
		 */
#ifdef SDI_DEBUG
		cmn_err(CE_CONT, "target %d, lun%d: TNC\n", scsiid, lun);
#endif
 
		return SDI_INQ_UNSUPPORTED;
	} else if (inq_datap->id_pqual == SCSI_INQ_TC) {
		/*
		 * This physical device is supported but is not currently
		 * connected to this logical unit. We will configure it and
		 * skip the test. This is common in RAID boxes.
		 */
                return SDI_INQ_DISCONNECTED;
	}

	/* REALLY check that a device is there */
	inquire[0] = '\0';
	inqp = inquire;
	pdtype = ID_NODEV;
	if (sap->scsi_lun > 0)
	{
		edtp = sdi_rxedt(sap->scsi_ctl, sap->scsi_bus,
				 sap->scsi_target, sap->scsi_lun - 1);
		if (edtp) {
			inqp = edtp->inquiry;
			pdtype = edtp->pdtype;
		}
	}
	
	if (!sdi_cklu(sap, inqp, &pdtype, inq_datap, sensep))
	{
#ifdef SDI_DEBUG
		cmn_err(CE_CONT, "target %d, lun %d: nodev\n", scsiid, lun);
#endif
		return SDI_INQ_UNSUPPORTED;
	}
	return SDI_INQ_CONNECTED;
}

/*
 * Searches the SCSI address specified by sap.
 * If a device is found then inq_datap is filled in and the
 * device is entered into the edt.
 *
 * returns SDI_RET_OK if a device is found
 * returns SDI_RET_ERR if a device is not found
 *
 * NOTE: assumptions are made that when a range of luns
 * are being inserted into the EDT that the insertions
 * are done in increasing order.
 */
int 
sdi_add_dev(HBA_IDATA_STRUCT *idp,
		struct scsi_adr *sap,
		struct ident *inq_datap,
		struct sense *sensep)
{
	struct sdi_edt *edtp;
	struct scsi_ad scsi_ad;

	/*
	 * check to see if a device is at this location
	 */
	if (sdi_check_dev(sap, inq_datap, sensep) == SDI_INQ_UNSUPPORTED)
		return SDI_RET_ERR;

	edtp = sdi_rxedt(sap->scsi_ctl, sap->scsi_bus,
			 sap->scsi_target, sap->scsi_lun);
	
	ASSERT(edtp == NULL); 
	
	if ( !edtp ) {
		
		if ((edtp = sdi_alloc_edt()) == NULL)
		{
			return SDI_RET_ERR;
		}
		edtp->scsi_adr = *sap;
		sdi_insert_edt(edtp);
	}

	/*
	 * Fill in the fields of the edt
	 * Keep SCSI address, hba_no, scsi_id, lun,
	 * fields of the sdi_edt for binary 
	 * compatibility, only.
	 */

	edtp->hba_no = edtp->scsi_adr.scsi_ctl;
	edtp->scsi_id = edtp->scsi_adr.scsi_target;
	edtp->lun = edtp->scsi_adr.scsi_lun;


	edtp->pdtype = inq_datap->id_type;

	bcopy(inq_datap->id_vendor,
	      edtp->inquiry,
	      INQ_EXLEN);
	edtp->inquiry[INQ_EXLEN-1] = '\0';
	edtp->edt_ident = *(inq_datap);

	edtp->timeout = kmem_zalloc(SDI_TIMEOUT_VALUES * sizeof(ushort_t),
						sdi_sleepflag);
	if (!edtp->timeout)
		cmn_err(CE_WARN, "sdi cannot allocate timeout values");
	
	if ((idp->version_num & HBA_VMASK) >= PDI_UNIXWARE20) {
		edtp->memaddr = idp->idata_memaddr;
		edtp->ctlorder = idp->idata_ctlorder;
	}

	/*
	 * set the iotype field in the edt
	 */
	scsi_ad.extended_address = 
		(struct sdi_extended_adr *)
		 kmem_zalloc(sizeof(struct sdi_extended_adr), sdi_sleepflag);

        if ( ! scsi_ad.extended_address ) {
                cmn_err(CE_WARN, "sdi_add_dev: insufficient memory"
                                        "to allocate extended address");
                return(SDI_RET_ERR);
        }

	scsi_ad.pdi_adr_version = sdi_ext_address(edtp->scsi_adr.scsi_ctl);
	if (! scsi_ad.pdi_adr_version ) {
		scsi_ad.sa_ct = SDI_SA_CT(edtp->scsi_adr.scsi_ctl,
                                  edtp->scsi_adr.scsi_target);
		scsi_ad.sa_exta = (uchar_t)edtp->scsi_adr.scsi_target;
        	scsi_ad.sa_lun = (ushort_t)edtp->scsi_adr.scsi_lun;
        	scsi_ad.sa_bus = (uchar_t)edtp->scsi_adr.scsi_bus;
	}
	else
		scsi_ad.extended_address->scsi_adr = edtp->scsi_adr;

	sdi_iotype(&scsi_ad, &edtp->iotype, &edtp->ex_iotype, inq_datap);
	kmem_free(scsi_ad.extended_address,
			 sizeof( struct sdi_extended_adr ));

	/*
	 * Fill in the hba ID
	 */
	if ((idp->version_num & HBA_VMASK) >= HBA_UW21_IDATA) {
		edtp->ha_chan_id = idp->ha_chan_id[SDI_BUS_32(&scsi_ad)];
	} else {
		edtp->ha_chan_id = idp->ha_id;
	}

	/*
	 * print out information about the device
	 */
	if ((edtp->scsi_adr.scsi_target == edtp->ha_chan_id) &&
	    (edtp->scsi_adr.scsi_bus == 0)) 
	{
	    cmn_err(CE_CONT, "%d:%d,%d,%d: %s: %s\n",
		edtp->scsi_adr.scsi_ctl,
		edtp->scsi_adr.scsi_bus,
		edtp->scsi_adr.scsi_target,
		edtp->scsi_adr.scsi_lun,
		sdi_scsi_pdt[0],
		edtp->inquiry);
	} else if ((edtp->scsi_adr.scsi_target == edtp->ha_chan_id) &&
	    (edtp->scsi_adr.scsi_bus != 0)) 
	{
	    cmn_err(CE_CONT, "  %d,%d,%d: %s: %s\n",
		edtp->scsi_adr.scsi_bus,
		edtp->scsi_adr.scsi_target,
		edtp->scsi_adr.scsi_lun,
		sdi_scsi_pdt[0],
		edtp->inquiry);
	} else if (edtp->pdtype <= ID_COMMUNICATION) {
	    cmn_err(CE_CONT, "  %d,%d,%d: %s: %s\n",
		edtp->scsi_adr.scsi_bus,
		edtp->scsi_adr.scsi_target,
		edtp->scsi_adr.scsi_lun,
		sdi_scsi_pdt[edtp->pdtype + 1],
		edtp->inquiry);
	} else {
	    cmn_err(CE_CONT, "  %d,%d,%d: %d: \n",
		edtp->scsi_adr.scsi_bus,
		edtp->scsi_adr.scsi_target,
		edtp->scsi_adr.scsi_lun,
		edtp->pdtype,
		edtp->inquiry);
	}

	return SDI_RET_OK;
}

/*
 * int
 * sdi_add_edt(HBA_IDATA_STRUCT *, 
 *             struct ident *inq_datap,
 *             struct sense *sensep,
 *             struct hab_info *infop)
 *
 *	Add the devices and HBA to the EDT.  The devices are added
 *	first, and if no devices exist, the return value is 0.  If
 *	one or more devices are found, the HBA is added to the EDT,
 *	and the return value is ndevices+1.
 *
 * Calling/Exit State:
 *	sdi_rinit_lock held across call, unless is called at init/start.
 */
int
sdi_add_edt(HBA_IDATA_STRUCT *idp,
	    struct ident *inq_datap,
            struct sense *sensep,
	    struct hba_info *infop)
{
	int scsiid, lun;
	int ndev;
	struct scsi_adr scsi_adr;
	int nbus, ntargets, nluns, vluns;
	int bus;
	int cntlrid;		/* SCSI target address of controller */

	ndev = 0;
	scsi_adr.scsi_ctl = idp->cntlr;

	if ((idp->version_num & HBA_VMASK) >= PDI_UNIXWARE20) {
		nbus = idp->idata_nbus;
		ntargets = idp->idata_ntargets;
		vluns = idp->idata_nluns;
	} else {
		nbus = 1;
		ntargets = MAX_TCS;
		vluns = MAX_LUS;
	}

	for (bus = 0; bus < nbus; bus++) {
		int bus_count;	/* number of devices per bus */

		bus_count=0;

		/* do the controller first */
		scsi_adr.scsi_bus = bus;

		if((idp->version_num & HBA_VMASK) >= HBA_UW21_IDATA) {
			idp->ha_chan_id[0] = idp->ha_id;
			cntlrid = (int)(idp->ha_chan_id[bus]);
		} else {
			cntlrid = (int)(idp->ha_id);
		}

		scsi_adr.scsi_target = cntlrid;
		scsi_adr.scsi_lun = 0;

		if(sdi_add_dev(idp, &scsi_adr, inq_datap, sensep) != SDI_RET_OK)
			continue;

		for (scsiid = 0; scsiid < ntargets; scsiid++) {
			int tmplun;

			if (scsiid == cntlrid) {
				/* skip the host adapter */
				continue;
			}
			scsi_adr.scsi_target = scsiid;

			/* If something is there, it must respond at LUN 0.  */
			scsi_adr.scsi_lun = 0;
			if (sdi_check_dev(&scsi_adr, inq_datap, sensep) == SDI_INQ_UNSUPPORTED)
				continue;

			ndev++;
			bus_count++;	/* Something hanging off this bus */

			if((tmplun = sdi_device_validate(inq_datap))
			    != SDI_RET_ERR) {
				/* Ill-behaved device */
				nluns = min(tmplun, vluns);
			} else if (inq_datap->id_ver & 0x03) {
				/* SCSI-3 compliant */
				nluns = min(MAX_EXLUS, vluns);
			} else if (inq_datap->id_ver & 0x02) {
				/* SCSI-2 compliant */
				nluns = min(MAX_LUS, vluns);
			} else {
				nluns = vluns;
			}

			for (lun = 0; lun < nluns; lun++) {
				scsi_adr.scsi_lun = lun;

				/*
				 * Check if the boot parameters have made this
				 * a forbidden LUN.
				 */
				if (!sdi_lun_validate(sdi_lunsearch, &scsi_adr))
					break;

				if(sdi_add_dev(idp, &scsi_adr, inq_datap, 
					sensep) == SDI_RET_OK)
					ndev++;
			}
		}
		if (!bus_count && 
		    ((infop->hba_flag == NULL) ||
		    !(*(infop->hba_flag) & HBA_HOT)))
		{
			scsi_adr.scsi_target = cntlrid;
			scsi_adr.scsi_lun = 0;

			if (sdi_delete_edt(sdi_rxedt(scsi_adr.scsi_ctl,
						     scsi_adr.scsi_bus,
						     scsi_adr.scsi_target,
						     scsi_adr.scsi_lun))
			    != SDI_RET_OK)
			{
				ASSERT(bus_count);
			}
			
		}
		else
		{
			ndev++;
		}
		
	}
	return(ndev);
}

/*
 * void
 * sdi_ultoh(char *output, ulong number, size_t bytes)
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_ultoh(char *output, ulong number, size_t bytes)
{
	char digit;

	bytes *= 2;

	while ( bytes-- )
		*output++ = '0';

	do {
		digit = number % 16;
		*(--output) = digit > 9 ? digit + '7' : digit + '0';
	} while ( number /= 16 );
}

/*
 * char *
 * sdi_comp_str(ulong comp_code)
 *
 * Calling/Exit State:
 *	None.
 */
char *
sdi_comp_str(ulong comp_code)
{
	register int index;

	for (index = 0; index < COMP_CODE_MSG_COUNT; index++) {
		if ( comp_code == CompCodeTable[index].CompCode )
			return(CompCodeTable[index].Msg);
	}

	sdi_ultoh(&CompCodeUnk.Msg[CompCodeUnk.CompCode],\
		comp_code,sizeof(comp_code));
	return(CompCodeUnk.Msg);
}

/*
 * char *
 * sdi_status_str(uchar_t status_code)
 *
 * Calling/Exit State:
 *	None.
 */
char *
sdi_status_str(uchar_t status_code)
{
	register int index;

	for (index = 0; index < STATUS_MSG_COUNT; index++) {
		if ( status_code == StatusTable[index].Status )
			return(StatusTable[index].Msg);
	}

	sdi_ultoh(&StatusUnk.Msg[StatusUnk.Status],\
		(ulong)status_code,sizeof(status_code));
	return(StatusUnk.Msg);
}

/*
 * char *
 * sdi_sense_str(uchar_t sense_key)
 *
 * Calling/Exit State:
 *	None.
 */
char *
sdi_sense_str(uchar_t sense_key)
{
	register int index;

	for (index = 0; index < SENSE_KEY_MSG_COUNT; index++) {
		if ( sense_key == SenseKeyTable[index].Key )
			return(SenseKeyTable[index].Msg);
	}

	sdi_ultoh(&SenseKeyUnk.Msg[SenseKeyUnk.Key],\
		(ulong)sense_key,sizeof(sense_key));
	return(SenseKeyUnk.Msg);
}

/*
 * char *
 * sdi_ext_sense_str(uchar_t sense_key,uchar_t qualifier)
 *
 * Calling/Exit State:
 *	None.
 */
char *
sdi_ext_sense_str(uchar_t sense_key,uchar_t qualifier)
{
	register int index;

	for (index = 0; index < EXT_SENSE_KEY_MSG_COUNT; index++) {
		if (( sense_key == ExtSenseKeyTable[index].Key ) &&
		    ( qualifier == ExtSenseKeyTable[index].Qual )) {
			return(ExtSenseKeyTable[index].Msg);
		}
	}

	sdi_ultoh(&ExtSenseKeyUnk.Msg[ExtSenseKeyUnk.Key],\
		(ulong)sense_key,sizeof(sense_key));
	sdi_ultoh(&ExtSenseKeyUnk.Msg[ExtSenseKeyUnk.Qual],\
		(ulong)qualifier,sizeof(qualifier));
	return(ExtSenseKeyUnk.Msg);
}

/*
 * void
 * sdi_errmsg(char *dev_id, struct scsi_ad *addr, struct sb *sbp,
 * 		struct sense *sense, int type, int err_code)
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_errmsg(char *dev_id, struct scsi_ad *addr, struct sb *sbp,
		struct sense *sense, int type, int err_code)
{
	char name[SDI_NAMESZ];

	sdi_xname(addr->pdi_adr_version, addr, name);

	switch (type) {
	case SDI_SFB_ERR:
		if (err_code == 0) {
			/*
			 *+ Error message from target driver
			 */
			cmn_err(CE_WARN, "%s Driver: %s LU %d - I/O ERROR:",
				dev_id, name, sbp->SFB.sf_dev.sa_lun);
		} else {
			/*
			 *+ Error message from target driver
			 */
			cmn_err(CE_WARN, "%s Driver: %s LU %d - I/O ERROR 0x%x:",
				dev_id, name, sbp->SFB.sf_dev.sa_lun, err_code);
		}
		/*
		 *+ Error message from target driver
		 */
		cmn_err(CE_CONT, "Completion code indicates \"%s\"\n",
				sdi_comp_str(sbp->SFB.sf_comp_code));
		break;
	case SDI_CKCON_ERR:
		/*
		 *	Put recovered errors and not ready errors to putbuf
		 */
		if ( sense->sd_key == SD_RECOVER || sense->sd_key == SD_NREADY ) 
		{
			if (err_code == 0) {
				/*
				 *+ Error message from target driver
				 */
				cmn_err(CE_WARN, "!%s Driver: %s LU %d - %s:",
					dev_id, name, sbp->SCB.sc_dev.sa_lun,
					sdi_status_str(sbp->SCB.sc_status));
			} else {
				/*
				 *+ Error message from target driver
				 */
				cmn_err(CE_WARN,
				"!%s Driver: %s LU %d - %s 0x%x:",
					dev_id, name, sbp->SCB.sc_dev.sa_lun,
					sdi_status_str(sbp->SCB.sc_status), err_code);
			}
			if ( sense->sd_key != SD_NOSENSE ) {
				/*
				 *+ Error message from target driver
				 */
				cmn_err(CE_CONT,
				"!A \"%s\" condition has been detected.\n", 
					sdi_sense_str(sense->sd_key));
				if ((sense->sd_sencode!=SC_NOSENSE) ||
					(sense->sd_qualifier!=SC_NOSENSE)) {
					cmn_err(CE_CONT,
					"!Additional data = \"%s\".\n",
					sdi_ext_sense_str(
					sense->sd_sencode,sense->sd_qualifier));
				}
			}

			if (sense->sd_valid)
			{
				/*
				 *+ Error message from target driver
				 */
				cmn_err(CE_CONT,
				"!Logical block address = 0x%x\n",
				  sense->sd_ba);
			}

		} else {	/* put messages on the console */

			if (err_code == 0) {
				/*
				 *+ Error message from target driver
				 */
				cmn_err(CE_WARN, "%s Driver: %s LU %d - %s:",
					dev_id, name, sbp->SCB.sc_dev.sa_lun,
					sdi_status_str(sbp->SCB.sc_status));
			} else {
				/*
				 *+ Error message from target driver
				 */
				cmn_err(CE_WARN,
				"%s Driver: %s LU %d - %s 0x%x:",
					dev_id, name, sbp->SCB.sc_dev.sa_lun,
					sdi_status_str(sbp->SCB.sc_status),
					err_code);
			}
			if ( sense->sd_key != SD_NOSENSE ) {
				/*
				 *+ Error message from target driver
				 */
				cmn_err(CE_CONT,
				"A \"%s\" condition has been detected.\n", 
					sdi_sense_str(sense->sd_key));
				if ((sense->sd_sencode!=SC_NOSENSE) ||
					(sense->sd_qualifier!=SC_NOSENSE)) {
					/*
					 *+ Error message from target driver
					 */
					cmn_err(CE_CONT,
					"Additional data = \"%s\".\n",
					sdi_ext_sense_str(
					sense->sd_sencode,sense->sd_qualifier));
				}
			}

			if (sense->sd_valid)
			{
				/*
				 *+ Error message from target driver
				 */
				cmn_err(CE_CONT,
				"Logical block address = 0x%x\n", sense->sd_ba);
			}
		}
		break;
	case SDI_CKSTAT_ERR:
		if (err_code == 0) {
			/*
			 *+ Error message from target driver
			 */
			cmn_err(CE_WARN, "%s Driver: %s LU %d - I/O ERROR:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun);
		} else {
			/*
			 *+ Error message from target driver
			 */
			cmn_err(CE_WARN,
			"%s Driver: %s LU %d - I/O ERROR 0x%x:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun, err_code);
		}
		/*
		 *+ Error message from target driver
		 */
		cmn_err(CE_CONT, "Target status - \"%s\"\n",
			sdi_status_str(sbp->SCB.sc_status));
		break;
	case SDI_DEFAULT_ERR:
		if (err_code == 0) {
			/*
			 *+ Error message from target driver
			 */
			cmn_err(CE_WARN, "%s Driver: %s LU %d - I/O ERROR:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun);
		} else {
			/*
			 *+ Error message from target driver
			 */
			cmn_err(CE_WARN,
			"%s Driver: %s LU %d - I/O ERROR 0x%x:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun, err_code);
		}
		if (sense->sd_valid)
		{
			/*
			 *+ Error message from target driver
			 */
			cmn_err(CE_CONT,
			"Logical block address = 0x%x\n", sense->sd_ba);
		}
		/*
		 *+ Error message from target driver
		 */
		cmn_err(CE_CONT, "Completion code indicates \"%s\"\n",
				sdi_comp_str(sbp->SCB.sc_comp_code));
		break;
	}

	return;
}

/*
 * Structure used to pass info between sdi_physio_start and its iodone
 * routine, sdi_physio_iodone, and the sdi_physio_strat strategy routine.
 */
struct sdi_physio_save {
	void	*o_misc;	/* original bp->b_misc */
	void	(*o_iodone)();	/* original bp->b_iodone */
	void	(*o_strat)();	/* original strategy routine */
	uint_t	o_bcount;	/* original bp->b_bcount */
	buf_t	*o_bp;		/* original bp */
	buf_t	*child_bp;	/* "child" bp */
};

/*
 * void
 * sdi_physio_iodone(buf_t *bp)
 *	I/O completion routine for sdi_physio_start.
 *
 * Calling/Exit State:
 *	Called at interrupt level with no locks held.
 */
void
sdi_physio_iodone(buf_t *bp)
{
	struct sdi_physio_save *savep = bp->b_misc;

	ASSERT(bp == savep->o_bp);

	if ((savep->child_bp != NULL) && (bp->b_addrtype != 0)) {
		bp = savep->child_bp;
		savep->child_bp = NULL;
		biodone(bp);
		return;
	}

	/*
	 * b_resid from the driver is invalid.
	 * Deduce a reasonable value heuristically.
	 */
	if ((bp->b_flags & B_ERROR))
		bp->b_resid = savep->o_bcount;
	else
		bp->b_resid = 0;

	bp->b_misc = savep->o_misc;
	bp->b_iodone = savep->o_iodone;
	biodone(bp);
}

/*
 * STATIC void
 * sdi_physio_strat2(buf_t *bp)
 *	2nd-level I/O strategy routine for sdi_physio_start.
 *
 * Calling/Exit State:
 *	Called at interrupt level with no locks held.
 */
STATIC void
sdi_physio_strat2(buf_t *bp)
{
	struct sdi_physio_save *savep = bp->b_priv2.un_ptr;

	if (bp != savep->o_bp) {
		ASSERT(savep->child_bp == NULL);
		savep->child_bp = bp;
	}

	(*savep->o_strat)(bp);
}

/*
 * STATIC void
 * sdi_physio_strat1(buf_t *bp)
 *	I/O strategy routine for sdi_physio_start.
 *
 * Calling/Exit State:
 *	Called at interrupt level with no locks held.
 */
STATIC void
sdi_physio_strat1(buf_t *bp)
{
	static bcb_t *bcbp;
#ifndef NO_RDMA
	extern bcb_t rdma_dflt_bcb;
#endif

	ASSERT(bp == ((struct sdi_physio_save *)bp->b_priv2.un_ptr)->o_bp);

	if (bcbp == NULL) {
		bcbp = bcb_alloc(KM_SLEEP);
		bcbp->bcb_addrtypes = BA_KVIRT;
		bcbp->bcb_flags = BCB_ONE_PIECE|BCB_PHYSCONTIG;
		bcbp->bcb_granularity = 1;
		bcbp->bcb_physreqp = physreq_alloc(KM_SLEEP);
#ifndef NO_RDMA
		*bcbp->bcb_physreqp = *rdma_dflt_bcb.bcb_physreqp;
#endif
		bcbp->bcb_physreqp->phys_align = 2;
		physreq_prep(bcbp->bcb_physreqp, KM_SLEEP);
	}

	buf_breakup(sdi_physio_strat2, bp, bcbp);
}

/*
 * void (*sdi_physio_start(buf_t *bp, void (*strat)(buf_t *)))(buf_t *)
 *	Special filter to handle pass-thru for old HBA drivers.
 *
 * Calling/Exit State:
 *	This routine may block, so no locks may be held on entry.
 *
 * Description:
 *	Pre-SVR4.2MP HBA drivers behave incorrectly in pass-thru mode,
 *	in several ways:
 *
 *		* They do not set b_resid to a proper value before returning
 *		  from their strategy routines.
 *
 *		* They save a pointer to the original bp before calling
 *		  physiock, and call biodone on that instead of the bp
 *		  passed to their strategy routines.
 *
 *		* They do not use appropriate breakup routines to ensure
 *		  the buffer(s) received in their strategy routines
 *		  conform to their expectations.  In particular, dcd
 *		  assumes the buffer is delivered in one piece.
 *
 *	Through a combination of a worst-case buf_breakup call, and special
 *	iodone processing, this routine provides workarounds for these
 *	problems.  It is only called when old HBAs call physiock.
 */
STATIC
void (*sdi_physio_start(buf_t *bp, void (*strat)(buf_t *)))(buf_t *)
{
	struct sdi_physio_save *savep;

	savep = kmem_alloc(sizeof *savep, KM_SLEEP);
	savep->o_misc = bp->b_misc;
	bp->b_misc = bp->b_priv2.un_ptr = savep;
	savep->o_iodone = bp->b_iodone;
	bp->b_iodone = sdi_physio_iodone;
	savep->o_strat = strat;
	savep->o_bcount = bp->b_bcount;
	savep->o_bp = bp;
	savep->child_bp = NULL;
	return sdi_physio_strat1;
}


struct sdi_event *
sdi_event_alloc(int sleepflag)
{
	return (struct sdi_event *)kmem_zalloc(sizeof(struct sdi_event),
								sleepflag);
}


void
sdi_event_free(struct sdi_event *event)
{
	kmem_free(event, sizeof(struct sdi_event));
}


int
sdi_addevent(struct sdi_event *event)
{
	int			pdtype;
	struct sdi_event_list	*ep;


	pdtype = event->event_pdtype;
	if (pdtype > ID_COMMUNICATION || pdtype < 0)
		return SDI_RET_ERR;

	if ((ep = (struct sdi_event_list *)
		kmem_zalloc(sizeof(struct sdi_event_list), sdi_sleepflag)) ==
			(struct sdi_event_list *)NULL)	{

		cmn_err(CE_WARN, "sdi_addevent: failed to allocate event structure.\n");
		return SDI_RET_ERR;
	}

	bcopy((caddr_t)event, (caddr_t)&ep->event, sizeof(struct sdi_event));

	sdi_event_pl = LOCK(sdi_event_mutex, pldisk);
	while (sdi_event_usecnt > 0)	{
		SV_WAIT(sdi_event_sv, pridisk, sdi_event_mutex);
		sdi_event_pl = LOCK(sdi_event_mutex, pldisk);
	}

	ep->next = sdi_events[pdtype];
	sdi_events[pdtype] = ep;

	UNLOCK(sdi_event_mutex, sdi_event_pl);

	return SDI_RET_OK;
}


STATIC int
sdi_matchevent(struct sdi_event *ep, int event, int pdtype, char inq[], struct scsi_adr *sap)
{
	if (ep->event_type != event ||
		ep->event_pdtype != pdtype ||
		(inq && ep->event_inquiry[0] &&
			strncmp(ep->event_inquiry, inq, INQ_EXLEN)) ||
		(sap && ((ep->event_scsi_adr.scsi_ctl != -1 &&
				ep->event_scsi_adr.scsi_ctl != sap->scsi_ctl) ||
			(ep->event_scsi_adr.scsi_bus != -1 &&
				ep->event_scsi_adr.scsi_bus != sap->scsi_bus) ||
			(ep->event_scsi_adr.scsi_target != -1 &&
			ep->event_scsi_adr.scsi_target != sap->scsi_target) ||
			(ep->event_scsi_adr.scsi_lun != -1 &&
				ep->event_scsi_adr.scsi_lun != sap->scsi_lun))))
		return B_FALSE;

	return B_TRUE;
}


STATIC int
sdi_sameevent(struct sdi_event *e1, struct sdi_event *e2)
{
	return sdi_matchevent(e1, e2->event_type, e2->event_pdtype,
				e2->event_inquiry, &e2->event_scsi_adr);
}


int
sdi_rmevent(struct sdi_event *event)
{
	int			pdtype;
	struct sdi_event_list	*ep, **epp;


	pdtype = event->event_pdtype;
	if (pdtype > ID_COMMUNICATION || pdtype < 0)
		return SDI_RET_ERR;

	sdi_event_pl = LOCK(sdi_event_mutex, pldisk);
	while (sdi_event_usecnt > 0)	{
		SV_WAIT(sdi_event_sv, pridisk, sdi_event_mutex);
		sdi_event_pl = LOCK(sdi_event_mutex, pldisk);
	}

	for (epp = &sdi_events[pdtype], ep = *epp; ep; ep = ep->next)	{
		if (sdi_sameevent(&ep->event, event))	{
			*epp = ep->next;
			kmem_free(ep, sizeof(struct sdi_event_list));

			UNLOCK(sdi_event_mutex, sdi_event_pl);

			return SDI_RET_OK;
		}
		epp = &ep->next;
	}

	UNLOCK(sdi_event_mutex, sdi_event_pl);

	return SDI_RET_ERR;
}


int
sdi_notifyevent(int event, struct scsi_adr *sap, struct sb *sbp)
{
	int	rc, err;
	struct sdi_edt	*edtp;
	struct sdi_event_list	*ep;


	if ((edtp = sdi_rxedt_l(sap)) == (struct sdi_edt *)NULL ||
					edtp->pdtype > ID_COMMUNICATION)

		return SDI_RET_ERR;	/* This shouldn't happen? */


	err = SDI_RET_OK;

	sdi_event_pl = LOCK(sdi_event_mutex, pldisk);
	sdi_event_usecnt++;
	UNLOCK(sdi_event_mutex, sdi_event_pl);

	for (ep = sdi_events[edtp->pdtype]; ep; ep = ep->next)	{
		if (sdi_matchevent(&ep->event, event, edtp->pdtype,
				edtp->inquiry, &edtp->scsi_adr))	{
			rc = (*ep->event.event_handler)(&ep->event,
							&edtp->scsi_adr, sbp);
			if (rc != SDI_RET_OK)
				err = rc;
		}
	}

	sdi_event_pl = LOCK(sdi_event_mutex, pldisk);
	sdi_event_usecnt--;
	UNLOCK(sdi_event_mutex, sdi_event_pl);
	
	SV_BROADCAST(sdi_event_sv, 0);

	return err;
}

/*
 * int
 * sdi_bootctl(void)
 *	Returns TRUE if an HBA registered with its idata_ctlorder
 *	field set to one, indicating the HBA is claiming to be the
 *	boot controller.  sdi_ctlorder will be set to the lowest 
 *	number HBA controller order.
 *	Otherwise, FALSE.
 *
 * Calling/Exit State:
 *	None.
 */

int
sdi_bootctl(void)
{
	if (sdi_ctlorder == 1)
		return TRUE;
	return FALSE;
}

/*
 * Given the external index of a controller (as in c0b0t0d0s0)
 * returns the edt index.
 * returns -1 if the index is invalid.
 */
int
sdi_edtindex(int index)
{
	int i;

	if (index < 0) return -1;

	/* check if index has been remapped */
	for (i=0; i < (sizeof(HBA_map) / sizeof(int)); i++)
	{
		if (HBA_map[i] == index)
			return i;
	}

	/* index has not been remapped,
	 * check if it's location has already been used */
	if (HBA_map[index] == -1)
		return index;
	else
		return -1;
}

/* Return the address of the struct sense related to this sb */
struct sense *   
sdi_sense_ptr(struct sb *sbp)
{
	return &((struct sb_extra *)sbp->SCB.sc_extra)->sb_sense;
}

ulong
sdi_data_type(struct sb *sbp)
{
	 return ((struct sb_extra *)sbp->SCB.sc_extra)->sb_data_type;
}

paddr32_t *
sdi_datapt_ptr(struct sb *sbp)
{
        return ((struct sb_extra *)sbp->SCB.sc_extra)->sb_datapt;
}

buf_t *
sdi_buf_ptr(struct sb *sbp)
{
        return ((struct sb_extra *)sbp->SCB.sc_extra)->sb_bufp;
}

void *
sdi_get_idata_ptr(struct sb *sbp)
{
        return ((struct sb_extra *)sbp->SCB.sc_extra)->sb_idatap;
}

void
sdi_set_idata_ptr(struct sb *sbp, void *idatap)
{
        ((struct sb_extra *)sbp->SCB.sc_extra)->sb_idatap = idatap;
}

/*
 * void *
 * sdi_kmem_alloc_phys_nosleep(size_t , const physreq_t *)
 *
 * This is hacked up version of sdi_kmem_alloc_phys which will not sleep
 * and will fail if the memory is not available.
 *
 * Restrictions
 * It is only used by sd01alloc_wkamp.
 */
void *
sdi_kmem_alloc_phys_nosleep(size_t size, const physreq_t *preqp)
{
	void *mem;
	paddr32_t *paddrp;
	size_t paddr_size;

	if (preqp->phys_dmasize == 0) {
		mem = kmem_zalloc(size, KM_NOSLEEP);
		return (mem);
	}

	paddr_size = SDI_PADDR_SIZE(preqp);
#ifdef SDI_PHYS_DEBUG
	cmn_err(CE_NOTE, "NEW_SIZE: %d", paddr_size);
#endif
	paddrp = (paddr32_t *)kmem_zalloc(paddr_size, KM_NOSLEEP);
	if (paddrp == NULL)
		return NULL;

	/*
	 * The KM_FAIL_OK flag is not a standard interface, hence it may
	 * brake in future releases.
	 */
	mem = kmem_alloc_phys(size+paddr_size, preqp, paddrp, KM_FAIL_OK);

	if (mem) {
		bzero(mem, size+paddr_size);
		bcopy((char *)paddrp, (char *)mem+size, paddr_size);
	}

	kmem_free((char *)paddrp, paddr_size);
	paddrp = (paddr32_t *)((char *)mem + size);
#ifdef SDI_PHYS_DEBUG
	cmn_err(CE_NOTE, "paddrp: 0x%x", *paddrp); 
#endif
	return mem;
}

void *
sdi_kmem_alloc_phys(size_t size, const physreq_t *preqp, int flags)
{
	void *mem;
	paddr32_t *paddrp;
	size_t paddr_size;

	if (preqp->phys_dmasize == 0) {
		mem = kmem_zalloc(size, KM_SLEEP);
		return (mem);
	}

	paddr_size = SDI_PADDR_SIZE(preqp);
#ifdef SDI_PHYS_DEBUG
	cmn_err(CE_NOTE, "NEW_SIZE: %d", paddr_size);
#endif
	paddrp = (paddr32_t *)kmem_zalloc(paddr_size, KM_SLEEP);

	mem = kmem_alloc_phys(size+paddr_size, preqp, paddrp, flags);

	if (mem) {
		bzero(mem, size+paddr_size);
		bcopy((char *)paddrp, (char *)mem+size, paddr_size);
	}

	kmem_free((char *)paddrp, paddr_size);
	paddrp = (paddr32_t *)((char *)mem + size);
#ifdef SDI_PHYS_DEBUG
	cmn_err(CE_NOTE, "paddrp: 0x%x", *paddrp); 
#endif
	return mem;
}


/*
 * int
 * sdi_hba_flag(int c)
 *	Returns the HBA_FLAG for the HBA driver attached to the
 *      specified controller. 
 */
int
sdi_hba_flag(int c)
{
	if ((c < 0) ||
	    (c >= sdi_hacnt) ||
	    (HBA_tbl[c].info == NULL) ||
	    (HIP(HBA_tbl[c].info)->hba_flag == NULL))
		return 0;
	else
		return *(HIP(HBA_tbl[c].info)->hba_flag);
}

/*
 * int
 * sdi_hba_version(int c)
 *	returns the idata version number of the HBA driver attached
 *      to the specified controller.
 */
int
sdi_hba_version(int c)
{
	if ((c < 0) ||
	    (c >= sdi_hacnt) ||
	    (HBA_tbl[c].idata == NULL))
		return 0;
	else
		return IDP(HBA_tbl[c].idata)->version_num;
}

/*
 * int sdi_ext_address(int c)
 *	returns 1 if the HBA is using the expanded addressing scheme
 */
int
sdi_ext_address(int c)
{
	if ((c < 0) ||
            (c >= sdi_hacnt) ||
            (HBA_tbl[c].idata == NULL))
                return 0;
        else
		return	(IDP(HBA_tbl[c].idata)->version_num & HBA_EXT_ADDRESS);
}

/* Timeout/Reset Support Routines */

/* sdi gauntlet structures */

struct sdi_gauntlet {
	struct sdi_gauntlet 	*next;
	struct sdi_gauntlet 	*prev;
	struct sdi_gauntlet_job *first_job;
	struct sdi_gauntlet_job *last_job;
	struct scsi_adr 	 sa;
	int 	   		 flag;
	void		       (*intr)();
	int			 reset_status;
};

/* Definitions for flag above */

#define GAUNTLET_ACTIVE		01
#define GAUNTLET_BUS_RESET 	02
#define GAUNTLET_DEV_RESET 	04

STATIC struct sdi_gauntlet *sdi_gauntlet_head, *sdi_gauntlet_tail;
STATIC void sdi_reset_timer(struct sb *sbp);
STATIC void sdi_reset_intr(struct sb *sbp);
	
struct sdi_gauntlet_job {
	struct sb *sbp;
	void      (*func)();
	struct sdi_gauntlet_job *next;
};

STATIC lock_t	*sdi_gauntlet_lock;
STATIC pl_t	 sdi_gauntlet_pl;
LKINFO_DECL( sdi_gauntlet_lkinfo, "IO:sdi:sdi_gauntlet_lkinfo", 0);

#define SDI_LOCK_GAUNTLET()   sdi_gauntlet_pl = LOCK(sdi_gauntlet_lock, pldisk)
#define SDI_UNLOCK_GAUNTLET() UNLOCK(sdi_gauntlet_lock, sdi_gauntlet_pl)

STATIC void
sdi_gauntlet_init()
{
	sdi_gauntlet_lock = 
	    LOCK_ALLOC(SDI_HIER_BASE, pldisk, &sdi_gauntlet_lkinfo, KM_NOSLEEP); 
	if (!sdi_gauntlet_lock)
		/*
		 *+ Allocation of sdi data structures failed at
		 *+ boot time, when there should be plenty of
		 *+ memory.  Check system configuration.
		 */
		cmn_err(CE_PANIC, "sdi_init: cannot allocate data structures");
}

STATIC struct sdi_gauntlet *
sdi_find_gauntlet(struct scsi_adr *sap, int check_target, int active )
{
	struct sdi_gauntlet *gp;
	for (gp = sdi_gauntlet_head; gp; gp = gp->next) {
		if (gp->sa.scsi_ctl == sap->scsi_ctl && 
		    gp->sa.scsi_bus == sap->scsi_bus) {
			if (active) {
				if (gp->flag & GAUNTLET_ACTIVE)
					break;
				continue;
			}
			if (!check_target)
				break;
			if (gp->sa.scsi_target == sap->scsi_target &&
			    gp->sa.scsi_lun == sap->scsi_lun) 
				break;
		}
	}
	return gp;
}

STATIC void
sdi_free_gauntlet(struct sdi_gauntlet *gp)
{
	if (gp->prev && gp->next) {
		gp->next->prev = gp->prev;
		gp->prev->next = gp->next;
	} else {
		if (!gp->prev) {
			sdi_gauntlet_head = gp->next;
			if (sdi_gauntlet_head)
				sdi_gauntlet_head->prev = NULL;
		}
		if (!gp->next) {
			sdi_gauntlet_tail = gp->prev;
			if (sdi_gauntlet_tail)
				sdi_gauntlet_tail->next = NULL;
		}
	}
	kmem_free(gp, sizeof(struct sdi_gauntlet));
}

STATIC struct sdi_gauntlet *
sdi_alloc_gauntlet(struct scsi_adr *sap)
{
	struct sdi_gauntlet *gp;

	gp = kmem_alloc(sizeof(struct sdi_gauntlet), KM_NOSLEEP);
	if (!gp)
		return NULL;

	gp->sa = *sap;
	gp->next = NULL;
	gp->first_job = gp->last_job = NULL;
	gp->flag = 0;
	gp->reset_status = 0;

	if (!sdi_gauntlet_head) {
		sdi_gauntlet_head = sdi_gauntlet_tail = gp;
		gp->prev = NULL;
	} else {
		gp->prev = sdi_gauntlet_tail;
		sdi_gauntlet_tail->next = gp;
		sdi_gauntlet_tail = gp;
	}
	return gp;
}

/* sdi_device_reset:
 *
 *      Called to issue a bus device reset.  The caller must be in the
 *      reset gauntlet.  No locks are required on entry to this function.
 *	The sf_dev field is assumed to be set upon entry.
 */
int
sdi_device_reset(struct scsi_adr *sap, struct sb *sbp)
{
	int lun;
	struct scsi_adr nsap = *sap;
	struct sdi_gauntlet *gp;
	int ret;
	int pdi_adr_version;

	cmn_err(CE_CONT, "sdi: Device Reset\n");

	for (lun=0; lun<MAX_EXLUS; lun++) {
		if (sdi_rxedt(sap->scsi_ctl, sap->scsi_bus, sap->scsi_target, lun)) {
			nsap.scsi_lun = lun;
			sdi_dofunc(&nsap, SFB_SUSPEND, KM_NOSLEEP);
		}
	}

	pdi_adr_version = sdi_ext_address(sap->scsi_ctl);

	sbp->sb_type = SFB_TYPE;
        sbp->SFB.sf_func = SFB_RESET_DEVICE;
	SDI_LOCK_GAUNTLET();

	gp = sdi_find_gauntlet(sap, 1, 0);
	gp->flag |= GAUNTLET_DEV_RESET;
	gp->flag &= ~GAUNTLET_BUS_RESET;
	gp->intr  = sbp->SFB.sf_int;
	gp->reset_status = 0;
	sbp->SFB.sf_int = sdi_reset_intr;
	SDI_UNLOCK_GAUNTLET();
        ret = sdi_xicmd(pdi_adr_version, sbp, sdi_sleepflag);
	if (ret == SDI_RET_OK)
		sdi_timeout(sdi_reset_timer, sbp, drv_usectohz(4000000), pldisk, &sbp->SFB.sf_dev);
	return ret;
}

/* sdi_bus_reset:
 *
 *      Called to issue a SCSI bus reset.  The caller must be in the
 *      reset gauntlet.  No locks are required on entry to this function.
 *      All devices on the bus are suspended.  The bus reset is sent
 *	to the controller/bus, not to device.
 *	The sf_dev field is not assumed to be set upon entry.
 */
int
sdi_bus_reset(struct scsi_adr *sap, struct sb *sbp)
{
	int controller = sap->scsi_ctl;
	int bus = sap->scsi_bus;
	int target, lun, ret;
	struct sdi_gauntlet *gp;
	struct scsi_adr nsap = *sap;
	int pdi_adr_version;

	struct hba_cfg *hbap = &HBA_tbl[controller];

	cmn_err(CE_CONT, "sdi: Bus Reset\n");

        /* Suspend all devices on bus */

	for (target=0; target<MAX_EXTCS; target++) {
		for (lun=0; lun<MAX_EXLUS; lun++) {
			if (sdi_rxedt(controller, bus, target, lun)) {
				nsap.scsi_target = target;
				nsap.scsi_lun = lun;
				sdi_dofunc(&nsap, SFB_SUSPEND, KM_SLEEP);
			}
		}
	}
	
	pdi_adr_version = sdi_ext_address(sap->scsi_ctl);

	sbp->sb_type = SFB_TYPE;
        sbp->SFB.sf_func = SFB_RESET_BUS;
	SDI_LOCK_GAUNTLET();

	gp = sdi_find_gauntlet(sap, 0, 1);
	gp->flag |= GAUNTLET_BUS_RESET;
	gp->flag &= ~GAUNTLET_DEV_RESET;
	gp->intr  = sbp->SFB.sf_int;
	gp->reset_status = 0;

	target = IDP(HBA_tbl[controller].idata)->ha_id;
	if (! pdi_adr_version ) {
		sbp->SFB.sf_dev.sa_ct = SDI_SA_CT(controller, target);
		sbp->SFB.sf_dev.sa_exta = target;
		sbp->SFB.sf_dev.sa_lun  = 0;
		sbp->SFB.sf_dev.sa_bus  = bus;
	}
	else {
		sbp->SFB.sf_dev.extended_address->scsi_adr.scsi_ctl = 
					controller;
		sbp->SFB.sf_dev.extended_address->scsi_adr.scsi_target =
					target;
		sbp->SFB.sf_dev.extended_address->scsi_adr.scsi_lun = 0;
		sbp->SFB.sf_dev.extended_address->scsi_adr.scsi_bus = bus;
	}

	sbp->SFB.sf_int = sdi_reset_intr;
	SDI_UNLOCK_GAUNTLET();
        ret = sdi_xicmd(pdi_adr_version, sbp, sdi_sleepflag);
	if (ret == SDI_RET_OK)
		sdi_timeout(sdi_reset_timer, sbp, drv_usectohz(4000000), pldisk, &sbp->SFB.sf_dev);
	return ret;
}

/* sdi_start_gauntlet:
 *
 *      Attempt to start the gauntlet on behalf of a job.  If the
 *      gauntlet has been started for any other job on the bus,
 *      this job will queued and processed when all jobs ahead of
 *      it are through the gauntlet.
 *
 *      func:      Target driver gauntlet routine.
 *
 *      The gauntlet lock is acquired for this bus to synchronize
 *      both access to the gauntlet and the job queueing.
 */
int
sdi_start_gauntlet(struct sb *sbp, void (*func)())
{
	struct sdi_gauntlet *gp;
	struct sdi_gauntlet_job *jp;
	struct scsi_adr sa;
	int completed;
	struct hba_cfg *hbap;

	if (!sbp->SCB.sc_dev.pdi_adr_version ) {
        	sa.scsi_ctl = SDI_EXHAN(&sbp->SCB.sc_dev);
        	sa.scsi_bus = SDI_BUS(&sbp->SCB.sc_dev);
        	sa.scsi_target = SDI_EXTCN(&sbp->SCB.sc_dev);
        	sa.scsi_lun = SDI_EXLUN(&sbp->SCB.sc_dev);
	}
	else 
		sa = sbp->SCB.sc_dev.extended_address->scsi_adr;

	hbap = &HBA_tbl[sa.scsi_ctl];

	if (!pdi_timeout || !hbap->active || !hbap->info->hba_flag 
	 || !(*hbap->info->hba_flag & HBA_TIMEOUT_RESET))
		return 0;

	cmn_err(CE_NOTE, "!sdi: Suspend Device");
	sdi_dofunc(&sa, SFB_SUSPEND, KM_NOSLEEP);

	SDI_LOCK_GAUNTLET();

	gp = sdi_find_gauntlet(&sa, 0, 1);

	if (gp) {
		cmn_err(CE_CONT, "sdi: Recovery Gauntlet Busy(sb:0x%x)\n", sbp);
		if (!(gp = sdi_find_gauntlet(&sa, 1, 0)))
			gp = sdi_alloc_gauntlet(&sa);
		jp = kmem_alloc(sizeof(struct sdi_gauntlet_job), KM_NOSLEEP);

		/* Check for memory failure. */
		if (!gp || !jp)
			goto bailout;

		jp->sbp  = sbp;
		jp->func = func;
		jp->next = NULL;
		if (gp->first_job)
			gp->last_job->next = jp;
		else
			gp->first_job = jp;
		gp->last_job = jp;
		SDI_UNLOCK_GAUNTLET();
		return 1;
	}

	/* Start the gauntlet */

	cmn_err(CE_CONT, "sdi: Start Recovery Gauntlet(sb:0x%x)\n", sbp);

	if (!(gp =  sdi_alloc_gauntlet(&sa)))
		goto bailout;

    SDI_UNLOCK_GAUNTLET();
    /* SDI_TIME indicates job was aborted */
    completed = sbp->SCB.sc_comp_code == SDI_TIME_NOABORT ? 0 : 1;
	gp->flag |= GAUNTLET_ACTIVE;
    (*func)(sbp, completed);
	return 1;

bailout:
	SDI_UNLOCK_GAUNTLET();
	return 0;
}

/* sdi_end_gauntlet:
 *
 *      - Announce that a job has completed the gauntlet.
 *      - If no more jobs are queued for this target, resume the device's queue.
 *      - If another job is waiting for the gauntlet, get it started, otherwise
 *        clear the gauntlet flag.
 *
 *      The gauntlet lock is acquired for this bus to synchronize
 *      both access to the gauntlet and the job queueing.
 */
void
sdi_end_gauntlet(struct scsi_adr *sap)
{

        int c = sap->scsi_ctl;
        int b = sap->scsi_bus;
        int t = sap->scsi_target;
        int l = sap->scsi_lun;
	struct sdi_gauntlet *gp;
	struct sdi_gauntlet_job *jp;
	void (*func)();
	struct sb *sbp;

        SDI_LOCK_GAUNTLET();

	gp = sdi_find_gauntlet(sap, 1, 0);

	gp->flag &= ~GAUNTLET_ACTIVE;

	if (!gp->first_job) {
		cmn_err(CE_CONT, "sdi: Device Recovery Complete\n");
		sdi_dofunc(sap, SFB_RESUME, KM_NOSLEEP);
		sdi_free_gauntlet(gp);
	} 

	/* Start next job waiting for gauntlet on this bus */

	gp = sdi_find_gauntlet(sap, 0, 0);

	if (!gp) {
		SDI_UNLOCK_GAUNTLET();
		return;
	}

	jp = gp->first_job;
        
	sbp = jp->sbp;
	func = jp->func;
	if (gp->last_job == jp)
		gp->first_job = gp->last_job = NULL;
	else
		gp->first_job = jp->next;

	kmem_free(jp, sizeof(struct sdi_gauntlet_job));
	gp->flag |= GAUNTLET_ACTIVE;

	SDI_UNLOCK_GAUNTLET();
	
	cmn_err(CE_CONT, "sdi: Continue Recovery Gauntlet(sb:0x%x)\n", sbp);
        (*func)(sbp,1);
}

/* sdi_reset_intr:
 *
 * Mark reset job as having interrupted.
 * 
 */
STATIC void
sdi_reset_intr(struct sb *sbp)
{
	struct scsi_adr sa;
	struct sdi_gauntlet *gp;

	if ( ! sbp->SFB.sf_dev.pdi_adr_version ) {
        	sa.scsi_ctl = SDI_EXHAN(&sbp->SFB.sf_dev);
        	sa.scsi_bus = SDI_BUS(&sbp->SFB.sf_dev);
        	sa.scsi_target = SDI_EXTCN(&sbp->SFB.sf_dev);
        	sa.scsi_lun = SDI_EXLUN(&sbp->SFB.sf_dev);
	}
	else
		sa = sbp->SFB.sf_dev.extended_address->scsi_adr;

	SDI_LOCK_GAUNTLET();
	if ((gp = sdi_find_gauntlet(&sa, 0, 1)) != NULL)
		gp->reset_status++;
	SDI_UNLOCK_GAUNTLET();
}

/* sdi_reset_timer:
 *
 * Complete reset operation
 *
 */
STATIC void
sdi_reset_timer(struct sb *sbp)
{
	struct scsi_adr sa;
	struct sdi_gauntlet *gp;
	int controller, bus, target, lun;

	if ( ! sbp->SFB.sf_dev.pdi_adr_version ) {
        	sa.scsi_ctl = controller = SDI_EXHAN(&sbp->SFB.sf_dev);
		sa.scsi_bus = bus = SDI_BUS(&sbp->SFB.sf_dev);
        	sa.scsi_target = SDI_EXTCN(&sbp->SFB.sf_dev);
        	sa.scsi_lun = SDI_EXLUN(&sbp->SFB.sf_dev);
	}
	else {
		sa = sbp->SFB.sf_dev.extended_address->scsi_adr;
		controller = 
			sbp->SFB.sf_dev.extended_address->scsi_adr.scsi_ctl;
		bus = sbp->SFB.sf_dev.extended_address->scsi_adr.scsi_bus;
	}

	SDI_LOCK_GAUNTLET();

	gp = sdi_find_gauntlet(&sa, 0, 1);
	if (!gp->reset_status)  {
		cmn_err(CE_CONT, "Reset timed out\n");
		sbp->SCB.sc_comp_code = SDI_ERROR;
	}

	if (gp->flag & GAUNTLET_BUS_RESET) {
		for (target=0; target<MAX_EXTCS; target++) {
			for (lun=0; lun<MAX_EXLUS; lun++) {
				if (sdi_rxedt(controller, bus, target, lun)) {
					sa.scsi_target = target;
					sa.scsi_lun = lun;
					if (!sdi_find_gauntlet(&sa, 1, 0))
						sdi_dofunc(&sa, SFB_RESUME, KM_SLEEP);
				}
			}
		}
	} else {
		for (lun=0; lun<MAX_EXLUS; lun++) {
			if (sdi_rxedt(controller, bus, target, lun)) {
				sa.scsi_lun = lun;
				if (!sdi_find_gauntlet(&sa, 1, 0))
					sdi_dofunc(&sa, SFB_RESUME, KM_SLEEP);
			}
		}
	}
	SDI_UNLOCK_GAUNTLET();
	
	(gp->intr)(sbp);
}
