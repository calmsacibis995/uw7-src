#ident	"@(#)dosfs.cmds:fsck.c	1.10.2.3"
#ident	"$Header$"

#include <stdio.h>
#include <sys/param.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pfmt.h>
#include <locale.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/fs/bootsect.h>
#include <sys/fs/bpb.h>
#include <sys/fs/direntry.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/fdisk.h>
#include <nl_types.h>
#include <langinfo.h>
#include <regex.h>
#include "fsck.h"

extern	char *strrchr();

static void	find_extdosfs(char *, long *);
static void	get_bootblk(char *, long);
static void	checkdosfs(void);
static int	seek_read(off_t, char *, size_t);
static int	get_buffaddr(off_t, char **, size_t);
static void	check_fats(void);
static void	synch_fats(void);
static void	get_fatinfo(void);
static void	mmap_fs(off_t);
static u_long	getfatentry(u_long, u_char *);
static u_long	getnextcluster(u_long);
static void	updatefats(u_long, u_short);
static void	putfatentry(u_long, u_char *, u_short);
static void	searchrootdir(void);
static void	checkdirents(u_long, u_long, long, size_t);
static void	searchdir(u_long, u_long);
static u_long	bytes_alloc(u_long clustern);
static void	check_alias(u_long, u_long, dosdirent_t *);
static void	check_dir(u_long, u_long, dosdirent_t *);
static void	check_file(dosdirent_t *);
static void	chk_startcluster(dosdirent_t *);
static void	chk_defatentries(dosdirent_t *);
static boolean_t	isfixwanted(void);
static void	fsreport(void);
static char	*unrawname(char *cp);
static char	*rawname(char *cp);

static void	check4multiref(void);
static void	chk_de4multiref(dosdirent_t *);
static void	add_dir2list(dosdirent_t *, clustermap_t *);
static void	del_dentry(clustermap_t *);
static void	markit_del(dosdirent_t *);
static void	update_clustermap(dosdirent_t *);

static void	pass1(void);
static void	pass2(void);
static void	(*pfunc)(u_long, u_long, dosdirent_t *);
static void	checkentry(u_long, u_long, dosdirent_t *);
static void	checkfstate(u_long, u_long, dosdirent_t *);
static void	setfsokay(u_long, u_long, dosdirent_t *);
static void	search4multiref(u_long, u_long, dosdirent_t *);

static struct fs_global	dosfs, *fsp;
static struct fat		*fattab;

static char	*myname;
static char	fstype[]=FSTYPE;
static char	*Usage = ":372:Usage:\n%s [-F %s] [-m] [-y | -n] special\n";
static int	fd;			/* file descriptor */
static int	fd_blk;			/* file descriptor for block device */
static char	*special = NULL;	/* name of the special device */
static char	*blk_special = NULL;	/* name of the special block device */
static caddr_t	fs_map;			/* FS map address */
static size_t	fs_size;		/* FS size in bytes */

static char		mflag = 0;
static char		yflag = 0;
static char		nflag = 0;
static boolean_t	fs_modified  = B_FALSE;
static boolean_t	fat_modified = B_FALSE;
static boolean_t	fs_needpass2 = B_FALSE;

