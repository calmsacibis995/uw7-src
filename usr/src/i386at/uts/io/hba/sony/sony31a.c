#ident	"@(#)kern-pdi:io/hba/sony/sony31a.c	1.8"
#ident	"$Header$"

/******************************************************************************
 ******************************************************************************
 *
 *	SONY31A.C
 *
 *	SONY CDU31A Version 0.00
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
#endif /* (PDI_VERSION >= 4) */

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
#if (PDI_VERSION >= 4)
#include <sys/resmgr.h>
#endif /* (PDI_VERSION >= 4) */

#include <sys/dynstructs.h>
#include <sys/sony.h>

#include <sys/moddefs.h>
#include <sys/dma.h>

#include <sys/hba.h>
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

#define SONY31A_TIMEOUT	TRUE


#define IS_ATTENTION(iobase)    ((inb(iobase) & ST_ATTN_BIT ) != 0 )
#define IS_BUSY(iobase)         ((inb(iobase) & ST_BUSY_BIT ) != 0 )
#define IS_DATARDY(iobase)      ((inb(iobase) & ST_DATA_RDY_BIT ) != 0)
#define IS_DATAREQ(iobase)      ((inb(iobase) & ST_DATA_REQUEST_BIT ) != 0)
#define IS_RESRDY(iobase)       ((inb(iobase) & ST_RES_RDY_BIT ) != 0)
#define IS_PARMWRDY(iobase)     ((inb(iobase+3) & SF_PARAM_WRITE_RDY_BIT ) != 0)
#define RESET_DRIVE(iobase)     outb((iobase+3), SC_DRIVE_RESET_BIT)
#define CLEAR_ATTENTION(iobase) outb((iobase+3),SC_ATTN_CLR_BIT )
#define CLEAR_RESRDY(iobase)    outb((iobase+3), SC_RES_RDY_CLR_BIT )
#define CLEAR_DATARDY(iobase)   outb((iobase+3), SC_DATA_RDY_CLR_BIT)
#define CLEAR_PARAM(iobase)     outb((iobase+3), SC_PARAM_CLR_BIT )
#define RD_STATUS(iobase)       inb(iobase)
#define RD_RESULT(iobase)       inb(iobase+1)
#define RD_DATA(iobase)         inb(iobase+2)
#define WR_PARAM(iobase, p)     outb((iobase+1),p)
#define WR_CMD(iobase, p, q)	if (q)\
				outb((iobase+3), q);\
				outb(iobase, p)
#define WR_CONTROL(iobase, x)	outb((iobase+3), x)


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

extern int		sony31a_spininc;
extern int		sony31a_dmaoff;
extern scsi_ha_t	*sony_sc_ha;		/* host adapter structs */
extern int		sony_sc_ha_size;	/* size of sony_sc_ha alloc */
extern ctrl_info_t	*sony_ctrl;		/* local controller structs */
extern int		sony_ctrl_size;		/* size of sony_ctrl alloc */
extern int		sony_sleepflag;		/* KM_SLEEP/KM_NOSLEEP */
extern int 		sony31a_cmdretry;
extern int 		sony31a_resultretry;
extern int 		sony31a_dataretry;

extern int		sony31a_max_idle_time; 
extern ushort_t 	sony31a_max_tries; 
extern int		sony31a_chktime;
extern int		sony31a_data_timeout;
extern int		sony31a_control_timeout;

extern int		sony_gtol[];		/* global to local */
extern void		sony_flushq( scsi_lu_t *q, int cc );
extern void             sony_next( scsi_lu_t *q );


/******************************************************************************
 *
 *	PROTOTYPES
 *
 *****************************************************************************/

/*
 *	sony31a support
 */

STATIC void 		sony31a_reconfigure(scsi_ha_t *ha);
STATIC void 		sony31a_rereconfigure(scsi_ha_t *ha);
STATIC int		sony31a_sendcmd();
STATIC int 		sony31abdinit( int c, HBA_IDATA_STRUCT *idata );
STATIC int 		sony31a_issue_rdcmd();
STATIC int 		sony31a_reissue_rdcmd();
STATIC int		sony31a_handle_attention ( scsi_ha_t *ha );
STATIC int		sony31a_spin ( struct scsi_ha *ha, int cmd, int intr);
STATIC int		sony31a_chkmedia(scsi_ha_t *ha);
STATIC int		sony31a_chkread(sblk_t *sp);
STATIC int		sony31a_stat ( struct scsi_ha *ha );
STATIC int		sony31a_abort(scsi_ha_t *ha);
#if PDI_VERSION >= PDI_UNIXWARE20
STATIC int		sony31averify(int iobase);
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */
STATIC void		sony31a_readtoc ( struct scsi_ha *ha, int fromrdcap);
STATIC void		sony31a_rdcap(sblk_t * sp);
STATIC void		sony31a_updateStatus ( scsi_ha_t *ha, uchar_t res );
STATIC void		sony31a_get_result( scsi_ha_t *ha, uchar_t *resbuf, ulong_t *ressize );
STATIC void		sony31aintr( struct scsi_ha *ha );
STATIC void 		sony31a_error();

struct sonyvector sony31avector = {
	sony31abdinit,
	sony31aintr,
	sony31a_rdcap,
	sony31a_chkmedia,
	sony31a_issue_rdcmd,
#if PDI_VERSION >= PDI_UNIXWARE20
	sony31a_error,
	sony31averify
#else  /* PDI_VERSION >= PDI_UNIXWARE20 */
	sony31a_error
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */
};


#ifdef SONY31A_TIMEOUT
STATIC void		sony31a_timer( scsi_ha_t  *ha );
#endif
/*
 *	cdrom block translation support
 */

STATIC void		sony31ablkcnt2buf(int blkcnt, unchar *buf);
STATIC int 		sony31amsf2sector(int m, int s, int f);
STATIC void 		sony31asector2msf(int sector, unchar *msf);

/*****************************************************************************
 *****************************************************************************
 *
 *	PDI/KERNEL INTERFACE ROUTINES
 *
 *****************************************************************************
 ****************************************************************************/

