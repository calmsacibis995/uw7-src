#ifndef _FS_MKFS_H	/* wrapper symbol for kernel use */
#define _FS_MKFS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/mkfs.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Exit codes for file system specific mkfs commands.
 * The codes from 32-40 are mainly common errors.
 * The rest are specific to the file systems.
 */
#define	RET_OK		0	/* success */
#define	RET_USAGE	32	/* usage error */

#define	RET_FSYS_STAT	33	/* cannot stat <fsys> */
#define	RET_FSYS_OPEN	34	/* cannot open <fsys> */
#define	RET_FSYS_CREATE	35	/* cannot create <fsys> */
#define	RET_FSYS_MOUNT	36	/* <fsys> is MOUNTED FILE SYSTEM */
#define	RET_FSYS_DEV	37	/* <fsys> is not special device */

#define	RET_ERR_READ	38	/* read error */
#define	RET_ERR_WRITE	39	/* write error */
#define	RET_ERR_SEEK	40	/* seek error */

/* s5 specific */
#define	RET_MALLOC	41	/* cannot allocate space */
#define	RET_EOF		42	/* EOF */
#define	RET_RATIO	43	/* bad ratio */
#define	RET_NO_SPACE	44	/* out of free space */
#define	RET_NOT_S5	45	/* not a s5 file system */
#define	RET_FILE_HUGE	46	/* file too large */
#define	RET_ILIST_SM	47	/* ilist too small */

#define	RET_PROTO_OPEN	48	/* cannot open proto file */
#define	RET_FILE_OPEN	49	/* cannot open <string> file */

#define	RET_ELF		50	/* cannot elf_begin */
#define	RET_PHDR	51	/* cannot get Phdr */

#define	RET_BBLK_WR	52	/* error writing boot-block */
#define	RET_SBLK_RD	53	/* cannot read superblock */
#define	RET_SBLK_WR	54	/* cannot write super-block */

#define	RET_BAD_BLKSZ	55	/* bad block size */
#define	RET_BAD_FTYPE	56	/* bad file type */
#define	RET_BAD_FS	57	/* bad file system type */
#define	RET_BAD_OCTAL	58	/* bad octal mode digit */
#define	RET_BAD_MODE	59	/* bad mode */
#define	RET_BAD_NUM	60	/* bad number */
#define	RET_BAD_BLKS	61	/* too many bad blocks */

/* sfs and ufs specific */
#define	RET_BAD_FSSIZE	70	/* invalid fssize */
#define	RET_BAD_NTRAK	71	/* invalid ntrak */
#define	RET_BAD_NSECT	72	/* invalid nsect */
#define	RET_BAD_RPS	73	/* invalid rps */
#define	RET_BAD_ROT	74	/* invalid rotational delay */
#define	RET_BAD_MAGIC	75	/* bad magic number */
#define	RET_BAD_CPC	76	/* number of cylinder of cycle > MAXCPG*/
#define	RET_BAD_SPC	77	/* too many sectors per cylinder */

#define	RET_FSIZE_POW2	78	/* fragment size must be power of 2 */
#define	RET_FSIZE_SMALL	79	/* fragment size too small */

#define	RET_BSIZE_POW2	80	/* block size must be power of 2 */
#define	RET_BSIZE_SMALL	81	/* block size too small */
#define	RET_BSIZE_BIG	82	/* block size too big */
#define	RET_BF_SIZE	83	/* block size cannot be smaller than fragment size */

#define	RET_CPG_BAD	84	/* cpg not tolerable */
#define	RET_CPG_ONE	85	/* cylinder groups have at least 1 cylinder */
#define	RET_CPG_MAX	86	/* cylinder groups are limited to <max> cylinders */
#define	RET_CPG_MULT	87	/* cylinder groups have a multiple of cylinders */
#define	RET_CPG_LARGE	88	/* cylinder group too large */
#define	RET_CPG_MORE	89	/* number of cylinders per group must increase */

#define	RET_FS_ONE	90	/* file systems must have at least one cylinder */

#define	RET_INODE_RANGE	91	/* inode value out of range */
#define	RET_ARG_RANGE	92	/* argument out of range */
#define	RET_ARG_BAD	93	/* bad numeric arg */

#define	RET_TTY_OPEN	94	/* cannot open /dev/tty */


#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_MKFS_H */
