#ident	"@(#)kern-i386at:io/pccard/pcic/pcic.c	1.4"

/* 	pcic.c
 * 
 *  	This file contains the software driver for the I82365SL 
 *	PC Card Interface Controller (PCIC).   This file also contains  
 *	stipped down versions of socket services, card services, and 
 *	a modem enabler.
 *      NOTE:   the code inside the #ifdef CS_EVENT blocks is semi
 * 	functional.  It may be used as a foundation for Event functionality
 *	in Card Services.
 */


#ifdef _KERNEL_HEADERS
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <mem/immu.h>
#include <util/cmn_err.h>
#include <mem/kmem.h>
#include <util/mod/moddefs.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <util/debug.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>
#else
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/moddefs.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/debug.h>
#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#endif

#include "pcic.h"
#include "pccardss.h"
#include "pccardcs.h"

#define DRVNAME "pcic - Intel PC Card Interface Controller"
#define DRVPREFIX "pcic"

#ifdef PCICDEBUG
#ifdef STATIC
#undef STATIC
#define STATIC
#endif 
#endif  /* PCICDEBUG */

#define MAXPCIC  2      /* We'll allow at most two pcic's at once */


/*  Globals specific to the pcic driver */
int 		pcicdevflag = 0;	
STATIC int 	pcic_numcntls = 0;		/* Number of pcic controllers found */
STATIC int 	pcic_sleepflag = KM_NOSLEEP;
STATIC pcic_t 	pciccntls[MAXPCIC];

/* Function declarations for the pcic driver */
int             pcic_load ( void );
int             pcic_unload ( void );
int		pcicinit( void );
STATIC uchar_t 	pcic_read( ulong_t, ulong_t );
STATIC void	pcic_write( ulong_t, uchar_t, uchar_t );

STATIC int 	pcic_get_status( int adapter, int socket, ss_socketstatus_t *s );
STATIC int 	pcic_setio( int adapter, int socket, ss_iowin_t *io );
STATIC int 	pcic_setmem( int adapter, int socket, int win, ss_memwin_t *mem);
STATIC int 	pcic_setirq( int adapter, int socket, ss_irq_t *irq );
STATIC int 	pcic_getstate( int adapter, int socket, ss_socket_state_t *s );
STATIC int 	pcic_setstate( int adapter, int socket, ss_socket_state_t *s );
STATIC int 	modem_enabler( void );

void		pcicintr( unsigned int );
#ifdef CSEVENT
STATIC void	pcic_setup_inthandler( pcic_t *, socket_t *);
STATIC int 	pcic_set_callback(int adapter, int socket, ss_event_t event, int (*cb)(int flags));
#endif

int pccardssinit( void );
int pccardcsinit( void );
STATIC int pcicinit2( void );

int pccardss_adapter_register( char *name, int boardnum, int numsockets, struct socket_services *ss, int nummem, int numio);

struct socket_services pcic_socket_services = {
	pcic_get_status,
	pcic_setio,
	pcic_setmem,
	pcic_setirq,
	pcic_getstate,
	pcic_setstate
#ifdef CSEVENT
	,
	pcic_set_callback
#endif  /* CS_EVENT */
} ;

/* 
 *    This driver is base on the following datasheets:	
 * 		Intel 82365SL PCIC January 1993.
 * 		Cirrus Logic CL-PD6729, June 1994	
 * 		Cirrus Logic CL-PD6710/PD672X, July 1994	
 * 		Vadem VG-469, July 1994
 * 		Vadem VG-365, January 1994
 * 		VLSI VL82C146A Functional Spec, November 21, 1994
 *
 */ 	

MOD_DRV_WRAPPER ( pcic, pcic_load, pcic_unload, NULL, DRVNAME );

/* NOTE:  This is a per driver lock.    Multiple adapter will share this lock 
 * the lock will primarily be used around the pcic_read/pcic_write
 * inb/outb settings.  (The lock should probably be managed 
 * on a per-adapter basis.)
 */
LKINFO_DECL(pcic_register_lkinfo, "IO:PCICSocketAdapterLock", 0);
STATIC lock_t *pcic_lock;

/* 
 *  int pcic_load( ) 
 *	-  performs any module-specific setup  and initialization needed 
 * 	   to dynamically load this module into a running system.
 * 
 *  In:  Nothing
 *  Out: ENODEV - if there is no device present
 *       0      - if successful   
 */
int 
pcic_load( void ) 
{
	/*  kmem_alloc, and physmap may need to sleep.  Drivers can sleep when
	 *  they are dynamically loaded, but cannot when they are static in the
	 *  kernel.  NOTE that pcic_sleepflag is initialized as KM_NOSLEEP above	
   	 *  we'll pass pcic_sleepflag into all calls to kmem_alloc, and physmap
	 */
	pcic_sleepflag = KM_SLEEP;
	if ( pcicinit() )
		return( ENODEV );
	return( 0 );
}


/*
 *  int pcic_unload( )
 *      -  performs any module-specific cleanup to dynamically unload 
 *         this module into a running system.
 * 
 *  In:  Nothing
 *  Out: 0  (for now!)
 */
int 
pcic_unload( void ) 
{	
	int i;
	
	for (i = 0; i < MAXPCIC; i++ ) {
		if ( pciccntls[i].pcic_numsockets ) {
			kmem_free( pciccntls[i].pcic_socketp, pciccntls[i].pcic_numsockets * sizeof(socket_t) );
		}
#ifdef CS_EVENT
		if ( pciccntls[i].pcic_irq )
			cm_intr_detach( pciccntls[i].pcic_intr_cookiep );
#endif
	}

	LOCK_DEALLOC( pcic_lock );
	/* Since we dont have an open() function, we done have any customers.
	 * This driver can unload anytime.
	 */
	return( 0 );
}


/* 
 *	pcic_socket_reset ( iobase, regbase )
 *		- reset a socket  called only from pcic_socket_init().
 *      Locks held: none
 *	In    :
 *		int iobase:   base address of pcic socket
 *		int regbase:  register base socket
 *	Out   :
 *		Nothing
 */
STATIC void
pcic_socket_reset ( int iobase, int regbase ) 
{
	int val;

	val = pcic_read ( iobase, PCIC_INTR+regbase );
	val &= ~PCIC_IRQ_NOTRESET;
	pcic_write ( iobase, PCIC_INTR+regbase, val );
	drv_usecwait ( 20000 );  /* Spec says to wait 20ms for card to settle */

	val |= PCIC_IRQ_NOTRESET;
	pcic_write ( iobase, PCIC_INTR+regbase, val );
	drv_usecwait ( 300000 ); /* Spec says to wait 300ms for card to settle */
	return;
}


/*
 *	pcic_socket_init - Turn the (CS) power on to the socket, and reset it.
 *			   Called only from pcicinit2();
 *      Locks held: none
 *
 *	In   : ioase   - the base address of the PCIC
 *	       regbase - the base address of the socket on the PCIC
 *	Out  :
 *	       Nothing
 */
STATIC void
pcic_socket_init ( int iobase, int regbase )
{
	uchar_t			val;

	val = pcic_read( iobase, regbase + PCIC_STATUS);
	/* If there not already power to the card  turn it on */
	if (!(val & PCIC_STATUS_PWR) )  {
		val = PCIC_PWR_DIS_RESETDRV;
		pcic_write ( iobase, PCIC_POWER+regbase, val ); 	/* power off */
		drv_usecwait ( 300000 ); 				/* Spec says at least 300ms for card to settle */
		val |= PCIC_PWR_AUTO | PCIC_PWR_ENABLE;
		pcic_write ( iobase, PCIC_POWER+regbase, val );	
		drv_usecwait ( 50000 ); 
		val |= PCIC_PWR_OENABLE;
		pcic_write ( iobase, PCIC_POWER+regbase, val );		/* Enable the output to the socket */
		drv_usecwait ( 300000 ); 				/* Spec says at least 300ms */
		pcic_socket_reset(iobase, regbase);
	}
}


/* 
 *  int pcicinit( ) 
 *	-  This function is responsible for init'ing the pcic functionality,
 * 	   socket services, and card services.  
 * 
 *  In:  Nothing
 *  Out: ENODEV - if there are no devices present
 * 	 ENINVAL- if too many sockets were registered 
 *       0      - if successful   
 */
int
pcicinit( void ) 
{
	int ret;

	
	if ( ret = pccardssinit() )
		return( ret );
	if ( ret = pcicinit2() )
		return( ret );
	if ( ret = pccardcsinit() )
		return( ret );

	ret = modem_enabler();

	return( 0 );
}


/* The next functions check for the difference hybrids of the pcic socke
 * controller, namely the cirrus, vadem, or vlsi implementations.
 * No locks are held on these calls.
 * As a result of these calls, the bd->pcic_model and bd->pcic_numsockets
 * values are set.
 */
STATIC void
checkcirrus( pcic_t *bd ) 
{
	uchar_t first, second;	

	first = pcic_read( bd->pcic_iobase, PCIC_TYPEPD67XX_IDREGISTER );
	second = pcic_read( bd->pcic_iobase, PCIC_TYPEPD67XX_IDREGISTER );

	/* The only difference should be the ID bits ... on first read, the 
	 * ID bits will be 11b, and on second read, the id bits will be 00b
	 */
	if ( (first & PCIC_TYPEPD67XX_IDMASK) && !(second & PCIC_TYPEPD67XX_IDMASK) ) {
		bd->pcic_model = CLPD67XX;
		bd->pcic_numsockets = (first & PCIC_TYPEPD67XX_SLOTMASK) ? 2 : 1;
	}
}

STATIC void
checkvlsi( pcic_t *bd ) 
{
	int id1;
	id1 = pcic_read( bd->pcic_iobase, 0);
	if (id1 == 0x84) {
		bd->pcic_numsockets = 1;
		bd->pcic_model = VLSIVL82C;
		id1 = pcic_read(bd->pcic_iobase + 4, 0x80);
		if (id1 != 0xff)
			bd->pcic_numsockets++;
		id1 = pcic_read(bd->pcic_iobase + 2, 0x0);
		if (id1 != 0xff)
			bd->pcic_numsockets++;
		id1 = pcic_read(bd->pcic_iobase + 6, 0x80);
		if (id1 != 0xff)
			bd->pcic_numsockets++;
	}
}

STATIC void
checkvadem( pcic_t *bd ) 
{
	int val, id1, id2;

	id1 = pcic_read( bd->pcic_iobase, 0);
	val = pcic_read( bd->pcic_iobase, PCIC_TYPEVADEM_DMAREG );
	val |= PCIC_TYPEVADEM_UNLOCK;
	pcic_write( bd->pcic_iobase, PCIC_TYPEVADEM_DMAREG, val );

	pcic_write( bd->pcic_iobase, 0, id1 | 0x08 );
	id2 = pcic_read( bd->pcic_iobase, 0);
	if (id2 == (id1 | 0x08) ) {
		val = pcic_read( bd->pcic_iobase, 0x80);
		bd->pcic_numsockets = (val == id1 ) ? 4 : 2;
		bd->pcic_model = VADEMVG;
	}
	val &= ~PCIC_TYPEVADEM_UNLOCK;
	pcic_write( bd->pcic_iobase, PCIC_TYPEVADEM_DMAREG, val );
}

/* 
 *  STATIC int pcicinit2( ) 
 *	-  This function is responsible for init'ing the pcic functionality,
 * 	   socket services, and card services.  
 * 
 *  In:  Nothing
 *  Out: ENODEV - if there are no devices present
 *       0      - if successful   
 */
