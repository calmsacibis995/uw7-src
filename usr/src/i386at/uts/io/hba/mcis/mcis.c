#ident	"@(#)kern-pdi:io/hba/mcis/mcis.c	1.78.3.1"
#ident	"@(#)kern-pdi:io/hba/mcis/mcis.c	1.78.3.1"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1991
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */


#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <proc/cred.h>
#include <util/sysmacros.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <proc/signal.h>
#include <proc/cred.h>
#include <svc/bootinfo.h>
#include <io/conf.h>
#include <io/dma.h>
#include <io/uio.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/dynstructs.h>
#include <io/hba/mcis/mcis.h>
#include <io/hba/hba.h>
#include <util/mod/moddefs.h>

/* These must come last: */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#define		DRVNAME	"mcis - MCA SCSI HBA driver"

#define		MCIS_MEMALIGN	512
#define MCIS_BLKSHFT	  9	/* PLEASE NOTE:  Currently pass-thru	    */
#define MCIS_BLKSIZE	512	/* SS_READ/SS_WRITE, SM_READ/SM_WRITE	    */
				/* supports only 512 byte blocksize devices */

#if PDI_VERSION >= PDI_UNIXWARE20

extern	HBA_IDATA_STRUCT	_mcis_idata[];
HBA_IDATA_STRUCT	*mcis_idata;
#define KMEM_ZALLOC	mcis_kmem_zalloc_physreq
STATIC void * 		mcis_kmem_zalloc_physreq (size_t size, int flags);

#else /* PDI_VERSION < PDI_UNIXWARE20 */

extern	HBA_IDATA_STRUCT	mcis_idata[];
#define KMEM_ZALLOC	kmem_zalloc

#endif /* PDI_VERSION < PDI_UNIXWARE20 */

extern void mod_drvattach(), mod_drvdetach();

int        mcis_getld(), mcis_docmd(); 
int	    mcis_wait(), mcis_findhba(), mcis_pollret();

STATIC void mcis_set_bsize(struct mcis_luq *q, struct sb *sp);
int	mcis_init();
int	mcis_start();
void	mcis_intr();

#define MCIS_BSIZE(sp, q) (sp->s_sbp->sb.SCB.sc_datasz>>q->q_shift)
#define MCIS_IS_READ(c)         (c == SM_READ)
#define MCIS_IS_WRITE(c)        (c == SM_WRITE)
#define MCIS_IS_RW(c) (MCIS_IS_READ(c) || MCIS_IS_WRITE(c))
#define MCIS_CMDPT(sp) (*((char *)sp->s_sbp->sb.SCB.sc_cmdpt))

MOD_HDRV_WRAPPER(mcis_, mcis_load, mcis_unload, NULL, DRVNAME);

HBA_INFO(mcis_, 0, 0x10000);

static	int	mcis_dynamic = 0;
int	mcis_first_disk = 0;

/*
** Global Variables from space.c
*/

extern int	mcis_cache;	/* = 0                        */
extern int	mcis__cntls;	/* = MCIS__CNTLS from idtools */
extern int	mcis_global_tout;	/* mcis board timeout */

#if PDI_VERSION >= PDI_UNIXWARE20

int	mcis__mca_conf();

STATIC int
mcis_load()
{
	mcis_dynamic = 1;

	if( mcis_start()) {
		sdi_acfree(mcis_idata, mcis__cntls);
		return( ENODEV );
	}
	return(0);
}

#else /* PDI_VERSION < PDI_UNIXWARE20 */

STATIC int
mcis_load()
{
	void	mod_drvattach();
	mcis_dynamic = 1;

	mod_drvattach( &mcis__attach_info );
	if( mcis_start()) {
		mod_drvdetach( &mcis__attach_info );
		return( ENODEV );
	}
	return(0);
}

#endif /* PDI_VERSION < PDI_UNIXWARE20 */

STATIC int
mcis_unload()
{
	return(EBUSY);
}


#ifdef DEBUG

#define MCIS_DENTRY	0x0001
#define MCIS_DINTR	0x0002
#define MCIS_DB_ENT_EXT	0x0004
#define MCIS_DB_DETAIL	0x0008
#define MCIS_DB_MISC	0x0010

ulong	mcis_debug = 0;

#endif /* DEBUG */

int	nbpp;
#define pgbnd(a) (nbpp - ((nbpp - 1) & (int )(a)))

extern struct mcis_blk	*mcis_cfginit();
extern int		mcis_hbainit();
extern long		mcis_send();
extern void		mcis_putq();
extern void		mcis_dmalist();
extern void		mcis_next();
extern void		mcis_sfbcmd();
extern void		mcis_getedt();
extern void		mcis_initld();
extern void		mcis_inqcmd();
extern void		mcis_poscmd();
extern void		mcis_invld();
extern int		mcis_waitedt();
extern void		mcis_pass_thru();
extern void		mcis_int();
extern void		mcis_scbcmd();
extern void		mcis_flushq();
extern void		mcis_freeld();

#ifndef PDI_SVR42
extern void		mcis_pass_thru0();
#else
#define mcis_pass_thru0 mcis_pass_thru
#endif

/* 
** Global variables from sdi.c 
** sm_poolhead is needed for the dynamic struct allocation routines to alloc
** the small 28 byte structs for mcis_srb structs
*/
extern struct head	sm_poolhead;

/*
** Global Variables
*/

int		mcis__devflag = 0;

struct ver_no	mcis_ver;

struct mcis_blk	*mcis_ha[ MAX_HAS ];		/* one per board */
int		mcis_lds_assigned = FALSE;
#define MAX_CMDSZ 12			/* currently in 3 headers */
int		mcis_gtol[ MAX_HAS ];
int		mcis_ltog[ MAX_HAS ];
int		mcis_hacnt; 		/* number of active HA's */
int		mcis_polltime = TRUE;
/*
 *	The array declared below, mcis_tcn_xlat, is used to translate all
 *	incoming and outgoing references to target controller number.
 *
 *	It is used to force the highest ID_RANDOM target-id on the SCSI bus 
 *	to be treated as target 0 by UNIX.  This is to make sure we can 
 *	always boot from this target.
 *
 *	This hack is neccessary because UNIX always treats the first DISK
 *	device ( in target number order ) as the boot device but the Model 90
 *	MUST have the boot disk at target 6.  If you add a second disk, it
 *	will therefore have to be at a lower target number and the boot
 *	will fail.
 *
 *	DO NOT CHANGE this array unless all you do is swap pairs of numbers.
 *	Doing so will break this because the same array is used to translate
 *	into and out-of the driver and this will only work if values are
 *	swapped in pairs.  If values are rotated in some manner, there must
 *	be a different array for gtol and ltog translations.
 */
int		mcis_tcn_xlat[] = { 0, 1, 2, 3, 4, 5, 6, 7 };



/*
** Function name: mcis_open()
**
** Description:
**	Open host adapter driver.  May be pass-thru open to a specific
**	LU if the specified device id is not equal to the board's id.
*/

/* ARGSUSED */
int
mcis_open( devp, flags, otype, cred_p )
dev_t	*devp;
int	flags;
int	otype;
cred_t	*cred_p;
{
	struct mcis_luq	*q;
	int		c;
	int		t;
	pl_t		opri;
	int		error;
	int		minor = geteminor( *devp );

	c = mcis_gtol[ MCIS_HAN( minor )];
	t = mcis_tcn_xlat[ MCIS_TCN( minor ) ];

	if ( t == mcis_ha[ c ]->b_targetid )	/* Regular open */
		return 0;

	/* pass-thru open to particular lu */

	q = MCIS_LU_Q( c, t, MCIS_LUN( minor ));

	error = EBUSY;
	opri = spldisk();

	if ( q->q_count == 0
		&& ! ( q->q_flag & ( MCIS_QLUBUSY | MCIS_QSUSP | MCIS_QPTHRU )))
	{
		q->q_flag |= MCIS_QPTHRU;
		error = 0;
	}

	splx( opri );
	return error;
}

/*
** Function name: mcis_close()
**
** Description:
**	Close connection to board or LU if pass-thru close.
*/

/* ARGSUSED */
int
mcis_close( dev, flags, otype, cred_p )
dev_t	dev;
int	flags;
int	otype;
cred_t	*cred_p;
{
	int		minor = geteminor( dev );
	int		c = mcis_gtol[ MCIS_HAN( minor )];
	int		t = mcis_tcn_xlat[ MCIS_TCN( minor ) ];
	pl_t		opri;
	struct mcis_luq	*q;

	if ( t == mcis_ha[ c ]->b_targetid )
		return 0;

	/* pass-thru close */

	q = MCIS_LU_Q( c, t, MCIS_LUN( minor ));

	opri = spldisk();

	q->q_flag &= ~MCIS_QPTHRU;

	/*
	** This is needed in case requests came in via
	** mcis_icmd() after pass-thru open. 
	*/

	mcis_next( q );
	splx( opri );
	return 0;
}

/*
** Function name: mcis_ioctl()
**
** Description:
*/

