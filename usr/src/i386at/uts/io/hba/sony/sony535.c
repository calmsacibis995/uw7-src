#ident	"@(#)kern-pdi:io/hba/sony/sony535.c	1.9"
#ident	"$Header$"

/******************************************************************************
 ******************************************************************************
 *
 *	SONY535.C
 *
 *	SONY CDU535 Version 0.00
 *
 ******************************************************************************
 *****************************************************************************/

/******************************************************************************
 *
 *	INCLUDES
 *
 *****************************************************************************/

#ifdef _KERNEL_HEADERS
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <mem/immu.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <io/target/scsi.h>
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>

#if (PDI_VERSION >= 4)
#include <io/autoconf/resmgr/resmgr.h>
#endif /* (PDI_VERSION >= 4 ) */

#if (PDI_VERSION <= 1)
#include <io/target/dynstructs.h>
#include <io/hba/sony/sony.h>
#else /* !(PDI_VERSION <= 1) */
#include <io/target/sdi/dynstructs.h>
#include <io/hba/sony/sony.h>
#endif /* !(PDI_VERSION <= 1) */

#include <util/mod/moddefs.h>
#include <io/dma.h>
#include <io/hba/hba.h>

#if PDI_VERSION >= PDI_SVR42MP
#include <util/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <io/ddi.h>
#include <io/ddi_i386at.h>
#else /* _KERNEL_HEADERS */
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/signal.h>
#include <sys/cmn_err.h>
#include <sys/buf.h>
#include <sys/immu.h>
#include <sys/conf.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/dynstructs.h>
#include <sys/sony.h>
#include <sys/moddefs.h>
#include <sys/dma.h>
#include <sys/hba.h>
#if (PDI_VERSION >= 4)
#include <sys/resmgr.h>
#endif /* (PDI_VERSION >= 4) */

#if PDI_VERSION >= PDI_SVR42MP
#include <sys/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#ifdef DDICHECK
#include <io/ddicheck.h>
#endif
#endif /* _KERNEL_HEADERS */


/******************************************************************************
 *
 *	DEFINES / MACROS
 *
 *****************************************************************************/

#define SONY535_TIMEOUT	TRUE

#define cmdReg(iobase)    (iobase)
#define statReg(iobase)   (iobase)     /* status register */
#define rdReg(iobase)     (iobase+1)   /* read data register */
#define flagReg(iobase)   (iobase+2)   /* flag register */
#define maskReg(iobase)   (iobase+2)   /* mask register */
#define selectReg(iobase) (iobase+3)   /* Drive select */
#define resetReg(iobase)  (iobase+3)   /* Reset register */

#define WR_PARAM(iobase, p)     outb(iobase, p)
#define WR_CMD(iobase, p) 	outb(iobase, p)


/******************************************************************************
 *
 *	GLOBALS
 *
 *****************************************************************************/



/******************************************************************************
 *
 *	EXTERNALS
 *
 *****************************************************************************/


extern void		sony_flushq( scsi_lu_t *q, int cc );
extern void             sony_next( scsi_lu_t *q );
extern int 		sony535_dmaon;
extern int		sony_sleepflag;	/* KM_SLEEP/KM_NOSLEEP */
extern scsi_ha_t	*sony_sc_ha;	/* host adapter structs */
extern int		sony_sc_ha_size;/* size of sony_sc_ha alloc */
extern ctrl_info_t	*sony_ctrl;	/* local controller structs */
extern int		sony_ctrl_size;	/* size of sony_ctrl alloc */
extern int 	sony535_dataretry;
extern int 	sony535_spinwait;
extern int 	sony535_resultretry;
extern int	sony535_max_idle_time; 
extern ushort_t sony535_max_tries; 
extern int	sony535_readchktime;
extern int	sony535_controlchktime;
extern int	sony535_control_timeout;
extern int	sony535_data_timeout;
extern int	sony_gtol[];		/* global to local */


/******************************************************************************
 *
 *	PROTOTYPES
 *
 *****************************************************************************/

/*
 *	sony535 support
 */

STATIC int		sony535_stat2( struct scsi_ha *ha );
STATIC void 		sony535_reconfigure(scsi_ha_t *ha);
STATIC void 		sony535_rereconfigure(scsi_ha_t *ha);
STATIC void 		sony535_error();
STATIC int		sony535_sendcmd();
STATIC int		sony535bdinit(int c, HBA_IDATA_STRUCT *idata);
STATIC int 		sony535_issue_rdcmd();
STATIC int 		sony535_reissue_rdcmd();
STATIC int		sony535_spin ( struct scsi_ha *ha, int cmd, int intr);
STATIC int		sony535_clrerr (scsi_ha_t *ha);
STATIC int		sony535_chkread(sblk_t *sp);
#if PDI_VERSION >= PDI_UNIXWARE20
STATIC int		sony535verify(int iobase);
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */
STATIC void		sony535_setintr(int iobase, int on);
STATIC void		sony535_readtoc ( struct scsi_ha *ha, int fromrdcap);
STATIC void		sony535_get_result( scsi_ha_t *ha, uchar_t *resbuf, ulong_t *ressize);
STATIC void		sony535_updatestatus ( scsi_ha_t *ha, uchar_t *res );
STATIC int		sony535_setmode( scsi_ha_t *ha, uchar_t newmode );
STATIC void             sony535_rdcap(sblk_t * sp);
STATIC void             sony535intr( struct scsi_ha *ha );
STATIC int              sony535_chkmedia(scsi_ha_t *ha);