STATIC int
pcicinit2( void ) 
{
	cm_args_t cm_args;
	cm_range_t range;
	int i, numboards = 0;
	pcic_t *bd;
	int pcic_found = 0; 
	uchar_t j;


	/* Get the number of pcic boards in the resmgr ... */
	if ( ( pcic_numcntls = numboards = cm_getnbrd( DRVPREFIX ) ) == 0 ) {
		cmn_err(CE_CONT, "!%s:No boards found in resmgr", DRVPREFIX);
		return( ENOENT );
	}

	/* If the nuber of boards is greater than MAXPCIC, issue a warning, and 
   	 * only configure the first MAXPCIC of them */
	if (numboards > MAXPCIC) {
		cmn_err(CE_WARN, "!%s:Only %s boards can be configured", DRVPREFIX, MAXPCIC);
		numboards = MAXPCIC;
	}
	
	if (!(pcic_lock = LOCK_ALLOC( PCICHEIR, plbase, &pcic_register_lkinfo, pcic_sleepflag))) {
		cmn_err(CE_WARN, "%s:no memory for adapter lock", DRVPREFIX);
		return( ENOMEM );
	}

	/* Walk through the resmgr's pcic entries, and initialize the adapter and socket structures */
	for (i = 0; i < numboards; i++ ) {
		int ret;
		int regbase = 0;

		bd = &pciccntls[i];

		cm_args.cm_key = cm_getbrdkey(DRVPREFIX, i);
		cm_args.cm_n = 0;

#ifdef CS_EVENT
		/* Get the IRQ */
		cm_args.cm_param = CM_IRQ;
		cm_args.cm_val = &(bd->pcic_irq);
		cm_args.cm_vallen = sizeof(cm_num_t);
		ret = cm_getval(&cm_args);
		if ( (ret) || ( (bd->pcic_irq) > 255 ) || ( (bd->pcic_irq) < 0 )) 
			bd->pcic_irq = 0;
#endif

		/* Get the I/O Address */
		cm_args.cm_param = CM_IOADDR;
                cm_args.cm_val = &range;
                cm_args.cm_vallen = sizeof(struct cm_addr_rng);
                ret = cm_getval(&cm_args);
                if (ret) {
			cmn_err(CE_WARN, "!%s:board %d, cm_getval:CM_IOADDR failed", DRVPREFIX, i);
			continue;
		}
		if((range.endaddr - range.startaddr) != 7 )
			cmn_err(CE_WARN, "!%s:board %d, possibly bogus I/O range", DRVPREFIX, i);

		bd->pcic_iobase = range.startaddr;


		/* Get the Memory Address */
		cm_args.cm_param = CM_MEMADDR;
                cm_args.cm_val = &range;
                cm_args.cm_vallen = sizeof(struct cm_addr_rng);
                ret = cm_getval(&cm_args);
                if (ret != 0){
			cmn_err(CE_WARN, "!%s board %d, cm_getval:CM_MEMADDR failed", DRVPREFIX, i);
			continue;  /*  for-i */
		} else  {
			/* Make sure we have at least PCIC_MIN_CMMEMORYRANGE (8K - 1) for mapping in common/attrib memory */
			if( (range.startaddr == 0) || (range.endaddr - range.startaddr) < PCIC_MIN_CMMEMORYRANGE ) {
				cmn_err(CE_WARN, "!%s:board %d, possibly bogus I/O range", DRVPREFIX, i);
				continue; /* for-i */
			} else {
				bd->pcic_memstart = range.startaddr;
				bd->pcic_memsize = (range.endaddr - range.startaddr) + 1;
			}
		}


		bd->pcic_revlevel = pcic_read( bd->pcic_iobase, PCIC_IDREV );
		pcic_socket_init(bd->pcic_iobase, 0);  /* Reset socket 0 */

		/* If the pcic_revlevel is not one of the expected values, skip it */
		switch( bd->pcic_revlevel ) {
			case PCIC_REVA:
			case PCIC_REVB:
			case PCIC_REVC:
			case PCIC_REVD:
			case PCIC_REVE:
		   	   	cmn_err(CE_NOTE, "!%s:Socket Adapter found at addr %x", DRVPREFIX, bd->pcic_iobase);
				break;
			default:
				cmn_err(CE_NOTE, "!%s:Socket Adapter not found at addr %x", DRVPREFIX, bd->pcic_iobase);
				continue; /* for-i */
		}

		/*
		 *  The check* functions look for a specific typeof PCIC.
		 *  While this type is not taken full advantage of yet,
		 *  it will be available for future use if we ever want to
		 *  take advantage of particular chip features.  In any case,
		 *  the checkcirrus will atleast figure out how many sockets we have,
		 *  checkvlsi will inform us that we have that stange register indexing
		 *  scheme.   checkvadem is thrown in for good measure since I have
		 *  datasheets for that adapter, and I can identify it...
		 *  By the by, these calls will also populate bd->pcic_numsockets
		 */
		checkcirrus(bd);
		if (bd->pcic_model == MODELNA)
			checkvadem(bd);
		if (bd->pcic_model == MODELNA)
			checkvlsi(bd);
		if (bd->pcic_model == MODELNA) {
			bd->pcic_numsockets = 2;  /* Default */
			if ( pcic_read( bd->pcic_iobase, 0x80 + PCIC_IDREV ) != 0xff )
				bd->pcic_numsockets += 2;
		}

		/* Allocate space for the socket structures */
		if ( (bd->pcic_socketp = (socket_t *) kmem_zalloc( bd->pcic_numsockets * sizeof(socket_t), 
				pcic_sleepflag) ) ==  (socket_t *) NULL ) {
			cmn_err(CE_NOTE, "%s: kmem_zalloc failed %d", DRVPREFIX, bd->pcic_numsockets * sizeof(socket_t));
			continue; /* for-i */
		}

		/* Since we have come this far, register the adapter with socket services */
		if ( pccardss_adapter_register( DRVPREFIX, i, bd->pcic_numsockets, &pcic_socket_services, PCIC_NUMMEMWIN, PCIC_NUMIOWIN)) {
			cmn_err(CE_NOTE, "%s:adapter_register failed", DRVPREFIX);
			continue; /* for-i */
		}

#ifdef CS_EVENT
		/* Attach interrupt once for each board */ 
		if ( bd->pcic_irq ) {
			if( !cm_intr_attach(cm_args.cm_key, pcicintr, &pcicdevflag, &bd->pcic_intr_cookiep)) {
				cmn_err(CE_NOTE, "%s board(%d) intr_attach failed", DRVPREFIX, i);
				bd->pcic_irq = 0;
			}
		}
#endif  /* CS_EVENT */

		/* The VLSI socket controller IC needs to have it's registers base, and offsets setup
		 * differently than the others ... 
		 */
		if (bd->pcic_model == VLSIVL82C) {
			for(j = 0 ; j < bd->pcic_numsockets; j++ ) {
				switch(j + 1) {
					case 1:
						bd->pcic_socketp[j].iobase = bd->pcic_iobase;
						bd->pcic_socketp[j].regbase = 0;
						break;
					case 2:
						bd->pcic_socketp[j].iobase = bd->pcic_iobase + 4;
						bd->pcic_socketp[j].regbase = 0x80;
						break;
					case 3:
						bd->pcic_socketp[j].iobase = bd->pcic_iobase + 2;
						bd->pcic_socketp[j].regbase = 0;
						break;
					case 4:
						bd->pcic_socketp[j].iobase = bd->pcic_iobase + 6;
						bd->pcic_socketp[j].regbase = 0x80;
						break;
				}
			}
		} else {
			for(j = 0 ; j < bd->pcic_numsockets; j++ ) {
				bd->pcic_socketp[j].iobase = bd->pcic_iobase;
				bd->pcic_socketp[j].regbase = regbase;
				if (j)
					pcic_socket_init(bd->pcic_iobase, regbase);
				regbase += 0x40;
#ifdef CSEVENT
				/* Tell pcic to interrupt for events on each socket */
				if ( bd->pcic_irq )  {
					pcic_setup_inthandler(bd, &bd->pcic_socketp[j] );
				}
#endif
			}
		} 
		pcic_found = 1;
	}

	/* If no devices are found, return ENODEV , otherwise, return success */
	if ( !pcic_found ) {
		LOCK_DEALLOC( pcic_lock );
		return (ENODEV);
	} else 
		return( 0 );

}

#ifndef CS_EVENT
/* We need this placeholder for the idtools ... they seem to always want
 * an interrupt function to put into the conf tables when compiled static.
 */
void
pcicintr( unsigned int ivect ) {
	return;
}
#else
/* 
 * pcic_intr - Interrup handler function for the PCIC.
 *             If a CardServices client asks to be notified upon a certain
 *             Event, an interrupts will come in there, and be passed back to the 
 *             client's handler.   NOTE:  This intr function does not have
 * 	       any locking code in it.
 *     
 *      In:    Interrupt number
 *     Out:    Nothing	
 */
void
pcicintr( unsigned int ivect ) {

	int i, j;
	socket_t *sock;

	for(i = 0 ; i < MAXPCIC; i++ ) {
		/*
		 *  If there are no sockets, on this board, dont bother continuing 
		 *  looking at this board.
		 */
		if ( ! pciccntls[i].pcic_numsockets ) 
			continue;

		for(j = 0; j < pciccntls[i].pcic_numsockets; j++) {
			int intrcode;
			sock =  &pciccntls[i].pcic_socketp[j];
			/* The pcic_read clears the interrupt */
			intrcode = pcic_read( sock->iobase, sock->regbase + PCIC_STSCHNG);
			if (!intrcode)
				continue;
		}
	}
}
STATIC void
pcic_setup_inthandler( pcic_t *bd, socket_t *sock)
{
	int irq = bd->pcic_irq;
	int base = sock->iobase;
	int reg = sock->regbase;
	int val;

	int flags = PCIC_DETECTCHG; /* For starters, catch insertion/removal events */

	/* turn off CSC ack writeback */
	val = pcic_read( base, reg + PCIC_GCR);
	pcic_write(base, reg + PCIC_GCR,  (val  & ~PCIC_GCR_EX_WR_CSC) );
	pcic_write(base, reg + PCIC_STSCHNG_INTR, ( (irq & 0x0f) << 4) | (flags & 0x0f) );

}
#endif  /* CS_EVENT */



/*
 *      pcic_write ( ulong_t base, uchar_t reg, uchar_t val )
 *
 *      Write a register in the PCIC
 *      Entry :
 *              base            base of the PCIC chip
 *              reg             register to write
 *              val             value to write
 *      Exit :  Nothing
 */
STATIC void
pcic_write ( ulong_t base, uchar_t reg, uchar_t val )
{
	pl_t opl;

	opl = LOCK(pcic_lock, plbase);	
        outb ( base, reg );
        outb ( base + PCIC_DATA, val );
	UNLOCK(pcic_lock, opl);
}



/*
 *      pcic_read ( ulong_t base, uchar_t reg )
 *          Read a register from the PCIC
 *
 *      Entry :
 *              base            base of the PCIC chip
 *              reg             register to read
 *      Exit  : 8 bit value of the register
 */
STATIC uchar_t
pcic_read ( ulong_t base, ulong_t reg )
{
	uchar_t val;
	pl_t opl;

	opl = LOCK(pcic_lock, plbase);	
        outb ( base, reg );
	val = inb ( base + PCIC_DATA );
	UNLOCK(pcic_lock, opl);	

        return ( val );

}



/*
 *  pcic_get_status - Get Card/Socket status
 *                    This release only supports card detection.
 *
 *  In:       adapter  - adapter number to look at
 *            socket   - socket on adapter
 *            s        - Pointer to ss_socketstatus_t to hold returned status
 *
 *  Out:      0
 *
 */
STATIC int
pcic_get_status( int adapter, int socket, ss_socketstatus_t *s ) 
{
	pcic_t *bd = &pciccntls[adapter];
	socket_t  *socketp = &(bd->pcic_socketp[socket]);
	uchar_t val;
	pl_t opl;

	val = pcic_read( socketp->iobase, socketp->regbase + PCIC_STATUS);

	/* If CD1 and CD2 are set, a card is fully seated */
	s->CardDetect = (val & PCIC_STATUS_CD1) && (val & PCIC_STATUS_CD2);

#ifdef FUTURECARDSERVICES
	s->BatteryDead = !(status & PCIC_STATUS_BVD1);
	s->BatteryWarning = (status & PCIC_STATUS_BVD1) &&  (! (status & PCIC_STATUS_BVD2));
	s->WriteProtect = (status & PCIC_STATUS_WP);
	s->Ready = (status & PCIC_STATUS_READY);
#endif /* FUTURECARDSERVICES */

	return ( 0 );
}



/* 
 *  pcic_getstate   - Get Socket state
 *                    This release only supports power, and  card type info
 *
 *  In:       adapter  - adapter number to look at
 *            socket   - socket on adapter
 *            s        - Pointer to ss_socket_state_t to hold returned status
 *
 *  Out:      0
 */
STATIC int 
pcic_getstate( int adapter, int socket, ss_socket_state_t *s ) {

	pcic_t *bd = &pciccntls[adapter];
	socket_t  *socketp = &(bd->pcic_socketp[socket]);
	int val;

	/* Is this a memory/io card or just a memory card? */
	val = pcic_read (socketp->iobase, socketp->regbase + PCIC_INTR);
	if (val & PCIC_IRQ_TYPE_IO)
		s->flags |= SOCKET_MEMIO;
	else
		s->flags &= ~SOCKET_MEMIO;

	val = pcic_read (socketp->iobase, socketp->regbase + PCIC_POWER);
	s->pwrinfo = val  & PCIC_PWR_ENABLE ?  POWERENA : 0;   /* Enable power to the card */
	s->pwrinfo |= val & PCIC_PWR_AUTO    ? AUTOPOWER : 0;  /* Do auto power switching */
	s->pwrinfo |= val & PCIC_PWR_OENABLE ? OUTPUTENA : 0;  /* Enable output of power to the card */

	return 0;
}



