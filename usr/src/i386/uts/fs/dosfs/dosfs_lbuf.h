#ifndef _FS_DOSFS_DOSFS_LBUF_H       /* wrapper symbol for kernel use */
#define _FS_DOSFS_DOSFS_LBUF_H       /* subject to change without notice */

#ident	"@(#)kern-i386:fs/dosfs/dosfs_lbuf.h	1.1.3.1"
#ident  "$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *
 *  Written for System V Release 4	(ESIX 4.0.4)	
 *
 *  Gerard van Dorth	(gdorth@nl.oracle.com)
 *  Paul Bauwens	(paul@pphbau.atr.bso.nl)
 *
 *  May 1993
 *
 *  This software is provided "as is".
 *
 *  The author supplies this software to be publicly
 *  redistributed on the understanding that the author
 *  is not responsible for the correct functioning of
 *  this software in any circumstances and is not liable
 *  for any damages caused by this software.
 *
 */

#include <mem/kmem.h>

/*
 * logical block I/O layer 
 * saves mapping junk for the moment
 */

/*
 * reserve pointers to handle 32K clustersize
 * with 16-bit FAT entries and 64 sectors per clusters, a maximum
 * of 2GB DOS partition can be supported. This is the maximum that
 * DOS 5 supports.
 *
 * 64K cluster * 64 sectors per clusters * 512 bytes per sector = 2GB
 */

#ifndef MAXCLUSTBLOCKS
#define MAXCLUSTBLOCKS	64
#endif

typedef struct	lbuf {
	union {
		caddr_t b_addr;	/* pointer to logical buffer */
	} b_un;
	size_t	b_bsize;	/* size of logical buffer    */
	buf_t	*bp[MAXCLUSTBLOCKS];	/* pointers to physical buffers */
} lbuf_t;


/* prototypes for dosfs low-level interface functions */

extern	lbuf_t *lbread(dev_t dev, daddr_t blkno, long int bsize);
extern	lbuf_t *lbreada(register dev_t dev, daddr_t blkno, daddr_t rablk, long int bsize);
extern	lbuf_t *lgetblk(register dev_t dev, daddr_t blkno, long int bsize);
extern	void	lbwrite(lbuf_t *lbp);
extern	void	lbawrite(lbuf_t *lbp);
extern	void	lbdwrite(lbuf_t *lbp);
extern	void	lbrelse(lbuf_t *lbp);
extern	void	lclrbuf(lbuf_t *lbp);
extern	int	lbiowait(lbuf_t *lbp);
extern	int	lbwritewait(lbuf_t *lbp);
extern	void	lbiodone(lbuf_t *lbp);
extern	int	lgeterror(lbuf_t *lbp);

#if defined(__cplusplus)
        }
#endif

#endif /* _FS_DOSFS_DOSFS_LBUF_H */