struct sonyvector sony535vector = {
        sony535bdinit,
        sony535intr,
        sony535_rdcap,
        sony535_chkmedia,
        sony535_issue_rdcmd,
#if PDI_VERSION >= PDI_UNIXWARE20
        sony535_error,
	sony535verify
#else  /* PDI_VERSION < PDI_UNIXWARE20 */
        sony535_error
#endif /* PDI_VERSION < PDI_UNIXWARE20 */
};

#ifdef SONY535_TIMEOUT
STATIC void		sony535_timer( scsi_ha_t  *ha );
#endif
/*
 *	cdrom block translation support
 */

STATIC void             sony535blkcnt2buf(uint_t blkcnt, unchar *buf);
STATIC int 		sony535msf2sector(int m, int s, int f);
STATIC void 		sony535sector2msf(int sector, unchar *msf);

/*****************************************************************************
 *****************************************************************************
 *
 *	PDI/KERNEL INTERFACE ROUTINES
 *
 *****************************************************************************
 ****************************************************************************/


int
sony535_control( struct scsi_ha	*ha )

{
	drv_info_t     		*drv;
	capacity_t      	cap;
	uchar_t			*resbuf = ha->ha_resbuf;
        ulong_t                 ressize;
	sblk_t 			*sp;
	struct sb      		*sb;
	struct scsi_ad *sa;
	int             c, t, l;

	sp = ha->ha_cmd_sp;
	if (sp) {
		sb = (struct sb *)&sp->sbp->sb;
		sa = (struct scsi_ad *) & sb->SCB.sc_dev;
		c = sony_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
	}

	/* this routine sets status in the ha structure */
	sony535_get_result(ha, resbuf, &ressize);
#ifdef SONY535_ERRS
cmn_err(CE_WARN, "SONY535: control resbuf0=%x resbuf1=%x ressize=%x", resbuf[0], resbuf[1], ressize);
#endif
        if (( ressize < 1 ) || ( resbuf[0]&ANY_ERROR )) {
          	/*
                 *      something went wrong
                 */
#ifdef SONY535_ERRS
                cmn_err(CE_WARN, "SONY535: control intr cmd res =%x ressize=%x", resbuf[0], ressize);
#endif
		if (sp) {
		   sony535_clrerr (ha);
		   if (sp->s_tries <= sony535_max_tries) {
			sp->s_tries++;
			if (ha->ha_casestate <= SONY535_CONFCASES) {
				sony535_reconfigure(ha);
			} else {
				ha->ha_casestate = SONY535_RDCAPTOCSPINUP;
				/* restart read toc state machine */
        			sony535_spin(ha, SONY535_SPIN_UP, SONY535_WANTINTR);
			}
		   } else {
			scsi_lu_t	*q;	/* queue ptr */

			ha->ha_casestate = 0;
			sony535_error(ha, sp);
			ha->ha_status = SONY535_ERROR;
			q = &SONY_LU_Q( c, t, l );
			sony_flushq(q, SDI_HAERR);
		   }
		} 
#ifdef SONY535_ERRS
		else {
                	cmn_err(CE_WARN, "SONY535: control intr while polling");
		}
#endif
		return (0);
       	}

	switch (ha->ha_casestate) {

		case SONY535_RECONFTOCSPINUP:
		case SONY535_RDCAPTOCSPINUP:
			if (ha->ha_casestate == SONY535_RECONFTOCSPINUP)
				ha->ha_casestate = SONY535_RECONFTOCDATA;
			else
				 ha->ha_casestate = SONY535_RDCAPTOCDATA;

			ha->ha_statsize = SONY535_TOCSIZE+1;
			sony535_sendcmd(ha, SONY535_GET_TOC_DATA, 0, NULL, 0);
			break;

		case SONY535_RDCAPTOCDATA:
			ha->ha_casestate = 0;

			drv = sony_ctrl[c].drive[t];

			/* check out start address offset algorithm for audio*/
			/* (first-1) * 5 + 17 - 1 */

			bcopy((caddr_t) &resbuf[1], (caddr_t) drv, (size_t) SONY535_TOCSIZE);
			drv->starting_track = (int) BCD2BIN(drv->drvtoc.trk_low);
			drv->ending_track = (int) BCD2BIN(drv->drvtoc.trk_high);

			drv->startsect = sony535msf2sector(drv->drvtoc.start_sect_m, drv->drvtoc.start_sect_s, drv->drvtoc.start_sect_f);
			drv->lastsect  = sony535msf2sector(drv->drvtoc.end_sect_m, drv->drvtoc.end_sect_s, drv->drvtoc.end_sect_f);
			drv->drvsize = drv->lastsect - drv->startsect;

			cap.cd_addr = sdi_swap32 ( drv->lastsect - drv->startsect);
			cap.cd_len = sdi_swap32 (SONY_BLKSIZE);
			bcopy ( (char *)&cap, sb->SCB.sc_datapt, RDCAPSZ );
#ifdef SONY535_DEBUG
			cmn_err(CE_NOTE, "sony535intr: start block %x, last block %x", drv->startsect, drv->lastsect);
#endif
		        ha->ha_cmd_sp = 0;
                        sb->SCB.sc_comp_code = SDI_ASW;
                        sdi_callback(sb);

			break;

		case SONY535_RECONFTOCDATA:
			sony535_issue_rdcmd(ha, sp);

			break;
	}
	return (0);
}

/*****************************************************************************
 *****************************************************************************
 *
 *	SONY535 HARDWARE SUPPORT
 *
 *		sony535intr		interrupt routine
 *
 *****************************************************************************
 ****************************************************************************/
/******************************************************************************
 *
 *	sony535intr( unsigned int vec )
 *
 *	Interrupt handler for the board
 *
 *	Entry :
 *		vec		interrupt vector
 *
 *	Exit :
 *		Nothing
 *
 *
 *****************************************************************************/