/*
 *  pcic_setstate   - Set Socket state
 *                    This release only supports power information
 *
 *  In:       adapter  - adapter number to look at
 *            socket   - socket on adapter
 *            s        - Pointer to ss_socket_state_t holding values to be 
 *			 set to the socket 
 *  Out:      0
 */
STATIC int 
pcic_setstate( int adapter, int socket, ss_socket_state_t *s ) {

	pcic_t *bd = &pciccntls[adapter];
	socket_t  *socketp = &(bd->pcic_socketp[socket]);
	int oval, val;

	/* Set this up to be a PCIC_IRQ_TYPE_IO card if SOCKET_MEMIO is set */
	val = pcic_read (socketp->iobase, socketp->regbase + PCIC_INTR);
	val &= ~PCIC_IRQ_TYPE_IO;
	val |= s->flags & SOCKET_MEMIO ? PCIC_IRQ_TYPE_IO : 0;
	pcic_write( socketp->iobase, socketp->regbase + PCIC_INTR, val);

	/* Read the current settings and modify them ... */
	oval = val = pcic_read (socketp->iobase, socketp->regbase + PCIC_POWER);
	(s->pwrinfo & POWERENA)  ?  (val |= PCIC_PWR_ENABLE)  : (val &= ~PCIC_PWR_ENABLE);
	(s->pwrinfo & AUTOPOWER) ?  (val |= PCIC_PWR_AUTO)    : (val &= ~PCIC_PWR_AUTO);


	/* ...but only if "val" changed, write it, otherwise dont bother...this is done 
	 * for performance...the drv_usecwait calls are expensive when parsing tuples.
	 */
	if (oval != val) {
		pcic_write( socketp->iobase, socketp->regbase + PCIC_POWER, val);
		drv_usecwait(10000);
	}

	(s->pwrinfo & OUTPUTENA) ?  (val |= PCIC_PWR_OENABLE) : (val &= ~PCIC_PWR_OENABLE);
	if (oval != val) {
		pcic_write( socketp->iobase, socketp->regbase + PCIC_POWER, val);
		drv_usecwait(30000);
	}

	return( 0 );
}



/*
 *
 *	real_pcic_setio ( ulong_t base, ulong_t ioreg, ulong_t start, ulong_t end,
 *		     ulong_t info, uchar_t rev )
 *
 *	Set a memory mapping
 *
 *	Entry :
 *		base		base of PCIC chip
 *		ioreg		io register base (with socket offset!)
 *		start		io start loc
 *		end		io end loc 
 *		info		io info 
 *		rev		pcic rev level
 *
 *	Exit :
 *		Nothing
 *
 */
STATIC void
real_pcic_setio( ulong_t base, ulong_t ioreg, ulong_t start, ulong_t end, ulong_t info, uchar_t rev )

{
	uchar_t			val;
	ulong_t			ioctrl;		/* ioctrl register */

	/*
	 *	card io start loc
	 */
	pcic_write ( base, ioreg+PCIC_IOSTARTLB, start & 0x00ff );
	pcic_write ( base, ioreg+PCIC_IOSTARTHB, (start & 0xff00) >> 8 );

	/*
	 *	card io end loc
	 */
	pcic_write ( base, ioreg+PCIC_IOSTOPLB, end & 0x00ff );
	pcic_write ( base, ioreg+PCIC_IOSTOPHB, (end & 0xff00) >> 8 );

	/*
	 *	card io control register
	 */
	ioctrl = (ioreg & 0xc0)+PCIC_IOCTRL;		/* ioctrl reg */

	val = pcic_read ( base, ioctrl );
	info = info & 0x0f;
	info |= PCIC_IO_WAIT1;	    /* 16 bit has waitstates */
	if ( info & PCIC_IO_SIZE16 ) {
		if ( ( rev == PCIC_REVC ) || ( rev == PCIC_REVD ) ) {
			/*
		 	 *	kludge for the IBM versions
		 	 */
			info &= ~PCIC_IO_IOCS16;
		}
		else {
			/*
		 	 *	just a "normal" pcic
			 */
			info |= PCIC_IO_IOCS16;
		}
	}
	if ( (ioreg & 0x3f) == PCIC_IO0 ) {
		/*
		 *	io port 0 
		 */
		val = ( val & 0xf0 ) | info;
	}
	else {
		/*
		 *	io port 1 
		 */
		val = ( val & 0x0f ) | ( info << 4 );
	}

	pcic_write ( base, ioctrl, val );

}



/*
 *  pcic_setio      - Set Socket IO windows.   This function just sets up the
 *		      call to the above function real_pcic_setio().
 *
 *  In:       adapter  - adapter number to look at
 *            socket   - socket on adapter
 *            io       - Pointer to ss_iowin_t holding values to be
 *                       set to the socket
 *  Out:      0
 */
STATIC int
pcic_setio( int adapter, int socket, ss_iowin_t *io ) 
{
	pcic_t    *bd = &pciccntls[adapter];
	socket_t  *socketp = &(bd->pcic_socketp[socket]);
	int 	  val;
	int       flags;

	/* 
	 * Read the PCIC_WINENABLE register, and if we are mapping any windows,
	 * enable 'em up (at the bottom on the function) 
	 */
	val = pcic_read( socketp->iobase, socketp->regbase+PCIC_WINENABLE);

	/* Window 1 */
	flags = 0;
	if(io->io_baseport1) {
		if( io->io_attr1 & IOWIN_WIDTH16  ) 
			flags = PCIC_IO_SIZE16;
		
		real_pcic_setio( socketp->iobase, socketp->regbase + PCIC_IO0, io->io_baseport1, 
					(io->io_baseport1 + io->io_numport1)-1, flags, bd->pcic_revlevel);
		val |= PCIC_EN_IO0;
	}

	/* Window 2 */
	flags = 0;
	if(io->io_baseport2)  {
		if( io->io_attr2 & IOWIN_WIDTH16  ) 
			flags = PCIC_IO_SIZE16;
		real_pcic_setio( socketp->iobase, socketp->regbase + PCIC_IO1, io->io_baseport2, 
					(io->io_baseport2 + io->io_numport2)-1, flags, bd->pcic_revlevel);
		val |= PCIC_EN_IO1;
	}

	val |= PCIC_EN_IOCS16;
	pcic_write( socketp->iobase, socketp->regbase + PCIC_WINENABLE, val);

	return( 0 );
}


/*
 *	real_pcic_setmem ( ulong_t base, ulong_t memreg, ulong_t start,
 *		      ulong_t end, ulong_t offset, ulong_t info )
 *
 *	Helper function to set mapping for a memory window 
 *
 *	Entry :
 *		base		base of PCIC chip
 *		memreg		memory register base (with socket offset!)
 *		start		memory start loc
 *		end		memory end loc
 *		offset		card memory offset
 *		info		memory info bits
 *	Exit :
 *		Nothing
 *
 */
STATIC void
real_pcic_setmem ( ulong_t base, ulong_t memreg, ulong_t start, ulong_t end, ulong_t offset, ulong_t info )
{
	uchar_t	high;
	ulong_t	off;

	/*
	 * Set the start address...
	 * The pcic requires the offset to be the two's complement of the 
	 * difference between the system memory startaddress, and the start 
	 * address of the PC Card.
	 */
	off = (~(start - offset)) + 1;

	pcic_write ( base, memreg+PCIC_MEMSTARTLB, ( start >> 12 ) & 0xff );
	high = ( start >> 20 ) & 0x0f;		/* high order nibble */
	if ( info & PCIC_MEM_SIZE16 ) {
		/*
		 *	16 bit path
	 	 */
		high |= 0x80;
	}
	if ( info & PCIC_MEM_ZWS ) {
		/*
		 *	zero wait state bit set 
		 */
		high |= 0x40;
	}
	pcic_write ( base, memreg+PCIC_MEMSTARTHB, high );

	
	/*
	 *	set the end address
	 */	
	pcic_write ( base, memreg+PCIC_MEMSTOPLB, ( end >> 12 ) & 0xff );
	high = ( end >> 20 ) & 0x0f;		/* high order nibble */
	if ( info & PCIC_MEM_WAIT1 ) {
		/*
		 *	first wait state bit
		 */
		high |= 0x40;
	}
	if ( info & PCIC_MEM_WAIT2 ) {
		/*
	 	 *	second wait state bit
		 */
		high |= 0x80;
	}
	pcic_write ( base, memreg+PCIC_MEMSTOPHB, high );


	/*
	 *	set the offset
	 */
	pcic_write ( base, memreg+PCIC_MEMOFFSETLB, ( off >> 12 ) & 0xff );
	high = ( off >> 20 ) & 0x3f;		/* high order 6 bits */
	if ( info & PCIC_MEM_ATTRIBUTE ) {
		/*
		 *	attribute memory instead of common memory 
		 */
		high |= 0x40;
	}
	if ( info & PCIC_MEM_WRTPROT ) {
		/*
		 *	write protect the memory
		 */
		high |= 0x80;
	}
	pcic_write ( base, memreg+PCIC_MEMOFFSETHB, high );
}




/*
 * pcic_setmem( int adapter, int socket, int win, ss_memwin_t *mem)
 *	In: adapter - The adapter the request is targeted to
 *	     socket -  The socket on the adapter
 *	        win - The window of the socket controller to use
 *	        mem - struct containing what/where/how info of what to map.
 *
 *	Out:  EBUSY -  If window is already mapped.
 *	     EINVAL -  Invald arguments 
 *	          0 -  Godspeed
 */
int
pcic_setmem( int adapter, int socket, int win, ss_memwin_t *mem) 
{
	pcic_t    *bd;
	socket_t  *socketp;
	ulong_t   offset, base, size;
	int	  status;
	ulong_t	  flags;

	bd = &pciccntls[adapter];

	if ( (win > PCIC_NUMMEMWIN) || (mem == (ss_memwin_t *) NULL) || (socket < 0) || (socket > (int) bd->pcic_numsockets)  ) {
		cmn_err(CE_NOTE, "!%s:pcic_setmem:invalid args", DRVPREFIX);
		return EINVAL;
	}

	socketp = &(bd->pcic_socketp[socket]);
	/* Read and store the PCIC_WINENABLE register */ 
	status = pcic_read( socketp->iobase, socketp->regbase + PCIC_WINENABLE);

	/* If we are coming in to enable the window, do it ... */ 
	if(mem->mem_attributes & MEMWIN_ENABLED) {


		/* If the user passed a physical address ... */
		if( mem->mem_pbase ) {
			/* Specified size must be a multiple of 4K cs_RequestWindow takes card of that! */
			size = mem->mem_size;
			offset = mem->mem_ibase;  /* This is the offset into the page ... set in MapMemPage 
						   * (or defaults to 0) See csRequestWindow 
						   */
			base = mem->mem_pbase;
		} else { 
			/* Base address can be specified by the user, or if it is zero, use the 
	 	 	 * pcic's memory (bd->pcic_memstart).   If pbase is not set, the window size is
	 	 	 * defaulted to the size of the pcic's memory window 
	 	         */
			base = bd->pcic_memstart;
			size = bd->pcic_memsize;

			/* We must be tuple parsing, map on 4k boundardies */
			offset = mem->mem_ibase & 0x00fff000;
		}



		/* Check to make sure that the window is not already enabled */
		if (status & (1 << win ) ) {
			cmn_err(CE_NOTE, "!%s:pcic_setmem:window %d already enabled", DRVPREFIX, win);
			return EBUSY;
		}

		flags = (mem->mem_attributes & MEMWIN_ATTRIBUTE) ? PCIC_MEM_ATTRIBUTE : 0;
		if (mem->mem_attributes & MEMWIN_WIDTH16) {
			/* 
			 * Wait states only apply to 16 bit accesses if 8 bit accesses, 
			 * wait is implied to be 6clk cycles  
			 */	
			int cycles;
			flags |= PCIC_MEM_SIZE16;
			if (mem->mem_speed) {
				cycles = mem->mem_speed / PCIC_MAXCYCLE;
				if (cycles > 3) {
					cycles = 3;
					cmn_err(CE_NOTE, "!%s:pcic_setmem:speed truncated to %dns", DRVPREFIX,  cycles * PCIC_MAXCYCLE);
				}
				flags |= (cycles == 3 ) ? PCIC_MEM_WAIT3 : 0;
				flags |= (cycles == 2 ) ? PCIC_MEM_WAIT2 : 0;
				flags |= (cycles == 1 ) ? PCIC_MEM_WAIT1 : 0;
			}
		}

		flags |= (mem->mem_attributes & MEMWIN_ZEROWS) ? PCIC_MEM_ZWS : 0;
		flags |= (mem->mem_attributes & MEMWIN_WRTPROT) ? PCIC_MEM_WRTPROT : 0;

		/* Set all the PCIC registers for this access */
		real_pcic_setmem(socketp->iobase, (socketp->regbase) + PCICWINNUM_TO_MEMREG(win), 
				base, base+size, offset, flags);

		/*
		 * For convenience, when tuple parsing, we will  
		 * set the physical base (for use with physmap later to point to the 
		 * window we just mapped, PLUS the offset left in the page )
		 */
		if (!mem->mem_pbase )
			mem->mem_pbase = bd->pcic_memstart + (mem->mem_ibase & 0x00000fff);	

		/* Enable the access via the PCIC_WINENABLE */
		pcic_write( socketp->iobase, socketp->regbase + PCIC_WINENABLE, status | (1 << win) );
	} else {
 		/* if the MEMWIN_ENABLED was not set, we must be turning off the window */
		pcic_write( socketp->iobase, socketp->regbase + PCIC_WINENABLE, status & ~(1 << win) );
	}
	return ( 0 );
}



/* 
 *
 * pcic_setirq  - Set the IRQ for a card in a socket 
 *  In:       adapter  - adapter number to look at
 *            socket   - socket on adapter
 *            irq      - Pointer to ss_irq_t holding values to be
 *                       set to the socket
 *  Out:      0        - Success
 *
 */
int
pcic_setirq( int adapter, int socket, ss_irq_t *irq ) 
{
	pcic_t    *bd = &pciccntls[adapter];
	socket_t  *socketp = &(bd->pcic_socketp[socket]);
	int 	  val;

	val = irq->irq & 0xf;
	val |= PCIC_IRQ_NOTRESET;
	pcic_write ( socketp->iobase, socketp->regbase + PCIC_INTR, val );

	return ( 0 );
}
/********************** END PCIC ************************/



/********************** BEGIN Socket Services ************************/
#include "pccardss.h"

#define SS_DRVPREFIX "SocketServices"

int pccardss_adapter_count;
STATIC pccard_adapter_t pccardss_adapter[MAXPCCARDADAPTERS];


/* 
 *   pccardssinit - Initialize socket services
 *    In:    Nothing
 *   Out:    0
 */
int
pccardssinit( void ) {
	pccardss_adapter_count = 0;
        return ( 0 );
}


/*
 *
 *    In:        name - a null terminated string with "name" of adapter
 *	     boardnum - Instance number of a particular type of adapter
 *         numsockets - number of sockets on the adapter
 *                 ss - Pointer to socket_services function structure for this adapter
 *	    nummemwin - number of memory windows adapter supports per socket
 *	     numiowin - number of io windows adapter supports per socket
 *
 *    Returns:  EINVAL if name is null, numsockets less than 0, or too many adapters have already registered
 *		ENOMEM if cannot allocate lock 
 */

int
pccardss_adapter_register( char *name, int boardnum, int numsockets, struct socket_services *ss, int nummemwin, int numiowin)
{
	if (pccardss_adapter_count >= MAXPCCARDADAPTERS) {
		cmn_err(CE_NOTE, "%s:Could not register PC Card adapter: Too many adapters", SS_DRVPREFIX);
		return EINVAL;
	}
	
        if ( name != (char *) NULL && numsockets > 0) {
		pccardss_adapter[pccardss_adapter_count].pcca_boardnum = boardnum;
                strncpy(pccardss_adapter[pccardss_adapter_count].pcca_name, name, PCCARDMAXNAME);
                pccardss_adapter[pccardss_adapter_count].pcca_numsockets = numsockets;
                pccardss_adapter[pccardss_adapter_count].pcca_services = ss;
                pccardss_adapter[pccardss_adapter_count].pcca_num_memwin = nummemwin;
                pccardss_adapter[pccardss_adapter_count].pcca_num_iowin = numiowin;
                pccardss_adapter_count++;
                return( 0 );
        } else
                return( EINVAL );
}


/*
 *  Socket Services GetAdapterCount function
 *  In: Nothing
 *  Out: Total count of socket adapters in the system
 *  BUGS:   Card Services will only probably call this function once...
 *          therefore any other socket adapter drivers registered after 
 *          Card Services calls this function will not be known to Card Serv.
 */
int
pccardss_GetAdapterCount( void ) {

        return pccardss_adapter_count;

}



/*
 *  Socket Services InquireAdapter function
 *  In:  adapter - the logical adapter to get information for
 *  In:  adapterinfo - pointer to pointer to adapter information struct
 *  Out:  0 - Success
 *  Out:  ENODEV - When there is an invalid adatper number passed
 */          
int
pccardss_InquireAdapter(unsigned adapter, pccard_adapter_t **adapterinfo) {
        if ( adapter < pccardss_adapter_count ) {
                *adapterinfo = &pccardss_adapter[adapter];
                return ( 0 );
        } else 
		return ( ENODEV );
}
/******************** END Socket Services Functionality ********************/



/******************** BEGIN Card Services Functionality **************************/
#include "pccardss.h"
#include "pccardcs.h"

#define CS_DRVPREFIX "CardServices"

pccard_socket_t pccardcs_socket[MAXPCCARDSOCKETS];
int cs_total_sockets = 0;
STATIC int cs_num_client;

LKINFO_DECL(cs_registration_lkinfo, "IO:CardServicesRegistrationLock", 0);
STATIC lock_t *cs_registration_lock;


#define validsocket( s ) ( (( (ushort)(s & 0xff)) >= (ushort) 0) && ( ( (ushort)(s & 0xff)) <  (ushort) cs_total_sockets))
#define validhandle( h ) ( h && (h->h_registered & CS_HANDLE_ACTIVE) )
#define islinktuple( t ) ( (t->Attribute & CS_TFLAGS_ISLINK) )


/*
 *   pccardcsinit - Initialize socket services
 *    In:    Nothing
 *   Out:     0 - Success
 *       ENODEV - no socket adapters found, or too many sockets  
 */
int
pccardcsinit( void ) {

        pccard_adapter_t *ap;
        unsigned numadapters, i, j;

        if ( (numadapters = pccardss_GetAdapterCount()) == 0 ) {
                cmn_err(CE_NOTE, "%s: No socket adapters found", CS_DRVPREFIX);
                return ENODEV;
        }

	/* 
	 * CS inquires the SS module and walks through all the adapters that have
	 * been registered....
	 */
        for(i = 0; i < numadapters; i++) {
                if (pccardss_InquireAdapter(i, &ap) != 0) {
                        cmn_err(CE_NOTE, "%s:InquireAdapter:%d", CS_DRVPREFIX, i);
                        continue;
                }

		/*  ...If the adapter struct is valid... */
	        if ( ap->pcca_name != (char *) NULL && ap->pcca_numsockets > 0) {

			/* ... Walk through all the sockets on that adapter... */
			for (j = 0; j < ap->pcca_numsockets; j++) {

				/* ...making sure not to overrun limits.  */
				if ( cs_total_sockets >= MAXPCCARDSOCKETS)  {
					cmn_err(CE_NOTE, "%s:Could not register PC Card adapter: Too many sockets", CS_DRVPREFIX);
					continue;
				}
				/* 
				 * pccardcs_socket is an array of logical sockets.  Since (eventually) we may have
				 * more than one controller, (and CardServices clients should never need to know 
				 * about adapters), we need a way of mapping a "logical" socket to a socket on an
				 * adapter.  This mapping takes place here... 
				 */
				pccardcs_socket[cs_total_sockets].adapterp = ap;
				pccardcs_socket[cs_total_sockets].realsock = j;
				cs_total_sockets++;
			}
		}
	}
	if (!(cs_registration_lock = LOCK_ALLOC( CSHEIR, plbase, &cs_registration_lkinfo, pcic_sleepflag))) {
		cmn_err(CE_WARN, "%s:no memory for adapter lock", DRVPREFIX);
		return( ENOMEM );
	}
        return( 0 );
}





/*
 * cs_GetStatus - Get the status of a card/socket.   
 *   In:   Pointer to struct CSStatus  to be populated
 *  Out:   Value returned by SocketSerivces get_status function
 *      The only status returned in this release is s->CardDetect
 */
STATIC int
cs_GetStatus(struct CSStatus *s) {
	int socket;
	pccard_adapter_t *ap;
 
	socket  = pccardcs_socket[ (s->LogicalSocket & 0xff)].realsock;
	ap = pccardcs_socket[socket].adapterp;
	return (ap->pcca_services->get_status)(ap->pcca_boardnum, socket, (ss_socketstatus_t *) (&s->SocketState) );
}




/*
 * cs_DeregisterClient - Deregister a client of CardServices
 *   In:   h - handle of client
 *  Out:   CS_SUCCESS
 *       No checking to see if this client handle is registered
 */
STATIC int
cs_DeregisterClient(handle_t *h)
{
	pl_t opl;

        if (h != (handle_t *) NULL) {
                bzero( (caddr_t) h, sizeof(handle_t) );
        }
	opl = LOCK( cs_registration_lock, plbase);
        cs_num_client--;
	UNLOCK( cs_registration_lock, opl);

        return CS_SUCCESS;
}




/*
 * cs_RegisterClient - register a client of CardServices
 *   In:   h - Pointer to handle of client
 *  Out:   CS_SUCCESS
 *       
 */
STATIC int
cs_RegisterClient(handle_t *h)
{
	pl_t opl;

	if (h != (handle_t *) NULL) {
		/* bzero the whole handle...Tuple Parsing requires 
		 * certain flags to be initialized to zero.
		 */
		bzero( (caddr_t) h, sizeof(handle_t) );
		h->h_registered = (uchar_t) CS_HANDLE_ACTIVE;
		h->h_client = (uchar_t) cs_num_client;
	}
	opl = LOCK( cs_registration_lock, plbase);
	cs_num_client++;
	UNLOCK( cs_registration_lock, opl);

	return( CS_SUCCESS );
}


/*
 * cs_RequestIRQ - Request an IRQ
 *   In:   h - Pointer to handle of client
 *       irq - Pointer to  struct CSIrq containing IRQ request
 *  Out:   CS_SUCCESS  - Success 
 *	   CS_BAD_IRQ  - Bad IRQ passed in
 *
 *        This function takes the requested IRQ, saves it in the handle 
 *        for when the client calls "RequestConfiguration"
 *	  NOTE:  There is no validation on input to this function
 */
STATIC int
cs_RequestIRQ (handle_t *handle, struct CSIrq *irq ) {
	int i;

	irq->AssignedIRQ = handle->h_irq.irq = 0;

	/* If the the 0x10 bit is set in IRQInfo1, then the requested
	 * IRQ is actually a bitmap in IRQInfo2.   If not, the IRQ is
	 * the encoded value in the low order 4 bits of IRQInfo1.
	 */
	if (irq->IRQInfo1 & 0x10) {
		/* The IRQ is in a bitmask in IRQInfo2 */
		for (i = 0 ; i < 16; i++ ) {
			if ( (1 << i) & irq->IRQInfo2)  
				irq->AssignedIRQ = handle->h_irq.irq = i;
		}
	}
	else {
		/* The  low order four bits are the IRQ */
		irq->AssignedIRQ = handle->h_irq.irq = irq->IRQInfo1 & 0xf;
	}

	if (irq->AssignedIRQ == 0)  {
		cmn_err(CE_NOTE, "!%s:RequestIRQ:IRQ not set", CS_DRVPREFIX);
		return (CS_BAD_IRQ);
	}

	handle->h_registered |= CS_HANDLE_REQIRQ;
	return( CS_SUCCESS );
}



/*
 * cs_RequestIO - Request IO windows
 *   In:   h - Pointer to handle of client
 *        io - Pointer to  struct CSIo containing IO window requests
 *  Out:   CS_SUCCESS  - Success 
 *
 *        This function takes the requested IO windows, saves them in the handle
 *        for when the client calls "RequestConfiguration"
 *	  NOTE:  There is no validation on input to this function
 */
STATIC int
cs_RequestIO( handle_t *handle, struct CSIo *io) {
	handle->h_io.io_baseport1 = io->Base1;
	handle->h_io.io_numport1  = io->NumPorts1;
	handle->h_io.io_attr1     = io->Attributes1;
	handle->h_io.io_baseport2 = io->Base2;
	handle->h_io.io_numport2  = io->NumPorts2;
	handle->h_io.io_attr2     = io->Attributes2;
	handle->h_registered |= CS_HANDLE_REQIO;
	return CS_SUCCESS;
}



