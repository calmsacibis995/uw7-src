#ident	"@(#)pdi.cmds:config.c	1.28.5.1"
#ident	"$Header$"

/*
 * sdiconfig [-aS] [-f driver] [-R root] [outputfile]
 *
 * /etc/scsi/sdiconfig is a utility that establishes the existence of
 * equipped SDI devices and outputs the data neccessary to run diskcfg.
 * It determines the device equippage by examining the edt.
 * Using its arguments, it can be forced to output information
 * that will cause the configuration of devices that do not exist.
 * This is useful for forcing the inclusion of drivers for hardware
 * that is about to be installed.
 *
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<libgen.h>
#include	<ctype.h>
#include	<string.h>
#include	<limits.h>
#include	<dirent.h>
#include	<nlist.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/errno.h>
#include	<sys/signal.h>
#include	<sys/vtoc.h>

#include	<sys/scsi.h>
#include	<sys/sdi_edt.h>
#include	<sys/sdi.h>

#include	"edt_sort.h"
#include	"scsicomm.h"
#include	"diskcfg.h"

#include	<sys/resmgr.h>
#include	<sys/confmgr.h>
#include	<sys/cm_i386at.h>
#include	<sys/p_sysi86.h>

#include	<locale.h>

extern int	errno,
			opterr,
			optind;
extern char	*optarg;
extern EDT *readxedt(int *);

#define CMDNAME "sdiconfig"
#define PDICMDNAME "pdiconfig"

#define MAX_FORCE 50
#define VB_SIZE	256

static char	*Cmdname,
			Sdiconfig = TRUE,
			*root,
			*force[MAX_FORCE],	/* pointers to short names of devices to force in */
			format[50], param_list[VB_SIZE], val_buf[VB_SIZE];

struct pdt_message {
	char *number;
	char *name;
};

struct pdt_message sdi_scsi_pdt[] = {	/* SCSI Peripherial Device types  */
	":474", "DISK ",		/* ID_RANDOM		*/
	":475", "TAPE ",		/* ID_TAPE		*/
	":476", "PRINTER",		/* ID_PRINTER		*/
	":477", "HBA  ",		/* ID_PROCESSOR		*/
	":478", "WORM ",		/* ID_WORM		*/
	":479", "CDROM",		/* ID_ROM		*/
	":480", "SCANNER",		/* ID_SCANNER		*/
	":481", "OPTICAL",		/* ID_OPTICAL		*/
	":482", "CHANGER",		/* ID_CHANGER		*/
	":483", "COMMUNICATION",	/* ID_COMMUNICATION	*/
};

int	Debug,
	err_flag,
	force_flag,
	scsicnt,
	all_flag,
	list_flag,
	adapter_count;
/*
 *	This array should be two arrays but, since it works like this,
 *	I am not going to change it just now.  There should be two arrays
 *	with the first containing index and edtptr and the second
 *	containing just index.  It would be less confusing to understand.
 */

struct HBA HBA[MAX_EXHAS];

static void
strip_quotes(input,output)
char *input, *output;
{
	register char *i, *j;

	for (i=input, j=output; i[0]; i++) {
		if ( i[0] != '"' ) {
			j[0] = i[0];
			j++;
		}
	}
	j[0] = '\0';
}

static int
field_number(name)
char *name;
{
	register int index;

	for (index = 0; index < NUM_FIELDS; index++)
		if (EQUAL(name,field_tbl[index].name))
			return((int)field_tbl[index].tag);

	return(-1);
}

