#ident	"@(#)pdi.cmds:boot_mkdev.c	1.16.5.1"

/*
 * boot_mkdev
 *
 * boot_mkdev is a utility that creates device nodes for
 * PDI devices for the boot floppy.  It gets the major numbers
 * by examining the edt. The device names that are created are
 * hard-coded into this program.
 *
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/mkdev.h>
#include	<sys/statfs.h>
#include	<ctype.h>
#include	<sys/stat.h>
#include	<sys/signal.h>
#include	<mac.h>
#include	<sys/fcntl.h>
#include	<sys/buf.h>
#include	<sys/vtoc.h>
#include	<string.h>

#include	<unistd.h>
#include	<sys/vfstab.h>
#include	<sys/fdisk.h>
#include	<sys/scsi.h>
#include	<sys/sdi_edt.h>
#include	<sys/sdi.h>
#include	"edt_sort.h"
#include	"scsicomm.h"
#include	"fixroot.h"

#define BLANK		'-'
#define DEV_MODE	0666	/* owner root */
#define DEV_UID		0	/* owner root */
#define DEV_GID		3	/* group sys */
#define DIRMODE		0775
#define	MAXLINE		256
#define MAXMAJOR	255
#define	MAXTCTYPE	32
#define	BIG		017777777777L
#define	BIG_I		2000

struct scsi_addr{
	unsigned int	sa_c;		/* controller occurrence        */
	unsigned int	sa_t;		/* target controller number     */
	unsigned int	sa_l;		/* logical unit number          */
	unsigned int	sa_b;		/* bus number                   */
	unsigned int	sa_n;		/* admin name unit number       */
};

extern int	errno;
extern char	*optarg;
extern EDT *readxedt(int *);

#ifdef DEBUG
static int	Debug;
#endif
static char	*DevRoot;

typedef struct template {
	char *dev;
	char *map;
} template_t;

typedef struct device {
	int	pdtype;
	int	count;
	char *bdir;
	char *cdir;
	template_t *fields;
} device_t;

template_t	tape[] = {
"tapeN","0+D*S",
"tapeNn","1+D*S",
"tapeNr","2+D*S",
"tapeNnr","3+D*S",
};

template_t	cdrom[] = {
"cdromN","0+D*S",
};

/*
 *  The first name in the disk array is significant.  This
 *  name is the name returned by bmkdev for each of the first
 *  two disk devices found.   bmkdev reports these names on stdout
 *  for the use of the installation scripts.  Change the first entry
 *  in this array at your own risk.
 */
template_t	disk[] = {
"cCbBtTdLs0","0+M",
"cCbBtTdLs1","1+M",
"cCbBtTdLs2","2+M",
"cCbBtTdLs3","3+M",
"cCbBtTdLs4","4+M",
"cCbBtTdLs5","5+M",
"cCbBtTdLs6","6+M",
"cCbBtTdLs7","7+M",
"cCbBtTdLs8","8+M",
"cCbBtTdLs9","9+M",
"cCbBtTdLsa","10+M",
"cCbBtTdLsb","11+M",
"cCbBtTdLsc","12+M",
"cCbBtTdLsd","13+M",
"cCbBtTdLse","14+M",
"cCbBtTdLsf","15+M",
"cCbBtTdLp0","184+M",
"cCbBtTdLp1","185+M",
"cCbBtTdLp2","186+M",
"cCbBtTdLp3","187+M",
"cCbBtTdLp4","188+M",
};

static device_t	Devices[] = {
ID_RANDOM, sizeof(disk)/sizeof(template_t), "/dev/dsk/", "/dev/rdsk/", disk,
ID_TAPE, sizeof(tape)/sizeof(template_t), "-", "/dev/rmt/", tape,
ID_ROM, sizeof(cdrom)/sizeof(template_t), "/dev/", "-", cdrom,
};

static int	DeviceSize = sizeof(Devices) / sizeof(device_t);

struct HBA HBA[MAX_EXHAS];

struct DRVRinfo {
	int	valid;
	char	name[NAME_LEN];	/* driver name			*/
	int	subdevs;	/* number of subdevices per LU	*/
	int	lu_occur;	/* occurrence of the current LU */
	int	instances; /* how many of each of these exist */
} DRVRinfo[MAXTCTYPE];

#undef makedev
#define makedev(major,minor)	(dev_t)(((major_t)(major) << 18 | (minor_t)(minor)))

device_t *
GetDevice(EDT *edtptr)
{
	int i;

	DTRACE;
	for (i = 0; i < DeviceSize; i++) {
		if ( Devices[i].pdtype == edtptr->xedt_pdtype )
			return(&Devices[i]);
	}
	return NULL;
}

#ifdef DEBUG
void
PrintDevice(device_t *device)
{
	int j;
	template_t current;

	DTRACE;
	fprintf(stderr,"%s,",device->bdir);
	fprintf(stderr,"%s,",device->cdir);
	for (j = 0; j < device->count; j++) {
		current = device->fields[j];
		fprintf(stderr,"%s,",current.dev);
		fprintf(stderr,"%s\n",current.map);
	}
}
#endif

#ifdef DPRINTF
void
PrintEDT(EDT *xedtptr, int edtcnt)
{
	int e;
	EDT *xedtptr2;
	fprintf(stderr,"driver\tha_slot\tSCSIbus\tTC\tnumlus\tmemaddr\tPDtype\tTCinq\n");
	for ( xedtptr2 = xedtptr, e = 0; e < edtcnt; e++, xedtptr2++ ) {
		fprintf(stderr,"%s\t%d\t%d\t%d\t%d\t0x%x\t%s\n",
			xedtptr2->xedt_drvname,
			xedtptr2->xedt_ctl,
			xedtptr2->xedt_bus,
			xedtptr2->xedt_target,
			xedtptr2->xedt_lun,
			xedtptr2->xedt_memaddr,
			xedtptr2->xedt_tcinquiry);
	}
}
#endif

void
error(message, data1, data2, data3, data4, data5)
char	*message;	/* Message to be reported */
long	data1;		/* Pointer to arg	 */
long	data2;		/* Pointer to arg	 */
long	data3;		/* Pointer to arg	 */
long	data4;		/* Pointer to arg	 */
long	data5;		/* Pointer to arg	 */
{
	DTRACE;
	(void) fprintf(stderr, "ERROR: ");
	(void) fprintf(stderr, message, data1, data2, data3, data4, data5);

	exit(1);
}

/*
 * Convert a numeric string arg to binary				
 * Arg:	string - pointer to command arg					
 *									
 * Always presume that operators and operands alternate.		
 * Valid forms:	123 | 123*123 | 123+123 | L*16+12			
 * Return:	converted number					
 */									
unsigned int
CalculateMinorNum(token, sa, drvr, edtptr)
register char *token;
struct scsi_addr *sa;
int drvr;
EDT *edtptr;
{
/*
 * The BIG parameter is machine dependent.  It should be a long integer
 * constant that can be used by the number parser to check the validity	
 * of numeric parameters.  On 16-bit machines, it should probably be	
 * the maximum unsigned integer, 0177777L.  On 32-bit machines where	
 * longs are the same size as ints, the maximum signed integer is more	
 * appropriate.  This value is 017777777777L.				
 */
	register char *cs;
	long n;
	long cut = BIG / 10;	/* limit to avoid overflow */

	DTRACE;
	cs = token;
	n = 0;
	/* check for operand */
	switch (*cs) {
	case 'C':
		n = sa->sa_c;
		cs++;
		break;
	case 'B':
		n = sa->sa_b;
		cs++;
		break;
	case 'T':
		n = sa->sa_t;
		cs++;
		break;
	case 'L':
		n = sa->sa_l;
		cs++;
		break;
	case 'D':
		n = DRVRinfo[drvr].lu_occur;
		cs++;
		break;
	case 'S':
		n = DRVRinfo[drvr].subdevs;
		cs++;
		break;
	case 'M':
		n = edtptr->xedt_first_minor;
		cs++;
		break;
	case 'P':
		n = SDI_MINOR(sa->sa_c, sa->sa_t, sa->sa_l, sa->sa_b);
		cs++;
		break;
	default:
		while ((*cs >= '0') && (*cs <= '9') && (n <= cut))
			n = n*10 + *cs++ - '0';
	}

	/* then check for the subsequent operator */
	switch (*cs++) {

	case '+':
		n += CalculateMinorNum(cs,sa,drvr,edtptr);
		break;
	case '*':
	case 'x':
		n *= CalculateMinorNum(cs,sa,drvr,edtptr);
		break;

	/* End of string, check for a valid number */
	case '\0':
		if ((n > BIG) || (n < 0)) {
			errno = 0;
			fprintf(stderr,"minor number out of range\n");
			exit(1);
		}
		return(n);
		/*NOTREACHED*/
		break;

	default:
		errno = 0;
		fprintf(stderr,"bad token in template file: \"%s\"\n",token);
		exit(1);
		break;
	}

	if ((n > BIG) || (n < 0)) {
		errno = 0;
		fprintf(stderr,"minor number out of range\n");
		exit(1);
	}

	return(n);
	/*NOTREACHED*/
}

/*
 * Using the directory and the token, substitute the ha, tc, and lu
 * numbers for the corresponding key letters in the token and concatenate
 * with the directory name to make the full path name of the device.
 * Return 1 if successful, return zero and a null devname if not.
 */
int
MakeNodeName(devname,dir,token,sa)
char *devname;
char *dir;
char *token;
struct scsi_addr *sa;
{
	int i;
	char c;

	DTRACE;
	devname[0] = '\0';
	if ((dir[0] == BLANK) || (token[0] == BLANK))
		return(0);

	i = 0;
	(void) strcat(devname,DevRoot);
	(void) strcat(devname,dir);
	while ( (c = token[i++]) != '\0') {

		switch (c) {
		case '\\':
			if ( token[i] != '\0' )
				(void) sprintf(devname,"%s%c",devname,token[i++]);
			break;
		case 'U':
			(void) sprintf(devname,"%s%d",devname,sa->sa_n-1);
			break;
		case 'C':
			(void) sprintf(devname,"%s%d",devname,sa->sa_c);
			break;
		case 'B':
			(void) sprintf(devname,"%s%d",devname,sa->sa_b);
			break;
		case 'T':
			(void) sprintf(devname,"%s%d",devname,sa->sa_t);
			break;
		case 'L':
			(void) sprintf(devname,"%s%d",devname,sa->sa_l);
			break;
		case 'N':
			(void) sprintf(devname,"%s%d",devname,sa->sa_n);
			break;
		default :
			(void) sprintf(devname,"%s%c",devname,c);
			break;
		}
	}
	return(1);
}

/* 
 * The CreateDirectory routine takes a directory path argument and creates
 * that directory if it does not yet exist. Error handling is not
 * performed since any errors in stat or mkdir that are ignored here
 * would be handled in the MakeDeviceNodes routine anyway.
 */
void
CreateDirectory(dir)
char *dir;
{
	struct stat	statbuf;
	char	newdir[MAXFIELD], tmpdir[MAXFIELD];
	char	*newdirp, *tok;
	DTRACE;

	/* check to see if directory field is blank */
	if (dir[0] == BLANK)
		return;

	/*
	 * Now start at beginning of path and create each
	 * directory in turn.
	 */
	strcpy(newdir,dir);
	newdirp=newdir;
	strcpy(tmpdir,DevRoot);
	while( (tok=strtok(newdirp,"/")) != NULL) {
		newdirp=NULL; 		/* set to null for next call to strtok */
		strcat(tmpdir,"/");
		strcat(tmpdir,tok);
		if (stat(tmpdir, &statbuf) < 0) {
			mkdir(tmpdir,DIRMODE);
		}
	}
}


/*
 * The MakeDeviceNodes routine is called for every logical unit. 
 * It checks the free inodes and then makes the device nodes.
 */
void
MakeDeviceNodes(sa,drvr,edtptr,nodename,save)
struct scsi_addr *sa;
int	drvr;	/* DRVRinfo index */
EDT *edtptr;
char **nodename;
int	save;
{
	int		i;
	minor_t		minornum;
	mode_t		modenum = DEV_MODE;
	char		block_name[MAXFIELD];
	char		char_name[MAXFIELD];
	device_t *device;
	template_t *template;
	
	DTRACE;
#ifdef DEBUG
	if (Debug) {
		fprintf(stderr,"making c%db%dt%dl%d\n",sa->sa_c,sa->sa_b,sa->sa_t,sa->sa_l);
	}
#endif

	if (!(device = GetDevice(edtptr))) {
		return;
	}

#ifdef DEBUG
	if (Debug)
		PrintDevice(device);
#endif

	/*
	 * Create directories if they don't already exist.
	 */
	CreateDirectory(device->bdir);
	CreateDirectory(device->cdir);

	/* read input lines for each subdevice, then make the device nodes */
	for(template = device->fields, i = 0; i < device->count; i++, template++) {

		minornum = CalculateMinorNum(template->map,sa,drvr,edtptr);

		if (MakeNodeName(block_name,device->bdir,template->dev,sa)) {
			(void)unlink(block_name);
			if (mknod(block_name,modenum | S_IFBLK,
					makedev(edtptr->xedt_bmaj,minornum)) < 0) {
				fprintf(stderr,"mknod failed for %s.\n", block_name);
				exit(1);
			} else {
				(void)chown(block_name,(uid_t)DEV_UID,(gid_t)DEV_GID);
			}
		}

		if (MakeNodeName(char_name,device->cdir,template->dev,sa)) {
			(void)unlink(char_name);
			if ( save && i == 0 )
				*nodename = strdup(char_name);
			if (mknod(char_name,modenum | S_IFCHR,
					makedev(edtptr->xedt_cmaj,minornum)) < 0) {
				fprintf(stderr,"mknod failed for %s.\n", char_name);
				exit(1);
			} else {
				(void)chown(char_name,(uid_t)DEV_UID,(gid_t)DEV_GID);
			}
		}
	}
}

void 
str_to_lower(s)
char *s;
{
	DTRACE;
	while(*s) {
		*s = (char)tolower(*s);
		s++;
	}
}

int
SpecialExists(int type, char *path, major_t dev_major, minor_t dev_minor)
{
	return TRUE;
}

/*
 *  Make the nodes /dev/[r]root and /dev/[r]swap
 */

void
make_bnodes(char *root, major_t char_maj, major_t blk_maj, minor_t minor, level_t level)
{
	int	offset, i;
	char dev_name[256], link_name[256];

	(void)mkdir(specials[0].dir,DIRMODE);

	for (i = 0; i < (sizeof(specials) / sizeof(io_template_t)); i++) {
		sprintf(dev_name, "%s/%s", root, specials[i].linkdir);

		CreateDirectory(specials[i].linkdir);

		offset = strlen(dev_name);

		sprintf(&dev_name[offset], "/%s", specials[i].linknode);
		(void)unlink(dev_name);
		sprintf(link_name, "%s/%s/%s", specials[i].dir, specials[i].device, specials[i].node);
		(void)symlink(link_name, dev_name);

		sprintf(&dev_name[offset], "/r%s", specials[i].linknode);
		(void)unlink(dev_name);
		sprintf(link_name, "%s/%s/r%s", specials[i].dir, specials[i].device, specials[i].node);
		(void)symlink(link_name, dev_name);
	}
}

