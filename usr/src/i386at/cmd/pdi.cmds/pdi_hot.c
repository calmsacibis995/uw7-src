#ident	"@(#)pdi.cmds:pdi_hot.c	1.2.3.1"
#ident	"$Header$"

#include <stdio.h>
#include <sys/lock.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/stat.h>
#include <sys/sysi86.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include <nl_types.h>	/* for nl_langinfo */
#include <langinfo.h>	/* for nl_langinfo, YESSTR/NOSTR */
#include <regex.h>
#include <sys/mkdev.h>
#include <sys/mod.h>


int hot_insert=0;		/* 1 if hot insertion */
int hot_remove=0;		/* 1 if hot removal */
int no_pause=0;			/* 1 if the bus should be paused */
char tempnode[]="/tmp/hotXXXXXX";

#define BUF_MAX 128		/* for fgets */

#undef makedev
#define makedev(major,minor)	(dev_t)(((major_t)(major) << 18 | (minor_t)(minor)))

/*
 * Given a file name, checks to see if the file exists and if
 * it does then returns the scsi_adr for that file.
 * 
 * Returns 0 on success, -1 on failure.
 */
int dev_to_addr(struct scsi_xedt *edtp, int edt_cnt,
		char *file, struct scsi_adr *sap)
{
	struct stat statbuf;
	major_t maj, edt_bmaj, edt_cmaj;
	minor_t min, edt_minor;
	dev_t edt_dev;
	int block;		/* block/character flag */

	if ((stat(file, &statbuf) < 0) ||
	    (((statbuf.st_mode & S_IFCHR) != S_IFCHR) &&
	     ((statbuf.st_mode & S_IFBLK) != S_IFBLK)))
	{
		return -1;
	}
	maj = major(statbuf.st_rdev);
	min = minor(statbuf.st_rdev);
	block = (statbuf.st_mode == S_IFBLK);

	for (; edt_cnt >= 0; edt_cnt--, edtp++)
	{
		edt_dev = makedev(edtp->xedt_bmaj, edtp->xedt_first_minor);
		edt_bmaj = major(edt_dev);
		edt_dev = makedev(edtp->xedt_cmaj, edtp->xedt_first_minor);
		edt_cmaj = major(edt_dev);
		edt_minor = minor(edt_dev);

		if ((((block ) && (edt_bmaj == maj)) ||
		    ((!block) && (edt_cmaj == maj))) &&
		    (edt_minor == ((min / edtp->xedt_minors_per) * edtp->xedt_minors_per)))
		{
			sap->scsi_ctl = edtp->xedt_ctl;
			sap->scsi_bus = edtp->xedt_bus;
			sap->scsi_target = edtp->xedt_target;
			sap->scsi_lun = edtp->xedt_lun;
			return 0;
		}
	}

	return -1;
}

/*
 * Given a device.tab name, checks to see if the file exists and if
 * it does then returns the scsi_adr for that file.
 * 
 * Returns 0 on success, -1 on failure.
 */
int name_to_addr(struct scsi_xedt *edtp, int edt_cnt,
		char *name, struct scsi_adr *sap)
{
	char *file;
	extern char *devattr();

	file = devattr(name, "cdevice");
	if (file == NULL)
	{
		return -1;
	}
	return dev_to_addr(edtp, edt_cnt, file, sap);

}

/*
 * Procedure:     yes
 *
 * Restrictions:
 *                 fgets: none
 */
int
yes(void)
{
	static char *yesstr, *nostr;
	static regex_t yesre;
	char resp[BUF_MAX];
	register size_t len;
	int err;
	char *fmt;

	if (yesstr == 0) {
		yesstr = nl_langinfo(YESSTR);
		nostr = nl_langinfo(NOSTR);
		err = regcomp(&yesre, nl_langinfo(YESEXPR),
			REG_EXTENDED | REG_NOSUB);
		if (err != 0) {
	badre:
			regerror(err, &yesre, resp, BUF_MAX);
			pfmt(stderr, MM_ERROR, "Regular Expression failure: %s\n",
			     resp);
			exit(2);
		}
	}

	pfmt(stderr, MM_NOSTD, ":10:Should this device be removed (%s/%s)?",
	     yesstr, nostr);

	resp[0] = '\0';
	(void)fgets(resp, BUF_MAX, stdin);
	len = strlen(resp);
	if (len && resp[len - 1] == '\n')
		resp[--len] = '\0';
	if (len) {
		err = regexec(&yesre, resp, (size_t)0, (regmatch_t *)0, 0);
		if (err == 0)
			return 1;
		if (err != REG_NOMATCH)
			goto badre;
	}
	return 0;
}