static void
get_values(diskdesc, cfgfp)
struct diskdesc *diskdesc;
FILE *cfgfp;
{
	int		field_number();
	long 	dcd_ipl_flag,dcd_ivect_flag,dcd_ishare_flag;
	ushort	dcd_ipl,dcd_ishare,dcd_ivect;
	char	ibuffer[BUFSIZ],
			tname[MAX_FIELD_NAME_LEN+1],
			rbuffer[BUFSIZ],
			tbuffer[BUFSIZ];

	dcd_ipl_flag = FALSE;
	dcd_ivect_flag = FALSE;
	dcd_ishare_flag = FALSE;

	do
		if (fgets(ibuffer,BUFSIZ,cfgfp) != NULL) {
			if (sscanf(ibuffer, format, tname, rbuffer) == 2) {
				strip_quotes(rbuffer,tbuffer);
				switch (field_number(tname)) {
				case DEVTYPE:
					diskdesc->devtype = strdup(tbuffer);
					if (Debug)
						(void)fprintf(stderr,"DEVTYPE is %s for %s\n", diskdesc->devtype, diskdesc->name);
					break;
				case NAMEL:
					(void)strcpy(diskdesc->fullname,tbuffer);
					if (Debug)
						(void)fprintf(stderr,"NAMEL is %s for %s\n", diskdesc->fullname, diskdesc->name);
					break;
				case DEVICE:
					(void)strcpy(diskdesc->type,tbuffer);
					if (Debug)
						(void)fprintf(stderr,"DEVICE is %s for %s\n", diskdesc->type, diskdesc->name);
					break;
				case DMA2:
					diskdesc->dma2 = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"DMA2 is %hu for %s\n", diskdesc->dma2, diskdesc->name);
					break;
				case IPL:
					if ( diskdesc->ipl == 0 )
						diskdesc->ipl = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"IPL is %hu for %s\n", diskdesc->ipl, diskdesc->name);
					break;
				case IVEC:
					if ( diskdesc->ivect == 0 )
						diskdesc->ivect = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"IVEC is %hu for %s\n", diskdesc->ivect, diskdesc->name);
					break;
				case SHAR:
					if ( diskdesc->ishare == 0 )
						diskdesc->ishare = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"SHAR is %hu for %s\n", diskdesc->ishare, diskdesc->name);
					break;
				case DCD_IPL:
					dcd_ipl_flag = TRUE;
					dcd_ipl = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"IPL is %hu for %s\n", diskdesc->ipl, diskdesc->name);
					break;
				case DCD_IVEC:
					dcd_ivect_flag = TRUE;
					dcd_ivect = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"IVEC is %hu for %s\n", diskdesc->ivect, diskdesc->name);
					break;
				case DCD_SHAR:
					dcd_ishare_flag = TRUE;
					dcd_ishare = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
					if (Debug)
						(void)fprintf(stderr,"SHAR is %hu for %s\n", diskdesc->ishare, diskdesc->name);
					break;
				}
			}
		}
	while (!feof(cfgfp) && !ferror(cfgfp));

	if (ferror(cfgfp))
		error(":442:Error occurred while reading %s file for module %s.\n", CFGNAME, diskdesc->name);

/*
 *	These assignments are done here to ensure that we have read in
 *	the device type before testing it.
 */
	if ( DEVICE_IS_DCD(diskdesc) ) {
		if ( ! dcd_ipl_flag )
			error(":459:A value for \"%s\" is required in the %s file for module %s.\n", "DCD_IPL", CFGNAME, diskdesc->name);
		else if ( ! dcd_ivect_flag )
			error(":459:A value for \"%s\" is required in the %s file for module %s.\n", "DCD_IVEC", CFGNAME, diskdesc->name);
		else if ( ! dcd_ishare_flag )
			error(":459:A value for \"%s\" is required in the %s file for module %s.\n", "DCD_SHAR", CFGNAME, diskdesc->name);

		diskdesc->ipl = dcd_ipl;
		diskdesc->ivect = dcd_ivect;
		diskdesc->ishare = dcd_ishare;
		if ( DEVICE_IS_DISK(diskdesc) ) {
			diskdesc->equip = EQUIP_DISK;
		} else if ( DEVICE_IS_TAPE(diskdesc) ) {
			diskdesc->equip = EQUIP_TAPE;
		}
	} else {
		diskdesc->equip = 0;
	}
}

static int
parse_System_line(diskdesc,ibuffer,direntp)
struct diskdesc *diskdesc;
char *ibuffer;
struct	dirent	*direntp;
{
	int found, retval;
	short dma;
	char	token[BUFSIZ],value[BUFSIZ];

	found = FALSE;

	switch (ibuffer[0]) {

	case '#':
		break;

	case '$':
		if (sscanf(ibuffer,"$%s %s",token,value) == 2) {
			if (EQUAL(token,"version")) {
				diskdesc->version = strtol(ibuffer, (char**)NULL, 0);
			} else if (EQUAL(token,"loadable")) {
				diskdesc->loadable = TRUE;
			}
		}
		break;

	default:
		retval = sscanf(ibuffer,"%8s %1s %d %hu %hu %hu %lx %lx %lx %lx %hd %d",
			diskdesc->name, diskdesc->configure, &diskdesc->unit,
		 	&diskdesc->ipl, &diskdesc->ishare, &diskdesc->ivect,
		 	&diskdesc->sioaddr, &diskdesc->eioaddr,
		 	&diskdesc->smemaddr, &diskdesc->ememaddr,
			&dma, &diskdesc->bind_cpu);
		if (retval == 11 || retval == 12) {
			if (Debug)
				(void)fprintf(stderr,"%s",ibuffer);
			if (EQUAL(direntp->d_name, diskdesc->name)) {
				found = TRUE;
				diskdesc->dma1 = ((dma < 0)?0:dma);
				if (retval == 11)
					diskdesc->bind_cpu = -1;
				diskdesc->configure[0] = 'N';
				(void)strcpy(diskdesc->name,direntp->d_name);
				if(Debug)
					(void)fprintf(stderr,"Found adapter %s, unit %hd.\n",diskdesc->name, diskdesc->unit);
			}
		}
		break;
	}
	return(found);
}

