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

#ident	"@(#)kern-i386:fs/dosfs/dosfs_lbuf.c	1.1.2.1"

#include "dosfs.h"

lbuf_t *
lbread(dev_t dev, daddr_t blkno, long int bsize)
{
	lbuf_t	*lbufp;
	addr_t	partp;
	int	i;

	/* allocate lbuf and cluster buffer */
	lbufp = kmem_alloc((sizeof(lbuf_t) + bsize),KM_SLEEP);

	/* initialise lbuf */
	bzero((caddr_t)lbufp,sizeof(lbuf_t));
	lbufp->b_bsize = bsize;

	/* pointer to cluster buffer */
	lbufp->b_un.b_addr = (caddr_t)lbufp + sizeof(lbuf_t);

	/* init partial buffer pointer */
	partp = lbufp->b_un.b_addr;

	/* get all partial cluster buffers */
	for(i = 0; i < bsize / DEV_BSIZE; i++)
	{
		lbufp->bp[i] = bread(dev, blkno+i, DEV_BSIZE);
		bcopy(lbufp->bp[i]->b_un.b_addr, partp, DEV_BSIZE);
		partp += DEV_BSIZE;
	}

	return(lbufp);
}

lbuf_t *
lbreada(register dev_t dev, daddr_t blkno, daddr_t rablk, long int bsize)
{
	lbuf_t	*lbufp;
	addr_t	partp;
	int	i;



	/* allocate lbuf and cluster buffer */
	lbufp = kmem_alloc((sizeof(lbuf_t) + bsize),KM_SLEEP);

	/* initialise lbuf */
	bzero((caddr_t)lbufp,sizeof(lbuf_t));
	lbufp->b_bsize = bsize;

	/* pointer to cluster buffer */
	lbufp->b_un.b_addr = (caddr_t)lbufp + sizeof(lbuf_t);

	/* init partial buffer pointer */
	partp = lbufp->b_un.b_addr;

	/* get all partial cluster buffers */
	for(i = 0; i < bsize / DEV_BSIZE; i++)
	{
		/* non-optimal read-ahead */
		lbufp->bp[i] = breada(dev, blkno+i, rablk+i, DEV_BSIZE);
		bcopy(lbufp->bp[i]->b_un.b_addr, partp, DEV_BSIZE);
		partp += DEV_BSIZE;
	}

	return(lbufp);
}

lbuf_t *
lgetblk(register dev_t dev, daddr_t blkno, long int bsize)
{
	lbuf_t	*lbufp;
	addr_t	partp;
	int	i;

	/* allocate lbuf and cluster buffer */
	lbufp = kmem_alloc((sizeof(lbuf_t) + bsize),KM_SLEEP);

	/* initialise lbuf */
	bzero((caddr_t)lbufp,sizeof(lbuf_t));
	lbufp->b_bsize = bsize;

	/* pointer to cluster buffer */
	lbufp->b_un.b_addr = (caddr_t)lbufp + sizeof(lbuf_t);

	/* init partial buffer pointer */
	partp = lbufp->b_un.b_addr;

	/* get all partial cluster buffers */
	for(i = 0; i < bsize / DEV_BSIZE; i++)
	{
		lbufp->bp[i] = getblk(dev, blkno+i, DEV_BSIZE, 0);
		bcopy(lbufp->bp[i]->b_un.b_addr, partp, DEV_BSIZE);
		partp += DEV_BSIZE;
	}

	return(lbufp);
}


void
lbwrite(lbuf_t *lbp)
{
	addr_t	partp;
	int i;

	partp = lbp->b_un.b_addr;

	/* copy cluster buffer into partial buffers */
	for(i = 0; i < lbp->b_bsize / DEV_BSIZE; i++) {
		bcopy(partp, lbp->bp[i]->b_un.b_addr, DEV_BSIZE);
		bwrite(lbp->bp[i]);
		partp += DEV_BSIZE;
	}

	kmem_free(lbp,(sizeof(lbuf_t) + lbp->b_bsize));	/* free lbuf structure */

}

int
lbwritewait(lbuf_t *lbp)
{
	addr_t	partp;
	int	error;
	int	i;

	error = 0;
	partp = lbp->b_un.b_addr;

	/* copy cluster buffer into partial buffers */
	for(i = 0; i < lbp->b_bsize / DEV_BSIZE; i++) {
		bcopy(partp, lbp->bp[i]->b_un.b_addr, DEV_BSIZE);
		if (!error) 
			error = bwrite(lbp->bp[i]);
		else 
			(void) bwrite(lbp->bp[i]);
		partp += DEV_BSIZE;
	}

	/* free lbuf structure */
	kmem_free(lbp,(sizeof(lbuf_t) + lbp->b_bsize));

	/* report first error */
	return error;
}


void
lbawrite(lbuf_t *lbp)
{
	addr_t	partp;
	int i;

	partp = lbp->b_un.b_addr;

	/* copy cluster buffer into partial buffers */
	for(i = 0; i < lbp->b_bsize / DEV_BSIZE; i++) {
		bcopy(partp, lbp->bp[i]->b_un.b_addr, DEV_BSIZE);
		bawrite(lbp->bp[i]);
		partp += DEV_BSIZE;
	}

	kmem_free(lbp,(sizeof(lbuf_t) + lbp->b_bsize));	/* free lbuf structure */
}


void
lbdwrite(lbuf_t *lbp)
{
	addr_t	partp;
	int i;


	partp = lbp->b_un.b_addr;

	/* copy cluster buffer into partial buffers */
	for(i = 0; i < lbp->b_bsize / DEV_BSIZE; i++) {
		bcopy(partp, lbp->bp[i]->b_un.b_addr, DEV_BSIZE);
		bdwrite(lbp->bp[i]);
		partp += DEV_BSIZE;
	}

	kmem_free(lbp,(sizeof(lbuf_t) + lbp->b_bsize));	/* free lbuf structure */
}

void
lbrelse(lbuf_t *lbp)
{
	int i;

	/* release partial buffers */
	for(i = 0; i < lbp->b_bsize / DEV_BSIZE; i++) {
		brelse(lbp->bp[i]);
	}

	kmem_free(lbp,(sizeof(lbuf_t) + lbp->b_bsize));	/* free lbuf structure */
}

void
lclrbuf(lbuf_t *lbp)
{
	/* zero cluster buffer */
	bzero(lbp->b_un.b_addr,lbp->b_bsize);
}

int
lbiowait(lbuf_t *lbp)
{
	int i;
	int error = 0;

	/* biowait partial buffers */
	for(i = 0; i < lbp->b_bsize / DEV_BSIZE; i++) {
		if (!error)
			error = biowait(lbp->bp[i]);
		else
			biowait(lbp->bp[i]);
	}
	/* return first error found */
	return error;
}

void
lbiodone(lbuf_t *lbp)
{
	int i;

	/* biodone partial buffers */
	for(i = 0; i < lbp->b_bsize / DEV_BSIZE; i++) {
		biodone(lbp->bp[i]);
	}
}

/* use geterror kernel function in stead of geterror macro */
#undef geterror
extern int geterror();

int
lgeterror(lbuf_t *lbp)
{
	int i;
	int error = 0;

	/* geterror partial buffers */
	for(i = 0; i < lbp->b_bsize / DEV_BSIZE; i++) {
		if ((error = geterror(lbp->bp[i])) != 0) {
			break;
			/* report first error found */
		}
	}
	return error;
}

