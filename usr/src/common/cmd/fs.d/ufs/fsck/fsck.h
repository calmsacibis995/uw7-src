/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/fsck/fsck.h	1.3.8.5"
#ident "$Header$"

#define	MAXDUP		10	/* limit on dup blks (per inode) */
#define	MAXBAD		10	/* limit on bad blks (per inode) */

typedef	int	(*SIG_TYP)();

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

#define PATHBUFSIZ	2048	/* pathname buffer size */

#define	USTATE	01		/* inode not allocated */
#define	FSTATE	02		/* inode is file */
#define	DSTATE	03		/* inode is directory */
#define	DFOUND	04		/* directory found during descent */
#define	DCLEAR	05		/* directory is to be cleared */
#define	FCLEAR	06		/* file is to be cleared */
#define	DCORRPT	07		/* directory is corrupted */

typedef struct dinode	DINODE;
typedef struct direct	DIRECT;

#define	ALLOC(dip)	(((dip)->di_mode & IFMT) != 0)
#define	DIRCT(dip)	(((dip)->di_mode & IFMT) == IFDIR)
#define MINDIRSIZE	(sizeof (struct dirtemplate))

#define	SPECIAL(dip) 	((((dip)->di_mode & IFMT) == IFBLK) || \
			    (((dip)->di_mode & IFMT)== IFCHR) || \
			    (((dip)->di_mode & IFMT)== IFNAM)) 

#define	MAXNINDIR	(MAXBSIZE / sizeof (daddr_t))
#define	MAXINOPB	(MAXBSIZE / sizeof (struct dinode))
#define	SPERB		(MAXBSIZE / sizeof(short))

struct bufarea {
	struct bufarea	*b_next;		/* must be first */
	daddr_t	b_bno;
	int	b_size;
	int	b_errs;
	union {
		char	b_buf[SBSIZE];		/* buffer space */
		short	b_lnks[SPERB];		/* link counts */
		daddr_t	b_indir[MAXNINDIR];	/* indirect block */
		struct	fs b_fs;		/* super block */
		struct	cg b_cg;		/* cylinder group */
		struct dinode b_dinode[MAXINOPB]; /* inode block */
	} b_un;
	char	b_dirty;
};

typedef struct bufarea BUFAREA;

BUFAREA	inoblk;			/* other blks in filesys */
BUFAREA	fileblk;		/* other blks in filesys */
BUFAREA	sblk;			/* file system superblock */
BUFAREA	cgblk;			/* cylinder group blocks */

#define	initbarea(x)	(x)->b_dirty = 0;(x)->b_bno = (daddr_t)-1
#define	dirty(x)	(x)->b_dirty = 1
#define	inodirty()	inoblk.b_dirty = 1
#define	sbdirty()	sblk.b_dirty = 1
#define	cgdirty()	cgblk.b_dirty = 1

int MEM;

#define INODIRTY(x) { \
	if (MEM) 					\
		*(inobuf.bb_dirty +	 		\
		((x - inobuf.bb_strino) / INOPB(&sblock))) = 1;	\
	else						\
		inoblk.b_dirty = 1;		\
}
#define	dirblk		fileblk.b_un
#define	sblock		sblk.b_un.b_fs
#define	cgrp		cgblk.b_un.b_cg

struct filecntl {
	int	rfdes;
	int	wfdes;
	int	mod;
} dfile;			/* file descriptors for filesys */

enum fixstate { DONTKNOW, NOFIX, FIX};

struct inodesc {
	enum fixstate id_fix;	/* policy on fixing errors */
	int (*id_func)();	/* function to be applied to blocks of inode */
	ino_t id_number;	/* inode number described */
	ino_t id_parent;	/* for DATA nodes, their parent */
	daddr_t id_blkno;	/* current block number being examined */
	int id_numfrags;	/* number of frags contained in block */
	long id_filesize;	/* for DATA nodes, the size of the directory */
	int id_loc;		/* for DATA nodes, current location in dir */
	int id_entryno;		/* for DATA nodes, current entry number */
	DIRECT *id_dirp;	/* for DATA nodes, ptr to current entry */
	char *id_name;		/* for DATA nodes, name to find or enter */
	char id_type;		/* type of descriptor, DATA or ADDR */
};
/* file types */
#define	DATA	1
#define	ADDR	2

#define DBLOCK 01
#define IBLOCK 02

/*
 * Linked list of duplicate blocks.
 * 
 * The list is composed of two parts. The first part of the
 * list (from duplist through the node pointed to by muldup)
 * contains a single copy of each duplicate block that has been 
 * found. The second part of the list (from muldup to the end)
 * contains duplicate blocks that have been found more than once.
 * To check if a block has been found as a duplicate it is only
 * necessary to search from duplist through muldup. To find the 
 * total number of times that a block has been found as a duplicate
 * the entire list must be searched for occurences of the block
 * in question. The following diagram shows a sample list where
 * w (found twice), x (found once), y (found three times), and z
 * (found once) are duplicate block numbers:
 *
 *    w -> y -> x -> z -> y -> w -> y
 *    ^		     ^
 *    |		     |
 * duplist	  muldup
 */
struct dups {
	struct dups *next;
	daddr_t dup;
};
struct dups *duplist;		/* head of dup list */
struct dups *muldup;		/* end of unique duplicate dup block numbers */

/*
 * Linked list of inodes with zero link counts.
 */
struct zlncnt {
	struct zlncnt *next;
	ino_t zlncnt;
};
struct zlncnt *zlnhead;		/* head of zero link count list */

char	rawflg;
char	*devname;
char	nflag;			/* assume a no response */
char	yflag;			/* assume a yes response */
int	bflg;			/* brief output */ 
int	Pflag;			/* Parallel fscks */
int	Lflag;			/* output order by file systems */
int	bflag;			/* alternate block */
int	wflag;			/* writable  */
int	wflg;			/* wait flag */
int	rflag;			/* check raw file systems */
int	debug;			/* output debugging info */
char	preen;			/* just fix normal inconsistencies */
char	fixstate;		/* is FsSTATE to be fixed */	
char	mountedfs;		/* checking mounted device */
int	exitstat;		/* exit status (set to 8 if 'No' response) */

char	*blockmap;		/* ptr to primary blk allocation map */
struct	stmap {
	char	flag;		/* state flag of a file */
	struct 	dirmap *dir_p;	/* ptr to dirmap table */
};

union {
	struct stmap *nstate; /* ptr to inode state table */
	char	*ostate;
} statemap;

#define nstatemap(i)	statemap.nstate[i]
#define ostatemap(i)	statemap.ostate[i]

short	*lncntp;		/* ptr to link count table */
char	pathname[PATHBUFSIZ];	/* current pathname */
char	*pathp;			/* pointer to pathname position */
char	*endpathname;

ino_t	imax;			/* number of inodes */
daddr_t	fmax;			/* number of blocks in the volume */
ino_t	lastino;		/* hiwater mark of inodes */
ino_t	lfdir;			/* lost & found directory inode number */
char	*lfname;		/* lost & found directory name */

off_t	maxblk;			/* largest logical blk in file */
off_t	bmapsz;			/* num chars in blockmap */

daddr_t	n_blks;			/* number of blocks used */
daddr_t	n_files;		/* number of files seen */

#define	zapino(x)	zino.di_gen = (x)->di_gen+1; (*(x) = zino)
struct	dinode zino;

#define	setbmap(x)	setbit(blockmap, x)
#define	getbmap(x)	isset(blockmap, x)
#define	clrbmap(x)	clrbit(blockmap, x)

#define	ALTERED	010
#define	KEEPON	04
#define	SKIP	02
#define	STOP	01

time_t	time();
DINODE	*ginode();
DINODE	*sginode();
BUFAREA	*getblk();
int	findino();
int	ckinode();
char	*setup();
void	setstate();
char	get_state();
struct mnttab *mntdup();
 
#define WYPFLG(w,y,p) 	do { \
				if (w && !y && !p) { \
					exit(41); \
				} \
			} while (0)

/* fsck semaphor to control /dev/tty */
#define FSCK_FILE	"/.fsck"	
#define FSCK_ID		'J'
struct sembuf {
	short	sem_num;	/* semaphore number */
	short	sem_op;		/* semaphore operation */
	short	sem_flg;	/* operation  flag */
} sembuf[1];
int semid;

/* 
 * The directory map structure 
 * This structure is composed of 2 lists: 
 * directory data block list, and directory inode list
 * the data block list is created in pass1 when inodes are read.
 * The inolist is created in pass2 when data blocks are read. 
 */
struct inolist {
	ino_t	inumber;		/* inode number */
	struct inolist *next;
};
struct dirmap {
	ino_t	dot;
	ino_t	dotdot;
	long	filesize;		/* inode size */
	struct inolist	*inolist;	/* inode number list */
	daddr_t	*dblist;		/* data block list */
}; 

/* 
 * The inobuf is used for 64k inode per block.
 * The bb_size field will be 64k and the bb_iperb field will be 512. 
 * The bb_dirty is used to mark a dirty block, which has size of file 
 * system block size.
 */
struct inobuf {
	DINODE	bb_buf[512];		/* inode block */
	char	*bb_dirty;		/* dirty inode block */
	ino_t	bb_strino;		/* starting inode */
	daddr_t	bb_blk;			/* block number */
	int	bb_size;		/* size of the buffer */
	int	bb_iperb;		/* number of inodes per buffer */
	int	bb_dbpbuf;		/* number disk blocks per file system block */
};
struct inobuf	inobuf;			/* inode buffer */

#define	dirtycnt	(inobuf.bb_size / sblock.fs_bsize) 
#define	blkcnt		(sblock.fs_bsize / DEV_BSIZE)

void	creatdirmap();

FILE	*print_fp;

#define	ftypeok(dp)				\
	(dp && (((dp->di_mode & IFMT) == IFDIR) || 	\
	((dp->di_mode & IFMT) == IFREG) || 	\
	((dp->di_mode & IFMT) == IFBLK) || 	\
	((dp->di_mode & IFMT) == IFCHR) || 	\
	((dp->di_mode & IFMT) == IFNAM) || 	\
	((dp->di_mode & IFMT) == IFLNK) || 	\
	((dp->di_mode & IFMT) == IFSOCK) || 	\
	((dp->di_mode & IFMT) == IFIFO) ? 1 : 0)) 


boolean_t	Parent_notified;

#define	getsem()	getrelsem(1)
#define	relsem()	getrelsem(0)