int
main(argc,argv)
int	argc;
char	**argv;
{
					
	EDT 	*xedtptr  = NULL;	 /* Pointer to edt */
	EDT 	*xedtptr2 = NULL;	 /* Temp pointer   */
	int	c, t, lu, e;
	int		ntargets, scsicnt;
	struct scsi_addr	sa;
	int		edtcnt, i, arg;
	int		driver_index;
	int		save, target;
	ulong_t	tempd, disk1, disk2;
	char	*nodename, *disk1name, *disk2name, root_flag;
	major_t	cmajor, bmajor;
	minor_t	cbminor;
	level_t level;
	int	HBA_map[MAX_EXHAS];
	
	DTRACE;

#ifdef DEBUG
	Debug = FALSE;
#endif
	DevRoot = strdup("");
	root_flag = 0;
	while ((arg = getopt(argc,argv,"Sr:")) != EOF)

		switch (arg) {
		case 'r' :
			DevRoot = strdup(optarg);
			root_flag = 1;
			break;
		case 'S' : /* Turn on debug messages */
#ifdef DEBUG
			Debug = TRUE;
#endif
			break;
		case '?' : /* Incorrect argument found */
			fprintf(stderr,"usage: bmkdev [-S] [-r directory]\n");
			exit(1);
		}
	
	DTRACE;

	/* Ignore certain signals */
#ifdef DEBUG
	if (!Debug) {
#endif
		(void)signal(SIGHUP,SIG_IGN);
		(void)signal(SIGINT,SIG_IGN);
		(void)signal(SIGTERM,SIG_IGN);
#ifdef DEBUG
	}
#endif

	umask(0);

	/* Initialize driver info structure */
	for(i = 0; i < MAXTCTYPE; i++) {
		DRVRinfo[i].valid = FALSE;
	}

	if ((xedtptr = readxedt(&edtcnt)) == 0) {
		fprintf(stderr,"Unable to read equipped device table.\n");
		exit(1);
	}

#ifdef DEBUG
	if (Debug) {
		fprintf(stderr,"edtcnt %d\n", edtcnt);
	}
#endif
#ifdef DPRINTF
	PrintEDT(xedtptr, edtcnt);
#endif

	DTRACE;
	scsicnt = edt_sort(xedtptr, edtcnt, HBA, 0, TRUE);

#ifdef DPRINTF
	PrintEDT(xedtptr, edtcnt);
#endif

	disk1 = ORDINAL_A(MAX_EXHAS,MAX_BUS,MAX_EXTCS) + 1;
	disk2 = ORDINAL_A(MAX_EXHAS,MAX_BUS,MAX_EXTCS) + 1;
	DTRACE;
	for ( e = 0; e < scsicnt; e++ ) {
		xedtptr2 = HBA[HBA[e].order].edtptr;
		ntargets = HBA[HBA[e].order].ntargets;

		for (target = 0; target < ntargets; target++, xedtptr2++) {
			if (xedtptr2->xedt_pdtype == ID_RANDOM) {
				for(lu = 0; lu < MAX_EXLUS; lu++) {
			    	if(xedtptr2->xedt_lun == lu) {
						tempd = xedtptr2->xedt_ordinal + lu;
						if (tempd < disk1) {
							disk1 = tempd;
							cmajor = xedtptr2->xedt_cmaj;
							bmajor = xedtptr2->xedt_bmaj;
							cbminor = xedtptr2->xedt_first_minor;
						} else if (tempd < disk2)
							disk2 = tempd;
					}
				}
			}
		}
	}
	DTRACE;
	for ( xedtptr2 = xedtptr, e = 0; e < edtcnt; e++, xedtptr2++ ) {
		str_to_lower(xedtptr2->xedt_drvname);
		if (xedtptr2->xedt_pdtype == ID_PROCESOR)
			continue;
		for (driver_index = -1, i = 0; i < MAXTCTYPE; i++) {
			if (DRVRinfo[i].valid == TRUE) {
				if (!strcmp(DRVRinfo[i].name, xedtptr2->xedt_drvname)) {
				/* found it */
					driver_index = i;
					break;
				}
			} else {
				DRVRinfo[i].valid = TRUE;
				strcpy(DRVRinfo[i].name, xedtptr2->xedt_drvname);
				DRVRinfo[i].subdevs  = xedtptr2->xedt_minors_per;
				DRVRinfo[i].lu_occur = -1;
				DRVRinfo[i].instances = 1;
				driver_index = i;
				break;
			}
		}

		if (driver_index < 0) {
			errno = 0;
			fprintf(stderr,"Too many drivers. Only %d drivers supported.\n", MAXTCTYPE);
			exit(1);
		}
	}

	DTRACE;

#ifdef DEBUG
	if (Debug) {
		fprintf(stderr,"driver\tha_slot\tSCSIbus\tTC\tnumlus\tPDtype\tTCinq\n");
	}
#endif

	disk1name = disk2name = NULL;
	for ( c = 0; c < scsicnt; c++ ) {

		xedtptr2 = HBA[HBA[c].order].edtptr;
		ntargets = HBA[HBA[c].order].ntargets;

		sa.sa_c = xedtptr2->xedt_ctl;

		for (t = 0; t < ntargets; t++, xedtptr2++) {

			sa.sa_b = xedtptr2->xedt_bus;
			sa.sa_t = xedtptr2->xedt_target;

#ifdef DEBUG
			if (Debug) {
			fprintf(stderr,"%s\t%d\t%d\t%d\t%d\t%d\t%s\n",
				xedtptr2->xedt_drvname,
				xedtptr2->xedt_ctl,
				xedtptr2->xedt_bus,
				xedtptr2->xedt_target,
				xedtptr2->xedt_lun,
				xedtptr2->xedt_pdtype,
				xedtptr2->xedt_tcinquiry);
			}
#endif
			if (xedtptr2->xedt_drvname == NULL ||
			   (strcmp((char *) xedtptr2->xedt_drvname,"void") == 0)) {
				continue;
			}

			/* increment the occurrence field for the driver */
			for(driver_index = -1,i = 0; i < MAXTCTYPE; i++) {
				if(DRVRinfo[i].valid == TRUE) {
					if (!strcmp(DRVRinfo[i].name, xedtptr2->xedt_drvname)) {
						driver_index = i;
						break;
					}
				} else
					break;
			}
			if (driver_index == -1) {
				errno = 0;
				fprintf(stderr,"Too many drivers. Only %d drivers supported.\n", MAXTCTYPE);
				exit(1);
			}

			for(lu = 0; lu < MAX_EXLUS; lu++) {
			    if(xedtptr2->xedt_lun == lu) {
					save = FALSE;
					if ((xedtptr2->xedt_ordinal+lu) == disk1 ||
						(xedtptr2->xedt_ordinal+lu) == disk2) {
						save = TRUE;
					}

					sa.sa_l = lu;
					sa.sa_n = DRVRinfo[driver_index].instances++;

					DRVRinfo[driver_index].lu_occur++;

					if (xedtptr2->xedt_pdtype != ID_RANDOM ||
						((xedtptr2->xedt_ordinal+lu) == disk1 ||
						 (xedtptr2->xedt_ordinal+lu) == disk2))
						MakeDeviceNodes(&sa, driver_index, xedtptr2, &nodename,save);

					if (save) {
						if ((xedtptr2->xedt_ordinal+lu) == disk1)
							disk1name = nodename;
						else
							disk2name = nodename;
					}
				}
			} /* end LU loop */
		} /* end TC loop */
	} /* end HA loop using boot-chain based index into EDT */

	if (!root_flag) {
	/*
	 *  generate a map of the new controller numbers so we can
	 *  update the kernel's HBA_map.
	 *  The mini-kernel HBA_map is wrong until this is done.
	 */
		for ( c = 0; c < MAX_EXHAS; c++ )
			HBA_map[c] = SDI_UNUSED_MAP;

		for ( c = 0; c < scsicnt; c++ ) {
			xedtptr2 = HBA[c].edtptr;
			HBA_map[c] = xedtptr2->xedt_ctl;
		}

		writehbamap(HBA_map, MAX_EXHAS);
	}

	level = 2;
	make_bnodes(DevRoot, cmajor, bmajor, cbminor, level);

	printf("%s\n",disk1name);
	if ( disk2name )
		printf("%s\n",disk2name);

	exit(NORMEXIT);
}