/*
 * int
 * disknumber()
 * dn is a pointer to a disknumber of the form
 * 	cWbXtYlZ
 * case is ignored.  Any pair may be left out except cW.
 * sap is filled in with the found values.  Any values
 * not specified are set to -1.
 * 
 * Returns 0 on success, -1 on incorrectly formed string
 * NOTE: This will let some incorrectly formed strings
 * through without error.  The addresses will simply have
 * mostly -1s.
 */
int disk_number(char *dn, struct scsi_adr *sap)
{
	char delimeter[]="CBTL";
	char *del;
	int address[4], *ap;
	int i;

	if ((dn == NULL) || (sap == NULL))
		return -1;

	/* get past white space */
	while ((*dn != '\0') && isspace(*dn))
		dn++;

	del = &(delimeter[0]);

	/* make sure that it starts with a 'C' */
	if ((*dn == '\0') || toupper(*dn) != *del)
		return -1;

	address[0] = address[1] = address[2] = address[3] = -1;

	for (ap = address;*del != '\0'; del++, ap++)
	{
		/* check if there is the correct delimiter */
		if ((*dn == '\0') || toupper(*dn) != *del)
			continue;

		/* yes there is the correct delimter */
		dn++;

		/* check if there is a number */
		if ((*dn == '\0') ||!isdigit(*dn))
		{
			/* no, incorrectly formed dn */
			return -1;
		}

		
		/* yes, its a number, extract it  */
		for (*ap = 0; isdigit(*dn) ; dn++)
		{
			*ap = *ap * 10 + (*dn - '0');
		}
	}

	sap->scsi_ctl = address[0];
	sap->scsi_bus = address[1];
	sap->scsi_target = address[2];
	sap->scsi_lun = address[3];

	return 0;
}

/*
 * int
 * make_sdi_dev()
 * returns file descriptor on success, -1 on failure
 */
int 
make_sdi_dev()
{
	dev_t	sdi_dev;
	int sdi_fd;
	char str[BUF_MAX];

	/* get device for sdi */
	if (sysi86(SI86SDIDEV, &sdi_dev) == -1) {
		perror("Unable to get SDI device node.");
		return -1;
	}

	mktemp(tempnode);

	if (mknod(tempnode, (S_IFCHR | S_IREAD), sdi_dev) < 0) {
		perror("Unable to make SDI device node name");
		return -1;
	}

	/*
	 * This open will no longer fail because we are using a
	 * special pass_thru major which is only for issuing sdi_ioctls.
	 * This open does not require exclusive use of the pass_thru
	 * to an HBA so there is no problem with it being in use.
	 */
	if ((sdi_fd = open(tempnode, O_RDONLY)) < 0) {
		unlink(tempnode);
		perror(tempnode);
		return -1;
	}
	return sdi_fd;
}

/*
 * void
 * cleanup_exit()
 * does any needed cleanup before calling exit(code).
 */
void cleanup_exit(int code)
{
	unlink(tempnode);
	exit(code);
}

/*
 *
 */
void
edt_print(struct scsi_xedt *edtp)
{
	pfmt(stdout, MM_INFO, ":1:\t%d:%d,%d,%d:%13.-13s:%s\n",
		edtp->xedt_ctl, edtp->xedt_bus,
		edtp->xedt_target, edtp->xedt_lun,
		edtp->xedt_drvname, edtp->xedt_tcinquiry);
}

/*
 * struct scsi_xedt *
 * edt_search()
 * returns edt on success or NULL on failure
 */
struct scsi_xedt *
edt_search(struct scsi_xedt *edtp, int edt_cnt, struct scsi_adr *sap)
{
	for (; edt_cnt; edt_cnt--, edtp++)
	{
		if (((sap->scsi_ctl == -1) ||
		     (sap->scsi_ctl == edtp->xedt_ctl)) &&
		    ((sap->scsi_bus == -1) ||
		     (sap->scsi_bus == edtp->xedt_bus)) &&
		    ((sap->scsi_target == -1) ||
		     (sap->scsi_target == edtp->xedt_target)) &&
		    ((sap->scsi_lun == -1) ||
		     (sap->scsi_lun == edtp->xedt_lun)))
		{
			/* its a match */
			return edtp;
		}
	}
	return NULL;
}

/*
 *
 */
void 
compare_edt(struct scsi_xedt *new_edtp, int new_edt_cnt,
	    struct scsi_xedt *old_edtp, int old_edt_cnt)
{
	int i;
	struct scsi_adr sa;

	for (i = 0; i < new_edt_cnt; i++)
	{
		sa.scsi_ctl = new_edtp[i].xedt_ctl;
		sa.scsi_bus = new_edtp[i].xedt_bus;
		sa.scsi_target = new_edtp[i].xedt_target;
		sa.scsi_lun = new_edtp[i].xedt_lun;
		if (edt_search(old_edtp, old_edt_cnt, &sa))
			continue;
		pfmt(stdout, MM_INFO, ":2:Device added\n");
		edt_print(&(new_edtp[i]));
	}

	for (i = 0; i < old_edt_cnt; i++)
	{
		sa.scsi_ctl = old_edtp[i].xedt_ctl;
		sa.scsi_bus = old_edtp[i].xedt_bus;
		sa.scsi_target = old_edtp[i].xedt_target;
		sa.scsi_lun = old_edtp[i].xedt_lun;
		if (edt_search(new_edtp, new_edt_cnt, &sa))
			continue;
		pfmt(stdout, MM_INFO, ":3:Device removed\n");
		edt_print(&(old_edtp[i]));
	}
}

/*
 * int
 * pause_bus()
 * returns 0 on success, -1 on failure
 */
int pause_bus(int fd, struct scsi_adr *sap, struct scsi_xedt *edtp, int edt_cnt)
{
	static char buffer[BUF_MAX];
	struct scsi_adr sa;
	int hba, hba_min, hba_max;
	int bus, bus_min, bus_max;
	int pause_cnt=0;	/* number of buses that have been paused */
	int continue_cnt=0;	/* number of buses that have been continued */

	sa = *sap;

	if (no_pause) return 0;


	if (sap->scsi_ctl != -1)
	{
		hba_min = sap->scsi_ctl;
		hba_max = sap->scsi_ctl + 1;
	}
	else
	{
		hba_min=0;
		hba_max = MAX_EXHAS;
	}

	if (sap->scsi_bus != -1)
	{
		bus_min = sap->scsi_bus;
		bus_max = sap->scsi_bus + 1;
	}
	else
	{
		bus_min=0;
		bus_max = MAX_BUS;
	}

	for (hba=hba_min; hba < hba_max; hba++)
	{
		for (bus=bus_min; bus < bus_max; bus++)
		{
			sa.scsi_ctl = hba;
			sa.scsi_bus = bus;
			sa.scsi_target = -1;
			sa.scsi_lun = -1;
			if (edt_search(edtp, edt_cnt, &sa) == NULL)
			{
				continue;
			}
			if (ioctl(fd, B_PAUSE, &sa) < 0) {
				continue;
			}
			pause_cnt++;
		}
	}

	if (pause_cnt == 0)
	{
		perror("Unable to pause SCSI bus");
		return -1;
	}

	pfmt(stdout, MM_INFO, ":4:Successfully stopped SCSI bus.\n");
	pfmt(stdout, MM_INFO, ":5:Hit return when device changes have been made.\n");
	
	fgets(buffer, BUF_MAX, stdin);

	for (hba=hba_min; hba < hba_max; hba++)
	{
		for (bus=bus_min; bus < bus_max; bus++)
		{
			sa.scsi_ctl = hba;
			sa.scsi_bus = bus;
			sa.scsi_target = -1;
			sa.scsi_lun = -1;
			if (edt_search(edtp, edt_cnt, &sa) == NULL) {
				continue;
			}
			if (ioctl(fd, B_CONTINUE, &sa) < 0) {
				continue;
			}
			continue_cnt++;
		}
	}

	if (pause_cnt != continue_cnt)
	{
		perror("Unable to continue SCSI bus");
		return -1;
	}

	pfmt(stdout, MM_INFO, ":6:Successfully continued SCSI bus\n");

	return 0;
}

