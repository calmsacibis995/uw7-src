#ident	"@(#)pdi.cmds:scl.c	1.14.3.1"


/*  SCSI Command Library - This file contains a library of SCSI
 *  commands that the user utilities can use. The SCSI commands
 *  are created for the user and then sent to the Target Controller
 *  via pass-through.
 */

/*  "put_string()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_NOSTD>.
 *
 *  "error()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 *
 *  "warning()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_WARNING>.
 */

#include	<stdio.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<sys/vtoc.h>		/* Included to satisfy scsicomm.h */
#include	"scsicomm.h"
#include	<sys/mkdev.h>
#include	<sys/stat.h>
#include	<sys/sdi_edt.h>
#include	<sys/scsi.h>
#include	<sys/sdi.h>
#include	"scl.h"
#include	"tokens.h"
#include	<string.h>
#include 	<signal.h>		/* used by format timer */
#include	"badsec.h"
#include	<sys/fdisk.h>
#include	<pfmt.h>

#define MARBLK	2
#define	TRUE	1
#define	FALSE	0
#define	MASK_FF	0xFF
#define	MASK_1E	0x1E
#define NORMEXIT 0

char		Cmdname[64];
char		Hostfile[128];	/* Host adapter pass thru file name */
extern int	gaugeflg;
extern int	Show;
extern int	Silent;
int		Hostfdes;
dev_t		Hostdev=0;
struct ident	Inquiry_data;
int 		Timer = 0;
int		Timerpid,		/* used by format timer */
		Stoptimer;
extern char	*malloc(),
		*realloc();
void		error(),
		warning();
extern void	qsort();
extern struct	badsec_lst *badsl_chain;
extern struct	disk_parms dp;

#ifdef SCL_DEBUG
struct	scm sx_scm;
int	badsec[] = {
0x700, 0x701, 0x730, 0x750, 
0x1200, 0x1234, 0x1235, 0x2345, 0x2346, 0x2347, 
0x4000, 0x4001, 0x4002, 0x4003, 0x4004, 0x4005, 0x4006, 0x4007,
0x4008, 0x4009, 0x400A, 0x400B, 0x400C, 0x400D, 0x400E, 0x400F, 0x4010, 
0x4050, 0x4052, 0x4054, 0x4056, 0x4058, 0x405a, 0x405c, 0x405e, 
0x4060, 0x4062, 0x4064, 0x4066, 0x4068, 0x406a, 0x406c, 0x406e, 
0x4070, 0x4072, 0x4074, 0x4076, 0x4078, 0x407a, 0x407c, 0x407e, 
0x4080, 0x4082, 0x4084, 0x4086, 0x4088, 0x408a, 0x408c, 0x408e, 
0x4090, 0x4092, 0x4094, 0x4096, 0x4098, 0x409a, 0x409c, 0x409e, 
0x40a0, 0x40a2, 0x40a4, 0x40a6, 0x40a8, 0x40aa, 0x40ac, 0x40ae, 
0x40b0, 0x40b2, 0x40b4, 0x40b6, 0x40b8, 0x40ba, 0x40bc, 0x40be, 
0xBADB, 0x10000,
0x12000, 0x12001, 0x12002, 0x12300, 0x12400, 0x12500
};
int	badsec_cnt = sizeof(badsec) / sizeof(int);
#endif


void
req_sense(sense_data)
struct sense	*sense_data;
{
	struct sb	req_sense_scb;
	struct scs	req_sense_cdb;

	if (Show)
		put_string(stderr, ":166:Request Sense");

	/* Fill in the Request Sense CDB */
	req_sense_cdb.ss_op = SS_REQSEN;
	req_sense_cdb.ss_lun = LUN(Hostdev);
	req_sense_cdb.ss_addr1 = 0;
	req_sense_cdb.ss_addr  = 0;
	req_sense_cdb.ss_len = SENSE_SZ;
	req_sense_cdb.ss_cont = 0;

	/* Fill in the Request Sense SCB */
	req_sense_scb.sb_type = ISCB_TYPE;
	req_sense_scb.SCB.sc_comp_code = SDI_PROGRES;
	req_sense_scb.SCB.sc_int = NULL;
	req_sense_scb.SCB.sc_cmdpt = SCS_AD(&req_sense_cdb);
	req_sense_scb.SCB.sc_datapt = SENSE_AD(sense_data);
	req_sense_scb.SCB.sc_wd = 0;
	req_sense_scb.SCB.sc_time = 60 * 1000;	/* 1 Minute */
	req_sense_scb.SCB.sc_mode = SCB_READ;
	req_sense_scb.SCB.sc_status = 0;
	req_sense_scb.SCB.sc_link = (struct sb *) NULL;
	req_sense_scb.SCB.sc_cmdsz = SCS_SZ;
	req_sense_scb.SCB.sc_datasz = SENSE_SZ;
	req_sense_scb.SCB.sc_resid = 0;

	if (Show) {
		put_string(stderr, ":167:SCB sent to Host Adapter");
		put_data(stderr, (char *) &req_sense_scb, sizeof(struct sb));
		put_string(stderr, ":168:CDB sent to Host Adapter");
		put_data(stderr, req_sense_scb.SCB.sc_cmdpt, req_sense_scb.SCB.sc_cmdsz);
	}
	/* Send the Request Sense SCSI Control Block to the Host Adapter */
	if (ioctl(Hostfdes, SDI_SEND, &req_sense_scb) < 0)
		error(":169:Request Sense ioctl failed\n");

	/* Check Completion Code of the job */
	if (req_sense_scb.SCB.sc_comp_code != SDI_ASW)
		error(":170:Request Sense SDI_SEND failed (0x%X)\n",
			req_sense_scb.SCB.sc_comp_code);

	if (Show) {
		put_string(stderr,
			":171:Data received from Host Adapter");
		put_data(stderr, req_sense_scb.SCB.sc_datapt, req_sense_scb.SCB.sc_datasz);
	}
}	/* req_sense() */

