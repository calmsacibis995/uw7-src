#ident	"@(#)pdi.cmds:disksetup.c	1.4.21.1"
#ident	"$Header$"

/* The file disksetup.c contains the architecture independent routines used   */
/* to install a hard disk as the boot disk or an additional disk. The tasks   */
/* it will handle are: retrieving the location of the UNIX partition, surface */
/* analysis, setting up the pdinfo, VTOC and alternates table and writing     */
/* them to the disk, loading the hard disk boot routine, issuing mkfs, labelit*/
/* and mount requests for slices which will be filesystems, and updating the  */
/* the vfstab file appropriately.					      */

#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/fsid.h>
#include <sys/fstyp.h>
#include <malloc.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vtoc.h>
#include <sys/sd01_ioctl.h>
#include <sys/alttbl.h>
#include <sys/altsctr.h>
#include <sys/param.h>
#include "badsec.h"
#include <sys/termios.h>
#include <deflt.h>

#include <errno.h>
#include <locale.h>
#include <pfmt.h>
#include <limits.h>
#include <langinfo.h>
#include <regex.h>

#ifdef SAFE
#undef O_RDWR
#undef O_CREAT
#define O_RDWR O_RDONLY
#define O_CREAT O_RDONLY
#endif /* SAFE */
/*
 * The following structure is used to contain information about partitions
 */
#define TRUE		1
#define FALSE		0
#define RESERVED        34	/* reserved sectors at start of drive */
#define ROOTSLICE	1
#define SWAPSLICE	2
#define USRSLICE	3
#define HOMESLICE	4
#define DOSSLICE	5
#define DUMPSLICE	6
#define BOOTSLICE	7
#define ALTSCTRSLICE	8
#define TALTSLICE	9
#define STANDSLICE	10
#define VARSLICE	11
#define HOME2SLICE	12
#define VPUBLICSLICE	14
#define VPRIVATESLICE	15
#define GOOD_BLK	0
#define SNAMSZ		33
#define BLKSZ		11
#define LINESIZE	512
#define ONEKB		1024L
#define ONEMB		1048576L
#define FOURMB		4194394L
#define MAXBLKSZ	(~0)
#define MACCEILING	"SYS_RANGE_MAX"
#define FS_DIR		"/etc/fs"
#define TOKEN_SEP	"/"
#define MOUNT_CMD	"mount"
#define LABELIT_CMD	"labelit"
#define MKFS_CMD	"mkfs"
#define MKFS_STYL1	1
#define MKFS_STYL2	2
#define MKFS_STYL3	3
#define MKFS_STYL4	4
#define ROOT_INO	2	/* root inode number */

int     diskfd;         	/* file descriptor for raw wini disk */
int     vfstabfd = -1;         	/* file descriptor for /etc/vfstab */
int     childfd;         	/* file descriptor for exec'd children */
int     defaultsfd;         	/* file descriptor for default setup file */
short	defaultsflag = FALSE;	/* Flag to designate valid def. file found */
FILE	*defaultsfile;		/* Flag to designate valid def. file found */
short	defaults_rejected = TRUE; /* Flag to designate if defaults choose */
int	bootfd;			/* boot file descriptor */
int	bootdisk = 0;		/* flag signifying if device is boot disk */
int	installflg = 0;		/* flag signifying installing disk */
int	gaugeflg = FALSE;		/* flag indicates display of verify progress */
extern int	inquiry_mode;	/* flag signifying checking disk  for type */
struct  disk_parms      dp;     /* Disk parameters returned from driver */
struct	vtoc_ioctl	vtoc;	/* struct containing slice info */
struct  pdinfo		pdinfo; /* struct containing disk param info */
char    replybuf[160];           /* used for user replies to questions */
char    *devname;		/* pointer to device name */
char    *bootname;		/* pointer to boot file name */
char    mkfsname[25];		/* pointer to device name to issue mkfs calls */
int     cylsecs;                /* number of sectors per cylinder */
long    cylbytes;               /* number of bytes per cylinder */
daddr_t	savesects = 0;		/* # of sectors to reserve */
daddr_t	unix_base;		/* first sector of UNIX System partition */
daddr_t	unix_size;		/* # sectors in UNIX System partition */
daddr_t pstart;			/* next slice start location */
int	load_boot = FALSE;      /* flag for load boot option */
int	scsi_flag = FALSE;	/* flag indicating a scsi drive */
int	instsysflag = FALSE;    /* indicates 2nd disk of dual disk install*/
struct absio	absio;		/* buf used for RDABS & WRABS ioctls */

/* querylist is used to request slices in the right order for a boot */
/* disk, i.e., stand, dump, swap, root, usr, home, var, tmp, etc.    */
/* the order creates precedence and physical location on the disk    */
int querylist[] = { 0, 10, 6, 2, 1, 3, 4, 5, 9, 11, 12, 13, 14, 15, 0, 0 };


/* sliceinfo has two purposes, first contain setup info for the first disk, */
/* second is to contain info the user chooses for setup of the disk. The */
/* sname field will contain the name of the slice/filesystem. The size field */
/* represents the minimum size slice can be for the system to install. The */
/* createflag designates if the slice is to be created. The field fsslice  */
/* designates the need to issue a mkfs on the slice. */
typedef struct _slice_info {
	char sname[SNAMSZ];	/* slice name */
	int  size;		/* recommended size if created */
	short createflag;	/* Turned on when user specified */
	short fsslice;		/* indicate valid file system slice */
	int fsblksz;		/* primary file system block size in bytes */
	char fstypname[SNAMSZ];	/* file system type name */
	short reqflag;		/* Used to indicate required slice (eg. / ) */
	int  minsz;		/* minimum recommended size */
	int flag;		/* general flag field */
} SliceInfo_t;

SliceInfo_t * sliceinfo;

#define SL_NO_AUTO_MNT		0x1
#define SL_NO_LARGE_FILE	0x2

/* The following structure contains the default file system
 * types and attributes located in the /etc/default directory.
 * The file /etc/default/fstyp contains available file system 
 * types. The the file system specific attributes are located in
 * /etc/defaults/file, where file is the name of the fs type.
 * 
 * Required identifiers in /etc/default/fstyp
 *		FSTYP=comma separated list of fstype names 
 *		BOOT_FSTYP=the boot file system type (default bfs)
 * Required identifiers in /etc/default/file (where file is fstyp name)
 *		BLKSIZE=comma separated list of block sizes
 *		MNTOPT=/etc/vfstab mount options
 *		MKFSTYP=1 of 3 mkfs types supported
 *		LABELIT=YES|NO (indicates whether labelit is called)
 */

#define MAXNAME 10	/* max length for fs type name */
#define MAXFS	10	/* max number of fs types */
#define MAXBLKCNT 5	/* max number of per fs block sizes */
#define MAXOPTCNT 80	/* max length of the mount option string */
#define MAXNAMLST 100 /* max length for the fstype name list */
#define BOOTFSTYPE 0x1	/* indicates a boot file system type */
#define DEF_DIR  "/etc/default/"

char fsnamelist[MAXNAMLST];
int fstyp_cnt;		/* cnt of fs types read from /etc/default/fstyp */
char *DEF_FILE = "fstyp";
	
struct fstyp {
	char fsname[MAXNAME];
	ulong blksize[MAXBLKCNT];
	char mntopt[MAXOPTCNT];
	short mkfstyp;
	short labelit;
	short blksiz_cnt;
	short flag;
	ulong maxsize;
} fstyp[MAXFS] = {0};

ullong_t totalmemsize = 0L;

int verify_flg = 	TRUE;	/* -V option default */
char options_string[] = "enVSsiIBb:x:d:m:gw";

extern	struct	badsec_lst *badsl_chain;
extern	int	badsl_chain_cnt;
extern  int	Show;
extern  int	Silent;
extern int	*alts_fd;
extern struct	alts_mempart *ap;	/* pointer to incore alts tables*/
extern char	Devfile[];
int    execfd = -1;
char   *execfile = NULL;
void writevtoc();
void offer_defaults();
extern int assign_dos();
extern void rd_fs_defaults();	/* read fs type defaults from /etc/defaults */
extern int find_fs_defaults();	/* find the fs default index */
extern int get_fs_blksize();	/* get fs specific available block sizes */
int xflg = 0;
extern int Debug = FALSE;

int	extendedflag = 0;		/* greater than 16 slices? */
int	max_slices = V_NUMPAR;		/* assume 16 slices for now */

int	wipeflag = 0;
int	wipeout(void);