int
sony31a_control( struct scsi_ha	*ha )

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
	sony31a_get_result(ha, resbuf, &ressize);
	if ( ( ressize < 2 ) || ( ( resbuf[0] & SONY31A_ERR_BIT ) == SONY31A_ERR_BIT ) ) {
          	/*
                 *      something went wrong
                 */
#ifdef SONY31A_ERRS
                cmn_err(CE_WARN, "SONY31A: intr internal cmd res =%x sp=%x", resbuf[1], sp);
#endif
		if (sp) {
		   if (sp->s_tries <= sony31a_max_tries) {
			sp->s_tries++;
			if (ha->ha_casestate <= SONY31A_CONFCASES) {
				sony31a_rereconfigure(ha);
			} else if ((ha->ha_casestate == SONY31A_CHKMEDIASPIN1) || (ha->ha_casestate == SONY31A_CHKMEDIASPIN2)) {
#ifdef SONY31A_ERRS
                cmn_err(CE_WARN, "SONY31A: _chkmedia out of _control");
#endif
				sony31a_chkmedia(ha);
			} else {
 				/* speed this up for no disk, install */
				sp->s_tries+=sony31a_spininc;
				ha->ha_casestate = SONY31A_RDCAPTOCSPINUP;
        			sony31a_spin(ha, SONY31A_SPIN_UP_DISK, SONY31A_WANTINTR);
			}
		   } else {
			scsi_lu_t	*q;	/* queue ptr */

			ha->ha_casestate = 0;
			sony31a_error(ha, sp);
			ha->ha_status = SONY31A_ERROR;
			q = &SONY_LU_Q( c, t, l );
			sony_flushq(q, SDI_HAERR);
		   }
		} 
#ifdef SONY31A_ERRS
		else {
                	cmn_err(CE_WARN, "SONY31A: intr internal cmd NO sp");
		}
#endif
		return (0);
       	}

	switch (ha->ha_casestate) {
		case SONY31A_CHKMEDIASPIN1:
		case SONY31A_CHKMEDIASPIN2:
			sony31a_chkmedia(ha);
			
			break;
		case SONY31A_RDCAPTOCSPINUP:
		case SONY31A_RECONFTOCSPINUP:
			if (ha->ha_casestate == SONY31A_RDCAPTOCSPINUP)
				ha->ha_casestate = SONY31A_RDCAPTOCREAD;
			else
				ha->ha_casestate = SONY31A_RECONFTOCREAD;
#ifdef SONY31A_ERRS
                cmn_err(CE_WARN, "SONY31A: SONY31A_READ_TOC");
#endif
			sony31a_sendcmd(ha, SONY31A_READ_TOC, 0, NULL, 0, SC_RES_RDY_INT_EN_BIT);
			break;
		case SONY31A_RDCAPTOCREAD:
		case SONY31A_RECONFTOCREAD:
			if (ha->ha_casestate == SONY31A_RDCAPTOCREAD)
				ha->ha_casestate = SONY31A_RDCAPTOCDATA;
			else
				ha->ha_casestate = SONY31A_RECONFTOCDATA;
#ifdef SONY31A_ERRS
                cmn_err(CE_WARN, "SONY31A: SONY31A_GET_TOCDATA");
#endif
			sony31a_sendcmd(ha, SONY31A_GET_TOCDATA, 0, NULL, 0, SC_RES_RDY_INT_EN_BIT);
			break;
		case SONY31A_RDCAPTOCDATA:
			ha->ha_casestate = 0;

			drv = sony_ctrl[c].drive[t];

			bcopy((caddr_t) &resbuf[2], (caddr_t) drv, (size_t) SONY31A_TOCSIZE);

#ifdef SONY31A_ERRS
cmn_err(CE_NOTE, "trk_low %x trk_high %x trkcnt %x", drv->drvtoc.trk_low, drv->drvtoc.trk_high, drv->drvtoc.trkcnt);
#endif

			/* check algorithm for start address msf ?? */
			drv->starting_track = (int) BCD2BIN(drv->drvtoc.trk_low);
			drv->ending_track = (int) BCD2BIN(drv->drvtoc.trk_high);

			drv->startsect = sony31amsf2sector(drv->drvtoc.start_sect_m, drv->drvtoc.start_sect_s, drv->drvtoc.start_sect_f);
			drv->lastsect  = sony31amsf2sector(drv->drvtoc.end_sect_m, drv->drvtoc.end_sect_s, drv->drvtoc.end_sect_f);
			drv->drvsize = drv->lastsect - drv->startsect;

			cap.cd_addr = sdi_swap32 ( drv->lastsect - drv->startsect);
			cap.cd_len = sdi_swap32 (SONY_BLKSIZE);
			/* ? should not move more than the cmd asks for */
			bcopy ( (char *)&cap, sb->SCB.sc_datapt, RDCAPSZ );
#ifdef SONY31A_ERRS
			cmn_err(CE_NOTE, "sony31aintr: start block %x, last block %x", drv->startsect, drv->lastsect);
#endif
			ha->ha_casestate = 0;
		        ha->ha_cmd_sp = 0;
			if (sp) {
                        	sb->SCB.sc_comp_code = SDI_ASW;
                        	sdi_callback(sb);
			}
#ifdef SONY31A_ERRS
			else
                		cmn_err(CE_WARN, "SONY31A: NO sp RDCAPTOCD");
#endif

			break;

		case SONY31A_RECONFTOCDATA:
#ifdef SONY31A_ERRS
                cmn_err(CE_WARN, "SONY31A: RECONFTOCDATA");
#endif
			if (sp)
				sony31a_issue_rdcmd(ha, sp);
#ifdef SONY31A_ERRS
			else
                		cmn_err(CE_WARN, "SONY31A: NO sp RECONFTOCD");
#endif

			break;
	}
	return (0);
}

/*****************************************************************************
 *****************************************************************************
 *
 *	SONY31A HARDWARE SUPPORT
 *
 *		sony31aintr		interrupt routine
 *
 *****************************************************************************
 ****************************************************************************/
/******************************************************************************
 *
 *	sony31aintr( struct scsi_ha *ha )
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
sony31aintr( struct scsi_ha *ha )

{

#ifdef SONY31A_DEBUG
	cmn_err( CE_CONT, "sony31a:(intr) " );
#endif

	ha->ha_driveStatus |= SONY31A_HAVEINTRPTS;

	if (ha->ha_casestate != SONY31A_RDDATA) {
		sony31a_control(ha);
	} else {
		ha->ha_casestate = 0;
		sony31a_chkread(ha->ha_cmd_sp);
	}
}


/******************************************************************************
 *
 *	sony31a_get_result((struct scsi_ha *ha,
 * 				uchar_t *resbuf, ulong_t *ressize )
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

#define SONY31A_INITIAL_EIGHT 8
#define SONY31A_SET_OF_TEN    10 

void
sony31a_get_result(struct scsi_ha *ha, uchar_t *resbuf, ulong_t *ressize)

{
        long	res0, res1;
        int     i;
	int     iobase = ha->ha_iobase;
	int	retry = sony31a_resultretry;
	int	retry2 = sony31a_cmdretry;
	int	att;
	int	rdsize;

	bzero(resbuf, SONY31A_RESSIZE);

	while (sony31a_handle_attention(ha) && retry2--)
		drv_usecwait(10);

        while (( IS_BUSY(iobase) || !IS_RESRDY(iobase) ) && retry--) {
		sony31a_handle_attention (ha);
		drv_usecwait(10);
	}

        if ( IS_BUSY(iobase) || !IS_RESRDY(iobase) ) {
                /*
                 *      drive timed out
                 */
