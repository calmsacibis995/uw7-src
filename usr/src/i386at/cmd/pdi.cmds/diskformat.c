#ident	"@(#)pdi.cmds:diskformat.c	1.5.2.1"

#include 	"stdio.h"
#include	"utmp.h"

#ifndef i386
#include	"sys/pdi.h"
#include	"sys/extbus.h"
#endif /* ix86 */

#include	"sys/stat.h"
#include	"scl.h"
#include	"tokens.h"
#include	"sys/vtoc.h"		/* Included just to satisfy scsicomm.h */
#include	"scsicomm.h"

#include <errno.h>
#include <locale.h>
#include <pfmt.h>

#define NORMEXIT	0
#define USAGE		2
#define TRUE		1
#define FALSE		0
#if u3b2
#define SCSI_DIR	"/usr/lib/scsi/"
#define SCSI_DIR2 	"/usr/lib/scsi/format.d/"
#define	SUBUTILDIR	"/usr/lib/scsi/format.d/"
#elif i386
#define SCSI_DIR	"/etc/scsi/"
#define SCSI_DIR2 	"/etc/scsi/format.d/"
#define	SUBUTILDIR	"/etc/scsi/format.d/"
#else
#define SCSI_DIR	"/etc/scsi.d/"
#define SCSI_DIR2	"/etc/scsi.d/format.d/"
#define	SUBUTILDIR	"/etc/scsi.d/format.d/"
#endif /* u3b2 || ix86 */
#define	HOSTFILE	"HAXXXXXX"
#define	INDEXFILE	"/etc/scsi/tc.index"

/* KLUDGE: for now to resolve undefined references */

struct	disk_parms	dp;	/* Disk parameters returned from driver */
struct	vtoc		vtoc;	/* struct containing slice info */
struct	pdinfo		pdinfo; /* struct containing disk param info */

/* END KLUDGE */

extern FILE	*scriptfile_open();
extern int	errno;
void		error();
void		giveusage();

char	Devfile[128];	/* Device File name */
char	replybuf[160];	/* User's reply */
extern	int	Show;

extern do_format;
extern ign_check;
extern no_format;
extern verify;
extern do_unix;
extern int Debug = FALSE;
int gaugeflg = FALSE;

main(argc, argv)
int 	argc;
char 	*argv[];
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
	char		*label;

	/* Strip pathname preceding Cmdname */
	if ((name = strrchr(argv[0], '/')) == 0)
		name = argv[0];
	else
		name++;

	strcpy(Cmdname, name);

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxdisksetup");
	label = (char *) malloc(strlen(name)+1+3);
	sprintf(label, "UX:%s", name);
	(void) setlabel(label);

       /* Don't check arguments so future subutilities can now have options */


	/* The last argument must be the device file */
	if (argc <= 1)
		/* Not enough arguments */
		giveusage();

	(void) strcpy(Devfile, argv[argc-1]);

	/* Clear Hostfile variable */
	Hostfile[0] = '\0';

	/* Open the SCSI special device files */
	if (scsi_open(Devfile, HOSTFILE))
		error(":18:SCSI special device file open failed\n");

	/* Open the script file */
	if ((scriptfp = scriptfile_open(INDEXFILE)) == NULL)
		/* Script file cannot be opened so the device cannot be formatted */
		error(":19:Script file open failed\n");

	/* Get the subutility name from the scriptfile */
	if (get_string(scriptfp,subutil) < 0)
		/* no subutility string on first line of script file */
		error(":20:Could not find sub-utility name in script file.\n");

#ifdef NOT_USED
	/* Check to see if the subutility exists in the current directory */
	(void) strcpy(subutilfile, subutil);
	if (stat(subutilfile, &buf) < 0) {
		/* The subutility file doesn't exist in the current
		 * directory, so next check the subutility directory.
		 */
		errno = 0;
		(void) strcpy(subutilfile, SUBUTILDIR);
		(void) strcat(subutilfile, subutil);
		if (stat(subutilfile, &buf) < 0) {
			   /* could not find subutilfile */
			   error(":21:Could not find sub-utility.\n");
		}
	} 
#endif
	/* Close Host Adapter special device file */
	close(Hostfdes);

	/* Unlink the Host Adapter special device file */
	unlink(Hostfile);

	do_format = TRUE;
	ign_check = TRUE;
	no_format = FALSE;
	verify = FALSE;
	do_unix = FALSE;
	scsi_setup( argv[ argc - 1 ]);
}


void
giveusage()
{
	(void) pfmt(stderr, MM_ACTION,
		":22:Usage: %s [-t] [-i] /dev/rdsk/c?t?d?s0\n", Cmdname);

	exit(USAGE);
}	/* giveusage() */