send_scb(scb, sense_data, errfunc)
struct sb	*scb;
struct sense	*sense_data;
void *errfunc();  /* error/warning routine to call after REQSEN data rcvd */
{
#ifdef SCL_DEBUG
	char	*scmp = ((char *)&sx_scm) + 2;
	struct	scm *smx_cdb = &sx_scm;
	int	smx_addr;
	short	smx_len;
	char	smx_op;
	int	i;
	char	*cmdpt = (char *) scb->SCB.sc_cmdpt;

	if (scb->SCB.sc_cmdsz == SCM_SZ) {
		for (i=0; i<SCM_SZ; i++)
			*scmp++ = *cmdpt++;
		smx_op = (char)smx_cdb->sm_op;
		if (smx_op == 0x2fL) {
			smx_addr = scl_swap32(smx_cdb->sm_addr);
			smx_len = (short)scl_swap16(smx_cdb->sm_len);
			for (i=0; i<badsec_cnt; i++) {
				if ((badsec[i] >= smx_addr) && 
			    	    (badsec[i] <= (smx_addr + smx_len))) {
(void) pfmt(stderr, MM_ERROR, ":172:send_scb: op(VERIFY) badsec match= 0x%x return SC_IDERR\n", badsec[i]);
					sense_data->sd_sencode = SC_IDERR;
					sense_data->sd_valid = 1;
					sense_data->sd_ba = badsec[i];
					return(sense_data->sd_sencode);
				}
			}
		}
	} else if (scb->SCB.sc_cmdsz == SCS_SZ) {
		smx_op = *cmdpt;
		if (smx_op == SS_REASGN) {
			smx_addr = ((int *)(scb->SCB.sc_datapt))[1];
			smx_addr = scl_swap32(smx_addr);
			smx_len = 0;
(void) pfmt(stderr, MM_ERROR, ":173:send_scb: op(SS_REASGN) addr= 0x%x \n", smx_addr);
			for (i=0; i<badsec_cnt; i++) {
				if ((badsec[i] >= smx_addr) && 
			    	    (badsec[i] <= (smx_addr + smx_len))) {
(void) pfmt(stderr, MM_ERROR, ":174:send_scb: badsec reassign match= 0x%x return SC_NODFCT\n", badsec[i]);
					sense_data->sd_sencode = SC_NODFCT;
					sense_data->sd_valid = 1;
					sense_data->sd_ba = badsec[i];
					return(SC_NODFCT);
				}
			}
			return(0);	/* no-op even real growing defect */
		}
	}
#endif
	/* Complete the SCSI Control Block */
	scb->sb_type = ISCB_TYPE;
	scb->SCB.sc_comp_code = SDI_PROGRES;
	scb->SCB.sc_int = NULL;
	scb->SCB.sc_wd = 0;
	scb->SCB.sc_status = 0;
	scb->SCB.sc_link = (struct sb *) NULL;
	scb->SCB.sc_resid = 0;

	if (Show) {
		put_string(stderr, ":167:SCB sent to Host Adapter");
		put_data(stderr, (char *) scb, sizeof(struct sb));
		put_string(stderr, ":168:CDB sent to Host Adapter");
		put_data(stderr, scb->SCB.sc_cmdpt, scb->SCB.sc_cmdsz);
		if ((scb->SCB.sc_datasz > 0) && (~scb->SCB.sc_mode & SCB_READ)) {
			put_string(stderr,
				":175:Data sent to Host Adapter");
			put_data(stderr, scb->SCB.sc_datapt, scb->SCB.sc_datasz);
		}
	}

	/* Send the SCSI Control Block to the Host Adapter */
	if (ioctl(Hostfdes, SDI_SEND, scb) < 0)
		error(":176:Send SCB Ioctl failed\n");

	/* Check Completion Code of the job */
	switch (scb->SCB.sc_comp_code & MASK_FF) {
	case (SDI_ASW & MASK_FF) :	/* Job completed normally           */
		break;
	case (SDI_CKSTAT & MASK_FF) :	/* Target returned check status     */
		errno = 0;

		if (Show)
			(void) pfmt(stderr, MM_ERROR,
				":177:Status: 0x%X\n", scb->SCB.sc_status);
		switch (scb->SCB.sc_status & MASK_1E) {
		case S_GOOD :	/* Job completed normally  */
			(void)(*errfunc)(":178:Good Status?\n");	/*UP*/
			break;
		case S_CKCON :	/* Error executing command */
			req_sense(sense_data);

			sense_data->sd_ba = scl_swap32(sense_data->sd_ba);

			/* Check Sense Key */

			if (Show) {
				(void) pfmt(stdout, MM_NOSTD,
					":179:send_scb(): Sense Key (0x%X)\n",
					    sense_data->sd_key);
				(void) pfmt(stdout, MM_NOSTD,
					":180:send_scb(): Sense Code (0x%X)\n",
					    sense_data->sd_sencode);
			}

			switch (sense_data->sd_key) {
			case SD_NOSENSE :
				break;
			case SD_VENUNI :
				(void)(*errfunc)(":181:Vendor Unique (0x%X)\n",
					sense_data->sd_sencode);
				break;
			case SD_RESERV :
				(void)(*errfunc)(":182:Reserved (0x%X)\n",
					sense_data->sd_sencode);
				break;
			default :
				switch (Inquiry_data.id_type) {
				case ID_RANDOM :
					switch (sense_data->sd_sencode) {
					case SC_NOSENSE :
						break;
					case SC_NOSGNL :
						(void)(*errfunc)(":183:No Index/Sector Signal\n");

						break;
					case SC_NOSEEK :
						(void)(*errfunc)(":184:No Seek Complete\n");

						break;
					case SC_WRFLT :	
						(void)(*errfunc)(":185:Write Fault\n");

						break;
					case SC_DRVNTRDY :
						(void)(*errfunc)(":186:Drive Not Ready\n");

						break;
					case SC_DRVNTSEL :
						(void)(*errfunc)(":187:Drive Not Selected\n");

						break;
					case SC_NOTRKZERO :
						(void)(*errfunc)(":188:No Track Zero found\n");

						break;
					case SC_MULTDRV :
						(void)(*errfunc)(":189:Multiple Drives Selected\n");

						break;
					case SC_LUCOMM :
						(void)(*errfunc)(":190:Logical Unit Communication Failure\n");

						break;
					case SC_TRACKERR :
						(void)(*errfunc)(":191:Track Following error\n");

						break;
					case SC_IDERR :	
					case SC_UNRECOVRRD :
					case SC_NOADDRID :
					case SC_NOADDRDATA :
					case SC_NORECORD :
					case SC_DATASYNCMK :
					case SC_RECOVRRD :
					case SC_RECOVRRDECC :
					case SC_CMPERR :
					case SC_RECOVRIDECC :
					case SC_MEDCHNG :
					case SC_RESET :	
						return(sense_data->sd_sencode);
					case SC_SEEKERR :
						(void)(*errfunc)(":192:Seek Positioning error\n");

						break;
					case SC_DFCTLSTERR :
						(void)(*errfunc)(":193:Defect List error\n");

						break;
					case SC_PARAMOVER :
						(void)(*errfunc)(":194:Paramater Overrun\n");

						break;
					case SC_SYNCTRAN :
						(void)(*errfunc)(":195:Synchronous Transfer error\n");

						break;
					case SC_NODFCTLST :
						(void)(*errfunc)(":196:Primary Defect List not found\n");

						break;
					case SC_INVOPCODE :
						(void)(*errfunc)(":197:Invalid Command Operation Code\n");

						break;
					case SC_ILLBLCK :
						(void)(*errfunc)(":198:Illegal Logical Block Address.\nAddress greater than the LBA returned by the READ CAPACITY data with PMI not set.\n");

						break;
					case SC_ILLFUNC :
						(void)(*errfunc)(":199:Illegal function for device type\n");

						break;
					case SC_ILLCDB :
						(void)(*errfunc)(":200:Illegal Field in CDB\n");

						break;
					case SC_INVLUN :
						(void)(*errfunc)(":201:Invalid LUN\n");

						break;
					case SC_INVPARAM :
						(void)(*errfunc)(":202:Invalid field in Parameter List\n");

						break;
					case SC_WRPROT :
						(void)(*errfunc)(":203:Write Protected\n");

						break;
					case SC_MDSELCHNG :
						(void)(*errfunc)(":204:Mode Select Parameters changed.\n");

						break;
					case SC_INCOMP :
						(void)(*errfunc)(":205:Incompatible Cartridge\n");

						break;
					case SC_FMTFAIL :
						(void)(*errfunc)(":206:Medium Format Corrupted\n");

						break;
					case SC_NODFCT :
						return(sense_data->sd_sencode);

					case SC_RAMFAIL :
						(void)(*errfunc)(":207:RAM Failure\n");

						break;
					case SC_DATADIAG :
						(void)(*errfunc)(":208:Data Path Diagnostic Failure\n");

						break;
					case SC_POWFAIL :
						(void)(*errfunc)(":209:Power On Diagnostic Failure\n");

						break;
					case SC_MSGREJCT :
						(void)(*errfunc)(":210:Message Reject Error\n");

						break;
					case SC_CONTRERR :
						(void)(*errfunc)(":211:Internal Controller Error\n");

						break;
					case SC_SELFAIL :
						(void)(*errfunc)(":212:Select/Reselect Failed\n");

						break;
					case SC_SOFTRESET :
						(void)(*errfunc)(":213:Unsuccessful Soft Reset\n");

						break;
					case SC_PARITY :
						(void)(*errfunc)(":214:SCSI Interface Parity Error\n");

						break;
					case SC_INITERR :
						(void)(*errfunc)(":215:Initiator Detected Error\n");

						break;
					case SC_ILLMSG :
						(void)(*errfunc)(":216:Inappropriate/Illegal Message\n");

						break;
					default :
						(void)(*errfunc)(":217:Unknown sense code (0x%X)\n", sense_data->sd_sencode);

						break;
					}
					break;
				default :
					(void)(*errfunc)(":218:Unknown device type (0x%X)\n", Inquiry_data.id_type);

					break;
				}
				break;
			}
			break;
		case S_BUSY :		/* Controller busy	   */
			(void)(*errfunc)(":219:Controller busy\n");	/*UP*/
			break;
		case S_RESER :		/* LUN Reserved		   */
			(void)(*errfunc)(":220:LUN Reserved\n");	/*UP*/
			break;
		default :
			(void)(*errfunc)(":221:Unknown status (0x%X)\n",
				scb->SCB.sc_status);
			break;
		}
		break;
	case (SDI_NOALLOC & MASK_FF) :	/* This block is not allocated      */
		error(":222:This block is not allocated\n");
	case (SDI_LINKF0 & MASK_FF) :	/* Linked command done without flag */
		error(":223:Linked command done without flag\n");
	case (SDI_LINKF1 & MASK_FF) :	/* Linked command done with flag    */
		error(":224:Linked command done with flag\n");
	case (SDI_QFLUSH & MASK_FF) :	/* Job was flushed                  */
		error(":225:Job was flushed\n");
	case (SDI_ABORT & MASK_FF) :	/* Command was aborted              */
		error(":226:Command was aborted\n");
	case (SDI_RESET & MASK_FF) :	/* Reset was detected on the bus    */
		error(":227:Reset was detected on the bus\n");
	case (SDI_CRESET & MASK_FF) :	/* Reset was caused by this unit    */
		error(":228:Reset was caused by this unit\n");
	case (SDI_V2PERR & MASK_FF) :	/* vtop failed                      */
		error(":229:Virtual to Physical failed\n");
	case (SDI_TIME & MASK_FF) :	/* Job timed out                    */
		error(":230:Job timed out\n");
	case (SDI_NOTEQ & MASK_FF) :	/* Addressed device not present     */
		error(":231:Addressed device not present\n");
	case (SDI_HAERR & MASK_FF) :	/* Host adapter error               */
		error(":232:Host Adapter error\n");
	case (SDI_MEMERR & MASK_FF) :	/* Memory fault                     */
		error(":233:Memory fault\n");
	case (SDI_SBUSER & MASK_FF) :	/* SCSI bus error                   */
		error(":234:SCSI bus error\n");
	case (SDI_SCBERR & MASK_FF) :	/* SCB error                        */
		error(":235:SCB error\n");
	case (SDI_OOS & MASK_FF) :	/* Device is out of service         */
		error(":236:Device is out of service\n");
	case (SDI_NOSELE & MASK_FF) :	/* The SCSI bus select failed       */
		error(":237:The SCSI bus select failed\n");
	case (SDI_MISMAT & MASK_FF) :	/* parameter mismatch               */
		error(":238:Parameter mismatch\n");
	case (SDI_PROGRES & MASK_FF) :	/* Job in progress                  */
		error(":239:Job in progress\n");
	case (SDI_UNUSED & MASK_FF) :	/* Job not in use                   */
		error(":240:Job not in use\n");
	case (SDI_ONEIC & MASK_FF) :	/* More than one immediate request */
		error(":241:More than one immediate request\n");
	case (SDI_SFBERR & MASK_FF) :	/* SFB error			   */
		error(":242:SFB error\n");
	default :
		error(":243:Unknown completion code (0x%X)\n",
			scb->SCB.sc_comp_code);
	}

	if (Show && (scb->SCB.sc_datasz > 0) && (scb->SCB.sc_mode & SCB_READ)) {
		put_string(stderr,
			":171:Data received from Host Adapter");
		put_data(stderr, scb->SCB.sc_datapt, scb->SCB.sc_datasz);
	}
	return(0);
}	/* send_scb() */

