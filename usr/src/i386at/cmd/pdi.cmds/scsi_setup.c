#ident	"@(#)pdi.cmds:scsi_setup.c	1.18.7.1"

/*  "error()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 *
 *  "warning()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_WARNING>.
 */

/* INCLUDES */
#include	<stdio.h>
#include	<string.h>
#include	<errno.h>
#include	<sys/mnttab.h>
#include	<utmp.h>
#include	<signal.h>
#include	<unistd.h>

/*
 * The following four files are to be included for "scsihdefix" mark
 * "file system dirty" feature.
*/

#include	<sys/fs/s5macros.h>
#include	<sys/param.h>
#include	<sys/fs/s5param.h>
#include	<sys/filsys.h>
#include	<fcntl.h>
#include	<sys/vtoc.h>
#include	<sys/scsi.h>
#include	<sys/stat.h>
#include	"scl.h"
#include	"tokens.h"
#include	"badsec.h"
#include	<sys/fdisk.h>
#include	"scsicomm.h"
#include	<pfmt.h>
#include	"readxedt.h"
#include 	"info.h"

/* DEFINES */

#define NORMEXIT	0
#define USAGE		2
#define TRUE		1
#define FALSE		0
#define SCSI_DIR	"/etc/scsi/"
#define SCSI_DIR2 	"/etc/scsi/format.d/"
#define	SUBUTILDIR	"/etc/scsi/format.d/"
#define	HOSTFILE	"HAXXXXXX"
#define	INDEXFILE	"/etc/scsi/tc.index"
#define	MNTTAB		"/etc/mnttab"
#define	NDISKPARTS	2        /* # of disk partitions per mirror partition */
#define MEGABYTE	(1000000)
#define	FDBLKNO	0
#define sbp	((struct filsys *) badr)


extern char	*malloc(),
	   	*realloc();
extern void	chkstate(),
		error(),
		format_disk(),
		hdefix_disk(),
		free();

int inquiry_mode = 0;
int	giveusage();
extern	FILE	*scriptfile_open();
extern	struct	badsec_lst *badsl_chain;
extern	int	badsl_chain_cnt;
extern int	errno;
void		error();
extern struct	disk_parms dp;	/* Disk parameters returned from driver */

/* GLOBALS */

char		Devfile[128];	/* Device File name */
char		bDevfile[128];	/* block Device File name */
char		*badblock   = NULL;	/* Bad Block number	 */
unsigned	block[2];
long		unixst;
long		unixsz;
int		Silent = FALSE;
int		Show = FALSE;
int		fd;
int		no_map = FALSE;		/* TRUE if bad blocks not to be mapped*/
int		do_format = FALSE;	/* TRUE if format operation */
int		do_hdefix = FALSE;	/* TRUE if hdefix operation */
int		do_unix = FALSE;	/* TRUE if operation only on UNIX part*/
int		u_found = FALSE;	/* TRUE if UNIX partition not found */
int		d_restore = FALSE;	/* TRUE if defects restored from file */
int	 	d_save = FALSE;		/* TRUE if defects saved in a file */
int		d_print = FALSE;	/* TRUE if defect list goes to term */
int		mapblock = FALSE;	/* TRUE if a bad block to be mapped */
int		bad_boot = FALSE;	/* Boot Sector is good */
int		bad_vtoc = FALSE;	/* VTOC is good */
int		vtoc_ver = TRUE;	/* VTOC ver =1 not more than 16 slices*/
int		verify = 0;		/* TRUE if the disk is to be verified */
int		ign_check = FALSE;
int		no_format = FALSE;

/* Below two declarations were added for hdefix mark file system dirty feature */
char		*badr;
struct absio	f_absio, io_arg;
struct ipart	*ipart;
struct vtoc_ioctl	buf_vtoc;
struct scm	Rdd_cdb;

void
scsi_setup( devname )
char	*devname;
{
	char		subutil[128];	/* Sub-utility file name */
	char		subutilfile[256];
	char		*scriptfile;	/* Script file name */
	FILE		*scriptfp;	/* Script file file pointer */
	struct stat	buf;
	int		argc1;		/* To copy subutility name with "-h" option */
	char		*argv1[64];	/* To copy subutility name with "-h" option: copy argument list to local array "argv1[]" */
	char		hopt[3];	/* To copy subutility name with "-h" option: array to hold "-h\NULL" */
	char		*name, *strrchr();	/* To strip pathname preceding Cmdname */

	strcpy(Cmdname, name);

	/* Clear Hostfile variable */
	Hostfile[0] = '\0';

	/* Open the SCSI special device files */
	if (scsi_open(Devfile, HOSTFILE))
		error(":18:SCSI special device file open failed\n");

	/* Open the script file */
	if ((scriptfp = scriptfile_open(INDEXFILE)) == NULL)
		/* Script file cant be opened so the dev cannot be formatted */
		error(":19:Script file open failed\n");

	/* Get the subutility name from the scriptfile */
	if (get_string(scriptfp,subutil) < 0)
		/* no subutility string on first line of script file */
		error(":20:Could not find sub-utility name in script file.\n");

	/* Close Host Adapter special device file */
	close(Hostfdes);

	/* Unlink the Host Adapter special device file */
	unlink(Hostfile);

	disk( devname );
}

/*
 * scsiformat [ -t ] [ -i ] [ -S ] [ -[n] v [u] | -[n] V [u] ] /dev/rdsk/c?t?d?s0
 *
 * /etc/scsi/format.d/DISK is a subutility which physically formats the
 * media associated with a SCSI peripheral device.
 * The -n option suppresses formatting.
 * The -r option informs the utility that there is a list of defects in
 * the file named on the command line which is to be restored during the
 * format.
 * The -v option performs a surface analysis on the formatted SCSI
 * peripheral device and maps the newly encountered bad blocks.
 * Device is the path name of the SCSI peripheral device.
 * The -V option performs a surface analysis on the formatted SCSI
 * peripheral device without mapping the newly encountered bad blocks.
 * Device is the path name of the SCSI peripheral device.
 * The -u option, used with either -v or -V option, handles only the
 * UNIX partition of the disk.
 * The -t option provides the user with an elapsed time
 * message on the screen, while the device is formatting.
 * The -i option is used to ignore the user level.
 * The -S option turns on the diagnostic mode.
 *
 * scsihdefix [ -p ] [ -b  0x(blockno) ] [ -r | -s file ] /dev/rdsk/c?t?d?s0
 *
 * /etc/scsi/format.d/scsihdefix is a subutility which  gives the user the
 * capability of manually mapping the bad blocks on the media associated
 * with a SCSI peripheral device which are not otherwise automatically mapped.
 *
 * The -p option prints out a report showing the bad blocks mapped on the
 * specified device.
 * The -b option, followed by a block number in hex will map this block on the
 * specified device.
 * The -s option saves the defect table in "file."
 * The -r option restores the defect table from "file."
 * The -i option is used to ignore the user level.
 * The -S option turns on the diagnostic mode.
 *
 * Before executing this utility, any removable media must be placed in
 * the SCSI peripheral device and the SCSI peripheral device must be
 * ready for use.  
 *
 * Since bad block logging for SCSI devices is target controller unique
 * and peripheral device unique, there will be script files that provide
 * the necessary information to subutilities that will perform the
 * bad block logging for each target controller and peripheral device
 * combination.
 *
 * Each peripheral device will have its own subutility that will handle
 * the bad block logging for that device. This subutility name will
 * be found in the first line of the scriptfile.
 *
 * Must be super-user to use this utility.
 */


int
disk( devname )
char	*devname;
{
	FILE		*scriptfp;	/* Script file file pointer */
	extern char	*optarg;
	char		*defectfile = NULL;	/* Defect List file name */
	char		*scriptfile;	/* Script file name */
	char		*name, *strrchr();
	extern int	optind;
	int		c;
	int		bsfd;
	struct		badsec_lst *blc_p;

	(void) strcpy(Devfile, devname);

	/* Clear Hostfile variable */
	Hostfile[0] = '\0';

#ifndef MULTIPE
	if (!ign_check) {
		/* Check that the user is in the proper state */
		chkstate();
		sync();
	}
#endif	/* MULTIPE */

	/* Check for hdefix usage */
	if ((d_restore && (d_save || d_print || mapblock)) ||
			 (d_save && (d_print || mapblock)) || (d_print && mapblock))
		giveusage();

	/* Check for UNIX partition */
	/* format [ -v | -V [ -u ] ] option, hdefix [ -b ] option */
	ckunixpart(verify, mapblock);

	/* Open the SCSI special device files */
	if (scsi_open(Devfile, HOSTFILE))
		error(":18:SCSI special device file open failed\n");

	/* Open the script file */
	if ((scriptfp = scriptfile_open(INDEXFILE)) == NULL)
		/* Script file cannot be opened so the device cannot be formatted */
		error(":19:Script file open failed\n");


	if (do_hdefix)
	/* Handle bad blocks through hdefix */
		hdefix_disk(scriptfp, defectfile, d_print, d_restore, d_save, mapblock, u_found, vtoc_ver);

	if (do_format)
	/* Format the peripheral device */
	format_disk(scriptfp, defectfile, no_format, verify, do_unix);

	badsl_chain_cnt = 0;

	for(blc_p=badsl_chain; blc_p != NULL; blc_p=blc_p->bl_nxt) {
		badsl_chain_cnt += blc_p->bl_cnt;
	}

	/* Close Host Adapter special device file */
	close(Hostfdes);

	/* Unlink the Host Adapter special device file */
	unlink(Hostfile);
	
	/* Inform driver of an invalid VTOC when the disk is formatted or a bad VTOC is encountered during scsihdefix */
	if (!no_format || (d_restore || bad_vtoc)) {
		if (Show)
			(void) pfmt(stderr, MM_INFO,
				":289:Opening %s\n", Devfile);

		/* Open the special device file */
		if ((fd = open(Devfile, O_RDONLY)) < 0)
			error(":275:%s open failed\n", Devfile);

		if (Show)
			(void) pfmt(stderr, MM_ERROR,
				":303:Ioctl V_REMOUNT %s\n", Devfile);

		if (ioctl(fd, V_REMOUNT, 0) == -1) {
			close(fd);
			error(":304:%s V_REMOUNT failed\n", Devfile);
		}
		
		if (Show)
			(void) pfmt(stderr, MM_ERROR,
				":305:Closing %s\n", Devfile);

		/* Close the special device file */
		close(fd);
	}
	return;
}	/* main() */

void
chkstate()
{
	register struct utmp *up;
	int c;
	extern struct utmp *getutid();
	struct utmp runlvl;

	runlvl.ut_type = RUN_LVL;
	if ((up = getutid(&runlvl)) == NULL)
		error(":306:Cannot determine run state\n");
	c = up->ut_exit.e_termination;
	if (c != 'S' && c != 's')
		error(":307:Not in single user state\n");
}	/* chkstate() */