/* ARGSUSED */
int
mcis_ioctl( dev, cmd, arg, mode, cred_p, rval_p )
dev_t	dev;
int	cmd;
caddr_t	arg;
int	mode;
cred_t	*cred_p;
int	*rval_p;
{
	int			minor = geteminor( dev );
	register int		c = mcis_gtol[ MCIS_HAN( minor )];
	register int		l = SC_LUN(dev);
	int			error = 0;
	pl_t			opri;

	/* uid = 0 is required: it's enforced in open */

	switch( cmd )
	{
		case SDI_SEND:
		{
			register int		t = mcis_tcn_xlat[ MCIS_TCN( minor ) ];
			register struct sb	*sp;
			register buf_t		*bp;
			char			*save_priv;
			struct sb		karg;
			int			rw;
			struct mcis_luq		*q;

			/* Only allowed for pass-thru */

			if ( t == mcis_ha[ c ]->b_targetid )
			{
				error = ENXIO;
				break;
			}

			if ( copyin( arg, (caddr_t)&karg, sizeof( struct sb )))
			{
				error = EFAULT;
				break;
			}

			/* ISCB Only - specified in SDI Manual */

			if ( karg.sb_type != ISCB_TYPE
				|| karg.SCB.sc_cmdsz <= 0
				|| karg.SCB.sc_cmdsz > MAX_CMDSZ )
			{ 
				error = EINVAL;
				break;
			}

			sp = sdi_getblk(KM_SLEEP);
			save_priv = sp->SCB.sc_priv;
			bcopy( (caddr_t)&karg, (caddr_t)sp, sizeof( struct sb ));
			bp = getrbuf(KM_SLEEP);
			opri = spldisk();
			bp->b_iodone = NULL;
			sp->SCB.sc_priv = save_priv;

			q = MCIS_LU_Q(c, t, l);
			if (q->q_sc_cmd == NULL) {
			/*
			 * Allocate space for the SCSI command and add 
			 * sizeof(int) to account for the scm structure
			 * padding, since this pointer may be adjusted -2 bytes
			 * and cast to struct scm.  After the adjustment we
			 * still want to be pointing within our allocated space!
			 */
				q->q_sc_cmd = (char *)KMEM_ZALLOC(MAX_CMDSZ + 
					sizeof(int), KM_SLEEP) + sizeof(int);
			}
			sp->SCB.sc_cmdpt = q->q_sc_cmd;
	
			if ( copyin( karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
							sp->SCB.sc_cmdsz ))
			{
				error = EFAULT;
				goto done;
			}
			sp->SCB.sc_dev.sa_lun = (unchar) l;
			sp->SCB.sc_dev.sa_fill = ( mcis_ltog[ c ] << 3 ) 
						 | mcis_tcn_xlat[ t ];
	
			rw = ( sp->SCB.sc_mode & SCB_READ ) ? B_READ : B_WRITE;
#ifdef PDI_SVR42
			bp->b_private = (char *)sp;
#else
			bp->b_priv.un_ptr = sp;
#endif
	
			/*
			 * If the job involves a data transfer then the
			 * request is done thru physio() so that the user
			 * data area is locked in memory. If the job doesn't
			 * involve any data transfer then mcis_pass_thru()
			 * is called directly.
			 */

			if ( sp->SCB.sc_datasz > 0 )
			{ 
				struct iovec	ha_iov;
				struct uio	ha_uio;
				char op;
	
				ha_iov.iov_base = sp->SCB.sc_datapt;	
				ha_iov.iov_len = sp->SCB.sc_datasz;	
				ha_uio.uio_iov = &ha_iov;
				ha_uio.uio_iovcnt = 1;
				ha_uio.uio_offset = 0;
				ha_uio.uio_segflg = UIO_USERSPACE;
				ha_uio.uio_fmode = 0;
				ha_uio.uio_resid = sp->SCB.sc_datasz;
				op = q->q_sc_cmd[0];
#if (PDI_VERSION >= PDI_UNIXWARE20)
				/*
				 * Save the starting block number, if r/w.
				 */
				if (op == SS_READ || op == SS_WRITE) {
					struct scs *scs = 
					   (struct scs *)(void *)q->q_sc_cmd;
					bp->b_priv2.un_int =
					   ((sdi_swap16(scs->ss_addr) & 0xffff)
					   + (scs->ss_addr1 << 16));
				}
				if (op == SM_READ || op == SM_WRITE) {
					struct scm *scm = 
					(struct scm *)(void *)((char *)q->q_sc_cmd - 2);
					bp->b_priv2.un_int =
					   sdi_swap32(scm->sm_addr);
				}
#else /* (PDI_VERSION < PDI_UNIXWARE20) */
				/*
				 * Please NOTE: 
				 * UnixWare 1.1 has 2Gig passthru limit
				 * due to physiock offset limitation.
				 */
				if (op == SS_READ || op == SS_WRITE) {
					struct scs *scs =
					   (struct scs *)(void *)q->q_sc_cmd;
					ha_uio.uio_offset = 
					   ((sdi_swap16(scs->ss_addr) & 0xffff)+
					   (scs->ss_addr1 << 16)) * MCIS_BLKSIZE;
				}
				if (op == SM_READ || op == SM_WRITE) {
					struct scm *scm =
					(struct scm *)(void *)((char *)q->q_sc_cmd - 2);
					ha_uio.uio_offset = 
					   sdi_swap32(scm->sm_addr) * MCIS_BLKSIZE;
				}
#endif /* (PDI_VERSION < PDI_UNIXWARE20) */
				error = physiock(mcis_pass_thru0, bp,
						dev, rw, HBA_MAX_PHYSIOCK,
						&ha_uio);
				if (error)
					goto done;
			}
			else
			{
				bp->b_un.b_addr = sp->SCB.sc_datapt;
				bp->b_bcount = sp->SCB.sc_datasz;
				bp->b_blkno = NULL;
				bp->b_edev = dev;
				bp->b_flags |= rw;
	
				mcis_pass_thru( bp );  /* bypass physio call */
				biowait( bp );
			}
	
			/* update user SCB fields */
	
			karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
			karg.SCB.sc_status = sp->SCB.sc_status;
			karg.SCB.sc_time = sp->SCB.sc_time;
	
			if ( copyout( (caddr_t)&karg, arg, sizeof( struct sb )))
				error = EFAULT;

done:			splx( opri );
			freerbuf( bp );
			sdi_freeblk( sp );
			break;
		}
	
		case B_GETTYPE:
		{
			if ( copyout( "scsi",
				((struct bus_type *)arg)->bus_name, 5 ))
			{
				error = EFAULT;
				break;
			}

			if ( copyout( "mcis_",
				((struct bus_type *)arg)->drv_name, 6 ))
			{
				error = EFAULT;
			}

			break;
		}

		case	HA_VER:

			if ( copyout( (caddr_t)&mcis_ver, arg,
						sizeof( struct ver_no )))
				error = EFAULT;

			break;

		case SDI_BRESET:
		{
			struct mcis_blk	*mcis_blkp = mcis_ha[ c ];
			int		ctrl_addr = mcis_blkp->b_ioaddr
								+ MCIS_CTRL;
			int		i = 6;

			opri = spldisk();

			if ( mcis_blkp->b_npend > 0 )	/* outstanding jobs */
				error = EBUSY;
			else
			{
				cmn_err( CE_WARN,
					"!IBM MCA Host Adapter: HA %d - Bus is being reset\n",
					mcis_ltog[ c ]);

				outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE
							| ISCTRL_RESET );


				while (i--)
					drv_usecwait(10);

				/* re-enable both dma and intr */

				outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE
						| ISCTRL_EDMA | ISCTRL_EINTR );

			}

			splx( opri );
			break;
		}

		case SDI_TRESET:
		default:

			error = EINVAL;
	}

	return( error );
}

/*
** Function name: mcis_getinfo()
**
** Description:
**	Return the name and iotype of the given device.  The name is copied
**	into a string pointed to by the first field of the getinfo structure.
*/

void
mcis_getinfo( sa, getinfo )
struct scsi_ad	*sa;
struct hbagetinfo	*getinfo;
{
 	register char	*s1, *s2;
	static char	temp[] = "HA X TC X";

#ifdef MCIS_DEBUG
	 if( mcis_debug > 0 )
		cmn_err(CE_CONT,  "mcis_getinfo( 0x%x, %s )\n", sa, name );
#endif

	s1 = temp;
	s2 = getinfo->name;
	temp[ 3 ] = SDI_HAN( sa ) + '0';
	temp[ 8 ] = SDI_TCN( sa ) + '0';

	while (( *s2++ = *s1++ ) != '\0' )
		;
	getinfo->iotype = F_DMA_32 | F_SCGTH;
#ifndef PDI_SVR42
	if (getinfo->bcbp) {
		getinfo->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfo->bcbp->bcb_flags = 0;
		getinfo->bcbp->bcb_max_xfer = mcis_hba_info.max_xfer;
		getinfo->bcbp->bcb_physreqp->phys_align = MCIS_MEMALIGN;
		getinfo->bcbp->bcb_physreqp->phys_boundary = 0;
		getinfo->bcbp->bcb_physreqp->phys_dmasize = 32;
	}
#endif
}

/*
** Function name: mcis_getblk()
**
** Description:
**	Allocate a SB structure for the caller.	 The function calls
**	sdi_get to return a struct will be cast as mcis_srb.
*/

struct hbadata *
mcis_getblk(sleepflag)
int sleepflag;
{
	struct mcis_srb	*srbp;

	srbp = (struct mcis_srb *) (void *) sdi_get(&sm_poolhead, sleepflag);
	return (struct hbadata *)(void *)srbp;
}

/*
** Function name: mcis_freeblk()
**
** Description:
**	Frees up hbadata/mcis_srb by calling sdi_free which returns
**	struct back to the pool.
*/

long
mcis_freeblk( hbap )
register struct hbadata	*hbap;
{
	struct mcis_srb	*srbp = (struct mcis_srb *) (void *) hbap;

	sdi_free(&sm_poolhead, (jpool_t *)(void *)srbp);
	return SDI_RET_OK;
}


/*
** Function name: mcis_xlat()
**
** Description:
**	Perform the virtual to physical translation on the SCB data pointer,
**	possibly creating scatter/gather list if buffer spans physical pages
**	and is non-contiguous.
*/

/* ARGSUSED */
int
mcis_xlat( hbap, flag, procp, sleepflag )
register struct hbadata	*hbap;
int			flag;
struct proc		*procp;
int			sleepflag;
{
	struct mcis_srb	*srbp = (struct mcis_srb *)hbap;
	struct xsb	*sbp = srbp->s_sbp;

#ifdef MCIS_DEBUG
	if ( mcis_debug > 0 )
		cmn_err(CE_CONT,
		"mcis_xlat( 0x%x, 0x%x, 0x%x )\n", hbap, flag, procp );
#endif /* MCIS_DEBUG */

	/*
	** The IBM MCA SCSI board supports linked commands, but the
	** SVR4 SDI manual says they are not supported.
	*/

	if ( sbp->sb.SCB.sc_link )
	{
		cmn_err( CE_WARN,
		   "!IBM MCA Host Adapter: Linked commands NOT available\n");

		sbp->sb.SCB.sc_link = NULL;
	}

	if ( sbp->sb.SCB.sc_datapt && (sbp->sb.SCB.sc_datasz > 0) )
	{
		srbp->s_addr = vtop( sbp->sb.SCB.sc_datapt, procp );
		srbp->s_size = sbp->sb.SCB.sc_datasz;
		srbp->s_proc = procp;
	}
	else
	{
		srbp->s_addr = 0;
		srbp->s_size = 0;
	}

#ifdef MCIS_DEBUG
	if ( mcis_debug > 3 )
		cmn_err(CE_CONT,  "mcis_xlat: returning\n" );
#endif
	return (SDI_RET_OK);
}

/*
** Function name: mcis_init()
**
** Description:
*/

int
mcis_init( )
{
	return(0);
}

int
mcis_start()
{
	static int	mcisfirst_time = TRUE;
	int		cntl_num, c, i, j;
	uint	bus_p;
	struct	mcis_blk	*blkp;
	int	ctrl_addr;

#ifdef MCIS_MLDEBUG
	cmn_err(CE_CONT,  "mcis_bdinit entry\n" );
#endif

	if (!mcisfirst_time) {
#ifdef MCIS_MLDEBUG
		cmn_err(CE_WARN,"MCIS init called more than once");
#endif
		return(-1);
	}
	nbpp = ptob(1);
	/* if not running in a micro-channel machine, skip initialization */
	if (!drv_gethardware(IOBUS_TYPE, &bus_p) && (bus_p != BUS_MCA))
		return(-1);

#if PDI_VERSION >= PDI_UNIXWARE20

	mcis_idata = sdi_hba_autoconf("mcis", _mcis_idata, &mcis__cntls);
	if(mcis_idata == NULL)	{
		return (-1);
	}
	sdi_mca_conf(mcis_idata, mcis__cntls, mcis__mca_conf);

	for (c = 0; c < mcis__cntls; c++) {
		mcis_idata[c].version_num &= ~HBA_VMASK;
		mcis_idata[c].version_num |= PDI_VERSION;
	}

#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

	for ( i = 0; i < MAX_HAS; i++ )
		mcis_gtol[ i ] = mcis_ltog[ i ] = -1;

	for (c = 0; c < mcis__cntls; c++) {

		if (!mcis_hbainit(c))	{
			continue;
		}
		else {
			if ((cntl_num = sdi_gethbano(mcis_idata[c].cntlr)) <= -1) {
				cmn_err(CE_WARN,"%s: No HBA number.",mcis_idata[c].name);
				continue;
			}
			mcis_idata[c].cntlr = cntl_num;
			mcis_gtol[cntl_num] = c;
			mcis_ltog[c] = cntl_num;

			if (mcis_hacnt == 0) {
				mcis_tcn_xlat[0] = mcis_first_disk;
				mcis_tcn_xlat[mcis_first_disk] = 0;
			}

			if ((cntl_num=sdi_register(&mcis_hba_info,&mcis_idata[c])) < 0) {
				cmn_err(CE_WARN,"!%s: HA %d SDI register slot %d failed",
                                	mcis_idata[c].name, c, cntl_num);
                                	continue;
			}
			mcis_idata[c].active = 1;

			mcis_hacnt++;

			/* enable dma and intr */
			blkp = mcis_ha[c];
			ctrl_addr = blkp->b_ioaddr + MCIS_CTRL;
			outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE | 
				ISCTRL_EDMA | ISCTRL_EINTR );
		}
	}
	if (mcis_hacnt == 0)	{
		cmn_err(CE_NOTE,"!%s: No HA's found.", mcis_idata[0].name);
		return(-1);
	}

#if PDI_VERSION >= PDI_UNIXWARE20

	sdi_intr_attach(mcis_idata, mcis__cntls, mcis_intr, mcis__devflag);