void
format(format_cdb, format_bufpt, format_bufsz, format_time)
FORMAT_T	format_cdb;
char		*format_bufpt;
long		format_bufsz;
int		format_time;
{
	int		done = FALSE;
	struct sb	format_scb;
	struct sense	sense_data;
	void killtimer();
	void (*catchsig)();			/* used by format timer */
	long start, now, elapsed;		/* used by format timer */
	int hours, minutes, seconds;		/* used by format timer */
	void error();

	(void) pfmt(stdout, MM_NOSTD,
		":244:Begin Format");
	if (format_time == 0)
		(void) printf("\n");
	else
		(void) pfmt(stdout, MM_NOSTD,
			":245: (No more than %d minutes)\n", format_time / 16);

	if (Timer) {
		Timerpid = -1;
		/* Fork off timer child */
		switch (Timerpid=fork()) {
		case -1 :
			warning(":246:format(): Can not fork timer display process\n");

			break;
		case 0 :
			/* then this is the child */

			(void)	signal(SIGHUP,SIG_DFL);
			(void) 	signal(SIGINT,SIG_DFL);
			(void)	signal(SIGQUIT,SIG_DFL);
			Stoptimer = FALSE;

			/* enable handling of SIGTERM from parent process */
			(void) signal(SIGTERM, catchsig);

			/* get and display starting time */
			(void) time(&start);
			(void) pfmt(stdout, MM_NOSTD,
				":247:\015Elapsed time: %.2d:%.2d:%.2d",
					0, 0, 0);
			fflush(stdout);


			while ( Stoptimer == FALSE ) {

				(void) sleep(10);
				(void) time(&now);

				elapsed = now - start;
				hours = (int) elapsed / 3600;
				minutes = (int) (elapsed % 3600) / 60;
				seconds = (int) (elapsed % 3600) % 60;

				/* backspace over elapsed time display and update */
				(void) pfmt(stdout, MM_NOSTD,
				    ":247:\015Elapsed time: %.2d:%.2d:%.2d",
					hours, minutes, seconds);
				fflush(stdout);

			}

			(void) fprintf(stdout,"\n");
			fflush(stdout);
			exit(NORMEXIT);
			break;

		default :
			/* this is the parent */
			break;
		}
	}

	/* Complete the Format SCSI Control Block */
	format_scb.SCB.sc_cmdpt = FORMAT_AD(&format_cdb);
	format_scb.SCB.sc_datapt = format_bufpt;
	format_scb.SCB.sc_mode = SCB_WRITE;
	format_scb.SCB.sc_cmdsz = FORMAT_SZ;
	format_scb.SCB.sc_datasz = format_bufsz;

	/* Send the Format SCSI Control Block to the Host Adapter */
	while (!done) {

		if (Show)
			put_string(stderr, ":248:Format");

		/* Complete the Format SCSI Control Block */
		format_scb.SCB.sc_time = format_time * 4500;

		switch (send_scb(&format_scb, &sense_data, &error)) {
		case SC_NOSENSE :
			done = TRUE;
			break;
		case SC_MEDCHNG :
			done = TRUE;
			break;
		case SC_RESET :
			done = FALSE;
			break;
		default :
			if (Timer)
				killtimer();
			error(":249:format(): Unknown error (0x%X)\n",
				sense_data.sd_sencode);
		}
	}

	if (Timer)
		killtimer();
}	/* format() */

void
catchsig()
{
	(void) signal(SIGTERM, SIG_IGN);
	Stoptimer=TRUE;
}

void
mdselect(mdselect_cdb, mdselect_bufpt, mdselect_bufsz)
struct scs	mdselect_cdb;
char		*mdselect_bufpt;
long	 	mdselect_bufsz;
{
	int		done = FALSE;
	struct sb	mdselect_scb;
	struct sense	sense_data;
	void		error();

	/* Fill in the Mode Select SCSI Control Block */
	mdselect_scb.SCB.sc_cmdpt = SCS_AD(&mdselect_cdb);
	mdselect_scb.SCB.sc_datapt = mdselect_bufpt;
	mdselect_scb.SCB.sc_mode = SCB_WRITE;
	mdselect_scb.SCB.sc_cmdsz = SCS_SZ;
	mdselect_scb.SCB.sc_datasz = mdselect_bufsz;

	/* Send the Mode Select SCSI Control Block to the Host Adapter */
	while (!done) {

		if (Show)
			put_string(stderr, ":250:Mode Select");

		/* Fill in the Mode Select SCSI Control Block */
		mdselect_scb.SCB.sc_time = 60 * 1000;	/* 1 Minute */

		switch (send_scb(&mdselect_scb, &sense_data, &error)) {
		case SC_NOSENSE :
			done = TRUE;
			break;
		case SC_MEDCHNG :
			done = TRUE;
			break;
		case SC_RESET :
			done = FALSE;
			break;
		default :
			error(":251:mdselect(): Unknown error (0x%X)\n",
				sense_data.sd_sencode);
		}
	}
}	/* mdselect() */

void
mdsense(mdsense_cdb, mdsense_bufpt, mdsense_bufsz, sense_data)
struct scs	mdsense_cdb;
char		*mdsense_bufpt;
long		mdsense_bufsz;
struct sense	*sense_data;
{
	int		done = FALSE;
	struct sb	mdsense_scb;
	void		nowarning();

	/* Fill in the Mode Sense SCSI Control Block */
	mdsense_scb.SCB.sc_cmdpt = SCS_AD(&mdsense_cdb);
	mdsense_scb.SCB.sc_datapt = mdsense_bufpt;
	mdsense_scb.SCB.sc_mode = SCB_READ;
	mdsense_scb.SCB.sc_cmdsz = SCS_SZ;
	mdsense_scb.SCB.sc_datasz = mdsense_bufsz;

	/* Send the Mode Sense SCSI Control Block to the Host Adapter */
	while (!done) {

		if (Show)
			put_string(stderr, ":252:Mode Sense");

		/* Fill in the Mode Sense SCSI Control Block */
		mdsense_scb.SCB.sc_time = 60 * 1000;	/* 1 Minute */

		switch (send_scb(&mdsense_scb, sense_data, &nowarning)) {
		case SC_NOSENSE :
			done = TRUE;
			break;
		case SC_MEDCHNG :
			done = TRUE;
			break;
		case SC_RESET :
			done = FALSE;
			break;
		default :
			error(":253:mdsense(): Unknown error (0x%X)\n",
				sense_data->sd_sencode);
		}
	}
}	/* mdsense() */