#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: result data never appeared iobase=%x", iobase);
#endif
                resbuf[0] = SONY31A_ERR_BIT;
                ha->ha_lasterr = resbuf[1] = ERR_TIMEOUT;
                *ressize = 2;
                return;
        }
        /*
         *      Get the first two bytes - figure out what else needs to be done
         */

        CLEAR_RESRDY(iobase);

        res0 = *resbuf++ = RD_RESULT(iobase);
        res1 = *resbuf++ = RD_RESULT(iobase);
        *ressize = 2;

        if ( res1 == 0 ) {
                /*
                 *      kludge up this error
                 */
#ifdef SONY31A_DEBUG
                cmn_err(CE_NOTE, "SONY31A: CGR: Setting Res0 to 0. iobase=%x", iobase);
#endif
                res0 = 0;
                resbuf -= 2;
                *resbuf = 0;
        }


        /*
         *      ERR_BIT means an error -- res1 will have the error code --
         *      otherwise the command succeeded and res1 will have the
         *      count of how many more status bytes are coming
         *
         *      the result reg can be read 10 bytes at a time -- wait for
         *      resrdy and read another 10
         */

        if ( ( res0 & 0xf0 ) != SONY31A_ERR_BIT ) {
                /*
                 *      no error -- read rest of first section
                 */

                ha->ha_lasterr = 0;
		/* using ha_status to track lasterr ??? */
		ha->ha_status = 0;

                if ( res1 == 0 ) {
                        /*
                         *      no more
                         */
                        return;
                }

		rdsize = res1 > SONY31A_INITIAL_EIGHT?
				 SONY31A_INITIAL_EIGHT : res1;
                for ( i = 0; i < rdsize; i++ ) {
                        *resbuf++ = RD_RESULT(iobase);
                }
                (*ressize) += rdsize;
                res1 -= rdsize;

                if ( res1 <= 0 ) {
                        /*
                         *      We read all results
                         */
                        return;
                }

                /*
                 *      now read in groups of 10
                 */

                while ( res1 > SONY31A_SET_OF_TEN ) {
                        /*
                         *      read in chunk-o-10
                         */

			retry = sony31a_resultretry;
                        while ( !IS_RESRDY(iobase) && retry--)
				drv_usecwait(10);
                        if ( !IS_RESRDY(iobase) ) {
                                /*
                                 *      something went wrong
                                 */
#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: final1 data never appeared. iobase=%x", iobase);
#endif
                                resbuf[0] = SONY31A_ERR_BIT;
                                ha->ha_lasterr = resbuf[1] = ERR_TIMEOUT;
                                *ressize = 2;
                                return;
                        }

                        CLEAR_RESRDY(iobase);

                        for ( i = 0; i < SONY31A_SET_OF_TEN; i++ ) {
                                *resbuf++ = RD_RESULT(iobase);
                        }
                	(*ressize) += SONY31A_SET_OF_TEN;
                        res1 -= SONY31A_SET_OF_TEN;
                }

                /*
                 *      now pick up final piece
                 */

                if ( res1 > 0 ) {
			retry = sony31a_resultretry;
                        while ( !IS_RESRDY(iobase) && retry--)
				drv_usecwait(10);
                        if ( !IS_RESRDY(iobase)) {
                                /*
                                 *      something went wrong
                                 */
#ifdef SONY31A_ERRS
                		cmn_err(CE_NOTE, "SONY31A: final2 data never appeared.");
#endif
                                resbuf[0] = SONY31A_ERR_BIT;
                                ha->ha_lasterr = resbuf[1] = ERR_TIMEOUT;
                                *ressize = 2;
                                return;
                        }

        		CLEAR_RESRDY(iobase);

                        while ( res1-- ) {
                                *resbuf++ = RD_RESULT(iobase);
                                (*ressize)++;
                        }
                }
        }
        else {
                /*
                 *      there was an error -- set the error code
                 */
		ha->ha_status = resbuf [1];
                ha->ha_lasterr = resbuf [1];
        }
}


/*******************************************************************************
 *
 *	sony31a_reset ( struct scsi_ha *ha )
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
sony31a_reset ( struct scsi_ha *ha )

{
	uchar_t				*resbuf = ha->ha_resbuf;
	ulong_t				ressize;
	uchar_t				param [ 5 ];
	int				iobase = ha->ha_iobase;
	int				attnwait = SONY31A_WAITATT; /* paranoid */

	/*
	 *	reset hardware
	 */
	RESET_DRIVE(iobase);

	while (!sony31a_handle_attention(ha) && attnwait--)
		drv_usecwait(100);
	while (sony31a_handle_attention(ha) && attnwait--)
		drv_usecwait(100);
	if ( IS_RESRDY(iobase) ) {
		/*
		 *	result is pending?
		 */
		sony31a_get_result ( ha, resbuf, &ressize );
	}


	/*
	 *	setup drive mechanical params
	 */

	param [ 0 ] = 5;		/* mech control */
	if (ha->ha_driveStatus&SONY31A_33A)
		param [ 1 ] = 7;	/* double speed/auto spinup/auto eject*/
	else
		param [ 1 ] = 3;		/* auto spinup/auto eject */

	sony31a_sendcmd (ha, SONY31A_SET_PARAM, 2000, param, 2, 0);

	sony31a_get_result(ha, resbuf, &ressize);

	if ( ( ressize < 2 ) || ( ( resbuf[0] & SONY31A_ERR_BIT ) == SONY31A_ERR_BIT ) ) {
		/*
		 *	something went wrong
		 */
#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: Cannot set Reset Param1. resbuf0=%x", resbuf[0]);
#endif
		return ( -1 );
	}


	return ( 0 );
}


/******************************************************************************
 *
 *	sony31abdinit( int c, HBA_IDATA_STRUCT *idata )
 *
 *	Initialize the SONY31A controller 
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
sony31abdinit( int c, HBA_IDATA_STRUCT *idata)
{
	register ushort_t	iobase = idata->ioaddr1;
	int 		i;
	drv_info_t		*drv;
	uint_t ndrives = 1;
	scsi_ha_t *ha = &sony_sc_ha[c];
        ulong_t ressize;
	uchar_t	*resbuf = ha->ha_resbuf;


	sony_ctrl[ c ].flags = 0;

	/* not always true that 0xFF at address without a board. Don't rely */
        if ( ( inb(iobase) == 0xff ) && ( inb(iobase+2) == 0xff ) && ( inb(iobase+3) == 0xff ) ) {
                /*
                 *      nothing at these addresses
                 */
                return ( -1 );
        }

	
	sony31a_reset (ha);           /* reset the drive ??? */

	/*
         *      get the configuration
         */

	sony31a_sendcmd(ha, SONY31A_GET_CFG, 500, NULL, 0, 0);

        sony31a_get_result(ha, resbuf, &ressize);

        if ( ( ressize < 2 ) || ( ( resbuf[0] & SONY31A_ERR_BIT ) == SONY31A_ERR_BIT ) ) {
                /*
                 *      something went wrong
                 */
#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: Cannot get configuration. adapter=%d" ,c);
#endif
                sony31a_reset (ha);         /* reset drive */
		return (-1);
	} else {
		int j;

		for (j=0; j<16; j++)
                        if (resbuf[j+10] == '3')
                                break;

                if ((j == 16) || ((resbuf[j+11] != '1') && (resbuf[j+11] != '3')) || resbuf[j+12] != 'A') {
                        bcopy((caddr_t)&resbuf[10], (caddr_t)ha->ha_sony_diskprod, 16);
			ha->ha_sony_diskprod[16] = '\0';
                        /*
                         *      bogus response
                         */
                        cmn_err(CE_WARN, "!SONY31A: This is not a 31A or 33A. product=%s", ha->ha_sony_diskprod);
                        return ( -1 );
                }
		if (resbuf[j+11] == '3') {
			ha->ha_driveStatus |= SONY31A_33A;
#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: Double Speed 33a");
#endif
		}
	}


       	/*
         *      got a sony 31a
         */

        bcopy((caddr_t)&resbuf[2],(caddr_t)ha->ha_sony_vendor,8);
	ha->ha_sony_vendor[8] = '\0';
        bcopy((caddr_t)&resbuf[10],(caddr_t)ha->ha_sony_diskprod,16);
	ha->ha_sony_diskprod[16] = '\0';
        bcopy((caddr_t)ha->ha_sony_diskprod, (caddr_t)ha->ha_sony_prod,17);
        bcopy((caddr_t)&resbuf[26],(caddr_t)ha->ha_sony_drive_ver,8);
	ha->ha_sony_drive_ver[8] = '\0';


	sony_ctrl[ c ].num_drives = ndrives;
	sony_ctrl[ c ].iobase = iobase;

	

	/*
	 * run through and allocate drives/dig up info on them
	 */

	for ( i = 0; i < ndrives; i++ ) {
		if (( drv = ( drv_info_t * ) kmem_zalloc( sizeof( struct drv_info ), sony_sleepflag ) ) == NULL ) {
			cmn_err( CE_WARN, "!sony:(_bdinit) Unable to allocate drv structure\n");
			return( -1 );
		}

		sony_ctrl[ c ].drive[ i ] = drv;
		cmn_err( CE_CONT, "!sony:(_bdinit) Configured HA %d TC %d\n",c,i );
	}

	/*
	 * For the case when a disk is inserted in the drive before the 
	 * driver is loaded, we need to spin up the drive to ensure that
	 * a meaningful mechanical status can be obtained later when 
	 * sony31a_chkmedia() gets that status.  If this is not done,
	 * sony31a_updateStatus() has no way of determing if a disk is
	 * inserted in the drive (a spin up has to occur before the 
	 * presence of a disk can be determined).
	 */
	sony31a_spin(ha, SONY31A_SPIN_UP_DISK, SONY31A_POLL);

	return(ndrives);
}


int 
sony31a_sendcmd(scsi_ha_t *ha, int cmd, int spintime, 
unchar cmdbuf[], int bufsize, int intmask)
{
	int	i;
	int	iobase = ha->ha_iobase;
	int	retry = sony31a_cmdretry;
	int	retry2 = sony31a_cmdretry;

#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: ****  CMD=%x", cmd);
#endif

	while (sony31a_handle_attention(ha) && retry--)
		drv_usecwait(10);

	retry = sony31a_cmdretry;
	while (IS_BUSY(iobase) && retry--) {
		retry2 = sony31a_cmdretry;
		while (sony31a_handle_attention(ha) && retry2--)
			drv_usecwait(10);
		drv_usecwait(10);
	}
        if ( IS_BUSY(iobase) ) {
                /*
                 *      drive timed out
                 */
#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: IS_BUSY iobase=%x", iobase);
#endif
                return (-1);
        }

	CLEAR_PARAM(iobase);
	retry = sony31a_cmdretry;
	while (!IS_PARMWRDY(iobase) && retry--) 
		drv_usecwait(10);
        if ( !IS_PARMWRDY(iobase) ) {
                /*
                 *      drive timed out
                 */
#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: !IS_PARMWRDY iobase=%x", iobase);
#endif
                return (-1);
        }
	for (i=0; i < bufsize; i++) {
		drv_usecwait(5);
		WR_PARAM(iobase, cmdbuf[i]);
	}
	drv_usecwait(5);

#ifdef SONY31A_TIMEOUT
	if (!spintime) {
		drv_getparm(LBOLT, (void *) &ha->ha_cmd_time);
		if(!ha->ha_tid) { 
#ifdef SONY31A_DEBUG
			cmn_err(CE_NOTE, "sony31a_timer is set");
#endif
			ha->ha_tid = ITIMEOUT(sony31a_timer, (void *) ha, sony31a_chktime, (pl_t) pldisk);
		}
	}
#endif
	WR_CMD (iobase, cmd, intmask );
	/* you may want to wait to see if the interrupt actually happens */
	/* hence  spintime is not redundant */
	if (spintime)
		drv_usecwait(spintime);
	return 0;
}

STATIC void 
sony31ablkcnt2buf(int blkcnt, unchar *buf)
{
	buf[ 0 ] = ( ( blkcnt >> 16 ) & 0xff );
        buf [ 1 ] = ( ( blkcnt >> 8 ) & 0xff );
        buf [ 2 ] = blkcnt & 0xff;
}

STATIC void 
sony31asector2msf(int sector, unchar *msf)
{
	msf[2] = BIN2BCD(sector % 75); sector /= 75;
	msf[1] = BIN2BCD(sector % 60);
	msf[0] = BIN2BCD(sector / 60);
}

STATIC int
sony31amsf2sector(int m, int s, int f)

{
	m = BCD2BIN(m);
	s = BCD2BIN(s);
	f = BCD2BIN(f);
	return (f + 75 * (s + 60 * m));
}


int
sony31a_chkmedia(scsi_ha_t *ha)
{
	sblk_t                  *sp;
        struct sb               *sb;

#ifdef SONY31A_DEBUG
	cmn_err(CE_NOTE, "sony31a_chkmedia: ");
#endif
	ha->ha_status = 0;
	sp = ha->ha_cmd_sp;
	sb = (struct sb *)&sp->sbp->sb;

	sony31a_stat(ha);

	if ( ha->ha_state == HA_UNINIT ) {
		cmn_err(CE_WARN, "!sony31a_chkmedia: HA_UNINIT");
		ha->ha_casestate = 0;
		ha->ha_cmd_sp = 0;
		sb->SCB.sc_comp_code = (unsigned long) SDI_ERROR;
		sdi_callback(sb);
		return(-1);
	}

	/*
	 * Spin up drive if there is a disk inserted and the drive
	 * is not spinning at this time (i.e., has auto spun down).
	 */
	if (( !(ha->ha_driveStatus&SONY31A_SPINNING)) &&
			( !(ha->ha_driveStatus&SONY31A_NODISK))){
		if ( ! ((ha->ha_casestate == SONY31A_CHKMEDIASPIN1) ||
			(ha->ha_casestate == SONY31A_CHKMEDIASPIN2))) {

			ha->ha_casestate = SONY31A_CHKMEDIASPIN1;
			sony31a_spin(ha, SONY31A_SPIN_UP_DISK, SONY31A_WANTINTR);
			return (0);
	    	}

		if (ha->ha_casestate == SONY31A_CHKMEDIASPIN1) {
			sony31a_reset(ha);
			ha->ha_casestate = SONY31A_CHKMEDIASPIN2;
			sony31a_spin(ha, SONY31A_SPIN_UP_DISK, SONY31A_WANTINTR);
			return (0);
		}
	}
	ha->ha_casestate = 0;

        if (ha->ha_driveStatus&SONY31A_NODISK) {
                cmn_err(CE_WARN, "!sony31a_chkmedia: SONY31A_NODISK failure");
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
sony31a_chkread(sblk_t *sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t, l;
	scsi_lu_t	*q;		/* queue ptr */
	int		iobase;
	scsi_ha_t      *ha;
	ulong_t		ressize;
	uchar_t		*resbuf;
	int		i;
	int		retry; /* make it smaller for this routine */
	uchar_t	*	addr = (uchar_t *) sp->sbp->sb.SCB.sc_datapt;
	int 		cnt = sp->s_size;
	int		status;

		c = sony_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &SONY_LU_Q( c, t, l );
		ha = &sony_sc_ha[c];
		resbuf = ha->ha_resbuf;
		iobase = ha->ha_iobase;

#ifdef SONY31A_DEBUG
	cmn_err(CE_NOTE, "sony31a_chkread: iobase %x, sb %x", iobase, sb);
#endif

		if ((ha->ha_dmachan > 0) && (!sony31a_dmaoff)) {
			if ( IS_RESRDY(iobase) ) {
			    sony31a_get_result (ha, resbuf, &ressize);
			    if ((ressize < 2) || ha->ha_status || ((resbuf[0] & SONY31A_ERR_BIT ) == SONY31A_ERR_BIT )) {
#ifdef SONY31A_ERRS
				cmn_err(CE_WARN, "SONY31A: chkread DMA error");
#endif
				if (sp->s_tries < sony31a_max_tries) {
				    	if (sp->s_tries == 3) {
						sony31a_reconfigure(ha);
						return (0);
					}
				    	if (sp->s_tries == 5) {
						sony31a_rereconfigure(ha);
						return(0);
					}
					sony31a_abort(ha);
					if ( sony31a_issue_rdcmd(ha, sp) >= 0)
						return (0);
			    	}
				if (ha->ha_lasterr == 0x30) /* focus error */
					cmn_err(CE_WARN, "sony31a: repeated focus errors. CD not of sufficient quality for this drive");
				else
					cmn_err(CE_WARN, "!sony31a lasterr= %x", ha->ha_lasterr);
				cmn_err(CE_WARN, "sony31a: cannot read block %x", (int)sp->s_blkno);
				sony31a_error(ha, sp);
				q->q_flag &= ~QBUSY;
				q->q_count--;
				sony_next(q);
				return (0) ;
			    }
			}

			CLEAR_DATARDY(iobase);

			ha->ha_casestate = 0;
			ha->ha_cmd_sp = 0;
                	sb->SCB.sc_comp_code = SDI_ASW;
                	sdi_callback(sb);

                	q->q_flag &= ~QBUSY;
                	q->q_count--;
                	sony_next(q);
	
			return (0);
		}


		for ( i = 0; i < cnt; i++ ) {
#ifdef SONY31A_TIMEOUT
			drv_getparm(LBOLT, (void *) &ha->ha_cmd_time);
#endif
			retry = sony31a_dataretry;
        		while (--retry) {
				status = RD_STATUS(iobase);
				if (((status & ST_DATA_RDY_BIT) != 0) &&
				    ((status & ST_BUSY_BIT) == 0))
					break;
				drv_usecwait(1);
			}

		    	if (retry <= 0) {
#ifdef SONY31A_ERRS
				cmn_err(CE_WARN, "SONY31A: testing !IS_DATARDY i=%d",i);
#endif
				if (sp->s_tries < sony31a_max_tries) {
				    	if (sp->s_tries == 3) {
						sony31a_reconfigure(ha);
						return (0);
					}
				    	if (sp->s_tries == 5) {
						sony31a_rereconfigure(ha);
						return (0);
					}
					sony31a_abort(ha);
					if ( sony31a_issue_rdcmd(ha, sp) >= 0)
						return (0);
			    	}
				if (ha->ha_lasterr == 0x30) /* focus error */
					cmn_err(CE_WARN, "sony31a: repeated focus errors. CD not of sufficient quality for this drive");
				else
					cmn_err(CE_WARN, "!lasterr= %x", ha->ha_lasterr);
				cmn_err(CE_WARN, "sony31a: cannot read block %x", (int)sp->s_blkno);
				sony31a_error(ha, sp);
				q->q_flag &= ~QBUSY;
				q->q_count--;
				sony_next(q);
				return (0) ;
			}
			CLEAR_DATARDY(iobase);

			retry = sony31a_dataretry;
        		while (--retry) {
				status = RD_STATUS(iobase);
				if ((status & ST_DATA_REQUEST_BIT) != 0)
					break;
				drv_usecwait(1);
			}
		    	if (retry <= 0) {
#ifdef SONY31A_ERRS
				cmn_err(CE_WARN, "SONY31A: testing !IS_DATAREQ i=%d",i);
#endif
				if (sp->s_tries < sony31a_max_tries) {
				    	if (sp->s_tries == 3) {
						sony31a_reconfigure(ha);
						return (0);
					}
				    	if (sp->s_tries == 5) {
						sony31a_rereconfigure(ha);
						return (0);
					}
					sony31a_abort(ha);
					if ( sony31a_issue_rdcmd(ha, sp) >= 0)
						return (0);
			    	}
				if (ha->ha_lasterr == 0x30) /* focus error */
					cmn_err(CE_WARN, "sony31a: repeated focus errors. CD not of sufficient quality for this drive");
				else
					cmn_err(CE_WARN, "!lasterr= %x", ha->ha_lasterr);
				cmn_err(CE_WARN, "sony31a: cannot read block %x", (int)sp->s_blkno);
				sony31a_error(ha, sp);
				q->q_flag &= ~QBUSY;
				q->q_count--;
				sony_next(q);
				return (0) ;
			}

			repinsb(iobase+2, addr, SONY_BLKSIZE);
		}

#ifdef SONY31A_DEBUG
		cmn_err(CE_NOTE, "SONY31A: finished reading the data cnt =%d", cnt);
#endif
		sony31a_get_result (ha, resbuf, &ressize);

		ha->ha_casestate = 0;
		ha->ha_cmd_sp = 0;
		sb->SCB.sc_comp_code = SDI_ASW;
		sdi_callback(sb);

		q->q_flag &= ~QBUSY;
		q->q_count--;
		sony_next(q);

		return (0);
}


int
sony31a_issue_rdcmd(scsi_ha_t *ha, sblk_t *sp)
{
	int	blkno;
	int	blkcnt;
	int	cmd, cmdsize;
	unchar cmdbuf[6];

	ASSERT(sp != NULL);

	blkno = sp->s_blkno;
	blkcnt = sp->s_size;

	/* blkcnt == 0 means just seek */
#ifdef SONY31A_ERRS
	if( ha->ha_lasterr == 0x11)
		cmn_err(CE_NOTE, "sony31a_issue_rdcmd: BLK tsno %d, cnt %d ", 
			blkno, blkcnt);
#endif
	sony31ablkcnt2buf(blkcnt, &cmdbuf[3]);
	sony31asector2msf(blkno, cmdbuf);

#ifdef SONY31A_DEBUG
	cmn_err(CE_NOTE, "sony31a_issue_rdcmd: BLK no %d, cnt %d ", 
			blkno, blkcnt);
#endif
	ha->ha_casestate = SONY31A_RDDATA;

	sp->s_tries++;
	cmd = ((blkcnt == 0)?SONY31A_SEEK:SONY31A_READ);
	(void) sony31a_sendcmd(ha, cmd, 0, cmdbuf, 6, SC_DATA_RDY_INT_EN_BIT);

	if ((ha->ha_dmachan <= 0) || (sony31a_dmaoff))
		return(0);
#if SONY_DMA
#if PDI_VERSION >= PDI_SVR42MP
        dma_disable(ha->ha_dmachan);

        ha->ha_cb->targbufs->address = sp->s_paddr;
        ha->ha_cb->targbufs->count = (ushort_t)sp->s_size*SONY_BLKSIZE;
        ha->ha_cb->command = DMA_CMD_READ;

        if (dma_prog(ha->ha_cb, ha->ha_dmachan, DMA_NOSLEEP) == FALSE) {
                cmn_err(CE_NOTE, "SONY31a: dma_prog() failed!");
                return (1);
        }
        dma_enable(ha->ha_dmachan);
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
		int 	   s;

                ushort_t size = (ushort_t)sp->s_size*SONY_BLKSIZE -1;


                b1_addr = (char)(padr & 0xFF);
                b2_addr = (char)((padr >> 8) & 0xFF);
                b3_addr = (char)((padr >> 16) & 0xFF);
                b1_len = (char)(size & 0xFF);
                b2_len = (char)((size >> 8) & 0xFF);

		s = splhi();

                outb( DMA_STR, 0x04|ha->ha_dmachan);

                outb(DMA1CBPFF, (char)0);
                outb(DMA1WMR, DMA_READ);
                outb(DMA1BCA1, b1_addr);
                outb(DMA1BCA1, b2_addr);
                outb(DMACH1PG, b3_addr);
                outb(DMA1BCWC1, b1_len);
                outb(DMA1BCWC1, b2_len);

		splx(s);

                outb( DMA_STR, ha->ha_dmachan);
        }
#endif
#endif /* SONY_DMA */

	return(0);
}


int
sony31a_reissue_rdcmd(scsi_ha_t *ha, sblk_t *sp)
{
	sony31a_abort(ha);
        sony31a_spin(ha, SONY31A_SPIN_UP_DISK, SONY31A_POLL);
	return (sony31a_issue_rdcmd(ha, sp));
}


void
sony31a_error(scsi_ha_t *ha, sblk_t *sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;

	ha->ha_casestate = 0;
	ha->ha_cmd_sp = 0;
	sb->SCB.sc_comp_code = (ulong_t) SDI_HAERR;
	
	cmn_err(CE_WARN, "!sony31a error");

	sdi_callback(sb);
}

#ifdef SONY31A_TIMEOUT
void
sony31a_timer(scsi_ha_t  *ha)
{
	clock_t now;
	ulong_t elapsed;
	sblk_t *sp = ha->ha_cmd_sp;
        ulong_t ressize;
	uchar_t	*resbuf = ha->ha_resbuf;


	ha->ha_tid = 0;
	drv_getparm(LBOLT, (void *) &now);
	elapsed = now - ha->ha_cmd_time;

	/* currently state machine reconfig, rdcap and read have an sp */
	if (sp != NULL) {
		struct sb      *sb = (struct sb *) & sp->sbp->sb;
		struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
		int             c, t, l;
		scsi_lu_t	*q;		/* queue ptr */
		int		sony31a_local_data_timeout;

		c = sony_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &SONY_LU_Q( c, t, l );


		if ( ha->ha_casestate != SONY31A_RDDATA ) {
			if(elapsed > sony31a_control_timeout) {
#ifdef SONY31A_ERRS
				cmn_err(CE_NOTE, "sony31a_timer:non data cmd timeout");
#endif
		     		sony31a_control(ha);
			} else {
				ha->ha_tid = ITIMEOUT(sony31a_timer, (void *) ha, sony31a_chktime, (pl_t) pldisk);
			}
			return;
		}
		if (sp->s_tries > 3)
			sony31a_local_data_timeout = sony31a_data_timeout*5;
		else
			sony31a_local_data_timeout = sony31a_data_timeout;

		if (elapsed > sony31a_local_data_timeout) {
			if (sp->s_tries <= sony31a_max_tries) {

				sony31a_get_result(ha, resbuf, &ressize);

        			if ( !( ( ressize < 2 ) || ( ( resbuf[0] & SONY31A_ERR_BIT ) == SONY31A_ERR_BIT ))) {
					sony31a_chkread(sp);
					return;
				}

                		/*
                 	 	 *      something went wrong
                 		 */
#ifdef SONY31A_ERRS
                		cmn_err(CE_WARN, "SONY31A: bad result in timer=%x", resbuf[1]);
#endif

				if(sp->s_tries == 3) {
#ifdef SONY31A_ERRS
					cmn_err(CE_NOTE, "sony31a_timer:reconfiguring device");
#endif
					sony31a_reconfigure(ha);
					return;
				}
				if((sp->s_tries == 5) || (sp->s_tries == 6)) {
#ifdef SONY31A_ERRS
					cmn_err(CE_NOTE, "sony31a_timer:rereconfiguring device");
#endif
					sony31a_rereconfigure(ha);
					return;
				}
				if(sp->s_tries == 7) {
					sony31a_issue_rdcmd(ha, sp);
#ifdef SONY31A_ERRS
                                        cmn_err(CE_NOTE, "sony31a_timer:command timeout, trying again3");
#endif
                                        return;
				}

				if((sp->s_tries == 2) || (sp->s_tries == 4)) {
					sony31a_reissue_rdcmd(ha, sp);
#ifdef SONY31A_ERRS
                                        cmn_err(CE_NOTE, "sony31a_timer:command timeout, trying again2");
#endif
                                        return;
				}
				if(sp->s_tries == 1) {
					sony31a_reissue_rdcmd(ha, sp);
#ifdef SONY31A_ERRS
                                        cmn_err(CE_NOTE, "sony31a_timer:command timeout, trying again1");
#endif
                                        return;
				}
			}

			if (ha->ha_lasterr == 0x30) /* focus error */
				cmn_err(CE_WARN, "sony31a: repeated focus errors. CD not of sufficient quality for this drive");
			else
				cmn_err(CE_WARN, "!sony31a:lasterr= %x", ha->ha_lasterr);
			cmn_err(CE_WARN, "!sony31a: abort command");
#ifdef SONY31A_SERIOUSERRS
/* so it doesn't scroll off the screen */
drv_usecwait(100000000);
#endif
			sony31a_error(ha, sp);
			ha->ha_status = SONY31A_ERROR;
			sony_flushq( q, SDI_HAERR);
			return;
		}
	} else if (elapsed > sony31a_max_idle_time) {
		/* 
		 * there is not that much activity. Stop the the timer.
		 * It will be enabled again the next command.
		 */
#ifdef SONY31A_DEBUG
		cmn_err(CE_NOTE, "sony31a_timer is canceled");
#endif
		return;
	}
	ha->ha_tid = ITIMEOUT(sony31a_timer, (void *) ha, sony31a_chktime, (pl_t) pldisk);
}
#endif



int
sony31a_abort(scsi_ha_t *ha)

{
        ulong_t                         ressize;
	uchar_t				*resbuf = ha->ha_resbuf;


	sony31a_sendcmd (ha, SONY31A_ABORT, 500, NULL, 0, 0);

        sony31a_get_result(ha, resbuf, &ressize);

        if ( ( ressize < 2 ) || ( ( resbuf[0] & SONY31A_ERR_BIT ) == SONY31A_ERR_BIT ) ) {
                /*
                 *      something went wrong
                 */
#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: Cannot abort.");
#endif
                return ( -1 );
        }

	return (0);
}

void
sony31a_reconfigure(scsi_ha_t *ha)
{

	sony31a_abort(ha);
	sony31a_reset(ha);

	sony31a_readtoc(ha, SONY535_FROMRECONF); /* get the state machine going */

}

/* last desperate attempt to get this request to work */

void
sony31a_rereconfigure(scsi_ha_t *ha)
{

        ulong_t                         ressize;
	uchar_t				*resbuf = ha->ha_resbuf;

	sony31a_reset(ha);
	sony31a_sendcmd(ha, SONY31A_GET_CFG, 500, NULL, 0, 0);
        sony31a_get_result(ha, resbuf, &ressize);

	sony31a_readtoc(ha, SONY535_FROMRECONF); /* get the state machine going */
}


/*******************************************************************************
 *
 *	sony31a_rdcap ( sblk_t *sp )
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
sony31a_rdcap(sblk_t * sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c;
	scsi_ha_t 	*ha;

	c = sony_gtol[SDI_HAN(sa)];
	ha = &sony_sc_ha[c];

	ha->ha_cmd_sp = sp;

#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: sony31a_rdcap");
#endif

	if (ha->ha_driveStatus&SONY31A_NODISK) {
		sony31a_error(ha, sp);
	} else {
		sony31a_readtoc(ha, SONY31A_FROMRDCAP) ; /* gets the state machine going with a spinup*/
	}
}

/*******************************************************************************
 *
 *	sony31a_spin ( struct scsi_ha *ha, int up, int intr )
 *
 *	Spin Up/Down a disk
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
sony31a_spin ( struct scsi_ha *ha, int cmd, int intr)
{
	uchar_t				*resbuf = ha->ha_resbuf;
        ulong_t                         ressize;

	/* SONY31A_SPIN_UP_DISK or SONY31A_SPIN_DOWN_DISK  */
	if (intr) {	
        	sony31a_sendcmd(ha, cmd, 0, NULL, 0, SC_RES_RDY_INT_EN_BIT);
		return (0);
	}

	/* get_result will wait upto a max. */
        sony31a_sendcmd(ha, cmd, 500, NULL, 0, 0);

	sony31a_get_result(ha, resbuf, &ressize);

	if ( ( ressize < 2 ) || ( ( resbuf[0] & SONY31A_ERR_BIT ) == SONY31A_ERR_BIT ) ) {
                /*
                 *      something went wrong
                 */
#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: Cannot spin up/down");
#endif
                sony31a_reset (ha);         /* reset drive */
                return ( -1 );
        }
	/* might want to check the ha_status ?? */

	return  ( 0 );
}

/******************************************************************************* *
 *      sony31a_handle_attention ( struct scsi_ha *ha )
 *
 *      Handle the drive attention
 *
 *      Entry :
 *              Nothing
 *
 *      Exit :
 *              0                       none found
 *              1                       found
 *
 *      Notes :
 *              - if one is found then this probably should be called again
 *                until there are no more attention requests
 *
 ******************************************************************************/



int
sony31a_handle_attention ( scsi_ha_t *ha )
{
        uchar_t                         atten_code;
	int				iobase = ha->ha_iobase;

        if ( IS_ATTENTION(iobase) ) {
                /*
                 *      drive has something to say
                 */
                CLEAR_ATTENTION(iobase);
                atten_code = RD_RESULT(iobase);
                switch ( atten_code ) {
                        case 0x28 : /* Eject Complete */
                        case 0x81 : /* Eject Button Pushed */
                        case 0x2c : /* Emergency Eject */
                          	/*
                                 *      someone changed the CD
                                 */
#ifdef SONY31A_DEBUG
        		cmn_err(CE_WARN, "SONY31A: someone changed the CD ");
#endif
                                ha->ha_driveStatus |= SONY31A_CHANGE;
                                break;

                        case    0x70 :
        		cmn_err(CE_WARN, "!SONY31A: Hardware Error. Ignore, probably a transient");
                                break;

                        case    0x00 :
                        case    0x24 : /* Spin up complete */
                        case    0x27 : /* Spin down complete */
                        case    0x62 : /* Toc read complete */
				return 0;

                        default :
                                /*
                                 *      probably some random audio thing
                                 */
#ifdef SONY31A_DEBUG
        		cmn_err(CE_WARN, "SONY31A: Unknown Attention = %x", atten_code);
#endif
                                break;
                }
                return ( atten_code );
        }
        /*
         *      nothing worth reporting
         */
	return (0);
}


/*******************************************************************************
 *
 *	sony31a_stat ( struct scsi_ha *ha )
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
sony31a_stat ( struct scsi_ha *ha )

{
	ulong_t				ressize;
	uchar_t				*resbuf = ha->ha_resbuf;

	sony31a_sendcmd (ha, SONY31A_GET_MECH_STAT, 250, NULL, 0, 0);

	sony31a_get_result(ha, resbuf, &ressize);

	if ( ( ressize < 2 ) || ( ( resbuf[0] & SONY31A_ERR_BIT ) == SONY31A_ERR_BIT ) ) {
		/*
		 *	something went wrong
		 */
#ifdef SONY31A_DEBUG
                cmn_err(CE_NOTE, "SONY31A: Cannot get mech status. drive");
#endif
		sony31a_reset (ha);
		return ( 0 );
	}

#ifdef SONY31A_DEBUG
        cmn_err(CE_NOTE, "SONY31A: stat - resuf 2 = %x", resbuf[2]);
#endif

	sony31a_updateStatus ( ha, resbuf [ 2 ] );

	if (resbuf[2] & MS0_DISK_LOADED) {
		/*
		 *	if disk is loaded then clear the disk change flag
		 */
		ha->ha_driveStatus &= ~(SONY31A_CHANGE|SONY31A_NODISK);
	} 

	return ( ha->ha_driveStatus );
}


/*******************************************************************************
 *
 *	sony31a_updateStatus ( struct scsi_ha *ha, uchar_t res )
 *
 *	Update the driveStatus variable with whatever the current status of 
 *	the drive is -- assumes that mech. status of drive was read in and
 *	is sitting in result[2].
 *
 *	Entry :
 *		result[2}
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void 
sony31a_updateStatus ( struct scsi_ha *ha, uchar_t res )

{
#ifdef SONY31A_ERRS
                cmn_err(CE_NOTE, "SONY31A: updateStatus. res=%x", res);
#endif
	
	/*
	 *	if disk loaded bit == 0 there could still be a disk in the drive
 	 *	but it hasn't been spun up and detected so we don't know anything
	 */

	if (res & MS0_DISK_LOADED) {
		ha->ha_driveStatus |= SONY31A_DISK;
		ha->ha_driveStatus &= ~SONY31A_NODISK;
	} else {
		ha->ha_driveStatus |= SONY31A_NODISK;
		ha->ha_driveStatus &= ~SONY31A_DISK;
	}
	

	if (res & MS0_ROTATING)
		ha->ha_driveStatus |= SONY31A_SPINNING;
	else
		ha->ha_driveStatus &= ~SONY31A_SPINNING;
}


/* get the state machine going */
void
sony31a_readtoc ( struct scsi_ha *ha, int fromrdcap)
{
	if (fromrdcap) {
		ha->ha_casestate = SONY31A_RDCAPTOCSPINUP;
	} else {
		ha->ha_casestate = SONY31A_RECONFTOCSPINUP;
	}
        sony31a_spin(ha, SONY31A_SPIN_UP_DISK, SONY31A_WANTINTR);

}

#if PDI_VERSION >= PDI_UNIXWARE20
/*
 * int
 * sony31averify(int iobase)
 *
 * Calling/Exit State:
 *	None
 *
 * Description:
 *	Verify the board instance.
 */
int
sony31averify(int iobase)
{

	/* not always true that 0xFF at address without a board.  Don't rely */
	if ((inb(iobase) == 0xff) && (inb((iobase)+2) == 0xff) && (inb((iobase)+3) == 0xff)) {
		/*
		 * Nothing at these addresses
		 */
		 return(-1);
	}

	return(0);
}
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */
