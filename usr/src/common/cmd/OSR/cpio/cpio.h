#ident	"@(#)OSRcmds:cpio/cpio.h	1.1"
/***************************************************************************
 *			cpio.h
 *--------------------------------------------------------------------------
 * Common include file for cpio c files.
 *
 *--------------------------------------------------------------------------
 *	@(#) cpio.h 25.5 94/09/13 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1994.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *--------------------------------------------------------------------------
 * Revision History:
 *
 *	L000		scol!ashleyb		1st Oct 1993
 *	- Created.
 *	L001		scol!ashleyb		08 Jun 1994
 *	- Changed name of truncate() to avoid clash with libsocket version.
 *	L002		scol!ashleyb		06 Jul 1994
 *	- Removed fperr and fperrno declarations.
 *	- Added block_usage declarations.
 *	L003		scol!ashleyb		08 Sep 1994
 *	- Added set_header_format() and generate_id()
 *
 *==========================================================================
 */

#include <sys/param.h>
#define TRUE 1

#ifdef  INTL
#define EQ(x,y) (strcoll(x,y)==0)
#else
#define EQ(x,y) (strcmp(x,y)==0)
#endif

#define BINARY		0		/* Cflag for old binary format. */
#define OASCII		1		/* Cflag for old ascii format. */
#define ASCII		2		/* Cflag for new ascii format. */
#define CRC		3		/* Cflag for new ascii format. */
#define NONE		4		/* Cflag not verified */
#define M_OASCII	"070707"	/* Old ASCII magic number */
#define M_ASCII		"070701"	/* New ASCII magic number */
#define M_CRC		"070702"	/* New CRC magic number */
#define M_BINARY	070707		/* Binary magic number */
#define M_STRLEN	6		/* number bytes in ASCII magic number */
/* #define PATHSIZE	1024 */		/* maximum PATH length */ /* L023 */
#define IN		'i'		/* copy in */
#define OUT		'o'		/* copy out */
#define PASS		'p'		/* direct copy */  
#define MAX_NAMELEN	14		/* S006 maximum length of file name */
#define LINKS		500		/* no. of links allocated per bunch */
#define BUFSIZE		512		/* In u370, can't use BUFSIZ or BSIZE */
#define CPIOBSZ		4096		/* file read/write */
#define MAX_BUFS	10		/* max number of blocks for buffer */
#define MK_USHORT(a)	((a) & 00000177777)
#define	FORMAT		"%b %e %H:%M:%S %Y"     /* Date time formats */
#define USED_CHECKSUM	59451301L
#define HIGHBIT		0x8000
#define USED_STD	 0		/* Used standard open() and close () */
#define USED_RECOVER	 1		/* Used MT_AMOUNT and MT_RECOVER */
#define OCHARS	76
#define CHARS	110
#define OHDRSIZE	26
#define BYTESWAP 1
#define HALFSWAP 2
#define BOTHSWAP 4

/* Operation types. */
#define SKIP_OP 0
#define NORMAL_OP 1
#define LINK_OP 2

#define IS_ASCII(x)	((x) == ASCII)
#define IS_OASCII(x)	((x) == OASCII)
#define IS_BINARY(x)	((x) == BINARY)
#define IS_CRC(x)	((x) == CRC)
#define IS_NEW(x)	(((x) == ASCII) || ((x) == CRC))

typedef struct {
	char	*b_base_p,	/* pointer to buffer's base */
		*b_out_p,	/* position to take bytes from buffer at */
		*b_in_p,	/* position to put bytes into buffer at */
		*b_end_p;	/* pointer to end of buffer */
	long	b_count,	/* count of unprocessed bytes in this buffer */
		b_size;		/* size of buffer in bytes */
} buf_info;

/* New extended cpio header. */
typedef struct {
	ushort	h_magic;
	ulong	h_ino;
	ulong	h_mode;
	ulong	h_uid;
	ulong	h_gid;
	ulong	h_nlink;
	ulong	h_mtime;
	ulong	h_filesize;
	long	h_dev_maj;
	long	h_dev_min;
	long	h_rdev_maj;
	long	h_rdev_min;
	ulong	h_namesize;
	ulong	h_chksum;
	char	h_name[PATHSIZE+1];
} header;


/* Old cpio header. */
typedef struct {
	short	h_magic;
	short	h_dev;
	ushort	h_ino;
	ushort	h_mode,
		h_uid,
		h_gid;
	short	h_nlink;
	short	h_rdev;
	ushort	h_mtime[2];
	short	h_namesize;
	ushort	h_filesize[2];
	char	h_name[PATHSIZE+1];
} oheader;

typedef struct devlist {
	char		*device;
	int		used;
	struct devlist  *next;
} DevList, *DevPtr;

typedef struct links {
	ulong	inode;
	ulong	major;
	ulong	minor;
	ulong	mode;
	ulong	nlink;
	ulong	uid;
	ulong	gid;
	ulong	mtime;
	char	*name;
	struct links	*prev;
	struct links	*next;
} Links, *LinksPtr;


/*  =====================Function Prototypes =================================*/
#ifndef _BUFIO_C
extern void	bflush(void);
extern int	bread(char *buf, int req, int filelen);
extern void	bwrite(char *buf, int amount);
extern void	rstbuf(void);
extern int	check_io(char *io_string, int *in_fd, int *out_fd); /* L033 */
extern int	register_io(int in_fd, int out_fd);		    /* L033 */
#endif

#ifndef _DEVLIST_C
extern DevPtr	blddevlist(char *device_string);
extern void	freedevlist(DevPtr devlist);
#endif

#ifndef _LINKS_C
extern int	addlink(header *hdr);
extern int	linkcount(header *hdr);
extern void	create_links(header hdr);
extern void	freelinkslist(void);
extern void	flush_link_list(void);
extern void	plinks(header *h, short toc, short v, ushort sum, short nflg);
extern char	*get_next_link(ulong inode, ulong major, ulong minor);
#endif

#ifndef _INODES_C
extern void	generate_id(header *Hdr);			/* L003 */
#endif

#ifndef _UTILITY_C
extern int	build_oa_header(char *buffer, header Hdr);
extern int	build_ob_header(char *buffer, header Hdr);
extern int	build_na_header(char *buffer, header Hdr);
extern int	build_header(short Cflag, char *buffer, header Hdr);
extern int	set_header_format(char *format);		/* L003 */
extern int	pwd(char *directory);
extern int	hdck(header Hdr);
extern int	ckname(char *namep);
extern int	ident(struct stat *in, struct stat *out);
extern int	missdir(char *namep);
extern int	zlstat(int severity, char *path, struct stat *buf);
extern void	set_time(char *namep, time_t atime, time_t mtime);
extern void	set_inode_info(header Hdr);
extern void	truncate_fn(char *path_ptr);			/* L001 */
extern void	usage(void);
extern void	verbdot(FILE *fp, char *cp );
extern void	cannotopen( char *sp, char *mode );
extern void	chartobin(char *buffer, header *Hdr);
extern void	ochartobin(char *buffer, header *Hdr);
extern void	cleanup(int exitcode);
extern void	expand_hdr(oheader Ohdr, header *Hdr_ptr);
extern void	kmesg(void);
extern void	swap(char *buf, int ct,short swapopt);
extern void	skipln(char *p, FILE *fp);
extern void	chkswfile(char *sp, char c, short option);
extern void	deroot(char *path_ptr);
extern ulong	crcsum(int file,ulong filesize);
extern ushort	sum(char *buffer,int sum,ushort lastsum);
extern void	pentry(char *filename,char *symto,ulong uid,ulong mode,ulong size,ulong mtime,ushort sum,short nflag);
extern ulong	block_usage(ulong x, ulong y, int z);		/* L002 */
#endif