void
scsi_write(write_start, write_bufpt, write_bufsz)
long		write_start;
char		*write_bufpt;
long		write_bufsz;
{
	int		done = FALSE;
	struct scs	write_cdb;
	struct sb	write_scb;
	struct sense	sense_data;
	int		i;
	struct badsec_lst *blc_p;
	void		error();

	/* Skip verification check for known bad sectors
 	 * that cannot be reassigned because the reserved defective
	 * disk table is full
	 */
	if (write_bufsz == dp.dp_secsiz) {
		for (blc_p=badsl_chain; blc_p; blc_p = blc_p->bl_nxt) {
			for (i=0; i<blc_p->bl_cnt; i++) {
				if (write_start==blc_p->bl_sec[i]) {
#ifdef SCL_DEBUG
					(void) pfmt(stderr, MM_WARNING,
						":254:No-op wr-chk sec=0x%x\n",
						write_start);
#endif
					return;
				}
			}
		}
	}

	/* Fill in the Write CDB */
	write_cdb.ss_op = SS_WRITE;
	write_cdb.ss_lun = LUN(Hostdev);
	write_cdb.ss_len = (write_bufsz+dp.dp_secsiz-1)/dp.dp_secsiz;
	write_cdb.ss_cont = 0;

#if defined (i386) || defined (i486)
	write_cdb.ss_addr1 = ((write_start & 0x1F0000)>>16);
	write_cdb.ss_addr  = (write_start & 0xFFFF);
	write_cdb.ss_addr  = scl_swap16(write_cdb.ss_addr);
#else
	write_cdb.ss_addr  = write_start;
#endif /* ix86 */

	/* Fill in the Write SCSI Control Block */
	write_scb.SCB.sc_cmdpt = SCS_AD(&write_cdb);
	write_scb.SCB.sc_datapt = write_bufpt;
	write_scb.SCB.sc_mode = SCB_WRITE;
	write_scb.SCB.sc_cmdsz = SCS_SZ;
	write_scb.SCB.sc_datasz = write_bufsz;

	/* Send the Write SCSI Control Block to the Host Adapter */
	while (!done) {

		if (Show)
			put_string(stderr, ":255:Write");


		/* Fill in the Write SCSI Control Block */
		write_scb.SCB.sc_time = 60 * 1000;	/* 1 Minute */

		switch (send_scb(&write_scb, &sense_data, &error)) {
		case SC_NOSENSE :
			done = TRUE;
			break;
		case SC_IDERR :
		case SC_UNRECOVRRD :
		case SC_NOADDRID :
		case SC_NOADDRDATA :
		case SC_NORECORD :
		case SC_DATASYNCMK :
		case SC_RECOVRRD :
		case SC_RECOVRRDECC :
		case SC_CMPERR :
		case SC_RECOVRIDECC :
			if (sense_data.sd_valid) {
				int	block[2];

				if (!Silent)
					(void) pfmt(stdout, MM_NOSTD,
						":256:Mapping Bad Block 0x%X (0x%X)\n",
						    sense_data.sd_ba, sense_data.sd_sencode);
				block[0] = scl_swap32(4);
				block[1] = scl_swap32(sense_data.sd_ba);
				reassign((char *) block, 8);
				done = FALSE;
			} else
				done = TRUE;
			break;
		case SC_MEDCHNG :
			done = TRUE;
			break;
		case SC_RESET :
			done = FALSE;
			break;
		default :
			error(":257:scsi_write(): Unknown error (0x%X)\n",
				sense_data.sd_sencode);
		}
	}
}	/* scsi_write() */

int
scsi_read(read_start, read_bufpt, read_bufsz)
long		read_start;
char		*read_bufpt;
long		read_bufsz;
{
	int		done = FALSE;
	int		rval = FALSE;
	struct scs	read_cdb;
	struct sb	read_scb;
	struct sense	sense_data;
	void		error();

	/* Fill in the Read CDB */
	read_cdb.ss_op = SS_READ;
	read_cdb.ss_lun = LUN(Hostdev);
	read_cdb.ss_len = (read_bufsz+dp.dp_secsiz-1)/dp.dp_secsiz;
	read_cdb.ss_cont = 0;

#if defined (i386) || defined (i486)
	read_cdb.ss_addr1 = ((read_start & 0x1F0000)>>16);
	read_cdb.ss_addr  = (read_start & 0xFFFF);
	read_cdb.ss_addr  = scl_swap16(read_cdb.ss_addr);
#else
	read_cdb.ss_addr  = read_start;
#endif /* ix86 */

/*
	if (read_bufsz % dp.dp_secsiz) {
		char ibuf[read_cdb.ss_len];
		int i;

		for(i=0; i < read_bufsz; ++i);
			ibuf[i] =  *read_bufpt+i;
	}
*/
		

	/* Fill in the Read SCSI Control Block */
	read_scb.SCB.sc_cmdpt = SCS_AD(&read_cdb);
	read_scb.SCB.sc_datapt = read_bufpt;
	read_scb.SCB.sc_mode = SCB_READ;
	read_scb.SCB.sc_cmdsz = SCS_SZ;
	read_scb.SCB.sc_datasz = read_bufsz;

	/* Send the Read SCSI Control Block to the Host Adapter */
	while (!done) {

		if (Show)
			put_string(stderr, ":258:Read");

		/* Fill in the Read SCSI Control Block */
		read_scb.SCB.sc_time = 60 * 1000;	/* 1 Minute */

		switch (send_scb(&read_scb, &sense_data, &error)) {
		case SC_NOSENSE :
			done = TRUE;
			break;
		case SC_IDERR :
		case SC_UNRECOVRRD :
		case SC_NOADDRID :
		case SC_NOADDRDATA :
		case SC_NORECORD :
			if (sense_data.sd_valid) {
				char	*bufpt;
				int	block[2];

				bufpt = malloc(dp.dp_secsiz);

				if (!Silent)
					(void) pfmt(stdout, MM_NOSTD,
						":256:Mapping Bad Block 0x%X (0x%X)\n",
						    sense_data.sd_ba, sense_data.sd_sencode);

				block[0] = scl_swap32(4);
				block[1] = scl_swap32(sense_data.sd_ba);
				reassign((char *) block, 8);
				scsi_write((long) sense_data.sd_ba, bufpt, dp.dp_secsiz);
				free(bufpt);
				rval = TRUE;
			}
			done = TRUE;
			break;
		case SC_DATASYNCMK :
		case SC_RECOVRRD :
		case SC_RECOVRRDECC :
		case SC_CMPERR :
		case SC_RECOVRIDECC :
			if (sense_data.sd_valid) {
				int	block[2];

				if (!Silent)
					(void) pfmt(stdout, MM_NOSTD,
						":256:Mapping Bad Block 0x%X (0x%X)\n",
						    sense_data.sd_ba, sense_data.sd_sencode);

				block[0] = scl_swap32(4);
				block[1] = scl_swap32(sense_data.sd_ba);
				reassign((char *) block, 8);
				scsi_write(read_start, read_bufpt, read_bufsz);
				rval = MARBLK;
			}
			done = TRUE;
			break;
		case SC_MEDCHNG :
			done = TRUE;
			break;
		case SC_RESET :
			done = FALSE;
			break;
		default :
			error(":259:scsi_read(): Unknown error (0x%X)\n",
				sense_data.sd_sencode);
		}
	}
	return(rval);
}	/* scsi_read() */