void usage()
{
	pfmt(stderr, MM_INFO, ":7:Usage: pdi_hot -i|-r [-n] device-number\n"); 
}

/*
 * int
 * parse_args()
 * return 0 on success, -1 on failure
 */
int parse_args(struct scsi_xedt *edt, int edt_cnt,
	       int argc, char **argv, struct scsi_adr *sap)
{
	extern char *optarg;
	extern int optind;
	int c;
	int errflg=0;


	sap->scsi_ctl = -1;
	sap->scsi_bus = -1;
	sap->scsi_target = -1;
	sap->scsi_lun =  -1;

	while ((c = getopt(argc, argv, "irn")) != EOF)	{
		switch (c) {
		case 'i':	/* hot insertion */
			hot_insert=1;
			break;
		case 'r':
			hot_remove=1;
			break;
		case 'n':
			no_pause=1;
			break;
		default:
			errflg=1;
			break;
		}
	}

	if ((hot_insert == hot_remove) || errflg)
	{
		usage();
		return -1;
	}
			
	if (optind == argc)
	{
		return 0;
	}

	if ((optind + 1 == argc) &&
	    (name_to_addr(edt, edt_cnt, argv[optind], sap) == 0))
	{
		return 0;
	}

	if ((optind + 1 == argc) &&
	    (dev_to_addr(edt, edt_cnt, argv[optind], sap) == 0))
	{
		return 0;
	}

	if ((optind + 1 == argc) && (disk_number(argv[optind], sap) == 0))
	{
		return 0;
	}

	usage();
	return -1;
}
		

/*
 *  Return the SCSI Extended Equipped Device Table
 *	Inputs:  hacnt - pointer to integer to place the number of HA's.
 *	Return:  address of the XEDT
 *	         0 if couldn't read the XEDT
 */
struct scsi_xedt *
readxedt(int *edtcnt, int fd)
{
	struct	scsi_xedt *xedt;
	char 	sditempnode[]="/tmp/scsiXXXXXX";
	dev_t	sdi_dev;

	*edtcnt = 0;

	/*  Get the Number of EDT entries in the system  */
	if (ioctl(fd, B_EDT_CNT, edtcnt) < 0)  {
		perror("");
		return NULL;
	}

	if (*edtcnt == 0)	{
		pfmt(stderr, MM_ERROR, ":8:Unable to determine the "
			"number of EDT entries.\n");
		return NULL;
	}

	/*  Allocate space for SCSI XEDT  */
	if ((xedt = (struct scsi_xedt *)
	     calloc(1, sizeof(struct scsi_xedt) * *edtcnt)) == NULL)	{
		pfmt(stderr, MM_ERROR, ":9:Unable to allocate memory for EDT entries\n");
		return NULL;
	}

	/*  Read in the SCSI XEDT  */
	if (ioctl(fd, B_RXEDT, xedt) < 0)  {
		perror("");
		return NULL;
	}

	return(xedt);
}

/*
 * int
 * hot_remove_device()
 * returns -1 on failure, 0 on success
 */
int
hot_remove_device(int fd, struct scsi_adr *sap,
		  struct scsi_xedt *edtp, int edt_cnt)
{
	char *b;
	struct scsi_adr sa;
	int min_lun, max_lun, lun;
	int removed=0, mod_id;
	struct scsi_xedt *local_edtp;

	sa = *sap;

	if ((sa.scsi_ctl == -1) || (sa.scsi_target == -1))
		return -1;

	if (sa.scsi_bus == -1)
		sa.scsi_bus = 0;

	if (sa.scsi_lun == -1)
	{
		min_lun = 0;
		max_lun = MAX_EXLUS;
	}
	else
	{
		min_lun = sa.scsi_lun;
		max_lun = sa.scsi_lun + 1;
	}

