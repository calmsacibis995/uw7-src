#ident	"@(#)dosfs.cmds:fsck.h	1.5"
#ident  "$Header$"


#define	FSTYPE		"dosfs"

#define	DOSFSROOT       0       /* cluster 0 means the root dir         */
#define	FREE_CLUSTER	0       /* cluster 0 also means a free cluster  */
#define	FIRST_CLUSTER	2       /* first legal cluster number           */
#define	SRSRV_CLUSTER	0xfff0  /* start of reserved cluster range      */
#define	ERSRV_CLUSTER	0xfff6  /* end of reserved cluster range        */
#define	BAD_CLUSTER	0xfff7  /* a cluster with a defect              */
#define	SEOF_CLUSTER	0xfff8  /* start of eof cluster range           */
#define	EEOF_CLUSTER	0xffff  /* end of eof cluster range             */

#define	SLOT_ISALIAS	0x2e

#define FAT12_MASK	0x0fff  /* mask for 12 bit cluster numbers      */
#define FAT16_MASK	0xffff  /* mask for 16 bit cluster numbers      */

#define FAT12      (fsp->fs_maxcluster <= 4086)
#define FAT16      (fsp->fs_maxcluster >  4086)

#define FATOFFSET(cn) \
	(FAT12 ? ((cn) + ((cn) >> 1)) : ((cn) << 1))

#define	DOSFS_RESERVED \
	(FAT12 ? (0xff0) : (0xfff0))

#define	DOSFS_BAD \
	(FAT12 ? (0xff7) : (0xfff7))

#define	DOSFS_EOF \
	(FAT12 ? (0x0ff8) : (0xfff8))

#define CNTOBN(cn) \
	((((cn) - FIRST_CLUSTER) * (fsp)->fs_SectPerClust) + \
	(fsp)->fs_firstcluster)

#define	BNTOCN(bn) \
	((((bn) - (fsp)->fs_firstcluster) / (fsp)->fs_SectPerClust) + \
	FIRST_CLUSTER)

union fatentry {
        u_short word;
        u_char  byte[2];
};

struct dirlist {
	struct dirlist	*next;
	dosdirent_t	*direntry;
};

struct clustermap {
	char		nref;
	struct dirlist	*dirlist;
};

typedef	struct clustermap	clustermap_t;

struct fs_global {
        u_short 	fs_BytesPerSec;   /* bytes per sector */
        u_char		fs_SectPerClust;  /* sectors per cluster */
        u_short		fs_ResSectors;    /* number of reserved sectors */
        u_char		fs_FATs;          /* number of FATs */
        u_short		fs_RootDirEnts;   /* number of root directory entries */
        u_short		fs_Sectors;       /* total number of sectors */
        u_long		fs_HugeSectors;   /* total number of sectors if 
					     fs_Sectors is == 0 */
        u_char		fs_Media;         /* media descriptor */
	u_short		fs_FATsecs;       /* number of sectors per FAT */
	u_char		*fs_fatpt;        /* memory address of FAT 1 */
	long		fs_fatblk;        /* block number of FAT 1 */
	long		fs_rootdirblk;    /* block number of root directory */
	long		fs_rootdirsize;   /* size in blocks of root directory */
	long		fs_firstcluster;  /* block number of first cluster */
	long		fs_nclusters;     /* number of clusters in FS */
	long		fs_maxcluster;    /* maximum cluster number */
	long		fs_nfreeclusters; /* number of free clusters in FS */
	long		fs_nbadclusters;  /* number of bad cluster in FS */
	long		fs_nrsrvclusters; /* num. of reserved clusters in FS */
	long		fs_lostclusters;  /* number of lost cluster in FS */
	clustermap_t	*fs_clustermap;   /* cluster map used for xref. check */
	u_long		fs_sdirectories;  /* number of subdirectories in FS */
	u_long		fs_sdirbytecnt;   /* size of subdirectories in bytes */
	u_long		fs_hiddenfiles;
	u_long		fs_hiddenbytecnt;
	u_long		fs_regfiles;
	u_long		fs_regbytecnt;
	u_char		fs_volumelable[12];
	dosdate_t	fs_volumedate;
	dostime_t	fs_volumetime;	
};

#define FAT_READABLE    B_TRUE
#define FAT_UNREADABLE  B_FALSE

/*
 * DOS file system have more than one FAT. Usualy there are 2 FATs per DOS
 * File systems.  All of these FATs are to be kept in Synch.
 * So, that if the first FAT goes bad (e.g., bad block(s)) the other(s) can
 * be used.
 * We use the following structure to keep needed information of all FATs
 * in the File system.
 */

struct fat {
	u_char		*fat_fatpt;	/* FAT address in memory              */
	long		fat_fatblk;	/* block number of FAT                */
	boolean_t	fat_status;	/* readable/unreadbale                */
};