static char *Months[] = {
	"   ",
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

regex_t yesre, nore;

main(int argc, char **argv)
{
	int		cc;
	int		err;
	int		errflg = 0;
	char		string[64];
	struct stat	statarea;
	extern int	optind;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxfsck");
	myname = strrchr(argv[0],'/');
	myname = (myname != 0)? myname+1: argv[0];
	sprintf(string, "UX:%s %s", fstype, myname);
	(void)setlabel(string);

	while ((cc = getopt(argc, argv, "mny")) != -1)
		switch (cc) {
		case 'm':
			if (nflag || yflag)
				errflg++;
			else
				mflag++;
			break;
		case 'n':
			if (mflag || yflag)
				errflg++;
			else
				nflag++;
			break;
		case 'y':
			if (mflag || nflag)
				errflg++;
			else
				yflag++;
			break;
		case '?':
			errflg++;
			break;
		}

	if (errflg || (argc - optind) < 1) {
		pfmt(stderr, MM_ACTION, Usage, myname, fstype);
		exit (31+8);
	}

	special= argv[optind];

	if (special == NULL) {
		pfmt(stderr, MM_ACTION, Usage, myname, fstype);
		exit(31+8);
	}

	if(stat(special, &statarea) < 0) {
		pfmt(stderr, MM_ERROR, ":2:%s: can not stat\n", special);
		exit(31+8);
	}

	if((statarea.st_mode & S_IFMT) == S_IFBLK) {
		if ((statarea.st_flags & _S_ISMOUNTED) && !nflag) {
			pfmt(stderr, MM_ERROR, ":3:%s: mounted file system\n",
			     special);
			exit(31+2);
		}
		blk_special = special;
	}
	else if((statarea.st_mode & S_IFMT) == S_IFCHR) {
		if ((statarea.st_flags & _S_ISMOUNTED) && !nflag) {
		  pfmt(stderr, MM_ERROR,
		    ":4:%s file system is mounted as a block device, ignored\n",
		    special);
		  exit(31+3);
		}
		blk_special = unrawname(special);
	}
	else 
	{
		pfmt(stderr, MM_ERROR,
		     ":5:%s is not a block or character device\n",
		     special);
		exit(31+8);
	}

	fd = open(special, O_RDWR|O_SYNC);

	if (strcmp(special, blk_special) != 0)
		fd_blk = open(blk_special, O_RDWR|O_SYNC);
	else
		fd_blk = fd;

	if (fd < 0)
	{
		pfmt(stderr, MM_ERROR,
		     ":6:can not open special file %s\n", special);
		pfmt(stderr, MM_NOGET|MM_ERROR, "%s\n", strerror(errno));
		exit(31+8);
	}

	/* internalization setup */
        err = regcomp(&yesre, nl_langinfo(YESEXPR), REG_EXTENDED | REG_NOSUB);
        if (err != 0) {
                char buf[BUFSIZ];

                regerror(err, &yesre, buf, BUFSIZ);
                pfmt(stderr, MM_ERROR, "uxcore.abi:1234:RE failure: %s\n", buf);
                exit(2);
        }
        err = regcomp(&nore, nl_langinfo(NOEXPR), REG_EXTENDED | REG_NOSUB);
        if (err != 0) {
                char buf[BUFSIZ];

                regerror(err, &nore, buf, BUFSIZ);
                pfmt(stderr, MM_ERROR, "uxcore.abi:1234:RE failure: %s\n", buf);
                exit(2);
        }

	checkdosfs();
	exit (0);
}

static void
checkdosfs(void)
{
	int	error;
	size_t	nitems;
	char	bootblock[512];
	union	bootsector       *bsp;
	struct	byte_bpb50       *b50;
	long	dos_offset = 0;
	u_short	jump_inst;


	/*
	 * The DOS boot block can be found in the first block of the
	 * boot partition. This is true iff:
	 *	- the DOS FS is in a floppy diskette
	 *		or 
	 *	- the DOS FS is a PRIMARY DOS partition in the hard disk
	 *
	 * If the DOS file system is an EXTENDED DOS partition in the
	 * hardisk, then the first block contains the relevant information
	 * that can be used to locate the actual begining of the DOS
	 * file system.
	 * 
	 */

	get_bootblk(bootblock, dos_offset);

	bsp = (union bootsector *)bootblock;

	if (*(bsp->bs50.bsJump) == 0) {
		/*
		 * This may be an extended DOS partition. Find out where
		 * DOS file system really starts!
		 */
		find_extdosfs(bootblock, &dos_offset);
		if (dos_offset <= 0) {
			pfmt(stderr,MM_ERROR, ":7:%s is not a %s file system\n",
				special, fstype);
			exit(1);
		}

		get_bootblk(bootblock, dos_offset);

		bsp = (union bootsector *)bootblock;
	}

	/*
	 * Accepted DOS file system formats must pass the following test:
	 *
	 * - The first byte of the jump instruction must be either
	 *   0xe9 or 0xeb (or 0x69 to support older Floppy disk formats).
	 *
	 */

	jump_inst = getushort(bsp->bs50.bsJump) & 0x00ff;

	if (jump_inst != 0xe9 && jump_inst != 0xeb && jump_inst != 0x69) {
		pfmt(stderr,MM_ERROR, ":7:%s is not a %s file system\n",
			special, fstype);
		exit(1);
	}

	b50 = (struct byte_bpb50 *)bsp->bs50.bsBPB;

	if (!mflag) {
		pfmt(stdout, MM_NOSTD, ":394:\n");
		pfmt(stdout, MM_NOSTD, ":8:Checking %s:\n", special);
	}

	fsp = &dosfs;

	fsp->fs_BytesPerSec  = getushort(b50->bpbBytesPerSec);
	fsp->fs_SectPerClust = b50->bpbSecPerClust;
	fsp->fs_ResSectors   = getushort(b50->bpbResSectors);
	fsp->fs_FATs         = b50->bpbFATs;
	fsp->fs_RootDirEnts  = getushort(b50->bpbRootDirEnts);
	fsp->fs_Sectors      = getushort(b50->bpbSectors);
	fsp->fs_Media        = b50->bpbMedia;
	fsp->fs_FATsecs      = getushort(b50->bpbFATsecs);
	fsp->fs_fatblk       = (long)fsp->fs_ResSectors + dos_offset;
	fsp->fs_rootdirblk   = fsp->fs_fatblk +
			       (fsp->fs_FATs * fsp->fs_FATsecs);
	fsp->fs_rootdirsize  = (fsp->fs_RootDirEnts * sizeof (dosdirent_t)) /
			       fsp->fs_BytesPerSec;
	fsp->fs_firstcluster = fsp->fs_rootdirblk + fsp->fs_rootdirsize;

	fsp->fs_volumelable[0] = '\0';

	if (fsp->fs_Sectors <= 0)
		fsp->fs_HugeSectors  = getulong(b50->bpbHugeSectors);
	else
		fsp->fs_HugeSectors  = fsp->fs_Sectors;

	if (fsp->fs_HugeSectors <= 0)
		exit (1);

	fsp->fs_nclusters = (long)(fsp->fs_HugeSectors + dos_offset -
			           fsp->fs_firstcluster) /
			    (long)(fsp->fs_SectPerClust);

	fsp->fs_maxcluster   = fsp->fs_nclusters + 1;

	/*
	 * set up a cluster byte map to be used for cross referencing checking
	 */
	nitems = (size_t)(fsp->fs_nclusters + FIRST_CLUSTER);
	if ((fsp->fs_clustermap =
		(clustermap_t *)calloc(nitems, sizeof(clustermap_t))) == NULL) {
		pfmt(stderr, MM_ERROR, ":373:calloc failed\n");
		exit (1);
	}

	/*
	 * mmap the entire file system
	 */
	mmap_fs(dos_offset * 512);

	/*
	 * check that FATs are readable. At least one must be!
	 */
	if (!mflag)
		pfmt(stdout, MM_NOSTD,
			":374:\n%s ** Phase 1 - Check FATs\n", special);

	check_fats();

	if (mflag) {
		pfunc = checkfstate;
		searchrootdir();
	} else {
		/*
		 * make sure that all readable FATs are in Synch.
		 */
		synch_fats();

		pfmt(stdout, MM_NOSTD,
		    ":375:\n%s ** Phase 2 - Check directory entries (pass 1)\n",
		       special);
		pass1();

		/*
		 * find out if there are any clusters being referenced by more
		 * than one directory entry.
		 */
		pfmt(stdout, MM_NOSTD,
		     ":376:\n%s ** Phase 3 - Cross reference Check\n", special);
		check4multiref();
		
		if (fs_needpass2) {
		  pfmt(stdout, MM_NOSTD,
		     ":377:\n%s ** Phase 4 - Check directory entries (pass 2)\n",
			    special);
		     pass2();
		}

		/*
		 * find out how many clusters are free/bad/lost/reserved
		 * and try to recover lost clusters.
		 */
		get_fatinfo();
	}

	if (!nflag) {
		/*
		 * Set state of FS to OK
		 */
		pfunc = setfsokay;
		searchrootdir();
	}

	if (fat_modified || fs_modified) {
		if (msync(fs_map, fs_size, MS_SYNC) != 0) {
			error = errno;
			pfmt(stderr, MM_ERROR,
				":378:msync failed with return error code %d\n",
				error);
			exit(1);
		}
		pfmt(stdout, MM_NOSTD,
			":379:\n%s *** FILE SYSTEM WAS MODIFIED ***\n",special);
	}

	fsreport();
}


static void
mmap_fs(off_t dos_offset)
{
	struct rlimit	reslimit, *rlp;
	int		error;

	rlp = &reslimit;

	/*
	 * Try to set the process adress space to infinity
	 * 	 - Try to set the hard limit to infinity
	 *	 - If successfull,
	 *		- set the soft limit to infinity
	 *	 - Else
	 *		get the hard limit
	 *		set the soft limit to the hard limit
	 */
	rlp->rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_VMEM, rlp) == 0) {
		rlp->rlim_cur = RLIM_INFINITY;
		setrlimit(RLIMIT_VMEM, rlp);
	} else {
		getrlimit(RLIMIT_VMEM, rlp);
		setrlimit(RLIMIT_VMEM, rlp);
	}

	fs_size = (fsp->fs_HugeSectors * fsp->fs_BytesPerSec) + dos_offset;


	if ((fs_map = mmap((caddr_t)0, fs_size, (PROT_READ | PROT_WRITE),
			   MAP_SHARED, fd_blk, (off_t)0)) == (caddr_t) -1) {
		error = errno;
		pfmt(stderr, MM_ERROR, 
			":380:mmap failed with return error code %d\n",
			 error);
		exit(1);
	}

	fsp->fs_fatpt = (u_char *)
			(fs_map + (fsp->fs_fatblk * fsp->fs_BytesPerSec));
}