#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

	mcisfirst_time = FALSE;
	mcis_ver.sv_release = 1;
	mcis_ver.sv_machine = SDI_386_MCA;
	mcis_ver.sv_modes = SDI_BASIC1;

 	/*
        ** Clear init time flag to stop the HA driver
	** from polling for interrupts and begin taking
	** interrupts normally.
	*/

	mcis_polltime = FALSE;

	/* Find devices that should be scheduled */

	for (c=0; c < mcis__cntls; c++) {
		if (!mcis_idata[c].active)
			continue;
		for (i=0; i<=7; i++) {
			for (j=0; j<=7; j++) {
				struct sdi_edt *edtp;
				edtp = sdi_redt(mcis_ltog[c], i, j);
				if (edtp && edtp->pdtype == ID_RANDOM) {
					/* Other device types can potentially
					 * be scheduled, but the scheduling
					 * algorithm has been tuned only for
					 * disks.
					 */
					struct mcis_luq *q = MCIS_LU_Q(c, i, j);
					q->q_flag |= MCIS_QSCHED;
				}
			}
		}
	}

#ifdef MCIS_MLDEBUG
	cmn_err(CE_CONT, "mcis_bdinit leave\n");
#endif
	return 0;
}


/*
** hba scsi command send routine
**
** This routine only accepts SCB_TYPE commands
*/

long
mcis_send( hbap )
struct hbadata *hbap;
{
	struct mcis_srb	*srbp = (struct mcis_srb *)hbap;
	struct xsb	*sbp = srbp->s_sbp;
	struct mcis_luq	*q;
	struct scsi_ad	*sa;
	long		error = SDI_RET_OK;
	pl_t		opri;

#ifdef DEBUG
 {
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		cmn_err(CE_CONT,  "mcis_send( 0x%x )\n", hbap );

	if ( sbp->sb.sb_type != SCB_TYPE )
	{
		cmn_err(CE_CONT,  "mcis_send: bad sb_type\n" );
		return SDI_RET_ERR;
	}
 }
#endif /* DEBUG */

	sa = &sbp->sb.SCB.sc_dev;

	q = MCIS_LU_Q( mcis_gtol[ SDI_HAN( sa )], mcis_tcn_xlat[ SDI_TCN( sa ) ], SDI_LUN( sa ));

	opri = spldisk();

	if ( q->q_flag & MCIS_QPTHRU )
	{
		error = SDI_RET_RETRY;
	}
	else
	{
		mcis_putq( q, srbp );
		mcis_next( q );
	}

	splx( opri );

#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		cmn_err(CE_CONT,  "mcis_icmd: exiting\n" );
#endif

	return error;
}


/*
** Function name: mcis_icmd()
**
** Description:
**	Send an immediate command.  If the logical unit is busy, the job
**	will be queued until the unit is free.  SFB operations will take
**	priority over SCB operations.
*/

long
mcis_icmd( hbap )
struct hbadata *hbap;
{
	struct mcis_srb	*srbp = (struct mcis_srb *)hbap;
	struct xsb	*sbp = srbp->s_sbp;
	struct scsi_ad	*sa;
	struct mcis_luq	*q;
	register int	c;
	register int	t;
	register int	l;
	pl_t		opri;
	int		sb_type = sbp->sb.sb_type;
	struct scs	*inq_cdb;

#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		cmn_err(CE_CONT,  "mcis_icmd( 0x%x )\n", hbap );
#endif

	if ( sb_type == SFB_TYPE )
		sa = &sbp->sb.SFB.sf_dev;
	else
		sa = &sbp->sb.SCB.sc_dev;

	c = mcis_gtol[ SDI_HAN( sa )];
	t = mcis_tcn_xlat[ SDI_TCN( sa ) ];
	q = MCIS_LU_Q( c, t, SDI_LUN( sa ));

#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_DETAIL )
		cmn_err(CE_CONT,
		"mcis_icmd: c = %d t = %d l = %d\n", c, t, sa->sa_lun );
#endif

	opri = spldisk();

	switch ( sb_type )
	{
		case SFB_TYPE:

			sbp->sb.SFB.sf_comp_code = SDI_ASW;
	
			switch ( sbp->sb.SFB.sf_func ) 
			{
				case SFB_RESUME:

					q->q_flag &= ~MCIS_QSUSP;
					mcis_next( q );
					break;

				case SFB_SUSPEND:

					q->q_flag |= MCIS_QSUSP;
					break;

				case SFB_ABORTM:
				case SFB_RESETM:

					mcis_putq( q, srbp );
					mcis_next( q );
					splx( opri );
					return SDI_RET_OK;

				case SFB_FLUSHR:

					mcis_flushq( q, SDI_QFLUSH, 0 );
					break;

				case SFB_NOPF:

					break;

				default:

					sbp->sb.SFB.sf_comp_code = SDI_SFBERR;
					break;
			}
	
			sdi_callback( &sbp->sb );
			splx( opri );
			return SDI_RET_OK;
	
		case ISCB_TYPE:

			inq_cdb = (struct scs *)(void *)sbp->sb.SCB.sc_cmdpt;
			l = SDI_LUN( sa );

			if ( inq_cdb->ss_op == SS_INQUIR )
			{
				struct ident *inq_data;	/* To addr	*/
				struct ident *ident;	/* From addr	*/
				struct ident inq;	/* tmp ident buf*/
				int inq_len;		/* user buf size*/

				inq_data = (struct ident *)(void *)sbp->sb.SCB.sc_datapt;
				inq_len = sbp->sb.SCB.sc_datasz;

				/* Is inquiry data request for the host adapter ? */

				if ( t == mcis_ha[ c ]->b_targetid )
				{
					if ( l )	/* fail if lu != 0 */
					{
						sbp->sb.SCB.sc_comp_code = SDI_SCBERR;
						splx( opri );
						sdi_callback( &sbp->sb );
						return SDI_RET_OK;
					}
	
					bzero(&inq, sizeof(struct ident));
					ident = &inq;
					inq.id_type = ID_PROCESOR;
	
					(void)strncpy( inq.id_vendor,
						mcis_idata[c].name, 
						VID_LEN+PID_LEN+REV_LEN );
	
				}
				else
				{
					ident = &mcis_ha[ c ]->b_scb_array[(t << 3) | l]->s_ident;
				}

				bcopy((char *)ident, (char *)inq_data, inq_len);
				sbp->sb.SCB.sc_comp_code = SDI_ASW;
				splx( opri );
				sdi_callback( &sbp->sb );
				return SDI_RET_OK;
			}
	
			mcis_putq( q, srbp );
			mcis_next( q );
			splx( opri );
			return SDI_RET_OK;
	
		default:

			splx( opri );
			sdi_callback( &sbp->sb );
			return SDI_RET_ERR;
	}
}

/*
 *	interrupt handler
 */

void
mcis_intr( vect )
int	vect;
{
	register struct	mcis_blk	*blkp;
	struct xsb			*sbp;
	register struct mcis_srb	*srbp;
	register struct	mcis_scb	*scbp;
	struct mcis_tsb			*tsbp;
	unsigned long			*comp_code;
	char				status;
	char				*sp = &status;
	ushort				ioaddr;
	int				scb_type = 1;
	int				c;
#ifdef MCIS_MLDEBUG3
	static	mcis_intrcnt = 0;

	mcis_intrcnt++;
#endif

	/* Determine which host adapter interrupted */

	for ( c = 0; c < mcis_hacnt; c++ )
	{
		blkp = mcis_ha[ c ];

		if ( blkp->b_intr != vect )
			continue;

		if (inb(blkp->b_ioaddr + MCIS_STAT) & ISSTAT_INTRHERE)
			break;
	}

	if ( c == mcis_hacnt)
		return;

	ioaddr = blkp->b_ioaddr;

	status = inb( ioaddr + MCIS_INTR );
	blkp->b_intr_code = MCIS_rintrp( sp )->ri_code;
	blkp->b_intr_dev  = MCIS_rintrp( sp )->ri_ldevid;

	if ( MCIS_BUSYWAIT( ioaddr ))
		return;

	MCIS_SENDEOI( ioaddr, blkp->b_intr_dev );
	blkp->b_npend--;
	scbp = blkp->b_ldm[ blkp->b_intr_dev ].ld_scbp;

	if( scbp == 0 ) {
#ifdef MCIS_MLDEBUG3
		cmn_err(CE_CONT,
		"mcis_intr: ioaddr 0x%x, vect 0x%x, mcis_intrcnt 0x%x\n",
			ioaddr, vect, mcis_intrcnt);
		cmn_err(CE_CONT,
		"mcis_intr:  b_intr_dev 0x%x, b_intr_code 0x%x, blkp->b_npend 0x%x\n",
			blkp->b_intr_dev, blkp->b_intr_code, blkp->b_npend);
#endif
		return;
	}
	srbp = scbp->s_srbp;
	sbp = srbp->s_sbp;

	if ( sbp->sb.sb_type == SFB_TYPE )
	{
		scb_type = FALSE;
		comp_code = &sbp->sb.SFB.sf_comp_code;
	}
	else
	{
		comp_code = &sbp->sb.SCB.sc_comp_code;
	}

	switch ( blkp->b_intr_code )
	{
		case ISINTR_SCB_OK:
		case ISINTR_SCB_OK2:

			*comp_code = SDI_ASW;
			break;

		case ISINTR_ICMD_OK:

			/* check for assign command */

			if ( scbp->s_status == MCIS_WAITLD )
			{
				blkp->b_ldm[ MCIS_HBALD ].ld_scbp = NULL;
				scbp->s_status = MCIS_GOTLD;

				if ( scb_type )
					mcis_scbcmd( srbp, scbp );
				else
					mcis_sfbcmd( srbp, scbp );

			/*
			** If there was an error sending the command,
			** mcis_s[cf]bcmd() set comp_code to SDI_ABORT and
			** did sdi_callback().  We cannot just return and
			** catch the error here.  mcis_s[cf]bcmd() MUST
			** deal with the error since they are normally
			** called outside of interrupt routine.
			*/

				return;
			}

			*comp_code = SDI_ASW;
			break;

		case ISINTR_CMD_FAIL:

			if ( ! scb_type )
			{
				*comp_code = SDI_ABORT;
				break;
			}

			tsbp = &scbp->s_tsb;
#ifdef MCIS_DEBUG
			if ( mcis_debug & MCIS_DINTR )
				cmn_err(CE_CONT,
				"mcis_intr: FAIL tstat= 0x%x terr= 0x%x herr= 0x%x\n",
					tsbp->t_targstat, tsbp->t_targerr, 
					tsbp->t_haerr);
#endif
			/* record target device status */

			/* This is correct */

			sbp->sb.SCB.sc_status = tsbp->t_targstat << 1;

			if ( ! tsbp->t_haerr && ! tsbp->t_targerr
					&& ! tsbp->t_targstat)
				*comp_code= SDI_ASW;

			else if ( tsbp->t_haerr )
				*comp_code = SDI_HAERR;

			else
				*comp_code = SDI_CKSTAT;

			break;

		default:

			*comp_code = SDI_ABORT;
			break;
	}
#ifdef MCIS_CACHE_STATS
#define MCIS_READ_HIT	01
#define MCIS_WRITE_HIT	02
#define MCIS_CACHE_RETRY	04
#define MCIS_CACHE_ENABLED	010
	if (scbp->s_ident.id_type == ID_RANDOM && 
	   ( *(char *) sbp->sb.SCB.sc_cmdpt == SM_READ ||
	     *(char *) sbp->sb.SCB.sc_cmdpt == SM_WRITE)) {
		static int counter;
		ushort hit_rate;
		ushort status;
		tsbp = &scbp->s_tsb;
		hit_rate = tsbp->t_crate;
		status = tsbp->t_cstat;
		if (status & MCIS_CACHE_RETRY)
			cmn_err(CE_NOTE, "\tCache Retry\n");
		if (counter++ % 1000 == 0) {
			if (status & MCIS_CACHE_ENABLED)
				cmn_err(CE_CONT, "Cache hit ratio: %x%%\n", hit_rate);
			else
				cmn_err(CE_CONT, "**** CACHE DISABLED\n");
		}

	}
#endif

	if ( ! mcis_lds_assigned )
		mcis_freeld( blkp, scbp->s_ldmap );

	scbp->s_luq.q_flag &= ~MCIS_QLUBUSY;

	if ( *comp_code == SDI_ASW )
	{
		char cmd = *(char *) sbp->sb.SCB.sc_cmdpt;
		if (cmd == SM_RDCAP || cmd == SS_MSELECT || cmd == SS_MSENSE)
			mcis_set_bsize(&scbp->s_luq, &sbp->sb);
		mcis_next( &scbp->s_luq );
		sdi_callback( &sbp->sb );
	}
	else
	{
		sdi_callback( &sbp->sb );
	
		/*
		** If comp_code == SDI_CKSTAT, a lot can happen before we ever
		** get to the mcis_next below().  sdi_callback will genereate
		** an SS_REQSEN command to mcis_icmd which will call mcis_putq()
		** and mcis_next().  If the request sense command is placed first
		** on the Q, all is fine. The command will get sent to the board,
		** and we will return here to finally call the following
		** mcis_next().  At which point the Q will be busy and mcis_next
		** will return.  Finally this interrupt routine will be called
		** and the sense info will be returned to the target driver.
		**
		** Let's consider the case where the request sense will not be
		** put on the front of the Q.  The only commands ahead of it will
		** be of type SFB_TYPE or ISCB_TYPE.  If the head of the Q is an
		** SFB_TYPE, it has to be an SFB_ABORTM in which case we don't
		** care that the request sense data we want may be invalidated by
		** the completion of the abort.  The target driver was aborting
		** the same command that generated the SDI_CKSTAT.
		**
		** If the command on the head of the Q was an ISCB_TYPE, there is
		** no guarantee what the command will be.  Whatever it is, it will
		** may (or is that will ?) invalidate the sense info for the
		** command we are processing right here.  This could be a problem,
		** however ISCB_TYPE are special purpose commands and for the time
		** being, we accept any consequences.
		**
		** If wierd things ever start to happen, this comment is meant to
		** indicate that there may be a potential hole here.
		**
		** I considered requesting the sense information right here,
		** saving it, and then returning it when the subsequent
		** request described above came in.  Unfortunately there are
		** several problems with that approach.  First I'd have to
		** poll for the resulting interrupt otherwise the SS_REQSEN
		** from the higher level driver may get here before the
		** sense information is returned.  The trouble with polling
		** is there may be many LU's attached with interrupts pending
		** ahead of the SS_REQSEN I'm waiting for.  Then I'd have to
		** deal with them also and they may require the same SS_REQSEN
		** processing, and well,  I think you see what I mean.  It
		** could get ugly.  I'm not saying it's impossible, I just
		** don't think the added complexity will buy us anything in
		** performance or correctness.
		*/
	
		mcis_next( &scbp->s_luq );
	}
}


