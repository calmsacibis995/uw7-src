/*		copyright	"%c%" 	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/mkfs/mkfs.c	1.3.11.13"

/*
 * make file system for SFS cylinder-group style file system
 *
 * usage:
 *
 *    mkfs [-F sfs] [-V] [-m] [options] [-o specific_options]  special size
 *	[ nsect ntrak bsize fsize cpg rps nbpi opt apc rotdelay ]
 *  
 *
 *  where specific_options are:
 *	M - make root of the file system a Multi-level directory
 *      N - no create
 *	nsect - The number of sectors per track
 *	ntrack - The number of tracks per cylinder
 *	bsize - block size
 *	fragsize - fragment size
 *	cgsize - The number of disk cylinders per cylinder group.
 *	rps - rotational speed (rev/sec).
 *	nbpi - number of bytes per allocated inode
 *	opt - optimization (space, time)
 *	apc - number of alternates
 *	gap - gap size
 *	L - large file system. Do not limit number of inodes.
 *	C - Compatability. Limit number of inodes to < 32 K
 *		(for compatability with pre SVR4.0 applications)
 *
 */

/*
 * MINFREE gives the minimum acceptable percentage of file system
 * blocks which may be free. If the freelist drops below this level
 * only the superuser may continue to allocate blocks. This may
 * be set to 0 if no reserve of free blocks is deemed necessary,
 * however throughput drops by fifty percent if the file system
 * is run at between 90% and 100% full; thus the default value of
 * fs_minfree is 10%. With 10% free space, fragmentation is not a
 * problem, so we choose to optimize for time.
 */
#define MINFREE		10
#define DEFAULTOPT	FS_OPTTIME

/*
 * ROTDELAY gives the minimum number of milliseconds to initiate
 * another disk transfer on the same clider. It is used in
 * determining the rotationally opiml layout for disk blocks
 * within a file; the default of fs_rotdelay is 4ms.
 */
#define ROTDELAY	4

/*
 * MAXCONTIG sets the default for the maximum number of blocks
 * that may be allocated sequentially. Since UNIX drivers are
 * not capable of scheduling multi-block transfers, this defaults
 * to 1 (ie no contiguous blocks are allocated).
 */
#define MAXCONTIG	1

/*
 * MAXBLKPG determines the maximum number of data blocks which are
 * placed in a single cylinder group. This is currently a function
 * of the block and fragment size of the file system.
 */
#define MAXBLKPG(fs)	((fs)->fs_fsize / sizeof(daddr_t))

/*
 * Each file system has a number of inodes statically allocated.
 * We allocate one inode slot per NBPI bytes, expecting this
 * to be far more than we will ever need.
 */
#define	NBPI		1024

/*
 * Disks are assumed to rotate at 60HZ, unless otherwise specified.
 */
#define	DEFHZ		60

#ifndef STANDALONE
#include <stdio.h>
#include <a.out.h>
#endif

#include <sys/param.h>
#include <time.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/vnode.h>
#include <sys/fs/sfs_fsdir.h>
#include <sys/acl.h>
#include <sys/fs/sfs_inode.h>
#include <sys/fs/sfs_fs.h>
#include <sys/mntent.h>
#include <sys/stat.h>
#include <sys/mac.h>					/* MLD */
#include <sys/mkfs.h>	/* exit code definitions */
#include <unistd.h>
#include <locale.h>
#include <ctype.h>
#include <pfmt.h>
#include <errno.h>
#include "mkfs_hw.h"

#define UMASK		0755
#define MAXINOPB	(MAXBSIZE / sizeof(struct dinode))
#define POWEROF2(num)	(((num) & ((num) - 1)) == 0)
#define BLK_TO_INODES   64
#define	NAME_MAX	64
#define	FSTYPE		"sfs"

extern int	optind;
extern char	*optarg;

union {
	struct fs fs;
	char pad[SBSIZE];
} fsun;
#define	sblock	fsun.fs
struct	csum *fscs;

union {
	struct cg cg;
	char pad[MAXBSIZE];
} cgun;
#define	acg	cgun.cg


char	*fsys;
time_t	utime;
int	fsi;
int	fso;
int	Nflag = 0;	/* do not execute the function */
int	mflag = 0;	/* return the command line used to create this FS */
int     Cflag = 0;      /* Limit number inodes < 32K; SVR3.2 Compatibility */
int	Lflag = 0;	/* Do not limit number of inodes. reverse of Cflag. */
int	Mflag = 0;	/* make root inode a Multi-Level Directory (MLD) */
daddr_t	alloc();

unsigned int number();
/*
 * zerobuf is used to clear out the first 8k of the slice,
 * as well as to clear out each inode block.
 */
char zerobuf[BBSIZE];

/* default values for mkfs */
int	nsect = DFLNSECT;	/* fs_nsect */
int	ntrack = DFLNTRAK;	/* fs_ntrak */
int	bsize = PAGESIZE; 	/* fs_bsize */
int	fragsize = DESFRAGSIZE; /* fs_fsize */
int	cgsize = DESCPG; 	/* fs_cpg */
int	cpg_flag = 0;		/* cgsize specified */
int	minfree= MINFREE; 		/* fs_minfree */
int	rps = DEFHZ; 
int	nbpi = NBPI;
int	nbpi_flag = 0;
char	opt = 't';		/* fs_optim */
int	apc = 0;		
int	apc_flag = 0;
int	rot =  -1;		/* gap size (rotational delay) */
char	*string;
char	*myname, fstype[]=FSTYPE;

/* Forward references */
void clrblock();
void dump_fs();
void setblock();
void usage();
void wtfs();


main(argc, argv)
	int argc;
	char *argv[];
{
	long cylno, rpos, blk, i, j, inos, fssize, warn = 0;
	int spc_flag = 0;
	struct stat statarea;
	int	c;
	char 	specfrag=0;
	char	label[NAME_MAX+8];

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmkfs");
	myname = (char *)strrchr(argv[0], '/');
	myname = (myname != 0)? myname+1: argv[0];
	sprintf(label, "UX:%s %s", fstype, myname);
	(void)setlabel(label);

	while ((c = getopt (argc, argv, "bmo:V")) != EOF) {   /* MLD */
		switch (c) {

		case 'm':	/* return the command line used to create this FS */
			mflag++;
			break;

		case 'o':
			/*
			 * sfs specific options.
			 */
			string = optarg;
			while (*string != '\0') {
				if (match("nsect=")) 
					nsect=number(BIG);
				else if (match("ntrack=")) 
					ntrack=number(BIG);
				else if (match("bsize=")) 
					bsize=number(BIG);
				else if (match("fragsize="))  {
					fragsize=number(BIG);
					specfrag=1;
				}
				else if (match("cgsize=")) {
					cpg_flag = 1;
					cgsize=number(BIG);
				}
				else if (match("rps=")) 
					rps=number(BIG);
				else if (match("nbpi=")) {
					nbpi_flag = 1;
					nbpi=number(BIG);
				}
				else if (match("opt=")) 
					opt = *string++;
				else if (match("apc=")) {
					apc=number(BIG);
					apc_flag = 1;
				}
				else if (match("gap=")) 
					rot=number(BIG);
				else if (match("N")) 
					Nflag++;
				else if (match("C"))
					/* Limit inodes < 32 K */
					Cflag++;
				else if (match("L"))
					Lflag++;
				else if (match("M")) {		/* MLD */
					Mflag++;		/* MLD */
				}
				/* remove the separator */
				if (*string == ',') string++;
				else if (*string == '\0') break;
				else {
					pfmt(stderr, MM_ERROR,
						":56:illegal option: %s\n",
							string);
					usage();
				}
			}
			break;

		case 'V':
			{
				char	*opt_text;
				int	opt_count;

				(void) fprintf (stdout, "mkfs -F sfs ");
				for (opt_count = 1; opt_count < argc ; opt_count++) {
					opt_text = argv[opt_count];
					if (opt_text)
						(void) fprintf (stdout, " %s ", opt_text);
				}
				(void) fprintf (stdout, "\n");
			}
			break;

		case 'b':	/* do nothing for this */
			break;

		/* -C is undocumented, unsupported, and replaced by -o C.
		   It is left in the code in case of commands that may
		   use it which might not be converted in the same load */
		case 'C':               /* Limit number inodes < 64K */
			Cflag++;	/* this is SVR3 "COMPATIBLE" option */
			break;

		case '?':
			usage();
			break;
		}
	}
	time(&utime);
	if (optind > (argc - 1)) {
		usage();
	}
	if (Cflag && Lflag) {
		pfmt(stderr, MM_ERROR, ":57:cannot specify both -o C and -o L\n");
		usage();
	}
	argc -= optind;
	argv = &argv[optind];
	fsys = argv[0];
	fsi = open(fsys, 0);
	if(fsi < 0) {
		pfmt(stderr, MM_ERROR, ":58:%s: cannot open\n", fsys);
		exit(RET_FSYS_OPEN);
	}
	if (mflag) {
		dump_fs (fsys, fsi);
		exit(RET_OK);
	}	
	if (argc < 2) {
		usage();
	}
	if (!Nflag) {
		fso = creat(fsys, 0666);
		if(fso < 0) {
			pfmt(stderr, MM_ERROR,
				":59:%s: cannot create\n", fsys);
			exit(RET_FSYS_CREATE);
		}
		if(stat(fsys, &statarea) < 0) {
			pfmt(stderr, MM_ERROR,
				":2:%s: cannot stat\n", fsys);
			exit(RET_FSYS_STAT);
		}
		if ((statarea.st_mode & S_IFMT) != S_IFBLK &&
		    (statarea.st_mode & S_IFMT) != S_IFCHR) {
			pfmt(stderr, MM_ERROR,
				":60:%s is not special device\n", fsys);
			exit(RET_FSYS_DEV);
		}
		if (statarea.st_flags & _S_ISMOUNTED) {
			pfmt(stderr, MM_ERROR,
				":61:%s is mounted, cannot mkfs\n", fsys);
			exit(RET_FSYS_MOUNT);
		}

	}
	/*
	 * Validate the given file system size.
	 * Verify that its last block can actually be accessed.
	 */
	if (argc < 2) {
		exit(RET_OK);
	}
	fssize = atoi(argv[1]);
	if (fssize <= 0) {
		pfmt(stderr, MM_ERROR,
			":62:invalid size %d\n", fssize);
		exit(RET_BAD_FSSIZE);
	}
	wtfs(fssize - 1, DEV_BSIZE, (char *)&sblock);
	/*
	 * collect and verify the sector and track info
	 */
	if (argc > 2)
		sblock.fs_nsect = atoi(argv[2]);
	else
		sblock.fs_nsect = nsect;
	if (argc > 3)
		sblock.fs_ntrak = atoi(argv[3]);
	else
		sblock.fs_ntrak = ntrack;
	if (sblock.fs_ntrak <= 0) {
		pfmt(stderr, MM_ERROR,
			":63:invalid ntrak %d\n", sblock.fs_ntrak);
		exit(RET_BAD_NTRAK);
	}
	if (sblock.fs_nsect <= 0) {
		pfmt(stderr, MM_ERROR,
			":64:invalid nsect %d\n", sblock.fs_nsect);
		exit(RET_BAD_NSECT);
	}
	sblock.fs_spc = sblock.fs_ntrak * sblock.fs_nsect;
	if (argc > 11) {
		sblock.fs_spc -= atoi(argv[11]);
		spc_flag = 1;
		apc_flag = 0;
	}
	if (apc_flag) {
		sblock.fs_spc -= apc;
		spc_flag = 1;
	}
	if (argc > 8)
		rps = atoi(argv[8]);
	if (rps <= 0) {
		pfmt(stderr, MM_ERROR,
			":65:invalid rotations per second %d\n", rps);
		exit(RET_BAD_RPS);
	}
	/* Now check for rotational delay argument */
	if (argc > 12) 
		/* if specified, use it */
		rot = atoi(argv[12]);
	if (rot <= -1) {	/* default by newfs and mkfs */
		rot = ROTDELAY;
		/*
		 * Best default depends on controller type.
		 * XXX - Controller should tell us (via some
		 * ioctl) what rotdelay it prefers.
		 */
	}
	else {
		if (rot >= (1000/rps)) {
			pfmt(stderr, MM_ERROR,
				":66:invalid rotational delay %d\n", rot);
			exit(RET_BAD_ROT);
		}
	}

	/*
	 * collect and verify the block and fragment sizes
	 */
	if (argc > 4)
		sblock.fs_bsize = atoi(argv[4]);
	else
		sblock.fs_bsize = bsize;;
	if ((sblock.fs_bsize <= NOFRAGBSIZE) && !specfrag)
		fragsize = bsize;
	if (argc > 5)
		sblock.fs_fsize = atoi(argv[5]);
	else
		sblock.fs_fsize = fragsize;
	if (!POWEROF2(sblock.fs_bsize)) {
		pfmt(stderr, MM_ERROR,
			":67:block size must be a power of 2, not %d\n",
		    		sblock.fs_bsize);
		exit(RET_BSIZE_POW2);
	}
	if (!POWEROF2(sblock.fs_fsize)) {
		pfmt(stderr, MM_ERROR,
			":68:fragment size must be a power of 2, not %d\n",
		    		sblock.fs_fsize);
		exit(RET_FSIZE_POW2);
	}
	if (sblock.fs_fsize < DEV_BSIZE) {
		pfmt(stderr, MM_ERROR,
			":69:fragment size %d is too small, minimum is %d\n",
				sblock.fs_fsize, DEV_BSIZE);
		exit(RET_FSIZE_SMALL);
	}
	if (sblock.fs_bsize < MINBSIZE) {
		pfmt(stderr, MM_ERROR,
			":70:block size %d is too small, minimum is %d\n",
				sblock.fs_bsize, MINBSIZE);
		exit(RET_BSIZE_SMALL);
	}
	if (sblock.fs_bsize > MAXBSIZE) {
		pfmt(stderr, MM_ERROR,
			":71:block size %d is too big, maximum is %d\n",
				sblock.fs_bsize, MAXBSIZE);
		exit(RET_BSIZE_BIG);
	}		
	if (sblock.fs_bsize < sblock.fs_fsize) {
		pfmt(stderr, MM_ERROR,
			":72:block size (%d) cannot be smaller than fragment size (%d)\n",
				sblock.fs_bsize, sblock.fs_fsize);
		exit(RET_BF_SIZE);
	}
	sblock.fs_bmask = ~(sblock.fs_bsize - 1);
	sblock.fs_fmask = ~(sblock.fs_fsize - 1);
	for (sblock.fs_bshift = 0, i = sblock.fs_bsize; i > 1; i >>= 1)
		sblock.fs_bshift++;
	for (sblock.fs_fshift = 0, i = sblock.fs_fsize; i > 1; i >>= 1)
		sblock.fs_fshift++;
	sblock.fs_frag = numfrags(&sblock, sblock.fs_bsize);
	for (sblock.fs_fragshift = 0, i = sblock.fs_frag; i > 1; i >>= 1)
		sblock.fs_fragshift++;
	if (sblock.fs_frag > MAXFRAG) {
		pfmt(stderr, MM_ERROR,
			":73:fragment size %d is too small, minimum with block size %d is %d\n",
				sblock.fs_fsize, sblock.fs_bsize,
				sblock.fs_bsize / MAXFRAG);
		exit(RET_FSIZE_SMALL);
	}
	sblock.fs_nindir = sblock.fs_bsize / sizeof(daddr_t);
	sblock.fs_inopb = sblock.fs_bsize / sizeof(struct dinode);
	sblock.fs_nspf = sblock.fs_fsize / DEV_BSIZE;
	for (sblock.fs_fsbtodb = 0, i = sblock.fs_nspf; i > 1; i >>= 1)
		sblock.fs_fsbtodb++;
	sblock.fs_sblkno =
	    roundup(howmany(BBSIZE + SBSIZE, sblock.fs_fsize), sblock.fs_frag);
	sblock.fs_cblkno = (daddr_t)(sblock.fs_sblkno +
	    roundup(howmany(SBSIZE, sblock.fs_fsize), sblock.fs_frag));
	sblock.fs_iblkno = sblock.fs_cblkno + sblock.fs_frag;
	sblock.fs_cgoffset = roundup(
	    howmany(sblock.fs_nsect, sblock.fs_fsize / DEV_BSIZE),
	    sblock.fs_frag);
	for (sblock.fs_cgmask = 0xffffffff, i = sblock.fs_ntrak; i > 1; i >>= 1)
		sblock.fs_cgmask <<= 1;
	if (!POWEROF2(sblock.fs_ntrak))
		sblock.fs_cgmask <<= 1;
	for (sblock.fs_cpc = NSPB(&sblock), i = sblock.fs_spc;
	     sblock.fs_cpc > 1 && (i & 1) == 0;
	     sblock.fs_cpc >>= 1, i >>= 1)
		/* void */;
	if (sblock.fs_cpc > MAXCPG) {
		pfmt(stderr, MM_ERROR,
			":74:maximum block size with nsect %d and ntrak %d is %d\n",
				sblock.fs_nsect, sblock.fs_ntrak,
				sblock.fs_bsize / (sblock.fs_cpc / MAXCPG));
		exit(RET_BAD_CPC);
	}
	/* 
	 * collect and verify the number of cylinders per group
	 */
	if (argc > 6) {
		sblock.fs_cpg = atoi(argv[6]);
		sblock.fs_fpg = (sblock.fs_cpg * sblock.fs_spc) / NSPF(&sblock);
	} else {
		if (cpg_flag) {
			sblock.fs_cpg = cgsize;
			sblock.fs_fpg = (sblock.fs_cpg * sblock.fs_spc) / NSPF(&sblock);
		}
		else {
			/* Code to compute default cylinders per group. 
			   KLUDGE. This should be much smarter.
			   what it does is merely shrink the default cpg
			   if it sees cylinders >= about 1M.
			   The purpose is to have a reasonable number 
			   of inodes on the system, and the number of
			   inodes per cylinder group is quite limited.
			   It SHOULD compute cpg based on number of inodes
			   desired as well as cylinder size !
			 */
			
			sblock.fs_cpg = DESCPG;
			if (sblock.fs_spc > 1536) /* about .75 M */
				sblock.fs_cpg = 3 * DESCPG / 4;	/* 12 */
			if (sblock.fs_spc > 3072) /* about 1.5 M */
				sblock.fs_cpg = DESCPG / 2; /* 8 */
			/* remaining code here is historical, 
			   except 'sblock.fs_cpg' used to be DESCPG */
			sblock.fs_cpg = MAX(sblock.fs_cpc, sblock.fs_cpg);
			sblock.fs_fpg = (sblock.fs_cpg * sblock.fs_spc) / NSPF(&sblock);
			while (sblock.fs_fpg / sblock.fs_frag > MAXBPG(&sblock) &&
		    	       sblock.fs_cpg > sblock.fs_cpc) {
				sblock.fs_cpg -= sblock.fs_cpc;
				sblock.fs_fpg =
			    		(sblock.fs_cpg * sblock.fs_spc) / NSPF(&sblock);
			}
		}
	}
	if (sblock.fs_cpg < 1) {
		pfmt(stderr, MM_ERROR,
			":75:cylinder groups must have at least 1 cylinder\n");
		exit(RET_CPG_ONE);
	}
	if (sblock.fs_cpg > MAXCPG) {
		pfmt(stderr, MM_ERROR,
			":76:cylinder groups are limited to %d cylinders\n",
				MAXCPG);
		exit(RET_CPG_MAX);
	}
	if (sblock.fs_cpg % sblock.fs_cpc != 0) {
		pfmt(stderr, MM_ERROR,
			":77:cylinder groups must have a multiple of %d cylinders\n",
				sblock.fs_cpc);
		exit(RET_CPG_MULT);
	}
	/*
	 * Now have size for file system and nsect and ntrak.
	 * Determine number of cylinders and blocks in the file system.
	 */
	sblock.fs_size = fssize = dbtofsb(&sblock, fssize);
	sblock.fs_ncyl = fssize * NSPF(&sblock) / sblock.fs_spc;
	if (fssize * NSPF(&sblock) > sblock.fs_ncyl * sblock.fs_spc) {
		sblock.fs_ncyl++;
		warn = 1;
	}
	if (sblock.fs_ncyl < 1) {
		pfmt(stderr, MM_ERROR,
			":78:file systems must have at least one cylinder\n");
		exit(RET_FS_ONE);
	}
	/*
	 * determine feasability/values of rotational layout tables
	 */
	if (sblock.fs_ntrak == 1) {
		sblock.fs_cpc = 0;
		goto next;
	}
	if (sblock.fs_spc * sblock.fs_cpc > MAXBPC * NSPB(&sblock) ||
	    sblock.fs_nsect > (1 << NBBY) * NSPB(&sblock)) {
		pfmt(stderr, MM_WARNING,
		    ":79:insufficient space in super block for\nrotational layout tables with nsect %d and ntrak %d.\n",
		    sblock.fs_nsect, sblock.fs_ntrak);
		pfmt(stderr, MM_NOSTD,
		    ":80:File system performance may be impaired.\n");
		sblock.fs_cpc = 0;
		goto next;
	}
	/*
	 * calculate the available blocks for each rotational position
	 */
	for (cylno = 0; cylno < MAXCPG; cylno++)
		for (rpos = 0; rpos < NRPOS; rpos++)
			sblock.fs_postbl[cylno][rpos] = -1;
	blk = sblock.fs_spc * sblock.fs_cpc / NSPF(&sblock);
	for (i = 0; i < blk; i += sblock.fs_frag)
		/* void */;
	for (i -= sblock.fs_frag; i >= 0; i -= sblock.fs_frag) {
		cylno = cbtocylno(&sblock, i);
		rpos = cbtorpos(&sblock, i);
		blk = i / sblock.fs_frag;
		if (sblock.fs_postbl[cylno][rpos] == -1)
			sblock.fs_rotbl[blk] = 0;
		else
			sblock.fs_rotbl[blk] =
			    sblock.fs_postbl[cylno][rpos] - blk;
		sblock.fs_postbl[cylno][rpos] = blk;
	}
next:
	/*
	 * Validate specified/determined cpg.
	 */
	if (sblock.fs_spc > MAXBPG(&sblock) * NSPB(&sblock)) {
		pfmt(stderr, MM_ERROR,
			":81:too many sectors per cylinder (%d sectors)\n",
				sblock.fs_spc);
		while(sblock.fs_spc > MAXBPG(&sblock) * NSPB(&sblock)) {
			sblock.fs_bsize <<= 1;
			if (sblock.fs_frag < MAXFRAG)
				sblock.fs_frag <<= 1;
			else
				sblock.fs_fsize <<= 1;
		}
		pfmt(stderr, MM_ERROR,
		    ":82:nsect %d, and ntrak %d, requires block size of %d,\n\tand fragment size of %d\n",
			sblock.fs_nsect, sblock.fs_ntrak, sblock.fs_bsize, sblock.fs_fsize);
		exit(RET_BAD_SPC);
	}
	if (sblock.fs_fpg > MAXBPG(&sblock) * sblock.fs_frag) {
		pfmt(stderr, MM_ERROR,
		    ":83:cylinder group too large (%d cylinders); ",
			sblock.fs_cpg);
		pfmt(stderr, MM_ERROR,
		    ":84:max: %d cylinders per group\n",
			MAXBPG(&sblock) * sblock.fs_frag /
			(sblock.fs_fpg / sblock.fs_cpg));
		exit(RET_CPG_LARGE);
	}
	sblock.fs_cgsize = fragroundup(&sblock,
	    sizeof(struct cg) + howmany(sblock.fs_fpg, NBBY));
	/*
	 * Compute/validate number of cylinder groups.
	 */
	sblock.fs_ncg = sblock.fs_ncyl / sblock.fs_cpg;
	if (sblock.fs_ncyl % sblock.fs_cpg)
		sblock.fs_ncg++;
	if ((sblock.fs_spc * sblock.fs_cpg) % NSPF(&sblock)) {
		pfmt(stderr, MM_ERROR,
		    ":85:nsect %d, ntrak %d, cpg %d is not tolerable\nas this would would have cyl groups whose size\nis not a multiple of %d; exiting!\n",
			sblock.fs_nsect, sblock.fs_ntrak,
			sblock.fs_cpg, sblock.fs_fsize);
		exit(RET_CPG_BAD);
	}
	/*
	 * Compute number of inode blocks per cylinder group.
	 * Start with one inode per NBPI bytes; adjust as necessary.
	 */
	/* Start with a reasonable default in case no value from user */
	inos = MAX(NBPI, sblock.fs_fsize);
	if (argc > 9) {
		i = atoi(argv[9]);
		if (i <= 0) {
			pfmt(stderr, MM_WARNING,
				":86:%s: invalid nbpi reset to %d\n",
					argv[9], inos);
		} else
			inos = i;
	}
	else if (nbpi_flag) {
		if (nbpi <= 0) 
			pfmt(stderr, MM_WARNING,
				":87:%d: invalid nbpi reset to %d\n",
					nbpi, inos);
		else
			inos = nbpi;
	}
	if (sblock.fs_size/BLK_TO_INODES > inos)
		inos = ckinos(inos);
	i = sblock.fs_iblkno + MAXIPG / INOPF(&sblock);
	inos = (fssize - sblock.fs_ncg * i) * sblock.fs_fsize / inos /
	    INOPB(&sblock);
	if (inos <= 0)
		inos = 1;
	sblock.fs_ipg = ((inos / sblock.fs_ncg) + 1) * INOPB(&sblock);
	if (sblock.fs_ipg > MAXIPG)
		sblock.fs_ipg = MAXIPG;
	sblock.fs_dblkno = sblock.fs_iblkno + sblock.fs_ipg / INOPF(&sblock);
	i = MIN(~sblock.fs_cgmask, sblock.fs_ncg - 1);
	if (cgdmin(&sblock, i) - cgbase(&sblock, i) >= sblock.fs_fpg) {
		pfmt(stderr, MM_ERROR,
		    ":88:inode blocks/cyl group (%d) >= data blocks (%d)\n",
			cgdmin(&sblock, i) - cgbase(&sblock, i) / sblock.fs_frag,
			sblock.fs_fpg / sblock.fs_frag);
		pfmt(stderr, MM_NOSTD,
		    ":89:number of cylinders per cylinder group must be increased\n");
		exit(RET_CPG_MORE);
	}
	j = sblock.fs_ncg - 1;
	if ((i = fssize - j * sblock.fs_fpg) < sblock.fs_fpg &&
	    cgdmin(&sblock, j) - cgbase(&sblock, j) > i) {
		pfmt(stderr, MM_WARNING,
		    ":90:inode blocks/cyl group (%d) >= data blocks (%d) in last\ncylinder group.",
			(cgdmin(&sblock, j) - cgbase(&sblock, j)) / sblock.fs_frag,
			i / sblock.fs_frag);
		pfmt(stderr, MM_NOSTD,
		    ":91: This implies %d sector(s) cannot be allocated.\n",
			i * NSPF(&sblock));
		sblock.fs_ncg--;
		sblock.fs_ncyl -= sblock.fs_ncyl % sblock.fs_cpg;
		sblock.fs_size = fssize = sblock.fs_ncyl * sblock.fs_spc /
		    NSPF(&sblock);
		warn = 0;
	}
	if (warn & !spc_flag) {
		pfmt(stderr, MM_WARNING,
		    ":92:%d sector(s) in last cylinder unallocated\n",
			sblock.fs_spc -
			(fssize * NSPF(&sblock) - (sblock.fs_ncyl - 1)
			 * sblock.fs_spc));
	}
	/*
	 * fill in remaining fields of the super block
	 */
	sblock.fs_csaddr = cgdmin(&sblock, 0);
	sblock.fs_cssize =
	    fragroundup(&sblock, sblock.fs_ncg * sizeof(struct csum));
	i = sblock.fs_bsize / sizeof(struct csum);
	sblock.fs_csmask = ~(i - 1);
	for (sblock.fs_csshift = 0; i > 1; i >>= 1)
		sblock.fs_csshift++;
	i = sizeof(struct fs) +
		howmany(sblock.fs_spc * sblock.fs_cpc, NSPB(&sblock));
	sblock.fs_sbsize = fragroundup(&sblock, i);
	fscs = (struct csum *)calloc(1, sblock.fs_cssize);
	sblock.fs_magic = SFS_MAGIC;
	sblock.fs_rotdelay = rot;
	/*
	 * If the fragment size equals the block size, fragmentation
   	 * can't occur, so no disk space should be reserved for it.
	 */	
	if (sblock.fs_fsize == sblock.fs_bsize) 
		minfree=0;
	sblock.fs_minfree = minfree;
	
	sblock.fs_maxcontig = MAXCONTIG;
	sblock.fs_maxbpg = MAXBLKPG(&sblock);
	sblock.fs_rps = rps;
	if (argc > 10)
		if (*argv[10] == 's')
			sblock.fs_optim = FS_OPTSPACE;
		else if (*argv[10] == 't')
			sblock.fs_optim = FS_OPTTIME;
		else {
			pfmt(stderr, MM_WARNING,
			    ":95:%s: invalid optimization preference reset to time\n",
				argv[10]);
			sblock.fs_optim = FS_OPTTIME;
		}
	else {
		if (opt  == 's')
			sblock.fs_optim = FS_OPTSPACE;
		else if (opt == 't')
			sblock.fs_optim = FS_OPTTIME;
		else {
			pfmt(stderr, MM_WARNING,
			    ":96:%c: invalid optimization preference reset to time\n",
				opt);
			sblock.fs_optim = FS_OPTTIME;
		}
	}
	sblock.fs_cgrotor = 0;
	sblock.fs_cstotal.cs_ndir = 0;
	sblock.fs_cstotal.cs_nbfree = 0;
	sblock.fs_cstotal.cs_nifree = 0;
	sblock.fs_cstotal.cs_nffree = 0;
	sblock.fs_fmod = 0;
	sblock.fs_ronly = 0;
	sblock.fs_time = utime;
	sblock.fs_state = FSOKAY - (long)sblock.fs_time;
	/*
	 * Dump out summary information about file system.
	 */
	pfmt(stdout, MM_NOSTD,
	    ":97:%s:\t%d sectors in %d cylinders of %d tracks, %d sectors\n",
		fsys, sblock.fs_size * NSPF(&sblock), sblock.fs_ncyl,
		sblock.fs_ntrak, sblock.fs_nsect);
	pfmt(stdout, MM_NOSTD,
	    ":98:\t%.1fMb in %d cyl groups (%d c/g, %.2fMb/g, %d i/g)\n",
		(float)sblock.fs_size * sblock.fs_fsize * 1e-6, sblock.fs_ncg,
		sblock.fs_cpg, (float)sblock.fs_fpg * sblock.fs_fsize * 1e-6,
		sblock.fs_ipg / NIPFILE);
	/*
	 * Now build the cylinders group blocks and
	 * then print out indices of cylinder groups.
	 */
	pfmt(stdout, MM_NOSTD,
		":99:super-block backups (for fsck -b#) at:");
	for (cylno = 0; cylno < sblock.fs_ncg; cylno++) {
		initcg(cylno);
		if (cylno % 10 == 0)
			printf("\n");
		printf(" %d,", fsbtodb(&sblock, cgsblock(&sblock, cylno)));
	}
	printf("\n");
	if (Nflag)
		exit(RET_OK);
	/*
	 * Clear the first 8192 bytes in case there are left over 
	 * information, such as a super-block from a S5 file system
	 */
	wtfs(BBLOCK, BBSIZE, (char *)zerobuf);

	/*
	 * Now construct the initial file system.
	 */
	fsinit();
	for (i = 0; i < sblock.fs_cssize; i += sblock.fs_bsize)
		wtfs(fsbtodb(&sblock, sblock.fs_csaddr + numfrags(&sblock, i)),
			sblock.fs_cssize - i < sblock.fs_bsize ?
			    sblock.fs_cssize - i : sblock.fs_bsize,
			((char *)fscs) + i);
	/* 
	 * Write out the duplicate super blocks
	 */
	for (cylno = 0; cylno < sblock.fs_ncg; cylno++)
		wtfs(fsbtodb(&sblock, cgsblock(&sblock, cylno)),
		    SBSIZE, (char *)&sblock);
	/*
         * Write out the super block last.
         */
	wtfs(SBLOCK, SBSIZE, (char *)&sblock);
	fsync(fso);
	close(fsi);
	close(fso);

#ifndef STANDALONE
	exit(RET_OK);
#endif
}

/*
 * Initialize a cylinder group.
 */
initcg(cylno)
	int cylno;
{
	daddr_t cbase, d, dlower, dupper, dmax;
	long i, j;
	register struct csum *cs;
	register long inoblks, blkno;

	/*
	 * Determine block bounds for cylinder group.
	 * Allow space for super block summary information in first
	 * cylinder group.
	 */
	cbase = cgbase(&sblock, cylno);
	dmax = cbase + sblock.fs_fpg;
	if (dmax > sblock.fs_size)
		dmax = sblock.fs_size;
	dlower = cgsblock(&sblock, cylno) - cbase;
	dupper = cgdmin(&sblock, cylno) - cbase;
	cs = fscs + cylno;
	acg.cg_time = utime;
	acg.cg_magic = CG_MAGIC;
	acg.cg_cgx = cylno;
	if (cylno == sblock.fs_ncg - 1)
		acg.cg_ncyl = sblock.fs_ncyl % sblock.fs_cpg;
	else
		acg.cg_ncyl = sblock.fs_cpg;
	acg.cg_niblk = sblock.fs_ipg;
	acg.cg_ndblk = dmax - cbase;
	acg.cg_cs.cs_ndir = 0;
	acg.cg_cs.cs_nffree = 0;
	acg.cg_cs.cs_nbfree = 0;
	acg.cg_cs.cs_nifree = 0;
	acg.cg_rotor = 0;
	acg.cg_frotor = 0;
	acg.cg_irotor = 0;
	for (i = 0; i < sblock.fs_frag; i++) {
		acg.cg_frsum[i] = 0;
	}
	/* 
	 * This loop goes one inode block at a time.
	 * One inode per file is marked free, and each
	 * "alternate" inode is marked allocated.
	 * The cylinder group free inode count does not
	 * include the number of alternate inodes.
	 */
	for (i = 0; i < sblock.fs_ipg; ) {
		for (j = INOPB(&sblock); j > 0; j -= NIPFILE) {
			clrbit(acg.cg_iused, i);
			setbit(acg.cg_iused, i + 1);
			i += NIPFILE;
		}
		acg.cg_cs.cs_nifree += (INOPB(&sblock)) / NIPFILE;
	}
	while (i < MAXIPG) {
		clrbit(acg.cg_iused, i);
		i++;
	}
	if (cylno == 0)
		for (i = 0; i < (int)SFSROOTINO; i += NIPFILE) {
			setbit(acg.cg_iused, i);
			acg.cg_cs.cs_nifree--;
		}
	/* zero out each inode block for this cylinder group */
	inoblks = sblock.fs_ipg / sblock.fs_inopb;
	blkno = fsbtodb(&sblock, cgimin(&sblock, cylno));
	for (i = 0; i < inoblks; i++, blkno += sblock.fs_bsize / DEV_BSIZE)
		wtfs(blkno, sblock.fs_bsize, (char *)zerobuf);
	for (i = 0; i < MAXCPG; i++) {
		acg.cg_btot[i] = 0;
		for (j = 0; j < NRPOS; j++)
			acg.cg_b[i][j] = 0;
	}
	if (cylno == 0) {
		/*
		 * reserve space for summary info and Boot block
		 */
		dupper += howmany(sblock.fs_cssize, sblock.fs_fsize);
		for (d = 0; d < dlower; d += sblock.fs_frag)
			clrblock(&sblock, acg.cg_free, d/sblock.fs_frag);
	} else {
		for (d = 0; d < dlower; d += sblock.fs_frag) {
			setblock(&sblock, acg.cg_free, d/sblock.fs_frag);
			acg.cg_cs.cs_nbfree++;
			acg.cg_btot[cbtocylno(&sblock, d)]++;
			acg.cg_b[cbtocylno(&sblock, d)][cbtorpos(&sblock, d)]++;
		}
		sblock.fs_dsize += dlower;
	}
	sblock.fs_dsize += acg.cg_ndblk - dupper;
	for (; d < dupper; d += sblock.fs_frag)
		clrblock(&sblock, acg.cg_free, d/sblock.fs_frag);
	if (d > dupper) {
		acg.cg_frsum[d - dupper]++;
		for (i = d - 1; i >= dupper; i--) {
			setbit(acg.cg_free, i);
			acg.cg_cs.cs_nffree++;
		}
	}
	while ((d + sblock.fs_frag) <= dmax - cbase) {
		setblock(&sblock, acg.cg_free, d/sblock.fs_frag);
		acg.cg_cs.cs_nbfree++;
		acg.cg_btot[cbtocylno(&sblock, d)]++;
		acg.cg_b[cbtocylno(&sblock, d)][cbtorpos(&sblock, d)]++;
		d += sblock.fs_frag;
	}
	if (d < dmax - cbase) {
		acg.cg_frsum[dmax - cbase - d]++;
		for (; d < dmax - cbase; d++) {
			setbit(acg.cg_free, d);
			acg.cg_cs.cs_nffree++;
		}
		for (; d % sblock.fs_frag != 0; d++)
			clrbit(acg.cg_free, d);
	}
	for (d /= sblock.fs_frag; d < MAXBPG(&sblock); d ++)
		clrblock(&sblock, acg.cg_free, d);
	sblock.fs_cstotal.cs_ndir += acg.cg_cs.cs_ndir;
	sblock.fs_cstotal.cs_nffree += acg.cg_cs.cs_nffree;
	sblock.fs_cstotal.cs_nbfree += acg.cg_cs.cs_nbfree;
	sblock.fs_cstotal.cs_nifree += acg.cg_cs.cs_nifree;
	*cs = acg.cg_cs;
	wtfs(fsbtodb(&sblock, cgtod(&sblock, cylno)),
		sblock.fs_bsize, (char *)&acg);
}

/*
 * initialize the file system
 */
struct inode node;
union i_secure node_isec;

#define PREDEFDIR 3
struct direct root_dir[] = {
	{ SFSROOTINO, sizeof(struct direct), 1, "." },
	{ SFSROOTINO, sizeof(struct direct), 2, ".." },
	{ LOSTFOUNDINO, sizeof(struct direct), 10, "lost+found" },
};
struct direct lost_found_dir[] = {
	{ LOSTFOUNDINO, sizeof(struct direct), 1, "." },
	{ SFSROOTINO, sizeof(struct direct), 2, ".." },
	{ 0, DIRBLKSIZ, 0, 0 },
};
char buf[MAXBSIZE];

fsinit()
{
	int i;

	/*
	 * initialize the node
	 */
	node.is_union = &node_isec;
	node.i_atime = utime;
	node.i_mtime = utime;
	node.i_ctime = utime;

	/*
	 * tag level of special file to root and lost+found inodes
	 */
	(void)lvlfile(fsys, MAC_GET, &node.i_lid);

	/*
	 * create the lost+found directory
	 */
	(void)makedir(lost_found_dir, 2);
	for (i = DIRBLKSIZ; i < sblock.fs_bsize; i += DIRBLKSIZ) {
		memcpy(&buf[i], &lost_found_dir[2], DIRSIZ(&lost_found_dir[2]));
	}
	/*
	 * If specified, make the root inode a multi-Level Directory (MLD).
	 */
	if (Mflag)				/* MLD */
		node.i_sflags = ISD_MLD;	/* MLD */
	node.i_number = LOSTFOUNDINO;
	node.i_smode = node.i_mode = IFDIR | 0777;
	node.i_uid = getuid();
	node.i_gid = getgid();
	node.i_eftflag = EFT_MAGIC;
	node.i_nlink = 2;
	node.i_size = sblock.fs_bsize;
	node.i_db[0] = alloc(node.i_size, node.i_mode);
	node.i_blocks = btodb(fragroundup(&sblock, node.i_size));
	wtfs(fsbtodb(&sblock, node.i_db[0]), node.i_size, buf);
	iput(&node);
	/*
	 * create the root directory
	 */
	node.i_number = SFSROOTINO;
	node.i_mode = node.i_smode = IFDIR | UMASK;
	node.i_uid = getuid();
	node.i_gid = getgid();
	node.i_eftflag = EFT_MAGIC;
	node.i_nlink = PREDEFDIR;
	node.i_size = makedir(root_dir, PREDEFDIR);
	node.i_db[0] = alloc(sblock.fs_fsize, node.i_mode);
	node.i_blocks = btodb(fragroundup(&sblock, node.i_size));
	wtfs(fsbtodb(&sblock, node.i_db[0]), sblock.fs_fsize, buf);
	iput(&node);
}

/*
 * construct a set of directory entries in "buf".
 * return size of directory.
 */
makedir(protodir, entries)
	register struct direct *protodir;
	int entries;
{
	char *cp;
	int i, spcleft;

	spcleft = DIRBLKSIZ;
	for (cp = buf, i = 0; i < entries - 1; i++) {
		protodir[i].d_reclen = DIRSIZ(&protodir[i]);
		memcpy(cp, &protodir[i], protodir[i].d_reclen);
		cp += protodir[i].d_reclen;
		spcleft -= protodir[i].d_reclen;
	}
	protodir[i].d_reclen = spcleft;
	memcpy(cp, &protodir[i], DIRSIZ(&protodir[i]));
	return (DIRBLKSIZ);
}

/*
 * allocate a block or frag
 */
daddr_t
alloc(size, mode)
	int size;
	int mode;
{
	int i, frag;
	daddr_t d;

	rdfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
	    (char *)&acg);
	if (acg.cg_magic != CG_MAGIC) {
		pfmt(stderr, MM_ERROR,
			":100:cg 0: bad magic number\n");
		return (0);
	}
	if (acg.cg_cs.cs_nbfree == 0) {
		pfmt(stderr, MM_ERROR,
			":101:first cylinder group ran out of space\n");
		return (0);
	}
	for (d = 0; d < acg.cg_ndblk; d += sblock.fs_frag)
		if (isblock(&sblock, acg.cg_free, d / sblock.fs_frag))
			goto goth;
	pfmt(stderr, MM_ERROR,
		":102:internal error: cannot find block in cyl 0\n");
	return (0);
goth:
	clrblock(&sblock, acg.cg_free, d / sblock.fs_frag);
	acg.cg_cs.cs_nbfree--;
	sblock.fs_cstotal.cs_nbfree--;
	fscs[0].cs_nbfree--;
	if (mode & IFDIR) {
		acg.cg_cs.cs_ndir++;
		sblock.fs_cstotal.cs_ndir++;
		fscs[0].cs_ndir++;
	}
	acg.cg_btot[cbtocylno(&sblock, d)]--;
	acg.cg_b[cbtocylno(&sblock, d)][cbtorpos(&sblock, d)]--;
	if (size != sblock.fs_bsize) {
		frag = howmany(size, sblock.fs_fsize);
		fscs[0].cs_nffree += sblock.fs_frag - frag;
		sblock.fs_cstotal.cs_nffree += sblock.fs_frag - frag;
		acg.cg_cs.cs_nffree += sblock.fs_frag - frag;
		acg.cg_frsum[sblock.fs_frag - frag]++;
		for (i = frag; i < sblock.fs_frag; i++)
			setbit(acg.cg_free, d + i);
	}
	wtfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
	    (char *)&acg);
	return (d);
}

/*
 * Allocate an inode on the disk
 */
iput(ip)
	register struct inode *ip;
{
	struct dinode buf[MAXINOPB];
	daddr_t d;
	int c;

	c = itog(&sblock, (int)ip->i_number);
	rdfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
	    (char *)&acg);
		if (acg.cg_magic != CG_MAGIC) {
			pfmt(stderr, MM_ERROR,
				":100:cg 0: bad magic number\n");
			exit(RET_BAD_MAGIC);
		}
		acg.cg_cs.cs_nifree--;
		setbit(acg.cg_iused, ip->i_number);
		wtfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
		    (char *)&acg);
		sblock.fs_cstotal.cs_nifree--;
		fscs[0].cs_nifree--;
		if((int)ip->i_number >= sblock.fs_ipg * sblock.fs_ncg) {
			pfmt(stderr, MM_ERROR,
				":103:inode value out of range (%d).\n",
			    		ip->i_number);
			exit(RET_INODE_RANGE);
		}
		d = fsbtodb(&sblock, itod(&sblock, (int)ip->i_number));
		rdfs(d, sblock.fs_bsize, buf);
		buf[itoo(&sblock, (int)ip->i_number)].di_ic = ip->i_ic;
		/*
		 * Set the alternate inode on the disk, also.
		 */
		buf[itoo(&sblock, (int)ip->i_number + 1)].di_ic = ip->is_ic;
		wtfs(d, sblock.fs_bsize, buf);
	}

	/*
	 * read a block from the file system
	 */
	rdfs(bno, size, bf)
		daddr_t bno;
		int size;
		char *bf;
	{
		int n;

		if (lseek(fsi, bno * DEV_BSIZE, 0) < 0) {
			pfmt(stderr, MM_ERROR,
				":46:seek error: %ld\n", bno);
			pfmt(stderr, MM_ERROR,
				":104:rdfs: %s\n", strerror(errno));
			exit(RET_ERR_SEEK);
		}
		n = read(fsi, bf, size);
		if(n != size) {
			pfmt(stderr, MM_ERROR,
				":44:read error: %ld\n", bno);
			pfmt(stderr, MM_ERROR,
				":104:rdfs: %s\n", strerror(errno));
			exit(RET_ERR_READ);
		}
	}

	/*
	 * write a block to the file system
	 */
void wtfs(bno, size, bf)
		daddr_t bno;
		int size;
		char *bf;
	{
		int n;


		if (Nflag)
			return;
		if (lseek(fso, bno * DEV_BSIZE, 0) < 0) {
			pfmt(stderr, MM_ERROR,
				":46:seek error: %ld\n", bno);
			pfmt(stderr, MM_ERROR,
				":105:wtfs: %s\n", strerror(errno));
			exit(RET_ERR_SEEK);
		}
		n = write(fso, bf, size);
		if(n != size) {
			pfmt(stderr, MM_ERROR,
				":45:write error: %ld\n", bno);
			pfmt(stderr, MM_ERROR,
				":105:wtfs: %s\n", strerror(errno));
			exit(RET_ERR_WRITE);
		}
	}

	/*
	 * check if a block is available
	 */
	isblock(fs, cp, h)
		struct fs *fs;
		unsigned char *cp;
		int h;
	{
		unsigned char mask;

		switch (fs->fs_frag) {
		case 8:
			return (cp[h] == 0xff);
		case 4:
			mask = 0x0f << ((h & 0x1) << 2);
			return ((cp[h >> 1] & mask) == mask);
		case 2:
			mask = 0x03 << ((h & 0x3) << 1);
			return ((cp[h >> 2] & mask) == mask);
		case 1:
			mask = 0x01 << (h & 0x7);
			return ((cp[h >> 3] & mask) == mask);
		default:
	#ifdef STANDALONE
			pfmt(stdout, MM_ERROR,
				":106:isblock bad fs_frag %d\n", fs->fs_frag);
	#else            
			pfmt(stderr, MM_ERROR,
				":106:isblock bad fs_frag %d\n", fs->fs_frag);
	#endif
			return;
		}
	}

	/*
	 * take a block out of the map
	 */
void clrblock(fs, cp, h)
		struct fs *fs;
		unsigned char *cp;
		int h;
	{
		switch ((fs)->fs_frag) {
		case 8:
			cp[h] = 0;
			return;
		case 4:
			cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
			return;
		case 2:  
			cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
			return;
		case 1:
			cp[h >> 3] &= ~(0x01 << (h & 0x7));
			return;
		default:
	#ifdef STANDALONE
			pfmt(stdout, MM_ERROR,
				":107:clrblock bad fs_frag %d\n", fs->fs_frag);
	#else            
			pfmt(stderr, MM_ERROR,
				":107:clrblock bad fs_frag %d\n", fs->fs_frag);
	#endif
			return;
		}
	}

	/*
	 * put a block into the map
	 */
void setblock(fs, cp, h)
		struct fs *fs;
		unsigned char *cp;
		int h;
	{
		switch (fs->fs_frag) {
		case 8:
			cp[h] = 0xff;
			return;
		case 4:
			cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
			return;
		case 2:
			cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
			return;
		case 1:  
			cp[h >> 3] |= (0x01 << (h & 0x7));
			return;
		default:
	#ifdef STANDALONE
			pfmt(stdout, MM_ERROR,
				":108:setblock bad fs_frag %d\n", fs->fs_frag);
	#else            
			pfmt(stderr, MM_ERROR,
				":108:setblock bad fs_frag %d\n", fs->fs_frag);
	#endif
			return;
		}
	}

void
usage()
{
	(void)pfmt(stderr, MM_ACTION,
		":109:Usage: %s [-F %s] [-V] [-m] [-o options] special size\n",
			myname, fstype);
	(void)pfmt(stderr, MM_NOSTD,
		":110:options:\nnsect=xx,ntrack=xx,bsize=xx,fragsize=xx,cgsize=xx\n");
	(void)pfmt(stderr, MM_NOSTD,
		":111:rps=xx,nbpi=xx,opt=x,apc=xx,gap=x\n");
	(void)pfmt(stderr, MM_NOSTD,
		":112:M=make root of file system a Multi-level directory\n");
	(void)pfmt(stderr, MM_NOSTD,
	    ":113:C=3.2 compatability; L=ignore 3.2 compatability\n");
        exit(RET_USAGE);
}

/* Compute nbpi from existing file system */
/* uses global sblock */
int	recompute_nbpi()
{
	int	i;
	int	inos;
	int	nbpi;
	
	i = sblock.fs_iblkno + MAXIPG / INOPF(&sblock);
		
	
	/* Reversing existing comp.
	   This is roughly number of inodes in file system.
	   Various factors ensure no overflow.
	   Also, know sblock.fs_ipg is integral multiple of INOPB(&sblock)
	*/
	
	inos = (sblock.fs_ipg / INOPB(&sblock) -1)
		* sblock.fs_ncg * INOPB (&sblock);
		
	nbpi = (sblock.fs_size - sblock.fs_ncg * i)
		* sblock.fs_fsize / inos ;

	return (nbpi);
}

void
dump_fs (fsys, fsi)
        char    *fsys;
        int     fsi;
{
        int nbpi;
 
        memset ((char *)&sblock, 0, sizeof (sblock));
        rdfs (SBLOCK, SBSIZE, (char *)&sblock);
 
        if (sblock.fs_magic != SFS_MAGIC)
                (void)pfmt(stderr, MM_WARNING,
			":114:[not currently a valid file system]\n");
 
	nbpi = recompute_nbpi();
        (void) printf ("mkfs -F sfs -o ");
        (void) printf ("nsect=%d,ntrack=%d,", sblock.fs_nsect, sblock.fs_ntrak);
        (void) printf ("bsize=%d,fragsize=%d,cgsize=%d,",
            sblock.fs_bsize, sblock.fs_fsize, sblock.fs_cpg);
        (void) printf ("rps=%d,nbpi=%d,opt=%c,apc=%d,gap=%d,L ",
            sblock.fs_rps, nbpi,(sblock.fs_optim == FS_OPTSPACE) ? 's' : 't',
            (sblock.fs_ntrak * sblock.fs_nsect) - sblock.fs_spc, sblock.fs_rotdelay);
        (void) printf ("%s %d\n", fsys,
            fsbtodb(&sblock, sblock.fs_size));
 
        memset ((char *)&sblock, 0, sizeof (sblock));
}

/* number ************************************************************* */
/*                                                                      */
/* Convert a numeric arg to binary                                      */
/*                                                                      */
/* Arg:         big - maximum valid input number                        */
/* Global arg:  string - pointer to command arg                         */
/*                                                                      */
/* Valid forms: 123 | 123k | 123*123 | 123x123				*/
/*                                                                      */
/* Return:      converted number                                        */
/*                                                                      */
/* ******************************************************************** */

unsigned int number(big)
long big;
{
        register char *cs;
        long n;
        long cut = BIG / 10;    /* limit to avoid overflow */

        cs = string;
        n = 0;
        while ((*cs >= '0') && (*cs <= '9') && (n <= cut))
        {
                n = n*10 + *cs++ - '0';
        }
        for (;;)
        {
                switch (*cs++)
                {

                case 'k':
                        n *= 1024;
                        continue;
 
                case '*':
                case 'x':
                        string = cs;
                        n *= number(BIG);
 
                /* Fall into exit test, recursion has read rest of string */
                /* End of string, check for a valid number */
 
		case ',':
                case '\0':
                        if ((n > big) || (n < 0))
                        {
                                pfmt(stderr, MM_ERROR,
					":115:argument out of range: \"%lu\"\n", n);
                                exit(RET_ARG_RANGE);
                        }
			cs--;
			string = cs;
                        return(n);
 
                default:
                        pfmt(stderr, MM_ERROR,
				":116:bad numeric arg: \"%s\"\n", string);
                        exit(RET_ARG_BAD);

                }
        } /* never gets here */
}

/* match ************************************************************** */
/*                                                                      */
/* Compare two text strings for equality                                */
/*                                                                      */
/* Arg:         s - pointer to string to match with a command arg       */
/* Global arg:  string - pointer to command arg                         */
/*                                                                      */
/* Return:      1 if match, 0 if no match                               */
/*              If match, also reset `string' to point to the text      */
/*              that follows the matching text.                         */
/*                                                                      */
/* ******************************************************************** */
 
int match(s)
char *s;
{
        register char *cs;
 
        cs = string;
        while (*cs++ == *s)
        {
                if (*s++ == '\0')
                {
                        goto true;
                }
        }
        if (*s != '\0')
        {
                return(0);
        }
 
true:
        cs--;
        string = cs;
        return(1);
}
/*************************************************************************
 * The input file system size is in blocks (512 bytes). ufs/mkfs converts
 * this to 1024 byte blocks. The most inodes a pre-SVR4 system can safely
 * handle is 65,536 (unsigned 2**16). This command defaults to a "number
 * of bytes per inodes" (nbpi) of 2048. This means that a file system
 * size of about 270,000 (512 byte) blocks will overflow inodes=65,536.
 *
 * The way the arithmetic works out, dividing sblock.fs_size (which
 * is fs size in 1024 byte blocks) by BLK_TO_INODES (64) gives an
 * approximate nbpi that will allow the file system to have < 65,536
 * inodes. This is a conservative estimate since the TOTAL fs size
 * includes superblocks, and blocks to hold the inodes themselves.
 *************************************************************************/
ckinos(ins)
int ins;
{
	char	in[81];
        FILE	*ttyp;
	int	inos = 0;

	if (Cflag)	/* if -o C option avoid prompting user */
		return (sblock.fs_size/BLK_TO_INODES);

	if (NOCOMPAT || Lflag)	/* if -o L option avoid prompting user */
		return (ins);
		
        if ((ttyp = fopen("/dev/tty", "w")) == (FILE *) NULL) {
		pfmt(stderr, MM_ERROR, ":117:cannot open /dev/tty\n");
                exit(RET_TTY_OPEN);
	}
        pfmt(ttyp, MM_WARNING, ":118:\nThis file system is able to support more\nthan 32,768 files.");
	pfmt(ttyp, MM_NOSTD, ":119:Some older applications (written for UNIX System V\nRelease 3.2 or before) may not work correctly on such a file system,\neven if fewer than 32,768 files are actually present.");
	pfmt(ttyp, MM_NOSTD, ":120:If you wish to run such applications (without recompiling them),\nyou should restrict the maximum number of files that may be created\nto fewer than 32,768.\n\n");

        while (inos == 0) {
		pfmt(ttyp, MM_NOSTD, ":121:Your choices are:\n\n");
		pfmt(ttyp, MM_NOSTD, ":122:1. Restrict this file system to fewer than 32,768 files.\n");	
		pfmt(ttyp, MM_NOSTD, ":123:2. Allow this file system to contain more than 32,768 files\n   (not compatible with some older applications).\n\n");
		pfmt(ttyp, MM_NOSTD, ":124:Press '1' or '2' followed by 'ENTER':\n");
		if (fgets(in, 81, stdin) == NULL) {
			pfmt(ttyp, MM_NOSTD, ":125:No input received; assuming '1'\n");
			in[0] = '\n';
		}
		switch (in[0]) {
			case '1':
			case '\n':
				inos = sblock.fs_size/BLK_TO_INODES;
				break;
			case '2':
				inos = ins;
				break;
 			default: 
				pfmt(ttyp, MM_NOSTD, ":126:Input not understood. Try again.\n\n");
		}
        }
	return inos;
}
