#ident	"@(#)sp.c	1.2"
#ident	"$Header$"

/*
 * SP - Stream "pipe" device.  Any two minor devices may
 * be opened and connected to each other so that each user
 * is at the end of a single stream.  This provides a full
 * duplex communications path and allows for the passing
 * of file descriptors as well.  
 *
 * WARNING - an interprocess stream does not have the same
 * 	     semantics as a pipe, and this does not replace
 *	     pipes.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <svc/errno.h>
#include <io/stropts.h>
#include <io/conf.h>
#include <io/stream.h>

int spdevflag = 0;

STATIC int spopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp);
STATIC int spclose(queue_t *q, int cflag, cred_t *crp);
STATIC int spput(queue_t *q, mblk_t *bp);
STATIC int sprsrv(queue_t *q);
STATIC int spwsrv(queue_t *q);

STATIC struct module_info spm_info = 
		{1111, "sp", 0, INFPSZ, 5120, 1024 };
STATIC struct qinit sprinit =
		{ NULL, sprsrv, spopen, spclose, NULL, &spm_info, NULL };
STATIC struct qinit spwinit =
		{ spput, spwsrv, NULL, NULL, NULL, &spm_info, NULL};

struct streamtab spinfo = { &sprinit, &spwinit, NULL, NULL };

extern struct sp {
	queue_t *sp_rdq;		/* this stream's read queue */
	queue_t *sp_ordq;		/* other stream's read queue */
		} sp_sp[];
extern spcnt;

/*
 * STATIC int spopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
 *	open routine
 *
 * Calling/Exit State:
 */
/*ARGSUSED*/
STATIC int
spopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	struct sp *spp;
	minor_t	min;
	mblk_t *mp;
	struct stroptions *strop;

	min = geteminor(*devp);
	switch (sflag) {
	case MODOPEN:
		min = (struct sp *)q->q_ptr - sp_sp;
		break;

	case CLONEOPEN:
		for (min = 0, spp = sp_sp; 
			((min < spcnt) && spp->sp_rdq); min++, spp++);
		break;
	}
	if (((int)min < 0) || ((int)min >= spcnt)) 
		return(ENXIO);
	mp = allocb(sizeof(struct stroptions), BPRI_MED);
	if (mp == NULL)
		return(EAGAIN);
	/* LINTED pointer alignment */
	strop = (struct stroptions *) mp->b_wptr;
	strop->so_flags = SO_LOOP;
	mp->b_wptr += sizeof(struct stroptions);
	mp->b_datap->db_type = M_SETOPTS;
	putnext(q, mp);
	spp = &sp_sp[min];
	if (spp->sp_rdq == NULL) {
		spp->sp_rdq = q;
		q->q_ptr = WR(q)->q_ptr = (caddr_t)spp;
	}
	*devp = makedevice(getemajor(*devp), min);
	return(0);
}

/*
 * STATIC int spclose(queue_t *q, int cflag, cred_t *crp)
 *	close routine
 *
 * Calling/Exit State:
 */
/*ARGSUSED*/
STATIC int
spclose(queue_t *q, int cflag, cred_t *crp)
{
	struct sp *spp, *osp;
	queue_t *orq;
	mblk_t *mp;

	spp = (struct sp *)q->q_ptr;
	spp->sp_rdq = NULL;
	if ((orq = spp->sp_ordq) != NULL) {
		osp = (struct sp *)orq->q_ptr;
		osp->sp_ordq = NULL;
		spp->sp_ordq = NULL;
		if (mp = allocb(0, BPRI_MED)) { 
			mp->b_datap->db_type = M_HANGUP;
			putnext(orq, mp);
		} else
			/*
			 *+ Kernel could not allocate memory for                                         *+ a streams message.  This indicates
			 *+ that there is not enough physical
			 *+ memory on the machine or that memory                                         *+ is being lost by the kernel.
			 */
			cmn_err(CE_WARN, "sclose: could not allocate block\n");
	}
	q->q_ptr = WR(q)->q_ptr = NULL;
	return(0);
}

/*
 * STATIC int spput(queue_t *q, mblk_t *bp)
 * 	put procedure
 *
 * Calling/Exit State:
 */
STATIC int
spput(queue_t *q, mblk_t *bp)
{
	struct sp *spp;
	queue_t *oq;
	struct sp *osp;
	int i;

	spp = (struct sp *)q->q_ptr;

	switch (bp->b_datap->db_type) {

	case M_IOCTL:
		bp->b_datap->db_type = M_IOCNAK;
		qreply(q, bp);
		return(0);

	case M_FLUSH:
		/*
		 * The meaning of read and write sides must be reversed
		 * for the destination stream head.
		 */
		flushq(q, FLUSHDATA);
		if (spp->sp_ordq == NULL) {
			*bp->b_rptr &= ~FLUSHW;
			if (*bp->b_rptr & FLUSHR) 
				qreply(q, bp);
			return(0);
		}
		switch (*bp->b_rptr) {
		case FLUSHR: 
			*bp->b_rptr = FLUSHW; 
			break;

		case FLUSHW: 
			*bp->b_rptr = FLUSHR; 
			break;
		}
		putnext(spp->sp_ordq, bp);		
		return(0);

	case M_PROTO:
		if (spp->sp_ordq != NULL) {
			putq(q, bp);
			return(0);
		}
		if (bp->b_cont) 
			goto errout;
		if ((bp->b_wptr - bp->b_rptr) != sizeof(queue_t *))
			goto errout;
		/* LINTED pointer alignment */
		oq = *((queue_t **)bp->b_rptr);
		for (i = 0, osp = sp_sp; 
		     ((i < spcnt) && (oq != osp->sp_rdq)); i++, osp++);
		if (i == spcnt) 
			goto errout;
		if (osp->sp_ordq) 
			goto errout;

		spp->sp_ordq = oq;
		osp->sp_ordq = RD(q);
		freemsg(bp);
		return(0);

	default:
		if (spp->sp_ordq != NULL) {
			putq(q, bp);
			return(0);
		}
		break;
	}

errout:
	/*
	 * The stream has not been connected yet.
	 */
	bp->b_datap->db_type = M_ERROR;
	bp->b_wptr = bp->b_rptr = bp->b_datap->db_base;
	*bp->b_wptr++ = EIO;
	qreply(q, bp);
	return(0);
}

/*
 * STATIC int sprsrv(queue_t *q)
 *	read side service procedure.  Only called on backenable.
 *
 * Calling/Exit State:
 */

STATIC int
sprsrv(queue_t *q)
{
	struct sp *spp;

	spp = (struct sp *)q->q_ptr;
	if (spp->sp_ordq)
		qenable(WR(spp->sp_ordq));
	return(0);
}


/*
 * STATIC int spwsrv(queue_t *q)
 *	write side service procedure
 *
 * Calling/Exit State:
 */

STATIC int
spwsrv(queue_t *q)
{
	mblk_t *bp;
	struct sp *spp;

	spp = (struct sp *)q->q_ptr;
	while ((bp = getq(q)) != NULL) {
		if (spp->sp_ordq != NULL) {
			if (canputnext(spp->sp_ordq))
				putnext(spp->sp_ordq, bp);
			else {
				putbq(q, bp);
				return(0);
			}
		} else {
			/* other side disappeared */
			bp->b_datap->db_type = M_ERROR;
			bp->b_wptr = bp->b_rptr = bp->b_datap->db_base;
			*bp->b_wptr++ = EIO;
			qreply(q, bp);
			flushq(q, FLUSHALL);
		}
	}
	return(0);
}