/*
 * void
 * mcis_set_bsize(struct mcis_luq *q, struct sb *sp)
 *      Record the sector size for device.  This function should only
 *	be called if the Read Capacity or Mode Select/Sense succeeded.
 *
 * Calling/Exit State:
 *      No locks held on entry or exit.
 */
STATIC void
mcis_set_bsize(struct mcis_luq *q, struct sb *sp)
{
	int size, shift;
	char *cmd;

	cmd  = (char *) sp->SCB.sc_cmdpt;
	if (*cmd == SM_RDCAP) {
		/*
		 * Read Capacity returns two 32-bit ints.  We are
		 * interested in the second.
		 */
		struct rdcap { int address; int length; } *rdcap;
		rdcap = (struct rdcap *) sp->SCB.sc_datapt;
		size = sdi_swap32(rdcap->length);
	} else {
		/*
		 * Doing SS_MSELECT or SS_MSENSE
		 */
		struct mode {
			unsigned len   :8;
			unsigned media :8;
			unsigned speed :4;
			unsigned bm    :3;
			unsigned wp    :1;
			unsigned bdl   :8;
			unsigned dens  :8;
			unsigned nblks :24;
			unsigned res   :8;
			unsigned bsize :24;
		} *mode = (struct mode *) sp->SCB.sc_datapt;

		size = sdi_swap24(mode->bsize);
	}

	/* Assume sector size is power of 2.  Is this ok? */
	for (shift = 0; shift < 32 && 1 << shift != size; shift++)
		;
	if (shift == 32)
		/*
		 *+ MCIS Illegal Sector Size
		 */
		cmn_err (CE_WARN, "%s: Illegal Sector Size (%x)\n", mcis_idata[0].name, size);
	else {
		q->q_shift = shift;
#ifdef DEBUG
		/*
		 *+ MCIS Setting sector size for device
		 */
		cmn_err ( CE_NOTE, "%s: Sector Size(%x) = %d (shift %d)\n", mcis_idata[0].name, q, size, shift);
#endif
	}
}

/*
** Function name: mcis_scbcmd()
**
** Description:
**	Create and send an SCB associated command. 
*/

void
mcis_scbcmd( srbp, scbp )
struct mcis_srb	*srbp;
struct mcis_scb	*scbp;
{
	struct mcis_blk	*blkp = scbp->s_blkp;
	struct xsb	*sbp = srbp->s_sbp;
	char		cmd;
	char 		*cmdpt;
	unchar		*scb_cdb;
	int		lcv;

	if ( ! mcis_lds_assigned )
	{
		if ( scbp->s_status == MCIS_GOTLD )
		{
			scbp->s_status = 0;
		}
		else
		{
			/* check for assign error */

			if ( mcis_getld( blkp, scbp, 1 ) == MCIS_HBALD )
			{
				goto abort;
			}
			else if ( scbp->s_status == MCIS_WAITLD )
				return;
		}
	}

	cmdpt = sbp->sb.SCB.sc_cmdpt;
	cmd = *cmdpt;

	if (scbp->s_ident.id_type == ID_RANDOM && 
		(cmd == SM_READ || cmd == SM_WRITE)) {

		/* Group 1 Command */
		/* (add 2 to cmdpt to account for pad in scm) */
		struct scm *scmp = (struct scm *)(cmdpt - 2);
		scbp->s_baddr = (daddr_t)sdi_swap32(scmp->sm_addr);
		((struct mcis_rdscb *)scbp)->s_blkcnt = sdi_swap16( scmp->sm_len);
		scbp->s_cmdsup = ISCMD_SCB;
		scbp->s_ch = 0;
		scbp->s_opfld = MCIS_RE | MCIS_ES;

#ifdef MCIS_CACHE_STATS
		scbp->s_opfld &= ~MCIS_ES;
#endif

		if (cmd == SM_READ) {
			scbp->s_cmdop = ISCMD_READ;
			scbp->s_opfld |= MCIS_RD;
		} else {
			scbp->s_cmdop = ISCMD_WRITE;
		}
		((struct mcis_rdscb *)scbp)->s_blklen = 1 << scbp->s_luq.q_shift;

		if (srbp->s_addr && sbp->sb.SCB.sc_datasz > pgbnd(srbp->s_addr))
			mcis_dmalist( scbp );

		scbp->s_hostaddr = srbp->s_addr;
		scbp->s_hostcnt = srbp->s_size;

		if ( mcis_docmd( blkp, scbp, scbp->s_ldmap->LD_CB.a_ld, ISATTN_SCB))
			goto abort;
		return;
	}

	/* Determine if it's a group 0 or group 1 scsi command */

	scb_cdb = scbp->s_cdb;
	lcv = SCS_SZ;
	if ( cmd & 0x20 )	/* Group 1 Command */
		lcv = SCM_SZ;
	while ( lcv-- )
		*scb_cdb++ = *cmdpt++;

	scbp->s_cmdop = ISCMD_SNDSCSI;
	scbp->s_cmdsup = ISCMD_LSCB;
	scbp->s_ch = 0;		/* zero this out in case anyone has set it */

	/*
	** I could do this during initialization, but as long as I
	** would have to set turn off MCIS_PT bit anyhow, this is
	** just as easy.
	*/

	scbp->s_opfld = MCIS_BB | MCIS_RE | MCIS_ES | MCIS_SS;

	if ( sbp->sb.SCB.sc_mode & SCB_READ )
		scbp->s_opfld |= MCIS_RD;

	if ( scbp->s_cdb[ 0 ] == SS_MSENSE )
		scbp->s_opfld |= MCIS_RD;

	scbp->s_baddr = sbp->sb.SCB.sc_cmdsz;

	if ( srbp->s_addr && sbp->sb.SCB.sc_datasz > pgbnd( srbp->s_addr ))
		mcis_dmalist( scbp );

	scbp->s_hostaddr = srbp->s_addr;
	scbp->s_hostcnt = srbp->s_size;

	if ( mcis_docmd( blkp, scbp, scbp->s_ldmap->LD_CB.a_ld, ISATTN_LSCB ))
	{
abort:
		sbp->sb.SCB.sc_comp_code = SDI_ABORT;
		scbp->s_luq.q_flag &= ~MCIS_QLUBUSY;
		sdi_callback( &sbp->sb );
		mcis_next(  &scbp->s_luq );
	}
}


/*
** Function name: mcis_dmalist()
**
** Description:
**	Build the physical address(es) for DMA to/from the data buffer.
**	If the data buffer is contiguous in physical memory, sp->s_addr
**	is already set for a regular SB.  If not, a scatter/gather list
**	is built, and the SB will point to that list instead.
*/

void
mcis_dmalist( scbp )
struct mcis_scb	*scbp;
{
	register struct mcis_dma	*dmap = scbp->s_dmaseg;
	register struct mcis_srb	*srbp = scbp->s_srbp;
	struct proc			*procp = srbp->s_proc;
	long				count;
	long				fraglen;
	long				thispage;
	caddr_t				vaddr;
	paddr_t				base;
	paddr_t				addr;
	int				i;

	vaddr = srbp->s_sbp->sb.SCB.sc_datapt;
	count = srbp->s_sbp->sb.SCB.sc_datasz;

	/* First build a scatter/gather list of physical addresses and sizes */

	for ( i = 0; i < MAXDSEG && count; i++, dmap++ )
	{
		base = vtop( vaddr, procp );	/* Physical address of segment */
		fraglen = 0;			/* Zero bytes so far */

		do {
			thispage = min( count, pgbnd( vaddr ));
			fraglen += thispage;	/* This many more are contiguous */
			vaddr += thispage;	/* Bump virtual address */
			count -= thispage;	/* Recompute amount left */

			if ( ! count )
				break;		/* End of request */

			addr = vtop( vaddr, procp ); /* Get next page's address */
		} while ( base + fraglen == addr );

		/* Now set up dma list element */

		dmap->d_addr = base;
		dmap->d_cnt = fraglen;
	}

	if ( count != 0 )
		cmn_err( CE_PANIC,
			"IMB MCA Host Adapter: Job too big for DMA list");

	/*
	 * If the data buffer was contiguous in physical memory,
	 * there was no need for a scatter/gather list; We're done.
	 */

	if ( i > 1 )	/* We need a scatter/gather list  */
	{

#ifndef PDI_SVR42
		srbp->s_addr = kvtophys( (vaddr_t)scbp->s_dmaseg );
#else
		srbp->s_addr = kvtophys( (caddr_t)scbp->s_dmaseg );
#endif
		srbp->s_size = i * sizeof( struct mcis_dma );
		scbp->s_opfld |= MCIS_PT;
	}
}

/*
** Function name: mcis_putq()
**
** Description:
**	Put a job on a logical unit queue.  Jobs are enqueued
**	on a priority basis.
*/

void
mcis_putq( q, srbp )
register struct mcis_luq	*q;
register struct mcis_srb	*srbp;
{
	register int	cls = MCIS_QCLASS( srbp );

	ASSERT( q );

#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		cmn_err(CE_CONT,  "mcis_putq( 0x%x, 0x%x )\n", q, srbp );
#endif

	/* 
	 * If queue is empty or queue class of job is less than
	 * that of the last one on the queue, tack on to the end.
	 */

	if ( ! q->q_first || cls <= MCIS_QCLASS( q->q_last ))
	{
		if ( q->q_first )
		{
			q->q_last->s_next = srbp;
			srbp->s_prev = q->q_last;
		}
		else
		{
			q->q_first = srbp;
			srbp->s_prev = NULL;
		}

		srbp->s_next = NULL;
		q->q_last = srbp;

	}
	else
	{
		register struct mcis_srb *next = q->q_first;

		while ( MCIS_QCLASS( next ) >= cls )
			next = next->s_next;

		srbp->s_next = next;
		srbp->s_prev = next->s_prev;

		if ( next->s_prev )
			next->s_prev->s_next = srbp;
		else
			q->q_first = srbp;

		next->s_prev = srbp;
	}

	q->q_count++;
}


/*
 * STATIC void
 * mcis_getq(struct mcis_luq *q, struct mcis_srb *sp)
 *	remove a job from a logical unit queue.
 *
 */
STATIC void
mcis_getq(struct mcis_luq *q, struct mcis_srb *sp)
{
	ASSERT(q);
	ASSERT(sp);
#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		cmn_err(CE_CONT,  "mcis_getq( 0x%x, 0x%x )\n", q, sp );
#endif

	if (sp->s_prev && sp->s_next) {
		sp->s_next->s_prev = sp->s_prev;
		sp->s_prev->s_next = sp->s_next;
	} else {
		if (!sp->s_prev) {
			q->q_first = sp->s_next;
			if (q->q_first)
				q->q_first->s_prev = NULL;
		} 
		if (!sp->s_next) {
			q->q_last = sp->s_prev;
			if (q->q_last)
				q->q_last->s_next = NULL;
		} 
	}
	q->q_count--;
}

/*
 * STATIC int
 * mcis_getadr(struct mcis_srb *sp)
 *	Return the logical address of the disk request pointed to by
 *	by sp.
 */
STATIC int
mcis_getadr(struct mcis_srb *sp)
{
	char *p;
	struct scm *scmp;
	ASSERT(sp->s_sbp->sb.sb_type != SFB_TYPE);
	ASSERT(MCIS_IS_RW(MCIS_CMDPT(sp)));
        p = (char *)sp->s_sbp->sb.SCB.sc_cmdpt;
        switch(p[0]) {
        case SM_READ:
        case SM_WRITE:
		scmp = (struct scm *)(p - 2);
		return sdi_swap32(scmp->sm_addr);
        case SS_READ:
        case SS_WRITE:
                return ((p[1]&0x1f) << 16) | (p[2] << 8) | p[3];
        }
	/* NOTREACHED */
}

/* STATIC int
 * mcis_serial(struct mcis_lu *q,struct mcis_srb *first, struct mcis_srb *last)
 *	Return non-zero if the last job in the chain from
 *	first to last can be processed ahead of all the 
 *	other jobs on the chain.
 */
STATIC int
mcis_serial(struct mcis_luq *q, struct mcis_srb *first, struct mcis_srb *last)
{
	while (first != last) {
		unsigned int sb1 = mcis_getadr(last);
		unsigned int eb1 = sb1 + MCIS_BSIZE(last,q) - 1;
		if (MCIS_IS_WRITE(MCIS_CMDPT(first)) || 
		    MCIS_IS_WRITE(MCIS_CMDPT(last))) {
			unsigned int sb2 = mcis_getadr(first);
			unsigned int eb2 = sb2 + MCIS_BSIZE(first, q) - 1;
			if (sb1 <= sb2 && eb1 >= sb2)
				return 0;
			if (sb2 <= sb1 && eb2 >= sb1)
				return 0;
		}
		first = first->s_next;
	}
	return 1;
}
			
#define MCIS_ABS_DIFF(x,y)	(x < y ? y - x : x - y)

/*
 * STATIC struct mics_srb *
 * mcis_schedule(struct mcis_luq *, int)
 *	Select the next job to process.  This routine assumes jobs
 *	are placed on the queue in sorted order.
 */
STATIC struct mcis_srb *
mcis_schedule(struct mcis_luq *q)
{
        struct mcis_srb *sp = q->q_first;
        struct mcis_srb  *best = NULL;
        unsigned int	best_position, best_distance, distance, position;
#ifdef DEBUG
        int count = 0;
#endif
	ASSERT(sp);
	if (MCIS_QCLASS(sp) != SCB_TYPE || !(q->q_flag & MCIS_QSCHED)
				|| !MCIS_IS_RW(MCIS_CMDPT(sp)))
		return sp;
	best_distance = (unsigned int)~0;
	
#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		count = mcis_schedule_debug(sp, q->q_addr);
#endif
        /* Implement shortest seek first */
	do {
		position = mcis_getadr(sp);
		distance = MCIS_ABS_DIFF(q->q_addr, position);
		if (distance < best_distance) {
			best_position = position;
			best_distance = distance;
			best = sp;
		}
		sp = sp->s_next;
        }
        while (sp && MCIS_QCLASS(sp) == SCB_TYPE && MCIS_IS_RW(MCIS_CMDPT(sp)));
#ifdef DEBUG
	if (count)
		cmn_err(CE_CONT,"Selected position %d\n", best_position);
#endif
	ASSERT(best);

	if (mcis_serial(q, q->q_first, best)) {
		q->q_addr = best_position + MCIS_BSIZE(best, q);
		return best;
	} 

	/* Should rarely if ever get here */
	q->q_addr = mcis_getadr(q->q_first) + MCIS_BSIZE(q->q_first, q);
	return q->q_first;
}
	
#ifdef DEBUG

/* Only print scheduler choices when there are more than MCIS_QDBG jobs */
#define MCIS_QDBG 2

/* 
 * STATIC int
 * mcis_schedule_debug(struct mcis_srb *sp, int head)
 *	Debug the disk scheduler.  Return non-zero if number of jobs being
 *	considered is at least MCIS_QDBG.
 */
STATIC int
mcis_schedule_debug(struct mcis_srb *sp, int head)
{
        struct mcis_srb *tsp = sp;
        int count=0;
        while (tsp && MCIS_QCLASS(tsp) == MCIS_QCLASS(sp)) {
                count++;
                tsp = tsp->s_next;
        }
        if (count < MCIS_QDBG)
                return 0;
        cmn_err(CE_CONT, "\n\nSchedule:\n");
        cmn_err(CE_CONT, "\thead position: %d\n", head);
        cmn_err(CE_CONT, "\tChoosing among %d choices\n",count);
        cmn_err(CE_CONT, "\tAddresses:");
        tsp = sp;
        while (tsp && MCIS_QCLASS(tsp) == MCIS_QCLASS(sp)) {
		if (tsp->s_sbp->sb.sb_type==SFB_TYPE ||!MCIS_IS_RW(MCIS_CMDPT(tsp)))
			cmn_err(CE_CONT, " COM");
		else
			cmn_err(CE_CONT, " %d", mcis_getadr(tsp));
                tsp = tsp->s_next;
        }
        cmn_err(CE_CONT, "\n");
        return count;
}
#endif /* DEBUG */

/*
** Function name: mcis_next()
**
** Description:
**	Attempt to send the next job on the logical unit queue.
*/

void
mcis_next( q )
register struct mcis_luq	*q;
{
	register struct mcis_srb	*srbp;
	struct xsb			*sbp;
	unsigned long			scb_type;

	ASSERT( q );

#ifdef DEBUG
	if ( mcis_debug & MCIS_DB_ENT_EXT )
		cmn_err(CE_CONT,  "mcis_next( 0x%x )\n", q );
#endif

	if (q->q_first == NULL || q->q_flag & MCIS_QLUBUSY )
		return;

	q->q_flag |= MCIS_QLUBUSY;

	/* The idea here is to make a quick decision as to what to
	 * send down to the target, since the target is currently
	 * empty.  Scheduling is done after the job is sent down,
	 * which means that we will make the choice of what to 
	 * service next without having all the jobs available.  
	 * Still, we should get some benefits of scheduling
	 * and still get jobs down quickly.
	 *
	 * It would be nice to put off the mcis_getq() until 
	 * after we send the command down, but mcis_next()
	 * can be called recursively.
	 */
	srbp = q->q_first;
	mcis_getq(q, srbp);

	sbp = srbp->s_sbp;

	q->q_scbp->s_srbp = srbp;
	
	if (sbp->sb.sb_type != SFB_TYPE) {
		sbp->sb.SCB.sc_status = 0;
		mcis_scbcmd( srbp, q->q_scbp );
		scb_type = 1;
	} else {
		mcis_sfbcmd( srbp, q->q_scbp );
		scb_type = 0;
	}


	if (q->q_first) {
		srbp = mcis_schedule(q);
		if (srbp != q->q_first) {
			mcis_getq(q, srbp);
			srbp->s_next = q->q_first;
			srbp->s_prev = NULL;
			q->q_first->s_prev = srbp;
			q->q_first = srbp;
			q->q_count++;
		}
	}

	if ( mcis_polltime )		/* need to poll */
	{
		register ulong	*comp_code;

		if ( scb_type )
			comp_code = &sbp->sb.SCB.sc_comp_code;
		else
			comp_code = &sbp->sb.SFB.sf_comp_code;

		*comp_code = SDI_ASW;

		if ( mcis_pollret( q->q_scbp->s_blkp ))
			*comp_code = SDI_TIME;

		q->q_flag &= ~MCIS_QLUBUSY;
		sdi_callback( &sbp->sb );
	}
}


/*
** Function name: mcis_sfbcmd()
** Description:
**	Create and send an SFB associated command. 
*/

void
mcis_sfbcmd( srbp, scbp )
register struct mcis_srb *srbp;
register struct mcis_scb *scbp;
{
	struct mcis_blk	*blkp = scbp->s_blkp;
	struct xsb	*sbp = srbp->s_sbp;

	if ( ! mcis_lds_assigned )
	{
		if ( scbp->s_status == MCIS_GOTLD )
		{
			scbp->s_status = 0;
		}
		else
		{
			/* check for assign error */

			if ( mcis_getld( blkp, scbp, 1 ) == MCIS_HBALD )
			{
				goto abort;
			}
			else if ( scbp->s_status == MCIS_WAITLD )
				return;
		}
	}

	scbp->s_cmdop = ISCMD_ABORT;
	scbp->s_cmdsup = ISCMD_ICMD;
 
	if ( mcis_docmd( blkp, scbp, scbp->s_ldmap->LD_CB.a_ld, ISATTN_ICMD ))
	{
abort:
		scbp->s_luq.q_flag &= ~MCIS_QLUBUSY;
		sbp->sb.SFB.sf_comp_code = SDI_ABORT;
		mcis_next( &scbp->s_luq );
		sdi_callback( &sbp->sb );
	}
}


/*
** Function name: mcis_flushq()
**
** Description:
**	Empty a logical unit queue.  If flag is set, remove all jobs.
**	Otherwise, remove only non-control jobs.
*/

void
mcis_flushq( q, cc, flag )
register struct mcis_luq	*q;
int			cc;
int			flag;
{
	register struct mcis_srb  *srbp;
	register struct mcis_srb  *nsrbp;

	ASSERT( q );

	srbp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_count = 0;

	while ( srbp )
	{
		nsrbp = srbp->s_next;

		if ( ! flag && MCIS_QCLASS( srbp ) != SCB_TYPE )
			mcis_putq( q, srbp );
		else
		{
			srbp->s_sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback( &srbp->s_sbp->sb );
		}

		srbp = nsrbp;
	}
}

/*
 *	hba initialization routine
 */