void
main(argc,argv)
int argc;
char *argv[];
{
	register int	i, j;
	char	*p;
	extern char	*optarg;
	extern int	optind;
	int	c, errflg = 0;
	struct stat statbuf;
	int openflags=O_APPEND|O_CREAT|O_RDWR;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxdisksetup");
	(void) setlabel("UX:disksetup");

	while ((c = getopt(argc, argv, options_string)) != -1) {
		switch (c) {
		case 'S':
			Show = TRUE;
			break;
		case 's':
			Silent = TRUE;
			break;
		case 'b': 
			if ((bootfd = open(optarg, O_RDONLY)) == -1) {
				(void) pfmt(stderr, MM_ERROR,
				  ":27:Unable to open specified boot routine.\n");
				exit(40);
			}
			bootname = optarg;
			load_boot = TRUE;  
			break;
		case 'B':
			bootdisk = TRUE;  
			openflags|=O_TRUNC;
			break;
		case 'g':
			gaugeflg = TRUE;
			break;
		case 'n':
			verify_flg = FALSE;
			break;
		case 'i':
			Silent = TRUE;
			inquiry_mode = TRUE;
			installflg = TRUE;
			break;
		case 'I':
			installflg = TRUE;  
			break;
		case 'd' :
			if (((defaultsfd = open(optarg, O_RDONLY)) == -1) ||
			   ((defaultsfile = fdopen(defaultsfd, "r"))  == NULL))
				(void) pfmt(stderr, MM_ERROR,
					":28:Unable to open defaults file.\n");
			else
				defaultsflag = TRUE;
			break;
		case 'V':
			verify_flg = TRUE;  
			break;
		case 'x' :
			xflg++;
			execfile = optarg;
			break;
			
		case 'm' :
			totalmemsize = strtoull(optarg, (char **) NULL, 10);
			if(totalmemsize  <= 0L)
				totalmemsize = FOURMB;
			break;
		case 'e' :
			extendedflag = TRUE;
			max_slices = V_NUMSLICES;
			break;
		case 'w' :
			wipeflag = TRUE;
			break;
		case '?':
			++errflg;
		}
	}

	if (argc - optind < 1)
		++errflg;
	if (errflg) {
		giveusage();
		exit(40);
	}
	if(xflg){
		execfd = open(execfile, openflags, 0666);
	}
	devname = argv[optind];
	strcpy( Devfile, devname );
	strncpy(mkfsname, devname, (strlen(devname) - 1)); 
	if (stat(devname, &statbuf)) {
		(void) pfmt(stderr, MM_ERROR,
			":29:stat of %s failed\n", devname);
		(void) pfmt(stderr, MM_ERROR|MM_NOGET,
			"%s\n", strerror(errno));
		giveusage();
		exit(1);
	}
	if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
		(void) pfmt(stderr, MM_ERROR,
			":30:device %s is not character special\n", devname);
		giveusage();
		exit(1);
	}
	if ((diskfd=open(devname,O_RDWR)) == -1) {
		(void) pfmt(stderr, MM_ERROR,
			":31:Disksetup unable to open %s\n", devname);
		exit(50);
	}
	alts_fd = &diskfd;
	if (ioctl(diskfd,V_GETPARMS,&dp) == -1) {
		(void) pfmt(stderr, MM_ERROR,
			":32:V_GETPARMS failed on %s", devname);
		exit(51);
	}

	if (wipeflag) {
		if (wipeout()) {
			(void)pfmt(
				stderr,
				MM_ERROR,
				":499:Could not wipe out first 32 sectors of device %s\n",
				devname
			);
			exit(1);
		}
		exit(0);
	}

        if ((dp.dp_type == DPT_SCSI_HD) || (dp.dp_type == DPT_SCSI_OD))
		scsi_flag = TRUE;
	else if (inquiry_mode)
		exit(1);

	sliceinfo = calloc(max_slices, sizeof(SliceInfo_t));

	get_unix_partition(); /*retrieve part. table from disk */

	if (load_boot)
		loadboot();  /* writes boot to track 0 of Unix part. */

	if (installflg) {  /* installing boot disk or additional disks */
		init_structs(); /* initialize pdinfo and vtoc */
		rd_fs_defaults();
		badsl_chain_cnt = 0;	/* assume no media defects found */
		if (verify_flg) {
			do_surface_analysis();	/* search for media defects */
		}
		alloc_altsctr_part();	/* allocate alternate sector partition */

		/* make sure pstart is track aligned */
		if (pstart % (daddr_t)dp.dp_sectors) 
			pstart = (pstart / (daddr_t)dp.dp_sectors + 1) 
				 * dp.dp_sectors;
		if (defaultsflag == TRUE)
			offer_defaults();
		if (defaults_rejected == TRUE)
			setup_vtoc(); /* query user for slices and sizes      */
		if (updatebadsec())
			wr_altsctr();
		(void)assign_dos(); /* assign dos partitions to vtoc */
		writevtoc();  /*writes pdinfo, vtoc and alternates table      */

		if (extendedflag) {
			(void)pfmt(
				stderr,
				MM_NOSTD,
				":495:\nCreating device nodes, please wait.\n"
			);
			exec_command("/etc/scsi/pdimkdev -u -s");
		}
		create_fs();  /* Issues mkfs calls, mounts and updates vfstab */

		/*
		 * only put these commands in the output stream if we
		 * are doing an install.
		 */
		if(execfd >= 0) {
			exec_command("cp /tmp/vfstab /mnt/etc/vfstab");
			exec_command("chmod 644 /tmp/vfstab /mnt/etc/vfstab");
			exec_command("[ -d /mnt/.io ] || mkdir /mnt/.io");
			close(execfd);
		}
	}
	free(sliceinfo);
	exit(0);
}

giveusage()
{
	(void) pfmt(stderr, MM_ACTION,
		":496:Usage: disksetup -BI[se] -b bootfile [-d configfile] raw-device (install boot disk)\n");
	(void) pfmt(stderr, MM_ACTION,
		":497:       disksetup -I[e] [-d configfile] raw-device (install additional disk(s))\n");
	(void) pfmt(stderr, MM_ACTION,
		":35:       disksetup -b bootfile raw-device (write boot code to disk)\n");
}

fs_error(fsname)
char *fsname;
{
	(void) pfmt(stderr, MM_ERROR,
		":36:Cannot create/mount the %s filesystem.", fsname);
	(void) pfmt(stderr, MM_ERROR,
		":37:Please contact\nyour service representative for further assistance.\n");
	free(sliceinfo);
	exit(1);
}

/* do_surface_analysis verifies all sectors in the Unix partition. It looks */
/* for bad tracks (3 or more bad sectors in the track) and bad sectors. All */
/* defects are then kept in the appropriate table (ie bad tracks in the bad */
/* track table). Alternates are then reserved for the found defects and for */
/* future defects. Number of alt sectors to be reserved should be the number*/
/* of bad sectors found + 1 sector/MB of space in UNIX partion (minimum 32) */
do_surface_analysis()
{
	extern void scsi_setup();
	extern int do_format;
	extern int ign_check;
	extern int no_format;
	extern int verify;
	extern int do_unix;

	if ((scsi_flag) && (!Silent)) {
		(void) pfmt(stdout, MM_NOSTD,
			":38:Surface analysis of your disk is recommended\nbut not required.\n\n");
		(void) pfmt(stdout, MM_NOSTD,
			":39:Do you wish to skip surface analysis? (y/n) ");
		if (yes_response()) 
			return(1);
	}
	if (!Silent)
	{
		(void) pfmt(stdout, MM_NOSTD,
			":40:\nChecking for bad sectors in the UNIX System partition...\n\n");
	}
	do_format = TRUE;
	ign_check = TRUE;
	no_format = TRUE;
	verify = TRUE;
	do_unix = TRUE;
	scsi_setup(devname);
	return(1);

}

/*
 * Writevtoc ()
 * Write out the updated volume label, pdinfo, vtoc, and alternate table.  We
 * assume that the pdinfo and vtoc, together, will fit into a single BSIZE'ed
 * block.  (This is currently true on even 512 byte/block systems;  this code
 * may need fixing if a data structure grows).
 * We are careful to read the block that the volume label resides in, and
 * overwrite the label at its offset;  writeboot() should have taken care of
 * leaving this hole.
 */
void
writevtoc()
{
  	int	i;

	for (i = 1; i < vtoc.v_nslices; i++) {
		if (vtoc.v_slices[i].p_size == 0) {
                                vtoc.v_slices[i].p_tag = 0;
                                vtoc.v_slices[i].p_flag = 0;
                                vtoc.v_slices[i].p_start = 0;
		}
	}

	if (ioctl(diskfd, SD_NEWSTAMP, &pdinfo.serial[0]) == -1) {
		(void)pfmt(
			stderr,
			MM_ERROR,
			":42:Error writing pdinfo and VTOC.\n",
			strerror(errno)
		);
		close(diskfd);
		free(sliceinfo);
		exit(51);
	}

	if (ioctl(diskfd, V_WRITE_PDINFO, &pdinfo) == -1) {
		(void)pfmt(
			stderr,
			MM_ERROR,
			":42:Error writing pdinfo and VTOC.\n",
			strerror(errno)
		);
		close(diskfd);
		free(sliceinfo);
		exit(51);
	}

	if (ioctl(diskfd, V_WRITE_VTOC, &vtoc) == -1) {
		(void)pfmt(
			stderr,
			MM_ERROR,
			":42:Error writing pdinfo and VTOC.\n",
			strerror(errno)
		);
		close(diskfd);
		free(sliceinfo);
		exit(51);
	}

	sync();
	ioctl(diskfd, V_REMOUNT, NULL);
	close(diskfd);
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

fill_vtoc()
{
	int	i, j;

	for (i = 1; i < vtoc.v_nslices; i++) {
		if (bootdisk && (i < V_NUMPAR)) {
			j = querylist[i];
		} else {
			j = i;
		}
		if (sliceinfo[j].size > 0) {
			vtoc.v_slices[j].p_start = pstart;
			vtoc.v_slices[j].p_size = sliceinfo[j].size;
			pstart += sliceinfo[j].size;
			vtoc.v_slices[j].p_flag = V_VALID;
			if (sliceinfo[j].fsslice == FALSE)
				vtoc.v_slices[j].p_flag |= V_UNMNT;
			switch (j) {
			case ROOTSLICE  : if (bootdisk == TRUE)
						vtoc.v_slices[j].p_tag = V_ROOT;
					  else
						vtoc.v_slices[j].p_tag = V_USR;
				  	  break;
			case SWAPSLICE  : if (strcmp(sliceinfo[j].sname,"/dev/swap")== 0)
						vtoc.v_slices[j].p_tag = V_SWAP;
					  else
						vtoc.v_slices[j].p_tag = V_USR;
				  	  break;
			case STANDSLICE : if (strcmp(sliceinfo[j].sname,"/stand") == 0) 
						vtoc.v_slices[j].p_tag = V_STAND;
					  else
						vtoc.v_slices[j].p_tag = V_USR;
				  	  break;
			case VARSLICE : if (strcmp(sliceinfo[j].sname,"/var") == 0) 
						vtoc.v_slices[j].p_tag = V_VAR;
					  else
						vtoc.v_slices[j].p_tag = V_USR;
				  	  break;
			case HOMESLICE  : 
			case HOME2SLICE : if (strncmp(sliceinfo[j].sname,"/home",5) == 0)  
					  	vtoc.v_slices[j].p_tag = V_HOME;
					  else
						vtoc.v_slices[j].p_tag = V_USR;
				  	  break;
			case USRSLICE   : vtoc.v_slices[j].p_tag = V_USR;
				  	  break;
			case DUMPSLICE  : if (strcmp(sliceinfo[j].sname,"/dev/dump") == 0)
					 	vtoc.v_slices[j].p_tag = V_DUMP;
					  else
						vtoc.v_slices[j].p_tag = V_USR;
				  	  break;
			case VPUBLICSLICE : if (strcmp(sliceinfo[j].sname,"/dev/volpublic") == 0)
					 	vtoc.v_slices[j].p_tag = V_MANAGED_1;
					  else
						vtoc.v_slices[j].p_tag = V_USR;
				  	  break;
			case VPRIVATESLICE : if (strcmp(sliceinfo[j].sname,"/dev/volprivate") == 0)
					 	vtoc.v_slices[j].p_tag = V_MANAGED_2;
					  else
						vtoc.v_slices[j].p_tag = V_USR;
				  	  break;
			default	 	: vtoc.v_slices[j].p_tag = V_USR;
			}
		}
	}
}

daddr_t
check_swapsz(numsects, memsize)
daddr_t numsects;
ullong_t memsize;
{
	daddr_t newsects;
	if (memsize > (24*ONEMB)/(daddr_t)dp.dp_secsiz)
		newsects = ((numsects/cylsecs) / 2) * cylsecs;
	else
		if (memsize > (12*ONEMB)/(daddr_t)dp.dp_secsiz)
			newsects = ((numsects/cylsecs) * 0.75) * cylsecs;
		else
			newsects = numsects;
	return newsects;
}

/* offer_defaults will read in defaults file, display the defaults, query the */
/* user if they want the defaults. If the user chooses the defaults, the vtoc */
/* and sliceinfo will be setup accordingly.				      */
void
offer_defaults()
{
	int i, n, slicenum, wflag = FALSE;
	int cnt, totalpcnt = 0;
	int prtflag = TRUE;
	daddr_t init_pstart, dfltsz, minsz;
	daddr_t oneMBsects = ONEMB/(daddr_t)dp.dp_secsiz; /* 1MB in sectors */
	daddr_t numsects, availsects, availcyls, reqcyls, totalcyls;
	FILE *pipe;
	short reqslice_err = FALSE;
	char slicename[SNAMSZ], fstyp[SNAMSZ], blksz[BLKSZ], sizetype, reqflg, line[LINESIZE];
	ullong_t memsize;

	if(totalmemsize <= 0L){
		if (((pipe = popen("memsize", "r")) == NULL) ||
		   	(fscanf(pipe, "%llu",&memsize) != 1)) {
			(void) pfmt(stderr, MM_WARNING,
				":44:Cannot retrieve size of memory, 4MB will be assumed\n");
			memsize = FOURMB;
		}
		if (pipe != NULL)
			(void)pclose(pipe);
		else
			memsize = FOURMB;
	} else memsize = totalmemsize;

	if (memsize % ONEMB != 0)
		memsize = ((memsize / ONEMB) + 1) * ONEMB;
	memsize /= (daddr_t)dp.dp_secsiz; /* convert memsize to sectors */
	availsects = (unix_base + unix_size) - pstart;
 
	for (i=1; (fgets(&line[0],LINESIZE,defaultsfile) != NULL); i++) {
		if (line[0] == '-') {
			savesects = atoi(line + 1) * oneMBsects;
			availsects -= savesects;
			continue;
		}
		n = sscanf(&line[0],"%d %32s %32s %10s %d%c %d%c", &slicenum,
		    slicename, fstyp, blksz, &dfltsz, &sizetype, &minsz, &reqflg); 
		if ((n < 7) ||
		   ((slicenum < 1) || (slicenum >= max_slices) || (slicenum == BOOTSLICE)) || (slicenum == ALTSCTRSLICE)) {
			(void) pfmt(stderr, MM_ERROR,
				":45:defaults file line %d is invalid and will be skipped.\n",i);
			continue;
		}
		if (slicenum >= vtoc.v_nslices) {
			vtoc.v_nslices = slicenum + 1;
		}
		strcpy(sliceinfo[slicenum].sname, slicename);
		sliceinfo[slicenum].createflag = TRUE;
		sliceinfo[slicenum].size = 0;
		if (sizetype == 'K')
			minsz = (minsz * ONEKB) / (daddr_t)dp.dp_secsiz;
		else
			minsz *= oneMBsects;
		if ((minsz % cylsecs) != 0)
			minsz = (minsz/cylsecs + 1) * cylsecs;
		sliceinfo[slicenum].minsz = minsz;
		if (reqflg == 'R')
			sliceinfo[slicenum].reqflag = TRUE;
		else
			sliceinfo[slicenum].reqflag = FALSE;

		if (strcmp(fstyp, "-") != 0) {
			int x;
			x=atoi(blksz);
			if ((x % 512) != 0) {
				(void) pfmt(stderr, MM_ERROR,
					":46:%s entry has a bad block size '%d', for %s file system, must be a multiple of 512\n",
						slicename, x, fstyp);
				(void) pfmt(stderr, MM_ERROR,
					":47:Entry ignored\n");
				strncpy(sliceinfo[slicenum].sname,"\0",SNAMSZ);
				sliceinfo[slicenum].createflag = FALSE;
				sliceinfo[slicenum].reqflag = FALSE;
				continue;
			}
			strcpy(sliceinfo[slicenum].fstypname, fstyp);
			sliceinfo[slicenum].fsblksz = x;
		    	sliceinfo[slicenum].fsslice = TRUE;
		}
		switch (sizetype) {
		/* set size as neg. to flag for calc. after M and m entries */
		case 'W': 
			  sliceinfo[slicenum].size = -(dfltsz);
			  totalpcnt += dfltsz;
			  wflag = TRUE;
			  break;
		case 'm':
			  numsects = dfltsz * memsize;
			  if ((numsects % cylsecs) != 0)
				numsects = (numsects/cylsecs + 1) * cylsecs;
			  if ((strcmp(slicename, "/dev/swap") == 0) && (dfltsz == 2))
				numsects = check_swapsz(numsects, memsize);
			  if (numsects < minsz)
				numsects = minsz;

			  if (numsects <= availsects)
				sliceinfo[slicenum].size = numsects;
			  else if (availsects < minsz && (!Silent))
				sliceinfo[slicenum].size = 0;
			  else	sliceinfo[slicenum].size = availsects;

			  availsects -= sliceinfo[slicenum].size;
			  break;
		case 'M': 
			  numsects = dfltsz * oneMBsects;
			  if ((numsects % cylsecs) != 0)
				numsects = (numsects/cylsecs + 1) * cylsecs;
			  if (numsects < minsz)
				numsects = minsz;

			  if (numsects <= availsects)
				sliceinfo[slicenum].size = numsects;
			  else if (availsects < minsz && (!Silent))
				sliceinfo[slicenum].size = 0;
			  else	sliceinfo[slicenum].size = availsects;

			  availsects -= sliceinfo[slicenum].size;
			  break;
		case 'K': 
/*	The next line has been changed for PTF3049 MR ul96-16907	*/

			  numsects = dfltsz * (ONEKB / (daddr_t)dp.dp_secsiz);
			  if ((numsects % cylsecs) != 0)
				numsects = (numsects/cylsecs + 1) * cylsecs;
			  if (numsects < minsz)
				numsects = minsz;

			  if (numsects <= availsects)
				sliceinfo[slicenum].size = numsects;
			  else if (availsects < minsz && (!Silent))
				sliceinfo[slicenum].size = 0;
			  else	sliceinfo[slicenum].size = availsects;

			  availsects -= sliceinfo[slicenum].size;
			  break;
		default:
			  (void) pfmt(stderr, MM_ERROR,
				":394:%s entry has an invalid size specifier character '%c'\n",
						slicename, sizetype);
			  (void) pfmt(stderr, MM_ERROR,
				":47:Entry ignored\n");
			  strncpy(sliceinfo[slicenum].sname,"\0",SNAMSZ);
			  sliceinfo[slicenum].createflag = FALSE;
			  sliceinfo[slicenum].reqflag = FALSE;
			  sliceinfo[slicenum].minsz = 0;
			  strncpy(sliceinfo[slicenum].fstypname,"\0",SNAMSZ);
			  sliceinfo[slicenum].fsblksz = 0;
			  sliceinfo[slicenum].fsslice = FALSE;
			  continue;
		}
	}
	if (wflag == TRUE) {
		if (availsects > 0) {
			availcyls = availsects / cylsecs;
			totalcyls = availcyls;
			for (i=1; i < vtoc.v_nslices; i++)
				if (sliceinfo[i].size < 0) {
					n = -(sliceinfo[i].size)*100/totalpcnt;
					reqcyls = (n * totalcyls) / 100;
					if ((reqcyls <= availcyls) &&
					   (reqcyls * cylsecs > sliceinfo[i].minsz))
						sliceinfo[i].size = reqcyls * cylsecs;
					else
						if (availcyls * cylsecs >= sliceinfo[i].minsz)
							sliceinfo[i].size =(sliceinfo[i].minsz/cylsecs + 1) * cylsecs;
						else
							sliceinfo[i].size = 0;
					availcyls -= sliceinfo[i].size/cylsecs;
					availsects -= sliceinfo[i].size;
				}
		}
		else /* W requests made but no sects left, set W slices to 0 */
			for (i=1; i < vtoc.v_nslices; i++)
				if (sliceinfo[i].size < 0) 
					sliceinfo[i].size = 0;
	}
	fclose(defaultsfile);
	close(defaultsfd);
	if (Silent) {
		init_pstart = pstart;
		fill_vtoc();
		defaults_rejected = FALSE;
		return;
	}
	(void) pfmt(stdout, MM_NOSTD,
		":48:The following slice sizes are the recommended configuration for your disk.\n");
	for (i=1; i < vtoc.v_nslices; i++)
		if (sliceinfo[i].createflag == TRUE && 
			sliceinfo[i].size > 0)
			if (sliceinfo[i].fsslice == TRUE)
				(void) pfmt(stdout, MM_NOSTD,
				  ":49:A %s filesystem of %ld cylinders (%.1f MB)\n",
				  sliceinfo[i].sname, sliceinfo[i].size/cylsecs,
				  (float)sliceinfo[i].size*(float)dp.dp_secsiz/ONEMB);
			else
				(void) pfmt(stdout, MM_NOSTD,
				  ":50:A %s slice of %ld cylinders (%.1f MB)\n",
				  sliceinfo[i].sname, sliceinfo[i].size/cylsecs,
				  (float)sliceinfo[i].size*(float)dp.dp_secsiz/ONEMB);
	for (i=1; i < vtoc.v_nslices; i++) 
		if ((sliceinfo[i].createflag == TRUE) && 
		   (sliceinfo[i].size == 0)) {
			if (prtflag == TRUE) {
				(void) pfmt(stdout, MM_NOSTD,
				":51:\nBased on the default size recommendations, disk space was not available\nfor the following slices:\n");
				prtflag = FALSE;
			}
			if (sliceinfo[i].fsslice == TRUE) 
				if (sliceinfo[i].reqflag == TRUE) {
					reqslice_err = TRUE;
					(void) pfmt(stdout, MM_NOSTD,
						":52:The Required %s filesystem was not allocated space.\n", sliceinfo[i].sname);
					(void) pfmt(stdout, MM_NOSTD,
						":53:This slice is required for successful installation.\n\n"); 
				}
				else
					(void) pfmt(stdout, MM_NOSTD,
						":54:The %s filesystem.\n", sliceinfo[i].sname);
			else
				if (sliceinfo[i].reqflag == TRUE) {
					reqslice_err = TRUE;
					(void) pfmt(stdout, MM_NOSTD,
						":55:The required %s slice was not allocated space.\n", sliceinfo[i].sname);
					(void) pfmt(stdout, MM_NOSTD,
						":53:This slice is required for successful installation.\n\n"); 
				}
				else
					(void) pfmt(stdout, MM_NOSTD,
						":56:The %s slice.\n", sliceinfo[i].sname);
		}
	init_pstart = pstart;
	fill_vtoc();
	if (reqslice_err == TRUE) {
		/* flush input prior to prompt -- prevent typeahead */
		/* note that we treat tcflush as void -- may not
		 * succeed because input is from file
		 */
		(void) tcflush(0,TCIFLUSH);
		(void) pfmt(stdout, MM_NOSTD,
			":57:\nThe default layout will not allow all required slices to be created.\n");
		(void) pfmt(stdout, MM_NOSTD,
			":58:You will be required to designate the sizes of slices to create a\nvalid layout for the slices you requested.\n\n");
	}
	else {
		(void) pfmt(stdout, MM_NOSTD,
			":59:\nIs this configuration acceptable? (y/n) ");
		if (yes_response()) 
			defaults_rejected = FALSE;
	}
	if ((reqslice_err == TRUE) || (defaults_rejected == TRUE)) {
		pstart = init_pstart;
		for (i=1; i < vtoc.v_nslices; i++) 
			if (sliceinfo[i].createflag && vtoc.v_slices[i].p_size) {
				vtoc.v_slices[i].p_size = 0;
				vtoc.v_slices[i].p_start = 0;
				vtoc.v_slices[i].p_flag = 0;
			}
	}
}

/* setup_vtoc will make the calls to first obtain the slice configuration   */
/* info and then obtain the sizes for the slices the user choose.           */
setup_vtoc()
{
	daddr_t init_pstart = pstart;
	int i, define_slices = TRUE;
	short reqslice_err;

	if (defaultsflag == TRUE)
		define_slices = FALSE;
	for (;;) {
		reqslice_err = FALSE;
		if (define_slices == TRUE) {
			if (bootdisk) {
				get_bootdsk_slices();
				if (extendedflag) {
					/* now populate any remaining slices */
					get_slices();
				}
			} else {
				get_slices();	
			}
		} else {
			free(sliceinfo);
			exit(0);
		}
		get_slice_sizes();
		(void) pfmt(stdout, MM_NOSTD,
			":60:\nYou have specified the following disk configuration:\n");
		for (i=1; i < vtoc.v_nslices; i++) {
			if (sliceinfo[i].createflag )
				if (sliceinfo[i].fsslice == TRUE)
					(void) pfmt(stdout, MM_NOSTD,
						":61:A %s filesystem with %d cylinders (%.1f MB)\n",
					  sliceinfo[i].sname,vtoc.v_slices[i].p_size/cylsecs,
					  (float)vtoc.v_slices[i].p_size*(float)dp.dp_secsiz/ONEMB);
				else
					(void) pfmt(stdout, MM_NOSTD,
					  ":62:A %s slice with %d cylinders (%.1f MB)\n",
					  sliceinfo[i].sname,vtoc.v_slices[i].p_size/cylsecs,
					  (float)vtoc.v_slices[i].p_size*(float)dp.dp_secsiz/ONEMB);
			if ((sliceinfo[i].reqflag == TRUE) &&
			   (vtoc.v_slices[i].p_size == 0)) {
				(void) pfmt(stdout, MM_NOSTD,
					":63:Required slice %s was not allocated space.\n", sliceinfo[i].sname);
				reqslice_err = TRUE;
			}
		}
		if (reqslice_err == TRUE) {
			(void) pfmt(stdout, MM_NOSTD,
				":64:A required slice was not allocated space.");
			(void) pfmt(stdout, MM_NOSTD,
				":65:You must reallocate the disk space\nsuch that all required slices are created.\n\n");
		}
		else {
			/* flush input prior to prompt -- prevent typeahead */
			/* note that we treat tcflush as void -- may not
		 	 * succeed because input is from file
			 */
			(void) tcflush(0,TCIFLUSH);
			(void) pfmt(stdout, MM_NOSTD,
				":66:\nIs this allocation acceptable to you (y/n)? ");
			if (yes_response())
				break;
		}
		if (defaultsflag == FALSE) {
			/* flush input prior to prompt -- prevent typeahead */
			/* note that we treat tcflush as void -- may not
		 	 * succeed because input is from file
		 	 */
			(void) tcflush(0,TCIFLUSH);
			(void) pfmt(stdout, MM_NOSTD,
				":67:\nYou have rejected the disk configuration.  ");
			(void) pfmt(stdout, MM_NOSTD,
				":68:Do you want\nto redefine the slices to be created? (y/n)? ");
			if (yes_response())
				define_slices = TRUE;
			else
				define_slices = FALSE;
		}
		pstart = init_pstart;
		for (i=1; i < vtoc.v_nslices; i++) {
			if (sliceinfo[i].createflag && vtoc.v_slices[i].p_size) {
				vtoc.v_slices[i].p_size = 0;
				vtoc.v_slices[i].p_start = 0;
				vtoc.v_slices[i].p_flag = 0;
				if (define_slices == TRUE) {
					sliceinfo[i].createflag = 0;
					vtoc.v_slices[i].p_tag = 0;
				}
			}
		}
		/* reset to the default 16 */
		vtoc.v_nslices = V_NUMPAR;
	}
}

int
get_fs_type(req_flag, slice_indx)
int req_flag; /* does the slice require a mkfs type */
int slice_indx;
{
int fs_indx;
char tmp[MAXNAMLST];
char *p;
int s;
	if (req_flag == TRUE) 
		for (;;) {
			strcpy(tmp, fsnamelist);
			p = strtok(tmp, ",");
			s = strlen(p);
			/* flush input prior to prompt -- prevent typeahead */
			/* note that we treat tcflush as void -- may not
		 	 * succeed because input is from file
		 	 */
			(void) tcflush(0,TCIFLUSH);
			(void) pfmt(stdout, MM_NOSTD,
				":69:\nEnter the filesystem type for this slice\n(%s), or press <ENTER> to use the default (%s): ", (fsnamelist+s+1), p);
			gets(replybuf);

			/* use the default fs type */
			if (strcmp(replybuf, "") == 0)
				strcpy(replybuf, p);

			if ((fs_indx=find_fs_defaults(replybuf, 0)) != -1) {
				strcpy(sliceinfo[slice_indx].fstypname, replybuf);
				sliceinfo[slice_indx].fsblksz = get_fs_blksize(fs_indx);
				return(TRUE);
			}

			(void) pfmt(stdout, MM_NOSTD,
				":70:Invalid response - please answer with %s\n",
				fsnamelist);
		}
	else
		for (;;) {
			strcpy(tmp, fsnamelist);
			p = strtok(tmp, ",");
			/* flush input prior to prompt -- prevent typeahead */
			/* note that we treat tcflush as void -- may not
			 * succeed because input is from file
			 */
			(void) tcflush(0,TCIFLUSH);
			(void) pfmt(stdout, MM_NOSTD,
				":71:\nEnter the filesystem type for this slice (%s),\ntype 'na' if no filesystem is needed, or press\n<ENTER> to use the default (%s): ",
				fsnamelist, p);
			gets(replybuf);

			/* use the default fs type */
			if (strcmp(replybuf, "") == 0)
				strcpy(replybuf, p);

			if ((strncmp(replybuf,"na",2) == 0) ||
		    	    (strncmp(replybuf,"NA",2) == 0))
				return(0);

			if ((fs_indx=find_fs_defaults(replybuf, 0)) != -1) {

				strcpy(sliceinfo[slice_indx].fstypname, replybuf);
				sliceinfo[slice_indx].fsblksz = get_fs_blksize(fs_indx);
				return(TRUE);
			}

			(void) pfmt(stdout, MM_NOSTD,
				":72:\nInvalid response - please answer with (%s or na.\n\n", fsnamelist);
		}
}

/* get_bootdsk_slices queries the user on their preferences for the setup  */
/* of a boot disk. This allows for setup of root, swap, usr, usr2, dump,   */
/* stand, home, volpublic, and volprivate slices.					   */
get_bootdsk_slices()
{
	int fs_indx;

	(void) pfmt(stdout, MM_NOSTD,
		":73:You will now be queried on the setup of your disk.");
	(void) pfmt(stdout, MM_NOSTD,
		":74:After you\nhave determined which slices will be created, you will be \nqueried to designate the sizes of the various slices.\n\n");
	sliceinfo[ROOTSLICE].createflag = TRUE;
	vtoc.v_slices[ROOTSLICE].p_tag = V_ROOT;
	sprintf(sliceinfo[ROOTSLICE].sname,"/");
	(void) pfmt(stdout, MM_NOSTD,
		":75:A root filesystem is required and will be created.\n");
	sliceinfo[ROOTSLICE].fsslice = get_fs_type(TRUE, ROOTSLICE);
	sliceinfo[SWAPSLICE].createflag = TRUE;
	vtoc.v_slices[SWAPSLICE].p_tag = V_SWAP;
	sprintf(sliceinfo[SWAPSLICE].sname,"/dev/swap");
	sliceinfo[STANDSLICE].createflag = TRUE;
	sprintf(sliceinfo[STANDSLICE].sname,"/stand");
	sliceinfo[STANDSLICE].fsslice = TRUE;
	/* find boot file system type */
	if (fs_indx=find_fs_defaults("bfs", 0) == -1) {
		if ((fs_indx=find_fs_defaults("", BOOTFSTYPE)) != -1) {
			strcpy(sliceinfo[STANDSLICE].fstypname, fstyp[fs_indx].fsname);
			sliceinfo[STANDSLICE].fsblksz = 
				fstyp[fs_indx].blksize[0];
		} else {
			(void) pfmt(stderr, MM_ERROR,
				":76:no valid boot file system type found, default is bfs\n");
			(void) pfmt(stderr, MM_ERROR,
				":77:check /etc/default/bfs\n");
			free(sliceinfo);
			exit(-1);
		}
	}
	else {
		sprintf(sliceinfo[STANDSLICE].fstypname, "bfs");
		sliceinfo[STANDSLICE].fsblksz = 512;
	}

	vtoc.v_slices[STANDSLICE].p_tag = V_STAND;
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	(void) pfmt(stdout, MM_NOSTD,
	 ":78:\nDo you wish to have separate root and usr filesystems (y/n)? ");
	if (yes_response()) {
		sliceinfo[USRSLICE].createflag = TRUE;
		vtoc.v_slices[USRSLICE].p_tag = V_USR;
		sliceinfo[USRSLICE].fsslice = get_fs_type(TRUE, USRSLICE);
		sprintf(sliceinfo[USRSLICE].sname,"/usr");
	}
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	(void) pfmt(stdout, MM_NOSTD,
	  ":79:\nDo you want to allocate a crash/dump area on your disk (y/n)? ");
	if (yes_response()) {
		sliceinfo[DUMPSLICE].createflag = TRUE;
		vtoc.v_slices[DUMPSLICE].p_tag = V_DUMP;
		sprintf(sliceinfo[DUMPSLICE].sname,"/dev/dump");
	}
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	(void) pfmt(stdout, MM_NOSTD,
		":80:\nDo you want to create a home filesystem (y/n)? "); 
	if (yes_response()) { 
		sliceinfo[HOMESLICE].createflag = TRUE; 
		vtoc.v_slices[HOMESLICE].p_tag = V_HOME; 
		sliceinfo[HOMESLICE].fsslice = get_fs_type(TRUE, HOMESLICE);
		sprintf(sliceinfo[HOMESLICE].sname,"/home");
	} 
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	(void) pfmt(stdout, MM_NOSTD,
		":81:\nDo you want to create a var filesystem (y/n)? "); 
	if (yes_response()) {
		sliceinfo[VARSLICE].createflag = TRUE;
		vtoc.v_slices[VARSLICE].p_tag = V_VAR;
		sliceinfo[VARSLICE].fsslice = get_fs_type(TRUE, VARSLICE);
		sprintf(sliceinfo[VARSLICE].sname,"/var");
	}
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	(void) pfmt(stdout, MM_NOSTD,
		":82:\nDo you want to create a home2 filesystem (y/n)? "); 
	if (yes_response()) { 
		sliceinfo[HOME2SLICE].createflag = TRUE; 
		vtoc.v_slices[HOME2SLICE].p_tag = V_HOME; 
		sliceinfo[HOME2SLICE].fsslice = get_fs_type(TRUE, HOME2SLICE);
		sprintf(sliceinfo[HOME2SLICE].sname,"/home2");
	} 
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	(void) pfmt(stdout, MM_NOSTD,
	    ":385:\nDo you want to create a volume management public area (y/n)? ");
	if (yes_response()) {
		sliceinfo[VPUBLICSLICE].createflag = TRUE;
		vtoc.v_slices[VPUBLICSLICE].p_tag = V_MANAGED_1;
		sprintf(sliceinfo[VPUBLICSLICE].sname,"/dev/volpublic");
	}
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	(void) pfmt(stdout, MM_NOSTD,
	    ":384:\nDo you want to create a volume management private area (y/n)? ");
	if (yes_response()) {
		sliceinfo[VPRIVATESLICE].createflag = TRUE;
		vtoc.v_slices[VPRIVATESLICE].p_tag = V_MANAGED_2;
		sprintf(sliceinfo[VPRIVATESLICE].sname,"/dev/volprivate");
	}
}

/* chkname verifies that the name for the slice is a directory file
 * that it isn't allocated to another slice and that the directory
 * is not currently mounted on. 
 */
int
chkname(slicename,cur_index)
char *slicename;
int cur_index;
{
 	int i;
	struct stat statbuf;

        if (stat(slicename, &statbuf) == 0) {
		if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {
			(void) putc('\n', stderr);
                        (void) pfmt(stderr, MM_ERROR,
				":83:%s is not a directory file.\n", slicename);
                        return(1);
		}
		if (statbuf.st_ino == ROOT_INO) {
			(void) putc('\n', stderr);
			(void) pfmt(stderr, MM_ERROR,
				":84:%s directory is already mounted on.\n", slicename);
			return(1);
		}
	}
	for (i=0; i<cur_index; i++) {
		if (strcmp(sliceinfo[i].sname,slicename) == 0) {
			(void) putc('\n', stderr);
 			(void) pfmt(stderr, MM_ERROR,
				":85:%s directory has already been used.\n", slicename);
			return(1);
		}
	}
	return(0);
}

/* get_slices will query the user to collect slices for configuring   */
/* additional disks added to the system. The user will be allowed to  */
/* choose the number of slices desired and the names of them.	      */
get_slices()
{
	long count, len, i, slices;
	long availslices=0;
	char *endptr;

	for (i=1; i < max_slices; i++)
		if (
			(vtoc.v_slices[i].p_size == 0) &&
			(vtoc.v_slices[i].p_tag == V_UNUSED)
		) {
			availslices++;
		}
	if (! bootdisk) {
		(void)pfmt(
			stdout,
			MM_NOSTD,
			":73:You will now be queried on the setup of your disk."
		);
		(void)pfmt(
			stdout,
			MM_NOSTD,
			":86:After you\nhave determined which slices will be created, you will be \nqueried to designate the sizes of the various slices.\n"
		);
	}
	for (;;) {
		/* flush input prior to prompt -- prevent typeahead */
		/* note that we treat tcflush as void -- may not
		 * succeed because input is from file
		 */
		(void) tcflush(0,TCIFLUSH);
		if (!bootdisk) {
			(void)pfmt(
				stdout,
				MM_NOSTD,
				":87:\nHow many slices/filesystems do you want created on the disk (1 - %d)? ", availslices
			);
		} else {
			(void)pfmt(
				stdout,
				MM_NOSTD,
				":498:\nHow many additional slices/filesystems do you wish to create (0 - %d)? ", availslices
			);
		}
		gets(replybuf);
		slices = strtol(replybuf, &endptr, 10);
		if ((replybuf != endptr) &&
		   ((slices >= 0) && (slices <= availslices)))
			break;
		(void) pfmt(stderr, MM_WARNING,
			":88:Illegal value: %d; try again. \n", slices);
	}
	for (i = 1, count = 1; count <= slices && i < max_slices; i++) 
		if (
			(vtoc.v_slices[i].p_size == 0) &&
			(vtoc.v_slices[i].p_tag == V_UNUSED)
		) {
			if (i >= vtoc.v_nslices) {
				vtoc.v_nslices = i + 1;
			}
			/* flush input prior to prompt -- prevent typeahead */
			/* note that we treat tcflush as void -- may not
			 * succeed because input is from file
			 */
			(void) tcflush(0,TCIFLUSH);
			for (;;) {
			(void) pfmt(stdout, MM_NOSTD,
				":89:\nPlease enter the absolute pathname (e.g., /home3) for \nslice/filesystem %d (1 - 32 chars)? ", count);
				gets(replybuf);
                                if (((len = strlen(replybuf) + 1) >= SNAMSZ) ||
                                   (replybuf[0] != '/')) {
			 /* flush input prior to prompt -- prevent typeahead */
			 /* note that we treat tcflush as void -- may not
			  * succeed because input is from file
			  */
			 		(void) tcflush(0,TCIFLUSH);
					(void) putc('\n', stderr);
                                        (void) pfmt(stderr, MM_WARNING,
						":90:Illegal value: %s \n",
							replybuf);
					(void) pfmt(stderr, MM_NOSTD,
						":91:Value must begin with '/' and contain 32 characters or less.\n");
                                }
                                else
                                    	if (chkname(replybuf,i) == 0)
                                                break;
			}
			sprintf(sliceinfo[i].sname, "          ");
			strncpy(sliceinfo[i].sname, replybuf, len);
			sliceinfo[i].createflag = TRUE;
			sliceinfo[i].size = 0;
			sliceinfo[i].fsslice = get_fs_type(FALSE, i); 
			vtoc.v_slices[i].p_tag = V_USR;
			(void) tcflush(0,TCIFLUSH);
			(void) pfmt(stdout, MM_NOSTD,
				":92:\nShould %s be automatically mounted during a reboot?\n",
					sliceinfo[i].sname);
			(void) pfmt(stdout, MM_NOSTD,
				":93:Type \"no\" to override auto-mount or press <ENTER> to enable the option: ");
		
			gets(replybuf);
			if ((strcmp(replybuf, "no") == 0) ||
			    (strcmp(replybuf, "NO") == 0))
				sliceinfo[i].flag |= SL_NO_AUTO_MNT;
			count++;
		}
}

/* get_slice_sizes will go through the sliceinfo structure to query the */
/* user on the desired slice size. The slices to be queried on will have */
/* the createflag set. Slices which have predetermined sizes (boot and alts */
/* will have been setup in other routines.				*/
get_slice_sizes()
{
	long cyls, i, j;
	long minsum;
	long minsumrem;
	long remcyls, minsz, maxsz, cyls_left, fs_limit;
	char *endptr;
	int fs_indx;

	minsum=0;
	for (i=1; i < vtoc.v_nslices; i++) {
		if (bootdisk && i < V_NUMPAR)
			j = querylist[i];
		else
			j = i;
		if (sliceinfo[j].createflag) 
			minsum += (sliceinfo[j].minsz+cylsecs/2)/cylsecs;
	}
	remcyls = ((unix_base + unix_size) - pstart) / cylsecs;
	(void) pfmt(stdout, MM_NOSTD,
	  ":94:\nYou will now specify the size in cylinders of each slice.\n");
	if (cylbytes <= ONEMB)
		(void) pfmt(stdout, MM_NOSTD,
			":95:(One megabyte of disk space is approximately %d cylinders.)\n",
			(ONEMB + cylbytes/2) / cylbytes);
	else
		(void) pfmt(stdout, MM_NOSTD,
			":395:(One cylinder is approximately %d megabytes of disk space.)\n",
			(cylbytes + ONEMB/2) / ONEMB);
	for (i=1; i < vtoc.v_nslices; i++) {
		if (bootdisk && i < V_NUMPAR)
			j = querylist[i];
		else
			j = i;
		if ((sliceinfo[j].createflag) && (remcyls > 0)) 
			for (;;) {
				minsz = 0;
				if (sliceinfo[j].minsz > 0) {
					minsz = (sliceinfo[j].minsz+cylsecs/2)/cylsecs;
					(void) pfmt(stdout, MM_NOSTD,
						":96:\nThe recommended minimum size for the %s slice is %d cylinders (%d MB).\n",
					sliceinfo[j].sname, minsz,
					(sliceinfo[j].minsz*(int)dp.dp_secsiz+ONEMB/2)/ONEMB);
				}
				/* Keep track of min reqts for remaining
				 * slices. subtract this slice's min from that
				 * of the remaining slices.
				 */
				minsumrem = minsum - minsz;

				/* maxsz is the track of min reqts for remaining
				 * slices. subtract this slice's min from that
				 * of the remaining slices.
				 *
				 * maxsz is adjusted to reflect the limit imposed ( if any )
				 * by the filesystem type selected for this slice ( if any )
				 */

				cyls_left = maxsz = remcyls - minsumrem;
				fs_limit = 0;
				if (sliceinfo[j].fsslice) {

					int blkspercyl = (cylsecs * dp.dp_secsiz) / 512;

					fs_indx=find_fs_defaults(sliceinfo[j].fstypname,0);
					fs_limit = fstyp[fs_indx].maxsize / blkspercyl;
					if ( maxsz > fs_limit )
						maxsz = fs_limit;
				}

				/* flush input prior to prompt -- prevent typeahead */
				/* note that we treat tcflush as void -- may not
		 		 * succeed because input is from file
		 		 */
				(void) tcflush(0,TCIFLUSH);
				(void) pfmt(stdout, MM_NOSTD,
					":490:There are now %d cylinders available on your disk.\n",
					cyls_left);
				if (fs_limit)
					(void) pfmt(stdout, MM_NOSTD,
						":491:The filesystem type you have chosen is limited to %d cylinders.\n",
						fs_limit);
				(void) pfmt(stdout, MM_NOSTD,
					":97:How many cylinders would you like for %s (%d - %d)?\n",
					sliceinfo[j].sname,minsz,maxsz);
				(void) pfmt(stdout, MM_NOSTD,
					":98:Press <ENTER> for %d cylinders: ", minsz);
				gets(replybuf);
				cyls = strtol(replybuf, &endptr, 10);
				if (replybuf[0] == '\0')
					/* if user typed return, use minimum value */
					cyls = minsz;
				if (cyls < minsz) {
					(void) pfmt(stdout, MM_NOSTD,
						":99:Slice %s must be at least %d cylinders; please enter again\n",
						sliceinfo[j].sname, minsz);
					continue;
				}
				if (cyls > maxsz ) {
					(void) pfmt(stdout, MM_NOSTD,
						":100:Slice %s must be no more than %d cylinders; please enter again\n",
						sliceinfo[j].sname, maxsz);
					continue;
				}
				vtoc.v_slices[j].p_start = pstart;
				vtoc.v_slices[j].p_size = cyls * cylsecs;
				vtoc.v_slices[j].p_flag = V_VALID;
				pstart += cyls * cylsecs;
				if (sliceinfo[j].fsslice == 0)
					vtoc.v_slices[j].p_flag |=V_UNMNT;
				remcyls -= cyls;
				minsum=minsumrem;
				break;
			}
	}
	if (remcyls)
		(void) pfmt(stdout, MM_NOSTD,
			":101:\nNotice: The selections you have made will leave %d cylinders unused.\n", remcyls);
}

/* issue_mkfs will handle the details of the mkfs exec. The items to be dealt */
/* with include which mkfs, and where mkfs is 				      */
issue_mkfs(slice, rawdev, size, secspercyl)
int slice;
char *rawdev, *size, *secspercyl;
{
        char *inodectrl; /* will be set conditionally to "-o C" or ""
			  * depending on if Silent is on. This is
			  * for ufs and sfs mkfs only in order to
			  * allow auto install create file systems
			  * that won't break COFFs.
	  		  */
	int fs_indx;
	char inodearray[30] = {0};
	if (sliceinfo[slice].fsslice) {
		fs_indx = find_fs_defaults(sliceinfo[slice].fstypname, 0);

		switch (fstyp[fs_indx].mkfstyp) {

		case MKFS_STYL1 :
			if (!Silent && !xflg) {
				ino_t ricount;
				ino_t icount = (atol(size)/(sliceinfo[slice].fsblksz/512))/4;

				(void) pfmt(stdout, MM_NOSTD,
					":102:Allocated approximately %d inodes for this file system\n", icount);
				(void) pfmt(stdout, MM_NOSTD,
					":103:Specify a new value or press <ENTER> to use the default: ");
				gets(replybuf);
				if (strcmp(replybuf, "") != 0) {
					ricount=atol(replybuf);
					if  (ricount != icount) {
						strcpy(inodearray, ":");
						strncat(inodearray, replybuf,(sizeof(inodearray)-2));
					}
				}
			} else
				strcpy(inodearray, "");
			sprintf(replybuf,"%s%s%s%s%s -b %d %s %s%s 1 %s >/dev/null 2>&1",
				FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname,
				TOKEN_SEP, MKFS_CMD, sliceinfo[slice].fsblksz,
				rawdev, size, inodearray, secspercyl);
			break;

		case MKFS_STYL2 : {
			/*
			 * The following icount code are for the MR
			 * us94-21401 to reduce the number of inodes specified
			 * for the /stand slice which uses the bfs file system
			 * type.
			 *
			 * The algorithm for calculating the number of inode is
			 * to allocate one inode per 512 bfs blocks (in other
			 * words, each MB of the bfs will have 4 inodes), and a
			 * minimum of 16 inodes are specified when the size of
			 * the bfs is less than 4MB.
			 */
			ino_t icount = (atol(size)/(sliceinfo[slice].fsblksz/512))/512;
			if (icount < 16)
				icount = 16;
				
			sprintf(replybuf,"%s%s%s%s%s %s %s %d >/dev/null 2>&1",
				FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname,
				TOKEN_SEP, MKFS_CMD, rawdev, size, icount);
			}
			break;

		case MKFS_STYL3 :
			if (!Silent && !xflg) {
				long rinodeval;
				long inodeval = 2048;
				(void) pfmt(stdout, MM_NOSTD,
					":104:One inode is allocated for each %d bytes of file system\nspace.", inodeval);
				(void) pfmt(stdout, MM_NOSTD,
					":105: Specify a value in units of bytes or press <ENTER>\nto use the default value: ");
				gets(replybuf);
				if (strcmp(replybuf, "") != 0) {
					rinodeval = atol(replybuf);
					if (rinodeval != inodeval) {
						strcpy(inodearray, ",nbpi=");
						strncat(inodearray, replybuf, (sizeof(inodearray)-6));
					}
				}
			} else
				strcpy(inodearray, "");
			if (Silent)
				inodectrl = "C,";
			else
				inodectrl = "";
			sprintf(replybuf,"%s%s%s%s%s -o %sbsize=%d%s %s %s >/dev/null 2>&1",
			FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname,
			TOKEN_SEP, MKFS_CMD, inodectrl,
			sliceinfo[slice].fsblksz, inodearray, rawdev, size);
			break;

		case MKFS_STYL4 :
			if (!Silent && !xflg) {
				ino_t ricount;
				ino_t icount = ((atol(size)-256)/(sliceinfo[slice].fsblksz/512))/4;

				(void) pfmt(stdout, MM_NOSTD,
					":106:Allocated approximately %d inodes for this file system.", icount);
				(void) pfmt(stdout, MM_NOSTD,
					":107: Specify a\nnew value or press <ENTER> to use the default: ");
				gets(replybuf);
				if (strcmp(replybuf, "") != 0) {
					ricount=atol(replybuf);
					if  (ricount != icount) {
						strcpy(inodearray, ",ninode=");
						strncat(inodearray, replybuf,(sizeof(inodearray)-9));
					}
				}
			} else
				strcpy(inodearray, "");

			if (Silent)
				inodectrl = "C,";
			else
				inodectrl = "";
			sprintf(replybuf,"%s%s%s%s%s  -o %sbsize=%d%s %s %s >/dev/null 2>&1",
				FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname,
				TOKEN_SEP, MKFS_CMD, inodectrl, sliceinfo[slice].fsblksz,
				inodearray, rawdev, size);
			break;

		default :
			(void) pfmt(stderr, MM_ERROR,
				":108:Invalid mkfs type specified for slice %s, file system type %s\n",
				sliceinfo[slice].sname, sliceinfo[slice].fstypname);
			(void) pfmt(stderr, MM_ERROR,
				":109:entry ignored\n");
			return;
		}
	}

	if (Show)
		(void) pfmt(stdout, MM_NOSTD,
			":110:Running cmd: %s\n", replybuf);
	if (exec_command(replybuf)) {
		(void) pfmt(stderr, MM_ERROR,
			":111:unable to create filesystem on %s.\n", rawdev);
		(void) pfmt(stderr, MM_ERROR|MM_NOGET,
			"%s\n", strerror(errno));
		fs_error(sliceinfo[slice].sname);
	}
}

create_fs()
{
	int i, status;
	char secspercyl[5];
	char gap[2];
	int blkspersec;
	char buf[LINESIZE];
	int fs_indx;

	blkspersec = dp.dp_secsiz / 512;
	sprintf(secspercyl, "%d", cylsecs);
	if (!Silent) {
		sprintf(buf, "echo \"%s\"",
			gettxt(":112", "\nFilesystems will now be created on the needed slices\n"));
		exec_command(buf);
	}
	for (i=1; i < vtoc.v_nslices; i++) {
		if ((vtoc.v_slices[i].p_size > 0) && (sliceinfo[i].fsslice == TRUE)) {
			char rawdev[25], size[12], msg[LINESIZE];
			sprintf(rawdev, "%s%x", mkfsname, i);
			if (!Silent) {
				sprintf(msg, (char *)gettxt(":492", "Creating the %s filesystem on %s \n"),
					sliceinfo[i].sname, rawdev);
				sprintf(buf, "echo %s", msg);
				exec_command(buf);
			}
			/*
			 * MR#ul94-20832: When slice size is larger than
			 * maximum file system size (because of rounding up
			 * to the next cylinder boundary), try to create file
			 * system using the maximum file system size instead of
			 * the slice size.
			 */
			fs_indx=find_fs_defaults(sliceinfo[i].fstypname,0);
			if (vtoc.v_slices[i].p_size > (fstyp[fs_indx].maxsize/blkspersec))
				sprintf(size, "%ld", fstyp[fs_indx].maxsize);
			else
				sprintf(size, "%ld", vtoc.v_slices[i].p_size*blkspersec);
			issue_mkfs(i, rawdev, size, secspercyl);
			if (fstyp[fs_indx].labelit == TRUE)
				label_fs(i, rawdev);
			write_vfstab(i, rawdev);
		}
	}
	close(vfstabfd);
}

label_fs(slice, dev)
int slice;
char *dev;
{
	int status;
	char disk[7];

	sprintf(disk,"slic%d",slice);
	/* build labelit command */
	sprintf(replybuf,"%s%s%s%s%s %s %.6s %.6s >/dev/null 2>&1",
		FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname, TOKEN_SEP,
		LABELIT_CMD, dev, sliceinfo[slice].sname, disk);
	if (Show)
		(void) pfmt(stdout, MM_NOSTD,
			":110:Running cmd: %s\n", replybuf);
	if (exec_command(replybuf)) {
		(void) pfmt(stderr, MM_WARNING,
			":114:unable to label slice %s %s %s\n",
			dev, sliceinfo[slice].sname, disk);
		(void) pfmt(stderr, MM_ERROR|MM_NOGET,
			"%s\n", strerror(errno));
	}
}

write_vfstab(slice, dev)
int slice;
char *dev;
{
	char blkdev[25], buf[1024], *tmppt;
	int perms, status, len;
	int mountfd, i, found = 0;
	struct stat statbuf;
	char	*mount_it;
	char option[MAXOPTCNT] = "-";

	sprintf(blkdev, "%s", dev);
	tmppt = blkdev;
	while (*tmppt != NULL) {
		if (*tmppt == 'r' || found) {
			*tmppt = *(tmppt+1);
			found = TRUE;
		}
		tmppt++;
	}
	if (bootdisk && slice == 1) {
			/* build mount cmd */
			sprintf(replybuf,"%s%s%s%s%s %s /mnt >/dev/null 2>&1",
			FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname, TOKEN_SEP,
			MOUNT_CMD, blkdev);
		if (Show)
			(void) pfmt(stdout, MM_NOSTD,
				":110:Running cmd: %s\n", replybuf);
		if (exec_command(replybuf)) {
			(void) pfmt(stderr, MM_ERROR,
				":115:cannot mount root\n");
			(void) pfmt(stderr, MM_ERROR|MM_NOGET,
				"%s\n", strerror(errno));
			fs_error(sliceinfo[slice].sname);
		}
		if(execfd >= 0) {
			sprintf(replybuf, "mkdir /mnt/etc; chmod 775 /mnt/etc\n");
			exec_command(replybuf);
			}
		else	if (mkdir("/mnt/etc", 0775) == -1) {
			(void) pfmt(stderr, MM_ERROR,
				":116:Cannot create /mnt/etc.\n");
			(void) pfmt(stderr, MM_ERROR|MM_NOGET,
				"%s\n", strerror(errno));
			fs_error(sliceinfo[slice].sname);
		}
		if (execfd < 0)
			vfstabfd=open("/mnt/etc/vfstab",O_CREAT|O_WRONLY,0644);
		else 	vfstabfd=open("/tmp/vfstab",O_CREAT|O_WRONLY,0644);
		if(vfstabfd < 0){
			(void) pfmt(stderr, MM_ERROR,
				":117:Cannot create /etc/vfstab.\n");
			(void) pfmt(stderr, MM_ERROR|MM_NOGET,
				"%s\n", strerror(errno));
			fs_error(sliceinfo[slice].sname);
		}
		else {

		    if (sliceinfo[slice].fsslice == TRUE) {
			int fs_indx;
			fs_indx = find_fs_defaults(sliceinfo[slice].fstypname, 0);
			if (strlen(fstyp[fs_indx].mntopt) > 0)
				strcpy(option, fstyp[fs_indx].mntopt); 
			len = sprintf(buf,
	"/dev/root	/dev/rroot	/	%s	1	no	%s	%s\n",
	sliceinfo[slice].fstypname, option, MACCEILING);

			if(write(vfstabfd, buf, len) != len ) {
				(void) pfmt(stderr, MM_ERROR,
					":118:cannot write /etc/vfstab entry.\n");
				(void) pfmt(stderr, MM_ERROR|MM_NOGET,
					"%s\n", strerror(errno));
				fs_error(sliceinfo[slice].sname);
			}

		    }
		}
	}
	else {
	   if (vfstabfd < 0) {
		if (execfd  < 0) {
		     vfstabfd=open("/etc/vfstab",O_WRONLY|O_APPEND);
		     if(vfstabfd < 0) {
			     vfstabfd=open("/mnt/etc/vfstab",O_WRONLY|O_APPEND);
			     instsysflag = TRUE;
		     }
		} else {
			vfstabfd=open("/tmp/vfstab",O_WRONLY|O_APPEND);
			instsysflag = TRUE;
		}
		if (vfstabfd < 0) {
			instsysflag = FALSE;
			(void) pfmt(stderr, MM_ERROR,
				":119:cannot open /etc/vfstab.\n");
			(void) pfmt(stderr, MM_ERROR|MM_NOGET,
				"%s\n", strerror(errno));
			fs_error(sliceinfo[slice].sname);
		}
	    }

		if( strcmp(sliceinfo[slice].sname, "/") == 0 ||
		    strcmp(sliceinfo[slice].sname, "/stand") == 0 ||
		    strcmp(sliceinfo[slice].sname, "/var") == 0 ||
		    sliceinfo[slice].flag & SL_NO_AUTO_MNT)
			mount_it = "no";
		else	mount_it = "yes";

		if (sliceinfo[slice].fsslice == TRUE) { 
			int fs_indx;
			fs_indx = find_fs_defaults(sliceinfo[slice].fstypname, 0);
			if (strlen(fstyp[fs_indx].mntopt) > 0)
				strcpy(option, fstyp[fs_indx].mntopt); 
		}
		if (sliceinfo[slice].fsslice == TRUE) {
			if (bootdisk && slice == 10) {
				len = sprintf(buf, "/dev/stand	/dev/rstand	%s	%s	1	%s	%s	%s\n",
				sliceinfo[slice].sname, sliceinfo[slice].fstypname,
				mount_it, option, MACCEILING);
			}
			else {
				len = sprintf(buf, "%s	%s	%s	%s	1	%s	%s	%s\n",
				blkdev, dev, sliceinfo[slice].sname, sliceinfo[slice].fstypname,
				mount_it, option, MACCEILING);
			}
		}
		if(write(vfstabfd, buf, len) != len) {
			(void) pfmt(stderr, MM_ERROR,
				":118:cannot write /etc/vfstab entry.\n");
			(void) pfmt(stderr, MM_ERROR|MM_NOGET,
				"%s\n", strerror(errno));
			fs_error(sliceinfo[slice].sname);
		}
		if ((bootdisk == TRUE) || (instsysflag == TRUE)) {
			sprintf(buf, "/mnt%s", sliceinfo[slice].sname);
			if (strncmp(sliceinfo[slice].sname,"/tmp",4) == 0)   
				perms = 01777;
			else 
				perms = 0755;
			if (execfd >= 0) {
				len = sprintf(replybuf, "mkdir -p %s; chmod %o %s\n",
					buf, perms, buf);
				write(execfd, replybuf, strlen(replybuf));
				}
			else	if (mkdir(buf, perms) != 0) {
				(void) pfmt(stderr, MM_ERROR,
					":120:could not create %s mount point.\n", buf);
				(void) pfmt(stderr, MM_ERROR|MM_NOGET,
					"%s\n", strerror(errno));
				fs_error(buf);
			}
		}
		else {
			strcpy(buf, sliceinfo[slice].sname);
			sprintf(replybuf,"mkdir -m 0755 -p %s",buf);
			if (execfd >= 0)
				exec_command(replybuf);
			else	if (stat(buf, &statbuf) == -1)  {
				if (Show)
					(void) pfmt(stdout, MM_NOSTD,
					     ":110:Running cmd: %s\n", replybuf);
				if (exec_command(replybuf) != 0) {
					(void) pfmt(stderr, MM_ERROR,
					     ":120:could not create %s mount point.\n", buf);
					(void) pfmt(stderr, MM_ERROR|MM_NOGET,
						"%s\n", strerror(errno));
				}
			}
			else {
				if (!(statbuf.st_mode & S_IFDIR)) {
					(void) pfmt(stderr, MM_ERROR,
						":121:%s is not a valid mount point\n", buf);
					fs_error(buf);
				}
			}
		}
		/* if a mount option is specified use it 
		 * in the mount request.
	 	 */
		if (sliceinfo[slice].fsslice == TRUE) {
			int fs_indx;
			fs_indx = find_fs_defaults(sliceinfo[slice].fstypname, 0);
			if (strlen(fstyp[fs_indx].mntopt) > 0) {
				strcpy(option, "-o");
				strcat(option, fstyp[fs_indx].mntopt); 
			} else
				strncpy(option, "", sizeof(option));

			sprintf(replybuf,"%s%s%s%s%s %s %s %s >/dev/null 2>&1",
			FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname, TOKEN_SEP,
			MOUNT_CMD, option, blkdev, buf);
		}

		if (Show)
			(void) pfmt(stdout, MM_NOSTD,
				":110:Running cmd: %s\n", replybuf);
		if (exec_command(replybuf)) {
			(void) pfmt(stderr, MM_ERROR,
				":122:unable to mount %s.\n", buf);
			(void) pfmt(stderr, MM_ERROR|MM_NOGET,
				"%s\n");
			fs_error(sliceinfo[slice].sname);
		}
	}
}

/* Utility routine for turning signals on and off */
set_sig(n)
void (*n)();
{
	signal(SIGINT, n);
	signal(SIGQUIT, n);
	signal(SIGUSR1, n);
	signal(SIGUSR2, n);
}

set_sig_on()
{
	set_sig(SIG_DFL);
}

set_sig_off()
{
	set_sig(SIG_IGN);
}

/*
 *	Allocate the alternate sector/track partition 
 *	enter the partition information in the vtoc table
 */
alloc_altsctr_part()
{
	int	altsec_size;
	int	extra_alt;

	altsec_size = cylsecs - (pstart % cylsecs);
 	if ((extra_alt=(unix_size * (daddr_t)dp.dp_secsiz)/ONEMB) < 32)
		extra_alt = 32;
 	if (altsec_size < (badsl_chain_cnt + extra_alt))
		altsec_size += (((badsl_chain_cnt + extra_alt) 
				/ (int)dp.dp_sectors) + 1) * dp.dp_sectors;

	/* reserve space in chunks of cylinders to correspond with the	*/
	/*	calculated need for alternates.				*/
	vtoc.v_slices[ALTSCTRSLICE].p_start = pstart;
	vtoc.v_slices[ALTSCTRSLICE].p_flag  = V_VALID | V_UNMNT;
	vtoc.v_slices[ALTSCTRSLICE].p_tag   = V_ALTSCTR;
	vtoc.v_slices[ALTSCTRSLICE].p_size  = altsec_size;
	pstart += altsec_size;
	ap->ap_flag |= ALTS_ADDPART;
}

exec_command(buf)
char	*buf;
{
	int	len = strlen(buf);

	if(execfd >= 0) {
		if(write(execfd, buf, len) != len ||
		   write(execfd, "\n", 1) != 1)
				return(1);
		return(0);
		}
	return(system(buf));
}

/* this routine reads in the available fs types from /etc/default/fstyp
 * and /etc/default/"file" where "file" contains the file system specific
 * identifiers required by disksetup.
 */
void
rd_fs_defaults()
{
int i;
FILE  *fd;
char *p, *s;
char buf[20];

	/* read in the available file system types */
	i=0;
	if ((fd = defopen(DEF_FILE)) != NULL) {
		if ((p = defread(fd, "FSTYP")) == NULL || 
		   strcpy(fsnamelist, p) == 0 ||
		   (s = strtok(p, ",")) == (char *)NULL) {
			(void) pfmt(stderr, MM_ERROR,
				":123:FSTYP identifier invalid or not specified in %s%s\n", DEF_DIR, DEF_FILE);  
			free(sliceinfo);
			exit(-1);
		}

		fstyp_cnt = 0;
		while(s != NULL && fstyp_cnt < MAXFS) {
			strncpy(fstyp[fstyp_cnt++].fsname, s, MAXNAME);
			s = strtok(NULL, ",");
		}

		/* check whether MAX file system types was exceeded */ 
		if (fstyp_cnt+1 == MAXFS) {
			(void) pfmt(stderr, MM_ERROR,
				":124:Too many fs types specified in %s%s\n",
				DEF_DIR, DEF_FILE);
			free(sliceinfo);
			exit(-1);
		}

		/* read the boot fs type in */
		if ((p = defread(fd, "BOOT_FSTYP")) != NULL) {
			strncpy(fstyp[fstyp_cnt].fsname, p, MAXNAME);
			fstyp[fstyp_cnt++].flag |= BOOTFSTYPE;
		} else {
			(void) pfmt(stderr, MM_ERROR,
				":125:BOOT_FSTYP identifier not specified in %s%s\n",
				DEF_DIR, DEF_FILE);
			free(sliceinfo);
			exit(-1);
		}
		defclose(fd);
		/* read in file system specific identifiers */
		for (i=0;i<fstyp_cnt;i++) {
			if ((fd = defopen(fstyp[i].fsname)) == NULL) {
				(void) pfmt(stderr, MM_ERROR,
				":126:failed to open %s%s\n",
					DEF_DIR, fstyp[i].fsname); 
				free(sliceinfo);
				exit(-1);
			}
			if ((p = defread(fd, "BLKSIZE")) != NULL ||
				*p != '\0') {
				int cnt = 0;
				s = strtok(p, ",");
				if (s == NULL) {
					(void) pfmt(stderr, MM_ERROR,
						":127:BLKSIZE identifer not specified in %s%s\n",
						DEF_DIR, fstyp[i].fsname);
					free(sliceinfo);
					exit(-1);
				}
				while (s != NULL) {
					int blksz = atol(s);
					if ((blksz % 512) != 0) {
						(void) pfmt(stderr, MM_ERROR,
							":128:bad block size identfier specified in %s%s\n",
						DEF_DIR, fstyp[i].fsname);
						(void) pfmt(stderr, MM_ERROR,
							":129:file system type ignored\n");
						strcpy(fstyp[i].fsname, "BADFSENTRY");
						cnt = 0;
						break;
					} 
					fstyp[i].blksize[cnt++] = atol(s);
					s = strtok(NULL, ",");

				}
				fstyp[i].blksiz_cnt = cnt;
			}
			if ((p = defread(fd, "MNTOPT")) != NULL) 
				strncpy(fstyp[i].mntopt, p, MAXOPTCNT);

			else
				strcpy(fstyp[i].mntopt, "");

			if ((p = defread(fd, "MKFSTYP")) != NULL) 
				fstyp[i].mkfstyp = atoi(p);
			else {
				(void) pfmt(stderr, MM_ERROR,
					":130:MKFSTYP identifier not found in %s%s\n",
					DEF_DIR, fstyp[i].fsname);
				free(sliceinfo);
				exit(-1);
			}

			if ((p = defread(fd, "LABELIT")) != NULL) {
				if (strcmp(p, "YES") == 0 ||
					strcmp(p, "yes") == 0)

					fstyp[i].labelit = TRUE;
				else
					fstyp[i].labelit = FALSE;
			} else
				fstyp[i].labelit = FALSE;

			if ((p = defread(fd, "MAXSIZE")) != NULL) 
				fstyp[i].maxsize = strtoul(p, (char **)NULL, 10);
			else
				fstyp[i].maxsize = MAXBLKSZ;

			defclose(fd);
		}

				

	} else {

		(void) pfmt(stderr, MM_ERROR,
			":131:open for fs default file failed %s%s\n",
			DEF_DIR, DEF_FILE);
		free(sliceinfo);
		exit(-1);

	}

}

/* this routine returns the file system default array
 * index for p 
 */
int
find_fs_defaults(p, flag)
char *p;
int flag;
{
int i;
	for(i=0;i<fstyp_cnt;i++){
		if (flag & BOOTFSTYPE && fstyp[i].flag & BOOTFSTYPE) 
			return(i);
		if (strcmp(p, fstyp[i].fsname) == 0)
			return(i);
	}
	return(-1);
			
}

/* give choices of available blksizes */
int
get_fs_blksize(indx)
int indx;
{
char buf[60] = {0};
char tmp[20] = {0};
int i, j;
int size;

	if (fstyp[indx].blksiz_cnt == 1)
		return(fstyp[indx].blksize[0]);
	for(i=0;i<fstyp[indx].blksiz_cnt;i++) {
		sprintf(tmp, "%d, ", fstyp[indx].blksize[i]);
		strcat(buf, tmp);
	}
	buf[strlen(buf)-2]  = '\0';	/* get rid of last comma */
	for(;;) {
		/* flush input prior to prompt -- prevent typeahead */
		/* note that we treat tcflush as void -- may not
		 * succeed because input is from file
		 */
		(void) tcflush(0,TCIFLUSH);
		(void) pfmt(stdout, MM_NOSTD,
			":132:\nSpecify the block size from the the following list\n(%s), or press <ENTER> to use the first one: ",
			buf); 
		gets(replybuf);

		/* use the default blocksize */
		if (strcmp(replybuf, "") == 0)
			return(fstyp[indx].blksize[0]);

		size = atol(replybuf);
		for(j=0;j<fstyp[indx].blksiz_cnt;j++)
			if (size == fstyp[indx].blksize[j])
				return(size);
		(void) pfmt(stdout, MM_NOSTD,
			":133:\nInvalid response:  block size specified was '%s'\n", replybuf);
	}
}

int
wipeout(void)
{
	struct phyio	p;
	char *		buf;

	if ((buf = calloc(32 * dp.dp_secsiz, sizeof(char))) == NULL) {
		return 1;
	}
	p.sectst = 0;
	p.memaddr = (unsigned long)buf;
	p.datasz = 32 * dp.dp_secsiz;
	if (ioctl(diskfd, V_PWRITE, &p) == -1) {
		free(buf);
		return 1;
	}
	free(buf);
	return 0;
}