int
ckunixpart(verify, mapblock)
int	verify;
int	mapblock;
{
	char		*buf;
	int j;

	/* Handle only UNIX partition */

	if ((verify && do_unix) || mapblock) {

		if ((fd = open(Devfile, O_RDONLY)) == -1 ) {
			(void) pfmt(stderr, MM_ERROR,
				":311:Cannot open Device file: %s\n", Devfile);
			exit (1);
		}

		if ((buf=(char *)malloc(dp.dp_secsiz)) == NULL) {
			(void) pfmt(stderr, MM_ERROR,
				":9:malloc of buffer failed\n");
			(void) pfmt(stderr, MM_ERROR|MM_NOGET,
				"%s\n", strerror(errno));
			exit(3);
		}
		f_absio.abs_sec = FDBLKNO;
		f_absio.abs_buf = buf;
		if (ioctl(fd, V_RDABS, &f_absio) < 0) {
			if (verify && do_unix) {
				(void) pfmt(stderr, MM_ERROR,
					":312:V_RDABS failed:");
				pfmt(stderr, MM_NOGET|MM_ERROR,
					"%s\n", strerror(errno));
				close(fd);
				exit (1);
			}
			else if (mapblock) {
				(void) pfmt(stdout,MM_NOSTD,
					":313:\tAttempt to locate UNIX partition has failed;\n\thowever, will still map the Bad Block.\n");
				close(fd);
			}
		}

		if ( ((struct mboot *)buf)->signature != MBB_MAGIC ) {
			(void) pfmt(stdout, MM_NOSTD,
				":314:The magic word in the fdisk table is not sane.\n");
			close(fd);
			exit (1);
		}

		ipart = (struct ipart*)((struct mboot *)buf)->parts;

		/* Find active FDISK partition and check if it is a UNIX partition */

		for ( j = 0; j < FD_NUMPART; j++, ipart++ ) {
			if ( ipart->systid == UNIXOS ) {
				u_found = TRUE;
				unixst = ipart->relsect;
				unixsz = ipart->numsect;

				if (mapblock) {
					if (Show) {
					    (void)pfmt(stdout, MM_NOSTD,
						":315:UNIX partition found on the disk;\n");
					    (void)pfmt(stdout, MM_NOSTD,
						":316:\tstarting UNIX File System check for the Bad Block\n");
					}
					/* Read PD Sector to get logical start address and then read VTOC */
					if (rd_vtoc(Devfile, &buf_vtoc) == 0 ) {
					    (void)pfmt(stdout, MM_NOSTD,
						":317:\tInvalid PDINFO/VTOC for the UNIX partition;\n\thowever, will still map the Bad Block.\n");
						bad_vtoc = TRUE;
						vtoc_ver = FALSE;
					}
				} /* mapblock */
			} /* UNIX found */
		} /* for loop */	

		if (!u_found) {
			(void) pfmt(stdout, MM_NOSTD,
			    ":318:Disk does not have a UNIX system partition.\n");

			if (verify && do_unix) {
				close(fd);
				exit (1);
			}
		} /* UNIX not found */

		free(buf);
		close(fd);
	}
	return(0);
} /* ckunixpart */

void
format_disk(scriptfp, defectfile, no_format, verify, do_unix)
FILE	*scriptfp;
char	*defectfile;
int	no_format;
int	verify;
int	do_unix;