void
scsi_verify(verify_cdb, verify_start, verify_len, verify_size, no_map)
struct scm	verify_cdb;
int		verify_start;
int		verify_len;
int		verify_size;
int		no_map;
{
	struct sb	verify_scb;
	struct sense	sense_data;
	void killtimer();
	void (*catchsig)();		/* used by format timer */
	long start, now, elapsed;		/* used by format timer */
	int hours, minutes, seconds;		/* used by format timer */
	void		error();
	int	out_count,total_count,loop_count,iter_count,modulo_value;

	if (verify_size <= 0) {
	     verify_size = 128; 
	}
	if (!Silent)
		(void) pfmt(stdout, MM_NOSTD,
			":260:Begin Verify (No more than %d minutes)\n",
			    ((verify_len * 2 / 1000) + 59) / 60);

	if (Timer) {
		Timerpid = -1;
		/* Fork off timer child */
		switch (Timerpid=fork()) {
		case -1 :
			warning(":261:scsi_verify(): Can not fork timer display process\n");

			break;
		case 0 :
			/* then this is the child */

			(void)	signal(SIGHUP,SIG_DFL);
			(void) 	signal(SIGINT,SIG_DFL);
			(void)	signal(SIGQUIT,SIG_DFL);
			Stoptimer = FALSE;

			/* enable handling of SIGTERM from parent process */
			(void) signal(SIGTERM, catchsig);

			/* get and display starting time */
			(void) time(&start);
			(void) pfmt(stdout, MM_NOSTD,
				":247:\015Elapsed time: %.2d:%.2d:%.2d",
					0, 0, 0);
			fflush(stdout);


			while ( Stoptimer == FALSE ) {

				(void) sleep(10);
				(void) time(&now);

				elapsed = now - start;
				hours = (int) elapsed / 3600;
				minutes = (int) (elapsed % 3600) / 60;
				seconds = (int) (elapsed % 3600) % 60;

				/* backspace over elapsed time display and update */
				(void) pfmt(stdout, MM_NOSTD,
					":247:\015Elapsed time: %.2d:%.2d:%.2d",
						hours, minutes, seconds);
				fflush(stdout);

			}

			(void) fprintf(stdout,"\n");
			fflush(stdout);
			exit(NORMEXIT);
			break;

		default :
			/* this is the parent */
			break;
		}
	}

	/* Fill in the Verify SCSI Control Block */
	verify_scb.SCB.sc_cmdpt = SCM_AD(&verify_cdb);
	verify_scb.SCB.sc_datapt = 0;
	verify_scb.SCB.sc_mode = SCB_WRITE;
	verify_scb.SCB.sc_cmdsz = SCM_SZ;
	verify_scb.SCB.sc_datasz = 0;

	if (gaugeflg) {
		total_count = iter_count = (verify_len / verify_size) + 1;
		modulo_value = 1;
		while (total_count > 100) {
			total_count = (total_count + 1) / 2;
			modulo_value *= 2;
		}
		fprintf(stdout,"%d\n",total_count);
		fflush(stdout);
		out_count = loop_count = 0;
	}

	while (verify_len > 0) {


		if (Show)
			put_string(stderr, ":262:Verify");


		/* Fill in the Verify CDB */
		verify_cdb.sm_addr = verify_start;
		verify_cdb.sm_len = ((verify_len > verify_size) ? verify_size : verify_len);

		/* swap */
		verify_cdb.sm_addr = scl_swap32(verify_cdb.sm_addr);
		verify_cdb.sm_len  = scl_swap16(verify_cdb.sm_len);

		/* Fill in the Verify SCSI Control Block */
		verify_scb.SCB.sc_time = ((verify_len > verify_size) ? verify_size : verify_len) * 2;

		/* Send the Verify SCSI Control Block to the Host Adapter */
		if (send_scb(&verify_scb, &sense_data, &error)) {
		        switch (sense_data.sd_sencode) {
			case SC_NOSENSE :
				break;
			case SC_IDERR :
			case SC_UNRECOVRRD :
			case SC_NOADDRID :
			case SC_NOADDRDATA :
			case SC_NORECORD :
				if (sense_data.sd_valid) {
					char	*bufpt;
					int	block[2];

					if (!no_map) {
						if (!Silent)
							(void) pfmt(stdout, MM_NOSTD,
							    ":256:Mapping Bad Block 0x%X (0x%X)\n",
								sense_data.sd_ba, sense_data.sd_sencode);

						block[0] = scl_swap32(4);
						block[1] = scl_swap32(sense_data.sd_ba);

						reassign((char *) block, 8);
/*
 * NOTE: block[0] is used by reassign to return the block
 *       address of the last block remapped.  reassign can
 *       remap more than 1 block if it find consecutive bad blocks
 */
						verify_len -= (block[0] - verify_start + 1);
						verify_start = block[0] + 1;
					}
					else {
						(void) pfmt(stdout, MM_NOSTD,
						     ":263:Bad block 0x%X (0x%X)\n",
							    sense_data.sd_ba, sense_data.sd_sencode);

						verify_len -= ((verify_len > verify_size) ? verify_size : verify_len);
						verify_start += ((verify_len > verify_size) ? verify_size : verify_len);
					}
				}
				break;
			case SC_DATASYNCMK :
			case SC_RECOVRRD :
			case SC_RECOVRRDECC :
			case SC_CMPERR :
			case SC_RECOVRIDECC :
				if (sense_data.sd_valid) {
					char	*bufpt;
					int	block[2];

					if (!no_map) {
						bufpt = malloc(dp.dp_secsiz);
						block[0] = scl_swap32(4);
						block[1] = scl_swap32(sense_data.sd_ba);

						if (scsi_read((long) block[1], bufpt, dp.dp_secsiz) == 0) {
							if (!Silent)
							    (void) pfmt(stdout,  MM_NOSTD,
								":256:Mapping Bad Block 0x%X (0x%X)\n",
									sense_data.sd_ba, sense_data.sd_sencode);

							reassign((char *) block, 8);
						}

						scsi_write((long) sense_data.sd_ba, bufpt, dp.dp_secsiz);
						free(bufpt);
						verify_len -= ((int)sense_data.sd_ba - verify_start + 1);
						verify_start = (int)sense_data.sd_ba + 1;
					}
					else {
						(void) pfmt(stdout, MM_NOSTD,
						    ":263:Bad block 0x%X (0x%X)\n",
							sense_data.sd_ba, sense_data.sd_sencode);

						verify_len -= ((verify_len > verify_size) ? verify_size : verify_len);
						verify_start += ((verify_len > verify_size) ? verify_size : verify_len);
					}
				}
				break;
			case SC_MEDCHNG :
			case SC_RESET :
				break;
			default :
				if (Timer)
					killtimer();
				error(":264:scsi_verify(): Unknown error (0x%X)\n",
				    sense_data.sd_sencode);
			}
		} else {

       			verify_len -= ((verify_len > verify_size) ? verify_size : verify_len);
			verify_start += ((verify_len > verify_size) ? verify_size : verify_len);
			if (gaugeflg) {
				if (!(++loop_count % modulo_value)) {
					out_count++;
					fprintf(stdout,"%d%%\n",((loop_count*100)/iter_count));
					fflush(stdout);
				}
			}
		}
	}

	/*
	 * If we are producing gauge output, make sure that we output at least
	 * as many updates as we promised we would.
	 */
	if (gaugeflg) {
		while ( out_count++ < total_count ) {
			fprintf(stdout,"100%%\n");
			fflush(stdout);
		}
	}

	if (Timer) 
		killtimer();

}	/* scsi_verify() */
void
killtimer()
{
	(void) printf("\n");
	if(Timerpid != -1) {
		if (kill(Timerpid,SIGTERM) < 0)
			/* issue a warning to the user */
			warning(":265:Could not kill timer display\n");
	}
}
void
readdefects(rdd_cdb, rdd_bufpt, rdd_bufsz)
struct scm	rdd_cdb;
char		**rdd_bufpt;
long		*rdd_bufsz;
{
	int		done = FALSE;
	struct sb	rdd_scb;
	struct sense	sense_data;
	void		error();

	/* Allocate memory to hold the defect list header */
	rdd_cdb.sm_len = DLH_SZ;
	*rdd_bufsz = rdd_cdb.sm_len;
	*rdd_bufpt = malloc(*rdd_bufsz);

	/* Fill in the READ DEFECT DATA SCB */
	rdd_scb.SCB.sc_cmdpt = SCM_AD(&rdd_cdb);
	rdd_scb.SCB.sc_datapt = *rdd_bufpt;
	rdd_scb.SCB.sc_mode = SCB_READ;
	rdd_scb.SCB.sc_cmdsz = SCM_SZ;
	rdd_scb.SCB.sc_datasz = *rdd_bufsz;

	/* Send the Read Defect Data SCB to the Host Adapter */
	while (!done) {

		if (Show)
			put_string(stderr, ":266:Read Defect Data");

		/* Fill in the READ DEFECT DATA SCB */

		rdd_scb.SCB.sc_time = 60 * 1000;	/* 1 Minute */

		switch (send_scb(&rdd_scb, &sense_data, &error)) {
		case SC_NOSENSE :
			done = TRUE;
			break;
		case SC_IDERR :
		case SC_UNRECOVRRD :
		case SC_NOADDRID :
		case SC_NOADDRDATA :
		case SC_NORECORD :
			if (sense_data.sd_valid) {
				char	*bufpt;
				int	block[2];

				bufpt = malloc(dp.dp_secsiz);
				if (!Silent)
					(void) pfmt(stdout, MM_NOSTD,
					    ":256:Mapping Bad Block 0x%X (0x%X)\n",
						sense_data.sd_ba, sense_data.sd_sencode);
				block[0] = scl_swap32(4);
				block[1] = scl_swap32(sense_data.sd_ba);
				reassign((char *) block, 8);
				scsi_write((long) sense_data.sd_ba, bufpt, dp.dp_secsiz);
				free(bufpt);
			}
			done = TRUE;
			break;
		case SC_DATASYNCMK :
		case SC_RECOVRRD :
		case SC_RECOVRRDECC :
		case SC_CMPERR :
		case SC_RECOVRIDECC :
			if (sense_data.sd_valid) {
				char	*bufpt;
				int	block[2];

				bufpt = malloc(dp.dp_secsiz);
				block[0] = scl_swap32(4);
				block[1] = scl_swap32(sense_data.sd_ba);

				if (scsi_read((long) sense_data.sd_ba, bufpt, dp.dp_secsiz) == 0) {
					if (!Silent)
						(void) pfmt(stdout, MM_NOSTD,
						    ":256:Mapping Bad Block 0x%X (0x%X)\n",
							sense_data.sd_ba, sense_data.sd_sencode);

					reassign((char *) block, 8);
				}

				scsi_write((long) sense_data.sd_ba, bufpt, dp.dp_secsiz);
				free(bufpt);
			}
			done = TRUE;
			break;
		case SC_MEDCHNG :
			done = TRUE;
			break;
		case SC_RESET :
			done = FALSE;
			break;
		default :
			error(":267:readdefects(): Unknown error (0x%X)\n",
				sense_data.sd_sencode);
		}
	}

	if (((DLH_T *) *rdd_bufpt)->dlh_len) {
		/* Set up to get all Defect Data */
	  	*rdd_bufsz = DLH_SZ + scl_swap16(((DLH_T *) *rdd_bufpt)->dlh_len);
		rdd_cdb.sm_len = *rdd_bufsz;
		rdd_scb.SCB.sc_datasz = *rdd_bufsz;

		/* Allocate memory to hold the defect list header and defect list */
		*rdd_bufpt = realloc(*rdd_bufpt, *rdd_bufsz);
		rdd_scb.SCB.sc_datapt = *rdd_bufpt;

		/* Send the Read Defect Data SCSI Control Block to the Host Adapter */
		done = FALSE;
		while (!done) {
			/* Fill in the READ DEFECT DATA SCB */
			rdd_scb.SCB.sc_time = 60 * 1000;	/* 1 Minute */

			switch (send_scb(&rdd_scb, &sense_data, &error)) {
			case SC_NOSENSE :
				done = TRUE;
				break;
			case SC_IDERR :
			case SC_UNRECOVRRD :
			case SC_NOADDRID :
			case SC_NOADDRDATA :
			case SC_NORECORD :
				if (sense_data.sd_valid) {
					char	*bufpt;
					int	block[2];

					bufpt = malloc(dp.dp_secsiz);
					if (!Silent)
						(void) pfmt(stdout, MM_NOSTD,
						    ":256:Mapping Bad Block 0x%X (0x%X)\n",
							sense_data.sd_ba, sense_data.sd_sencode);

					block[0] = scl_swap32(4);
					block[1] = scl_swap32(sense_data.sd_ba);
					reassign((char *) block, 8);
					scsi_write((long) sense_data.sd_ba, bufpt, dp.dp_secsiz);
					free(bufpt);
				}
				done = TRUE;
				break;
			case SC_DATASYNCMK :
			case SC_RECOVRRD :
			case SC_RECOVRRDECC :
			case SC_CMPERR :
			case SC_RECOVRIDECC :
				if (sense_data.sd_valid) {
					char	*bufpt;
					int	block[2];

					bufpt = malloc(dp.dp_secsiz);
					block[0] = scl_swap32(4);
					block[1] = scl_swap32(sense_data.sd_ba);

					if (scsi_read((long) sense_data.sd_ba, bufpt, dp.dp_secsiz) == 0) {
						if (!Silent)
							(void) pfmt(stdout, MM_NOSTD,
							    ":256:Mapping Bad Block 0x%X (0x%X)\n",
								sense_data.sd_ba, sense_data.sd_sencode);

						reassign((char *) block, 8);
					}

					scsi_write((long) sense_data.sd_ba, bufpt, dp.dp_secsiz);
					free(bufpt);
				}
				done = TRUE;
				break;
			case SC_MEDCHNG :
				done = TRUE;
				break;
			case SC_RESET :
				done = FALSE;
				break;
			default :
				error(":267:readdefects(): Unknown error (0x%X)\n",
					sense_data.sd_sencode);
			}
		}
	}
}	/* readdefects() */