void
sony535intr( struct scsi_ha *ha )

{
#ifdef SONY535_DEBUG
        cmn_err( CE_CONT, "sony535:(intr)");
#endif

	if (ha->ha_driveStatus & SONY535_NOTEXPECTINT) {
#ifdef SONY535_ERRS
		cmn_err( CE_CONT, "sony535:(intr). ignore unsolicited interrupt" );
#endif
		return;
	}

	if (ha->ha_casestate != SONY535_RDDATA) {
		sony535_control(ha);
	} else {
		ha->ha_casestate = 0;
		if (ha->ha_cmd_sp)
			sony535_chkread(ha->ha_cmd_sp);
	}
}


void
sony535_updatestatus( struct scsi_ha *ha, uchar_t *resbuf )

{

	ha->ha_status = resbuf [0];
        if ((resbuf[0]&ANY_ERROR)) {
        	ha->ha_lasterr = resbuf [0];
	}


        if (!(resbuf[0] & (S1B1_TWO_BYTES|S1B1_NOT_SPINNING))) {
                /*
                 *      spinning
                 */
                ha->ha_driveStatus |= SONY535_DISK | SONY535_SPINNING;
                ha->ha_driveStatus &= ~SONY535_NODISK;
        }
        else {
                /*
                 *      not spinning
                 */
                ha->ha_driveStatus &= ~SONY535_SPINNING;
        }

        if (resbuf[0] & (S1B1_NO_CADDY|S1B1_EJECT_PRESSED)) {
                /*
                 *      if door open or eject pressed
                 */
                ha->ha_driveStatus |= SONY535_CHANGE | SONY535_NODISK;
                ha->ha_driveStatus &= ~(SONY535_DISK|SONY535_SPINNING);
        }
}

/******************************************************************************
 *
 *	sony535_get_result((struct scsi_ha *ha,
 * 			uchar_t *resbuf, ulong_t *ressize)
 *
 *      Read data from the command result register
 *
 *      Entry :
 *		*ha			ptr to host adapter structure
 *              *resbuf                 ptr to result buffer
 *              *ressize                ptr to loc to return result size in
 *
 *      Exit :
 *              Nothing
 *
 *      Notes :
 *              - based on flowcharts in manual
 *
 *
 *
 *****************************************************************************/

void
sony535_get_result(struct scsi_ha *ha, uchar_t *resbuf, ulong_t *ressize)

{
	int     iobase = ha->ha_iobase;
	int	retry =  sony535_resultretry;
	int	ridx = 0;
	int	i;

#ifdef SONY535_ERRS
	cmn_err(CE_WARN, "SONY535: in _get_result");
#endif
	bzero(resbuf, SONY535_RESHEAD); /* clear a small piece */

	for (i=0; i < ha->ha_statsize; i++) {
	    while( ((inb(flagReg(iobase))) & F_STATUS) && --retry ) {
		drv_usecwait(10);
	    }
	    if ( ! retry) {
#ifdef SONY535_ERRS
			cmn_err(CE_WARN, "SONY535: !retry in _get_result");
#endif
		break;
	    }

            if ( !((inb(flagReg(iobase))) & F_STATUS) && (ridx < SONY535_RESSIZE)) {
                resbuf[ridx++] = inb(statReg(iobase));
            }
	}
	*ressize = ridx;
	ha->ha_statsize = 1; /* usual status length */

	if (ridx)
		sony535_updatestatus(ha, resbuf);

}


/*******************************************************************************
 *
 *	sony535_reset ( struct scsi_ha *ha )
 *
 *	Reset the controller
 *
 *	Entry :
 *		Nothing
 *
 *	Exit :
 *		0			cool
 *		-1			unable to reset drive
 *	
 ******************************************************************************/

int 
sony535_reset ( struct scsi_ha *ha )

{
	uchar_t			*resbuf = ha->ha_resbuf;
        ulong_t                 ressize;
	int			iobase = ha->ha_iobase;

        /*
         *      reset drive by reading reset register
         */
#ifdef SONY535_ERRS
       	cmn_err(CE_WARN, "SONY535: in _reset");
#endif

        inb (resetReg(iobase));

	/* let the reset settle */
	drv_usecwait(200000);

	sony535_get_result(ha, resbuf, &ressize);
        if  (( ressize < 1 ) || (resbuf[0] & ANY_ERROR)) {
               	/*
               	 *      something went wrong
               	 */
#ifdef SONY535_ERRS
               	cmn_err(CE_WARN, "SONY535: reset cmd res =%x", resbuf[0]);
#endif
		return (-1);
	}

	sony535_setintr(iobase, INTERRUPT_MASK|BURST_MASK);

	return ( 0 );
}

void
sony535_setintr(int iobase, int on)
{
    	if (on)
        	outb(maskReg(iobase), INTERRUPT_MASK|BURST_MASK);
    	else
        	outb(maskReg(iobase), 0);
	/* the act of changing the mask seems to generate an interrupt */
	drv_usecwait(10);

}


/******************************************************************************
 *
 *	sony535bdinit( int c, HBA_IDATA_STRUCT *idata )
 *
 *	Initialize the SONY535 controller 
 *
 *	Entry :
 *		c		controller number( local )
 *		*idata		ptr to hardware info
 *
 *	Exit :
 *		-1		no controller
 *		0		no disks found
 *		x		number of disks found
 *
 *	Notes :
 *
 *****************************************************************************/

int
sony535bdinit( int c, HBA_IDATA_STRUCT *idata)
{
	register ushort_t	iobase = idata->ioaddr1;
	int 		i;
	drv_info_t	*drv;
	uint_t ndrives = 1;
	int addrok; 
	scsi_ha_t *ha = &sony_sc_ha[c];
	int	mask;
        ulong_t ressize;
	uchar_t	*resbuf = ha->ha_resbuf;

	sony_ctrl[ c ].flags = 0;

	/* 
	 * DMA can be disabled from space.c, so only allow DMA if 
	 * sony535_dmaon is > 0
	 */
	if (sony535_dmaon <= 0)
		idata->dmachan1 = -1;

        /* clear any pending int's */
        inb(statReg(iobase));
        inb(statReg(iobase));

	/* de-select all */
	outb(selectReg(iobase), 0xFF);

        /* OK, this isn't documented, but we observe that the high bits of
         * the flag register are the same as the high bits from the last
         * read of the software reset register -- so, we do a reset,
         * remember what the returned value was, and see if the flag reg
         * returns the same thing modulo the low-order bits.
         */

        i = inb(resetReg(iobase));
        addrok = ((inb(flagReg(iobase)) & 0x7C) == (i & 0x7C)) && (i != 0xFF);

        if (!addrok) {
                /*
                 *      wrong address stuff
                 */
#ifdef SONY535_ERRS
                cmn_err(CE_NOTE, "SONY535: bdinit bad ioaddress=%x", iobase);
#endif
                return(-1);         /* no adapter found, done */
	}

	/* let the reset settle */
	drv_usecwait(5000);


	/* turn off Burst mode and interrupts */
	sony535_setintr(iobase, 0);

	mask = 1<<0;  /* just unit 0 for now */
        /* select drive zero (low active)  for now ?? */
        outb(selectReg(iobase), ~mask);

#ifdef SONY535_ERRS
                cmn_err(CE_NOTE, "SONY535: bdinit ioaddress=%x", iobase);
#endif

#ifdef NOTYET
        if (sony535_setmode(ha, MODE_DATA) < 0) {
#ifdef SONY535_ERRS
                cmn_err(CE_NOTE, "SONY535: setmode in bdinit failed");
#endif
		return (-1);
	}
#endif

	/* let settle some more */
	drv_usecwait(1000000);
	sony535_sendcmd(ha, SONY535_INQUIRY, 2000, NULL, 0);

	ha->ha_statsize = SONY535_INQUIRSIZE;
	sony535_get_result(ha, resbuf, &ressize);

        if  ( ressize < 1 ) {
                /*
                 *      something went wrong
                 */
#ifdef SONY535_ERRS
                cmn_err(CE_NOTE, "SONY535: Cannot INQUIRY resbuf=%x ressize=%x", resbuf[0], ressize);
drv_usecwait(1000000);
#endif
		sony535_clrerr (ha);
                return (-1);
        } else  {
		int j;

		for (j=0; j<15; j++)
			if (resbuf[j+10] == '5')
				break;

		if ((j == 15) || resbuf[j+11] != '3' || resbuf[j+12] != '5') {
        		bcopy((caddr_t)&resbuf[10], (caddr_t)ha->ha_sony_diskprod, 15);
        		ha->ha_sony_diskprod[15] = '\0';
                	/*
                 	 *      bogus response
                 	 */
                	cmn_err(CE_WARN, "!SONY535: This may not be a 535. product=%s", ha->ha_sony_diskprod);
        	}
	}


        bcopy((caddr_t)&resbuf[1], (caddr_t)ha->ha_sony_vendor, 9);
        ha->ha_sony_vendor[9] = '\0';
        bcopy((caddr_t)&resbuf[10], (caddr_t)ha->ha_sony_diskprod, 15);
        ha->ha_sony_diskprod[15] = '\0';
        bcopy((caddr_t)&resbuf[25], (caddr_t)ha->ha_sony_drive_ver, 4);
        ha->ha_sony_drive_ver[4] = '\0';


        sony_ctrl[ c ].num_drives = ndrives;
        sony_ctrl[ c ].iobase = iobase;

        /*
         * run through and allocate drives/dig up info on them
         */

        for ( i = 0; i < ndrives; i++ ) {
                if (( drv = ( drv_info_t * ) kmem_zalloc( sizeof( struct drv_info ), sony_sleepflag ) ) == NULL ) {
                        cmn_err( CE_WARN, "!sony535:(_bdinit) Unable to allocate drv structure\n");
                        return( -1 );
                }

                sony_ctrl[ c ].drive[ i ] = drv;
                cmn_err( CE_CONT, "!sony535:(_bdinit) Configured HA %d TC %d\n",c,i );
        }

	/* no longer done in start */
	sony535_spin(ha, SONY535_SPIN_UP, SONY535_POLL);

	/* turn on interrupts */
	sony535_setintr(iobase, INTERRUPT_MASK|BURST_MASK);

#ifdef NOTYET
	sony535_stat2( ha );
#endif

        return (ndrives);
}



int 
sony535_sendcmd(scsi_ha_t *ha, int cmd, int spintime, 
unchar cmdbuf[], int bufsize)
{
	int	i;
	int	iobase = ha->ha_iobase;

#ifdef SONY535_ERRS
        cmn_err(CE_NOTE, "SONY535: ****  CMD=%x", cmd);
#endif

	/*
         *      first clear any pending status if it doesn't clear after
         *      30 bytes then it isn't a real drive
         */

        for (i=30; i>0; --i) {
                if (inb(flagReg(iobase)) & F_STATUS)
                        break;
                inb(statReg(iobase));
        }

	/* the read does not generate an interrupt until the block is read */
	if ((spintime) || ((cmd == SONY535_READ1) && (ha->ha_dmachan <= 0) )) 
		ha->ha_driveStatus |= SONY535_NOTEXPECTINT;
	else
		ha->ha_driveStatus &= ~SONY535_NOTEXPECTINT;
		

	WR_CMD (iobase, cmd);
	for (i=0; i < bufsize; i++) {
		WR_PARAM(iobase, cmdbuf[i]);
	}

	if (spintime) {
		drv_usecwait(spintime);
		ha->ha_driveStatus &= ~SONY535_NOTEXPECTINT;
	} else {
#ifdef SONY535_TIMEOUT
		drv_getparm(LBOLT, (void *) &ha->ha_cmd_time);
		if (!ha->ha_tid) { 
#ifdef SONY535_DEBUG
			cmn_err(CE_NOTE, "sony535_timer is set");
#endif
			ha->ha_tid = ITIMEOUT(sony535_timer, (void *) ha, ((cmd==SONY535_READ1)?sony535_readchktime:sony535_controlchktime), (pl_t) pldisk);
		}
#endif
	}
	return (0);
}

STATIC void
sony535blkcnt2buf(uint_t blkcnt, unchar *buf)
{
        buf [ 0 ] = ( ( blkcnt >> 16 ) & 0xff );
        buf [ 1 ] = ( ( blkcnt >> 8 ) & 0xff );
        buf [ 2 ] = blkcnt & 0xff;
}

STATIC void 
sony535sector2msf(int sector, unchar *msf)
{
	msf[2] = BIN2BCD(sector % 75); sector /= 75;
	msf[1] = BIN2BCD(sector % 60);
	msf[0] = BIN2BCD(sector / 60);
}

STATIC int
sony535msf2sector(int m, int s, int f)

{
	m = BCD2BIN(m);
	s = BCD2BIN(s);
	f = BCD2BIN(f);
	return (f + 75 * (s + 60 * m));
}


int 
sony535_stat2 ( struct scsi_ha *ha )

{
	uchar_t			*resbuf = ha->ha_resbuf;
        ulong_t                 ressize;

        /*
         *      get status 2
         */
#ifdef SONY535_ERRS
       	cmn_err(CE_WARN, "SONY535: in status 2");
#endif

	sony535_sendcmd(ha, SONY535_GET_DRIVE_STAT2, 500, NULL, 0);

	sony535_get_result(ha, resbuf, &ressize);
        if  ( ressize < 1 ) {
               		/*
               	  	 *      something went wrong
               	 	 */
#ifdef SONY535_ERRS
               		cmn_err(CE_WARN, "SONY535: get status 2 failed");
#endif
			return (-1);
	}

	/* for purposes of uniquie bdinit */
	switch (resbuf[0] & 0xF0) {

		case 0x0:
		case 0x70:
		case 0x80:
		case 0x90:
			break;
		default:
			return (-1);
	}

	switch (resbuf[0] & 0xF) {

		case 0x0:
		case 0x2:
		case 0x6:
		case 0xE:
		case 0xF:
			break;
		default:
			return (-1);
	}

	if ((resbuf[0] & 0xF0) == S2B1_NO_DISK)
		ha->ha_driveStatus |= SONY535_NODISK;
	else
		ha->ha_driveStatus &= ~SONY535_NODISK;

#ifdef SONY535_ERRS
               	cmn_err(CE_WARN, "SONY535: status 2 = %x", resbuf[0]);
#endif

	return ( 0 );
}

int
sony535_chkmedia(scsi_ha_t *ha)
{
	sblk_t      *sp;
        struct sb   *sb;

#ifdef SONY535_DEBUG
	cmn_err(CE_NOTE, "sony535_chkmedia");
#endif
	ha->ha_status = 0;
	sp = ha->ha_cmd_sp;
	sb = (struct sb *)&sp->sbp->sb;

	if ( ha->ha_state == HA_UNINIT ) {
		cmn_err(CE_WARN, "!sony535_chkmedia: HA_UNINIT");
		ha->ha_cmd_sp = 0;
		sb->SCB.sc_comp_code = (unsigned long) SDI_ERROR;
		sdi_callback(sb);
		return(-1);
	}

	if (sony535_stat2( ha ) < 0) {
		cmn_err(CE_WARN, "!sony535_chkmedia: SONY535_NODISK");
		ha->ha_cmd_sp = 0;
		sb->SCB.sc_comp_code = (unsigned long) SDI_ERROR;
		sdi_callback(sb);
		return(-1);
	}

	ha->ha_cmd_sp = 0;
	sb->SCB.sc_comp_code = SDI_ASW;
        sdi_callback(sb);
	return (1);
}


int
sony535_clrerr (scsi_ha_t *ha)

{
	int 	iobase = ha->ha_iobase;
        int	flag, i;
        ulong_t ressize;
	uchar_t	resbuf[SONY535_RESHEAD];

        if ( !(flag = inb(flagReg(iobase))&F_DATA) ) {
                /*
                 *      terminate any pending read command
                 */
		sony535_sendcmd(ha, SONY535_HOLD, 500, NULL, 0);

		/* use local resbuf so not corrupt drive specific resbuf */
		sony535_get_result(ha, resbuf, &ressize);
        	if  (( ressize < 1 ) || (resbuf[0] & ANY_ERROR)) {
               		/*
               	  	 *      something went wrong
               	 	 */
#ifdef SONY535_ERRS
               	cmn_err(CE_WARN, "SONY535: clrerr cmd res =%x ressize=%x", resbuf[0], ressize);
#endif
			/* return only ??? */
			return (-1);
		}
	}

        if  ( !(flag & F_STATUS)) {
                /*
                 *      status is left and the interrupt routine SHOULD have
                 *      taken care of everything
                 */
                i = SONY535_RESSIZE;    /* set limit so we won't loop forever */
                do {
                        /*
                         *      clear out status buffer
                         */
                        inb(statReg(iobase));           /* read & discard */
                } while ( !(inb(flagReg(iobase))&F_STATUS) && i--);
        }

        /*
         *      check for remaining status
         */

        if ( ! (inb(flagReg(iobase)) & (F_DATA|F_STATUS)) ) {
                /*
                 *      something went wrong
                 */
#ifdef SONY535_ERRS
		cmn_err(CE_WARN, "SONY535: Something died in clear error!");
#endif
		return ( -1 );
        }

        return ( 0 );
}