static int
get_buffaddr(off_t blkoffset, char **buffer, size_t nbytes)
{
	off_t	byte_offset;

	byte_offset = blkoffset * fsp->fs_BytesPerSec;
	if (byte_offset > fs_size) {
		pfmt(stderr, MM_ERROR, ":381:%s CAN NOT ACCESS BLK %ld\n",
			 special, blkoffset);
		return -1;
	}
	
	*buffer = fs_map + byte_offset;

	if (nbytes <= (fs_size - byte_offset))

		return (int)nbytes;
	else
		return (int)(fs_size - byte_offset);
}


static int
seek_read(off_t blkoffset, char *buffer, size_t nbytes)
{
	if (lseek(fd, (long)(blkoffset * 512), 0) < 0L) {
		pfmt(stderr, MM_ERROR, ":382:%s CAN NOT SEEK BLK %ld\n",
			 special, blkoffset);
		exit(1);
	}
	return read(fd, buffer, nbytes);
}


static void
check_fats(void)
{
	struct fat	*fp;
	size_t		nbytes;
	size_t		bytesperfat;
	char		buf[512];
	long		bn;
	long		lastblock;
	int		i;

	nbytes = sizeof(struct fat) * fsp->fs_FATs;
	if ((fattab = (struct fat *)malloc(nbytes)) == NULL) {
		pfmt(stderr, MM_ERROR, ":383:malloc failed\n");
		exit (1);
	}

	bytesperfat = (fsp->fs_FATsecs * fsp->fs_BytesPerSec);

	/*
	 * Find out if all FATs in the DOS FS are READABLE/UNREADABLE
	 * and record the information.
	 */
	for (fp = fattab, i = 0; i < (int)fsp->fs_FATs; i++, fp++) {
		fp->fat_fatpt  = fsp->fs_fatpt + (i * bytesperfat);
		fp->fat_fatblk = fsp->fs_fatblk + (i * fsp->fs_FATsecs);
		fp->fat_status = FAT_READABLE;
		lastblock = fp->fat_fatblk + fsp->fs_FATsecs - 1;

		for (bn = fp->fat_fatblk; bn <= lastblock; bn++) {

			if (seek_read(bn, buf, sizeof(buf)) < 0) {
				pfmt(stderr, MM_ERROR,
				     ":384:%s ERROR reading FAT %d blk %ld\n",
				     special, (i + 1), bn);
				fp->fat_status = FAT_UNREADABLE;
				break;
			}
		}
	}

	/*
	 * Find the first readable FAT in the table. This will be
	 * consider FAT 1.
	 */
	for (fp = fattab, i = 0; i < (int)fsp->fs_FATs; i++, fp++) {

		if (fp->fat_status == FAT_READABLE) {
			fsp->fs_fatblk = fp->fat_fatblk;
			fsp->fs_fatpt  = fp->fat_fatpt;
			break;
		}
	}

	if (fp->fat_status == FAT_UNREADABLE) {
		pfmt(stderr, MM_ERROR,
			":385:%s All FATs are partially unreadable", special);
		exit (1);
	}
}

static void
synch_fats(void)
{
	struct fat	*fp;
	size_t		bytesperfat;
	int		i;

	bytesperfat = (fsp->fs_FATsecs * fsp->fs_BytesPerSec);

	/*
	 * Check that all readable FATs are in SYNCH.
	 * We assume that FAT 1 is the most up to date since the
	 * kernel does writes to FAT 1 first and then updates all
	 * other FATs.
	 */
	for (fp = fattab, i = 0; i < (int)fsp->fs_FATs; i++, fp++) {


		if (fp->fat_fatpt  == fsp->fs_fatpt ||
		    fp->fat_status == FAT_UNREADABLE)
			continue;

		if (strncmp((char *)fp->fat_fatpt, (char *)fsp->fs_fatpt,
			    bytesperfat) != 0) {

			pfmt(stderr, MM_ERROR,
				":386:%s FATs are out of synch\n", special);
			if (isfixwanted()) {
				strncpy((char *)fp->fat_fatpt,
				        (char *)fsp->fs_fatpt, bytesperfat);
				fat_modified = B_TRUE;
			}
		}
	}
}


static void
get_fatinfo(void)
{
	size_t	nbytes;
	u_long  cn;
	u_long  fatentry;
	u_long	i;
	
	fsp->fs_nfreeclusters = 0;
	fsp->fs_nbadclusters  = 0;

	nbytes = (fsp->fs_FATsecs * fsp->fs_BytesPerSec);
	cn = FIRST_CLUSTER;

	for (i = FATOFFSET(cn); i <= nbytes && cn <= fsp->fs_maxcluster;
						 cn++, i = FATOFFSET(cn)) {
		fatentry = getfatentry(cn, (fsp->fs_fatpt + i));

		/*
		 * If the cluster byte map shows that a cluster is  FREE_CLUSTER
		 * (i.e. no CLAIMED by any of the directory entries in the FS)
		 * AND the corresponding FAT entry for that cluster is
		 * marked with a valid cluster number range OR marked with
		 * a DOSFS_EOF, then this cluster is a lost cluster!
		 */
		if (fsp->fs_clustermap[cn].nref == FREE_CLUSTER && 
		    ((fatentry >= FIRST_CLUSTER &&
		      fatentry <= fsp->fs_maxcluster) ||
		     (fatentry >= DOSFS_EOF))) {

			pfmt(stderr, MM_ERROR,
				":387:Found a lost allocation unit\n");
			if (isfixwanted()) {
				updatefats(cn, FREE_CLUSTER);
				fat_modified = B_TRUE;
				fsp->fs_nfreeclusters++;
			}else
				fsp->fs_lostclusters++;
		}

		if (fatentry == FREE_CLUSTER)
			fsp->fs_nfreeclusters++;
		if (fatentry == BAD_CLUSTER)
			fsp->fs_nbadclusters++;
		if (fatentry >= SRSRV_CLUSTER && fatentry <= ERSRV_CLUSTER)
			fsp->fs_nrsrvclusters++;
	}
}


static u_long
getfatentry(u_long clustern, u_char *bpt)
{
	union fatentry fat;

	if (FAT12) {
		fat.byte[0] = *bpt;
		fat.byte[1] = *(bpt + 1);
		if (clustern & 1)
			fat.word >>= 4;
		fat.word &= FAT12_MASK;
	} else
		fat.word = *(u_short *)(bpt); 

	return (u_long)fat.word;

}


static u_long
getnextcluster(u_long clustern)
{
	return getfatentry(clustern, fsp->fs_fatpt + FATOFFSET(clustern));
}


#define	NIBLE0	0x0F
#define	NIBLE1	0xF0

static void
putfatentry(u_long clustern, u_char *bpt, u_short newvalue)
{
	union fatentry fat;

	if (FAT12) {
		newvalue &= FAT12_MASK;
		if (clustern & 1) {
			fat.word  = newvalue << 4;
			*bpt      = (*bpt & NIBLE0) | fat.byte[0];
			*(bpt +1) = fat.byte[1];
		} else {
			fat.word  = newvalue;
			*bpt      = fat.byte[0];
			*(bpt +1) = (*(bpt + 1) & NIBLE1) | fat.byte[1];
		}
	} else
		*bpt = newvalue;
	return;
}


static void
updatefats(u_long clustern, u_short newvalue)
{
	int		i;
	struct fat	*fp;
	/*
	 * Update all FATS marked  READABLE
	 */
	for (fp = fattab, i = 0; i < (int)fsp->fs_FATs; i++, fp++) {


		if (fp->fat_status == FAT_READABLE) {
		      putfatentry(clustern, fp->fat_fatpt + FATOFFSET(clustern),
		    		  newvalue);
		}
	}

}


static void
pass1()
{
	/*
	 * The following kind of checking will be performed during pass1:
	 *	- If the entry is for "..", check that the start cluster
	 *	  matches the parent directory.
	 *	- If the entry is for ".", check that the start cluster
	 *	  matches the current directory.
	 *	- If the entry is for a directory, check that file size is ZERO.
	 *	- If the entry is for a file, the file size is checked.
	 *	- For all entries:
	 *		- check that start cluster is within range.
	 *		- check that all its FAT entries are within range.
	 *
	 * The following information is collected during pass1:
	 *	- number of subdirectories
	 *	- number of regular files
	 *	- number of hidden files
	 *	- byte count per subdirectories
	 *	- byte count per regular files
	 *	- byte count per hidden files
	 *	- for each cluster in the file system, number of time it is
	 *	  being claimed. This info is collected in the cluster byte
	 *	  map "fsp->fs_clustermap" and it is used for the following
	 *	  cross reference checking:
	 *		- more than one directory entry claiming cluster
	 *		- lost clusters (i.e. clusters not being claimed
	 *		  by any directory entry but not being marked FREE.
	 * For all the above cheking a corrective action is performed.
	 */

	pfunc = checkentry;
	searchrootdir();
}


static void
pass2()
{
	int	i;
	size_t  nitems;
	struct dirlist  *lp, *plp;

	/*
	 * Re-initialized to Zero all global variables that are used in
	 * the collection of data when the filesystem is checked.
	 */
	fsp->fs_sdirectories = 0;
	fsp->fs_sdirbytecnt  = 0;
	fsp->fs_regfiles     = 0;
	fsp->fs_regbytecnt   = 0;
	fsp->fs_hiddenfiles   = 0;
	fsp->fs_hiddenbytecnt = 0;
	
	nitems = fsp->fs_nclusters + FIRST_CLUSTER;

	for (i = 0; i < nitems; i++) {
		
		fsp->fs_clustermap[i].nref = 0;

		for (lp = fsp->fs_clustermap[i].dirlist; lp != NULL;) {
			plp = lp->next;
			free(lp);
			lp = plp;
		}

		fsp->fs_clustermap[i].dirlist = lp;
			
	}

	/*
	 * This is same as pass1. Operation needed again because the
	 * corrective action performed during cross-referencing checking
	 * had invalidated the data collected during pass1.
	 */
	pfunc = checkentry;
	searchrootdir();
}


static void
searchrootdir(void)
{
	size_t  nbytes;
	long	bn;
	long	lastrootblk;

	nbytes = fsp->fs_BytesPerSec;

	lastrootblk = fsp->fs_rootdirblk + fsp->fs_rootdirsize - 1;

	for (bn = fsp->fs_rootdirblk; bn <= lastrootblk; bn++) {
		checkdirents(DOSFSROOT, DOSFSROOT, bn, nbytes);
	}
}


static void
checkdirents(u_long dot_cn, u_long dotdot_cn, long bn, size_t nbytes)
{
	char		*bpt;
	dosdirent_t	*dpt;
	int		i;
	size_t		buff_bytes;

	if ((buff_bytes = get_buffaddr(bn, &bpt, nbytes)) < 0) {
		pfmt(stderr, MM_ERROR,
			":381:%s CAN NOT ACCESS BLK %ld \n", special, bn);
		return;
	}

	nbytes = buff_bytes;

	for (i = 0; i < nbytes; i += sizeof(dosdirent_t)) {
		dpt = (dosdirent_t *)(bpt + i);
		(*pfunc)(dot_cn, dotdot_cn, dpt);
	}
}


static void
searchdir(u_long dot_cn, u_long dotdot_cn) 
{
	size_t  nbytes;
	u_long	cn;
	long	bn;

	nbytes = fsp->fs_SectPerClust * fsp->fs_BytesPerSec;

	for (cn = dot_cn; cn < DOSFS_EOF; cn = getnextcluster(cn)) {

		if (cn < FIRST_CLUSTER || cn > fsp->fs_maxcluster)
			break;

		bn = CNTOBN(cn);

		checkdirents(dot_cn, dotdot_cn, bn, nbytes);
	}
}


static void
checkentry(u_long dot_cn, u_long dotdot_cn, dosdirent_t *dpt)
{
	int i;

	if (dpt->deName[0] == SLOT_EMPTY)
		return;

	if (dpt->deName[0] == SLOT_DELETED)
		return;

	if (dpt->deName[0] == SLOT_ISALIAS) {
		check_alias(dot_cn, dotdot_cn, dpt);
		return;
	}

	if (dpt->deAttributes & ATTR_DIRECTORY) {

		check_dir(dot_cn, dotdot_cn, dpt);

		fsp->fs_sdirectories++;
		fsp->fs_sdirbytecnt += bytes_alloc((u_long)dpt->deStartCluster);
		dotdot_cn = dot_cn;
		dot_cn = dpt->deStartCluster;
		searchdir(dot_cn, dotdot_cn);
		return;
	}

	if (dot_cn == DOSFSROOT && (dpt->deAttributes & ATTR_VOLUME)) {
		for (i = 0; i < sizeof(fsp->fs_volumelable); i++)
			fsp->fs_volumelable[i] = '\0';

		strncpy((char *)fsp->fs_volumelable, (char *)dpt->deName,
			sizeof(dpt->deName));
		strncat((char *)fsp->fs_volumelable, (char *)dpt->deExtension,
			sizeof(dpt->deExtension));
		fsp->fs_volumedate.ddi = dpt->deDate;
		fsp->fs_volumetime.dti = dpt->deTime;
		return;
	}

	if (dpt->deAttributes & ATTR_HIDDEN) {
		check_file(dpt);
		fsp->fs_hiddenfiles++;
		fsp->fs_hiddenbytecnt +=
				       bytes_alloc((u_long)dpt->deStartCluster);

	} else if (dpt->deAttributes &  ATTR_READONLY ||
		   dpt->deAttributes &  ATTR_SYSTEM   ||
		   dpt->deAttributes &  ATTR_ARCHIVE  ||
		   dpt->deAttributes == ATTR_NORMAL    ) {

		check_file(dpt);
		fsp->fs_regfiles++;
		fsp->fs_regbytecnt += bytes_alloc((u_long)dpt->deStartCluster);
	}

}


static void
checkfstate(u_long dot_cn, u_long dotdot_cn, dosdirent_t *dpt)
{
	static u_short	entry_cnt = 0;

	/*
	 * If this entry correspondes to the fstate file
	 */
	if (strncmp((char *)dpt->deName, "FS_STATE", 8) == 0) {

		pfmt(stderr, MM_ERROR,
			":165:sanity check: %s needs checking\n", special);
		exit(32);
	}

	/*
	 * The file fstate is not in the root directory
	 */
	if (dpt->deName[0] == SLOT_EMPTY || ++entry_cnt == fsp->fs_RootDirEnts){
		pfmt(stderr, MM_ERROR,
			":166:sanity check: %s okay\n", special);
		exit(0);
	}

	return;

}


static void
setfsokay(u_long dot_cn, u_long dotdot_cn, dosdirent_t *dpt)
{

	/*
	 * If this entry correspondes to the fstate file
	 * then mark it DELETED.
	 */
	if (strncmp((char *)dpt->deName, "FS_STATE", 8) == 0 &&
	    dpt->deFileSize == 0) {

		markit_del(dpt);
		fsp->fs_regfiles--;
	}

	return;
}


static u_long
bytes_alloc(u_long clustern)
{
	u_long	numofclusters = 0;
	u_long	cn;

	if (clustern < FIRST_CLUSTER || clustern > fsp->fs_maxcluster)
		return 0;

	for (cn = clustern; cn < DOSFS_EOF; cn = getnextcluster(cn)) {
		/*
		 * Found a (Free/bad/reserved) cluster mark before reaching EOF.
		 * Something is wrong!!!
		 */
		if (cn == 0 || (cn >= DOSFS_RESERVED && cn <= DOSFS_BAD))
			numofclusters--;

		/*
		 * cluster number out of range. Stop search for next!!!
		 */
		if (cn < FIRST_CLUSTER || cn > fsp->fs_maxcluster)
			break;

		numofclusters++;
	}
	return (numofclusters * fsp->fs_SectPerClust * fsp->fs_BytesPerSec);

}


static void
check_alias(u_long dot_cn, u_long dotdot_cn, dosdirent_t *dpt)
{

	/*
	 * If this is a slot for .. 
	 */
	if (dpt->deName[0] == SLOT_ISALIAS && dpt->deName[1] == SLOT_ISALIAS) {
		if (dpt->deStartCluster != dotdot_cn) {
		        pfmt(stderr, MM_ERROR,
	  	        ":388:'..' cluster number does not match parent dir\n");
			if (isfixwanted()) {
				dpt->deStartCluster = dotdot_cn;
				fs_modified = B_TRUE;
			}
		}
	} else { /* slot is for . */
		if (dpt->deStartCluster != dot_cn) {
		        pfmt(stderr, MM_ERROR,
		        ":389:'.' cluster number does not match current dir\n");
			if (isfixwanted()) {
				dpt->deStartCluster = dot_cn;
				fs_modified = B_TRUE;
			}
		}
	}
}


static void
check_dir(u_long dot_cn, u_long dotdot_cn, dosdirent_t *dpt)
{
	/*
	 * Will not  do this check until the dosfs kernel part is fixed.
	 * At this point, it writes to disk the actual directory size.
	 * DOS however does not do that. 
	 *
	if (dpt->deFileSize != 0) {
		pfmt(stderr, MM_ERROR,
			":390:File Size for %s is incorrect\n", dpt->deName);
		if (isfixwanted()) {
			dpt->deFileSize = 0;
			fs_modified = B_TRUE;
		}
	}
	 *
	 */
	chk_startcluster(dpt);
	chk_defatentries(dpt);
	
}


static void
check_file(dosdirent_t *dpt)
{
	u_long	nbytes;

	chk_startcluster(dpt);
	chk_defatentries(dpt);

	nbytes = bytes_alloc(dpt->deStartCluster);
	if (dpt->deFileSize > nbytes || dpt->deFileSize < 0) {
		pfmt(stderr, MM_ERROR,
			":390:File Size for %s is incorrect\n", dpt->deName);
		if (isfixwanted()) {
			dpt->deFileSize = nbytes;
			fs_modified = B_TRUE;
		}
	}
	
}


static void
chk_startcluster(dosdirent_t *dpt)
{
	if (dpt->deStartCluster != 0 &&
	    ((long)dpt->deStartCluster < FIRST_CLUSTER ||
	     (long)dpt->deStartCluster > fsp->fs_maxcluster)    ) {
		pfmt(stderr, MM_ERROR,
			":391:Start cluster for %s is out of range\n",
			 dpt->deName);
		if (isfixwanted()) {
			dpt->deStartCluster = 0;
			dpt->deFileSize     = 0;
			fs_modified = B_TRUE;
		}
	}
}


static void
chk_defatentries(dosdirent_t *dpt)
{
	long	pcn, cn;

	if ((long)dpt->deStartCluster >= FIRST_CLUSTER &&
		(long)dpt->deStartCluster <= fsp->fs_maxcluster) {

		pcn = dpt->deStartCluster;
		fsp->fs_clustermap[pcn].nref++; 

		for (cn = getnextcluster(pcn); cn < DOSFS_EOF;
					   pcn = cn, cn = getnextcluster(pcn)) {

			if (cn < FIRST_CLUSTER || cn > fsp->fs_maxcluster ) {

				pfmt(stderr, MM_ERROR,
				      ":392:FAT entry for %s is out of range\n",
					dpt->deName);
				if (isfixwanted()) {
					updatefats(pcn, EEOF_CLUSTER);
					fat_modified = B_TRUE;
					fsp->fs_clustermap[pcn].nref--;
				}
				break;
			}

			fsp->fs_clustermap[cn].nref++;
		}
	}
}


static boolean_t
isfixwanted(void)
{
	FILE    *fi = stdin;
	int	len;
	char 	line[BUFSIZ];

	if (nflag)
		return B_FALSE;
	if (yflag)
		return B_TRUE;
	if (!yflag && !nflag) {
		while (B_TRUE) {
			fflush(fi);
			pfmt(stdout, MM_NOSTD,
			  ":393:Fix it? Please enter 'y' or 'n' (yes or no): ");
			fgets(line, BUFSIZ, fi);
			*strchr(line,'\n') = '\0';
			len = strlen(line);
                	if (len && regexec(&yesre, line, (size_t)0,
	                   (regmatch_t *)0, 0 == 0))
				return B_TRUE;
                	if (len && regexec(&nore, line, (size_t)0,
	                   (regmatch_t *)0, 0 == 0))
				return B_FALSE;
			pfmt(stdout, MM_NOSTD, ":394:\n");
		}
	}
}


static void
fsreport()
{
	pfmt(stdout, MM_NOSTD, ":394:\n");
	if (fsp->fs_volumelable[0] != '\0') {
		pfmt(stdout, MM_NOSTD,
			":395:Volume %s   Created %s-%.2d-%.4d  %2d:%.2d\n\n",
		       fsp->fs_volumelable,
		       Months[fsp->fs_volumedate.dds.dd_month],
		       fsp->fs_volumedate.dds.dd_day,
		       fsp->fs_volumedate.dds.dd_year + 1980,
		       fsp->fs_volumetime.dts.dt_hours,
		       fsp->fs_volumetime.dts.dt_minutes);
	} else 
		pfmt(stdout, MM_NOSTD,
		  ":396:Volume in drive %s does not have a label\n\n", special);

	if (fsp->fs_lostclusters > 0) 
		pfmt(stdout, MM_NOSTD,
			":397:\n%ld lost allocation unit(s)\n\n",
			fsp->fs_lostclusters);

	pfmt(stdout, MM_NOSTD,
		":398:% 10lu bytes total File system space\n",
		fsp->fs_nclusters * fsp->fs_SectPerClust * fsp->fs_BytesPerSec);

	if (fsp->fs_hiddenfiles > 0)
		pfmt(stdout, MM_NOSTD,
			":399:% 10ld bytes in % 4ld hidden file(s)\n",
			fsp->fs_hiddenbytecnt, fsp->fs_hiddenfiles);
	if (fsp->fs_sdirectories > 0)
		pfmt(stdout, MM_NOSTD,
			":400:% 10ld bytes in % 4ld subdirectories\n",
			fsp->fs_sdirbytecnt, fsp->fs_sdirectories);
	if (fsp->fs_regfiles > 0)
		pfmt(stdout, MM_NOSTD,
			":401:% 10ld bytes in % 4ld regular file(s)\n",
			fsp->fs_regbytecnt, fsp->fs_regfiles);

	pfmt(stdout, MM_NOSTD,
		":402:% 10lu bytes available in File system\n\n",
	    fsp->fs_nfreeclusters * fsp->fs_SectPerClust * fsp->fs_BytesPerSec);

	pfmt(stdout, MM_NOSTD,
		":403:% 10d bytes in each allocation unit\n",
		fsp->fs_SectPerClust * fsp->fs_BytesPerSec);

	pfmt(stdout, MM_NOSTD,
		":404:% 10ld total allocation units on File system\n",
		fsp->fs_nclusters);

	pfmt(stdout, MM_NOSTD,
		":405:% 10ld available allocation units on File system\n",
		fsp->fs_nfreeclusters);

	if (fsp->fs_nrsrvclusters > 0)
		pfmt(stdout, MM_NOSTD,
			":406:\n%ld reserved allocation unit(s)\n\n",
			fsp->fs_nrsrvclusters);
	if (fsp->fs_nbadclusters > 0)
		pfmt(stdout, MM_NOSTD,
			":407:\n%ld allocation unit(s) marked bad\n\n",
			fsp->fs_nbadclusters);
}


static char *
unrawname(char *cp)
{
	char *dp = strrchr(cp, '/');
	struct stat stb;
	static char rawbuf[MAXPATHLEN];

	if (dp == 0)
		return (cp);
	if (stat(cp, &stb) < 0)
		return (cp);
	if ((stb.st_mode&S_IFMT) != S_IFCHR)
		return (cp);
	/* for device naming convention /dev/dsk/c1d0s2 */
	sprintf(rawbuf, "/dev/dsk/%s", dp+1);
	if (stat(rawbuf, &stb) == 0)
		return(rawbuf);
	
	/* for device naming convention /dev/save */
	if (*(dp+1) != 'r')
		return (cp);
	(void)strcpy(dp+1, dp+2);
	return (cp);
}

static char *
rawname(char *cp)
{
	static char rawbuf[MAXPATHLEN];
	char *dp = strrchr(cp, '/');
	struct stat statb;

	if (dp == 0)
		return (0);
	/* for device naming convention /dev/dsk/c1d0s2 */
	sprintf(rawbuf, "/dev/rdsk/%s", dp+1);
	if (stat(rawbuf, &statb) == 0)
		return (rawbuf);
	/* for device naming convention /dev/save */
	*dp = 0;
	(void)strcpy(rawbuf, cp);
	*dp = '/';
	(void)strcat(rawbuf, "/r");
	(void)strcat(rawbuf, dp+1);
	return (rawbuf);
}



static void
check4multiref()
{
	int	i;
	size_t  nitems;

	nitems = fsp->fs_nclusters + FIRST_CLUSTER;

	for (i = 0; i < nitems; i++) {
		if (fsp->fs_clustermap[i].nref > 1) {
			/*
			 * Found that at least one cluster is being claimend by
			 * more than one directory entry. 
			 * Must search the entire tree to get the address of
			 * all directory entries claiming the same cluster
			 * number.
			 */
			pfunc = search4multiref;
			searchrootdir();
			/*
			 * Must break from this loop. The clustermap now has
			 * all the information needed.
			 */
			break;
		}
	}

	/*
	 * If any multi references are found, try to fix it.
	 */
	for (i = 0; i < nitems; i++) {
		if (fsp->fs_clustermap[i].nref > 1) {

		   pfmt(stderr,MM_ERROR,
		   ":408:%s: %d directory enries claiming allocation unit %d\n",
			special, fsp->fs_clustermap[i].nref, i);

			if (isfixwanted()) {

				/*
				 * Delete/truncate the most recently created 
				 * directory entries claiming this cluster
				 * 
				 */
				del_dentry(&(fsp->fs_clustermap[i]));
				fs_modified = B_TRUE;
				fs_needpass2 = B_TRUE;
			}
		}
	}
}


static void
chk_de4multiref(dosdirent_t *dpt)
{
	long	pcn, cn;

	if ((long)dpt->deStartCluster >= FIRST_CLUSTER &&
		(long)dpt->deStartCluster <= fsp->fs_maxcluster) {

		pcn = dpt->deStartCluster;
		if (fsp->fs_clustermap[pcn].nref > 1) {
			/*
			 * Add the directory entry address to the list
			 */
			add_dir2list(dpt, &(fsp->fs_clustermap[pcn]));
		}

		for (cn = getnextcluster(pcn); cn < DOSFS_EOF;
					   pcn = cn, cn = getnextcluster(pcn)) {

			if (cn < FIRST_CLUSTER || cn > fsp->fs_maxcluster ) {

				/*
				 * Fat entry is out range. Stop search for this
				 * directory entry here.
				 */
				break;
			}

			if (fsp->fs_clustermap[cn].nref > 1) {
				/*
				 * Add the directory entry address to the list
				 */
				add_dir2list(dpt, &(fsp->fs_clustermap[cn]));
			}
		}
	}
}


static void
search4multiref(u_long dot_cn, u_long dotdot_cn, dosdirent_t *dpt)
{

	if (dpt->deName[0] == SLOT_EMPTY   ||
	    dpt->deName[0] == SLOT_DELETED ||
	    dpt->deName[0] == SLOT_ISALIAS    )
		return;

	if (dpt->deAttributes & ATTR_DIRECTORY) {
		chk_de4multiref(dpt);

		dotdot_cn = dot_cn;
		dot_cn = dpt->deStartCluster;
		searchdir(dot_cn, dotdot_cn);
		return;
	}

	if (dpt->deAttributes &  ATTR_HIDDEN    ||
	    dpt->deAttributes &  ATTR_READONLY  ||
	    dpt->deAttributes &  ATTR_SYSTEM    ||
	    dpt->deAttributes &  ATTR_ARCHIVE   ||
	    dpt->deAttributes == ATTR_NORMAL      ) {

		chk_de4multiref(dpt);
		return;
	}
}


static void
add_dir2list(dosdirent_t *dpt, clustermap_t *cp)
{
	struct dirlist	*lp, *plp;

	for (plp = lp = cp->dirlist; lp != NULL; plp = lp, lp = lp->next)
		;

	lp = (struct dirlist *)malloc(sizeof(struct dirlist));
	lp->next = NULL;
	lp->direntry = dpt;
	if (plp != NULL)
		plp->next = lp;
	else
		cp->dirlist = lp;
}


static void
del_dentry(clustermap_t *cp)
{

	struct dirlist	*lp1;
	dosdirent_t	*dpt1, *dpt2;

	for (lp1 = cp->dirlist; lp1 != NULL && lp1->next != NULL;) {
		dpt1 = lp1->direntry;
		dpt2 = lp1->next->direntry;

		if ( dpt1->deDate >  dpt2->deDate ||
		    (dpt1->deDate == dpt2->deDate &&
		     dpt1->deTime >= dpt2->deTime)  ){

			/*
			 * Mark this directory entry as DELETED
			 */
			markit_del(dpt1);

			update_clustermap(dpt1);
		} else {
			/*
			 * Mark this directory entry as a DELETED entry
			 */
			markit_del(dpt2);

			update_clustermap(dpt2);
		}

		lp1 = cp->dirlist;
	}
}


static void
markit_del(dosdirent_t *dpt)
{
		dpt->deName[0] = SLOT_DELETED;
		dpt->deFileSize = 0;
		fs_modified = B_TRUE;
}


static void
update_clustermap(dosdirent_t *dpt)
{
	int		i;
	size_t		nitems;
	clustermap_t	*cp;
	struct dirlist	*lp, *plp;

	nitems = fsp->fs_nclusters + FIRST_CLUSTER;

	/*
	 * This directory entry may be referencing more than one cluster,
	 * therrefore, the directory entry maybe in more than one list.
	 * So we must check and remove the directory entry from all the lists
	 * it may be found. This is because the caller has already marked this
	 * entry as DELETED in the file system!!!
	 */
	for (i = 0; i < nitems; i++) {
		cp = &(fsp->fs_clustermap[i]);
		for (plp = lp = cp->dirlist; lp != NULL;
						      plp = lp, lp = lp->next) {
			/*
			 * If directory entry is in this list, remove it from
			 * the list.
			 */
			if (lp->direntry == dpt) {
				if (lp != plp)
					plp->next = lp->next;
				else
					cp->dirlist = lp->next;
				free(lp);

				cp->nref--;

				break;
			}
		}

	}
}


void
get_bootblk(char *bootblock, long offset)
{
	union bootsector	*bsp;

	if (seek_read(offset, bootblock, 512) < 512) {
		pfmt(stderr, MM_ERROR, ":409:%s CAN NOT READ BLK %ld\n",
			 special, offset);
		exit(1);
	}

	bsp = (union bootsector *)bootblock;
	
	if (bsp->bs50.bsBootSectSig != BOOTSIG) {
		pfmt(stderr, MM_ERROR, ":7:%s is not a %s file system\n",
			special, fstype);
		exit(1);
	}
}


void
find_extdosfs(char *bootblock, long *dos_offset)
{
	union bootsector	*bsp;
	struct ipart	*ipt;
	int		i;

	for (i = 0; i < FD_NUMPART; i++) {

		ipt = (struct ipart *)((bootblock + BOOTSZ) + 
				       (i * sizeof(struct ipart)));

		if ( ipt->numsect > 0 &&
		    (ipt->systid == DOSOS12 ||
		     ipt->systid == DOSOS16 ||
		     ipt->systid == DOSHUGE ||
		     ipt->systid == EXTDOS)) {
			*dos_offset += ipt->relsect;
			break;
		}
	}

	if ( ipt->relsect > 0 && ipt->systid == EXTDOS) {
		get_bootblk(bootblock, *dos_offset);

		bsp = (union bootsector *)bootblock;

		if (*(bsp->bs50.bsJump) == 0)
			find_extdosfs(bootblock, dos_offset);
	}
}