static struct diskdesc *
read_config(char init_flag)
{
	DIR		*parent;
	struct	dirent	*direntp;
	struct diskdesc	*adapters,		/* Chain of adapter descriptions */
					*diskdesc,		/* Current description */
					**nextlink;		/* Next link in adapter chain */
	FILE	*pipefp, *cfgfp;		/* pipe to idinstall */
	unsigned short	found;		/* Found previous sdevice match? */
	char	command[PATH_MAX+PATH_MAX+10],
			cmd_line[PATH_MAX+PATH_MAX+31],
			ibuffer[BUFSIZ],	/* a temp input buffer */
			basename[PATH_MAX+sizeof(PACKD)+1],
			cfgname[PATH_MAX+sizeof(PACKD)+MAXNAMLEN+sizeof(CFGNAME)+3];
	int temp_int;

	if ( access(IDINSTALL, X_OK) ) {
		temp_int = errno;
		if ( Debug )
			fprintf(stderr,"The error number is %d.\n",temp_int);
		errno = 0;
		switch(temp_int) {
			case EPERM:
			case EACCES:
				error(":460:You do not have sufficient privilege to use this command.\n");
			default:
				error(":461:The program %s is not accessible and must be.\n", IDINSTALL);
		}
	}

	if (root == NULL) {
		(void)sprintf(command,"%s -G -s",IDINSTALL);
		(void)sprintf(basename, "%s", PACKD);
	} else {
		(void)sprintf(command,"%s -G -s -R %s/etc/conf",IDINSTALL,root);
		(void)sprintf(basename, "%s%s", root, PACKD);
	}
	(void)sprintf(format, "%%%d[^=]=%%%d[^\n]", MAX_FIELD_NAME_LEN, BUFSIZ-1);

	adapters = NULL;
	nextlink = &adapters;
	adapter_count = 0;

	if ((diskdesc = (struct diskdesc *) malloc(sizeof(struct diskdesc))) == NULL)
		error(":430:Insufficient memory\n");
	diskdesc->next = NULL;
	diskdesc->loadable = FALSE;
	diskdesc->version = 0;

	if ((parent = opendir(basename)) == NULL)
			error(":462:Could not process directory %s\n", basename);

/*
 *	This while loop can be read as "for each entry in the pack.d directory"
 */
	while ((direntp = readdir( parent )) != NULL) {

		(void)sprintf(cfgname, "%s/%s/%s", basename, direntp->d_name, CFGNAME);

		if (access(cfgname, F_OK))		/* if there is no disk.cfg file here */
			continue;

		if (!init_flag) {
			(void)strcpy(diskdesc->name,direntp->d_name);
			
			*nextlink = diskdesc;			/* Add to end of chain */
			nextlink = &diskdesc->next;		/* Point to next link */

			if ((diskdesc = (struct diskdesc *) malloc(sizeof(struct diskdesc))) == NULL)
				error(":430:Insufficient memory\n");
			diskdesc->next = NULL;
			continue;
		}

		if ( Debug )
			sprintf(cmd_line,"%s %s",command,direntp->d_name);
		else
			sprintf(cmd_line,"%s %s 2>/dev/null",command,direntp->d_name);

		if ((pipefp = popen(cmd_line, "r")) == NULL) 
			error(":463:Cannot read System entry for module %s.\n", direntp->d_name);

		if ((cfgfp = fopen(cfgname, "r")) == NULL) 
			error(":440:Cannot open file %s for reading.\n", cfgname);

		found = FALSE;

		do {
			if (fgets(ibuffer,BUFSIZ,pipefp) != NULL) {
				if ( parse_System_line(diskdesc, ibuffer, direntp) ) {
					found++;
					adapter_count++;

					get_values(diskdesc, cfgfp);
					rewind(cfgfp);

					*nextlink = diskdesc;			/* Add to end of chain */
					nextlink = &diskdesc->next;		/* Point to next link */

					if ((diskdesc = (struct diskdesc *) malloc(sizeof(struct diskdesc))) == NULL)
						error(":430:Insufficient memory\n");
					diskdesc->next = NULL;
					diskdesc->loadable = FALSE;
					diskdesc->version = 0;
				}
			}
		} while (!feof(pipefp) && !ferror(pipefp));

		if (!found)
			error(":464:No System entry for module %s.\n", direntp->d_name);

		if (pclose(pipefp) == -1)
			error(":439:Problem closing pipe to %s\n", cmd_line);

		if (fclose(cfgfp) == -1)
			error(":441:Problem closing file %s\n", cfgname);

	}

	(void)closedir( parent );

	return (adapters);
}