int
sony535_chkread(sblk_t *sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t, l;
	scsi_lu_t	*q;		/* queue ptr */
	int		iobase;
	scsi_ha_t      *ha;
	int		i;
	int		retry; /* make it smaller for this routine */
	uchar_t	*	addr = (uchar_t *) sp->sbp->sb.SCB.sc_datapt;
	int 		cnt = sp->s_size*SONY_BLKSIZE;
        ulong_t 	ressize;
	uchar_t	*	resbuf;

#ifdef SONY535_ERRS
		cmn_err(CE_WARN, "SONY535: in chkread" );
#endif

		c = sony_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &SONY_LU_Q( c, t, l );
		ha = &sony_sc_ha[c];
		resbuf = ha->ha_resbuf;
		iobase = ha->ha_iobase;


		if (ha->ha_dmachan > 0) {
                    sony535_get_result(ha, resbuf, &ressize);
                    if  ((ressize < 1) || (resbuf[0] & ANY_ERROR)) {
#ifdef SONY535_ERRS
			cmn_err(CE_WARN, "SONY535: chkread result err=%x", resbuf[0]);
#endif
			if (sp->s_tries < sony535_max_tries) {
				if (sp->s_tries == 3) {
					sony535_reconfigure(ha);
					return(0);
				}
				if (sp->s_tries == 5) {
					sony535_rereconfigure(ha);
					return(0);
				}
				if ( sony535_reissue_rdcmd(ha, sp) >= 0)
					return (0);
			}
			cmn_err(CE_WARN, "sony535: cannot read block %x", (int)sp->s_blkno);
			cmn_err(CE_CONT, "SONY535: err=%x", resbuf[0]);
			sony535_error(ha, sp);
			q->q_flag &= ~QBUSY;
			q->q_count--;
			sony_next(q);
			return (0);
                    }
#ifdef SONY535_ERRS
		    cmn_err(CE_WARN, "SONY535: chkread - no error");
#endif
		    ha->ha_casestate = 0;
		    ha->ha_cmd_sp = NULL;
		    sb->SCB.sc_comp_code = SDI_ASW;
		    sdi_callback(sb);

		    q->q_flag &= ~QBUSY;
	  	    q->q_count--;
		    sony_next(q);
		    return (0);
		}


		retry = sony535_dataretry;
		for ( i = 0; i < cnt; i++ ) {

			    /* while no data */
        		    while ( ((inb(flagReg(iobase))) & F_DATA) && --retry) {
                		drv_usecwait(10);
        		    }
		    	    if ( !retry ) {
#ifdef SONY535_ERRS
				cmn_err(CE_WARN, "SONY535: testing for data i=%d",i);
#endif
				if (sp->s_tries < sony535_max_tries) {
				    	if (sp->s_tries == 3) {
						sony535_reconfigure(ha);
						return(0);
					}
				    	if (sp->s_tries == 5) {
						sony535_rereconfigure(ha);
						return(0);
					}
					if ( sony535_reissue_rdcmd(ha, sp) >= 0)
						return (0);
			    	}
				cmn_err(CE_WARN, "sony535: cannot read block %x", (int)sp->s_blkno);
				cmn_err(CE_CONT, "SONY535: err=%x", resbuf[0]);
				sony535_error(ha, sp);
				q->q_flag &= ~QBUSY;
				q->q_count--;
				sony_next(q);
				return (0);
			    }

			    *addr++ = inb(rdReg(iobase));
		}

                sony535_get_result(ha, resbuf, &ressize);
                if  ((ressize < 1) || (resbuf[0] & ANY_ERROR)) {
#ifdef SONY535_ERRS
			cmn_err(CE_WARN, "SONY535: chkread result err=%x", resbuf[0]);
#endif
			if (sp->s_tries < sony535_max_tries) {
				if (sp->s_tries == 3) {
					sony535_reconfigure(ha);
					return(0);
				}
				if (sp->s_tries == 5) {
					sony535_rereconfigure(ha);
					return(0);
				}
				if ( sony535_reissue_rdcmd(ha, sp) >= 0)
					return (0);
			}
			cmn_err(CE_WARN, "sony535: cannot read block %x", (int)sp->s_blkno);
			cmn_err(CE_CONT, "SONY535: err=%x", resbuf[0]);
			sony535_error(ha, sp);
			q->q_flag &= ~QBUSY;
			q->q_count--;
			sony_next(q);
			return (0);
                }

#ifdef SONY535_DEBUG
		cmn_err(CE_NOTE, "SONY535: finished reading the data cnt =%d", cnt);
#endif
		ha->ha_casestate = 0;
		ha->ha_cmd_sp = NULL;
		sb->SCB.sc_comp_code = SDI_ASW;
		sdi_callback(sb);

		q->q_flag &= ~QBUSY;
		q->q_count--;
		sony_next(q);
		return (0);
}

int
sony535_setmode( scsi_ha_t *ha, uchar_t newmode )

{
        ulong_t ressize;
	uchar_t	*resbuf = ha->ha_resbuf;
	uchar_t	param [ 5 ];


	param [ 0 ] = newmode;
	sony535_sendcmd (ha, SONY535_SET_MODE, 1000, param, 1);

        sony535_get_result(ha, resbuf, &ressize);
        if  (( ressize < 1 ) || (resbuf[0] & ~(S1B1_TWO_BYTES|S1B1_NOT_SPINNING))) {
               	/*
               	 *      something went wrong
               	 */
#ifdef SONY535_ERRS
               	cmn_err(CE_WARN, "SONY535: SET_MODE cmd res =%x ressize=%x", resbuf[0], ressize);
#endif
		sony535_clrerr (ha);
		return (-1);
	}
	return (0);
}


int
sony535_issue_rdcmd(scsi_ha_t *ha, sblk_t *sp)
{
	int	blkno = sp->s_blkno;
	int	blkcnt = sp->s_size;
	int 	channel;
	unchar  cmdbuf[6];

	ASSERT( sp != NULL );

	/* blkcnt == 0 means just seek */

	sony535blkcnt2buf(blkcnt, &cmdbuf[3]);
	sony535sector2msf(blkno, cmdbuf);

#ifdef SONY535_DEBUG
	cmn_err(CE_NOTE, "sony535_issue_rdcmd: BLK no %d, cnt %d ", 
			blkno, blkcnt);
#endif
	ha->ha_casestate = SONY535_RDDATA;

	sp->s_tries++;
	(void) sony535_sendcmd(ha, SONY535_READ1, 0, cmdbuf, 6);

	if ((channel = ha->ha_dmachan) <= 0)
		return (0);

#if PDI_VERSION >= PDI_SVR42MP
        dma_disable(channel);

        ha->ha_cb->targbufs->address = sp->s_paddr;
        ha->ha_cb->targbufs->count = (ushort_t)sp->s_size*SONY_BLKSIZE;
        ha->ha_cb->command = DMA_CMD_READ;

        if (dma_prog(ha->ha_cb, channel, DMA_NOSLEEP) == FALSE) {
                cmn_err(CE_NOTE, "SONY535: dma_prog() failed!");
                return (1);
        }
        dma_enable(channel);
#else
#define DMA_READ        0x45
#define DMA_STR         0x0A
        {
                uchar_t    b1_addr;
                uchar_t    b2_addr;
                uchar_t    b3_addr;
                uchar_t    b1_len;
                uchar_t    b2_len;
                paddr_t padr =  sp->s_paddr;
		int s;

                ushort_t size = (ushort_t)sp->s_size*SONY_BLKSIZE -1;


                b1_addr = (uchar_t)(padr & 0xFF);
                b2_addr = (uchar_t)((padr >> 8) & 0xFF);
                b3_addr = (uchar_t)((padr >> 16) & 0xFF);
                b1_len = (uchar_t)(size & 0xFF);
                b2_len = (uchar_t)((size >> 8) & 0xFF);

		s = splhi();

                outb( DMA_STR, 0x04|channel);

                outb(DMA1CBPFF, (char)0);
                outb(DMA1WMR, DMA_READ);
                outb(DMA1BCA1, b1_addr);
                outb(DMA1BCA1, b2_addr);
                outb(DMACH1PG, b3_addr);
                outb(DMA1BCWC1, b1_len);
                outb(DMA1BCWC1, b2_len);

		splx(s);

                outb( DMA_STR, channel);
        }
#endif

	return(0);
}


int
sony535_reissue_rdcmd(scsi_ha_t *ha, sblk_t *sp)
{
	sony535_clrerr (ha);
	return (sony535_issue_rdcmd(ha, sp));
}


void
sony535_error(scsi_ha_t *ha, sblk_t *sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	ha->ha_cmd_sp = NULL;
	sb->SCB.sc_comp_code = (ulong_t) SDI_HAERR;
	
	cmn_err(CE_WARN, "!sony535_error");

	sdi_callback(sb);
}


#ifdef SONY535_TIMEOUT
void
sony535_timer(scsi_ha_t  *ha)
{
	clock_t now;
	ulong_t elapsed;
	sblk_t *sp;


	ha->ha_tid = 0;
	drv_getparm(LBOLT, (void *) &now);
	elapsed = now - ha->ha_cmd_time;

	/* currently state machine reconfig, rdcap and read have an sp */
	if ((sp = ha->ha_cmd_sp) != NULL) {
		struct sb      *sb = (struct sb *) & sp->sbp->sb;
		struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
		int             c, t, l;
		scsi_lu_t	*q;		/* queue ptr */

		c = sony_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &SONY_LU_Q( c, t, l );

		if ( ha->ha_casestate != SONY535_RDDATA ) {
			if (elapsed > sony535_control_timeout) {
#ifdef SONY535_ERRS
			cmn_err(CE_NOTE, "sony535_timer:non data cmd timeout");
#endif
		     		sony535_control(ha);
			} else {
				ha->ha_tid = ITIMEOUT(sony535_timer, (void *) ha, sony535_controlchktime, (pl_t) pldisk);
			}
			return;
		}
		if (sp->s_tries <= sony535_max_tries) {
#ifdef SONY535_ERRS
                		cmn_err(CE_WARN, "SONY535: timer call chkread");
#endif
			if ((elapsed > sony535_data_timeout) || (ha->ha_dmachan<=0))
				sony535_chkread(sp);
			else
				ha->ha_tid = ITIMEOUT(sony535_timer, (void *) ha, sony535_controlchktime, (pl_t) pldisk);

			return;
		}
		cmn_err(CE_WARN, "sony535_timer abort command");
		sony535_error(ha, sp);
		ha->ha_status = SONY535_ERROR;
		sony_flushq(q, SDI_HAERR);
		return;
	} else if (elapsed > sony535_max_idle_time) {
		/* 
		 * there is not that much activity. Stop the the timer.
		 * It will be enabled again the next command.
		 */
#ifdef SONY535_DEBUG
		cmn_err(CE_NOTE, "sony535_timer is canceled");
#endif
		return;
	}
	ha->ha_tid = ITIMEOUT(sony535_timer, (void *) ha, sony535_controlchktime, (pl_t) pldisk);
}
#endif



void
sony535_reconfigure(scsi_ha_t *ha)
{
	sony535_clrerr (ha);
	sony535_reset(ha);
	sony535_readtoc(ha, SONY535_FROMRECONF); /* get the state machine going */

}

/* last desperate attempt to get this request to work */

void
sony535_rereconfigure(scsi_ha_t *ha)
{
	sony535_reset(ha);
	sony535_clrerr (ha);
	sony535_readtoc(ha, SONY535_FROMRECONF); /* get the state machine going */

}


/*******************************************************************************
 *
 *	sony535_rdcap ( sblk_t *sp )
 *
 *	Tell the pdi about the capacity of the drive
 *
 *	Entry :
 *		*sp		ptr to scsi job ptr
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void
sony535_rdcap(sblk_t * sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c;
	scsi_ha_t 	*ha;

	c = sony_gtol[SDI_HAN(sa)];
	ha = &sony_sc_ha[c];

	sony535_clrerr(ha);

	ha->ha_cmd_sp = sp;


	if (ha->ha_driveStatus&SONY535_NODISK) {
		sony535_error(ha, sp);
	} else {
		/* gets the state machine going with a spinup */
		sony535_readtoc(ha, SONY535_FROMRDCAP); 
	}
}


/*******************************************************************************
 *
 *	sony535_spin ( struct scsi_ha *ha, int cmd, int intr )
 *
 *	Spin Up/Down a disk
 *	if POLL mode - don't call from interrupt handler
 *
 *	Entry :
 *		unit			drive disk unit
 *		up			0 : spin down
 *					1 : spin up
 *
 *	Exit :
 *		0			cool -- disk up/down
 *		-1			error
 *
 ******************************************************************************/

int 
sony535_spin ( struct scsi_ha *ha, int cmd, int intr)

{
	uchar_t				*resbuf = ha->ha_resbuf;
        ulong_t                         ressize;

	/* SONY535_SPIN_UP or SONY535_SPIN_DOWN  */
	if (intr) {	
        	sony535_sendcmd(ha, cmd, 0, NULL, 0);
		return (0);
	}

        sony535_sendcmd(ha, cmd, 0, NULL, 0);

	/* this way don't inconvenience everybody else on system */
	delay(sony535_spinwait*HZ);

	sony535_get_result(ha, resbuf, &ressize);

	if ( (ressize < 1) || ((resbuf[0]&(S1B1_TWO_BYTES|S1B1_NOT_SPINNING)) && (cmd==SONY535_SPIN_UP)) ) {
                /*
                 *      something went wrong
                 */
#ifdef SONY535_ERRS
                cmn_err(CE_NOTE, "SONY535: Cannot spin up/down");
#endif
		sony535_clrerr (ha);
                return ( -1 );
        }
	return  ( 0 );
}


#ifdef NOTYET

/*******************************************************************************
 *
 *	sony535_stat ( struct scsi_ha *ha )
 *
 *	Return the state of the drive
 *
 *	Entry :
 *		ha			ptr to ha structure
 *
 *	Exit :
 *		x			drive state
 *
 ******************************************************************************/

int 
sony535_stat ( struct scsi_ha *ha )

{
        if (ha->ha_driveStatus & SONY535_DISK) {
                /*
                 *      if disk in drive clear changed flag
                 */
                ha->ha_driveStatus &= ~SONY535_CHANGE;
        }

        return(ha->ha_driveStatus);
}

#endif

/* get the state machine going */
void
sony535_readtoc ( struct scsi_ha *ha, int fromrdcap)
{
	if (fromrdcap)
		ha->ha_casestate = SONY535_RDCAPTOCSPINUP;
	else
		ha->ha_casestate = SONY535_RECONFTOCSPINUP;

        sony535_spin(ha, SONY535_SPIN_UP, SONY535_WANTINTR);
}

#if PDI_VERSION >= PDI_UNIXWARE20 
/*
 * int
 * sony535verify(int iobase)
 *
 * Description:
 *	Verify the board instance.
 */
int
sony535verify(int iobase)
{
	int	i;
	int	addrok;
	int	mask;

	/* clear any pending int's */
	inb(statReg(iobase));
	inb(statReg(iobase));

	/* de-select all */
	outb(selectReg(iobase), 0xFF);

	/* OK, this isn't documented, but we observe that the high bits of
	 * the flag register are the same as the high bits from the last
	 * read of the software reset register -- so, we do a reset,
	 * remember what the returned value was, and see if the flag reg
	 * returns the same thing modulo the low-order bits.
	 */
	i = inb(resetReg(iobase));
	addrok = ((inb(flagReg(iobase)) & 0x7C) == (i & 0x7C)) && (i != 0xFF);

	if (!addrok) {
		/*
		 *	wrong address stuff
		 */
#ifdef SONY535_ERRS
		cmn_err(CE_NOTE, "SONY535: verify() bad ioaddress=%x", iobase);
#endif
		return(-1);	/* no adapter found, done */
	}

	return(0);
}
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