void
readcap(readcap_cdb, readcap_bufpt, readcap_bufsz)
struct scm	readcap_cdb;
char		*readcap_bufpt;
long		readcap_bufsz;
{
	int		done = FALSE;
	struct sb	readcap_scb;
	struct sense	sense_data;
	void		error();

	/* Fill in the READ CAPACITY SCB */
	readcap_scb.SCB.sc_cmdpt = SCM_AD(&readcap_cdb);
	readcap_scb.SCB.sc_datapt = readcap_bufpt;
	readcap_scb.SCB.sc_mode = SCB_READ;
	readcap_scb.SCB.sc_cmdsz = SCM_SZ;
	readcap_scb.SCB.sc_datasz = readcap_bufsz;

	/* Send the Read Capacity SCB to the Host Adapter */
	while (!done) {

		if (Show)
			put_string(stderr, ":268:Read Capacity");

		/* Fill in the READ CAPACITY SCB */
		readcap_scb.SCB.sc_time = 60 * 1000;	/* 1 Minute */

		switch (send_scb(&readcap_scb, &sense_data, &error)) {
		case SC_NOSENSE :
			done = TRUE;
			break;
		case SC_IDERR :
		case SC_UNRECOVRRD :
		case SC_NOADDRID :
		case SC_NOADDRDATA :
		case SC_NORECORD :
			if (sense_data.sd_valid) {
				char	*bufpt;
				int	block[2];

				bufpt = malloc(dp.dp_secsiz);
				if (!Silent)
					(void) pfmt(stdout, MM_NOSTD,
					    ":256:Mapping Bad Block 0x%X (0x%X)\n",
						sense_data.sd_ba, sense_data.sd_sencode);
				block[0] = scl_swap32(4);
				block[1] = scl_swap32(sense_data.sd_ba);
				reassign((char *) block, 8);
				scsi_write((long) sense_data.sd_ba, bufpt, dp.dp_secsiz);
				free(bufpt);
			}
			done = TRUE;
			break;
		case SC_DATASYNCMK :
		case SC_RECOVRRD :
		case SC_RECOVRRDECC :
		case SC_CMPERR :
		case SC_RECOVRIDECC :
			if (sense_data.sd_valid) {
				char	*bufpt;
				int	block[2];

				bufpt = malloc(dp.dp_secsiz);
				block[0] = scl_swap32(4);
				block[1] = scl_swap32(sense_data.sd_ba);

				if (scsi_read((long) sense_data.sd_ba, bufpt, dp.dp_secsiz) == 0) {
					if (!Silent)
						(void) pfmt(stdout, MM_NOSTD,
						    ":256:Mapping Bad Block 0x%X (0x%X)\n",
							sense_data.sd_ba, sense_data.sd_sencode);

					reassign((char *) block, 8);
				}

				scsi_write((long) sense_data.sd_ba, bufpt, dp.dp_secsiz);
				free(bufpt);
			}
			done = TRUE;
			break;
		case SC_MEDCHNG :
			done = TRUE;
			break;
		case SC_RESET :
			done = FALSE;
			break;
		default :
			error(":269:readcap(): Unknown error (0x%X)\n",
				sense_data.sd_sencode);
		}
	}
}	/* readcap() */

void
reassign(rabl_bufpt, rabl_bufsz)
int		*rabl_bufpt;
long		rabl_bufsz;
{
	int		last_block_remapped;
	int		done = FALSE;
	struct sb	rabl_scb;
	struct scs	rabl_cdb;
	struct sense	sense_data;
	void		error();

	/* Fill in the Reassign Blocks CDB */
	rabl_cdb.ss_op = SS_REASGN;
	rabl_cdb.ss_lun = LUN(Hostdev);
	rabl_cdb.ss_addr1 = 0;
	rabl_cdb.ss_addr  = 0;
	rabl_cdb.ss_len = 0;
	rabl_cdb.ss_cont = 0;

	/* Fill in the Reassign Blocks SCB */
	rabl_scb.SCB.sc_cmdpt = SCS_AD(&rabl_cdb);
	rabl_scb.SCB.sc_datapt = (char *)rabl_bufpt;
	rabl_scb.SCB.sc_mode = SCB_WRITE;
	rabl_scb.SCB.sc_cmdsz = SCS_SZ;
	rabl_scb.SCB.sc_datasz = rabl_bufsz;

	last_block_remapped = scl_swap32(rabl_bufpt[scl_swap32(rabl_bufpt[0])/4]);

	/* Send the Reassign Blocks SCSI Control Block to the Host Adapter */
	while (!done) {

		if (Show)
			put_string(stderr, ":270:Reassign Blocks");

		/* Fill in the Reassign Blocks SCB */
		rabl_scb.SCB.sc_time = rabl_bufsz * 60 * 1000;

		switch (send_scb(&rabl_scb, &sense_data, &error)) {
		case SC_NOSENSE :
			done = TRUE;
			break;
		case SC_IDERR :
		case SC_UNRECOVRRD :
		case SC_NOADDRID :
		case SC_NOADDRDATA :
		case SC_NORECORD :
			if (sense_data.sd_valid) {
				int	*block, i, j;
				int	new_size = rabl_bufsz + 4;
				int	test_value;
				int	insert = TRUE;

				block = (int *)malloc(new_size);

				for (j = i = 1; i < rabl_bufsz / 4; i++) {

					if ( insert ) {
						test_value = scl_swap32(rabl_bufpt[i]);
						if ( sense_data.sd_ba < test_value ) {
							block[j++] = scl_swap32(sense_data.sd_ba);
							insert = FALSE;
						} else if (sense_data.sd_ba == test_value) {
							scsi_nodfct(rabl_bufpt, rabl_bufsz);
							free(block);
							goto done;
						}
					}

					block[j++] = rabl_bufpt[i];
				}
				if ( insert )
					block[j] = scl_swap32(sense_data.sd_ba);

				if (!Silent)
					(void) pfmt(stdout, MM_NOSTD,
					    ":256:Mapping Bad Block 0x%X (0x%X)\n",
						sense_data.sd_ba, sense_data.sd_sencode);

				block[0] = scl_swap32(new_size - 4);

				reassign((char *) block, new_size);
				last_block_remapped = block[0];

				free(block);
			}
		done:
			done = TRUE;
			break;
		case SC_DATASYNCMK :
		case SC_RECOVRRD :
		case SC_RECOVRRDECC :
		case SC_CMPERR :
		case SC_RECOVRIDECC :
			if (sense_data.sd_valid) {
				char	*bufpt;
				int	block[2];

				bufpt = malloc(dp.dp_secsiz);
				block[0] = scl_swap32(4L);
				block[1] = scl_swap32(sense_data.sd_ba);

				if (scsi_read((long) sense_data.sd_ba, bufpt, dp.dp_secsiz) == 0) {
					if (!Silent)
						(void) pfmt(stdout, MM_NOSTD,
						    ":256:Mapping Bad Block 0x%X (0x%X)\n",
							sense_data.sd_ba, sense_data.sd_sencode);

					reassign((char *) block, 8);
					last_block_remapped = block[0];
				}

				scsi_write((long) sense_data.sd_ba, bufpt, dp.dp_secsiz);
				free(bufpt);
			}
			done = TRUE;
			break;
		case SC_MEDCHNG :
			done = TRUE;
			break;
		case SC_RESET :
			done = FALSE;
			break;
		case SC_NODFCT :
			scsi_nodfct(rabl_bufpt, rabl_bufsz);
			done = TRUE;
			break;
		default :
			error(":271:reassign(): Unknown error (0x%X)\n",
				sense_data.sd_sencode);
		}
	}
	rabl_bufpt[0] = last_block_remapped;
}	/* reassign() */

#define INQ_REQD_LEN	8

void
inquiry(inquiry_data)
struct ident	*inquiry_data;
{
	int		done = FALSE;
	struct sb	inquiry_scb;
	struct scs	inquiry_cdb;
	struct sense	sense_data;
	void		error();

	/* Fill in the Inquiry CDB */
	inquiry_cdb.ss_op = SS_INQUIR;
	inquiry_cdb.ss_lun = LUN(Hostdev);
	inquiry_cdb.ss_addr1 = 0;
	inquiry_cdb.ss_addr  = 0;
	inquiry_cdb.ss_len = INQ_REQD_LEN;
	inquiry_cdb.ss_cont = 0;

	/* Fill in the Inquiry SCB */
	inquiry_scb.SCB.sc_cmdpt = SCS_AD(&inquiry_cdb);
	inquiry_scb.SCB.sc_datapt = IDENT_AD(inquiry_data);
	inquiry_scb.SCB.sc_mode = SCB_READ;
	inquiry_scb.SCB.sc_cmdsz = SCS_SZ;
	inquiry_scb.SCB.sc_datasz = INQ_REQD_LEN;

	/* Send the Inquiry SCSI Control Block to the Host Adapter */
	while (!done) {

		if (Show)
			put_string(stderr, ":272:Inquiry");

		/* Fill in the Inquiry SCB */
		inquiry_scb.SCB.sc_time = 60 * 1000;	/* 1 Minute */

		switch (send_scb(&inquiry_scb, &sense_data, &error)) {
		case SC_NOSENSE :
			done = TRUE;
			break;
		case SC_MEDCHNG :
			done = TRUE;
			break;
		case SC_RESET :
			done = FALSE;
			break;
		default :
			error(":273:inquiry(): Unknown error (0x%X)\n",
				sense_data.sd_sencode);
		}
	}

	if ( inquiry_data->id_len < (unsigned char)(IDENT_SZ - INQ_REQD_LEN))
		/* Not enough INQUIRY Data to name Target Controller */
		error(":274:Insufficient INQUIRY Data\n");

	/* Set up to get all INQUIRY Data */
	inquiry_cdb.ss_len = IDENT_SZ;
	inquiry_scb.SCB.sc_datasz = IDENT_SZ;

	/* Send the Inquiry SCSI Control Block to the Host Adapter */
	done = FALSE;
	while (!done) {

		if (Show)
			put_string(stderr, ":272:Inquiry");

		/* Fill in the Inquiry SCB */
		inquiry_scb.SCB.sc_time = 60 * 1000;	/* 1 Minute */

		switch (send_scb(&inquiry_scb, &sense_data, &error)) {
		case SC_NOSENSE :
			done = TRUE;
			break;
		case SC_IDERR :
		case SC_UNRECOVRRD :
		case SC_NOADDRID :
		case SC_NOADDRDATA :
		case SC_NORECORD :
			if (sense_data.sd_valid) {
				char	*bufpt;
				int	block[2];

				bufpt = malloc(dp.dp_secsiz);
				if (!Silent)
					(void) pfmt(stdout, MM_NOSTD,
					    ":256:Mapping Bad Block 0x%X (0x%X)\n",
						sense_data.sd_ba, sense_data.sd_sencode);
				block[0] = scl_swap32(4);
				block[1] = scl_swap32(sense_data.sd_ba);
				reassign((char *) block, 8);
				scsi_write((long) sense_data.sd_ba, bufpt, dp.dp_secsiz);
				free(bufpt);
			}
			done = TRUE;
			break;
		case SC_DATASYNCMK :
		case SC_RECOVRRD :
		case SC_RECOVRRDECC :
		case SC_CMPERR :
		case SC_RECOVRIDECC :
			if (sense_data.sd_valid) {
				char	*bufpt;
				int	block[2];

				bufpt = malloc(dp.dp_secsiz);
				block[0] = scl_swap32(4);
				block[1] = scl_swap32(sense_data.sd_ba);

				if (scsi_read((long) sense_data.sd_ba, bufpt, dp.dp_secsiz) == 0) {
					if (!Silent)
						(void) pfmt(stdout, MM_NOSTD,
						    ":256:Mapping Bad Block 0x%X (0x%X)\n",
							sense_data.sd_ba, sense_data.sd_sencode);

					reassign((char *) block, 8);
				}

				scsi_write((long) sense_data.sd_ba, bufpt, dp.dp_secsiz);
				free(bufpt);
			}
			done = TRUE;
			break;
		case SC_MEDCHNG :
			done = TRUE;
			break;
		case SC_RESET :
			done = FALSE;
			break;
		default :
			error(":273:inquiry(): Unknown error (0x%X)\n",
				sense_data.sd_sencode);
		}
	}
}	/* inquiry() */

void
get_defect(defectfile, bufpt, bufsz)
char	*defectfile;
char	**bufpt;
long	*bufsz;
{
	char	*defects;	/* Start of defect list		*/
	int	(*compar)();	/* Pointer to Defect List sorting function */
	int	defect_sz;	/* Size of each defect		*/
	short	number;		/* Total number of defects	*/
	FILE	*defectfp;	/* Defect file pointer		*/

	/* Open the defect file */
	if ((defectfp = fopen(defectfile, "r")) == NULL)
		/* Defect file cannot be opened */
		error(":275:%s open failed\n", defectfile);

	switch (get_token(defectfp)) {
	case BLOCK :
		defect_sz = BLOCK_SZ;
		compar = blocksort;
		break;
	case BYTES :
		defect_sz = BYTES_SZ;
		compar = bytessort;
		break;
	case PHYSICAL :
		defect_sz = PHYSICAL_SZ;
		compar = physicalsort;
		break;
	default :
		error(":276:Unknown token in %s\n", defectfile);
		break;
	}

	/* Read the number of defects in the defect list */
	if (get_data(defectfp, (char *) &number, 2) != 2)
		error(":277:Cannot read number of defects in %s\n",
			defectfile);

	/* Allocate memory to hold the defect list header and defect list */
	*bufsz = defect_sz * number + DLH_SZ;
	*bufpt = realloc(*bufpt, *bufsz);

	((DLH_T *) *bufpt)->dlh_len = defect_sz * number;
	defects = *bufpt + DLH_SZ;

	/* Read the defect list from the defect file */
	if (get_data(defectfp, defects, scl_swap16(((DLH_T *) *bufpt)->dlh_len)) !=  scl_swap16(((DLH_T *) *bufpt)->dlh_len))
		error(":278:Defect list is incomplete in %s\n",
			defectfile);

	/* Close the defect file */
	fclose(defectfp);
	
	/* Sort the defect list */
	qsort(defects, number, defect_sz, compar);
}	/* get_defect() */

void
put_defect(defectfp, bufpt)
FILE	*defectfp;
char	*bufpt;
{
	char	*defects;	/* Start of defect list		*/
	int	token;		/* Type of defects in the list	*/
	short	number;		/* Total number of defects	*/
	short	number1;
	int	i;

	if (Show)
		(void) pfmt(stderr, MM_ERROR,
			":279:DLF: (0x%X)\n", ((DLH_T *) bufpt)->dlh_dlf);

	switch (((DLH_T *) bufpt)->dlh_dlf) {
	case DLF_BLOCK :
		number = scl_swap16(((DLH_T *) bufpt)->dlh_len) / BLOCK_SZ;
		(void) pfmt(stdout, MM_NOSTD,
			":280:Defect List Length: %d Defective Logical Blocks\n", number);
		token = BLOCK;
		break;
	case DLF_BYTES :
		number = scl_swap16(((DLH_T *) bufpt)->dlh_len) / BYTES_SZ;
		(void) pfmt(stdout, MM_NOSTD,
			":280:Defect List Length: %d Defective Logical Blocks\n", number);
		token = BYTES;
		break;
	case DLF_PHYSICAL :
		number = scl_swap16(((DLH_T *) bufpt)->dlh_len) / PHYSICAL_SZ;
		(void) pfmt(stdout, MM_NOSTD,
			":280:Defect List Length: %d Defective Logical Blocks\n", number);
		token = PHYSICAL;
		break;
	default :
		error(":281:Unknown Defect List Format (%d)\n",
			((DLH_T *) bufpt)->dlh_dlf);
		break;
	}

	/* Put the type of defect in the defect file */
	if (defectfp != stdout)
		put_token(defectfp, token);

	/* Put the number of defects in the defect list in the defect file */
	if (defectfp != stdout) {
		number = scl_swap16(number);
		put_data(defectfp, (char *) &number, 2);
		number = scl_swap16(number);
	}

	/* Write the defect list to the defect file */
	defects = bufpt + DLH_SZ;
	switch (token) {
	int cur;
	case BLOCK :
		if (defectfp == stdout) {
			(void) pfmt(stdout, MM_NOSTD,
				":282:Logical Block Number\n");

			number1 = number;

			for (cur = 0; cur < number; cur+= 8) {
				for (i = 0; i < 8 && number1 > 0; ++i, --number1) {
					(void) pfmt(defectfp, MM_NOSTD,
						":283:%.8X ", scl_swap32(((BLOCK_T *) defects)->dl_addr));
					defects += BLOCK_SZ;
				}
				(void) printf("\n");
			}
		}
		else {
			for (cur = 0; cur < number; cur++) {
				(void) pfmt(defectfp, MM_NOSTD,
					":284:      %.8X\n", scl_swap32(((BLOCK_T *) defects)->dl_addr));
				defects += BLOCK_SZ;
			}
		}
		break;
	case BYTES :
		if (defectfp == stdout)
			(void) pfmt(stdout, MM_NOSTD,
				":285:Cylinder Track Bytes from Offset\n");
		for (cur = 0; cur < number; cur++) {
			(void) pfmt(defectfp, MM_NOSTD,
					":286: %.6X   %.2X       %.8X\n",
				scl_swap24(((BYTES_T *) defects)->dl_cyl),
				((BYTES_T *) defects)->dl_head,
				scl_swap32(((BYTES_T *) defects)->dl_byte));
			defects += BYTES_SZ;
		}
		break;
	case PHYSICAL :
		if (defectfp == stdout)
			(void) pfmt(stdout, MM_NOSTD,
				":287:Cylinder Track  Sector\n");
		for (cur = 0; cur < number; cur++) {
			(void) pfmt(defectfp, MM_NOSTD,
					":288: %.6X   %.2X   %.8X\n",
				scl_swap24(((PHYSICAL_T *) defects)->dl_cyl),
				((PHYSICAL_T *) defects)->dl_head,
				scl_swap32(((PHYSICAL_T *) defects)->dl_sec));
			defects += PHYSICAL_SZ;
		}
		break;
	}
}	/* put_defect() */

int
blocksort(defect1, defect2)
BLOCK_T	*defect1, *defect2;
{
	return(scl_swap32(defect1->dl_addr) - scl_swap32(defect2->dl_addr));
}	/* blocksort() */

int
bytessort(defect1, defect2)
BYTES_T	*defect1, *defect2;
{
	int cyldiff = scl_swap24(defect1->dl_cyl) - scl_swap24(defect2->dl_cyl);

	if (cyldiff == 0) {
		int headdiff = defect1->dl_head - defect2->dl_head;

		if (headdiff == 0)
			return(scl_swap32(defect1->dl_byte) - scl_swap32(defect2->dl_byte));
		else
			return(headdiff);
	} else
		return(cyldiff);
}	/* bytessort() */

int
physicalsort(defect1, defect2)
PHYSICAL_T	*defect1, *defect2;
{
	int cyldiff = scl_swap24(defect1->dl_cyl) - scl_swap24(defect2->dl_cyl);

	if (cyldiff == 0) {
		int headdiff = defect1->dl_head - defect2->dl_head;

		if (headdiff == 0)
			return(scl_swap32(defect1->dl_sec) - scl_swap32(defect2->dl_sec));
		else
			return(headdiff);
	} else
		return(cyldiff);
}	/* physicalsort() */

int
scsi_open(devicefile, bhostfile)
char	*devicefile;
char	*bhostfile;
{
	int	devicefdes;
	char	*ptr;
	

	/* Create the host adapter node in the same directory as
	/* the device node. */
	(void) strcpy(Hostfile, devicefile);
	if ((ptr = strrchr(Hostfile, '/')) != NULL)
		(void) strcpy(++ptr, bhostfile);
	else
		(void) strcpy(Hostfile, bhostfile);

	mktemp(Hostfile);
	errno = 0;

	if (Show)
		(void) pfmt(stderr, MM_ERROR,
			":289:Opening %s\n", devicefile);

	/* Open the special device file */
	if ((devicefdes = open(devicefile, O_RDONLY)) < 0) {
		/* Cannot continue if we cannot open the device */

		if (Show)
			(void) pfmt(stderr, MM_ERROR,
				":275:%s open failed\n", devicefile);

		return(TRUE);
	}

	/* Get the Host Adapter device number from the device driver */
	if (ioctl(devicefdes, B_GETDEV, &Hostdev) < 0) {

		if (Show)
			(void) pfmt(stderr, MM_ERROR,
				":290:B_GETDEV ioctl failed\n");

		close(devicefdes);
		return(TRUE);
	}

	/* Close the special device file */
	close(devicefdes);

	if (Show)
		(void) pfmt(stderr, MM_ERROR,
			":291:Creating %s\n", Hostfile);

	/* Create the Host Adapter special device file */
#if defined (i386) || defined (i486)
	if (mknod(Hostfile, (S_IFCHR | S_IREAD | S_IWRITE), Hostdev) < 0)
#else
	if (mknod(Hostfile, (S_IFCHR | S_IREAD | S_IWRITE),(makedev(Hostdev.maj),(Hostdev.min))) < 0)
#endif /* ix86 */
	{
		if (Show)
			(void) pfmt(stderr, MM_ERROR,
				":292:%s mknod failed\n", Hostfile);
		return(TRUE);
	}

	sync(); sync(); sleep(2);

	if (Show)
		(void) pfmt(stderr, MM_ERROR,
			":289:Opening %s\n", Hostfile);

	/* Open the SCSI Host Adapter special device file */
	if ((Hostfdes = open(Hostfile, O_RDWR|O_EXCL)) < 0) {
		/* Remove the Host Adapter special device file */
		unlink(Hostfile);

		/* Cannot continue if we cannot open the device */

		if (Show)
			(void) pfmt(stderr, MM_ERROR,
				":275:%s open failed\n", Hostfile);
		return(TRUE);
	}

	/* Get INQUIRY Data */
	inquiry(&Inquiry_data);

	return(FALSE);
}	/* scsi_open() */

scsi_nodfct(rabl_bufpt, rabl_bufsz)
int		rabl_bufpt[];
long		rabl_bufsz;
{
	int	i;
	struct	badsec_lst *blc_p;

	if (!badsl_chain) {
		badsl_chain = (struct badsec_lst *) malloc(BADSLSZ);
		blc_p = badsl_chain;
		if (!blc_p) 
			error(":293:scsi_nodfct: bad sector chain list broken.\n");

		memset(blc_p,0,BADSLSZ);
	} else {
		for (blc_p=badsl_chain; blc_p->bl_nxt; )
			blc_p = blc_p->bl_nxt;
	}

	for (i = 1; i < rabl_bufsz / sizeof(int); i++) {
		if (blc_p->bl_cnt == MAXBLENT) {
			blc_p->bl_nxt = (struct badsec_lst *) malloc(BADSLSZ);
			blc_p = blc_p->bl_nxt;
			if (!blc_p) 
				error(":293:scsi_nodfct: bad sector chain list broken.\n");

			memset(blc_p,0,BADSLSZ);
		}
		blc_p->bl_sec[blc_p->bl_cnt] = scl_swap32(rabl_bufpt[i]);
		blc_p->bl_cnt++;
	}

}