static char *
get_name(inquiry_string, unit)
char	*inquiry_string;
int	*unit;
{
	register char *paren;

	if (inquiry_string[0] != '(') {
		*unit = -1;
		return NULL;
	}

	if ((paren = strchr(inquiry_string, ')')) == NULL)
		error(":465:No device name found in %s.\n", inquiry_string);

	paren[0] = '\0';

	if ((paren = strchr(inquiry_string, ',')) == NULL)
		*unit = 1;
	else {
		*unit = (int)strtol(&paren[1], (char**)NULL, 0);
		paren[0] = '\0';
	}

	if ((paren = strchr(inquiry_string, '(')) == NULL)
		error(":465:No device name found in %s.\n", inquiry_string);

	return(++paren);
}

static void
force_configure(adapters)
struct diskdesc *adapters;
{
	struct diskdesc *device;

	if (Debug)
		(void)fprintf(stderr,"Marking all adapters as configure = Y.\n");

	for (device = adapters; device != NULL; device = device->next) {
		if (Debug)
			(void)fprintf(stderr,"\tMarking %s as configured in.\n",device->name);
		device->configure[0] = 'Y';
		device->unit = -1;
	}
}

static void
force_individual(adapters)
struct diskdesc *adapters;
{
	struct diskdesc *device;
	int index;

	for (index=0; index < force_flag; index++) {
		for (device = adapters; device != NULL; device = device->next) {
			if (EQUAL(device->name,force[index])) {
				if (Debug)
					(void)fprintf(stderr,"Forcing %s to be configured in.\n",device->name);
				device->configure[0] = 'Y';
				device->unit = -1;
				break;
			}
		}
		if (device == NULL) {
			errno = 0;
			warning(":471:A device driver for %s was not found on this system.\n", force[index]);
		}
	}
}

static void
mark_configure(adapters, tc_name, tc_unit, equipment, controller)
struct diskdesc *adapters;
char *tc_name;
int tc_unit, controller;
unsigned long equipment;
{
	struct diskdesc *device;
	int	unit;

	if (Debug)
		(void)fprintf(stderr,"Name found is %s, unit %hd\n", (tc_name?tc_name:"NULL"), tc_unit);

	if ( tc_name == NULL ) {
		return;
	}

	unit = tc_unit;

	for (device = adapters; device != NULL; device = device->next) {
		if (EQUAL(device->name,tc_name)) {
			if ( --unit == 0 ) {
				if (Debug)
					(void)fprintf(stderr,"Marking %s number %d as configured in.\n",tc_name,tc_unit);
				device->configure[0] = 'Y';
				device->unit = controller;
				device->equip = equipment;
				device->active++;
				return;
			}
		}
	}
	error(":466:No entry for %s number %d found in current configuration.\n", tc_name, tc_unit);
}

static void
write_output(adapters)
struct diskdesc *adapters;
{
	struct diskdesc *diskdesc;
	if (Debug) {
		(void)fprintf(stderr,"name\t\"fullname\"\t\ttype\tconf\tunit\tequip\tdma1\tdma2\tipl\tivect\tishare\tsio\teio\tsmem\temem\n");
	}

	for (diskdesc = adapters; diskdesc != NULL; diskdesc = diskdesc->next) {
		if (Debug) {
			(void)fprintf(stderr,"%s\t\"%s\"\t%s\t%s\t%d\t0x%lx\t%hu\t%hu\t%hu\t%hu\t%hu\t0x%lx\t0x%lx\t0x%lx\t0x%lx\t%d\n",
			       diskdesc->name, diskdesc->fullname,
			       diskdesc->type, diskdesc->configure,
			       diskdesc->unit, diskdesc->equip,
			       diskdesc->dma1, diskdesc->dma2,
			       diskdesc->ipl, diskdesc->ivect,
			       diskdesc->ishare,
			       diskdesc->sioaddr, diskdesc->eioaddr,
			       diskdesc->smemaddr, diskdesc->ememaddr, diskdesc->bind_cpu);
		}
		(void)printf("%s\t\"%s\"\t%s\t%s\t%d\t0x%lx\t%hu\t%hu\t%hu\t%hu\t%hu\t0x%lx\t0x%lx\t0x%lx\t0x%lx\t%d\n",
			       diskdesc->name, diskdesc->fullname,
			       diskdesc->type, diskdesc->configure,
			       diskdesc->unit, diskdesc->equip,
			       diskdesc->dma1, diskdesc->dma2,
			       diskdesc->ipl, diskdesc->ivect,
			       diskdesc->ishare,
			       diskdesc->sioaddr, diskdesc->eioaddr,
			       diskdesc->smemaddr, diskdesc->ememaddr, diskdesc->bind_cpu);
	}
}

static int
alpha_compare(device1,device2)
struct diskdesc *device1, *device2;
{
/*
 * SCSI devices sort higher than DCD devices
 */
	if (DEVICE_IS_DCD(device1) && DEVICE_IS_DCD(device2))
		return(FALSE);

	if (DEVICE_IS_SCSI(device1) && DEVICE_IS_DCD(device2))
		return(FALSE);

	if (DEVICE_IS_DCD(device1) && DEVICE_IS_SCSI(device2))
		return(TRUE);
/*
 * now we know that they are both SCSI devices,
 * lets compare their names
 */
	return((strcmp(device1->name,device2->name)<=0)?FALSE:TRUE);
}

static struct diskdesc *
adapter_sort(adapters,compare)
struct diskdesc *adapters;
int 			(*compare)();
{
	struct diskdesc	*adapter, *top, *prev, *temp;
	int		sorted;

	if ( adapter_count == 1 )	/* if there is only one, no need to sort */
		return(adapters);

/*
 * here we malloc another diskdesc structure so we can have
 * a dummy one to use to point to the beginning of the adapters.
 * This simplifies the sort algorithm immensely.
 */
	if ((top = (struct diskdesc *) malloc(sizeof(struct diskdesc))) == NULL)
		error(":430:Insufficient memory\n");
	top->next = adapters;
/*
 * This is simply a dumb bubble-sort.  Since the adapters structure
 * is relatively small, this will suffice.  At the present time,
 * small means 8 or 9 devices.
 */
	sorted = FALSE;

	while ( !sorted ) {
		sorted = TRUE;
		adapter = top->next;
		prev = top;
		while ( adapter->next != NULL ) {
			if ( (*compare)(adapter,adapter->next) ) {  /* do we need to bubble */
				sorted = FALSE;
				temp = adapter->next->next;
				adapter->next->next = prev->next;
				prev->next = adapter->next;
				adapter->next = temp;
				prev = prev->next;
			} else {			/* go to the next one */
				prev = adapter;
				adapter = adapter->next;
			}
		}
	}

	return(top->next);
}

#ifdef PDI_DEBUG
HBA_print(struct HBA *HBA, int size)
{
	int i;

	fprintf(stderr,"index\torder\tntargets\tedtptr\tcntl\n");
	for (i=0; i < size; i++)
		fprintf(stderr,"%d\t%d\t%d\t0x%x\t0x%x\n", HBA[i].index, HBA[i].order, HBA[i].ntargets, HBA[i].edtptr, HBA[i].cntl);
}
#endif

edt_print(EDT *xedtptr, int edtcnt)
{
	EDT *edt2,*edt3;
	int	index, nctls, last_ctl, unsorted, target;

	unsorted = edt_fix(xedtptr, edtcnt);

	nctls = edt_sort(xedtptr,edtcnt,HBA,unsorted,FALSE);

	for (last_ctl = -1, index = 0; index < nctls; index++) {
		edt2 = HBA[HBA[index].order].cntl;
		if ( edt2->xedt_ctl != last_ctl ) {
			last_ctl = edt2->xedt_ctl;
			printf(gettxt(":472","%d:%d,%d,%d: %-8s: %s\n"), edt2->xedt_ctl,
		         	edt2->xedt_bus, edt2->xedt_target, edt2->xedt_lun,
			     	gettxt(sdi_scsi_pdt[edt2->xedt_pdtype].number,
		               	   sdi_scsi_pdt[edt2->xedt_pdtype].name),
		         	edt2->xedt_tcinquiry);
		} else {
			printf(gettxt(":473","  %d,%d,%d: %-8s: %s\n"),
			         edt2->xedt_bus, edt2->xedt_target, edt2->xedt_lun,
			         gettxt(sdi_scsi_pdt[edt2->xedt_pdtype].number,
			                sdi_scsi_pdt[edt2->xedt_pdtype].name),
			         edt2->xedt_tcinquiry);
		}
		edt3 = HBA[HBA[index].order].edtptr;
		for (target = 0; target < HBA[HBA[index].order].ntargets ; target++, edt3++) {
			if ( edt3 == edt2 )
				continue;
			printf(gettxt(":473","  %d,%d,%d: %-8s: %s\n"),
			         edt3->xedt_bus, edt3->xedt_target, edt3->xedt_lun,
			         gettxt(sdi_scsi_pdt[edt3->xedt_pdtype].number,
			                sdi_scsi_pdt[edt3->xedt_pdtype].name),
			         edt3->xedt_tcinquiry);
		}
	}
}

char
sdi_driver(struct diskdesc *adapters, char *driver)
{
	struct diskdesc	*adapter;
	for (adapter = adapters; adapter != NULL; adapter = adapter->next) {
		if (!strcmp(driver,adapter->name))
			return TRUE;
	}
	return FALSE;
}

static void
sdiunits(int argc,char **argv)
{
	char update, force_to_run, autoconfig, oldstyle, *tc_name;
	char *name, *aunit;
	int arg, edtcnt, unsorted, scsicnt, rv, largest;
	int cntl, ntargets, target, tc_unit, iunit;
	struct diskdesc	*adapters, *adapter;
	EDT 	*xedtptr,*xedtptr2;
	rm_key_t	rmkey;

	force_to_run = FALSE;
	autoconfig = FALSE;
	oldstyle = FALSE;
	update = FALSE;

	while ((arg = getopt(argc,argv,"auofS")) != EOF) {
		switch (arg) {
		case 'a' : /* process autoconfig drivers */
			autoconfig = TRUE;
			break;
		case 'o' : /* process non-autoconfig drivers */
			oldstyle = TRUE;
			break;
		case 'u' : /* only update datebase */
			update = TRUE;
			break;
		case 'f' : /* force to run regardless of run-level */
			force_to_run = TRUE;
			break;
		case 'S' : /* Turn on debug messages */
			Debug = TRUE;
			break;
		case '?' : /* Incorrect argument found */
			return;
			/*NOTREACHED*/
			break;
		}
	}

	if (!autoconfig && !oldstyle)
		return;

	if (autoconfig && oldstyle)
		return;

	/* if the parent is init then run this command */
	if (getppid() == 1)
		force_to_run = TRUE;

	if (!force_to_run)
		return;

	if (!Debug) {
		(void) signal(SIGHUP,SIG_IGN);
		(void) signal(SIGINT,SIG_IGN);
		(void) signal(SIGTERM,SIG_IGN);
	}

	if ((rv = RMopen(O_RDWR)) != 0)
		error("%s() failed, errno=%s\n", "RMopen", rv);


	if (autoconfig) {

		(void)sprintf(param_list, "%s %s", CM_MODNAME, CM_UNIT);
		if ((adapters = read_config(FALSE)) == NULL)
			error(":469:No PDI devices found on system.\n");

		rmkey = RM_NULL_KEY;
		largest = -1;
		while (!nextkey(&rmkey)) {
			(void)RMbegin_trans(rmkey, RM_READ);
			if ((rv = RMgetvals(rmkey, param_list, 0, val_buf, VB_SIZE)) != 0)
					error(":489:%s() failed, errno=%s\n", "RMgetvals", rv);
			name = strtok(val_buf," ");
			if (sdi_driver(adapters, name)) {
				aunit = strtok(NULL," ");
				iunit = strcmp(aunit,"-") ? atoi(aunit) : -1;
				if (iunit > largest)
					largest = iunit;
			}
			RMend_trans(rmkey);
		}
		rmkey = RM_NULL_KEY;
		while (!nextkey(&rmkey)) {
			(void)RMbegin_trans(rmkey, RM_READ);
			if ((rv = RMgetvals(rmkey, param_list, 0, val_buf, VB_SIZE)) != 0)
					error(":489:%s() failed, errno=%s\n", "RMgetvals", rv);
			name = strtok(val_buf," ");
			RMend_trans(rmkey);
			if (sdi_driver(adapters, name)) {
				aunit = strtok(NULL," ");
				iunit = strcmp(aunit,"-") ? atoi(aunit) : -1;
				if (iunit < 0) {
					(void)RMbegin_trans(rmkey, RM_RDWR);
					(void)RMdelvals(rmkey, CM_UNIT);
					sprintf(val_buf, "%d", ++largest);
					if (RMputvals(rmkey, CM_UNIT, val_buf))
						error(":458:RMputvals() %s failed\n", "CM_UNIT");
					RMend_trans(rmkey);
				}
			}
		}
	} else {
		if ((xedtptr = readxedt(&edtcnt)) == NULL)
			error(":470:Unable to read system Equipped Device Table.\n");

		unsorted = edt_fix(xedtptr, edtcnt);

		scsicnt = edt_sort(xedtptr,edtcnt,HBA,unsorted,FALSE);

		if ( scsicnt != unsorted ) {
		  (void)sprintf(param_list, "%s %s", CM_MODNAME, CM_UNIT);

          for (cntl = 0; cntl < scsicnt; cntl++) {

            xedtptr2 = HBA[HBA[cntl].order].edtptr;
            ntargets = HBA[HBA[cntl].order].ntargets;

            for (target = 0; target < ntargets; target++, xedtptr2++) {
               tc_name = get_name((char *) xedtptr2->xedt_tcinquiry, &tc_unit);
               if (!tc_name)
                  continue;

               if ( xedtptr2->xedt_rmkey == RM_NULL_KEY ) {
                  rmkey = RM_NULL_KEY;
                  largest = -1;
                  while (!nextkey(&rmkey)) {
					 (void)RMbegin_trans(rmkey, RM_READ);
                     if ((rv = RMgetvals(rmkey,param_list,0,val_buf,VB_SIZE)))
                           error(":489:%s() failed, errno=%s\n","RMgetvals",rv);
					 (void)RMend_trans(rmkey);
                     name = strtok(val_buf," ");
                     if (!strcmp(tc_name, name) && !(--tc_unit)) {
                        aunit = strtok(NULL," ");
                        iunit = atoi(aunit);
                        if ( iunit != xedtptr2->xedt_ctl ) {
						   (void)RMbegin_trans(rmkey, RM_RDWR);
                           (void)RMdelvals(rmkey, CM_UNIT);
                           sprintf(val_buf, "%d", xedtptr2->xedt_ctl);
                           if (RMputvals(rmkey, CM_UNIT, val_buf))
                              error(":458:RMputvals() %s failed\n", "CM_UNIT");
						   RMend_trans(rmkey);
                        }
                        break;
                     }
                  }
               } else {
                  rmkey = xedtptr2->xedt_rmkey;
				  (void)RMbegin_trans(rmkey, RM_READ);
                  if ((rv = RMgetvals(rmkey, param_list, 0, val_buf, VB_SIZE)))
                        error(":489:%s() failed, errno=%s\n", "RMgetvals", rv);
				  (void)RMend_trans(rmkey);
                  name = strtok(val_buf," ");
                  aunit = strtok(NULL," ");
                  iunit = atoi(aunit);
                  if ( iunit != xedtptr2->xedt_ctl ) {
					 (void)RMbegin_trans(rmkey, RM_RDWR);
                     (void)RMdelvals(rmkey, CM_UNIT);
                     sprintf(val_buf, "%d", xedtptr2->xedt_ctl);
                     if (RMputvals(rmkey, CM_UNIT, val_buf))
                        error(":458:RMputvals() %s failed\n", "CM_UNIT");
					 (void)RMend_trans(rmkey);
                  }
               }
            }
          }
          if (!update)
              system("/etc/conf/bin/idbuild > /dev/null 2>&1");
		}
	}
	RMclose();
}

main(argc,argv)
int	argc;
char	**argv;
{
	register EDT 	*xedtptr  = NULL;	 /* Pointer to edt */
	register EDT 	*xedtptr2 = NULL;	 /* Temp pointer   */
	register int	c, t;
	char	*tc_name, install_time;
	int	tc_unit;
	unsigned long	equipment;
	int	arg;
	struct diskdesc	*adapters;	/* Chain of adapter descriptions */
	char	*label;
	int	unsorted, edtcnt, ntargets;
#ifdef PDI_DEBUG
	FILE *temp_edt_fp;
#endif
	
	Cmdname = strdup(basename(argv[0]));

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxdisksetup");
	label = (char *) malloc(strlen(Cmdname)+1+3);
	sprintf(label, "UX:%s", Cmdname);
	(void) setlabel(label);

	Sdiconfig = (!strcmp(Cmdname, CMDNAME) || 
                     !strcmp(Cmdname, PDICMDNAME)) ? TRUE : FALSE;

	Debug = FALSE;
	opterr = 0;		/* turn off the getopt error messages */

	if (!Sdiconfig) {
		sdiunits(argc,argv);
		exit(NORMEXIT);
	}
	root = NULL;
	err_flag = FALSE;
	force_flag = FALSE;
	all_flag = FALSE;
	list_flag = FALSE;
	install_time = FALSE;

	while ((arg = getopt(argc,argv,"IalSR:f:")) != EOF) {

		switch (arg) {
		case 'I' : /* running during installation */
			install_time = TRUE;
			break;
		case 'a' : /* configure in all sdi devices */
			all_flag = TRUE;
			break;
		case 'f' : /* configure in the specified sdi device */
			if (force_flag >= MAX_FORCE)
				error(":467:Too many -f options specified, try again with no more than %d.\n", MAX_FORCE);
			force[force_flag++] = strdup(optarg);
			break;
		case 'l' : /* list all sdi devices */
			list_flag = TRUE;
			break;
		case 'S' : /* Turn on debug messages */
			Debug = TRUE;
			break;
		case 'R' : /* set the root variable */
			root = strdup(optarg);
			break;
		case '?' : /* Incorrect argument found */
			errno=0;
			error(":468:Usage: %s [-alS] [-f device] [-R root] [outputfile]\n",Cmdname);
			break;
		}
	}

/*
 *	If there is an argument left, use it to name the output file
 *	instead of using stdout.
 */
	if (optind < argc)
		(void)freopen(argv[optind], "w", stdout);

	/* Ignore certain signals */
	if (!Debug) {
		(void) signal(SIGHUP,SIG_IGN);
		(void) signal(SIGINT,SIG_IGN);
		(void) signal(SIGTERM,SIG_IGN);
	}
	umask(0); /* use template file permission (mode) */

	if ( list_flag ) {
		if ((xedtptr = readxedt(&edtcnt)) == 0) {
			error(":470:Unable to read system Equipped Device Table.\n");
		}
#ifdef PDI_DEBUG
		if (Debug) {
		if ((temp_edt_fp = fopen("/tmp/tmp_edt","w+")) != NULL) {
			fwrite (xedtptr, sizeof(struct scsi_xedt), edtcnt, temp_edt_fp);
			(void)fclose(temp_edt_fp);
		}
		}
#endif
		edt_print(xedtptr, edtcnt);
		(void)fclose(stdout);
		exit(NORMEXIT);
	}
/*
 *	Read in all of the configuration info on all SDI devices in this system.
 *  This consists of all devices that have a disk.cfg file in their pack.d
 *
 *	Besides reading in the disk.cfg file, read in the sdevice.d entry as well.
 *
 *	If there is more than one entry in the sdevice.d file for any
 *	device, there is more than one entry in adapters as well.
 */
	system("/etc/conf/bin/idconfupdate");
	adapters = read_config(TRUE);
	if (adapters == NULL)
		error(":469:No SDI devices found on system.\n");

	if ( all_flag ) {
		force_configure(adapters);

		/*
		 * Now we sort the adapters into alphabetic order, making
		 * SCSI devices first.
		 */
		adapters = adapter_sort(adapters,alpha_compare);

	} else {
		tload(root);
		if ((xedtptr = readxedt(&edtcnt)) == 0) {
			tuload();
			error(":470:Unable to read system Equipped Device Table.\n");
		}
		tuload();

		unsorted = edt_fix(xedtptr, edtcnt);

		scsicnt = edt_sort(xedtptr,edtcnt,HBA,unsorted,install_time);  /* set-up the HBA array */
#ifdef PDI_DEBUG
		HBA_print(HBA, scsicnt);
#endif

		if (Debug) {
			(void)fprintf(stderr,"\nscsicnt %d\n", scsicnt);
			(void)fprintf(stderr,"\ndriver\tHA\tbus\tTC\tnumlus\tPDtype\tha_id\tTCinq\n");
		}
	
		for (c = 0; c < scsicnt; c++) {

			xedtptr2 = HBA[HBA[c].order].edtptr;
			ntargets = HBA[HBA[c].order].ntargets;

			for (equipment = 0, t = 0; t < ntargets; t++, xedtptr2++) {

				if (Debug) {

					(void)fprintf(stderr,"%s\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n",
						xedtptr2->xedt_drvname, c,
						xedtptr2->xedt_bus,
						xedtptr2->xedt_target,
						xedtptr2->xedt_lun,
						xedtptr2->xedt_pdtype,
						xedtptr2->xedt_ha_id,
						xedtptr2->xedt_tcinquiry);
				}

				/*
				 * record the device types attached to this HBA
				 * we use this info below to sort the entries
				 */
				equipment |= (1 << (xedtptr2->xedt_pdtype));
				if (xedtptr2->xedt_pdtype != ID_PROCESOR ||
					(xedtptr2->xedt_pdtype == ID_PROCESOR &&
					xedtptr2->xedt_target != xedtptr2->xedt_ha_id))
					continue;
				tc_name = get_name((char *) xedtptr2->xedt_tcinquiry, &tc_unit);
				mark_configure(adapters, tc_name, tc_unit, equipment, xedtptr2->xedt_ctl);
			}
		}

		/*
		 * Now turn on any adapters specified on the command
		 * line with the -f option.
		 */
		if ( force_flag ) {
			force_individual(adapters);
		}
	}

	write_output(adapters);

	(void)fclose(stdout);

	exit(NORMEXIT);
}
int
nextkey(int *key){

	int ret;

	RMbegin_trans(*key, RM_READ);
	ret = RMnextkey(key);
	RMend_trans(*key);
	return ret;
}