	for (lun = min_lun; lun < max_lun; lun++)
	{
		sa.scsi_lun = lun;

		/* Check if device is there */
		if ((local_edtp = edt_search(edtp, edt_cnt, &sa)) == NULL)
			continue;

		/*
		 * Load the target driver, so that the target driver remove
		 * device routine (e.g. st01rm_dev()) will be available for
		 * B_RM_DEV ioctl processing.
		 */
		mod_id = modload(local_edtp->xedt_drvname);
#ifdef DEBUG
		fprintf(stderr, "modload(\"%s\") returns %d.\n",
			local_edtp->xedt_drvname, mod_id);
#endif
		if (!no_pause)
		{
			edt_print(local_edtp);
			
			if (yes())
			{
				if (!ioctl(fd, B_RM_DEV, &sa))
					removed++;
			}
		}
		else
		{
			if (ioctl(fd, B_RM_DEV, &sa))
				continue;
			else
				removed++;
		}

		moduload(mod_id);
	}

	if (removed)
		return 0;
	else
		return -1;
}

/*
 * int
 * hot_insert_device()
 * returns -1 on failure, 0 on success
 */
int
hot_insert_device(int fd, struct scsi_adr *sap)
{
	if (ioctl(fd, B_ADD_DEV, sap)) {
		return -1;
	}
	return 0;
}

main(int argc, char **argv)
{
	int sdi_fd;		/* file descriptor to sdi pass thru node */
	struct scsi_adr sa;
	struct scsi_xedt *old_edtp, *new_edtp;
	int old_edt_cnt, new_edt_cnt;

	setlocale(LC_ALL, "");
	setcat("uxpdi_hot");

	if ((sdi_fd = make_sdi_dev()) < 0)
	{
		cleanup_exit(1);
	}

	if ((old_edtp = readxedt(&old_edt_cnt, sdi_fd)) == NULL)
	{
		pfmt(stderr, MM_ERROR, ":12:Unable to read system configuration.\n");
		cleanup_exit(1);
	}

	if (parse_args(old_edtp, old_edt_cnt, argc, argv, &sa) < 0) {
		cleanup_exit(1);
	}
		
	/* if this isn't a terminal than force a -n option */
	if (!isatty(0))
		no_pause = 1;

	if (!no_pause && strcmp(ttyname(0), "/dev/console")) {
		pfmt(stderr, MM_ERROR, ":11:Program must be run from console.\n");
		cleanup_exit(1);
	}

	/* lock this process in memory to prevent paging */
	if (plock(PROCLOCK) < 0) {
		perror("Couldn't lock program into memory");
		cleanup_exit(1);
	}

	if (hot_insert)
	{
		if (pause_bus(sdi_fd, &sa, old_edtp, old_edt_cnt) < 0)
		{
#ifdef DEBUG
			fprintf(stderr, "pause_bus() failed\n");
#endif
			cleanup_exit(1);
		}

		if (hot_insert_device(sdi_fd, &sa) < 0)
		{
#ifdef DEBUG
			fprintf(stderr, "hot_insert_device() failed\n");
#endif
			cleanup_exit(1);
		}
	}

	if (hot_remove)
	{
		if (hot_remove_device(sdi_fd, &sa, old_edtp, old_edt_cnt) < 0)
		{
#ifdef DEBUG
			fprintf(stderr, "hot_remove_device() failed\n");
#endif
			cleanup_exit(1);
		}

		if (pause_bus(sdi_fd, &sa, old_edtp, old_edt_cnt) < 0)
		{
#ifdef DEBUG
			fprintf(stderr, "pause_bus() failed\n");
#endif
			cleanup_exit(1);
		}
	}


	if ((new_edtp = readxedt(&new_edt_cnt, sdi_fd)) == NULL)
	{
		pfmt(stderr, MM_ERROR, ":13:Unable to read new configuration.\n");
		cleanup_exit(1);
	}

	fprintf(stdout, "\n");

	compare_edt(new_edtp, new_edt_cnt, old_edtp, old_edt_cnt);


	system("/etc/scsi/pdimkdev -u -s");

	cleanup_exit(0);
}