/*
 * cs_RequestWindow - Request memory windows
 *   In:    h - Pointer to handle of client
 *        mem - Pointer to  struct CSIo containing IO window requests
 *         wh - Pointer to the window handle structure
 *  Out:     CS_SUCCESS  - Success 
 *  CS_OUT_OF_RESOURCES  - No memory windows left
 *
 *        This function takes the requested mem windows, saves them in the handle
 *        for when the client calls "RequestConfiguration"
 *        wh, window handle, is set to the number of the memory window assigned
 *        The CardServices spec says wh is opaque to the caller.
 *	  NOTE:  There is no validation on input to this function
 */
STATIC int
cs_RequestWindow( handle_t *handle, win_handle_t *wh, struct CSMem *mem) {


	/* If the base address or size are not on 4K boundaries, return
	 * appropriate values...
	 */
	if (mem->Size % 4096) 
		return ( CS_BAD_SIZE );
	if (mem->Base % 4096) 
		return ( CS_BAD_BASE );

	/* Set up the window handle */
	wh->lsocket = mem->LogicalSocket;
	wh->winnumber = handle->h_num_memwin;
	wh->handle = handle;
	
	if (handle->h_num_memwin < (uchar_t) (MAXMEMWIN - 1) ) {
		handle->h_mem[handle->h_num_memwin].mem_attributes = mem->Attributes;
		handle->h_mem[handle->h_num_memwin].mem_pbase = mem->Base;
		handle->h_mem[handle->h_num_memwin].mem_size = mem->Size;
		handle->h_mem[handle->h_num_memwin].mem_speed = mem->u.AccessSpeed;
		handle->h_mem[handle->h_num_memwin].winnum = handle->h_num_memwin;
		handle->h_mem[handle->h_num_memwin].mem_ibase = 0;  /* set in MapMemPage */
		handle->h_num_memwin++;
		handle->h_registered |= CS_HANDLE_REQMEM;
		return( CS_SUCCESS );
	} else  
		return( CS_OUT_OF_RESOURCE );
}

 
/*  
 * get_tuple - Helper function for GetFirst/NextTuple
 *      In:     t -      Pointer to the tuple to be populated
 *              handle - Pointer to the Client's handle
 * 
 *      Out:    CS_SUCCESS - Valid Tuple
 *		CS_NO_MORE_ITEMS -  No tuples left of specified type
 *		CS_BAD_ARGS - Something bad happened (physmap failed)
 */
STATIC int get_tuple( struct CSTuple *t, handle_t *handle ) {

	int socket, winnum, status, mapsize;
	pccard_adapter_t *ap;
	caddr_t tuplep;
	char *tupledatap;
	ss_memwin_t mem;	
	ss_socket_state_t old, new;	
	uchar_t i;

	/*  Before beginning, save the socket state, when done,
	 *  we'll restore the socket state.  
	 */
	bzero(&old, sizeof(old) );
	bzero(&new, sizeof(new) );


	/* We already know that there are no more items to be retreived... 
	 * so lest pass pass it on.  (This is set in a previous call
	 * to this function)  The spec says the caller must pass the 
	 * same Tuple pointer struct (CSTuple) down for each call.  State 
	 * information is stored in this structure. 
	 */
	if (t->Flags & CS_TFLAGS_NOMORE) 
		return (CS_NO_MORE_ITEMS);

	/*  Might be hasty, but if nothing goes wrong, this will be
         *  the returned status 
	 */
	status = CS_SUCCESS;	

	/* Set up the memory structure to get the next tuple */
	mem.mem_lsocket = t->LogicalSocket;
	mem.mem_attributes = (t->Flags & CS_TFLAGS_ATTRIBUTE) ? MEMWIN_ATTRIBUTE : 0;
	mem.mem_attributes |= MEMWIN_ENABLED;
	mem.mem_speed = CISSPEED; /* nanoseconds ... from the spec */
	mem.mem_ibase = t->CISOffset;	/* The index into PC Card memory */
	mem.mem_pbase = 0;		/* Use of 0 indicates we should use the physical memory 
					 * of the pcic rather than one specified by a client 
				  	 * with pbase of zero, no need to specify a size
					 */


	socket = pccardcs_socket[ (t->LogicalSocket & 0xff)].realsock;
	ap = pccardcs_socket[socket].adapterp;
	
	/* Always use the last available window ... */
	winnum = ap->pcca_num_memwin - 1;
	

	/* Set up the socket adapter to map the memory in */
	(ap->pcca_services->set_memwin)(ap->pcca_boardnum, socket, winnum, &mem);

	/* Check the Power ... if powered off, turn it on */
	(ap->pcca_services->get_state)(ap->pcca_boardnum, socket, &old);
	bcopy(&old, &new, sizeof(new) );

        new.pwrinfo |= POWERENA | AUTOPOWER;
	new.flags |= SOCKET_MEMIO;
        (ap->pcca_services->set_state)(ap->pcca_boardnum, socket, &new);

        new.pwrinfo |= OUTPUTENA;
        (ap->pcca_services->set_state)(ap->pcca_boardnum, socket, &new);

	/* Remember, in attribute memory, only even bytes are valid ... so adjust
	 * the map-in size appropriately, and physmap the area in
	 */
	mapsize = (t->Flags & CS_TFLAGS_ATTRIBUTE) ? MAXTUPLESIZE * 2 : MAXTUPLESIZE;
	tuplep = physmap(mem.mem_pbase, mapsize, pcic_sleepflag);

	/* Physmap fail? */	
	if (tuplep == (caddr_t) NULL) {
		cmn_err(CE_WARN, "!%s:get_tuple:physmap failed", CS_DRVPREFIX);
		return ( CS_BAD_ARGS );
	}

	/*  Get the tuple code and link */
	if (t->Flags & CS_TFLAGS_ATTRIBUTE) {
		/* Remember, Attribute memory is even only
                 * ... odds are undefined.
		 */
		t->TupleCode = ((char *) tuplep)[0];	
		/* Null tuple does not have a link */
		if (t->TupleCode != CISTPL_NULL)  
			t->TupleLink = ((char *) tuplep)[2]; 
		tupledatap =  (char *) &tuplep[4];
	} else {
		t->TupleCode = ((char *) tuplep)[0];	
		/* Null tuple does not have a link */
		if (t->TupleCode != CISTPL_NULL) 
			t->TupleLink = ((char *) tuplep)[1];	
		tupledatap =  (char *) &tuplep[2];
	}

	/* We expect a link target in one of two spots, either at the first
	 * element of the primary CIS (optional), or as the first element
	 * of a linked tuple chain (required).
	 */
	if (t->TupleCode == CISTPL_LINKTARGET) {
		if ( !( t->Flags & CS_TFLAGS_EXPECTLT || ((t->Flags & CS_TFLAGS_ATTRIBUTE) && t->CISOffset == 0) )) {
			cmn_err(CE_NOTE, "!%s:get_tuple:CISTPL_LINKTARGET found when not expected", CS_DRVPREFIX);
		}
	} 
	else if ( t->Flags & CS_TFLAGS_EXPECTLT ) {
		/* We did not get a LinkTarget when we expected to...Warning and continuing does not 
	 	 * sound like the best idea,  subsequent gets will return bogus data.  
		 */
		cmn_err(CE_NOTE, "!%s:get_tuple:CISTPL_LINKTARGET expected but not found", CS_DRVPREFIX);

		/* Free the mapped memory and then turn off the mempage at the socket adapter */
		physmap_free(tuplep, mapsize, 0);
		mem.mem_attributes &= ~MEMWIN_ENABLED;
		(ap->pcca_services->set_memwin)(ap->pcca_boardnum, socket, winnum, &mem);

		/* Put the power flags back where they were */
		(ap->pcca_services->set_state)(ap->pcca_boardnum, socket, &old);

		return (CS_NO_MORE_ITEMS);
	}

	/* Regardless of the test above, turn off the EXPECTLT */
	t->Flags &= ~CS_TFLAGS_EXPECTLT;

	/* 
  	 * Parse the tuple code, looking at all the fine details, and
	 * get ready for the next call!
	 */
	switch(t->TupleCode) {
		case CISTPL_LINKTARGET:
			t->Flags |= CS_TFLAGS_ISLINK;
			t->CISOffset += (t->Flags & CS_TFLAGS_ATTRIBUTE) ? (t->TupleLink * 2) + 4 : t->TupleLink + 2;
			break;
		case CISTPL_LONGLINK_C: 
			t->Flags |= CS_TFLAGS_LLCOMMON;  /* Note: fall through to next case... */
			/*FALLTHRU*/
		case CISTPL_LONGLINK_A:
			t->Flags |= CS_TFLAGS_LONGLINK;  
			if(t->Flags & CS_TFLAGS_ATTRIBUTE)  {
				/* Offset is stored Lowest byte first ... note multiply offset by 2 */
				t->LinkOffset = (ulong_t) ((((char *) tuplep)[4]) | (((char *) tuplep)[6]) << 8 | 
						(((char *) tuplep)[8]) << 16 | (((char *) tuplep)[10]) << 24) ;
				
			} else {
				/* Offset is stored Lowest byte first */					
				t->LinkOffset = (ulong_t) ( (((char *)tuplep)[2]) | (((char *)tuplep)[3]) << 8 | 
						(((char *)tuplep)[4]) << 16 | (((char *)tuplep)[5]) << 24 );
			}

			/* Spec says that the TPLL_ADDR field in a CISTPL_LONGLINK_A does not contain the
			 * real address in Attribute memory, and needs to be multiplied by 2; 
			 */
			if (t->TupleCode == CISTPL_LONGLINK_A) {
				t->LinkOffset *= 2;
			}

			t->Flags |= CS_TFLAGS_ISLINK;

			/* OFFSET CALCULATION...
			 * A relatively complicated calculation...if we are in attribute memory, 
			 * tuple link contains the value which is the distance to the next tuple.
			 * That distance begins at the next byte.   So, the calculation of the next
  			 * offset must take into consideration, the two bytes containing the TupleCode,
			 * and TupleLink.
			 */
			t->CISOffset += (t->Flags & CS_TFLAGS_ATTRIBUTE) ? (t->TupleLink * 2) + 4 : t->TupleLink + 2;

			break;
			
		case CISTPL_NULL:  /* Free form ... ignore the Link Code skip 2 bytes if in attrib, 1 if in common */
			t->Flags |= CS_TFLAGS_ISLINK;
			t->CISOffset += (t->Flags & CS_TFLAGS_ATTRIBUTE) ? 2 : 1;
			break;
		case CISTPL_END:
			t->Flags |= CS_TFLAGS_ISLINK;
			if (t->Flags & CS_TFLAGS_LONGLINK) {

				if (t->Flags & CS_TFLAGS_LLCOMMON) /* Longlink to common memory */
					t->Flags &= ~CS_TFLAGS_ATTRIBUTE;
				else /* Longlink to attribute memory */
					t->Flags |= CS_TFLAGS_ATTRIBUTE;
				
				t->CISOffset = t->LinkOffset;

				/* We're not on the primary anymore */
				t->Flags &= ~CS_TFLAGS_PRIMARY;

				/* Turn off long-link indicators */
				t->Flags &= ~(CS_TFLAGS_LONGLINK | CS_TFLAGS_LLCOMMON);

				/* Expect a link target */
				t->Flags |= CS_TFLAGS_EXPECTLT;
				t->LinkOffset = 0;

			} else {
				/* The spec says that if you get the CISTPL_END, and you 
			         * are still processing the primary chain 
				 * (the chain starting at attrib offset 0)
				 * and you have never got a CISTPL_NO_LINK, jump to
				 * common memory offset 0, (and expect a link target there).	
				 */
				if ( !(t->Flags & CS_TFLAGS_NOLINKS) && (t->Flags & CS_TFLAGS_PRIMARY) ) {
					t->Flags |= CS_TFLAGS_EXPECTLT;   /* 1. Expect link target */
					t->Flags &= ~CS_TFLAGS_ATTRIBUTE; /* 2. Go into Common */
					t->CISOffset = 0;	          /* 3. Set the offset to zero */
					t->Flags &= ~CS_TFLAGS_PRIMARY;   /* 4. We are no longer the primary chain */
				} else {
					t->Flags |= CS_TFLAGS_NOMORE;		
				}
				
			}
			break;  

		case CISTPL_NO_LINK: 
			t->Flags |= CS_TFLAGS_NOLINKS;
			t->CISOffset += (t->Flags & CS_TFLAGS_ATTRIBUTE) ? (t->TupleLink * 2) + 4 : t->TupleLink + 2;
			break;	
		default:
			/* A tuple link code of 0xff can mean that we are at the end of chain...lets do processing
			 * similar to above.
			 */
			if (t->TupleLink == 0xff)  {
				if (t->Flags & CS_TFLAGS_LONGLINK) {
	
					if (t->Flags & CS_TFLAGS_LLCOMMON)  /* Longlink to common memory */
						t->Flags &= ~CS_TFLAGS_ATTRIBUTE;
					else /* Longlink to attribute memory */
						t->Flags |= CS_TFLAGS_ATTRIBUTE;

					t->CISOffset = t->LinkOffset;

					/* We're not on the primary anymore */
					t->Flags &= ~CS_TFLAGS_PRIMARY;
	
					/* Turn off long-link indicators */
					t->Flags &= ~(CS_TFLAGS_LONGLINK | CS_TFLAGS_LLCOMMON);
					t->LinkOffset = 0;
	
					/* The next tuple should be a link target */
					t->Flags |= CS_TFLAGS_EXPECTLT;
	
				} else {
					/* The spec says that if you get the CISTPL_END, and you 
				         * are still processing the primary chain 
					 * (the chain starting at attrib offset 0)
					 * and you have never got a CISTPL_NO_LINK, jump to
					 * common memory offset 0, (and expect a link target there).	
					 */
					if ( !(t->Flags & CS_TFLAGS_NOLINKS) && (t->Flags & CS_TFLAGS_PRIMARY) ) {
						t->Flags |= CS_TFLAGS_EXPECTLT;   /* 1. Expect link target */
						t->Flags &= ~CS_TFLAGS_ATTRIBUTE; /* 2. Go into Common */
						t->CISOffset = 0;     /* 3. Set the offset to zero */
						t->Flags &= ~CS_TFLAGS_PRIMARY;   /* 4. We are no longer the primary chain */
					} else {
						t->Flags |= CS_TFLAGS_NOMORE;
					};
						
				}
			}
			/*  We are obviously not expecting a Link Target tuple anymore */
			t->Flags &= ~CS_TFLAGS_EXPECTLT;
			/* Calculate the offset to the next tuple */
			t->CISOffset += (t->Flags & CS_TFLAGS_ATTRIBUTE) ? (t->TupleLink * 2) + 4 : t->TupleLink + 2;
			break;
	}

	/* Lets cache the tuple data for a possible call to get_tuple_data() 
         * NULL and END are FREE FORM ... no data with the tuple, and no link 
	 * The spec says this is not the best thing to do since the "read" operation of 
	 * the memory page might cause a reaction on the device ...(kind of 
	 * like when a io register is read it clears an interrupt...i.e. destructive read)
	 * I dont know what else to do...If this becomes problematic we will fix it.
	 */
	if (t->TupleCode != CISTPL_NULL && t->TupleCode != CISTPL_END)  {
		handle->h_tuplesize = t->TupleLink;
		for (i = 0; i <	t->TupleLink; i++)
			handle->h_tupledata[i] = tupledatap[ ((t->Flags & CS_TFLAGS_ATTRIBUTE) ? i * 2 : i ) ];
	}
	
	/* Free the mapped memory and then turn off the mempage at the socket adapter */
	physmap_free(tuplep, mapsize, 0);

	mem.mem_attributes &= ~MEMWIN_ENABLED;
	if ( (ap->pcca_services->set_memwin)(ap->pcca_boardnum, socket, winnum, &mem) != 0)  {
		status = CS_BAD_ARGS;
	} 

	/* Put the power flags back where they were */
	(ap->pcca_services->set_state)(ap->pcca_boardnum, socket, &old);
	return (status);
	
}


/*
 * cs_GetNextTuple - Get the next tuple of the specified type
 *      In:     t - Pointer to the tuple to be populated
 *              h - Pointer to the Client's handle
 *
 *      Out:    CS_SUCCESS - Valid Tuple
 *              CS_NO_MORE_ITEMS -  No tuples left of specified type
 *              CS_BAD_ARGS - Something bad happened (physmap failed)
 */
STATIC int
cs_GetNextTuple( struct CSTuple *t, handle_t *h ) {

	int status;

	/* Set up a counter ... if the counter reaches MAXTUPLENOLINK 
	 * w/out hitting a tuple, lets quit with CS_NO_MORE_ITEMS.
	 */
	int counter = 0;

	/* If we have not read tuples before, set up some state */
	if ( !(h->h_registered & CS_HANDLE_TUPLES_READ) ) {
		h->h_registered |= CS_HANDLE_TUPLES_READ;
		t->Flags  = CS_TFLAGS_ATTRIBUTE | CS_TFLAGS_PRIMARY;
		t->CISOffset = 0;
		t->LinkOffset = 0;
	}
		
	/* Give the next available Tuple */
	if (t->DesiredTuple == 0xff) { 
		/* Get the next tuple */
		status = get_tuple(t, h); 

		/* If the caller wants links, just return, otherwise, look for the first
		 * non link tuple 
		 */
		if ( !(t->Attribute & CS_TATTR_RETURNLINK) ) {
			while( (status == CS_SUCCESS) && islinktuple( t ) )  {
				status = get_tuple(t, h); 
				if (++counter > MAXTUPLENOLINK) 
					break;
			}
		} 
	} else {
		/* Search for a specific Tuple */
		while( (status = get_tuple(t, h)) == CS_SUCCESS ) {
			if (t->TupleCode == t->DesiredTuple)
				break;
			else
				if (++counter > MAXTUPLENOLINK)
					break;
		}
	}
	if (counter >= MAXTUPLENOLINK)
		status = CS_NO_MORE_ITEMS;

	/* We've either found a tuple, or hit the end */
	return( status );	
}




/*
 * cs_GetNextTuple - Get the first tuple of the specified type
 *      In:     t - Pointer to the tuple to be populated
 *              h - Pointer to the Client's handle
 *
 *      Out:    CS_SUCCESS - Valid Tuple
 *              CS_NO_MORE_ITEMS -  No tuples left of specified type
 *              CS_BAD_ARGS - Something bad happened (physmap failed)
 */
STATIC int
cs_GetFirstTuple( struct CSTuple *t, handle_t *h ) {
	int status;
	h->h_registered &= ~CS_HANDLE_TUPLES_READ;
	status = cs_GetNextTuple(t, h);
	return status;
}



/*
 * cs_GetTupleData - Get the next tuple of the specified type
 *      In:     tdata - Pointer to the tuple data structure
 *              handle - Pointer to the Client's handle
 *
 *      Out:    CS_SUCCESS - Valid Tuple Data
 *              CS_NO_MORE_ITEMS -  Attempt to read beyond the end of Tuple
 */
STATIC int	
cs_GetTupleData( struct CSTupleData *tdata, handle_t *h) {
	
	int size;

	/* Calculate the size to transfer out */
	size = h->h_tuplesize - tdata->TupleOffset;

	/* Attempt to read beyond the end of Tuple */
	if (size <= 0)  
		return( CS_NO_MORE_ITEMS );

	/* In the spec, it is not clear whether the TupleDataLen is 
	 * supposed to be the size of the entire tuple (which the caller
	 * should know already!), or the size being transferred.   We'll assume
	 * the latter, and set things up that way...
	 */
	tdata->TupleDataLen = (ushort_t) size; 

	/* Careful not to overflow the callers buffer.  The spec says that 
	 * tdata->TupleDataLen can be larger than tdata->TupleDataMax
	 */
	if ( (ushort_t) size > tdata->TupleDataMax)
		size = tdata->TupleDataMax;

	bcopy( h->h_tupledata + tdata->TupleOffset, tdata->TupleData, size);
	return( CS_SUCCESS );
}



/*
 * cs_RequestConfiguration - Set up the socket adapter with previously
 *			     requested IRQ,IO,MEM windows.
 *      In:     c - Pointer to the CSConfuig structure to be setup
 *              h - Pointer to the Client's handle
 *
 *      Out:    CS_SUCCESS  - Configuration set
 *              CS_BAD_ARGS - Something bad happened (physmap failed)
 */
STATIC int	
cs_RequestConfiguration( struct CSConfig *c, handle_t *h) {
	int i, socket;
	pccard_adapter_t *ap;
	ss_socket_state_t state;

	socket  = pccardcs_socket[ (c->LogicalSocket & 0xff)].realsock;
	ap = pccardcs_socket[socket].adapterp;

	/* If the IRQ is enabled, and client called RequestIRQ */
	if(c->Attributes & CS_CONFIG_ENIRQ && (h->h_registered & CS_HANDLE_REQIRQ)) 
		(void) (ap->pcca_services->set_irqinfo)(ap->pcca_boardnum, socket, &(h->h_irq) );

	/* If client called RequestIO */
	if (h->h_registered & CS_HANDLE_REQIO)
		(void) (ap->pcca_services->set_iowin)(ap->pcca_boardnum, socket, &h->h_io );

	/* For each window the client called RequestWindow */
	if (h->h_registered & CS_HANDLE_REQMEM) {
		for(i = 0; i < (int) h->h_num_memwin; i++) {
			(void) (ap->pcca_services->set_memwin)(ap->pcca_boardnum, socket, i, &h->h_mem[i] );
		}
	}

        /* Check the Power ... if powered off, turn it on */
	bzero( &state, sizeof(state));
        (ap->pcca_services->get_state)(ap->pcca_boardnum, socket, &state);
	state.flags = (c->IntType & CS_CONFIG_TYPE_MEMIO) ? SOCKET_MEMIO : 0;
        state.pwrinfo |= POWERENA | AUTOPOWER;
        (ap->pcca_services->set_state)(ap->pcca_boardnum, socket, &state);

        state.pwrinfo |= OUTPUTENA;
        (ap->pcca_services->set_state)(ap->pcca_boardnum, socket, &state);


	/* If any of the configuration flags are present, set the registers  */
	if (c->Present) {
		ss_memwin_t win;
		uchar_t *configreg;

		/* Map in the attriubute memory with the configuration registers and then physmap it in */
		win.mem_lsocket = c->LogicalSocket;
		win.mem_attributes = MEMWIN_ATTRIBUTE | MEMWIN_ENABLED | MEMWIN_WIDTH16;
		win.mem_speed = CISSPEED;
		win.mem_size = CISWINSIZE; /* smallest window possible */
		win.mem_ibase = c->ConfigBase;
		win.mem_pbase = 0; /* Use pcic memory */
		(ap->pcca_services->set_memwin)(ap->pcca_boardnum, socket, ap->pcca_num_memwin - 1, &win);
		
		if ( (configreg = (uchar_t *) physmap(win.mem_pbase, 4096, pcic_sleepflag)) == (uchar_t *) NULL)
			return( CS_BAD_ARGS );

#ifdef CARDRESET
		/* Reset the card */
		configreg[0] = (uchar_t) 0x80;  /* SRESET */   
		drv_usecwait(2000000);
		configreg[0] = (uchar_t) 0x00;  /* SRESET */   
#endif

		/* If the option register is set by the caller */
		if (c->Present & CS_CONFIG_OPTPRESENT) {
			configreg[0] = (uchar_t) c->ConfigIndex & 0x3f; 
		}
		/* If the status register is set by the caller */
		if (c->Present & CS_CONFIG_STATPRESENT) {
			configreg[2] = (uchar_t) c->Status;	
		}	
		/* If the pin register is set by the caller */
		if (c->Present & CS_CONFIG_PINPRESENT) {
			configreg[4] = (uchar_t) c->Pin;	
		}	
		/* If the copy register is set by the caller */
		if (c->Present & CS_CONFIG_COPYPRESENT) {
			configreg[6] = (uchar_t) c->Copy;	
		}	
		/* If the extended status register is set by the caller */
		if (c->Present & CS_CONFIG_ESPRESENT) {
			configreg[8] = (uchar_t) c->ExtendedStatus;	
		}
	
		/* Unmap the window, and disable it */
		physmap_free((addr_t) configreg, 4096, 0);
		win.mem_attributes &= ~MEMWIN_ENABLED;
		(ap->pcca_services->set_memwin)(ap->pcca_boardnum, socket, ap->pcca_num_memwin - 1, &win);
		
		/* 
		 * After writing the config option register, we should sleep
		 * for a bit ... 2 seconds should do.   Most cards require this 
		 */
		drv_usecwait(2000000);
	}
	return( CS_SUCCESS );
}


STATIC int
cs_MapMemPage( struct CSMemPage *mempage, win_handle_t *wh ) {

	/* Make sure the offset is on a 4K boundary */
	if ( mempage->CardOffset % 4096 )
		return CS_BAD_OFFSET;

	/* Check to see if the socket is valid return CS_BAD_HANDLE */
	if ( !validsocket(wh->lsocket) )
		return CS_BAD_HANDLE; 

	/* Check to see if the winnumber in valid...use  (MAXMEMWIN - 1 )
	 * because one window is reserved for CardServices Tuple parsing
	 */
	if ( wh->winnumber > (MAXMEMWIN - 1 ) )
		return CS_BAD_HANDLE; 

	/* the window handle has a pointer to the client handle.  The Client's
	 * Requested memory windows are stored in the handle.  Lets just add the 
	 * last piece of information, the offset, into the client handle.
	 */
	wh->handle->h_mem[ wh->winnumber ].mem_ibase = mempage->CardOffset;  /* Set the ibase */

	return ( CS_SUCCESS );
}

/*
 * 
 *   CardServices - 
 * 	In:	Service - service requested
 *		Handle  - Client handle 
 *		Pointer - Generic "pointer" type argument...specific to service code
 *		ArgLength - Length of data pointed to by ArgPointer 
 * 		ArgPointer - Pointer to data ... specific to Service code
 *     Out:     (possibly handle, Pointer, and ArgPointer)
 *   		CS_BAD_ARG_LENGTH - if ArgLength is incorrect for Service
 *   		CS_BAD_SOCKET     - if an invalid socket is specified
 *   		CS_BAD_HANDLE     - if an invalid handle is specified
 */
int
CardServices( uchar_t Service, handle_t *handle, void *Pointer, uint_t ArgLength, void *ArgPointer)
{
	int status;

	switch( Service ) {

	case DeregisterClient:
                status = cs_DeregisterClient(handle);
                break;

	case RegisterClient:
#ifdef FUTURECARDSERVICES
		/* Check to make sure ArgLength  is equal to 14 ... */

		if (ArgLength != 14) {
			status = CS_BAD_ARG_LENGTH;
			break;
		}
#endif  /* FUTURECARDSERVICES */
		status = cs_RegisterClient(handle);
		break;	

	case GetStatus:
		/* Check to make sure ArgLength  is equal to 6 ... */
		if ( ArgLength != sizeof(struct CSStatus) ) {
			status = CS_BAD_ARG_LENGTH;
			break;
		}
		/* ... and check that we have a valid socket! */
		if ( !validsocket( ((struct CSStatus *) ArgPointer)->LogicalSocket) ) {
			status = CS_BAD_SOCKET;
			break;
		}
		status = cs_GetStatus( (struct CSStatus *) ArgPointer );
		break;

	case RequestIRQ:
		/* Check to make sure ArgLength is equal to 8 ... */
		if ( ArgLength != sizeof(struct CSIrq) ) {
			status = CS_BAD_ARG_LENGTH;
			break;
		}
		/* ...check that we have a valid socket... */	
		if ( !validsocket( ((struct CSIrq *) ArgPointer)->LogicalSocket) ) {
			status = CS_BAD_SOCKET;
			break;
		}
		/* ...and lastly, check that we have a valid handle. */	
		if ( !validhandle( handle ) ) {
			status = CS_BAD_HANDLE;
			break;
		}
		status = cs_RequestIRQ( handle, (struct CSIrq *) ArgPointer );	
		break;

	case RequestIO:
		/* Check to make sure ArgLength is equal to 8 ... */
		if ( ArgLength != sizeof(struct CSIo) ) {
			status = CS_BAD_ARG_LENGTH;
			break;
		}
		/* ...check that we have a valid socket... */	
		if ( !validsocket( ((struct CSIo *) ArgPointer)->LogicalSocket) ) {
			status = CS_BAD_SOCKET;
			break;
		}
		/* ...and lastly, check that we have a valid handle. */	
		if ( !validhandle( handle ) ) {
			status = CS_BAD_HANDLE;
			break;
		}
		status = cs_RequestIO( handle, (struct CSIo *) ArgPointer );	
		break;

	case RequestWindow:
		/* Check to make sure ArgLength is equal to 8 ... */
		if ( ArgLength != sizeof(struct CSMem) ) {
			status = CS_BAD_ARG_LENGTH;
			break;
		}
		/* ...check that we have a valid socket... */	
		if ( !validsocket( ((struct CSMem *) ArgPointer)->LogicalSocket) ) {
			status = CS_BAD_SOCKET;
			break;
		}
		/* ...and lastly, check that we have a valid handle. */	
		if ( !validhandle( handle ) ) {
			status = CS_BAD_HANDLE;
			break;
		}
		if (!Pointer) {
			status = CS_BAD_ARGS;
			break;
		}
		status = cs_RequestWindow( handle, (win_handle_t *) Pointer, (struct CSMem *) ArgPointer );	
		break;

	case GetFirstTuple:
		/* Check to make sure ArgLength is equal to 18 ... */
		if ( ArgLength != sizeof(struct CSTuple) ) {
			status = CS_BAD_ARG_LENGTH;
			break;
		}
		/* ...check that we have a valid socket... */	
		if ( !validsocket( ((struct CSTuple *) ArgPointer)->LogicalSocket) ) {
			status = CS_BAD_SOCKET;
			break;
		}
		/* and lastly, check that we have a valid handle. */	
		if ( !validhandle( handle ) ) {
			status = CS_BAD_HANDLE;
			break;
		}
		status = cs_GetFirstTuple( (struct CSTuple *) ArgPointer, handle);
		break;

	case GetNextTuple:
		/* Check to make sure ArgLength is equal to 18 ... */
		if ( ArgLength != 18 ) {
			status = CS_BAD_ARG_LENGTH;
			break;
		}
		/* ...check that we have a valid socket... */	
		if ( !validsocket( ((struct CSStatus *) ArgPointer)->LogicalSocket) ) {
			status = CS_BAD_SOCKET;
			break;
		}
		/* ... and lastly, check that we have a valid handle. */	
		if ( !validhandle( handle ) ) {
			status = CS_BAD_HANDLE;
			break;
		}
		status = cs_GetNextTuple( (struct CSTuple *) ArgPointer, handle);
		break;

	case GetTupleData:
		/* Check to make sure ArgLength is equal to 18 ... */
		if ( ArgLength < sizeof(struct CSTupleData) ) {
			status = CS_BAD_ARG_LENGTH;
			break;
		}
		/* ...check that we have a valid socket... */	
		if ( !validsocket( ((struct CSTupleData *) ArgPointer)->LogicalSocket) ) {
			status = CS_BAD_SOCKET;
			break;
		}
		/* ... and lastly, check that we have a valid handle. */	
		if ( !validhandle( handle ) ) {
			status = CS_BAD_HANDLE;
			break;
		}
		status = cs_GetTupleData( (struct CSTupleData *) ArgPointer, handle);
		break;

	case MapMemPage:
		/* Check to make sure ArgLength is equal to 5 ... */
		if ( ArgLength != sizeof(struct CSMemPage) ) {
			status = CS_BAD_ARG_LENGTH;
			break;
		}
		if ((void *) handle == (void *) NULL) {
			status = CS_BAD_HANDLE;
			break;
		}
		status = cs_MapMemPage( (struct CSMemPage *) ArgPointer, (win_handle_t *) handle);
                break;

	case RequestConfiguration:
		if ( ArgLength != sizeof(struct CSConfig) ) {
			status = CS_BAD_ARG_LENGTH;
			break;
		}
		/* ...check that we have a valid socket... */
		if ( !validsocket( ((struct CSConfig *) ArgPointer)->LogicalSocket) ) {
			status = CS_BAD_SOCKET;
			break;
		}
		/* ... and lastly, check that we have a valid handle. */
		if ( !validhandle( handle ) ) {
			status = CS_BAD_HANDLE;
			break;
		}
		status = cs_RequestConfiguration( (struct CSConfig *) ArgPointer, handle);
		break;

	default: 
		status = CS_UNSUPPORTED_SERVICE;
		break;
	}	

	return ( status );
}

extern int pcic_cfgentry_override;

/*   
 * This function has been added to aid in the setting of the Configuration
 * Option Register (COR) on the PC Card.   Currently, the resource database settings
 * (and the user for that matter) drive the settings that are getting 
 * programmed into the PC Card.    The problem is that since the configuration
 * settings are coming from outside of the card, we haveto have logic to match
 * them to one of the settings supported by the card.  The parsecfg function will
 * take:
 * 	- A pointer to a cfgtuple....(a sequence of bytes)
 * 	- The start I/O address found in the resource database
 *	  for the card we are going to program.
 *
 * This function will return: 
 * 	-1 if the tuple is bogus 
 *   	 0 if the tuple does not match the configuration entry 
 * 	 1 if the tuple matches the I/O address passed in
 *
 * The writing ofthe COR actually activates the PC Card.
 * On the first go-round of the modem_enabler, we just found the first
 * COR, and use that value; The three targeted modems did work (Hayes,
 * USRobotics, and MegaHertz.   We found that other modems did not work,
 * namely, Eiger (Cirrus Logic), and HotLine (RFI: popular in Europe).
 * 
 * NOTE:  Only the I/O address fields in the config tuple are considered
 * meaningful by this function.  The rest of the code surrounding  
 * this function simply supports walking through the tuple. 
 */

uchar_t
cs_parsecfg(uchar_t *cfgtuple, int startaddr) {
	
	int index = 0;
	int cfgent = 0;
	int fsb, numpower, i, j;
	unsigned int saddr;

	/* TPCE_INDX: Configuration table index byte */
	cfgent = cfgtuple[ index ] & TPCE_INDX_ENTRY_MASK;

	/* The TPCE_INDX has the "interface" bit set, read it */
	if( TPCE_INDX_HAS_INTERFACE( cfgtuple[ index ] ) ) {
		index++; 
		/* TPCE_IF: Table Entry Interface Description byte 
		 * This is where we should parse the interface description field 
		 * We are currently ignoring these values.  Also, the 
		 * TPCE_INDX_DEFAULT but should be parsed here.
		 */
	}
	index++;

	
	/* TPCE_FS: Feature selection byte(fsb).   We will remember the feature 
	 * selection byte, and immediately start walking through the config 
	 * entry using the fsb to determine which other fields are present
	 * NOTE: Power requirements are pretty much ignored
	 */
	fsb = cfgtuple[ index++ ];

	if (fsb & TPCE_FS_PWRMASK) {
		/* TPCE_PD:  Power description structure.  The number of power
		 * description structs is stored in the TPCE_FS.    The power
       		 * structures immediately follow the TPCE_FS. 
  		 */
		numpower = TPCE_NUMPOWER( fsb );

		/* For each power description structure specified */
		for (i = 0; i < numpower; i++) {
			/* Count the number of paramter definitions */
			int numpowerparam = TPCE_CNTPARAM(cfgtuple[ index ]);
			index++;
			/* Walk through the paramter definitions */
			for( j = 0; j < numpowerparam; j++) {
				/* If there are extension bytes eat em up... */
				while( cfgtuple[index] & TPCE_POWEREXT )  {
					index++;
				}
				index++;
			}  /* for-j */
		} /* for-i */

	} /* if (fsb & TPCE_FS_PWRMASK) */


	/* If the timing bit is set  walk over the timing information...
	 * NOTE: Timing requirements are pretty much ignored
	 */
	if ( fsb & TPCE_FS_TIMING ) {
		/* TPCE_TD: Configruation Timing information */
		uchar_t tconfig = cfgtuple[index++];
		uchar_t wait;
		uchar_t ready;

		if ( TPCE_TD_WAITNOTUSED( tconfig ) )  {
			wait = cfgtuple[index++];
			/* TPCE_TD: parse WAIT timing info here */

			if ((wait & 0x7) == 0x7) {
				cmn_err(CE_WARN, "Cannot parse extended device speed tuples");
				return -1;
			}
		} /* TPCE_TD_WAITNOTUSED */

		if ( TPCE_TD_READYNOTUSED( tconfig ) ) {
			ready = cfgtuple[index++];
			/* TPCE_TD: parse READY timing info here */

			if ((ready & 0x7) == 0x7) {
				cmn_err(CE_WARN, "Cannot parse extended device speed tuples");
				return -1;
			}
		} /* TPCE_TD_READYNOTUSED */

	} /* fsb & TPCE_FS_TIMING */


	/* Here is the one we want... */
	if ( fsb & TPCE_FS_IOSPACE ) {
		/* TPCE_IO:  I/O Address space derscription */
		int addrlines = cfgtuple[index] & TPCE_IO_IOLINESMASK;

		if (TPCE_IO_HASRANGE( cfgtuple[index] ) ) {
			int numiorange;
			int lensiz, addrsiz;

			index++;
			numiorange = TPCE_IO_NUMRANGE( cfgtuple[index] );

			lensiz = TPCE_IO_LENGTH_SIZE( cfgtuple[index] );
			addrsiz = TPCE_IO_ADDRESS_SIZE( cfgtuple[index++] );
			for(i = 0; i < numiorange; i++) {
				if (addrsiz) {
					saddr = cfgtuple[index++];
					if (addrsiz >= 2)
						saddr |= (cfgtuple[index++] << 8);
					if (addrsiz == 3) {
						saddr |= (cfgtuple[index++] << 16);
						saddr |= (cfgtuple[index++] << 24);
					}
				}
			}
					
		} else {
			/* If Io Address Lines is zero, a range must be 
			 * specified.   If not, we'll get out of here 
			 */ 
			if ( addrlines == 0 ) {
				cmn_err(CE_NOTE, "CISTPL_CFTABLE_ENTRY:TPC_IO in error");
				return -1;
			}
		}
	} /* fsb & TPCE_FS_IOSPACE */

	/* Don't card about the rest of the Tuple */

	if ( startaddr == saddr) 
		return 1;
	else
		return 0;

}

STATIC int
modem_enabler( ) {

	struct CSStatus stat;
	struct CSConfig config;
	struct CSIrq irqreq;
	struct CSIo ioreq;
	struct CSTuple  *tp;
	struct CSTupleData *tdp;
	char tbuf[MAXTUPLESIZE], prodinfo[MAXTUPLESIZE], cfgentry = -1;
	handle_t *handle, *hp;
	int i, k, status, funcid = -1, numserial;
	ulong_t configreg = 0;
	cm_args_t cm_args;
	cm_num_t unit, irq;
	cm_range_t ioaddr;
	ushort_t s;
 
	if ( (numserial = cm_getnbrd( SERIALDRIVERNAME ) ) == 0) {
		/* No serial ports config'd, lets not bother  */
		return 0;
	}

	/* Zero out automatic memory structures */	
	bzero(&stat, sizeof(stat));
	bzero(&config, sizeof(config));
	bzero(tbuf, sizeof(tbuf));
	bzero(&irqreq, sizeof(irqreq));
	bzero(&ioreq, sizeof(ioreq));

	/* Alloc memory to hold the handle structures 
	 * NOTE: Since there is no use for this later, (if we find a modem we 
	 * do not unregister), THIS SPACE IS FREE'D AT THE END OF THIS FUNCTION 
	 */
	if (( handle = kmem_zalloc( cs_total_sockets * sizeof(handle_t), pcic_sleepflag) ) ==  (handle_t *) NULL) {
		cmn_err(CE_NOTE, "%s:kmem_zalloc:Out of memory", CS_DRVPREFIX);
	}
	tp = (struct CSTuple *) tbuf;
	tdp = (struct CSTupleData *) tbuf;

	s = 0;

	/* 
	 * Begin the arduous :) task of walking through the resource database	
	 * for "asyc" entries which have a UNIT field >= MINSERIALPCCARDUNIT.  If found, tool through 
	 * through the sockets loooking for the modem.   So the rule is, the first PC Card
	 * modem entry in the resource database maps to the modem card in the lowest number 
   	 * socket!
	 */	
	for(k = 0; k < numserial; k++) {
		cm_args.cm_key = cm_getbrdkey(SERIALDRIVERNAME, k);
		cm_args.cm_n = 0;
	
		/* Get the UNIT entry */
		cm_args.cm_param = CM_UNIT;
		cm_args.cm_val = &unit;
		cm_args.cm_vallen = sizeof(unit);

		if ( cm_getval(&cm_args) ) {
			cmn_err(CE_NOTE, "!%s:cm_getval:UNIT:%s:%x failed", CS_DRVPREFIX, SERIALDRIVERNAME, k);
			continue;
		}
		if (unit < MINSERIALPCCARDUNIT)	
			continue; /* Regular serial port */

		cm_args.cm_param = CM_IOADDR;
		cm_args.cm_val = &ioaddr;
		cm_args.cm_vallen = sizeof(ioaddr);
		if ( cm_getval(&cm_args) ) {
			cmn_err(CE_NOTE, "!%s:cm_getval:CM_IOADDR:%s:%x failed", CS_DRVPREFIX, SERIALDRIVERNAME, k);
			continue;
		}


		/* 
		 * We found an entry which indicates its a PC Card, so try to find a socket with 
		 * a modem.  No need to look at all the sockets every time through, ("s" was 
		 * initialized to zero prior to entering the "k" loop)
		 */
		for(;s < (ushort_t) cs_total_sockets; s++) {

			hp = &handle[s];
	
			stat.LogicalSocket = s;
			status = CardServices( GetStatus, (handle_t *) NULL,  (void *) NULL, sizeof(struct CSStatus), &stat);
	
			/* If that call fails, or if there is not a card in the socket, 
			 * move on to the next socket.
			 */
			if (status != CS_SUCCESS  || !stat.SocketState.CardDetect ) 
				continue;
			
			
			/* Register to be a client so we can read the CIS */
			status = CardServices( RegisterClient, hp, (void *) NULL, NULL, NULL);
			if (status != CS_SUCCESS)
				continue;
	
				
			tp->DesiredTuple = 0xff;
			tp->Attribute = 0;
			tp->LogicalSocket = s;
			
			/* 
			 * Walk through all the tuples, recording the function id, the config tuple, 
			 * the version tuple, and the first config table entry tuple.
			 */
			while( (status = CardServices( GetNextTuple, hp, (void *) NULL, sizeof(struct CSTuple), tp)) == CS_SUCCESS) {
				switch( tp->TupleCode ) {
				case CISTPL_FUNCID:
					tdp->TupleDataMax = 255;
					status = CardServices( GetTupleData, hp, (void *) NULL, sizeof(struct CSTupleData), tdp);	
					if (status != CS_SUCCESS) 
						break;  /* From while */
					funcid = tdp->TupleData[0];
					break;

				case CISTPL_CONFIG:
					tdp->TupleDataMax = 255;
					status = CardServices( GetTupleData, hp, (void *) NULL, sizeof(struct CSTupleData), tdp);	
					if (status != CS_SUCCESS) 
						break;  /* From while */
					else {
						/*LINTED*/
						uchar_t msksz = tdp->TupleData[0] & 0x3c;
						uchar_t rasz = tdp->TupleData[0] & 0x03;	
	
						configreg = tdp->TupleData[2];
						if (rasz >= 1)
							configreg |= tdp->TupleData[3] << 8;
						if (rasz >= 2)
							configreg |= tdp->TupleData[4] << 16;
						if (rasz == 3)
							configreg |= tdp->TupleData[5] << 24;
					}
					break;

				case CISTPL_VERS_1:
					tdp->TupleDataMax = 255;
					status = CardServices( GetTupleData, hp, (void *) NULL, sizeof(struct CSTupleData), tdp);	
					if (status == CS_SUCCESS) {
						strcpy(prodinfo, (char *) &tdp->TupleData[2]);
						strcat(prodinfo, "-");
						strcat(prodinfo, (char *) &tdp->TupleData[ (strlen(prodinfo) + 2) ]);
					}
					break;
				case CISTPL_CFTABLE_ENTRY:
					/*  If we get back good tuple data, parse the tuple, and see if it matches,
					 *  the I/O address found in the resource database.   If not, try the next
					 *  CISTPL_CFTABLE_ENTRY tuple.   If we dont find any that match the  I/O
					 *  address, we'll default to the "first" cfgentry found.
					 */
					tdp->TupleDataMax = 255;
					status = CardServices( GetTupleData, hp, (void *) NULL, sizeof(struct CSTupleData), tdp);
					if (status == CS_SUCCESS) {
						if (cfgentry == -1) {
							/* First time through */
							cfgentry = tdp->TupleData[0] & 0x3f;
						}
						if (cs_parsecfg((uchar_t *) tdp->TupleData, ioaddr.startaddr) == 1) {
							cfgentry =  tdp->TupleData[0] & 0x3f;
						}
					}
					break;	
				default:
					break;
				}
			}

			/* We should have "no more items" left in the CIS  if not, something is wrong */
			if ( status != CS_NO_MORE_ITEMS ) {
				cmn_err(CE_NOTE, "!%s:Expected CS_NO_MORE_ITEMS: %x", CS_DRVPREFIX, status);
				(void) CardServices( DeregisterClient, hp, (void *) NULL, NULL, NULL);
				continue;  /* for-s  */
			}

			if (pcic_cfgentry_override != -1) 
				cfgentry = pcic_cfgentry_override;
	
			/* 
			 * Some older cards do not have a CISTPL_FUNCID tuple.  If funcid is
			 * still -1, then there was no CISTPL_FUNCID tuple found.  For this case,
			 * we need a mechanism to try to determine a card type.  We'll look it up
		 	 * in a user-modifiable table initialized in Space.c.   In this subroutine,
			 * we are simple looking for modems (funcid==TPLFID_SERIAL).  Here we will 
			 * walk through the cardmap structure, and try to look up the string prodinfo.
			 * prodinfo is defined to be 'product manufacturer' SPACE 'name of product'.
			 * Since we dont really have a way for the user to dump out this string if their 
			 * card does not have a funcid tuple, we'll put it in the putbuf for them so they
			 * can look it up if they have a problem.
			 */
			if (funcid == -1) {
				for(i = 0; i < pcic_cardmap_entries; i++ ) {
					if ( cardmap[i].ident ) {	
						if (strcmp(prodinfo, cardmap[i].ident) == 0)
							funcid = cardmap[i].type;
							break; /* for */
					}
				}
				/* if funcid still == -1, not found -double whammy- put a message 
				 * in the putbuf, and proceed to the next socket.
				 */
				if (funcid == -1)
					cmn_err(CE_NOTE, "!%s:%s:No CISTPL_FUNCID", CS_DRVPREFIX, prodinfo);
			}
			if (funcid == TPLFID_SERIAL)  {
				break; /* out of for-s */
			}
		}

		/* If we've walked all through the sockets and have not found anything
		 * we are done.  Break out of the for-k.
		 */
		if (s >= (ushort_t) cs_total_sockets) 
			break; /* for-k */

		/* 
		 * We have a modem! 
		 * Pick up where we left off in the resource database, and get the 
		 * (IRQ, I/O Address) ...
		 */
		cm_args.cm_param = CM_IRQ;
		cm_args.cm_val = &irq;
		cm_args.cm_vallen = sizeof(irq);
		if ( cm_getval(&cm_args) ) {
			cmn_err(CE_NOTE, "!%s:cm_getval:CM_IRQ:%s:%x failed", CS_DRVPREFIX, SERIALDRIVERNAME, k);
			continue;
		}

		/* "s" is the socket which has the modem card in it */
		irqreq.LogicalSocket = s;
		irqreq.IRQInfo1 = (char) irq & 0xf; 
		irqreq.IRQInfo2 = 0;
		if ((status = CardServices( RequestIRQ, hp, (void *) NULL, sizeof(struct CSIrq), &irqreq)) != CS_SUCCESS) {
			cmn_err(CE_NOTE, "!%s:RequestIRQ failed:%x:%x", CS_DRVPREFIX, irq, status);
			continue;
		}

		
		ioreq.LogicalSocket = s;
		ioreq.Base1 = (ushort) ioaddr.startaddr;
		ioreq.NumPorts1 = (ioaddr.endaddr - ioaddr.startaddr) + 1;
		/* ioreq.Attributes1 = CS_IOPATH16; */
		ioreq.Attributes1 = 0;
		if (CardServices( RequestIO, hp, (void *) NULL, sizeof(struct CSIo), &ioreq) != CS_SUCCESS) {
			cmn_err(CE_NOTE, "!%s:RequestIO failed:%x-%x:%x", CS_DRVPREFIX, ioaddr.startaddr, ioaddr.endaddr, status);
			continue;
		}

		config.LogicalSocket = s;
		config.Attributes = CS_CONFIG_ENIRQ;
		config.Vcc = config.Vpp1 = config.Vpp2 = 0;
		config.IntType = CS_CONFIG_TYPE_MEMIO;
		config.ConfigBase = configreg;
		config.ConfigIndex = cfgentry;
		config.Status = CS_STATUS_MODEMAUDIO; /* Audio */
		config.Present = CS_CONFIG_OPTPRESENT | CS_CONFIG_STATPRESENT;
		if ((status = CardServices( RequestConfiguration, hp, (void *) NULL, sizeof(struct CSConfig), &config)) != CS_SUCCESS) {
			cmn_err(CE_NOTE, "!%s:RequestConfiguration failed:%x:%x", CS_DRVPREFIX, status, sizeof(struct CSConfig));
			continue;
		}
		cmn_err(CE_CONT, "%s modem in socket %d, enabled\n", prodinfo, s);
		s++; /* This socket has been looked at ... start looking next time at the next socket */
	}
	kmem_free(handle, cs_total_sockets * sizeof(handle_t) );
	return 0;
}