int
mcis_hbainit(cntlnum)
int	cntlnum;
{
	register struct	mcis_blk	*blkp;
	struct	mcis_scb		*scbp;
	struct	mcis_scb		*nodev_scbp;
	int				first_time = TRUE;
	int				size;
	int				t;
	int				l;
	int				i;
	int				dma_listsize;
	struct mcis_dma			*dmalist;

	/* detect the hba location */

	if ( mcis_findhba( mcis_idata[cntlnum].ioaddr1 ))
		return 0;

	/* hba local configuration initialization */

	if (( blkp = mcis_cfginit(mcis_idata[cntlnum].ioaddr1, 
					mcis_idata[cntlnum].dmachan1 )) == NULL )
		return 0;

	/* get equipment description table - edt */

	mcis_getedt( blkp );

	if ( ! blkp->b_numdev )
	{
		kmem_free( (_VOID *)blkp->b_scbp, blkp->b_scballoc );
		blkp->b_scbp = NULL;
		kmem_free( (_VOID *)blkp, sizeof( struct mcis_blk ));
		blkp = NULL;
		return 0;
	} 

	/*
	** Now there are 128 mcis_scb's alloced, with every other one full
	** of inquiry data.  In general, there will usually be only a few
	** actual scsi devices attached to the bus.  To save space, I'm
	** going to allocate one per actual device and one extra to be
	** the nodev case.  The nodev case is only really needed for its
	** inquiry data.  I could probably do without it, but it would
	** further complicate the code.  My objective is to greatly reduce
	** the run time space with the least impact on the code.
	*/

	size = ( blkp->b_numdev + 1 ) * sizeof( struct mcis_scb );

	if (( scbp = (struct mcis_scb *)KMEM_ZALLOC( size, (mcis_dynamic ? KM_SLEEP : KM_NOSLEEP ))) == NULL ) {
		cmn_err( CE_WARN, "%s: Initialization error - cannot allocate scb's", mcis_idata[cntlnum].name );
		return 0;
	}
		
	nodev_scbp = scbp++;
	mcis_lds_assigned = ( blkp->b_ldcnt >= blkp->b_numdev );

	/*
	 * Allocate the dma scatter gather lists.  These must fall within
	 * a page, since none of the MAXDSEG chunks of lists can cross a
	 * physical page boundary.
	 */
	dma_listsize = blkp->b_numdev * MAXDSEG * sizeof(struct mcis_dma);
	for (i = 2; i < dma_listsize; i *= 2);
	dma_listsize = i;

#ifdef	DEBUG
	if(dma_listsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: dmalist exceeds pagesize (may cross physical page boundary)\n", mcis_idata[0].name);
#endif /* DEBUG */

	if((dmalist = (struct mcis_dma *)KMEM_ZALLOC(dma_listsize, mcis_dynamic ? KM_SLEEP:KM_NOSLEEP)) == NULL) {
		cmn_err(CE_WARN, "%s: Initialization error allocating dmalist\n", mcis_idata[cntlnum].name);
		kmem_free(scbp, size );
		return 0;
	}

	for ( t = 0; t < MAX_TCS; t++ )
	{
		register int	offset;

		for ( l = 0; l < MAX_LUS; l++ )
		{
			offset = ( t << 3 ) | l;

			/* This is assigned a value in mcis_waitedt() */

			if ( blkp->b_scb_array[ offset ])
			{
				/* Default is 512 byte sectors */
				scbp->s_luq.q_shift = 9;
			
				blkp->b_scb_array[ offset ] = scbp;
				bcopy( &blkp->b_scbp[ offset << 1 ], scbp,
						sizeof( struct mcis_scb ));

				bcopy( &blkp->b_scbp[( offset << 1 ) + 1 ],
					&scbp->s_ident, sizeof( struct ident ));

				/*
				** These need to be re-initialized since
				** the location of the structs is changing
				*/

				scbp->s_luq.q_scbp = scbp;
#ifndef PDI_SVR42
				scbp->s_tsbp = kvtophys( (vaddr_t)( &scbp->s_tsb ));
#else
				scbp->s_tsbp = kvtophys( (caddr_t)( &scbp->s_tsb ));
#endif

				if ( mcis_lds_assigned
					&& mcis_getld( blkp, scbp, 0 ) != MCIS_HBALD )
				{
					scbp->s_status = MCIS_OWNLD;
				}

				scbp->s_dmaseg = dmalist;
				dmalist += MAXDSEG;

				if(!cntlnum && scbp->s_ident.id_type == ID_RANDOM)
					mcis_first_disk = t;
				
				scbp++;
			}
			else
			{
				if ( first_time )
				{
					bcopy( &blkp->b_scbp[ offset << 1 ],
						nodev_scbp,
						sizeof( struct mcis_scb ));

					bcopy( &blkp->b_scbp[( offset << 1 )+ 1 ],
						&nodev_scbp->s_ident,
						sizeof( struct ident ));

					/* 
					** id_type may be ID_NODEV
					*/

					nodev_scbp->s_ident.id_type = ID_NODEV;
					first_time = FALSE;
				}

				blkp->b_scb_array[ offset ] = nodev_scbp;
			}
		}

	}

	kmem_free( (_VOID *)blkp->b_scbp, blkp->b_scballoc );
	blkp->b_scbp = NULL;
	blkp->b_scballoc = 0;

	mcis_ha[cntlnum] = blkp;
	return 1;
}


/*
 *	Adapter detection routine
 */

int
mcis_findhba( ioaddr )
register uint	ioaddr;
{
	register int	i;
	char		 status;
	char	 	*sp = &status;
	int		ctrl_addr = ioaddr + MCIS_CTRL;

	/* hardware reset */

	outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE | ISCTRL_RESET );

	/* According to ref man:
	   must wait for at least 50ms and then turn off controller reset */

	for ( i = 6; i; i-- )
		drv_usecwait(10);

	outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE );

	/* check busy in status reg  */

	if ( MCIS_BUSYWAIT( ioaddr ))
		return 1;

	/* wait for interrupt */

	if ( MCIS_INTRWAIT( ioaddr ))
		return 1;
	
	status = inb( ioaddr + MCIS_INTR );
	MCIS_SENDEOI( ioaddr, MCIS_HBALD );

	if ( MCIS_rintrp(sp)->ri_code ) 
		return 1;

	/* enable dma channel */

	outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE | ISCTRL_EDMA );

	status = inb( ctrl_addr );

	cmn_err(CE_NOTE, "!MCA SCSI Host Adapter found at address 0x%X\n", ioaddr);

#ifdef MCIS_MLDEBUG2
	cmn_err(CE_CONT,  "mcis_findhba: returning\n" );
#endif
	return 0;
}


/*
 *	adapter local structures configuration routine
 */

struct  mcis_blk *
mcis_cfginit( ioaddr, dmachan )
uint 	ioaddr;
uint	dmachan;
{
	struct mcis_blk	*blkp;
	struct mcis_pos	*posp;
	void mcis_fccmd();

#ifdef MCIS_MLDEBUG2
	cmn_err(CE_CONT,  "mcis_cfginit: entry\n" );
#endif

	/* allocate memory for initialization */

	blkp = (struct mcis_blk *)KMEM_ZALLOC( sizeof( struct mcis_blk ),
				(mcis_dynamic ? KM_SLEEP : KM_NOSLEEP) );

	if ( blkp == NULL )
		goto clean1;

	blkp->b_ioaddr = (ushort) ioaddr;
	blkp->b_dmachan = (ushort) dmachan;
	blkp->b_scb_cnt = 128;
	blkp->b_scballoc = 128 * sizeof( struct mcis_scb );
	blkp->b_scbp = (struct mcis_scb *)KMEM_ZALLOC( blkp->b_scballoc,
				 (mcis_dynamic ? KM_SLEEP : KM_NOSLEEP) );

	if ( blkp->b_scbp == NULL )
		goto clean2;

	/* scsi bus reset */

	if ( mcis_docmd( blkp, (( ISCMD_ICMD << 8 ) | ISCMD_RESET ), 
						MCIS_HBALD, ISATTN_ICMD)
		|| mcis_pollret( blkp )
		|| blkp->b_intr_code != ISINTR_ICMD_OK )
	{
#ifdef MCIS_MLDEBUG
		cmn_err(CE_CONT,
		"mcis_cfginit: fail on scsi reset retcod = 0x%x\n",
			blkp->b_intr_code );
#endif
		goto clean3;
	}

	/* get POS information */

	posp = (struct mcis_pos *)( blkp->b_scbp + 1 );
	mcis_poscmd( blkp->b_scbp, posp );

	if ( mcis_docmd( blkp, blkp->b_scbp, MCIS_HBALD, ISATTN_SCB )
		|| mcis_pollret( blkp )
		|| ( blkp->b_intr_code != ISINTR_SCB_OK  &&
		     blkp->b_intr_code != ISINTR_SCB_OK2 ))
	{
#ifdef MCIS_MLDEBUG
		cmn_err(CE_CONT,  "mcis_cfginit: fail on getpos retcod = 0x%x\n",
						blkp->b_intr_code );
#endif
		goto clean3;
	}

	/* Issue a feature control command to set HBA global timeout */

	mcis_fccmd( (struct fcscb *)blkp->b_scbp );

	if ( mcis_docmd( blkp, *((int *)(void *)(blkp->b_scbp)),
						MCIS_HBALD, ISATTN_ICMD)
		|| mcis_pollret( blkp ) != 0
		|| blkp->b_intr_code != ISINTR_ICMD_OK )
	{
		cmn_err( CE_WARN,
		   "!IBM MCA Host Adapter: Feature Control command failed\n");
#ifdef MCIS_MLDEBUG
		cmn_err(CE_CONT,  "mcis_cfginit: fail on feature control cmd, retcod = 0x%x\n",
						blkp->b_intr_code );
#endif
	}

	/* someday, someone might have to set intr vect at runtime */

	blkp->b_intr = posp->p_intr;
	blkp->b_targetid = posp->p3_targid;

	/* calculate available logical device */

	if ( posp->p_ldcnt < MCIS_MAXLD )
		blkp->b_ldcnt = posp->p_ldcnt - 1;
	else
		blkp->b_ldcnt = MCIS_MAXLD - 1;

#ifdef MCIS_MLDEBUG2
	cmn_err(CE_CONT,  "mcis_cfginit: mcis_blkp = 0x%x scbp = 0x%x\n", 
						blkp, blkp->b_scbp );

	cmn_err(CE_CONT,  "mcis_cfginit: hbaid = 0x%x targetid = %d\n", 
					posp->p_hbaid, blkp->b_targetid );

	cmn_err(CE_CONT,  "dma = 0x%x fair = %d ioaddr = 0x%x romseg = 0x%x\n",
		posp->p3_dma, posp->p3_fair, posp->p2_ioaddr, posp->p2_romseg );

	cmn_err(CE_CONT,  "intr = 0x%x pos4 = %d slotsz = 0x%x ldcnt = 0x%x\n",
		posp->p_intr, posp->p_pos4, posp->p_slotsz, posp->p_ldcnt );

	cmn_err(CE_CONT,
		"pacing = 0x%x tmeoi = 0x%x tmreset = 0x%x cache = 0x%x\n",
		posp->p_pacing, posp->p_tmeoi, posp->p_tmreset, posp->p_cache );
#endif

	/* initialize logical device manager */

	mcis_initld( blkp );

	return blkp;

clean3: kmem_free( (_VOID *)blkp->b_scbp, blkp->b_scballoc );	
		blkp->b_scbp = NULL;
clean2: kmem_free( (_VOID *)blkp, sizeof( struct mcis_blk ));
		blkp = NULL;
clean1: 
	/*
	 *+ The mcis adapter local structures configuration routine
	 *+ was unable to allocate memory to initialize the device.
	 */
	cmn_err(CE_WARN, "mcis_cfginit: unable to allocate memory\n" );
	return NULL;
}

/*
 *	Adapter get equipment description table routine
 *	inquiry commands are mutli-thread through all possible scsi devices
 */

void
mcis_getedt( blkp )
register struct	mcis_blk *blkp;
{
	register struct mcis_scb	*scbp;
	struct ident			*ident;
	register int			targ;
	int				lun;
	int				ld;
	int				ctrl_addr = blkp->b_ioaddr + MCIS_CTRL;

	/* check the complete scsi bus */

	for ( lun = 0; lun < MAX_LUS; lun++ )
		for ( targ = 0; targ < MAX_TCS; targ++ )
		{
#ifdef MCIS_MLDEBUG2
			cmn_err(CE_CONT, "mcis_getedt: < %d, %d > \n", targ, lun );
#endif
			scbp = (struct mcis_scb *)(&blkp->b_scbp[
						( targ << 3 | lun ) << 1 ]);

			scbp->s_luq.q_scbp = scbp;
			scbp->s_blkp = blkp;
			scbp->s_targ = (unchar) targ;
			scbp->s_lun  = (unchar) lun;
#ifndef PDI_SVR42
			scbp->s_tsbp = kvtophys( (vaddr_t)( &scbp->s_tsb ));
#else
			scbp->s_tsbp = kvtophys( (caddr_t)( &scbp->s_tsb ));
#endif

			if ( targ == blkp->b_targetid )
				continue;

			ident = (struct ident *)(void *)(scbp + 1);
			mcis_inqcmd( scbp, ident );
			ident->id_type = ID_NODEV;

			/* get logical device with no wait */

			ld = mcis_getld( blkp, scbp, 1 );

			/* check for assign error */

			if ( ld == MCIS_HBALD )
				continue;

			/* check for available logical device */

			if ( ld != -1 )
			{
				if ( mcis_docmd( blkp, scbp, ld, ISATTN_SCB))
				{
					mcis_freeld( blkp, scbp->s_ldmap );
					continue;
				}
			}

			/* wait for outstanding inquiry command to arrive */
			while( !mcis_waitedt( blkp ) );
		}

	if ( ! blkp->b_numdev ) 
		return;

	/* invlidate logical devices assignment */

	mcis_invld( blkp );

	/* enable dma */

	outb( ctrl_addr, inb( ctrl_addr ) & ISCTRL_RESERVE
						| ISCTRL_EDMA );
}


/*
 *	wait for edt information
 */

int
mcis_waitedt( blkp )
register struct	mcis_blk *blkp;
{
	register struct	mcis_scb	*scbp;
	register struct	ident		*ident;
	struct mcis_ldevmap		*ldp;
	if ( mcis_pollret( blkp )) 
		return 0;

	scbp = blkp->b_ldm[ blkp->b_intr_dev ].ld_scbp;
	ident = (struct ident *)(void *)(scbp + 1);

	/* check for outstanding logical device assignment completion */

	if ( blkp->b_intr_dev == MCIS_HBALD )
	{
#ifdef MCIS_MLDEBUG2
		cmn_err(CE_CONT,  "mcis_waitedt: ASGN retcod = 0x%x\n",
						blkp->b_intr_code );
#endif
		blkp->b_ldm[ MCIS_HBALD ].ld_scbp = NULL;
		scbp->s_status = 0;

		if ( blkp->b_intr_code == ISINTR_ICMD_OK
			&& ! mcis_docmd( blkp, scbp,
				scbp->s_ldmap->LD_CB.a_ld, ISATTN_SCB ))
			return 0;

		/* check for inquiry command return status */

	}
	else if ( blkp->b_intr_code != ISINTR_SCB_OK
			&&  blkp->b_intr_code != ISINTR_SCB_OK2 )
	{
		/* If there is a failure not due to the
		 * a scsi selection timeout retry the inquiry.
		 */
		struct mcis_tsb *tsbp = &scbp->s_tsb;
		ldp = &blkp->b_ldm[ blkp->b_intr_dev ];
		if (tsbp->t_targerr != 0x10 &&  scbp->s_luq.q_retries < 4) {
			scbp->s_luq.q_retries++;
			cmn_err(CE_NOTE, "!mcis: Retry inquiry <target,lun> = <%d,%d>", ldp->LD_CB.a_targ, ldp->LD_CB.a_lun);
			mcis_docmd(blkp, scbp, scbp->s_ldmap->LD_CB.a_ld, ISATTN_SCB);
			return 0;
		}
		ident->id_type = ID_NODEV;

#ifdef MCIS_MLDEBUG2

		cmn_err(CE_CONT,
		"mcis_waitedt: INQCMD retcod = 0x%x on ld = %d < %d, %d > id_type = %d\n",
			blkp->b_intr_code, blkp->b_intr_dev, 
			ldp->LD_CB.a_targ, ldp->LD_CB.a_lun, ident->id_type );
#endif
	/* increment device count for any new devices responded */
	}
	else if ( ident->id_type != ID_NODEV )
	{

		if (scbp->s_luq.q_retries) 
			cmn_err(CE_NOTE, "!mcis: Device found after %d retries", scbp->s_luq.q_retries);
		blkp->b_numdev++;

		/*
		** This is added to support the change to reduce the space
		** required during run time.  A non-NULL value in b_scb_array
		** indicates <t:l> is a real device.  This value is checked
		** in mcis_hbainit().
		*/

		ldp = &blkp->b_ldm[ blkp->b_intr_dev ];
		blkp->b_scb_array[( ldp->LD_CB.a_targ << 3 )
			| ldp->LD_CB.a_lun ] = (struct mcis_scb *)1;

#ifdef MCIS_MLDEBUG2
		cmn_err(CE_CONT,
		"mcis_waitedt: <targ,lun> = < %d, %d > dev = 0x%x\n",
			ldp->LD_CB.a_targ, ldp->LD_CB.a_lun, ident->id_type );
#endif
	}

	mcis_freeld( blkp, scbp->s_ldmap );
	return 1;
}

/*
 *	invalidate all logical device mapping	
 */

void
mcis_invld( blkp )
register struct mcis_blk *blkp;
{
	register struct	mcis_ldevmap	*ldp;
	register struct	mcis_scb	*scbp;
	struct ident			*ident;

#ifdef MCIS_MLDEBUG2
	cmn_err(CE_CONT,  "mcis_invld: entry\n" );
#endif

	/* search through the logical device pool */

	ldp = LDMHD_FW( blkp );

	for (; ldp != &(blkp->b_ldmhd); ldp = ldp->ld_avfw )
	{
		scbp = ldp->ld_scbp;
		ident = (struct ident *)(void *)(scbp + 1);

		if ( ident->id_type == ID_NODEV )
			continue;

		/* remove the previous assignment */

		ldp->LD_CB.a_rm = 1;		

		if (mcis_docmd( blkp, ldp->LD_CMD, MCIS_HBALD, ISATTN_ICMD) != 0
			|| mcis_pollret( blkp ) != 0
			|| blkp->b_intr_code != ISINTR_ICMD_OK) 
		{
#ifdef MCIS_MLDEBUG
			cmn_err(CE_CONT,
			"mcis_invld: fail = 0x%x retcod = 0x%x\n",
					ldp->LD_CMD, blkp->b_intr_code );
#else
		  	/*EMPTY*/
#endif
		}
		ldp->LD_CB.a_rm = 0;		
		scbp->s_ldmap = NULL;
	}
}


/*
 *	initialize the logical device map
 */

void
mcis_initld( blkp )
struct mcis_blk *blkp;
{
	register struct	mcis_ldevmap	*dp;
	register struct	mcis_ldevmap	*lp;
	register struct	mcis_scb	*scbp;
	int				i;
	int				maxld;
	int				targ;

	dp = &(blkp->b_ldmhd);
	dp->ld_avfw = dp->ld_avbk = dp;
	maxld = blkp->b_ldcnt;

	lp = blkp->b_ldm;

	for ( targ = 0, i = 0; i < maxld; i++, lp++ )
	{
		lp->ld_avbk = dp;
		lp->ld_avfw = dp->ld_avfw;
		dp->ld_avfw->ld_avbk = lp;
		dp->ld_avfw = lp;
		lp->LD_CMD  = 0;
		lp->LD_CB.a_cmdop = (ushort)((ISCMD_ICMD << 8) | ISCMD_ASSIGN);
		lp->LD_CB.a_ld = i;

		/* targets 0 to 6 has been assigned by the firmware */

		if ( targ >= 8 )
			continue;

		if ( targ != blkp->b_targetid )
		{
			scbp = &blkp->b_scbp[ targ << 4 ];
			lp->LD_CB.a_targ = targ;
			lp->LD_CB.a_lun  = 0;
			lp->ld_scbp = scbp;
			scbp->s_ldmap = lp;
		}

		targ++;
	}
}

/*
 *	setup an inquiry command
 */

void
mcis_inqcmd( scbp, hostaddr )
struct mcis_scb	*scbp;
int		hostaddr;
{
	scbp->s_luq.q_retries = 0;
	scbp->s_cmdop = ISCMD_DEVINQ;
	scbp->s_cmdsup = ISCMD_SCB;
	scbp->s_opfld = MCIS_BB | MCIS_SS | MCIS_RE | MCIS_ES | MCIS_RD;
	scbp->s_hostcnt = sizeof( struct ident );
#ifndef PDI_SVR42
	scbp->s_hostaddr = kvtophys( (vaddr_t)hostaddr );
	scbp->s_tsbp = kvtophys( (vaddr_t)( &scbp->s_tsb ));
#else
	scbp->s_hostaddr = kvtophys( (caddr_t)hostaddr );
	scbp->s_tsbp = kvtophys( (caddr_t)( &scbp->s_tsb ));
#endif
}

/*
 *	setup a pos command
 */

void
mcis_poscmd( scbp, hostaddr )
struct mcis_scb	*scbp;
int		hostaddr;
{
	scbp->s_cmdop = ISCMD_GETPOS;
	scbp->s_cmdsup = ISCMD_SCB;
	scbp->s_opfld = MCIS_BB | MCIS_RE | MCIS_ES | MCIS_RD;
	scbp->s_hostcnt = sizeof( struct mcis_pos );
#ifndef PDI_SVR42
	scbp->s_hostaddr = kvtophys( (vaddr_t)hostaddr );
	scbp->s_tsbp = kvtophys( (vaddr_t)( &scbp->s_tsb ));
#else
	scbp->s_hostaddr = kvtophys( (caddr_t)hostaddr );
	scbp->s_tsbp = kvtophys( (caddr_t)( &scbp->s_tsb ));
#endif
}

/*
 *	setup a feature control command
 */

void
mcis_fccmd( fcscbp )
struct mcis_fcscb	*fcscbp;
{
	fcscbp->fc_cmdop = ISCMD_HBACTRL;
	fcscbp->fc_cmdsup = ISCMD_ICMD;
	fcscbp->fc_gtv = (ushort) mcis_global_tout;
	fcscbp->fc_drate = 0;
}

/*
 *	adapter command interface routine
 */

int
mcis_docmd( blkp, cmd, dev, opcode )
struct mcis_blk	*blkp;
int		cmd;
unchar		dev;
unchar		opcode;
{
	register ushort		ioaddr;
	register int		i;
	register paddr_t	outcmd;
	pl_t			oldspl;

	ioaddr = blkp->b_ioaddr;
	outcmd = cmd;

	if ( opcode != ISATTN_ICMD )
		outcmd = kvtophys( outcmd );

/*	check busy in status reg 					*/

	if ( MCIS_CMDOUTWAIT( ioaddr ))
		return 1;

	oldspl = spl7();

	for ( i=0; i < 4 ; i++ )
	{
		outb( ioaddr + MCIS_CMD + i, ( outcmd & 0xff ));
		outcmd >>= 8;
	}

	outb( ioaddr + MCIS_ATTN, opcode | dev );
	blkp->b_npend++;
	splx( oldspl );

#ifdef MCIS_MLDEBUG2
	cmn_err(CE_CONT,
	"mcis_docmd: cmd= 0x%x attn= 0x%x\n", cmd, opcode | dev );
#endif

	return 0;
}

/*
 *	wait for interrupt return by polling
 */

int
mcis_pollret( mcis_blkp )
register struct	mcis_blk *mcis_blkp;
{
	register ushort	ioaddr;
	char		status;
	char		*sp = &status;

	ioaddr = mcis_blkp->b_ioaddr;

/*	wait for interrupt						*/

	if ( MCIS_INTRWAIT( ioaddr ))
		return 1;
	
	status = inb( ioaddr + MCIS_INTR );
	mcis_blkp->b_intr_code = MCIS_rintrp( sp )->ri_code;
	mcis_blkp->b_intr_dev  = MCIS_rintrp( sp )->ri_ldevid;

	if ( MCIS_BUSYWAIT( ioaddr ))
		return 1;

	MCIS_SENDEOI( ioaddr, mcis_blkp->b_intr_dev );
	mcis_blkp->b_npend--;

#ifdef MCIS_MLDEBUG2
	cmn_err(CE_CONT,  "mcis_pollret: icode= 0x%x idev= 0x%x\n",
			mcis_blkp->b_intr_code, mcis_blkp->b_intr_dev );
#endif

	return 0;
}

/*
 *	generic adapter ready wait routine
 */

int
mcis_wait( ioaddr, mask, onbits, offbits )
ushort	ioaddr;
ushort	mask;
ushort	onbits;
ushort	offbits;
{
	register ushort	port;
	register int	i;
	register ushort	maskval; 

	port = ioaddr + MCIS_STAT;

	for ( i = 3000000; i; i-- )
	{
		maskval = inb( port ) & mask;

		if (( maskval & onbits ) == onbits
				&& ( maskval & offbits ) == 0 )
			return 0;

		drv_usecwait(10);
	}

#ifdef MCIS_MLDEBUG
	cmn_err(CE_CONT,
	"mcis_wait: ERROR on = 0x%x off = 0x%x\n", onbits, offbits );
#endif

	return 1;
}

/*
 *	Get logical device mapping routine
 */

int
mcis_getld( mcis_blkp, mcis_scbp, nowait )
register struct	mcis_blk	*mcis_blkp;
register struct	mcis_scb	*mcis_scbp;
int				nowait;
{
	register struct	mcis_ldevmap	*ldp;
	pl_t				op;
	int				avld = -1;

	op = spldisk();
	ldp = mcis_scbp->s_ldmap;

	/* check for valid assignement */

	if ( ldp && ( ldp->LD_CB.a_targ == mcis_scbp->s_targ &&
		ldp->LD_CB.a_lun == mcis_scbp->s_lun ))
	{
		avld = ldp->LD_CB.a_ld;
	}
	else
	{
		/* allocate a new logical device number */

		ldp = LDMHD_FW( mcis_blkp );

		/* check for empty list */

		if ( ldp == &(mcis_blkp->b_ldmhd) )
		{
#ifdef MCIS_MLDEBUG
			cmn_err(CE_CONT,  "mcis_getld: empty ld list\n" );
#endif
			splx( op );
			return MCIS_HBALD;
		}
	}

	ldp->ld_avbk->ld_avfw = ldp->ld_avfw;
	ldp->ld_avfw->ld_avbk = ldp->ld_avbk;
	splx( op );
	ldp->ld_avbk = NULL;
	ldp->ld_avfw = NULL;
	ldp->ld_scbp = mcis_scbp;

	if ( avld != -1 )
		return avld;

	ldp->LD_CB.a_targ = mcis_scbp->s_targ;
	ldp->LD_CB.a_lun = mcis_scbp->s_lun;

	if ( mcis_docmd( mcis_blkp, ldp->LD_CMD, MCIS_HBALD, ISATTN_ICMD ))
	{
		mcis_freeld( mcis_blkp, ldp );
		return MCIS_HBALD;
	}

	/* set the ld ptr to me */

	mcis_scbp->s_ldmap = ldp;

	if ( nowait )
	{
		mcis_scbp->s_status = MCIS_WAITLD;
		mcis_blkp->b_ldm[ MCIS_HBALD ].ld_scbp = mcis_scbp;
		return avld;
	}

	/* wait for return status if specified - in device initialization */

	if ( mcis_pollret( mcis_blkp )
		|| mcis_blkp->b_intr_code != ISINTR_ICMD_OK )
	{
#ifdef MCIS_MLDEBUG
		cmn_err(CE_CONT,  "mcis_getld: fail = 0x%x retcod = 0x%x\n",
					ldp->LD_CMD, mcis_blkp->b_intr_code );
#endif
		mcis_freeld( mcis_blkp, mcis_scbp->s_ldmap );
		mcis_scbp->s_ldmap = NULL;
		return MCIS_HBALD;
	}
	
	return ldp->LD_CB.a_ld;
}


/*
 *	free logical device mapping
 */

void
mcis_freeld( mcis_blkp, ldp )
register struct	mcis_blk	*mcis_blkp;
register struct	mcis_ldevmap	*ldp;
{
	register struct	mcis_ldevmap	**ldmhdp;
	pl_t				op;

#ifdef MCIS_MLDEBUG2
	cmn_err(CE_CONT,  "mcis_freeld: entry\n" );
#endif
	/* relink back to the logical device map pool */

	op = spldisk();
	ldmhdp = &(mcis_blkp->b_ldmhd.ld_avbk);
	(*ldmhdp)->ld_avfw = ldp;
	ldp->ld_avbk = *ldmhdp;
	*ldmhdp = ldp;
	ldp->ld_avfw = &(mcis_blkp->b_ldmhd);
	splx( op );
}

#ifndef PDI_SVR42
/*
 * STATIC void
 * mcis_pass_thru0(buf_t *bp)
 *	Calls buf_breakup to make sure the job is correctly broken up/copied.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
mcis_pass_thru0(buf_t *bp)
{
	int	minor = geteminor( bp->b_edev );
	int	c = mcis_gtol[ MCIS_HAN( minor )];
	int	t = mcis_tcn_xlat[ MCIS_TCN( minor ) ];
	int	l = MCIS_LUN( minor );
	struct mcis_luq	*q = MCIS_LU_Q(c, t, l);

	if (!q->q_bcbp) {
		struct sb *sp;
		struct scsi_ad *sa;

		sp = bp->b_priv.un_ptr;
		sa = &sp->SCB.sc_dev;
		if ((q->q_bcbp = sdi_getbcb(sa, KM_SLEEP)) == NULL) {
			sp->SCB.sc_comp_code = SDI_ABORT;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		q->q_bcbp->bcb_granularity = 1;
		q->q_bcbp->bcb_addrtypes = BA_KVIRT;
		q->q_bcbp->bcb_flags = BCB_SYNCHRONOUS;
		q->q_bcbp->bcb_max_xfer = mcis_hba_info.max_xfer - ptob(1);
		q->q_bcbp->bcb_physreqp->phys_align = MCIS_MEMALIGN;
		q->q_bcbp->bcb_physreqp->phys_boundary = 0;
		q->q_bcbp->bcb_physreqp->phys_dmasize = 32;
	}

	buf_breakup(mcis_pass_thru, bp, q->q_bcbp);
}
#endif /* !PDI_SVR42 */

/*
 * STATIC void
 * mcis_pass_thru(buf_t *bp)
 *	Send a pass-thru job to the HA board.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
mcis_pass_thru(buf_t *bp)
{
	register struct mcis_luq	*q;
	register struct sb		*sp;
	int				minor = geteminor( bp->b_edev );
	int				c = mcis_gtol[ MCIS_HAN( minor )];
	int				t = mcis_tcn_xlat[ MCIS_TCN( minor ) ];
	int				l = MCIS_LUN( minor );
	pl_t				opri;
	caddr_t *addr;
	char op;
	daddr_t blkno_adjust;

#ifdef PDI_SVR42
	sp = (struct sb *)bp->b_private;
#else
	sp = bp->b_priv.un_ptr;
#endif

	sp->SCB.sc_wd = (long)bp;
	sp->SCB.sc_datapt = (caddr_t)paddr( bp );
	sp->SCB.sc_datasz = bp->b_bcount;
	sp->SCB.sc_int = mcis_int;

	sdi_translate( sp, bp->b_flags, bp->b_proc, KM_SLEEP );

	q = MCIS_LU_Q( c, t, l );
#if (PDI_VERSION >= PDI_UNIXWARE20)
	/*
	 * (This is a workaround for the 2G limitation for physiock offset.)
	 * Set new block number in the SCSI command if breakup occurred
	 * by adjusting the starting block number with the adjustment 
	 * from b_blkno.  Starting block number was saved in b_priv2,
	 * and having told physiock that the offset was 0, we add
	 * the adjustment now.
	 */
	blkno_adjust = bp->b_blkno;
	op = q->q_sc_cmd[0];
	if (op == SS_READ || op == SS_WRITE) {
		struct scs *scs = (struct scs *)(void *)q->q_sc_cmd;
		daddr_t blkno;

		if (blkno_adjust) {
			blkno = bp->b_priv2.un_int + blkno_adjust;
			scs->ss_addr1 = (blkno & 0x1F0000) >> 16;
			scs->ss_addr  = sdi_swap16(blkno);
		}
		scs->ss_len   = (char)(bp->b_bcount >> MCIS_BLKSHFT);
	}
	if (op == SM_READ || op == SM_WRITE) {
		daddr_t blkno;
		struct scm *scm = (struct scm *)(void *)((char *)q->q_sc_cmd - 2);

		if (blkno_adjust) {
			blkno = bp->b_priv2.un_int + blkno_adjust;
			scm->sm_addr = sdi_swap32(blkno);
		}
		scm->sm_len  = sdi_swap16(bp->b_bcount >> MCIS_BLKSHFT);
	}
#else /* (PDI_VERSION < PDI_UNIXWARE20) */
	/*
	 * Please NOTE:
	 * UnixWare 1.1 has 2Gig passthru limit
	 * due to physiock offset limitation.
	 */
	op = q->q_sc_cmd[0];
	if (op == SS_READ || op == SS_WRITE) {
		struct scs *scs = (struct scs *)(void *)q->q_sc_cmd;
		daddr_t blkno = bp->b_blkno;
		scs->ss_addr1 = (blkno & 0x1F0000) >> 16;
		scs->ss_addr  = sdi_swap16(blkno);
		scs->ss_len   = (char)(bp->b_bcount >> MCIS_BLKSHFT);
	}
	if (op == SM_READ || op == SM_WRITE) {
		struct scm *scm = (struct scm *)(void *)((char *)q->q_sc_cmd - 2);
		scm->sm_addr = sdi_swap32(bp->b_blkno);
		scm->sm_len  = sdi_swap16(bp->b_bcount >> MCIS_BLKSHFT);
	}
#endif /* (PDI_VERSION < PDI_UNIXWARE20) */

	sdi_icmd(sp, KM_SLEEP);
}

/*
** Function name: mcis_int()
**
** Description:
**	This is the interrupt handler for pass-thru jobs.  It just
**	wakes up the sleeping process.
*/

void
mcis_int( sp )
struct sb *sp;
{
	struct buf *bp;

#ifdef MCIS_DEBUG
	if ( mcis_debug > 0 )
		cmn_err(CE_CONT,  "mcis_int: sp = %x\n",sp );
#endif

	bp = (struct buf *)sp->SCB.sc_wd;
	biodone( bp );
}

#if PDI_VERSION >= PDI_UNIXWARE20

int
mcis__mca_conf(HBA_IDATA_STRUCT *idp, int *ioalen, int *memalen)
{
	unsigned	char	pos[5];

	if (cm_read_devconfig(idp->idata_rmkey, 0, pos, 5) != 5) {
		cmn_err(CE_CONT,
			"!%s could not read POS registers for MCA device\n",
			idp->name);
		return (1);
	}

	idp->idata_memaddr = 0xC0000 + (0x10000 * (pos[2] >> 7)) +
			     (0x2000 * ((pos[2] >> 4) & 0x7));
	if (idp->idata_memaddr == 0xDE000) {
		idp->idata_memaddr = 0;
	}

	idp->ioaddr1 = (ulong)(0x3540 + (((pos[2] >> 1) & 0x7) * 0x8));

	idp->iov = 14;
	idp->dmachan1 = -1;

	*ioalen = 8;
	if (idp->idata_memaddr == 0)	{
		*memalen = 0;
	}
	else	{
		*memalen = 2;
	}

	return (0);
}

/*
 * STATIC void *
 * mcis_kmem_zalloc_physreq (size_t size, int flags)
 *
 * function to be used in place of kmem_zalloc
 * which allocates contiguous memory, using kmem_alloc_physreq,
 * and zero's the memory.
 *
 * Entry/Exit Locks: None.
 */
STATIC void *
mcis_kmem_zalloc_physreq (size_t size, int flags)
{
	void *mem;
	physreq_t *preq = physreq_alloc(flags);
	if (preq == NULL)
		return NULL;
	preq->phys_align = MCIS_MEMALIGN;
	preq->phys_boundary = 0;
	preq->phys_dmasize = 32;
	preq->phys_flags |= PREQ_PHYSCONTIG;
	if (!physreq_prep(preq, flags)) {
		physreq_free(preq);
		return NULL;
	}
	mem = kmem_alloc_physreq(size, preq, flags);
	physreq_free(preq);
	if (mem)
		bzero(mem, size);
	return mem;
}
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */
