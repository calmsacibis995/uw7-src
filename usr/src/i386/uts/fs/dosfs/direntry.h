#ifndef _FS_DOSFS_DIRENTRY_H       /* wrapper symbol for kernel use */
#define _FS_DOSFS_DIRENTRY_H       /* subject to change without notice */


#ident	"@(#)kern-i386:fs/dosfs/direntry.h	1.2"
#ident  "$Header$"

#if defined(__cplusplus)
extern "C" {
#endif


/*
 *  Structure of a dos directory entry.
 */
struct direntry {
	unsigned char deName[8];	/* filename, blank filled	*/
#define	SLOT_EMPTY	0x00		/* slot has never been used	*/
#define	SLOT_E5		0x05		/* the real value is 0xe5	*/
#define	SLOT_DELETED	0xe5		/* file in this slot deleted	*/
	unsigned char deExtension[3];	/* extension, blank filled	*/
	unsigned char deAttributes;	/* file attributes		*/
#define	ATTR_NORMAL	0x00		/* normal file			*/
#define	ATTR_READONLY	0x01		/* file is readonly		*/
#define	ATTR_HIDDEN	0x02		/* file is hidden		*/
#define	ATTR_SYSTEM	0x04		/* file is a system file	*/
#define	ATTR_VOLUME	0x08		/* entry is a volume label	*/
#define	ATTR_DIRECTORY	0x10		/* entry is a directory name	*/
#define	ATTR_ARCHIVE	0x20		/* file is new or modified	*/
	char deReserved[10];		/* reserved			*/
	unsigned short deTime;		/* create/last update time	*/
	unsigned short deDate;		/* create/last update date	*/
	unsigned short deStartCluster;	/* starting cluster of file	*/
	unsigned long deFileSize;	/* size of file in bytes	*/
};

typedef struct direntry dosdirent_t;

#define DOSDIRSIZ	(sizeof((dosdirent_t*)0)->deName +\
			 sizeof((dosdirent_t*)0)->deExtension)

/*
 *  This is the format of the contents of the deTime
 *  field in the direntry structure.
 */
struct DOStime {
	unsigned short
	dt_2seconds:5,	/* seconds divided by 2		*/
	dt_minutes:6,	/* minutes			*/
	dt_hours:5;	/* hours			*/
};

/*
 *  This is the format of the contents of the deDate
 *  field in the direntry structure.
 */
struct DOSdate {
	unsigned short
	dd_day:5,	/* day of month			*/
	dd_month:4,	/* month			*/
	dd_year:7;	/* years since 1980		*/
};

union dostime {
	struct DOStime dts;
	unsigned short dti;
};

typedef union dostime	dostime_t;

union dosdate {
	struct DOSdate dds;
	unsigned short ddi;
};

typedef union dosdate	dosdate_t;

/*
 *  The following defines are used to rename fields in
 *  the ufs_specific structure in the nameidata structure
 *  in namei.h
 */
#define	ni_dosfs		ni_ufs

#if defined(_KERNEL)
void unix2dostime (timestruc_t *tvp, union dosdate *ddp, union dostime *dtp);
void dos2unixtime (union dosdate *ddp, union dostime *dtp, timestruc_t *tvp);
int  dos2unixfn (unsigned char *dn, unsigned char *un, int*len);
int unix2dosfn (unsigned char *un, unsigned char *dn, int unlen);

#endif /* defined(_KERNEL) */

#if defined(__cplusplus)
        }
#endif

#endif /* _FS_DOSFS_DIRENTRY_H */

