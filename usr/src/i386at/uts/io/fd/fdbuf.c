#ident	"@(#)fdbuf.c	1.8"
#ident	"$Header$"

#include <util/param.h>
#include <util/types.h>
#include <mem/kmem.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <fs/buf.h>
#include <svc/errno.h>

/** New headers for DDI support **/
#include <io/open.h>
#include <io/uio.h>
#include <proc/cred.h>

#include <svc/systm.h>
#include <io/elog.h>
#include <io/iobuf.h>
#include <fs/file.h>
#include <io/ioctl.h>

#include <util/cmn_err.h>
#include <io/dma.h>
#include <io/i8237A.h>
#include <io/cram/cram.h>
#include <io/fd/fd.h>
#include <io/vtoc.h>

#include <util/debug.h>
#include <util/inline.h>
#include <io/conf.h>

#include <util/mod/moddefs.h>

/** New headers for DDI support -- MUST COME LAST **/
#include <io/ddi.h>
#include <io/ddi_i386at.h>

void fdbufinit(void);
int fdbufgrow(unsigned , int );
extern	void	outb(int, uchar_t);

struct  fdbufstruct     fdbufstruct;
struct	fdstate		fd[NUMDRV];

caddr_t     fd_trk_bufp;       /* Track buffer */
paddr_t     fd_tbuf_paddr;     /* Track buffer physical address */
int         fd_tbuf_size;      /* Total memory allocated for trk buffer */

int	fdloaded = 0;

bcb_t *fdbcb;
bcb_t *fdwbcb;
bcb_t *fdtrkbcb;

/*
 * void
 * fdbufinit(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
fdbufinit(void)
{
	long	bus_type;
	unsigned char	ddtb;
	int		buf_size;

    /*
     * If fdloaded is 1 it means that fdinit() has initialized
     * the device, so don't reset the controller.
     */
    if(!fdloaded)   {
		drv_gethardware( IOBUS_TYPE, &bus_type );

		if ( bus_type == BUS_MCA ) {
			outb(FDCTRL, ENAB_MCA_INT);
		}
		else {
			outb(FDCTRL, ENABINT);
		}
	}

	fdwbcb = bcb_alloc(KM_NOSLEEP);
	fdwbcb->bcb_addrtypes = BA_KVIRT;
	fdwbcb->bcb_max_xfer = ptob(1);
	fdwbcb->bcb_granularity = NBPSCTR;
	fdwbcb->bcb_physreqp = physreq_alloc(KM_NOSLEEP);
/*
	fdwbcb->bcb_flags |= BCB_SYNCHRONOUS;
*/

	if (fdwbcb->bcb_physreqp == NULL) {
		return;
	}

	dma_physreq(DMA_CH2, DMA_PATH_8, fdwbcb->bcb_physreqp); 
	if (!physreq_prep(fdwbcb->bcb_physreqp, KM_NOSLEEP)) {
		physreq_free(fdwbcb->bcb_physreqp);
		return;
	}

	fdbcb = bcb_alloc(KM_NOSLEEP);
	fdbcb->bcb_addrtypes = BA_KVIRT;
	fdbcb->bcb_max_xfer = ptob(16);
	fdbcb->bcb_granularity = NBPSCTR;
	fdbcb->bcb_physreqp = physreq_alloc(KM_NOSLEEP);
	fdbcb->bcb_flags &= ~BCB_EXACT_SIZE;
/*
	fdbcb->bcb_flags |= BCB_ONE_PIECE;
	fdbcb->bcb_flags |= BCB_SYNCHRONOUS;
*/

	if (fdbcb->bcb_physreqp == NULL) {
		return;
	}
	if (!physreq_prep(fdbcb->bcb_physreqp, KM_NOSLEEP)) {
		physreq_free(fdbcb->bcb_physreqp);
		return;
	}

	fdtrkbcb = bcb_alloc(KM_NOSLEEP);
	fdtrkbcb->bcb_addrtypes = BA_KVIRT;
	fdtrkbcb->bcb_max_xfer = ptob(5);
	fdtrkbcb->bcb_granularity = NBPSCTR;
	fdtrkbcb->bcb_physreqp = physreq_alloc(KM_NOSLEEP);

	if (fdtrkbcb->bcb_physreqp == NULL) {
		return;
	}

	dma_physreq(DMA_CH2, DMA_PATH_8, fdtrkbcb->bcb_physreqp); 
	if (!physreq_prep(fdtrkbcb->bcb_physreqp, KM_NOSLEEP)) {
		physreq_free(fdtrkbcb->bcb_physreqp);
		return;
	}

	fd_tbuf_size = ptob(5);
	fd_trk_bufp = (caddr_t)kmem_zalloc_physreq(fd_tbuf_size, 
					fdtrkbcb->bcb_physreqp, KM_NOSLEEP);
	fd_tbuf_paddr = vtop(fd_trk_bufp, NULL);

	fd[0].fd_intlv_tbl = (char *)kmem_zalloc(64, KM_NOSLEEP);
	fd[1].fd_intlv_tbl = (char *)kmem_zalloc(64, KM_NOSLEEP);

	fdbufstruct.fbs_size = 0;
	fdbufstruct.fbs_addr = (caddr_t)NULL;
}

/*
 * int
 * fdbufgrow(unsigned bytesize, int sleepflag)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdbufgrow(unsigned bytesize, int sleepflag)
{
	/* Allocate a buffer to hold  bytesize  bytes  without */
	/* crossing a dma boundary.                                  */

	/* We can't handle more than 64k, since this has to cross  a */
	/* dma boundary.                                             */
	if ( bytesize > 0x10000 || bytesize == 0 )
	    return (EINVAL);

	/* We already have enough mem. */
	if (bytesize <= fdbufstruct.fbs_size)
	    return (0);

	if ( (int)fdbufstruct.fbs_size > 0 ) {
	    kmem_free(fdbufstruct.fbs_addr, fdbufstruct.fbs_size);
	    fdbufstruct.fbs_size = 0;
	    fdbufstruct.fbs_addr = (caddr_t)NULL;
	}

	/* Get the contiguous pages */
	fdbufstruct.fbs_addr = kmem_alloc_physreq(bytesize, 
				fdtrkbcb->bcb_physreqp, sleepflag);

	if (fdbufstruct.fbs_addr == NULL) {
	    return (EAGAIN);
	}

	fdbufstruct.fbs_size = bytesize;
	return (0);
}
