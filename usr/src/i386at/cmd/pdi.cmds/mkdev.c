#ident	"@(#)pdi.cmds:mkdev.c	1.6.15.1"
#ident	"$Header$"

/*
 * sdimkdev [-d tcindexfile] [-s] [-f] [-i] [-S]
 *
 * /etc/scsi/sdimkdev is a utility that creates device nodes for
 * equipped SDI devices. It determines the device equippage
 * by examining the edt. Since the device 
 * nodes that are created for each device are unique to that 
 * device type, template files are used to specify the device 
 * naming conventions. The location of the template files is
 * specified in a target controller index file which may be 
 * supplied as a command line argument. The display of a new
 * device can also be controlled by an argument.
 *
 * sdimkdev has been modified to set the level on all nodes
 * to SYS_PRIVATE.  This change should be OK on a non-sfs filesystem
 * since the attempt to set the level will be ignored.
 *
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/statfs.h>
#include	<ctype.h>
#include	<sys/errno.h>
#include	<sys/stat.h>
#include	<sys/signal.h>
#include	<sys/mkdev.h>
#include	<sys/fcntl.h>
#include	<sys/buf.h>
#include	<sys/vtoc.h>
#include	<search.h>
#include	<string.h>
#include 	<ftw.h>

#include	<devmgmt.h>
#include	<unistd.h>
#include	<libgen.h>
#include	<mac.h>
#ifdef OUT		/* Take everything inside #ifdef OUT out */
#include	<sys/vfstab.h>
#endif /* OUT */
#include	<sys/fdisk.h>
#include	<sys/sd01_ioctl.h>

#include	<sys/sysi86.h>
#include	<sys/scsi.h>
#include	<sys/sdi_edt.h>
#include	<sys/sdi.h>
#include	"edt_sort.h"
#include	"scsicomm.h"
#include	"fixroot.h"

#include	<locale.h>
#include	<pfmt.h>
#include	<limits.h>
#include	<langinfo.h>
#include	<regex.h>

#include	<mac.h>

#undef makedev
#define makedev(major,minor)	(dev_t)(((major_t)(major) << 18 | (minor_t)(minor)))

#define NO_CHANGES_EXIT 1
#define PDIMKDEV	"pdimkdev"
#define SDIMKDEV	"sdimkdev"
#define PDIMKDTAB	"pdimkdtab"
#define SDIMKDTAB	"sdimkdtab"
#define SDIGHOST	"sdighost"
#define	TCINDEX		"/etc/scsi/tc.index"
#define LAST_BOOT_EDT	"/etc/scsi/sdi_edt"
#define TEMP_EDT	"/etc/scsi/tmp.edt"
#define INSTALLF_INPUT	"/etc/scsi/installf.input"
#define MASK		00644
#define BLANK		'-'
#define YES		1
#define NO		0
#define DEV_MODE	0600
#define DEV_UID		0	/* owner root */
#define DEV_GID		3	/* group sys */
#define DIRMODE		0775
#define	MAXLINE		256
#define MAXMAJOR	255
#define	MAXDRVR	32
#define INODECRITICAL	50
#define INODELOW	200
#define	BIG		017777777777L
#define	BIG_I		2000

/* definitions of template file fields */
#define KEY		0
#define MINOR		1
#define MODE		2
#define	BLOCK		3
#define CHAR		4
#define SA		5
#define RSA		6
#define NUMFIELDS	7 /* the total number of fields in the template file */

/* Type Definitions */
typedef	struct	stat		STAT;

typedef struct  tokens {
	char string[32];
	int  token;
} TOKENS_T;

typedef struct  devices {
	char string[32];
} DEVICE_T;

struct scsi_addr{
	unsigned int	sa_c;		/* controller occurrence        */
	unsigned int	sa_t;		/* target controller number     */
	unsigned int	sa_l;		/* logical unit number          */
	unsigned int	sa_b;		/* bus number                   */
	unsigned int	sa_n;		/* admin name unit number       */
};

typedef struct disk_info {
	int	disk_valid;
	major_t	disk_bmajor;
	major_t	disk_cmajor;
	int	disk_count;
	minor_t	disk_minor;
} disk_info_t;

extern int	errno;
extern void	error(), warning();
extern EDT *readxedt(int *);

extern void   free();

/* Static Variables */
static char	Sdimkdev = FALSE;
static char	Sdimkdtab = FALSE;
static char	Silent = FALSE;
static char	UpdateMode = FALSE;
static char	Cmdname[64];
static char	TCindex[128];
static FILE	*contents_fp = NULL;
static FILE	*TCindexfp;
static FILE	*last_edt_fp;
static FILE	*temp_edt_fp;
static char	SCSI_name[49];
static level_t level;
static char	*ES_attr = "";
static disk_info_t	disks;
static char	nodeTemplate[28];
static	char	hexstr[] = "0123456789abcdef";
int	Debug;

#define GADDR_LEN	16	/* cCbBtTdL */
#define ALIAS_LEN	12	/* disk1, disk2 ... */

static char	*Cmdopt = "-fu"; /* sdimkdev -fu is invoked by sdighost */

static char 	**ghost_names = NULL;
static int	ghost_cnt = 0;

struct ghost {
	char	device[ALIAS_LEN];	/* device alias */
	char	stamp[sizeof(struct pd_stamp) + 1];
	char	real_addr[GADDR_LEN + 1];
	char	ghost_addr[GADDR_LEN + 1];
};

static struct ghost *ghost_p = NULL;

static struct pd_stamp default_stamp = { 
	PD_STAMP_DFLT, PD_STAMP_DFLT, PD_STAMP_DFLT 
};

/* Token Definitions:
 *	To add a token, add a define below and update NUMTOKENS. Then
 *	add the string and token to the Tokens array that follows.
 */
#define MKDEV		0
#define TCTYPE		1
#define QUERY		2
#define POSTMSG		3
#define COMMENT		4
#define DATA		5
#define TCINQ		6
#define ALIAS		7
#define DGRP 		8
#define ATTR 		9
#define FSATTR 		10
#define DPATTR 		11
#define GENERIC		12
#define UNKNOWN		13
#define TCLEN		14
#define NODEFMT		15
#define NUMTOKENS	16 /* Should be one beyond maximum number of tokens */
#define UNTOKEN		-2

TOKENS_T	Tokens[] = {
	"MKDEV",	MKDEV,
	"TCTYPE",	TCTYPE,
	"QUERY",	QUERY,
	"POSTMSG",	POSTMSG,
	"#",		COMMENT,
	"DATA",		DATA,
	"TCINQ",	TCINQ,
	"ALIAS",	ALIAS,
	"DGRP",		DGRP,
	"ATTR",		ATTR,
	"FSATTR",	FSATTR,
	"DPATTR",	DPATTR,
	"GENERIC",	GENERIC,	
	"UNKNOWN",	UNKNOWN,	
	"TCLEN",	TCLEN,	
	"NODEFMT",	NODEFMT,	
};

DEVICE_T	Device_Types[] = {
	"Disk",			/*	ID_RANDOM	*/
	"Tape",			/*	ID_TAPE		*/
	"Printer",		/*	ID_PRINTER	*/
	"Processor",		/*	ID_PROCESSOR	*/
	"WORM",			/*	ID_WORM		*/
	"CD-ROM",			/*	ID_ROM		*/
	"Scanner",		/*	ID_SCANNER	*/
	"Optical",		/*	ID_OPTICAL	*/
	"Changer",		/*	ID_CHANGER	*/
	"Communication",	/*	ID_COMMUNICATION*/
};

struct HBA HBA[MAX_EXHAS];

struct DRVRinfo {
	int	valid;
	char	name[NAME_LEN];	/* driver name			*/
	struct drv_majors majors;	/* array of all major sets	*/
	int	subdevs;	/* number of subdevices per LU	*/
	int	lu_occur;	/* occurrence of the current LU */
	int	instances; /* how many of each of these exist */
} DRVRinfo[MAXDRVR];

void E333A_nodes(void);
int MakeDeviceTable(char *, char *, uchar_t *, uchar_t, char *, struct ghost *);

#ifdef DPRINTF
void
PrintEDT(EDT *xedtptr, int edtcnt)
{
	int e;
	EDT *xedtptr2;
	fprintf(stderr,"driver\tHA\tBUS\tTC\tLUN\tmemaddr\tStamp\tPDtype\tTCinq\n");
	for ( xedtptr2 = xedtptr, e = 0; e < edtcnt; e++, xedtptr2++ ) {
		fprintf(stderr,"%s\t%d\t%d\t%d\t%d\t0x%x\t\"%12s\"\t%d\t%s\n",
			xedtptr2->xedt_drvname,
			xedtptr2->xedt_ctl,
			xedtptr2->xedt_bus,
			xedtptr2->xedt_target,
			xedtptr2->xedt_lun,
			xedtptr2->xedt_memaddr,
			(char *) &xedtptr2->xedt_stamp,
			xedtptr2->xedt_pdtype,	
			xedtptr2->xedt_tcinquiry);
	}
}
#endif

struct nodes {
	dev_t	device;
	char	type;
};
typedef struct nodes nodes_t;
static int *block_majors, *char_majors, block_count, char_count;

/*
 * the RemoveMatchingNode routine is called from the search_and_remove routine.
 * It is used in a call to nftw() which will recursively descend a directory and
 * execute RemoveMatchingNode for each file in that directory tree.
 * It compares the major number of each file to a list of PDI major numbers.
 * Files matching those major numbers are removed that are not in the hash
 * table are removed.
 */
int 
RemoveMatchingNode(const char *name, const STAT *statbuf, int code, struct FTW *ftwbuf)
{
	char	found;
	major_t	maj;
	int	search_index;
	ENTRY	new_item;

	found = FALSE;
	switch(code) {
	case FTW_F:
		maj = major(statbuf->st_rdev);
		switch(statbuf->st_mode & S_IFMT) {
		case S_IFCHR:
#ifdef DEBUG
			fprintf(stderr,"char special %s found\n",name);
#endif
			for(search_index = 0; char_majors[search_index] >= 0; search_index++){
				if ( char_majors[search_index] == maj )
					found = TRUE;
			}
			break;
		case S_IFBLK:
#ifdef DEBUG
			fprintf(stderr,"block special %s found\n",name);
#endif
			for(search_index = 0; block_majors[search_index] >= 0; search_index++){
				if ( block_majors[search_index] == maj )
					found = TRUE;
			}
			break;
		default:
			return(0);
		}

		if (found) {
			new_item.key = (void *)name;
			if (hsearch(new_item,FIND) == NULL) {
#ifdef DEBUG
				fprintf(stderr, "unlinking %s\n", name);
#endif
				unlink(name);
			}
#ifdef DEBUG
			else
				fprintf(stderr, "leaving alone %s\n", name);
#endif
		}
	}
	return(0);
}

int
search_and_remove(char *path)
{
	return(nftw(path, RemoveMatchingNode, 4, FTW_PHYS));
}