{
	char		*format_bufpt = NULL;
	char		*mdselect_bufpt = NULL;
	char		*mdsense_bufpt = NULL;
	char		string[MAX_LINE];
	char		*mdprt_bufpt;
	int		done = FALSE;	/* TRUE if done formatting */
	int		i;	/* used for counter */
	int		readcap_done = FALSE;
	int		size_found = FALSE;
	long		cyls;
	long		format_bufsz = 0;
	long		gapsz = 0;
	long		mdselect_bufsz = 0;
	long		mdsense_bufsz = 0;
	struct sense	sense_data;
	long		sec_cyl;
	DADF_T		*dadf = (DADF_T *) NULL;
	DISK_INFO_T	disk_info;
	DISK_INFO_T	tmp_disk_info;
	FORMAT_T	format_cdb;
	RDDG_T		*rddg = (RDDG_T *) NULL;
	struct capacity	readcap_bufpt;
	struct scm	readcap_cdb;
	struct scm	verify_cdb;
	struct scs	mdselect_cdb;
	struct scs	mdsense_cdb;
	long  verify_staddr;
	long  verify_sz;
	union 	io_arg ia;
	int	devicefdes;
	DISK_INFO_T	*diptr;
	int warnmsg = 0;

	/* Locate the disk section of the script file */
	while (!done) {
		switch (get_token(scriptfp)) {
		case EOF :
			error(":319:Script file missing DISK token\n");
		case DISK :
			done = TRUE;
			break;
		default :
			break;
		}
	}

	if ( !do_unix ) {	/* do entire disk */
		if (!no_format) {
			if (!verify)
				(void) pfmt(stdout, MM_NOSTD,
					":320:Format %s:\n", Devfile);
			else
				(void) pfmt(stdout, MM_NOSTD,
					":321:Format and Verify %s:\n", Devfile);
		} else {
			if (!verify)
				giveusage();
			else
				if (!Silent)
					(void) pfmt(stdout, MM_NOSTD,
						":322:Verify %s:\n", Devfile);
		}
	} else {
		if (!verify) /* do only UNIX part */
			giveusage();
		else {
			if (!no_format)
				(void) pfmt(stdout, MM_NOSTD,
					":323:Format and Verify (verify UNIX partition only) %s:\n",
						Devfile);
			else
				if (!Silent)
					(void) pfmt(stdout, MM_NOSTD,
						":324:Verify (UNIX partition only) %s:\n",
							Devfile);
		}
	}

	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);

	/* Format the disk based on the script file */
	if (inquiry_mode)
		verify = FALSE;
	done = FALSE;
	while (!done) {
		switch (get_token(scriptfp)) {
		case EOF :	/* Must be done */
			if (Show) {
				fprintf(stderr, "EOF\n");
			}
			done = TRUE;
			break;
		case DISKINFO :
			if (!readcap_done)
				error(":325:Script file missing READCAP Token\n");

			if (Show) {
				fprintf(stderr, "DISKINFO\n");
			}
			if (!size_found) {
				int	meg = (readcap_bufpt.cd_addr *
						readcap_bufpt.cd_len) / MEGABYTE;
				/* Read DISK INFO */
				if (get_data(scriptfp, (char *) &tmp_disk_info, DISK_INFO_SZ) != DISK_INFO_SZ)
					error(":326:Script file has insufficient data for Disk Information\n");

				diptr = &tmp_disk_info;
				diptr->di_tracks  = scl_swap16(diptr->di_tracks);
				diptr->di_sectors = scl_swap16(diptr->di_sectors);
				diptr->di_asec_t  = scl_swap16(diptr->di_asec_t);
				diptr->di_bytes   = scl_swap16(diptr->di_bytes);
				
				if (tmp_disk_info.di_size >= meg) {
					size_found = TRUE;
					if (rddg && dadf) {
						disk_info.di_size = tmp_disk_info.di_size;
						disk_info.di_gapsz = tmp_disk_info.di_gapsz;
						disk_info.di_sectors = (dadf->pg_bytes_s * dadf->pg_sec_t) / readcap_bufpt.cd_len;  
						disk_info.di_bytes = readcap_bufpt.cd_len;

					} else {
						(void) memcpy((char *) &disk_info, (char *) &tmp_disk_info, DISK_INFO_SZ);
					}
				}
			}
			break;	/* DISKINFO */
		case FORMAT :
			if (Show) {
				fprintf(stderr, "FORMAT\n");
			}
			if (!no_format && !defectfile) {
				long	disk_size;

				/* Read size of disk */
				if (get_data(scriptfp, (char *) &disk_size, sizeof(long)) != sizeof(long))
					error(":327:Script file has insufficient data for Format disk size\n");
				disk_size = scl_swap32(disk_size);
				if (Show) {
					(void) printf("%d\n", disk_size);
				}

				if (rddg && dadf) {
					/* Calculate number of MB */
					disk_size = (long) RD_PG_CYL(rddg)
						  * (long) disk_info.di_tracks
						  * (long) disk_info.di_sectors
						  * (long) disk_info.di_bytes
						  / (long) MEGABYTE;
				} else if (!disk_size) {
					error(":328:Format: Unknown disk size\n");
				}

				/* Read FORMAT CDB */
				if (get_data(scriptfp, FORMAT_AD(&format_cdb), FORMAT_SZ) != FORMAT_SZ)
					error(":329:Script file has incomplete Format CDB\n");

				if (format_cdb.ss_fd == 1) {
					/* Allocate memory to hold the defect list header */
					format_bufpt = malloc(DLH_SZ);
					format_bufsz = DLH_SZ;

					/* Read WRITE DEFECT LIST HEADER */
					if (get_data(scriptfp, format_bufpt, DLH_SZ) != DLH_SZ)
						error(":330:Script file has incomplete Format Defect List Header\n");
				}

				format_cdb.ss_lun = LUN(Hostdev);

				/* Send the FORMAT CDB */
				format(format_cdb, format_bufpt, format_bufsz, disk_size);

				/* Release allocated memory */
				if (format_bufsz > 0)
					free(format_bufpt);
			}
			break;	/* FORMAT */
		case FORMAT_DEFECTS :
			if (Show) {
				fprintf(stderr, "FORMAT_DEFECTS\n");
			}
			if (!no_format && defectfile) {
				long	disk_size;

				/* Read size of disk */
				if (get_data(scriptfp, (char *) &disk_size, sizeof(long)) != sizeof(long))
					error(":331:Script file has insufficient data for Format with Defects disk size\n");

				disk_size = scl_swap32(disk_size);
				if (rddg && dadf) {
					/* Calculate number of MB */
					disk_size = (long) RD_PG_CYL(rddg)
						  * (long) disk_info.di_tracks
						  * (long) disk_info.di_sectors
						  * (long) disk_info.di_bytes
						  / (long) MEGABYTE;
				} else if (!disk_size) {
					error(":332:Format with Defects: Unknown disk size\n");
				}

				/* Read FORMAT with DEFECTS CDB */
				if (get_data(scriptfp, FORMAT_AD(&format_cdb), FORMAT_SZ) != FORMAT_SZ)
					error(":333:Script file has incomplete Format with Defects CDB\n");
	
				/* Allocate memory to hold the defect list header */
				format_bufpt = malloc(DLH_SZ);
				format_bufsz = DLH_SZ;
	
				/* Read WRITE DEFECT LIST HEADER */
				if (get_data(scriptfp, format_bufpt, DLH_SZ) != DLH_SZ)
					error(":334:Script file has incomplete Format with Defects Defect List Header\n");

				/* Read the defect list from the restore file */
				get_defect(defectfile, &format_bufpt, &format_bufsz);

				format_cdb.ss_lun = LUN(Hostdev);

				/* Send the FORMAT with DEFECTS CDB */
				format(format_cdb, format_bufpt, format_bufsz, disk_size);

				/* Release allocated memory */
				free(format_bufpt);
			}
			break;	/* FORMAT_DEFECTS */
		case MDSELECT :
			if (Show) {
				fprintf(stderr, "MDSELECT\n");
			}
			/* Read MODE SELECT CDB */
			if (get_data(scriptfp, SCS_AD(&mdselect_cdb), SCS_SZ) != SCS_SZ)
				error(":335:Script file has incomplete Mode Select CDB\n");

			mdselect_cdb.ss_lun = LUN(Hostdev);
			mdselect_bufsz = mdselect_cdb.ss_len;

			if (mdselect_bufsz > 0) {
				/* Allocate memory for MODE SELECT Data */
				mdselect_bufpt = malloc(mdselect_bufsz);

				/* Read MODE SELECT Data */
				if (get_data(scriptfp, mdselect_bufpt, mdselect_bufsz) != mdselect_bufsz)
					error(":336:Script file has insufficient Mode Select Data\n");
			}

			/* Send the MODE SELECT CDB */
			mdselect(mdselect_cdb, mdselect_bufpt, mdselect_bufsz);

			/* Release allocated memory */
			if (mdselect_bufsz > 0)
				free(mdselect_bufpt);
			break;	/* MDSELECT */
		case MDSENSE :
			if (Show) {
				fprintf(stdout, "MDSENSE\n");
			}
			/* Read MODE SENSE CDB */
			if (get_data(scriptfp, SCS_AD(&mdsense_cdb), SCS_SZ) != SCS_SZ)
				error(":337:Script file has incomplete Mode Sense CDB\n");

			mdsense_cdb.ss_lun = LUN(Hostdev);
			mdsense_bufsz = mdsense_cdb.ss_len;

			if (mdsense_bufsz > 0) {
				/* Allocate memory for MODE SENSE Data */
				mdsense_bufpt = malloc(mdsense_bufsz);
			}

			/* Send the MODE SENSE CDB */
			sense_data.sd_key = 0;
			mdsense(mdsense_cdb, mdsense_bufpt, mdsense_bufsz, &sense_data);
			if(sense_data.sd_key != SC_NOSENSE) {
				if(Show) {
					pfmt(stderr, MM_ERROR, ":338:format_disk: mdsense sense key %x\n", sense_data.sd_key);
				}
				if(!no_format) {
					exit(ERREXIT);
				}
			}
			if (mdsense_bufsz > 0) {
				switch (PC_TYPE(&mdsense_cdb)) {
				case PC_DADF :
				if(sense_data.sd_key) {
					if (inquiry_mode) {
						close(Hostfdes);
						unlink(Hostfile);
						exit(3);
					}
                                        /*
					if(!warnmsg) {
						warning(":339:Mode Sense, Format Device Page(PC 3), not supported on this drive.\n\t\tSurface analysis will be done one sector at a time.\n\n");
						pfmt(stdout, MM_NOSTD, ":340:Again, do you wish to skip surface analysis? (y/n) ");
						if (yes_response()) {
							verify = FALSE;
						}
					}
					warnmsg = 1;
                                        */
					verify = FALSE;
					break;
				}
				dadf = (DADF_T *) (mdsense_bufpt + SENSE_PLH_SZ +
							((SENSE_PLH_T *) mdsense_bufpt)->plh_bdl);

				dadf->pg_trk_z   = scl_swap16(dadf->pg_trk_z);
				dadf->pg_asec_z  = scl_swap16(dadf->pg_asec_z);
				dadf->pg_atrk_z  = scl_swap16(dadf->pg_atrk_z);
				dadf->pg_atrk_v  = scl_swap16(dadf->pg_atrk_v);
				dadf->pg_sec_t   = scl_swap16(dadf->pg_sec_t);
				dadf->pg_bytes_s = scl_swap16(dadf->pg_bytes_s);
				dadf->pg_intl    = scl_swap16(dadf->pg_intl);
				disk_info.di_sectors = dadf->pg_sec_t;
				disk_info.di_asec_t  = dadf->pg_asec_z;
				disk_info.di_bytes   = dadf->pg_bytes_s;
				break;

				case PC_RDDG :
				if(sense_data.sd_key) {
					if (inquiry_mode) {
						close(Hostfdes);
						unlink(Hostfile);
						exit(3);
					}
                                        /*
					if(!warnmsg) {
						warning(":341:Mode Sense, Rigid Disk Geometry Page(PC 4), not supported on this drive.\n\t\tSurface analysis will be done one sector at a time.\n\n");
						pfmt(stdout, MM_NOSTD, ":340:Again, do you wish to skip surface analysis? (y/n) ");
						if (yes_response()) {
							verify = FALSE;
						}
					}
					warnmsg = 1;
                                        */
					verify = FALSE;
					break;
				}
				rddg = (RDDG_T *) (mdsense_bufpt + SENSE_PLH_SZ +
							((SENSE_PLH_T *) mdsense_bufpt)->plh_bdl);
				rddg->pg_cylu = scl_swap16(rddg->pg_cylu);
				rddg->pg_wrpcompu = scl_swap16(rddg->pg_wrpcompu);
				rddg->pg_redwrcur = scl_swap24(rddg->pg_redwrcur);
				disk_info.di_tracks = rddg->pg_head;
				break;
				default :
					error(":342:Unknown Page Code Specified (0x%X)\n",
					    PC_TYPE(&mdsense_cdb));
				}
			}
			if (inquiry_mode) {
				if (dadf && rddg) {
					close(Hostfdes);
					unlink(Hostfile);
					exit(2);
				}
			}
			break;	/* MDSENSE */
		case READCAP :
			if (Show) {
				fprintf(stderr, "READCAP\n");
			}
			/* Read READ CAPACITY CDB */
			if (get_data(scriptfp, SCM_AD(&readcap_cdb), SCM_SZ) != SCM_SZ)
				error(":343:Script file has incomplete Read Capacity CDB\n");

			readcap_cdb.sm_lun = LUN(Hostdev);

			/* Send the READ CAPACITY CDB */
			readcap(readcap_cdb, (char *) &readcap_bufpt, CAPACITY_SZ);
			readcap_bufpt.cd_addr = scl_swap32(readcap_bufpt.cd_addr);
			readcap_bufpt.cd_len = scl_swap32(readcap_bufpt.cd_len);
			readcap_done = TRUE;
			break;	/* READCAP */
		case VERIFY :
			if (Show) {
				fprintf(stderr, "VERIFY\n");
			}
			if (verify) {
				if (size_found) {
					/* Read VERIFY CDB */
					if (get_data(scriptfp, SCM_AD(&verify_cdb), SCM_SZ) != SCM_SZ)
						error(":344:Script file has incomplete Verify CDB\n");

					verify_cdb.sm_lun = LUN(Hostdev);

					/* Send the VERIFY CDB */

					if (!do_unix) {
						verify_staddr = 0;
						verify_sz = readcap_bufpt.cd_addr;
					} else {
						verify_staddr = unixst;
						verify_sz = unixsz;
				   	}

					scsi_verify(verify_cdb, verify_staddr, verify_sz, disk_info.di_tracks *
									(disk_info.di_sectors - disk_info.di_asec_t), no_map);

				} else
					error(":345:Verify: Unknown disk size\n");
			}
			break;	/* VERIFY */
		case UNKNOWN :	/* Don't care so skip over */
			if (Show) {
				fprintf(stderr, "UNKNOWN\n");
			}
			(void) get_string(scriptfp, string);
			break;
		default :
			break;
		}
	}

}	/* format_disk() */