void
remove_nodes(FILE *fp)
{
	char device[MAXFIELD], type, *result, found;
	int search_index, new_major, new_minor, count, index;
	dev_t	new_device;
	nodes_t *nodes, *node;
	ENTRY	new_item, *found_item;
	char *link_device;
	
	rewind(fp);

	count = 0;
	while (fscanf(fp,"%*[^\n]\n") != EOF) {
		count++;
	}

	nodes = (nodes_t *)calloc(count, sizeof(nodes_t));
	block_majors = (int *)calloc(count, sizeof(int));
	char_majors = (int *)calloc(count, sizeof(int));
	(void)hcreate(count);

	rewind(fp);
	for(search_index = 0; search_index < count; search_index++) {
		block_majors[search_index] = -1;
		char_majors[search_index] = -1;
	}

	block_count = char_count = index = 0;
	while (fscanf(fp,"%s %c",device, &type) != EOF) {
		if ( type == 'c' ) {
			(void)fscanf(fp,"%d %d %*[^\n]\n",&new_major, &new_minor);
			found = FALSE;
			for(search_index = 0; char_majors[search_index] >= 0; search_index++){
				if ( char_majors[search_index] == new_major )
					found = TRUE;
			}
			if ( !found ) {
				char_majors[search_index] = new_major;
				char_count++;
			}
		} else if ( type == 'b' ) {
			(void)fscanf(fp,"%d %d %*[^\n]\n",&new_major, &new_minor);
			found = FALSE;
			for(search_index = 0; block_majors[search_index] >= 0; search_index++){
				if ( block_majors[search_index] == new_major )
					found = TRUE;
			}
			if ( !found ) {
				block_majors[search_index] = new_major;
				block_count++;
			}
		}
		else {
			/* skip the rest of the line */
			(void)fscanf(fp,"%*[^\n]\n");
			link_device = strchr(device, '=');
			*link_device++ = '\0';
			new_item.key = link_device;
			if ((found_item = hsearch(new_item,FIND)) != NULL) {
				type = ((nodes_t *)found_item->data)->type;
				new_major = major(((nodes_t *)found_item->data)->device);
				new_minor = minor(((nodes_t *)found_item->data)->device);
			} else {
				warning(":500:%s: not found\n", link_device);
				continue;
			}
		}
		
		new_item.key = strdup(device);
		node = &nodes[index++];
		node->device = makedev(new_major, new_minor);
		node->type = type;
		new_item.data = (void *)node;
		(void)hsearch(new_item, ENTER);
	}

#ifdef SEARCH_DEBUG
	for(search_index = 0; block_majors[search_index] >= 0; search_index++){
		printf("block_major=%d\n",block_majors[search_index]);
	}
	for(search_index = 0; char_majors[search_index] >= 0; search_index++){
		printf("char_major=%d\n",char_majors[search_index]);
	}
#endif
#ifdef SEARCH_DEBUG_PROMPT

	new_item.key = device;
	printf("enter a device name:");
	while (scanf("%s", device) != EOF) {
		if ((found_item = hsearch(new_item,FIND)) != NULL) {
			printf("found %s, type=%c, major=%d, minor=%d\n",
				found_item->key, ((nodes_t *)found_item->data)->type,
				major(((nodes_t *)found_item->data)->device),
				minor(((nodes_t *)found_item->data)->device));
		} else {
			printf("not found\n");
		}
		printf("enter a device name:");
	}
	printf("\n");
#endif
	search_and_remove("/dev");
}

void
before_exit(int remove_nodes_flag)
{
	char system_line[MAXLINE];
	long	length;
	struct stat stat_buf;

	if ( Sdimkdev ) {

		E333A_nodes(); /* last ditch attempt to make basic nodes */

		if (contents_fp) {
			length = ftell(contents_fp);
			if ( length && remove_nodes_flag ) {
				remove_nodes(contents_fp);
			}

			fclose(contents_fp);
		}
	} 
#ifndef DEBUG
	if ( !Sdimkdev || UpdateMode )
		(void)unlink(INSTALLF_INPUT);
#endif
}


/*
 * This routine writes a scsi address string to the location provided.
 */
void
scsi_name(struct scsi_addr *sa, char *message)
{
	if (sa == NULL) {
		sprintf(message, "");
	} else {
		/* occurrence based addressing */
		if (sa->sa_b)
			sprintf(message, "HA %u BUS %u TC %u LU %u",
				sa->sa_c, sa->sa_b, sa->sa_t, sa->sa_l);
		else
			sprintf(message, "HA %u TC %u LU %u",
				sa->sa_c, sa->sa_t, sa->sa_l);
	}
}

/*
 * Check for the existence and correctness of a special device. Returns TRUE if
 *      the file exists and has the correct major and minor numbers
 *      otherwise removes the file and returns FALSE, warns for error.
 */
int
SpecialExists(char *path, int type, dev_t dev_num)
{
	STAT	statbuf;

	if (lstat(path,&statbuf) < 0) {
		/* 
		 * errno == ENOENT if file doesn't exist.
		 * Otherwise exit because something else is wrong.
		 */
		if (errno != ENOENT) {
			warning(":396:stat failed for %s\n", path);
			return TRUE;
		} else {/* file does not exist */
			return FALSE;
                }
	}

	/* file exists */

	statbuf.st_mode &= S_IFMT;

	/* do it's major and minor numbers match the one we need */

	if (type == S_IFCHR && ((statbuf.st_mode & S_IFCHR) == S_IFCHR)) {
		if (statbuf.st_rdev == dev_num) {
			return TRUE;
		}
	} 
	else if (type == S_IFBLK && ((statbuf.st_mode & S_IFBLK) == S_IFBLK)) {
		if (statbuf.st_rdev == dev_num) {
			return TRUE;
		}
	}

	(void) unlink(path);

	return FALSE;
}

/*
 * Make the device node. First, check for the existence and correctness of 
 * a special device. 
 * If the file doesn't exist, make the node. 
 * Returns TRUE only if the node is actually created.
 */
int
MakeSpecial(char *path, mode_t modenum, int type, dev_t dev_num)
{
	if (SpecialExists(path, type, dev_num))
		return FALSE;

	if (mknod(path, modenum | type, dev_num) < 0) {
		warning(":403:mknod failed for %s.\n", path);
		return FALSE;
	} else {
		(void)chown(path, (uid_t)DEV_UID,(gid_t)DEV_GID);
		(void)lvlfile(path, MAC_SET, &level);
	}

	return TRUE;
}

/*
 * Link the device nodes. First, check for the existence and correctness of 
 * a special device. 
 * Returns TRUE only if the node is actually linked.
 */
int
LinkSpecial(char *topath, char *frompath, int type, dev_t dev_num)
{
	if (SpecialExists(topath, type, dev_num))
		return FALSE;

	if (link(frompath, topath) < 0) {
		warning(":404:%s\n", topath);
		return FALSE;
	} else {
		(void)chown(topath, (uid_t)DEV_UID,(gid_t)DEV_GID);
		(void)lvlfile(topath, MAC_SET, &level);
	}

	return TRUE;
}

/*
 * Check for the existence of a file. Returns TRUE if the file exists,
 * returns FALSE if the file doesn't exist, warns for error.
 */
int
FileExists(char *path)
{
	STAT	statbuf;

	if (stat(path,&statbuf) < 0) {
		/* 
		 * errno == ENOENT if file doesn't exist.
		 * Otherwise exit because something else is wrong.
		 */
		if (errno != ENOENT) {
			warning(":396:stat failed for %s\n", path);
			return(TRUE);
		}
		else /* file does not exist */
			return(FALSE);
	}

	/* file exists */
	return(TRUE);
}


/* GetToken() - reads the SCSI template file and returns the token found */
int
GetToken(FILE *templatefp)
{
	char	token[MAXLINE];
	int	curtoken;

	/* Read the next token from the template file */
	switch (fscanf(templatefp,"%s",token)) {
	case EOF :
		return(EOF);
	case 1	: /* normal return */
		break;
	default :
		return(UNKNOWN);
		/*NOTREACHED*/
		break;
	}

	/* Determine which token */
	for (curtoken = 0; curtoken < NUMTOKENS; curtoken++) {
		if (strcmp(Tokens[curtoken].string,token) == 0) {
			return(Tokens[curtoken].token);
		}
	}

	/* allow comment tokens to not be separated by white space from text */
	if (strncmp(Tokens[COMMENT].string,token,strlen(Tokens[COMMENT].string)) == 0)
		return(Tokens[COMMENT].token);

	/* Token not found */
	return(UNTOKEN);
}	/* GetToken() */


FILE *
OpenTemplate(uchar_t tc_pdtype, uchar_t *tcinq)
{
	int	tctype_match, template_found, tokenindex, i;
	FILE	*templatefp;
	int	end;
	char	templatefile[MAXFIELD], tctypestring[MAXFIELD];
	char	tcinqstring[INQ_EXLEN], tctypetoken[MAXFIELD];
	int	num_pdtype;
	int	generic_match, index_tctype, index_inqlen;
	int	generic_tctype;
	TOKENS_T	Generics[] = {
		"NO_DEV",	ID_NODEV,
		"RANDOM",	ID_RANDOM,
		"TAPE",		ID_TAPE,
		"PRINTER",	ID_PRINTER,
		"PROCESSOR",	ID_PROCESOR,
		"WORM",		ID_WORM,
		"ROM",		ID_ROM,
		"SCANNER",	ID_SCANNER,
		"OPTICAL",	ID_OPTICAL,
		"CHANGER",	ID_CHANGER,
		"COMMUNICATION",	ID_COMMUNICATION,
	};
	num_pdtype = sizeof(Generics) / sizeof(TOKENS_T);

	templatefile[0] = '\0';
	tctypestring[0] = '\0';
	tcinqstring[0]  = '\0';
	tctypetoken[0]  = '\0';
	generic_match = FALSE;

	strcpy(tcinqstring, (char *)tcinq);
	/* pad inquiry string with blanks if necessary */
	if (strlen(tcinqstring) != (VID_LEN + PID_LEN + REV_LEN)) {
		end = strlen(tcinqstring);
		for(i = end; i < (VID_LEN + PID_LEN + REV_LEN); i++)
			tcinqstring[i] = ' ';
		tcinqstring[VID_LEN + PID_LEN + REV_LEN] = '\0';
	}
	tctype_match = FALSE;
	template_found = FALSE;
	generic_tctype = UNKNOWN;
	index_inqlen = PID_LEN;
	index_tctype = UNKNOWN;

	/* start at the beginning of the tcindex file */
	rewind(TCindexfp);
	while (!template_found) {
		tokenindex = GetToken(TCindexfp);
		switch (tokenindex) {

		case TCTYPE:
			if(fscanf(TCindexfp," %[^\n]\n",tctypestring) == EOF) {
				errno = 0;
				warning(":397:Format of the target controller index file: %s\n", TCindex);
				return(NULL);
			}
			for (i=0; i <= num_pdtype; i++) {
				if (strcmp(Generics[i].string,tctypestring) == 0) {
					index_tctype = Generics[i].token;
					break;
				}
			}
			break;

		case TCLEN:
			if (fscanf(TCindexfp," %d\n",&index_inqlen) == EOF) {
				errno = 0;
				warning(":397:Format of the target controller index file: %s\n", TCindex);
				return(NULL);
			}
			index_inqlen -= VID_LEN;
			if ( index_inqlen < 1 )
				index_inqlen = PID_LEN;
			break;

		/* A new TC INQuiry token was added to allow the TC's inquiry 
		 * string to specify the devices template file.
		 */
		case TCINQ:
			if (fscanf(TCindexfp," %[^\n]\n",tctypetoken) == EOF) {
				errno = 0;
				warning(":397:Format of the target controller index file: %s\n", TCindex);
				return(NULL);
			}

			/* pad inquiry string with blanks if necessary */

			if ((end = strlen(tctypetoken)) != (VID_LEN + PID_LEN)) {
				for(i = end; i < (VID_LEN + PID_LEN); i++)
					tctypetoken[i] = ' ';
				tctypetoken[VID_LEN + PID_LEN] = '\0';
			}

			/*
			 * Compares given length of vendor and product names.
			 * If the tctype is specified in the index file, it must
			 * match as well.
			 */

			if ((index_tctype == UNKNOWN || tc_pdtype == index_tctype) &&
			    (strncmp(&tctypetoken[0 + VID_LEN],&tcinqstring[0 + VID_LEN],
			    index_inqlen) == 0)) {
				tctype_match = TRUE;
			}

			index_inqlen = PID_LEN;
			index_tctype = UNKNOWN;
			break;

		case GENERIC:
			if(fscanf(TCindexfp," %[^\n]\n",tctypestring) == EOF) {
				errno = 0;
				warning(":397:Format of the target controller index file: %s\n", TCindex);
				return(NULL);
			}
			for (i=0; i <= num_pdtype; i++) 
				if (strcmp(Generics[i].string,tctypestring) == 0) {
					generic_tctype = Generics[i].token;
					break;
				}
			if (tc_pdtype == generic_tctype)
				generic_match = TRUE;
			break;

		case MKDEV:
			if (tctype_match || generic_match) {
				if(fscanf(TCindexfp," %[^\n]\n",templatefile) == EOF) {
					errno = 0;
					warning(":397:Format of the target controller index file: %s\n", TCindex);
					return(NULL);
				}
				template_found = TRUE;
			} else {
				/* read the remainder of the input line */
				fscanf(TCindexfp,"%*[^\n]%*[\n]");
			}
			break;

		case EOF:
			errno = 0;
			warning(":398:TC entry not found in %s for \"%s\".\n",TCindex,tcinq);
			return(NULL);
		case COMMENT:
		case UNTOKEN:
		case UNKNOWN:
		default:       /* read the remainder of the input line */
			fscanf(TCindexfp,"%*[^\n]%*[\n]");
			break;
		}
	}
	/* Check to see that the template file exists. */
	if (!FileExists(templatefile)) {
		warning(":399:template file %s does not exist.\n", templatefile);
		return(NULL);
	}
	/* Open the target controller template file. */
	if ((templatefp = fopen(templatefile,"r")) == NULL) 
		warning(":400:Could not open %s.\n", templatefile);
	
	return(templatefp);
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
CalculateMinorNum(char *token, struct scsi_addr *sa, int drvr, EDT *edtptr)
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
			before_exit(FALSE);
			errno = 0;
			scsi_name(sa, SCSI_name);
			error(":401:minor number out of range for %s\n", SCSI_name);
		}
		return(n);
		/*NOTREACHED*/
		break;

	default:
		before_exit(FALSE);
		errno = 0;
		error(":402:bad token in template file: \"%s\"\n", token);
		break;
	}

	if ((n > BIG) || (n < 0)) {
		before_exit(FALSE);
		errno = 0;
		scsi_name(sa, SCSI_name);
		error(":401:minor number out of range for %s\n", SCSI_name);
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
MakeNodeName(char *devname, char *dir, char *token,struct scsi_addr *sa)
{
	int i;
	char c;
#ifdef GDEBUG
	printf("MakeNodeName: ");
	printf("dir = %s; ", dir);
	printf("token = %s; ", token);
	printf("sa = c%db%dt%dl%d n%d\n", sa->sa_c,sa->sa_b,sa->sa_t,sa->sa_l,sa->sa_n);
#endif
	devname[0] = '\0';
	if ((dir != NULL && dir[0] == BLANK) || (token[0] == BLANK))
		return(0);

	i = 0;
	if (dir != NULL)
		(void) strcat(devname,dir);

	while ( (c = token[i++]) != '\0') {

		switch (c) {

		case '\\':
			if ( token[i] != '\0' ) {
				c = token[i++];
				(void) sprintf(devname,"%s%c",devname,c);
			}
			break;
		case 'U':
			(void) sprintf(devname,"%s%d",devname,sa->sa_n-1);
			break;
		case 'N':
			(void) sprintf(devname,"%s%d",devname,sa->sa_n);
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
CreateDirectory(char *dir)
{
	STAT	statbuf;

	/* check to see if directory field is blank */
	if (dir[0] == BLANK)
		return;

	/* check to see if the directory already exists */
	if (stat(dir,&statbuf) == 0)
		return;
	
	/*
         * Now start at beginning of path and create each
	 * directory in turn.
	 */
	(void) mkdirp(dir,DIRMODE);
}

/*
 * hexToString
 *	Make the string representation of an hex number.
 *	Notice: This routine is limited to 2 digits!
 */
void
hexToString(char *field, int index)
{
	if ((index >> 4) & 0x0F) {
		field[0] = hexstr[(index >> 4) & 0x0F];
		field[1] = hexstr[index & 0x0F];
		field[2] = NULL;
	} else {
		field[0] = hexstr[index & 0x0F];
		field[1] = NULL;
	}
}

/*
 * makeExtNodes
 *	Read the VTOC and attempt to create all the nodes for the extended
 *	slices. All necessary information besides the VTOC is received from
 *	the calling routine.
 */
int
makeExtNodes(char (*dir)[MAXFIELD], struct scsi_addr *sa, int drvr, EDT *edtptr)
{
	char devname[NUMFIELDS][MAXFIELD];
	int good_name[NUMFIELDS];
	int	fd;
	struct vtoc_ioctl vtoc;
	major_t tc_bmajor, tc_cmajor;
	minor_t	minornum;
	dev_t cdev_num, bdev_num;
	int i;
	int NodesCreated = FALSE;

	/* Make name for slice 0 */
	nodeTemplate[9] = '0';
	nodeTemplate[10] = NULL;
	if (!MakeNodeName(devname[CHAR], dir[CHAR], nodeTemplate, sa)) {
#ifdef DEBUG
		fprintf(stderr, "Could not make node\n");
#endif
		return NodesCreated;
	}

	if (!rd_vtoc(devname[CHAR],&vtoc)) {
#ifdef DEBUG
		fprintf(stderr, "Could not get VTOC for %s\n", devname[CHAR]);
#endif
		return NodesCreated;
	}

	tc_cmajor = DRVRinfo[drvr].majors.c_maj;
	tc_bmajor = DRVRinfo[drvr].majors.b_maj;

	/* Create the extended nodes */
#ifdef DEBUG
	fprintf(stderr, "slices = %d\n", vtoc.v_nslices);
#endif
	for (i = V_NUMPAR; i < vtoc.v_nslices; i ++) {

		/* Do nothing if the slice has size 0. */
		if (vtoc.v_slices[i].p_size == 0 || ((vtoc.v_slices[i].p_flag & V_VALID) != V_VALID))
			continue;

		minornum = edtptr->xedt_first_minor + i;
		cdev_num = makedev(tc_cmajor, minornum);
		bdev_num = makedev(tc_bmajor, minornum);

		hexToString(&nodeTemplate[9], i);

		good_name[BLOCK] = MakeNodeName(devname[BLOCK], dir[BLOCK],
						nodeTemplate, sa);
		good_name[CHAR] =MakeNodeName(devname[CHAR], dir[CHAR],
					      nodeTemplate,sa);
		good_name[SA] = MakeNodeName(devname[SA], dir[SA],
					     nodeTemplate, sa);
		good_name[RSA] = MakeNodeName(devname[RSA], dir[RSA],
					     nodeTemplate,sa);
#ifdef DEBUG
		fprintf(stderr, "block = %s char = %s sa = %s rsa = %s\n",devname[BLOCK], devname[CHAR], devname[SA], devname[RSA]);
#endif


		if ( good_name[BLOCK] ) {
			NodesCreated = MakeSpecial(devname[BLOCK], DEV_MODE, S_IFBLK, bdev_num);
			fprintf(contents_fp,"%s b %d %d ? ? ? %d NULL NULL\n",devname[BLOCK],major(bdev_num),minor(bdev_num),level);
		}

		if ( good_name[CHAR] ) {
			NodesCreated = MakeSpecial(devname[CHAR], DEV_MODE, S_IFCHR, cdev_num);
			fprintf(contents_fp,"%s c %d %d ? ? ? %d NULL NULL\n",devname[CHAR],major(cdev_num),minor(cdev_num),level);
		}

		/* link the system admin names if necessary */
		if ( good_name[SA] ) {
			NodesCreated = LinkSpecial(devname[SA], devname[BLOCK], S_IFBLK, bdev_num);
			fprintf(contents_fp,"%s=%s l\n",devname[SA],devname[BLOCK]);
		}

		if ( good_name[RSA] ) {
			NodesCreated = LinkSpecial(devname[RSA], devname[CHAR], S_IFCHR, cdev_num);
			fprintf(contents_fp,"%s=%s l\n",devname[RSA],devname[CHAR]);
		}
	}
	return NodesCreated;
}

/*
 * The MakeDeviceNodes routine is called for every logical unit. 
 * It checks the free inodes and then makes the device nodes.
 */
void
MakeDeviceNodes(struct scsi_addr *sa, int drvr, EDT *edtptr)
{
	int	p, tokenindex;
	int		data_begin;
	minor_t		minornum;
	mode_t		modenum;
	dev_t	cdev_num, bdev_num;
	char		field[NUMFIELDS][MAXFIELD], dir[NUMFIELDS][MAXFIELD];
			/*
			 * the devname array below is used to hold the block,
			 * character, SA, and rSA filenames for the device.
			 * The values in these fields correspond to the current
			 * line being examined in the device template file
			 */
	char		devname[NUMFIELDS][MAXFIELD];
	char		query[MAXLINE];
	char		tempmsg[MAXLINE], postmsg[MAXLINE], alias[MAXLINE];
	char		bdevlist[MAXLINE*2], cdevlist[MAXLINE*2], dtab_alias[MAXLINE];
	FILE		*templatefp;
	int		NodesCreated;
	int		good_name[NUMFIELDS];
	major_t tc_bmajor, tc_cmajor;
	char *strp, *endp;
	struct scsi_addr ga, real_sa;
	int		ghost_status;
	struct ghost *gsp;
	
	if (Debug) {
		scsi_name(sa, SCSI_name);
		printf("MakeDeviceNodes:%s occ=%d\n", SCSI_name, DRVRinfo[drvr].lu_occur);
	}

	postmsg[0]='\0';


	/* if this is an unknown tctype then simply return */
	if ((templatefp = OpenTemplate(edtptr->xedt_pdtype, edtptr->xedt_tcinquiry)) == NULL)
		return;

	data_begin=FALSE;
	while (!data_begin) {
		tokenindex = GetToken(templatefp);
		switch (tokenindex) {
		case NODEFMT:
			fscanf(templatefp, " %[^\n]\n", nodeTemplate);
			break;
		case QUERY:
			fscanf(templatefp, " %[^\n]\n", query);
			break;
		case POSTMSG:
			/*
			 * Copy token value to postmsg with "\n" character
			 * pairs converted to new line characters
			 */
			fscanf(templatefp, " %[^\n]\n", tempmsg);
			strp = tempmsg;
			while ((endp = strstr(strp, "\\n")) != NULL) {
				strncat(postmsg, strp, endp - strp);
				strcat(postmsg, "\n");
				strp = endp + 2;
			}
			strcat(postmsg, strp);
			break;
		case ALIAS:
			fscanf(templatefp, " %[^\n]\n", alias);
			break;
		case DATA:
			data_begin=TRUE;
			break;
		case EOF:
			(void) fclose(templatefp);
			return;
			/* NOTREACHED */
			break;
		default:
		case COMMENT:
			fscanf(templatefp, "%*[^\n]%*[\n]");
			break;
		}
	}

	/*
	 * check if this device address conflicts with a ghost address
	 * or needs a ghost address 
	 */

	ghost_status = GhostAddress(sa, drvr, edtptr, &ga, &gsp);
	if (ghost_status < 0) {
		/*
		 * This device address conflicts with a ghost address
		 * which is in use currently
		 * fail the request to create any node for this device
		 * until the ghost name is resolved
		 */
		(void) fclose(templatefp);
		return;
	}
	if (ghost_status) {
		real_sa = *sa;
		ga.sa_n = sa->sa_n;
		*sa = ga;
	}

	/* 
	 * The next line contains the device directories in cols 4 to 7
	 */
	(void)ParseLine(dir,templatefp,NUMFIELDS);

	/*
	 * Create directories if they don't already exist.
	 */
	CreateDirectory(dir[BLOCK]);
	CreateDirectory(dir[CHAR]);
	CreateDirectory(dir[SA]);
	CreateDirectory(dir[RSA]);

	NodesCreated = FALSE;

	bdevlist[0] = '\0';
	cdevlist[0] = '\0';
	dtab_alias[0] = '\0';

	/* read input lines for each subdevice, then make the device nodes */
	while ((p = ParseLine(field,templatefp,NUMFIELDS)) != EOF) {

		if (p != NUMFIELDS) {
			break; /* corrupted template file */
		}

		if (ghost_status)
			minornum = CalculateMinorNum(field[MINOR],&real_sa,drvr,edtptr);
		else
			minornum = CalculateMinorNum(field[MINOR],sa,drvr,edtptr);
		modenum = (int) strtol(field[MODE],(char **)NULL,8);

		/* generate the special device file names */
		good_name[BLOCK] = MakeNodeName(devname[BLOCK],dir[BLOCK],field[BLOCK],sa);
		good_name[CHAR] = MakeNodeName(devname[CHAR],dir[CHAR],field[CHAR],sa);
		good_name[SA] = MakeNodeName(devname[SA],dir[SA],field[SA],sa);
		good_name[RSA] = MakeNodeName(devname[RSA],dir[RSA],field[RSA],sa);
		if ( Sdimkdev || UpdateMode ) {
			tc_cmajor = DRVRinfo[drvr].majors.c_maj;
			tc_bmajor = DRVRinfo[drvr].majors.b_maj;
			cdev_num = makedev(tc_cmajor, minornum);
			bdev_num = makedev(tc_bmajor, minornum);

			if ( good_name[BLOCK] ) {
				NodesCreated = MakeSpecial(devname[BLOCK], modenum, S_IFBLK, bdev_num);
				fprintf(contents_fp,"%s b %d %d ? ? ? %d NULL NULL\n",devname[BLOCK],major(bdev_num),minor(bdev_num),level);
			}
			
			if ( good_name[CHAR] ) {
				NodesCreated = MakeSpecial(devname[CHAR], modenum, S_IFCHR, cdev_num);
				fprintf(contents_fp,"%s c %d %d ? ? ? %d NULL NULL\n",devname[CHAR],major(cdev_num),minor(cdev_num),level);
			}
			
			/* link the system admin names if necessary */
			if ( good_name[SA] ) {
				NodesCreated = LinkSpecial(devname[SA], devname[BLOCK], S_IFBLK, bdev_num);
				fprintf(contents_fp,"%s=%s l\n",devname[SA],devname[BLOCK]);
			}
			
			if ( good_name[RSA] ) {
				NodesCreated = LinkSpecial(devname[RSA], devname[CHAR], S_IFCHR, cdev_num);
				fprintf(contents_fp,"%s=%s l\n",devname[RSA],devname[CHAR]);
			}
		}

		if ( !Sdimkdev || UpdateMode ) {
			if (strchr(field[KEY], 'O') != NULL) {
				/* Make device table entry.  Also make
				 * entries for disk slices for disk devices.
				 */
				(void)MakeDeviceTable(devname[BLOCK], devname[CHAR], edtptr->xedt_tcinquiry, edtptr->xedt_pdtype, dtab_alias, gsp);
				umask(0);
			} else if (strcmp(alias, "disk") != 0) {
				if ( good_name[BLOCK] ) {
					strcat(bdevlist, devname[BLOCK]);
					strcat(bdevlist, ",");
				}
				if ( good_name[CHAR] ) {
					strcat(cdevlist, devname[CHAR]);
					strcat(cdevlist, ",");
				}
			}
			if ( good_name[SA] ) {
				strcat(bdevlist, devname[SA]);
				strcat(bdevlist, ",");
			}
			if ( good_name[RSA] ) {
				strcat(cdevlist, devname[RSA]);
				strcat(cdevlist, ",");
			}
		}

	} /* end of ParseLine while loop */

	/* Attempt to build the extended nodes */
	if ( (Sdimkdev || UpdateMode) && !strcmp(alias, "disk") ) {
 		NodesCreated = makeExtNodes(dir, sa, drvr, edtptr);
	}

	if ( !Sdimkdev || UpdateMode ) {
		(void)UpdateDeviceTable( bdevlist, cdevlist,  dtab_alias );
		umask(0);
	}

	if (!Silent && NodesCreated) {
		(void) pfmt(stdout, MM_INFO, ":405:\nDevice files have been created for a new %s device:\n", Device_Types[edtptr->xedt_pdtype].string);
		(void) pfmt(stdout, MM_NOSTD, ":406:Host Adapter (HA)          = %d\n", sa->sa_c);
		if (sa->sa_b) {
		(void) pfmt(stdout, MM_NOSTD, ":407:SCSI Bus (BUS)             = %d\n", sa->sa_b);
		}
		(void) pfmt(stdout, MM_NOSTD, ":408:Target Controller (TC) ID  = %d\n", sa->sa_t);
		(void) pfmt(stdout, MM_NOSTD, ":409:Logical Unit (LU) ID       = %d\n",sa->sa_l);
		if (postmsg[0] != BLANK) {
			(void) pfmt(stdout, MM_NOSTD, postmsg);
		}
	}

	(void) fclose(templatefp);
	if (ghost_status) {
		*sa = real_sa;
	}
}

/*
 * Create the special nodes used before the
 * root filesystem is mounted read/write
 */
void
E333A_nodes(void)
{
	int	offset, i;
	char dev_name[256], link_name[256];

	if (!disks.disk_valid)
		return;

	(void)mkdir(specials[0].dir,DIRMODE);

	for (i = 0; i < (sizeof(specials) / sizeof(io_template_t)); i++) {
		sprintf(dev_name, "%s", specials[i].linkdir);

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

int sdimkcmd(int argc, char **argv);

int
main(int argc, char **argv)
{
	(void) setlocale(LC_ALL, "");
	(void) setcat("uxdisksetup");

	(void) strcpy(Cmdname,basename(argv[0]));

	/*
	 * Check if Device Database exists and is in consistent state 
	 * This will also initialize the __tabversion__ variable wich
	 * must be initialized before any access to the device table 
	 */
	if (ddb_check() <= 0) {
		/* Not all the DDB files are present/accessible.    */
		error(":511:One or more of the DDB files are not present/corrupt/inaccessible\n");
	} 

	if (!strcmp(Cmdname, SDIGHOST)) 
		sdighost(argc, argv);
	else
		sdimkcmd(argc, argv);
}

int sdimkcmd(int argc, char **argv)
{
	register EDT 	*xedtptr  = NULL;	 /* Pointer to edt */
	register EDT 	*xedtptr2 = NULL;	 /* Temp pointer   */
	register int	c, t, lu, e;
	int		ntargets, scsicnt;
	struct scsi_addr	sa;
	char		tcinq[INQ_EXLEN];
	int		edtcnt, i, arg, force_to_run, ppid;
	int		ignore_old_edt, driver_index;
	extern char	*optarg;
	int		no_match;
	STAT		ostatbuf, nstatbuf;
	int		c1, c2;
	char		*label;
	dev_t	sdi_dev;
	major_t	sdi_major;
	int		unsorted;
	ulong_t	tempd, disk1;
	
	/*
	 * Force the command to be sdimkdev if it was run as sdimkdtab
	 * but no arguements were given. Sigh... Can't change it.
	 * Compatability, you know.
	 */

	Sdimkdtab = !strcmp(Cmdname, PDIMKDTAB) || !strcmp(Cmdname, SDIMKDTAB);
	Sdimkdev = !Sdimkdtab;

#ifdef DEBUG
	printf("sdimkcmd: Sdimkdtab=%d, Sdimkdev=%d\n", Sdimkdtab, Sdimkdev);
#endif

	label = (char *) malloc(strlen(Cmdname)+1+3);
	sprintf(label, "UX:%s", Cmdname);
	(void) setlabel(label);

	if (lvlin(SYS_PUBLIC, &level) == 0)
		ES_attr = "mode=static state=private range=SYS_RANGE_MAX-SYS_RANGE_MIN startup=no ual_enable=yes other=\">y\"";

	disks.disk_valid = FALSE;

	/* set TCindex to default location */
	(void) strcpy(TCindex,TCINDEX);

	/* set the default MAC level for all nodes to SYS_PRIVATE */
	level = 2;

	Debug = FALSE;
	ignore_old_edt = FALSE;
	force_to_run=FALSE;
	while ((arg = getopt(argc,argv,"Sd:isfu")) != EOF)

		switch (arg) {
		case 'd' : /* alternate tc index supplied */
			(void) strcpy(TCindex,optarg);
			break;
		case 's' : /* Silent mode */
			Silent = TRUE;
			break;
		case 'f' : /* force to run regardless of run-level */
			force_to_run = TRUE;
			break;
		case 'S' : /* Turn on debug messages */
			Debug = TRUE;
			break;
		case 'i' : /* ignore configuration saved in previous boot */
			ignore_old_edt = TRUE;
			break;
		case 'u' : /* update mode  for hot insert/removal */
			UpdateMode = TRUE;
			force_to_run = TRUE;
			ignore_old_edt = TRUE;
			break;
		case '?' : /* Incorrect argument found */
			before_exit(FALSE);
			errno = 0;
			error(":410:Usage: %s [-f] [-i] [-d file].\n", Cmdname);
			/*NOTREACHED*/
			break;
		}
	
	/* if the parent is init then run this command */
	ppid = getppid();
	if (ppid == 1) {
		force_to_run = TRUE;
	}

	/* Print error unless forced to run */
	/* or running as sdimkdtab         */
	if (!force_to_run && Sdimkdtab) {
		before_exit(FALSE);
		errno = 0;
		error(":411:This command is designed to run during the boot sequence.\nUse the -f option to force its execution.\n");
	}

	/* Ignore certain signals */
if (!Debug) {
	(void) signal(SIGHUP,SIG_IGN);
	(void) signal(SIGINT,SIG_IGN);
	(void) signal(SIGTERM,SIG_IGN);
}

	umask(0); /* use template file permission (mode) tokens */

	/* Check to see that the tc.index file exists. */
	if (!FileExists(TCindex)) {
		before_exit(FALSE);
		error(":412:index file %s does not exist.\n", TCindex);
	}

	/* Open the target controller index file. Exits on failure.  */
	if ((TCindexfp = fopen(TCindex,"r")) == NULL) {
		before_exit(FALSE);
		error(":400:Could not open %s.\n", TCindex);
	}

	/* Initialize driver info structure */
	for(i = 0; i < MAXDRVR; i++) {
		DRVRinfo[i].valid = FALSE;
	}

	tload(NULL);
	if ((xedtptr = readxedt(&edtcnt)) == 0) {
		tuload();
		before_exit(FALSE);
		error(":413:Unable to read equipped device table.\n");
	}
	tuload();
	if (Debug) {
		printf("edtcnt %d\n", edtcnt);
	}

	if (!ignore_old_edt) {
		if (!FileExists(LAST_BOOT_EDT)) 
			ignore_old_edt = TRUE;
		else
			if ((last_edt_fp = fopen(LAST_BOOT_EDT,"r")) == NULL) 
				ignore_old_edt = TRUE;
	}

	if ((temp_edt_fp = fopen(TEMP_EDT,"w+")) == NULL) {
		before_exit(FALSE);
		error(":400:Could not open %s.\n", TEMP_EDT);
	}
	
	/* write current edt to temp file */
	fwrite (xedtptr, sizeof(struct scsi_xedt), edtcnt, temp_edt_fp);

	/* compare the size of old and new files. no need to do byte
	 * comparison if the sizes are different */
	if (!ignore_old_edt) {
		rewind(temp_edt_fp);	/* get offset from the file beginning */
		if (stat(TEMP_EDT, &nstatbuf) < 0) {
			before_exit(FALSE);
			error(":396:stat failed for %s\n", TEMP_EDT);
		}
		if (stat(LAST_BOOT_EDT, &ostatbuf) < 0) {
			before_exit(FALSE);
			error(":396:stat failed for %s\n", LAST_BOOT_EDT);
		}
		if ( ostatbuf.st_size != nstatbuf.st_size )
			ignore_old_edt = TRUE;
	}

	if (!ignore_old_edt) {

		no_match = FALSE;

		/* byte compare previous and current edt */ 
		while ((c1=getc(temp_edt_fp))!= EOF && (c2=getc(last_edt_fp))!= EOF) {
			if (c1 != c2) {
			no_match = TRUE;
			break;
			}
		}

		fclose(temp_edt_fp);
		fclose(last_edt_fp);
		if (!no_match) {
			struct stat edvtocstat;

			unlink (TEMP_EDT);

			/*
			 * Check for the file-flag that edvtoc leaves behind.
			 */
			if (stat(EDVTOC_LOCK, &edvtocstat) < 0) {
				exit(NO_CHANGES_EXIT);
			}
			unlink (EDVTOC_LOCK);
		}
	} else
		fclose(temp_edt_fp);

	rename(TEMP_EDT, LAST_BOOT_EDT);
	chmod(LAST_BOOT_EDT, MASK);

	/*
	 * Now that we know we have something to do, open the input file
	 * we create for sdimkdtab so that it can update the contents file.
	 */
	if ( Sdimkdev ) {
		if ((contents_fp = fopen(INSTALLF_INPUT,"w+")) == NULL)
			error(":400:Could not open %s.\n", INSTALLF_INPUT);
	}

	/*
	 * set the owner of host adapter entries to SDI
	 *
	 * "VOID" means a device not yet claimed by a target driver.
	 * ID_PROCESOR indicates a host adapter.
	 *
	 * This allows a processor target driver to claim certain ID_PROCESOR
	 * devices and sdi will get the rest.
	 */
	/* get device for sdi */
	if (sysi86(SI86SDIDEV, &sdi_dev) == -1) {
		errno = 0;
		error(":387:ioctl(SI86SDIDEV) failed.\n");
	}

	xedtptr2 = xedtptr;
	sdi_major = major(sdi_dev) - 1;
	for (e = 0; e < edtcnt; ++e, xedtptr2++) {
	 	if (!strcmp((char *) xedtptr2->xedt_drvname,"VOID") &&
				xedtptr2->xedt_pdtype == ID_PROCESOR) {
			strcpy(xedtptr2->xedt_drvname, "SDI");
			xedtptr2->xedt_cmaj = sdi_major;
			xedtptr2->xedt_bmaj = sdi_major;
			xedtptr2->xedt_minors_per = 32;
		} 
	}

#ifdef DPRINTF
	PrintEDT(xedtptr, edtcnt);
#endif
	unsorted = edt_fix(xedtptr, edtcnt);

#ifdef DPRINTF
	PrintEDT(xedtptr, edtcnt);
#endif
	scsicnt = edt_sort(xedtptr, edtcnt, HBA, unsorted, FALSE);

#ifdef DPRINTF
	PrintEDT(xedtptr, edtcnt);
#endif

	disk1 = ORDINAL_A(MAX_EXHAS,MAX_BUS,MAX_EXTCS) + 1;
	disks.disk_count = 0;
	for ( xedtptr2 = xedtptr, e = 0; e < edtcnt; e++, xedtptr2++ ) {
		if (xedtptr2->xedt_pdtype == ID_RANDOM) {
			disks.disk_count++;
			tempd = xedtptr2->xedt_ordinal + xedtptr2->xedt_lun;
			if (tempd < disk1) {
				disk1 = tempd;
				disks.disk_cmajor = xedtptr2->xedt_cmaj;
				disks.disk_bmajor = xedtptr2->xedt_bmaj;
				disks.disk_minor  = xedtptr2->xedt_first_minor;
			}
		}
	}
	disks.disk_valid = TRUE;

	for ( xedtptr2 = xedtptr, e = 0; e < edtcnt; e++, xedtptr2++ ) {
		if (!strcmp((char *) xedtptr2->xedt_drvname,"VOID")) {
			continue;
		}
		for (driver_index = -1, i = 0; i < MAXDRVR; i++) {
			if (DRVRinfo[i].valid == TRUE) {
				if (!strcmp(DRVRinfo[i].name, xedtptr2->xedt_drvname)) {
				/* found it */
					driver_index = i;
					break;
				}
			} else {
				DRVRinfo[i].valid = TRUE;
				strcpy(DRVRinfo[i].name, xedtptr2->xedt_drvname);
				DRVRinfo[i].majors.b_maj = xedtptr2->xedt_bmaj;
				DRVRinfo[i].majors.c_maj = xedtptr2->xedt_cmaj;
				DRVRinfo[i].subdevs = xedtptr2->xedt_minors_per;
				DRVRinfo[i].lu_occur = -1;
				DRVRinfo[i].instances = 1;
				driver_index = i;
				break;
			}
		}

		if (driver_index < 0) {
			errno = 0;
			error(":414:Too many drivers to start. Only %d drivers supported.\n", MAXDRVR);
		}
	}

	if (!ghost_cnt) /* load ghost names only if they are not loaded */
		GetGhostNames();

	if ( !Sdimkdev || UpdateMode )
		GetDeviceTable();

	if (Debug) {
		printf("driver\tHA\tBUS\tTC\tLUN\tmemaddr\tStamp\tPDtype\tTCinq\n");
	}

	for ( c = 0; c < scsicnt; c++ ) {

		/*
		 * The structure sa is used to pass the current values
		 *	of C,B,T,L and N out of this loop into MakeDeviceNodes
		 *
		 * We need to actually number things from 0 as we start
		 *	here.  So the controller number used to name all nodes
		 *	we make or link in this loop is simply incremented.
		 */


		/*
		 * now start in the order we want to really make the devices in.
		 *
		 * The device that we are making nodes for, as described by
		 *	the array of edt entries, may not be the first device in the
		 *	edt ( Equipped Device Table ).  This is caused by constraints
		 *	on ordering the HBA table imposed by the Loadable Driver stuff.
		 *
		 * So, lets set our starting point for each controller.  We need
		 *	a pointer into the edt that reflects actual position in the edt.
		 *
		 * Now lets make devices for all the targets and luns used by
		 *	the edt_order'th controller in the HBA table.
		 *
		 */

		xedtptr2 = HBA[HBA[c].order].edtptr;
		ntargets = HBA[HBA[c].order].ntargets;
		sa.sa_c = xedtptr2->xedt_ctl;;

		for (t = 0; t < ntargets; t++, xedtptr2++) {

			sa.sa_b = xedtptr2->xedt_bus;
			sa.sa_t = xedtptr2->xedt_target;

			strcpy(tcinq,(char *) xedtptr2->xedt_tcinquiry);

			if (Debug) {
				printf("%s\t%d\t%d\t%d\t%d\t0x%x\t\"%12s\"\t%d\t%s\n",
					xedtptr2->xedt_drvname,
					xedtptr2->xedt_ctl,
					xedtptr2->xedt_bus,
					xedtptr2->xedt_target,
					xedtptr2->xedt_lun,
					xedtptr2->xedt_memaddr,
					(char *) &xedtptr2->xedt_stamp,
					xedtptr2->xedt_pdtype,
					xedtptr2->xedt_tcinquiry);
			}

			if (xedtptr2->xedt_drvname == NULL ||
			   (strcmp((char *) xedtptr2->xedt_drvname,"VOID") == 0)) {
				continue;
			}

			/* find the DRVRinfo entry */
			for(driver_index = -1, i = 0; i < MAXDRVR; i++) {
				if(DRVRinfo[i].valid == TRUE) {
					if (!strcmp(DRVRinfo[i].name, xedtptr2->xedt_drvname)) {
						driver_index = i;
						break;
					}
				} else {
					break;
				}
			}
			if (driver_index == -1) {
				before_exit(FALSE);
				errno = 0;
				error(":415:Too many drivers. Only %d drivers supported.\n", MAXDRVR);
			}

			for(lu = 0; lu < MAX_EXLUS; lu++) {
			    if(xedtptr2->xedt_lun == lu) {

					sa.sa_l = lu;
					sa.sa_n = DRVRinfo[driver_index].instances++;

					DRVRinfo[driver_index].lu_occur++;

					MakeDeviceNodes(&sa, driver_index, xedtptr2);
				}
			} /* end LU loop */
		} /* end TC loop */
	} /* end HA loop using boot-chain based index into EDT */

	if ( !Sdimkdev || UpdateMode ) {
		ClearDeviceTable();
		umask(0);
	}

	(void) fclose (TCindexfp);

	/* make compatibility nodes and update the contents file */
	/* before_exit does nothing if this is mkdtab */
	before_exit(TRUE);

	exit(NORMEXIT);
	/* NOTREACHED */
}


/* OA&M population utility code starts here */

#define SCSI_ATTR	"scsi=true"
#define ATTR_TERM	(char *)NULL

#define GOODEXIT	0
#define BADEXIT		-1

#define DONE		1
#define NOT_DONE	0

typedef struct type_value
{
	char	type[MAXLINE];
	int	N;
	struct type_value *next;
} TYPE_VALUE_T;
struct type_value	*list;

static int getvar(uchar_t *, char *, char *, char *, char *, char *, uchar_t);
static int  get_serial_n();
static void expand();
#ifdef OUT
static int		vfsnum;
static struct vfstab	*vfstab;

static void add_dpart();
static int  initialize();
#endif /* OUT */

static char **dev_list = NULL;
static int  dev_cnt = 0;

/*
 *  GetDeviceTable
 *       get all the scsi device entries from the device table
 *
 *       returns 0 on success, -1 on failure
 */
int
GetDeviceTable(void)
{
	char	*criteria_list[2];	/* Two pointers needed, one for
					   "scsi=true" and one for NULL */
	char	**criteria_ptr;
	char	**dev_ptr;

	/* initialize list to empty (list used in get_serial_n()) */
	list = (struct type_value *)NULL;

	/* get list of devices with attribute scsi=true */
	/* build criteria list */
	criteria_ptr = criteria_list;
	*criteria_ptr++ = SCSI_ATTR;
	*criteria_ptr = ATTR_TERM;

	if ( (dev_list = getdev((char **)NULL, criteria_list, DTAB_ANDCRITERIA)) == (char **)NULL)
	{
		warning(":416:Unable to clear device table %s, %s failure.\n", DTAB_PATH, "getdev");
		return(BADEXIT);
	}

	/* count the number of entries */
	dev_cnt = 0;
	for (dev_ptr = dev_list; *dev_ptr != (char *)NULL; dev_ptr++) {
		dev_cnt++;
#ifdef DPRINTF
		printf("%s ", *dev_ptr);	
#endif
	}
#ifdef DPRINTF
		printf("\n");	
#endif

#ifdef OUT
	/* initialize copy of vfstab in memory for use in adding dpart entries */
	return(initialize());
#else
	return(GOODEXIT);
#endif /* OUT */
}

/*
 *  RemoveDeviceTableEntry
 *       removes a device entry from the device table
 *       also removes the device entry from the scsi device groups
 *            but not from administrator defined groups
 *
 *       returns 0 on success, -1 on failure
 */
int
RemoveDeviceTableEntry(char *device, int done)
{
	char	*criteria_list[3];	/* Three pointers needed, one each for
					   "scsi=true", "type=<device type>" 
					   and NULL */
	char	**criteria_ptr;
	char	*type;
	char	criteria[MAXLINE];
	char	**dgrp_list;
	char	**dgrp_ptr;
	int	cnt;
	char	attrlist[2*MAXLINE];
	char	*argv[2];
	char	**notfound;

	 /* first find out which type scsi device we are removing */
	if ( (type = devattr(device, "type")) == (char *)NULL) {
		warning(":416:Unable to clear device table %s, %s failure.\n", DTAB_PATH, "devattr");
		return(BADEXIT);
	}
	/* build criteria list */
	criteria_ptr = criteria_list;
	*criteria_ptr++ = SCSI_ATTR;

	/* then get a list of groups with that device type */
	(void) sprintf(criteria, "type=%s", type);
	*criteria_ptr++ = criteria;
	*criteria_ptr = ATTR_TERM;
	if ( (dgrp_list = getdgrp((char **)NULL, criteria_list, DTAB_ANDCRITERIA)) == (char **)NULL) {
		/*
		 * This will only occur if an error occurs during getdgrp.
		 * If no groups are present meeting the criteria, a list
		 * with the first element a NULL pointer is returned.
		 * This is true for all device table functions.
		 */
		warning(":417:Unable to clear device group table %s, getdgrp failure.\n", DGRP_PATH);
		return(BADEXIT);
	}

	/* remove the device from the SCSI device groups */
	for (dgrp_ptr = dgrp_list; *dgrp_ptr != (char *)NULL; dgrp_ptr++) {
		if (strncmp(*dgrp_ptr, "scsi", 4) != 0)
			continue;
		/*  attempt to remove device from group
		 *  ignore return code since we didn't
		 *       check that device is a member of dgrp
		 */
		argv[0] = attrlist;
		(void) sprintf(argv[0],"%s",device);
		argv[1] = NULL;
#ifdef DPRINTF
		printf("_rmdgrpmems(%s, %s)\n", *dgrp_ptr, argv[0]);
#endif
		(void) _rmdgrpmems(*dgrp_ptr, argv, &notfound);
		free((char *)notfound);
	}
	free((char *)dgrp_list);
	free(type);

	/* remove device from device table */
	(void) remdevrec(device, FALSE);

	if (done) {
		/* these lines are a work-around for a bug in libadm
		 * close the device and device group tables so that
		 * the above removals will be written to the tables
		 */
		_enddevtab();
		_enddgrptab();
	}

	return(GOODEXIT);
}

/* 
 * Check if the device already exists in the device table.
 * Return pointer to the device entry in the list if the entry exists
 * otherwise return NULL.
 */
char **
CheckDeviceTable(char *device)
{
	int	cnt;
	char	**dev_ptr;

	for (cnt = 0, dev_ptr = dev_list; cnt < dev_cnt; cnt++, dev_ptr++) {
		if (*dev_ptr == NULL)
			continue;
		if (!strcmp(*dev_ptr, device))
			return (dev_ptr);
	}

	return (NULL);
}

/*
 *  ClearDeviceTable
 *      removes all scsi device entries which exist in the device list
 *	from the device table
 *      also removes scsi device entries from the scsi device groups
 *            but not from administrator defined groups
 *
 *      returns 0 on success, -1 on failure
 */
int
ClearDeviceTable(void)
{
	char	*criteria_list[3];	/* Three pointers needed, one each for
					   "scsi=true", "type=<device type>" 
					   and NULL */
	char	**criteria_ptr;
	char	**dev_ptr;
	char	*type;
	char	criteria[MAXLINE];
	char	**dgrp_list;
	char	**dgrp_ptr;
	int	cnt;
	char	attrlist[2*MAXLINE];
	char	*argv[2];
	char	**notfound;

	if ( dev_list == (char **)NULL) {
		/* nothing to be cleared */
		return(GOODEXIT);
	}

	/* build criteria list */
	criteria_ptr = criteria_list;
	*criteria_ptr = SCSI_ATTR;

	for (cnt = 0, dev_ptr = dev_list; cnt < dev_cnt; cnt++, dev_ptr++) {
		if (*dev_ptr == NULL)
			continue;

		if (Debug) 
			printf("Removing %s from device.tab\n", *dev_ptr);

		RemoveDeviceTableEntry(*dev_ptr, NOT_DONE);

		*dev_ptr = NULL;
	}
	/* free((char *)dev_list); */
	dev_list = NULL;
	dev_cnt = 0;

	/* these lines are a work-around for a bug in libadm
	 * close the device and device group tables so that
	 * the above removals will be written to the tables
	 */
	_enddevtab();
	_enddgrptab();

}

/*
 *  MakeDeviceTable
 *       reads the template file to get the alias, device group
 *            and attribute list
 *       create a unique alias for the device table using the alias
 *            for example, the alias is disk and disk1, disk2 exist
 *            then the alias for this device should be disk3
 *       translate variables in the attribute list using the info
 *            passed in
 *            for example, an attribute may be prtvtoc $CDEVICE$ so
 *            substitute the character device passed in for CDEVICE
 *       add the new device to the device table
 *       add the new device to the device group
 *       if type is disk, add partition entries
 *
 *       returns 0 on success, -1 on failure
 */
int
MakeDeviceTable(char *b_dev_name, char *c_dev_name, uchar_t *tc_inquiry, uchar_t tc_pdtype, char *dtab_alias, struct ghost *gsp)
{
	char	alias[MAXLINE], dgrp[MAXLINE], attr[2*MAXLINE], fsattr[2*MAXLINE], dpattr[2*MAXLINE];
	int	serial_n;
	char	serial_n_str[5];
	char	exp_alias[MAXLINE];
	char	exp_attr[2*MAXLINE];
#ifdef OUT
	char	dpartlist[16*MAXLINE];
#endif /* OUT */
	char	**dev_ptr;
	int	found;
	int	argc, i;
	char	*argv[64];
	char	attrlist[4*MAXLINE];

	(void) sprintf(alias, "");
	(void) sprintf(dgrp, "");
	(void) sprintf(attr, "");
	(void) sprintf(fsattr, "");
	(void) sprintf(dpattr, "");
#ifdef OUT
	(void) sprintf(dpartlist, "");
#endif /* OUT */
	
	/* retrieve alias, device group and attribute list */
	if (getvar(tc_inquiry, alias, dgrp, attr, fsattr, dpattr, tc_pdtype) == BADEXIT)
	{
		/* set errno to zero so that warning will not use perror */
		errno = 0;
		warning(":418:Unable to retrieve alias, device group and attribute list\n\tfrom template file specified by tc_inquiry string %s.\nNo device entry added to device table %s\n\tfor character device %s.\n", tc_inquiry, DTAB_PATH, c_dev_name);
		return(BADEXIT);
	}

	/* get number value necessary to create unique alias */
	serial_n = get_serial_n(alias);
	(void) sprintf(serial_n_str, "%d", serial_n);
	(void) sprintf(exp_alias, "%s%d", alias, serial_n);

	/* return the expanded alias string in dtab_alias */
	(void)strcpy(dtab_alias,exp_alias);

	/* expand variables in attribute list to character or block device
	 * passed in
	 */
	expand(b_dev_name, c_dev_name, serial_n_str, attr, exp_attr, tc_inquiry);
	/* check if the device already exists */
	found = FALSE;
	if ( (dev_ptr = CheckDeviceTable(exp_alias)) != NULL) {
		found = TRUE;
	}

	/* add new device to device table */
	/* need more than one form of command because the block device
	 * name may be null and putdev no longer accepts null parameters
	 */
	if (b_dev_name == (char *)NULL || strlen(b_dev_name) == 0) {
		sprintf(attrlist, "%s=%s %s", DTAB_CDEVICE,c_dev_name,exp_attr);
	}
	else {
		sprintf(attrlist, "%s=%s %s=%s %s", DTAB_BDEVICE, b_dev_name,
					DTAB_CDEVICE, c_dev_name, exp_attr);
	}
	argc = 0;
	attr_to_arg(argv, &argc, attrlist);
	if (found) {
		/* check the attributes */
		if (checkattrs(*dev_ptr, argv, argc)) {
			found = FALSE;
			RemoveDeviceTableEntry(*dev_ptr, DONE);
			*dev_ptr = NULL;
		}
	}

	if (found)
		*dev_ptr = NULL;
	else {
		attr_to_arg(argv, &argc, ES_attr);
		argv[argc] = NULL;
		(void) adddevrec(exp_alias, argv, FALSE);

		/* add new device to device group table */
		argv[0] = attrlist;
		(void) sprintf(argv[0],"%s",exp_alias);
		argv[1] = NULL;
#ifdef DPRINTF
		printf("_adddgrptabrec(%s, %s)\n", dgrp, argv[0]);
#endif
		(void) _adddgrptabrec(dgrp, argv);
	}
	if (strcmp(alias, "disk") == 0) {
		PutGhostNames(dtab_alias, gsp, found);
	}

	return(GOODEXIT);
}

/*
 *  UpdateDeviceTable
 *
 *       add the bdevlist and cdevlist attributes to the device table
 *
 *       returns 1 on success, 0 on failure
 */
int
UpdateDeviceTable(char *bdevlist, char *cdevlist, char *dtab_alias)
{
	int		blen,clen;
	char	attrlist[4*MAXLINE];
	char	*argv[3];

	if ( ! strlen(dtab_alias) )	/* no alias means no 'O' in template */
		return(0);

	blen = strlen(bdevlist);
	if ( blen ) { /* was anything added to bdevlist */
		bdevlist[blen-1] = '\0';	/* yes, remove trailing comma */
		/* check the existing attribute */
		if ( !checkattr(dtab_alias, DDB_BDEVLIST, bdevlist) ) 
			blen = 0;
	}

	clen = strlen(cdevlist);
	if ( clen ) {	/* was anything added to cdevlist */
		cdevlist[clen-1] = '\0';	/* yes, remove trailing comma */
		/* check the existing attribute */
		if ( !checkattr(dtab_alias, DDB_CDEVLIST, cdevlist) ) 
			clen = 0;
#ifdef DPRINTF
		else
			printf("%s: Device attribute (%s) mismatch: %s\n", dtab_alias, DDB_CDEVLIST, cdevlist);
#endif
	}
	
	if ( !blen && !clen )	/* nothing in either, nothing to do */
		return(0);

	if ( blen && clen ) {
		argv[0] = attrlist;
		(void)sprintf(attrlist, "%s=%s", DDB_BDEVLIST, bdevlist);
		argv[1] = &attrlist[strlen(argv[0]) + 1];
		(void)sprintf(argv[1], "%s=%s", DDB_CDEVLIST, cdevlist);
		argv[2] = NULL;
	}
	else if ( blen ) {
		argv[0] = attrlist;
		(void)sprintf(attrlist, "%s=%s", DDB_BDEVLIST, bdevlist);
		argv[1] = NULL;
	}
	else if ( clen ) {
		argv[0] = attrlist;
		(void)sprintf(attrlist, "%s=%s", DDB_CDEVLIST, cdevlist);
		argv[1] = NULL;
	}
	
#ifdef DPRINTF
	if (argv[1]) 
		printf("argc=2 argv[]=%s %s\n", argv[0], argv[1]);
	else
		printf("argc=1 argv[]=%s\n", argv[0]);
#endif
	(void)moddevrec(dtab_alias, argv, FALSE);
	return(1);
}

/*
 * getvar
 *
 * open template file using tc_inquiry string
 * read template file searching for the ALIAS, DGRP, ATTR and possibly
 *      FSATTR and DPATTR tokens
 * close template file
 */
static int
getvar(uchar_t *tc_inquiry, char *alias, char *dgrp, char *attr, char *fsattr, char *dpattr, uchar_t tc_pdtype)
{
	FILE		*templatefp;
	int		data_begin;
	register int	tokenindex;

	if ((templatefp = OpenTemplate(tc_pdtype, tc_inquiry)) == NULL)
		return(BADEXIT);

	data_begin = 0;
	while (!data_begin)
	{
		tokenindex = GetToken(templatefp);
		switch(tokenindex)
		{
			case ALIAS:
				(void) fscanf(templatefp, " %[^\n]\n", alias);
				break;
			case DGRP:
				(void) fscanf(templatefp, " %[^\n]\n", dgrp);
				break;
			case ATTR:
				(void) fscanf(templatefp, " %[^\n]\n", attr);
				break;
			case FSATTR:
				(void) fscanf(templatefp, " %[^\n]\n", fsattr);
				break;
			case DPATTR:
				(void) fscanf(templatefp, " %[^\n]\n", dpattr);
				break;
			case DATA:
				data_begin = 1;
				break;
			case EOF:
				(void) fclose(templatefp);
				return(BADEXIT);
				/* NOTREACHED */
				break;
			default:
				(void) fscanf(templatefp, "%*[^\n]%*[\n]");
				break;
		}
	}
	(void) fclose(templatefp);
	if (strlen(alias) == 0 || strlen(dgrp) == 0 || strlen(attr) == 0)
		return(BADEXIT);
	return(GOODEXIT);
}

/*
 *  get_serial_n
 *
 *  get entries already in device table of this type
 *  find the current highest value of N in typeN
 *  add 1 to N and return for use in new alias
 */
static int
get_serial_n(char *alias)
{
	struct type_value	*ptr;
	struct type_value	*lastptr;
	int 			n;

	/* has this type been evaluated before */
	for (ptr = list, lastptr = list; ptr != (struct type_value *)NULL; ptr = ptr->next)
	{
		/* save value for end of list in case another member must be added */
		if (ptr->next != (struct type_value *)NULL)
			lastptr = ptr->next;
		if (strcmp(alias, ptr->type) == 0)
			break;
	}

	if (ptr != (struct type_value *)NULL)
	{
		/* N value for this type has been found from table already */
		n = ptr->N;
		ptr->N = n + 1;
		return(n);
	}

	/* add this type to the list */
	n = 1;
	ptr = (struct type_value *)malloc(sizeof(struct type_value));
	(void) strcpy(ptr->type, alias);
	ptr->N = n + 1;
	ptr->next = (struct type_value *)NULL;
	if (lastptr == (struct type_value *)NULL)
		/* first member of list */
		list = ptr;
	else
		lastptr->next = ptr;

	/* return N for use in creating alias */
	return(n);
}

/*
 * convert a string of arguments to a argv style list of arguments 
 * in order to be able to pass it to the devmgmt library functions.
 * most of this string is obtained from the template file
 */
attr_to_arg(char **argv, int *argc, char *attrlist)
{
	register char *p, *s;
	int	done, i;

#ifdef DPRINTF
	printf("attrlist = %s\n", attrlist);
#endif
	p = attrlist;
	i = *argc;

	while(*p && isspace(*p)) p++;
	while (*p) {
		argv[i++] = p;
		p = strchr(p, '=');
		p++;		/* skip '=' */
		if (*p == '"') {
			s = p;
			p++;	/* skip '"' */
			done = FALSE;
			while (!done) {
				while (*p != '"') *s++ = *p++;
				if (*(p-1) == '\\') 	/* check if escaped */
					*(s-1) = *p++;
				else 
					done = TRUE;
			}
			*s = '\0';
			p++;		/* skip trailing '"' */
		}
		else {
			while (*p && !isspace(*p)) p++;
			if (*p)
				*p++ = '\0';
		}
		while(*p && isspace(*p)) p++;
	}
	*argc = i;
#ifdef DPRINTF
	printf("argc=%d, argv[]=", *argc);
	for (i = 0;i < *argc; i++)
		printf("%s ", argv[i]);
	printf("\n");
#endif
}
/*
 * checkattr -
 * Check for the existance and correctness of one attribute 
 * Return value:
 *	0 if the attribute exists and has same value
 *	1 otherwise
 */
int
checkattr(char *alias, char *attr, char *value)
{
	char *dev_attr = NULL;
	int ret = 0;
	
#ifdef GDEBUG
	printf("checkattr: alias=%s attr=%s value=%s\n",alias, attr, value);
#endif
	if ( ((dev_attr = devattr(alias, attr)) == (char *)NULL) || strcmp(dev_attr, value) ) {
		ret = 1;
#ifdef GDEBUG
		printf("%s: %s attribute mismatch (%s %s)\n", alias, attr, dev_attr, value);
#endif
	}
	free(dev_attr);
	return (ret);
}

/*
 * Compare all the attributes (passed in argv parameter) of a device entry
 * to those already present in the device table. If all the attributes match
 * return 0 otherwise return 1
 */
int
checkattrs(char *device, char**argv,int argc)
{
	register char *p;
	int i;
	char attr[MAXLINE];
	char *attr_name, *attr_val;
	
	for (i = 0;i < argc; i++) {
		(void) strcpy(attr, argv[i]);
		attr_name = attr;
		p = strchr(attr, '=');
		if (p)
			*p++ = '\0';
		else
			return (1);

		attr_val = p;

		if ( checkattr(device, attr_name, attr_val) )
			return (1);
	}
	return (0);
}

/*
 *  expand
 * 
 *  variables in attribute list are surrounded by $ characters
 *  find $ character and translate next token
 */
static void
expand(char *b_dev_name, char *c_dev_name, char *serial_n_str, char *attr, char *exp_attr, uchar_t *tc_inquiry)
{
	char	*attr_var;
	char	*dev_name;
	char	*dev;

	/* dev_name is device name without path (cNtNdNsN)
	 * dev is device name without path or slice (cNtNdN)
	 * create device name from character device name
	 * this assumes that device name is in form /dev/rxxx/device_name
 	 * or something similar
	 * strip the sN for dev
 	 */
	dev_name = strrchr(c_dev_name, '/') + 1;
	dev = strdup(dev_name);
	(void)strtok(dev, "s");

	attr_var = strtok(attr, "$");
	(void) strcpy(exp_attr, attr);
	while (attr_var != (char *)NULL)
	{
		attr_var = strtok((char *)NULL, "$");
		if (attr_var == NULL)
			continue;
		/* N value */
		if (strcmp(attr_var, "N") == 0)
		{
			(void) strcat(exp_attr, serial_n_str);
		}
		/* block device name */
		else if (strcmp(attr_var, "BDEVICE") == 0)
		{
			(void) strcat(exp_attr, b_dev_name);
		}
		/* character device name */
		else if (strcmp(attr_var, "CDEVICE") == 0)
		{
			(void) strcat(exp_attr, c_dev_name);
		}
		/* device name without character or block path */
		else if (strcmp(attr_var, "DEVICE") == 0)
		{
			(void) strcat(exp_attr, dev_name);
		}
		else if (strcmp(attr_var, "DEV") == 0)
		{
			(void) strcat(exp_attr, dev);
		}
		else if (strcmp(attr_var, "INQUIRY") == 0)
		{
			if (tc_inquiry)
				(void) strcat(exp_attr, (char *)tc_inquiry);
			else
				(void) strcat(exp_attr, "                        ");
		}
		else
		{
			errno = 0;
			warning(":421:Cannot expand variable %s.\n", attr_var);
		}

		attr_var = strtok((char *)NULL, "$");
		if (attr_var != NULL)
			(void) strcat(exp_attr, attr_var);
	}
	return;
}

#define DISK_ATTR	"type=disk"

GetGhostNames(void)
{
	char	*criteria_list[2];	/* Two pointers needed, one for
					   "type=disk" and one for NULL */
	char	**criteria_ptr;
	char	**dev_ptr, *dev_attr;

	/* get list of devices with attribute type=disk */
	/* build criteria list */
	criteria_ptr = criteria_list;
	*criteria_ptr++ = DISK_ATTR;
	*criteria_ptr = ATTR_TERM;

#ifdef GDEBUG
	printf("GetGhostNames:\n");
#endif
	if ( (ghost_names = getdev((char **)NULL, criteria_list, DTAB_ANDCRITERIA)) == (char **)NULL) {
		warning(":512:Unable to get Ghost Names from device table %s, %s failure.\n", DTAB_PATH, "getdev");
		return(BADEXIT);
	}

	/* count the number of entries */
	ghost_cnt = 0;
	for (dev_ptr = ghost_names; *dev_ptr != (char *)NULL; dev_ptr++) {
			ghost_cnt++;
	}
#ifdef GDEBUG
	printf("ghost_cnt = %d\n", ghost_cnt);	
#endif
	if (ghost_cnt == 0)
		return(GOODEXIT);

	ghost_p = (struct ghost *)calloc(ghost_cnt, sizeof(struct ghost));

#ifdef GDEBUG
	printf("Device           Stamp               Real Addr   Ghost Addr\n");
#endif
	ghost_cnt = 0; /* count the number of relevent entries only */
	for (dev_ptr = ghost_names; *dev_ptr != (char *)NULL; dev_ptr++) {
		strcpy(ghost_p[ghost_cnt].device, *dev_ptr);
		if ( (dev_attr = devattr(*dev_ptr, "stamp")) == NULL )
			continue;
		strcpy(ghost_p[ghost_cnt].stamp, dev_attr);
		free(dev_attr);
		if ( (dev_attr = devattr(*dev_ptr, "real_addr")) == NULL )
			continue;
		strcpy(ghost_p[ghost_cnt].real_addr, dev_attr);

		if ( (dev_attr = devattr(*dev_ptr, "ghost_addr")) != NULL )
			strcpy(ghost_p[ghost_cnt].ghost_addr, dev_attr);
		else
			ghost_p[ghost_cnt].ghost_addr[0] = NULL;
#ifdef GDEBUG
	printf("%s    %12s    %s    %s\n", ghost_p[ghost_cnt].device,
					ghost_p[ghost_cnt].stamp,
					ghost_p[ghost_cnt].real_addr,
					ghost_p[ghost_cnt].ghost_addr);
#endif 
		ghost_cnt++;
	}

	/* adjust the size of memory allocation */
	ghost_p = (struct ghost *)realloc(ghost_p, ghost_cnt * sizeof(struct ghost));

	return (GOODEXIT);
}

PutGhostNames(char *alias, struct ghost *gsp, int found)
{
	int	i, argc, attrlen;
	char	attrlist[2*MAXFIELD];
	char	*argv[4]; /* three attributes - stamp, real_addr, ghost_addr */
	unsigned char c;

#ifdef GDEBUG
	printf("PutGhostNames: found=%d alias=%s, stamp=%s, real_addr=%s, ghost_addr=%s\n", found, alias, gsp->stamp, gsp->real_addr, gsp->ghost_addr);
#endif
	/* check the attributes and add/modify if needed */

	argc = 0;
	attrlen = 0;
	if ( !found || checkattr(alias, "stamp", gsp->stamp) ) {
		argv[argc] = &attrlist[attrlen];
		(void)sprintf(argv[argc], "%s=%s", "stamp", gsp->stamp);
		attrlen += strlen(argv[argc]) + 1;
		argc++;
	}

	if ( !found || checkattr(alias, "real_addr", gsp->real_addr) ) {
		argv[argc] = &attrlist[attrlen];
		(void)sprintf(argv[argc], "%s=%s", "real_addr", gsp->real_addr);
		attrlen += strlen(argv[argc]) + 1;
		argc++;
	}

	if ( (gsp->ghost_addr[0] != NULL) &&
	     ( !found || checkattr(alias, "ghost_addr", gsp->ghost_addr)) ) {
		argv[argc] = &attrlist[attrlen];
		(void)sprintf(argv[argc], "%s=%s", "ghost_addr", gsp->ghost_addr);
		attrlen += strlen(argv[argc]) + 1;
		argc++;
	}
	argv[argc] = NULL;
#ifdef GDEBUG
	printf("argc=%d ", argc);
	for (i = 0; i < argc; i++)
		printf("argv[%d]=%s ", i, argv[i]);
	printf("\n");
#endif
	if (argc)
		(void)moddevrec(alias, argv, FALSE);

	if ( found &&
	     (gsp->ghost_addr[0] == NULL) && 
	     (devattr(alias, "ghost_addr") != (char *)NULL) ) {
		/* remove the ghost name entry */
		(void)sprintf(attrlist, "%s", "ghost_addr");
		argv[0] = attrlist;
		argv[1] = NULL;
#ifdef GDEBUG
		printf("argc=1 ", argc);
		printf("argv[0]=%s\n", argv[0]);
#endif
		(void)rmdevattrs(alias, argv, FALSE);
	}
}

/*
 * GhostAddress -
 * Check if the device address conflicts with a ghost address
 * or if the device address needs a ghost address
 * Returns
 *	-1	if the device address conflicts with a ghost address
 *	1  	if the device address needs a ghost address
 *	0  	otherwise
 */

int
GhostAddress(struct scsi_addr *sa, int drvr, EDT *edtptr, struct scsi_addr *ga, struct ghost **gsp)
{
	char	real_addr[MAXFIELD];
	char	ghost_addr[MAXFIELD];
	char	stamp[sizeof(struct pd_stamp) + 1];
	int	i, new_disk;
	unsigned char c;
#ifdef GDEBUG
	printf("GhostAddress: \n");
#endif
	
	if (edtptr->xedt_pdtype != ID_RANDOM) {
		return 0;	/* not a disk */
	}

	new_disk = 0;
	if (PD_STAMPMATCH(&edtptr->xedt_stamp, &default_stamp)) {
		new_disk = 1;	/* uninitialized disk */
	}

	MakeNodeName(real_addr, NULL, nodeTemplate, sa);
	real_addr[GADDR_LEN] = '\0';

	bcopy((char *) &edtptr->xedt_stamp, stamp, sizeof(struct pd_stamp));
	stamp[sizeof(struct pd_stamp)] = '\0';
	
		
#ifdef GDEBUG
	printf("GhostAddress: ghost_cnt=%d stamp=%s real_addr=%s\n", ghost_cnt, stamp, real_addr);
#endif

	for (i = 0; i < ghost_cnt; i++) {
		if (strcmp(ghost_p[i].stamp, stamp)) {
			continue;
		}

		if (new_disk && strcmp(ghost_p[i].real_addr, real_addr))
			continue;

		*gsp = &ghost_p[i];

#ifdef GDEBUG
		printf("Old device found: count=%d stamp=%s, real_addr=%s, ghost_addr=%s\n", i, ghost_p[i].stamp, ghost_p[i].real_addr, ghost_p[i].ghost_addr);
#endif
		if (!strcmp(ghost_p[i].real_addr, real_addr)) {
			/* The device id and real address match */
			/* Check if there is a ghost address for this device */
			if (ghost_p[i].ghost_addr[0] == NULL) {
				/* no ghost names */
#ifdef GDEBUG
				printf("Using Real Name (%s) for this device\n", real_addr);
#endif
				return 0;
			}
		}
		else {
			/* Check if the device has come back to its 
			   original address */
			if (ghost_p[i].ghost_addr[0] &&
			    !strcmp(ghost_p[i].ghost_addr, real_addr)) {
				/* no ghost names */
				ghost_p[i].ghost_addr[0] = NULL;
				strcpy(ghost_p[i].real_addr, real_addr);
#ifdef GDEBUG
				printf("Using Real Name (%s) for this device\n", real_addr);
#endif
				return 0;

			}
		}

		/*
		 * The device needs a ghost address either because it already
		 * had a ghost address or its real address has changed
		 */

		/* Check if the device already had a ghost address */
		if (ghost_p[i].ghost_addr[0] == NULL) 
			strcpy(ghost_p[i].ghost_addr, ghost_p[i].real_addr);

		(void)sscanf(ghost_p[i].ghost_addr, "c%db%dt%dd%d", &ga->sa_c, &ga->sa_b, &ga->sa_t, &ga->sa_l);

		strcpy(ghost_p[i].real_addr, real_addr);
		
		if (!Silent) {
			MakeNodeName(ghost_addr, NULL, nodeTemplate, ga);
			warning(":501:Using Ghost Name %s for device %s\n", ghost_addr, real_addr);
		}
#ifdef GDEBUG
		MakeNodeName(ghost_addr, NULL, nodeTemplate, ga);
		printf("Using Ghost Name %s for device %s\n", ghost_addr, real_addr);
#endif
		return 1;
	}

#ifdef GDEBUG
	printf("New device (%s) found: ", real_addr);
#endif
	/* This is a new device */
	/* Cehck if its name conflicts with a ghost name in use */
	for (i = 0; i < ghost_cnt; i++) {
		if (ghost_p[i].ghost_addr[0] &&
		    !strcmp(ghost_p[i].ghost_addr, real_addr)) {
			(void) pfmt(stderr, MM_ERROR, ":502:%s: This device cannot be added to the system becuase the SCSI address of this device is used as Ghost Address for %s\n", real_addr, ghost_p[i].real_addr);
			return -1;
		}
	}

	/* Add this device to the ghost name list */
	/* adjust the memory allocation */
	ghost_p = (struct ghost *)realloc(ghost_p, 
				(ghost_cnt+1) * sizeof(struct ghost));
	strcpy(ghost_p[ghost_cnt].stamp, stamp);
	strcpy(ghost_p[ghost_cnt].real_addr, real_addr);
	ghost_p[ghost_cnt].ghost_addr[0] = NULL;
#ifdef GDEBUG
	printf("stamp=%s", stamp);
	printf(" real_addr=%s", ghost_p[ghost_cnt].real_addr);
	printf("\n");
#endif
	*gsp = &ghost_p[ghost_cnt];
	ghost_cnt++;
	return 0;
}

sdighost(int argc, char **argv)
{
	int	lflag = FALSE, rflag = FALSE;
	int	arg, i, count;
	char *label;

	label = (char *) malloc(strlen(Cmdname)+1+3);
	sprintf(label, "UX:%s", Cmdname);
	(void) setlabel(label);

	/* this is sdighost command */	
	while ((arg = getopt(argc,argv,"lr")) != EOF)

		switch (arg) {
		case 'l' : /* list ghost names */
			lflag = TRUE;
			break;
		case 'r' : /* resolve ghost names */
			rflag = TRUE;
			break;
		case '?' : /* Incorrect argument found */
			(void) pfmt(stderr, MM_ACTION, ":503:Usage: %s [-l] [-r]\n", Cmdname);
			exit(ERREXIT);
			/*NOTREACHED*/
			break;
		}

	if (!lflag && !rflag) {
		/* no/invalid arguments */
		(void) pfmt(stderr, MM_ACTION, ":503:Usage: %s [-l] [-r]\n", Cmdname);
		exit(ERREXIT);
	}

	GetGhostNames();

	if (lflag && ghost_cnt) {
		(void) pfmt(stdout, MM_NOSTD, ":506:\nDevice      Stamp         Real Address   Ghost Address\n");
		for (i = 0; i < ghost_cnt; i++) {
			(void) pfmt(stdout, MM_NOSTD, ":507:%s    %12s    %s       %s\n", 
						ghost_p[i].device,
						ghost_p[i].stamp,
						ghost_p[i].real_addr,
						ghost_p[i].ghost_addr);
		}
		(void)fprintf(stdout,"\n");
	}	

	/* count the actual number of ghost names */
	count = 0;
	for (i = 0; i < ghost_cnt; i++)
		if (ghost_p[i].ghost_addr[0])
			count++;
	
	if (count == 0) {
		(void) pfmt(stdout, MM_INFO, ":504:Currently, no names are in use as Ghost Names\n");
	}

	if (!rflag || !count) 
		exit(NORMEXIT);

	(void) pfmt(stdout, MM_INFO, ":505:Currently, following names are in use as ghost names:\n\n");
	(void) pfmt(stdout, MM_NOSTD, ":506:\nDevice           Stamp              Real Address   Ghost Address\n");
	for (i = 0; i < ghost_cnt; i++) {
		if (ghost_p[i].ghost_addr[0] == NULL)
			continue;

		(void) pfmt(stdout, MM_NOSTD, ":507:%s    %s    %s       %s\n", 
					ghost_p[i].device,
					ghost_p[i].stamp,
					ghost_p[i].real_addr,
					ghost_p[i].ghost_addr);
	}

	(void) pfmt(stdout, MM_NOSTD, ":508:\nMake sure that all references to the device(s) listed under\n\"Ghost Address\" are removed from '/etc/vfstab' and any other\napplications which might be using them.\n");
	(void) pfmt(stdout, MM_NOSTD, ":509:\nIf you continue further, the nodes for the device(s) listed under\n\"Ghost Address\" will be removed. If references to those nodes are\nnot removed, the filesystem(s) residing on these device(s) may not\nmount and the applications using these device(s) may fail to run.\n");

	(void) pfmt(stdout, MM_NOSTD, ":510:\nContinue? (y/n) ");
	
	if (yes_response()) {
		for (i = 0; i < ghost_cnt; i++) {
			ghost_p[i].ghost_addr[0] = NULL;
		}
#ifdef GDEBUG
		printf("Device           Stamp               Real Addr   Ghost Addr\n");
		for (i = 0; i < ghost_cnt; i++)
			printf("%s    %s    %s    %s\n",
						ghost_p[i].device,
						ghost_p[i].stamp,
						ghost_p[i].real_addr,
						ghost_p[i].ghost_addr);
#endif /* GDEBUG */

		argc = 2;
		(void) strcpy(Cmdname,SDIMKDEV);
		argv[0] = Cmdname;
		argv[1] = Cmdopt;
		argv[2] = NULL;
		optind = 1;	/* reset the getopt index */
		sdimkcmd(argc, argv);
	}

	exit(NORMEXIT);
}

int 
yes_response()
{
	static char *yesstr = NULL, *nostr = NULL;
	static regex_t yesre, nore;
	char resp[MAX_INPUT];
	int err;

	if (yesstr == NULL) {
		yesstr = nl_langinfo(YESSTR);
		nostr = nl_langinfo(NOSTR);
		err = regcomp(&yesre, nl_langinfo(YESEXPR), REG_EXTENDED|REG_NOSUB);
		if (err != 0) {
			regerror(err, &yesre, resp, MAX_INPUT);
			pfmt(stderr, MM_ERROR, ":493:Regular expression failure: %s\n", resp);
			exit(6);
		}
		err = regcomp(&nore, nl_langinfo(NOEXPR), REG_EXTENDED|REG_NOSUB);
		if (err != 0) {
			regerror(err, &nore, resp, MAX_INPUT);
			pfmt(stderr, MM_ERROR, ":493:Regular expression failure: %s\n", resp);
			exit(6);
		}
	}
	for (;;) {
		fgets(resp, MAX_INPUT, stdin);
		if (regexec(&yesre, resp, (size_t) 0, (regmatch_t *) 0, 0) == 0)
			return(1);
		if (regexec(&nore, resp, (size_t) 0, (regmatch_t *) 0, 0) == 0)
			return(0);
		(void) pfmt(stdout, MM_NOSTD,
			":494:\nInvalid response - please answer with %c or %c.",
			yesstr[0], nostr[0]);
	}
}