void
hdefix_disk(scriptfp, defectfile, d_print, d_restore, d_save, mapblock, u_found, vtoc_ver)
FILE	*scriptfp;
char	*defectfile;
int	d_print;
int	d_restore;
int	d_save;
int	mapblock;
int	u_found;
int	vtoc_ver;

{
	FILE		*savefp;
	FILE		*restorefp;
	char		*rdd_bufpt = NULL;
	char		*format_bufpt = NULL;
	char		*mdsense_bufpt = NULL;
	char		string[MAX_LINE];
	int		done = FALSE;	/* TRUE if done formatting */
	int		i;	/* integer used for count */
	long		rdd_bufsz;
	long		format_bufsz = 0;
	long		mdsense_bufsz = 0;
	struct scs	mdsense_cdb;
	struct sense	sense_data;
	DADF_T		*dadf = (DADF_T *) NULL;
	RDDG_T		*rddg = (RDDG_T *) NULL;
	DISK_INFO_T	disk_info;
	DISK_INFO_T	tmp_disk_info;
	FORMAT_T	format_cdb;


	/* Added for mark file system dirty feature */
	daddr_t		pend, pbno, sbbno;
	register int	sz = dp.dp_secsiz;
	register int	fspart;
	long		fssize,	rootino;


	if ( !vtoc_ver ) {
		(void) pfmt(stderr, MM_ERROR,
			":346:does not support version %d of VTOC\n",
				"<blank>");
		close(Hostfdes);
		unlink(Hostfile);
		exit(1);
	}

	if (mapblock) {
		char	*bufpt;
		uint	block[2];

		bufpt = malloc(sz);
		block[0] = 4;
		sscanf(badblock, "%x", &block[1]);
		
		if (u_found) {
			/* Check to see if the badblock is in the "sacred area" of the disk */

			if (block[1] > unixst && block[1] <= (unixst + 28)) {
				(void) pfmt(stderr, MM_ERROR,
				    ":347:CRITICAL PROBLEM: Bad Block is in the BOOT sector\n");
				bad_boot = TRUE;
			}

			if (block[1] == (unixst + 29)) {
				(void) pfmt(stderr, MM_ERROR,
				    ":348:CRITICAL PROBLEM: Bad Block is in the VTOC sector\n");
				bad_vtoc = TRUE;
			}
			
			if (!bad_boot && !bad_vtoc) {

				/* Allocate memory to read from super block to filsys structure */
				if (!(badr = malloc(sz))) {
					(void) pfmt(stderr, MM_ERROR,
					    ":349:malloc of \"Super Block buffer\" failed\n");
					exit(1);
				}

				/* Check to see where the block is in a partition */
				fspart = 0;

				for (i = 1; i < (int) (buf_vtoc.v_nslices); ++i) {  	/* Loop through partitions; ignore partition 0 */
					if (buf_vtoc.v_slices[i].p_size <= 0)
						continue;

					if (buf_vtoc.v_slices[i].p_flag & V_UNMNT)
						continue;

					if (buf_vtoc.v_slices[i].p_flag & V_VALID && (buf_vtoc.v_slices[i].p_tag == V_ROOT ||
						buf_vtoc.v_slices[i].p_tag == V_USR || buf_vtoc.v_slices[i].p_tag == V_SWAP)) {
						pend = buf_vtoc.v_slices[i].p_start + buf_vtoc.v_slices[i].p_size;

						if (buf_vtoc.v_slices[i].p_start > block[1] || pend <= block[1])
							continue;

						if (buf_vtoc.v_slices[i].p_tag == V_SWAP) {
						    (void)pfmt(stderr, MM_ERROR,
							":350:CRITICAL PROBLEM: Bad Block is in the SWAP partition %d\n",
								i);
							break;
						} else {
						    (void)pfmt(stderr, MM_ERROR,
							":351:CRITICAL PROBLEM: Bad Block in UNIX partition %d.\n",
								i);
                    					pbno = block[1] - buf_vtoc.v_slices[i].p_start;

						    (void)pfmt(stdout, MM_NOSTD,
							    ":352:\tIt is block %d in the partition.\n",
								pbno);

							/* Read the super block of the file system */
							sbbno = buf_vtoc.v_slices[i].p_start + 1;

							/* Bad Block is the Super Block */
							if (scsi_read(sbbno, badr, sz) == 1) {
								if (pbno == 0 || pbno == 1) {
								    (void)pfmt(stdout, MM_NOSTD,
									":353:\tIf the partition %d contained a file system,\n\tthat file system's superblock was lost!\n", i);

									continue;
								}
								(void)pfmt(stdout, MM_NOSTD,
								    ":354:\tCannot read potential superblock sector of partition.\n");

								(void) pfmt(stderr, MM_WARNING,
								    ":355:If a file system exists on partition %d, run Fsck on the file system\n", i);
								continue;
							}

							/* Check the MAGIC number of the file system */
							if (sbp->s_magic != FsMAGIC) {
								if (Show)
								    (void)pfmt(stdout, MM_NOSTD,
									":356:File system has a bad MAGIC number\n");

								continue;
							}

							/* The following code is commented out for the time being for File System dependency */

							/* fssize = sbp->s_fsize;
							rootino = 2;

							if (sbp->s_type == Fs1b || sbp->s_type == Fs2b || sbp->s_type == Fs4b) {
								if (sbp->s_type == Fs2b) {
									fssize *= FsBSIZE(2*dp.dp_secsiz)/dp.dp_secsiz;
									rootino *= FsBSIZE(2*dp.dp_secsiz)/dp.dp_secsiz;
								}

								if (sbp->s_type == Fs4b) {
									fssize *= FsBSIZE(4*dp.dp_secsiz)/dp.dp_secsiz;
									rootino *= FsBSIZE(4*dp.dp_secsiz)/dp.dp_secsiz;
								}

								if (pbno >= fssize) {
									(void) printf("\tBad Block was past end of file system.\n");
									continue;
								}

								(void) printf("\tPartition has a file system containing the bad block.\n");


							} else
								(void) printf("\tCannot calculate the end of this type of file system.\n"); */

							/* Mark the file system "dirty" */
							/* sbp->s_state = FsBADBLK;
							fspart++;
							(void) printf("\tMarking file system dirtied by bad block handling.\n"); */

							/* Write the "marked" superblock */
							/* scsi_write(sbbno, badr, sz); */

							/* File system type check */
							/* if (sbp->s_type != Fs1b && sbp->s_type != Fs2b && sbp->s_type != Fs4b) {
								(void) printf("\tFile system is type %d and I don't know how to do anymore!\n",
																sbp->s_type);
								continue;
							} */

							/* Check to see if the Bad Block is in the inode area */
							/* fssize = 2 + sbp->s_isize;
							if (sbp->s_type == Fs2b)
								fssize *= FsBSIZE(2*dp.dp_secsiz)/dp.dp_secsiz;
							if (sbp->s_type == Fs4b)
								fssize *= FsBSIZE(4*dp.dp_secsiz)/dp.dp_secsiz;

							if(pbno < fssize) {
								(void) printf("\tThe Bad Block is an inode block.\n");

								if (pbno == rootino)
									(void) printf("\tThe root inode of the file system was lost.\n");
							} else
								(void) printf("\tThe Bad Block was a file block.\n"); */
						} /* good_swap */
					}

					/* if (fspart > 1) {
						(void) printf("WARNING: %d file systems claimed to contain the Bad Block;\n", fspart);
						(void) printf("all but one claim should be false, but I marked all!\n");
					} */
				} /* for loop */
				free(badr);
			} /* good boot and good vtoc */
		} /* u_found */

		/* Go ahead and map the Bad Block */
		if (!Silent)
			(void) pfmt(stdout, MM_NOSTD,
				":357:Mapping Bad Block 0x%X\n", block[1]);
		block[0] = scl_swap32(block[0]);
		block[1] = scl_swap32(block[1]);
		reassign((char *) block, 8);
		sscanf(badblock, "%x", &block[1]);
		scsi_write((long) block[1], bufpt, sz);
		free(bufpt);
		close(Hostfdes);
		unlink(Hostfile);
		exit(NORMEXIT);
	}


	/* Locate the disk section of the script file */
	while (!done) {
		switch (get_token(scriptfp)) {
		case EOF :
			error(":319:Script file missing DISK token\n");
		case DISK :
			done = TRUE;
			break;
		default :
			break;
		}
	}

	if (d_restore) {
		(void) pfmt(stdout, MM_NOSTD,
		    ":358:Restoring defects from the restore file: \"%s\"\n",
			defectfile);
		(void) pfmt(stdout, MM_NOSTD,
		    ":359:Scsihdefix %s:\n", Devfile);
		(void) pfmt(stdout, MM_NOSTD,
		    ":360:(DEL if wrong)\n");
		(void) sleep(10);
		(void) signal(SIGHUP, SIG_IGN);
		(void) signal(SIGINT, SIG_IGN);
		(void) signal(SIGQUIT, SIG_IGN);
		(void) signal(SIGTERM, SIG_IGN);
	}

	/* Map the defects for hdefix */
	done = FALSE;
	while (!done) {
		switch (get_token(scriptfp)) {
		case EOF :	/* Must be done */
			if (Show) {
				pfmt(stderr, MM_NOGET|MM_NOSTD,
					"EOF\n");
			}
			done = TRUE;
			break;
		case MDSENSE :
			if (Show) {
				pfmt(stdout, MM_NOGET|MM_NOSTD,
					"MDSENSE\n");
			}
			/* Read MODE SENSE CDB */
			if (get_data(scriptfp, SCS_AD(&mdsense_cdb), SCS_SZ) != SCS_SZ)
				error(":337:Script file has incomplete Mode Sense CDB\n");

			mdsense_cdb.ss_lun = LUN(Hostdev);
			mdsense_bufsz = mdsense_cdb.ss_len;

			if (mdsense_bufsz > 0) {
				/* Allocate memory for MODE SENSE Data */
				mdsense_bufpt = malloc(mdsense_bufsz);
			}

			/* Send the MODE SENSE CDB */
			mdsense(mdsense_cdb, mdsense_bufpt, mdsense_bufsz, &sense_data);
			if(sense_data.sd_key != SC_NOSENSE) {
				if(Show) {
					pfmt(stdout, MM_INFO, ":361:hdefix_disk: mdsense sense key %x\n", sense_data.sd_key);
				}
				exit(0);
			}

			if (mdsense_bufsz > 0) {
				switch (PC_TYPE(&mdsense_cdb)) {
				case PC_DADF :
				dadf = (DADF_T *) (mdsense_bufpt + SENSE_PLH_SZ +
							((SENSE_PLH_T *) mdsense_bufpt)->plh_bdl);

				dadf->pg_trk_z   = scl_swap16(dadf->pg_trk_z);
				dadf->pg_asec_z  = scl_swap16(dadf->pg_asec_z);
				dadf->pg_atrk_z  = scl_swap16(dadf->pg_atrk_z);
				dadf->pg_atrk_v  = scl_swap16(dadf->pg_atrk_v);
				dadf->pg_sec_t   = scl_swap16(dadf->pg_sec_t);
				dadf->pg_bytes_s = scl_swap16(dadf->pg_bytes_s);
				dadf->pg_intl    = scl_swap16(dadf->pg_intl);
				disk_info.di_sectors = dadf->pg_sec_t;
				disk_info.di_asec_t  = dadf->pg_asec_z;
				disk_info.di_bytes   = dadf->pg_bytes_s;
				break;

				case PC_RDDG :
				rddg = (RDDG_T *) (mdsense_bufpt + SENSE_PLH_SZ +
							((SENSE_PLH_T *) mdsense_bufpt)->plh_bdl);
				rddg->pg_cylu = scl_swap16(rddg->pg_cylu);
				rddg->pg_wrpcompu = scl_swap16(rddg->pg_wrpcompu);
				rddg->pg_redwrcur = scl_swap24(rddg->pg_redwrcur);
				disk_info.di_tracks = rddg->pg_head;
				break;
				default :
					error(":342:Unknown Page Code Specified (0x%X)\n",
						PC_TYPE(&mdsense_cdb));
				}
			}
			break;	/* MDSENSE */
		case FORMAT_DEFECTS :
			if (Show) {
				pfmt(stderr, MM_NOGET|MM_NOSTD,
					"FORMAT_DEFECTS\n");
			}
			if (d_restore) {
				long	disk_size;

				/* Read size of disk */
				if (get_data(scriptfp, (char *) &disk_size, sizeof(long)) != sizeof(long))
					error(":331:Script file has insufficient data for Format with Defects disk size\n");

				disk_size = scl_swap32(disk_size);
				if (rddg && dadf) {
					/* Calculate number of MB */
					disk_size = (long) RD_PG_CYL(rddg)
						  * (long) disk_info.di_tracks
						  * (long) disk_info.di_sectors
						  * (long) disk_info.di_bytes
						  / (long) MEGABYTE;
				} else if (!disk_size) {
					error(":332:Format with Defects: Unknown disk size\n");
				}

				/* Read FORMAT with DEFECTS CDB */
				if (get_data(scriptfp, FORMAT_AD(&format_cdb), FORMAT_SZ) != FORMAT_SZ)
					error(":333:Script file has incomplete Format with Defects CDB\n");
	
				/* Allocate memory to hold the defect list header */
				format_bufpt = malloc(DLH_SZ);
				format_bufsz = DLH_SZ;
	
				/* Read WRITE DEFECT LIST HEADER */
				if (get_data(scriptfp, format_bufpt, DLH_SZ) != DLH_SZ)
					error(":334:Script file has incomplete Format with Defects Defect List Header\n");

				/* Read the defect list from the restore file */
				get_defect(defectfile, &format_bufpt, &format_bufsz);

				format_cdb.ss_lun = LUN(Hostdev);

				/* Send the FORMAT with DEFECTS CDB */
				format(format_cdb, format_bufpt, format_bufsz, disk_size);

				/* Release allocated memory */
				free(format_bufpt);
			}
			break;	/* FORMAT_DEFECTS */
		case RDDEFECT :
			if (Show) {
				pfmt(stderr, MM_NOGET|MM_NOSTD,
					"RDDEFECT\n");
			}
			if (d_save) {
				/* Read READ DEFECT LIST CDB */
				if (get_data(scriptfp, SCM_AD(&Rdd_cdb), SCM_SZ) != SCM_SZ) {
					/* Close script file */
					fclose(scriptfp);

					error(":362:Script file has incomplete Read Defect List CDB\n");
				}

				Rdd_cdb.sm_lun = LUN(Hostdev);

				/* Read defect list from the disk */
				readdefects(Rdd_cdb, &rdd_bufpt, &rdd_bufsz);

				/* Open Save File */
				if ((savefp = fopen(defectfile, "w")) == NULL) {
					/* Close script file */
					fclose(scriptfp);

					error(":363:Unable to create save file: \"%s\"\n",
						defectfile);
				}

				/* Write Defect Data to the Save File */
				if (rdd_bufpt != NULL)
					put_defect(savefp, rdd_bufpt);
				else {
					/* Close save file */
					fclose(savefp);

					error(":364:Save unsuccessful\n");
				}
				/* Close Save File */
				fclose(savefp);

				(void) pfmt(stderr, MM_ERROR,
					":365:Save successful\n");
			}

			if (d_print) {
				/* Read READ DEFECT LIST CDB */
				if (get_data(scriptfp, SCM_AD(&Rdd_cdb), SCM_SZ) != SCM_SZ) {
					/* Close script file */
					fclose(scriptfp);

					error(":362:Script file has incomplete Read Defect List CDB\n");
				}

				Rdd_cdb.sm_lun = LUN(Hostdev);

				/* Read defect list from the disk */
				readdefects(Rdd_cdb, &rdd_bufpt, &rdd_bufsz);

				(void) putc('\n', stderr);
				(void) pfmt(stdout, MM_ERROR,
				    ":366:Bad blocks for Device %s : \n", Devfile);
				/* Write Defect Data to the terminal */
				if(rdd_bufpt != NULL)
					put_defect(stdout, rdd_bufpt);
			}
			break;	/* RDDEFECT */
		case DISKINFO :
		case FORMAT :
		case MDSELECT :
		case READCAP :
		case VERIFY :
		case UNKNOWN :	/* Don't care so skip over */
			if (Show) {
				pfmt(stderr, MM_NOGET|MM_NOSTD,
					"UNKNOWN\n");
			}
			(void) get_string(scriptfp, string);
			break;
		default:
			break;
		}
	}
	/* hdefix_disk() */
}
