#ident	"@(#)asyc.c	1.81"
#ident 	"$Header$"

/*                                                                            */
/*                                                                            */
/*      Copyright (c) 1991 Intel Corporation                                  */
/*      Copyright (c) 1997 SCO Ltd.                                           */
/*      All Rights Reserved                                                   */
/*      INTEL CORPORATION PROPRIETARY INFORMATION                             */
/*                                                                            */
/*                                                                            */
/*      This software is supplied to AT & T under the terms of a license      */
/*      agreement with Intel Corporation and may not be copied nor            */
/*      disclosed except in accordance with the terms of that agreement.      */
/*                                                                            */
/*                                                                            */

/*** Include Files ***/

#include <io/asy/asyc/asyc.h>                   /* Public interfaces    */
#include <io/asy/asyc/serial.h>                 /* Private definitions  */
#include <io/asy/iasy.h>
#include <io/conf.h>
#include <io/conssw.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termiox.h>
#include <io/termios.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/kdb/xdebug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>

#include <io/ddi.h>                     /* Must be last system include file  */

/*
 * MACROs/literal definitions 
 *      - Used to define ASYC_DEBUG on DEBUG flag set for kernel builds, BUT, 
 *        possibility of distributing the TOOLS (DEBUG set) build: don't set 
 *        ASYC_DEBUG on DEBUG until resolved.
 */

#ifdef MERGE386 
	#undef MERGE386 
#endif 

/*      #define ASYC_DEBUG                      1       */
#define ASYC_DEBUG_PROD         1

#ifdef ASYC_DEBUG 

        #ifndef ASYC_DEBUG_PROD 
                #define ASYC_DEBUG_PROD         1
        #endif

        #ifdef ASSERT
                #undef ASSERT
        #endif 


        #define ASSERT(EX) \
        ( (EX) || asyc_cmnerr ( #EX " (ASSERT) file:%s, line: %d" , \
                                                                                                __FILE__, __LINE__ ) )
        #ifdef STATIC  
                #undef STATIC
        #endif 

        #define STATIC  

#else 
        
        #ifdef ASSERT
                #undef ASSERT
        #endif 

        #define ASSERT(EX)

        #ifndef STATIC 
                #define STATIC static 
        #endif 

#endif  /* ASYC_DEBUG */

/*
 * Prototypes 
 */


/* 
 * Entry points (defined in Master(4) file
 */

void                            asyc_asyinit(void);
void                            asycintr(int);
void                            asycstart(void);

/* 
 * System console routines. 
 */

STATIC dev_t            asyccnopen(minor_t, boolean_t, const char *);
STATIC void                     asyccnclose(minor_t, boolean_t);
STATIC int                      asyccnputc(minor_t, int);
STATIC int                      asyccngetc(minor_t);
STATIC void                     asyccnsuspend(minor_t);
STATIC void                     asyccnresume(minor_t);

/* 
 * Private driver routines.
 */

STATIC void                     asyc_inchar(struct strtty *);
STATIC void                     asycpoll(void *);
STATIC int                      asycproc(struct strtty *, int);
STATIC void                     asyc_txsvc(struct strtty *); 
STATIC void                     asyc_deltaDCD(struct strtty *, uchar_t); 
STATIC int                      asycparam(struct strtty *);
STATIC void                     asycwakeup(struct strtty *);
STATIC void                     asyc_setspeed(struct strtty *, speed_t);
STATIC void                     asychwdep(queue_t *, mblk_t *, struct strtty *);
STATIC boolean_t        asyc_uartpresent(ioaddr_t io); 
STATIC uart_state_t     asyc_loopback(ioaddr_t io);
STATIC uart_type_t      asyc_uarttype(ioaddr_t iobase);
STATIC int                      asyc_setconsole(minor_t , const char * parms);  
STATIC int                      asyc_asytabinit(uint_t unit); 
STATIC int                      asyc_initporthw(uint_t unit); 
STATIC int                      asyc_setFIFO(uint_t unit);
STATIC int                      asyc_memalloc(void);
STATIC uint_t           asyc_chartime(speed_t baud); 
STATIC void                     asyc_loadtxbuffer(uint_t unit); 
STATIC int                      asyc_parsebs(const char *bs, struct strtty *tp );
STATIC int                      asyc_atoi(char *);
STATIC boolean_t        asyc_clocal ( struct strtty *tp ); 
STATIC boolean_t        asyc_chktunevars ( void ); 
STATIC void                     asyc_settxtout ( asyc_t *, speed_t );
STATIC int                      asyc_ctime(struct strtty *tp,int ccnt); 
STATIC void                     asyc_idle(struct strtty *tp); 
STATIC void                     asyc_idle_wrapper(struct strtty *tp); 
STATIC void                     asyc_schedule_idle(asyc_t *asyc); 
STATIC char     *               asyc_getUARTdevname(uart_type_t); 

/*
 * Following debug routines are left in distribution code for beta 
 * as they can be used to provide useful debugging data (calling 
 * with kdb). They are all autonomous routines (not called by any 
 * other routine) : they consume a few hundred bytes of kernel text.
 */

#ifdef ASYC_DEBUG_PROD 

void                            asyc_dump ( uint_t unit);                       /* strtty data          */
void                            asyc_stats ( uint_t unit );             /* Driver stats         */
void                            asyc_fcstate ( uint_t unit);            /* Flow control         */
void                            asyc_asyc (uint_t unit);                        /* UART/HW setup        */
void                            asyc_scan (uint_t unit);                        /* uart_scantbl         */
void                            asyc_uart (uint_t unit);                        /* Baud etc                     */

#endif 

#ifdef ASYC_DEBUG 

STATIC int                      asyc_cmnerr (const char *, char *, int ); /* assert */

#endif

/* 
 * Externally defined (high baud rate support) functions.
 * NB.  (1) Include the io/tcspeed.c module in asyc.o link for UW2.1 port.
 *              (2) TCGET ioctl(2) result undefined for speeds > 38400 (no space)
 */

extern speed_t  tpgetspeed(tcstype_t, const struct strtty *);
extern void     tpsetspeed(tcstype_t,struct strtty *,speed_t); 

/* 
 * Data declarations. 
 */ 

/* 
 * Defined in iasy space.c file. NB. Shared between ldterm, iasy, asyc. 
 */ 

extern struct termiox   asy_xtty[];     /* Extended (HW flow control)   */
extern struct strtty    asy_tty[];      /* Standard STREAMS tty         */

/* 
 * OS required (unset D_MP indicates execution on base CPU only).
 */

int     asycdevflag     = 0;                            /* Holds driver attributes              */

/* 
 * space.c tunables ( documented in space.c )
 * 
 * asyc_sminor  -       starting minor number 
 * asyc_nscdevs -       uart_scantbl[] entries to scan (0 - (asyc_nscdevs -1)).
 * uart_scantbl -       Initialisation parameters for UARTs.
 * asyc_oblowat -       transmit buffer get data level(%).
 * asyc_iblowat -       receive buffer disable flow control level (%).
 * asyc_obhiwat -       transmit buffer stop getting data level(%).
 * asyc_ibhiwat -       receive buffer enable flow control level (%).
 * asyc_pollfreq -      Period between calls of 2ary input data processing. 
 * asyc_tiocm   -       cause TIOCM* ioctls to disable HW flow control.
 *
 */

extern uint_t           asyc_sminor;                            /* Starting minor number        */
extern uint_t           asyc_nscdevs;                           /* Scan table entries           */
extern uart_data_t      uart_scantbl[];                         /* UART scan data                       */ 
extern uint_t           asyc_iblowat;                           /* Start flow control           */
extern uint_t           asyc_ibhiwat;                           /* Stop flow control            */
extern uint_t           asyc_oblowat;                           /* Tx Buffer fill start         */
extern uint_t           asyc_obhiwat;                           /* Tx buffer stop filling       */
extern ulong_t          asyc_pollfreq;                          /* Poll requency in usec        */
extern int                      asyc_debug;                             /* Various DEBUG modes          */
extern int                      asyc_rldcnt;                            /* Buffer fill limiting         */
extern int                      asyc_use_itimeout;                      /* Use itimeout(D3) call        */
 
/* 
 * Private data 
 */ 

STATIC boolean_t        asyinitd = B_FALSE;     /* init(D2) routine ran         */
STATIC boolean_t        consinit = B_TRUE;              /* console_init() time          */
STATIC conspec_t        cons[2];                                /* Console setup                        */
STATIC asyc_t           *asyctab = 0;                   /* Ptr to asyc structs          */
STATIC uint_t           asyncfg = 0;                    /* Count of dcu(1M) ports       */
STATIC asycbuf_t        *asyc_bufp = 0;                 /* Ptr to buffer struct         */
STATIC astats_t         *asyc_statp     = 0;            /* Statistics table pointer     */
STATIC int                      asyc_id = 0;                    /* set for sminor != 0          */
STATIC long                     asyc_pollticks = 0;             /* ticks per poll invocation*/

/* 
 * Console switch table : export to iasy_hw[] via iasy_register()
 * Defined for all console devices. 
 */

struct conssw asycconssw = {
        asyccnopen,
        asyccnclose,
        asyccnputc,                                     /* Return 1 if OK, 0 if BUSY            */
        asyccngetc,                                     /* Return -1 if no data, else char      */
        asyccnsuspend,                          /* Called prior to getc                         */
        asyccnresume                            /* Called after end of getc calls       */
};

/* 
 * Baud rate / Clock divisor table. 
 * This maps baud rates to equivalent UART clock divisors. Each entry 
 * defines a Baud Rate and the required UART clock divisor; * the 
 * divisor(D) is calculated as (Clock(Hz) / ( 16 * Baud)); it allows 
 * adjustment by resetting the clock rate (via space.c).
 */

STATIC line_speed_t baud_divs[] = { 
        MKBD(115200), MKBD(57600), MKBD(38400), MKBD(19200), MKBD(9600), 
        MKBD(4800), MKBD(2400), MKBD(1200), MKBD(600), MKBD(300), 
        MKBD(150), MKBD(110)
}; 

#define NBAUD   (sizeof(baud_divs)/(sizeof(line_speed_t))) 

/*
 * Function definitions 
 */

/*
 * void
 * asyc_asyinit(void)
 *
 * Calling/Exit State:
 *      Called at init(D2) time for non serial console/kdb console. 
 *      If console used (boot(4) | kernel(4)) asyc_asyinit() called by
 *      asyccnopen() early in sysinit() (No canonical DKI list available).
 *      If kdb console, and kdb.rc(4) invokes at startup, called after 
 *      init_console(), still prior to init(D2). Else, called at init(D2)
 *      time. 
 *      Interrupts and timeouts are disabled at this point. 
 * 
 * Description:
 *      Use space.c uart_scantbl[] to search for UART hardware. Set 
 *      uart_scantbl[] to devices found. Register scanned for devices 
 *      whether hardware present or not; allows console parameter string 
 *      override to use console on pathologically configured systems.
 *  Once all uart_scantbl[] entries scanned, if there are some empty 
 *      entries (status: ABSENT) copy upper entries over them. It should 
 *  improve DCU match after ISL (missimg devices (eg. No COM2) will 
 *      be deleted from DCU, and 'extra' HW (eg. COM3) will be next 
 *  resmgr device present (unless >1 extra resmgr line allows the 
 *      'new' entries in a different order (eg. COM4, COM3).
 *  Since only missing entries overwritten (which wont match any DCU) 
 *      if the DCU matches the original data  we can always copy the 
 *  shifted down entries back (eg. if uart_scantbl[2] matches DCU[3])
 *      If any of this prestidigitation occurs cmn_err() the user.
 *
 */

void
asyc_asyinit(void)
{
        int                                     j ; 
        ioaddr_t                        io; 
        uart_data_t                     *up ; 
        minor_t                         rmin ; 

#ifdef ASYC_DEBUG 
        if (asyc_debug & DSP_FUNCS)
                cmn_err(CE_CONT,"!asyc_asyinit:");
#endif 


        if (asyinitd)                   /* Already initialised */ 
                return;
        
        /* 
         * Search for UART device hardware, according to tunable data 
         * parameters. If found, save device information for later 
         * inheritance by STREAMS driver data, and in case of console 
         * assignment. 
         */ 

        for ( j = 0, up = uart_scantbl; j < asyc_nscdevs; j++, up++ ) { 

                /* Get the current minor */

                int             mc = asyc_sminor + (4 * j) ; 

                /* 
                 * Set asy_tty[] termio[sx] data up from 
                 * uart_scantbl[] for scanned devices only.
                 */ 

                if ( up->termp ) { 
                        asy_tty[j].t_iflag = up->termp->c_iflag ; 
                        asy_tty[j].t_oflag = up->termp->c_oflag ; 
                        asy_tty[j].t_cflag = up->termp->c_cflag ; 
                        asy_tty[j].t_lflag = up->termp->c_lflag ; 
                } 
                        
                /* Only x_hflag holds useful termiox data */

                if ( up->termxp ) { 
                        asy_xtty[j].x_hflag = up->termxp->x_hflag ; 
                } 

                io = up->iobase;

                /* 
                 * Perform tests in same order as the previous driver 
                 * which had no known compatibility problems. Do ICR R/W
                 * then setup loopback mode and verify OP->IP line echo. 
                 */

                if (!asyc_uartpresent(io)) { 
#ifdef ASYC_DEBUG
                        /* 
                         * NB. Trying to display via the console cmn_err(D3), 
                         * errors in console initialisation causes PANICs.
                         * Hence consinit : dont cmn_err at console init time.
                         */

                        if (!consinit)
                                /* 
                                 *+ UART ICR failed to read written data. 
                                 */
                                cmn_err(CE_NOTE,"!UART not present at %x",io); 
#endif
                        up->status = ABSENT ; 
                        continue; 
                } 
                
                /*
                 * loopback test performs old OP->IP line test then 
                 * verifies data path is reliable. Returns either 
                 * INIT/HWERR/NOSTATE for good/bad/absent hardware.
                 */
                
                if ((up->status = asyc_loopback(io)) != INIT) { 
#ifdef ASYC_DEBUG
                        if (!consinit)
                                /* 
                                 *+ Bad (error in testing) or missing hardware.
                                 */ 
                                cmn_err(CE_NOTE,"!UART loopback error at %x",io);
#endif
                        up->status = HWERR; 
                        continue; 
                } 
                
                up->uart = asyc_uarttype(io); 


                /* 
                 * Register devices found with iasy. Creates a mapping between
                 * minor number and IO address; iasy routes conssw_t calls via
                 * iasy_hw[]. If DCU/resmgr data differs from our initial guess
                 * devices de & reregistered.
                 * OS boot loader uses COM1..4 for bootstrings, if DCU etc. 
                 * differs (so the original bootstring was to the wrong device 
                 * make a translation table to route later console requests
                 * to the boot COM port whatever the OS minor device is. This 
                 * means the console will always continue to work (it could 
                 * have changed identity after asycstart() ran. Inform the user
                 * to let them configure (via changing space.c/uart_scantbl
                 * or DCU) the system properly later.
                 */

                rmin = iasy_register ( mc, 1, asycproc, asychwdep, &asycconssw); 

                /* 
                 * Successful iasy_register() calls return the requested minor 
                 * (mc = current minor) or -1. 
                 */ 

                if (rmin != mc) { 
#ifdef ASYC_DEBUG
                        if (!consinit)
                                /* 
                                 *+ iasy-register() failed: return value != asyc_sminor 
                                 */ 
                                cmn_err(CE_NOTE,"!asyinit: iasy_register failed"); 
#endif
                        asyinitd = B_TRUE;
                        continue;               
                } 
        } 

        /* 
         * asyc_id is used throughout the code: outght to be same as 
         * asyc_sminor. 
         */ 
        
        asyc_id = asyc_sminor; 
        asyinitd = B_TRUE;
        return;
        
}

/*
 * void 
 * asycstart(void)
 *
 * Calling/Exit State:
 *      Serial port interrupt handler not installed at entry. At 
 *      call to cm_intr_attach() if there is an interrupt pending 
 *  the routine is called. Block interrupt calls until the rest
 *      of the driver has been initialised.
 *
 * Remarks:
 *      This routine does standard init(D2) tasks because asycinit may be 
 *      called prior to DKI available. Gets DCU data and sets any guessed
 *      setup values to those set in the DCU. 
 *
 */

void 
asycstart(void)
{       
        int                                     i, u, scanidx, ready = 0 ;
        asyc_t                          *ap;
        ioaddr_t                        io; 
        struct cm_addr_rng      asyc_ioaddr;
        cm_args_t                       cm_args;        
        uart_data_t                     *up; 
        static boolean_t        running = B_FALSE ;
        void                            **ivp ; 
        long                            ticks ; 

#ifdef ASYC_DEBUG 
        if (asyc_debug & DSP_FUNCS)
                cmn_err(CE_NOTE,"asycstart"); 
#endif 

        /* 
         * Check if initialised through the config manager database 
         */
        
        if (running)
                return;

        /*
         * Call routine to verify the tunable data.
         */

        if (asyc_chktunevars()) { 
                cmn_err(CE_WARN,"asyc_chktunevars: Found bad tunable & reset");
        }

        /* 
         * Get the DCU driver configuration. These entries define
         * the IO address/device mapping. Each device uses 4 minors. 
         */ 

        if (( asyncfg = cm_getnbrd("asyc")) == 0 ) {    

                /* No boards present    */ 

#ifdef ASYC_DEBUG 
                if (asyc_debug & DSP_DCU) 
                        cmn_err (CE_NOTE,"!asycstart: No asyc devices configured"); 
#endif 

                return; 
        } 

        /* 
         * Alloc memory for the ports in dcu(1M). This uses kmem_alloc()
         * and allocates asyc_t, astats_t & asycbuf_t for each port.
         */

        if ( asyc_memalloc() != 0) {
                cmn_err(CE_WARN,"asycstart:kmem_alloc(%d) failed",asyncfg);
                running = B_TRUE; 
                return;
        }

        /* 
         * Loop through the DCU ports, filling in the data in asyctab[].
         * For each port that was found in the initial scan, inherit 
         * the CONDEV : if a console device dont reset the UART.
         */ 

        for (u = 0, ap = asyctab; u < asyncfg; u++, ap++ ) {

                scanidx = -1;

                /* 
                 * Get board key from Config Manager. On error, just goto
                 * next device. Data will be clear as for unused device.
                 */

                if((cm_args.cm_key = cm_getbrdkey("asyc",u)) == RM_NULL_KEY) {
                        /* 
                         *+ Couldn;t get board ID key:port unusable.
                         */
                        cmn_err(CE_NOTE,"asycstart:Get board %d key error", u);
                        continue;
                }

                cm_args.cm_n = 0;

                /* 
                 * Get IO range of board and determine if the uart_scantbl[]
                 * contains a valid entry for this HW instance.
                 */

                cm_args.cm_param = CM_IOADDR;
                cm_args.cm_val = &(asyc_ioaddr);
                cm_args.cm_vallen = sizeof(asyc_ioaddr);
                
                if ( cm_getval(&cm_args) ) {
                        /* 
                         *+ Error occured in getting IO base: port unusable
                         */
                        cmn_err(CE_NOTE,"asycstart:CM Get IO err, unit: %d ",u);
                        continue;
                }
                
                /* 
                 * Store IO base address in DAT field.  
                 */

                io = ap->asyc_dat = asyc_ioaddr.startaddr;

                if ( uart_scantbl[u].iobase != io ) { 

                        /*
                         *+ DCU(1M) & asyc(7) disagree on device minor numbering.
                         *+ ie. boot console minor != Unix console minor. 
                         *+ Requires uart_scantbl(asyc/space.c) or DCU change.
                         *  L002 - made send to osmlog not screen.
                         */

                        cmn_err(CE_WARN, 
                                        "!asycstart:Unit %d IO mismatch DCU: %x asyc: %x", 
                                                u, ap->asyc_dat, uart_scantbl[u].iobase);
                } 

                /*
                 * Get resmgr(1M) HW IRQ line assigned.
                 */

                cm_args.cm_param = CM_IRQ;
                cm_args.cm_val = &(ap->asyc_vect);
                cm_args.cm_vallen = sizeof(ap->asyc_vect);

                if ( cm_getval( &cm_args ) ) {  
                        /* 
                         *+ Error getting IRQ 
                         */
                        cmn_err(CE_NOTE, "asycstart:Get board %d IRQ error",u);
                        ap->asyc_vect = 0;       
                        continue;
                } 

                /* 
                 * Using the IO base address, determine if this is a device 
                 * that has been scanned and possibly designated as a console
                 * device, by init(D2).
                 */ 

                for ( up = uart_scantbl, i = 0; i < asyc_nscdevs; i++, up++){ 
                        if (up->iobase == ap->asyc_dat) { 
                                scanidx = up - uart_scantbl; 
                                break; 
                        } 
                } 

                if (scanidx < 0) { 

                        if (!asyc_uartpresent(ap->asyc_dat)) { 
#ifdef ASYC_DEBUG
                                /* 
                                 * a DCU(1M) defined device has failed the presence
                                 * test (could be at ISL so no cmn_err)
                                 */
                                cmn_err(CE_WARN,
                                        "!asycstart:HW error UART io %x",ap->asyc_dat); 
#endif
                                continue; 
                        } 

                        if ( asyc_loopback(ap->asyc_dat) != INIT) { 
                                /* 
                                *+ Broken hardware for dcu(1M) entry. 
                                */ 
                                cmn_err(CE_WARN,"asycstart:HW error UART io %x",
                                                                                                                ap->asyc_dat); 
                                break; 
                        } 

                        ap->asyc_utyp = asyc_uarttype (ap->asyc_dat); 
        
                        /* 
                         * For purposes of support device identification, send a 
                         * UART description message for each UART found to the osmlog.
                         */

                        cmn_err(CE_NOTE, 
                                "!Found %s UART at IO 0x%x IRQ %d", 
                        asyc_getUARTdevname(ap->asyc_utyp),ap->asyc_dat,ap->asyc_vect);
                
                } else { 

                        /* 
                         * Initialisation for devices in the scan table.
                         */

                        if (up->status & (ABSENT|HWERR)) {
                                /* 
                                 *+ hardware not detected by console setup
                                 */
                                cmn_err(CE_WARN, "!UART at io %x absent/broken",io); 
                                continue; 
                        } 

                        /* 
                         * Inherit any data saved by the scanning 
                         * eg. UART type, etc. Only works for terminals 
                         * on kdb that are set by newterm at kdb.rc time.
                         */ 

                        if (up->status & (SYSCON|KDBCON)) 
                                ap->flags |= CONDEV; 

                        if (up->uart)
                                ap->asyc_utyp = up->uart; 
                        else 
                                cmn_err(CE_WARN,"asycstart: UART type unset IO 0x%x",
                                                        ap->asyc_dat);  

                        cmn_err(CE_NOTE, 
                                "!Found %s UART at IO 0x%x IRQ %d", 
                        asyc_getUARTdevname(ap->asyc_utyp),ap->asyc_dat,ap->asyc_vect);

                } /* Scanned init */

                /* 
                 * Init port fixed data (Eg. IO, IRQ, asycbuf_t addr ).
                 */

                if ( asyc_asytabinit(u) < 0 )  { 
                        cmn_err(CE_WARN,"asyc_asytabinit error IO %x",io);
                        continue; 
                } 

                /* 
                 * Initialise (non console) UART HW to quiescent state. 
                 * (Ensure that the device does not interrupt). Ideally 
                 * set back to quiescent state after power on. It may have
                 * been set to an active state by the BIOS. Only do this 
                 * if we are not using it as a console - in which case disturb 
                 * it as little as possible. 
                 */

                if ( !(ap->flags & CONDEV)) { 
                        if (asyc_initporthw(u) < 0 ) {
                                cmn_err(CE_WARN,"asycstart: asyc_initporthw error IO %x",io);
                                continue; 
                        }
                } 

                /* 
                 * Initialise magic cookie and install interrupt handler,
                 * n.b. this will usually cause immediate call to asycintr
                 * if a UART pending IRQ exists (eg. modem status).
                 */

                ivp = (void *) &ap->asyc_irqmagic; 

                if ( !cm_intr_attach( cm_args.cm_key, asycintr, 
                                                                                &asycdevflag, ivp )){

                        /* 
                        *+ No interrupt: flag off the device not present
                        *+ (cannot be used as terminal device - system 
                        *+ console or serial debugger use OK.)
                        */

                        up->status |= RSRCERR ; 
            cmn_err (CE_WARN,
                                        "asycstart:cm_intr_attach failed, release IRQ");
                        cm_intr_detach ( ivp );
                        continue; 
                }

                /* 
                 * For terminals that are defined as console/debugger terminals 
                 * enable the receive interrupt to allow the special characters
                 * to be processed: asycintr discards when it finds !ISOPEN. 
                 */

                if (ap->flags & CONDEV) 
                        outb(ap->asyc_icr,ICR_RIEN); 

                /* 
                 * IO/IRQ allocated OK 
                 * Bump success count i
                 */
        
                ++ready; 

        } /* For all boards in DCU */

#ifdef ASYC_DEBUG
        cmn_err(CE_NOTE,"!asycstart: %d ports found / %d in DCU",ready,asyncfg); 
#endif

        /* 
         * De-register and register terminal server only if mis-match 
         * and there are some ports to be used (asyc_cfg != 0)
         */

        if (asyncfg && (asyc_nscdevs != asyncfg)) {

                /* 
                 * De-register the old terminal server 
                 */

#ifdef ASYC_DEBUG
                /*
                 *+ Mismatch in ports defined in DCU and space.c
                 */
                cmn_err(CE_WARN,"!asycstart: CM ports %d != ports %d", 
                                                asyncfg, asyc_nscdevs);  
#endif

                if (iasy_deregister(asyc_sminor,asyc_nscdevs) < 0) {
                        /*
                        *+ Fatal error: cant deregister old ports 
                        *+ due to config mismatch. All ports are unusable. 
                        */
                        cmn_err(CE_WARN, "!asycstart: Cant deregister ports");
                        running = B_TRUE; 
                        return; 
                }

                /*  
                 * Register terminal server. 
                 */
                asyc_id = iasy_register(asyc_sminor, asyncfg, 
                                                        asycproc, asychwdep, &asycconssw);
        }

        /* 
         * Allow any non zero pollfreq: this sets the response to 
         * serial input for STREAMS data. If char echo seems delayed 
         * this could be the cause. It also adds a callout() overhead 
         * to the system. (add callout() boolean_t gate to wishlist).
         */


        ticks = drv_usectohz(asyc_pollfreq ? asyc_pollfreq : 20000); 
        asyc_pollticks = ticks;         /* L999 */
        ASSERT(asyc_pollticks > 0); /* L999     */
        ticks |= TO_PERIODIC; 

        if (ready) {
                if ( ! itimeout ( asycpoll, (void *) 0, ticks, plstr )) { 
                        /* 
                         *+ There is no slot available to drive the asycpoll()
                         *+ serial port interrupt handling support functions. 
                         *+ Ports can only be console/debug polled devices.
                         */ 

                        cmn_err(CE_WARN, "asycstart: No timeout: all ports disabled");

                        /* 
                         * Detach interrupts, clear all data. 
                         */

                        for (u = 0; u < asyncfg; u++ ) { 
                                
                                cm_intr_detach((void *) &asyctab[u].asyc_irqmagic); 
                                uart_scantbl[u].status = RSRCERR ; 
                                bzero ((void *) &asyctab[u], sizeof(asyc_t));

                        }               
                } 
        } 

        running = B_TRUE; 

        return;
}

/*
 * void
 * asycintr(int) 
 *      process device interrupt
 *
 * Calling/Exit State:
 *      Called at SPL 9, cannot be blocked by splhi(D3). UP runs on base CPU.
 *      NB: Cannot invoke possibly interrupted (inconsistent) STREAMS calls.
 *
 * Description:
 *      Service UART IRQ : save input data/errors for processing in asycpoll.
 *      Service TxRDY interrupts: schedule idle timeout/reload TxFIFO.
 *      Service Modem status changes : update flow control & carrier status.
 */

void
asycintr(int vect)
{       
        register struct strtty  *tp;    
        uint_t                                  unit;
        register asyc_t                 *asyc;
        register asycbuf_t              *ap;
        register astats_t               *sp; 
        register ushort_t               c;              
        uchar_t                                 irqid, lsr, msr, mcr; 
        register int                    fifo_charcount;
        int                                             proc_irq = 0; 
        struct termiox                  *xtp; 

        for (asyc = &asyctab[0], unit = 0; unit < asyncfg; asyc++, unit++) {
        
                if ( asyc->asyc_vect != vect )
                        continue;

#ifdef MERGE386
                /*
                 * needed for com port attachment 
                 */
                if ( asyc->mrg_data.com_ppi_func && 
                   (*asyc->mrg_data.com_ppi_func)(&asyc->mrg_data, -1)) 
                        return;

#endif /* MERGE386 */

                tp = IASY_UNIT_TO_TP(asyc_id, unit);
                ap = asyctab[unit].asyc_bp;
                sp = asyctab[unit].asyc_stats; 
                xtp = IASY_TP_TO_XTP(tp);

                for (;;) {

                        irqid = inb(asyc->asyc_isr) & 0x0f; 

                        /*  
                         * Only log the first interrupt 
                         */

                        if (irqid & ISR_NOINTRPEND) { 
                                if (!proc_irq++)
                                        sp->nointrpending++; 
                                break;
                        } 

#ifdef ASYC_ENABLE_FASTPATH
{
                        /* 
                         * If RxFIFO > trigger level and FASTPATH receive is enabled, 
                         * ie. no need to examine each byte: no SW flow control, not a
                         * console device, input buffer space plentiful.
                         */

                        if ((irqid == ISR_RxRDY) && (asyc->flags & FASTPATH)) {
                                        
                                int                             i;
                                ushort_t                *dp; 

#ifdef ASYC_DEBUG 
                                uchar_t                 lsr;

                                /* 
                                 * If there were errors the IRQ would be an RSTATUS 
                                 */

                                GET_LSR(lsr);
                                ASSERT(!(lsr & LSR_RFBE)); 
#endif 
                                sp->rxdataisr++; 

                                if (ap->ibuf1_active) {
                                        dp = &ap->ibuf1[ap->iput1] ; 
                                        ap->iput1 += asyc->rxfifo; 
                                } else { 
                                        dp = &ap->ibuf2[ap->iput2]; 
                                        ap->iput2 += asyc->rxfifo; 
                                }

                                /*
                                For DEBUG stop at penultimate character and check there
                                is a final character and there aren't any errors. 
                                Also, test for buffer limits not exceeded,      
                                */

                                for ( i = 0; i < asyc->rxfifo; i++ )    
                                        *dp++ = (inb(asyc->asyc_dat) & 0xFF);


                                drv_setparm(SYSRINT, 1); 
                                sp->rawrxchars += asyc->rxfifo; 
                                continue; 

                        } 
                        else 

}
#endif  /* ASYC_ENABLE_FASTPATH         */

                        /* 
                         * Normal character at a time receive processing. 
                         */

                        if ((irqid == ISR_FFTMOUT)||(irqid == ISR_RSTATUS)||
                                (irqid == ISR_RxRDY)) {
                
                                sp->rxdataisr++; 
                                fifo_charcount = 0;
                                drv_setparm(SYSRINT, 1); 

                                /* 
                                 * Continue reading the FIFO until none available.
                                 */

                                while ((lsr = inb(asyc->asyc_lsr)) & LSR_RCA) {

                                        c = inb(asyc->asyc_dat) & 0xff;
                                        sp->rawrxchars++; 
                                        fifo_charcount++;

                                        /* 
                                         * If assigned as system or debugger console process 
                                         * data input for 'special' characters: invoke debugger,
                                         * reboot/PANIC/shutdown system.
                                         */

                                        if (asyc->flags & CONDEV) { 
                                                switch (c) {
#ifndef NODEBUGGER
                                                        /* 
                                                         * Security causes kdb invocation failure 
                                                         * if kernel tunable kdb_security unset.
                                                         */
                                                        case DEBUGGER_CHAR:
                                                                sp->spclrxchars++; 
                                                                (*cdebugger)(DR_USER, NO_FRAME);
                                                                continue; 
#endif 
                                case REBOOT_CHAR:
                                                                if (console_security & CONS_REBOOT_OK){
                                                                        sp->spclrxchars++; 
                                                                        drv_shutdown(SD_SOFT, AD_BOOT);
                                                                        continue; 
                                                                } 
                                                                break; 

                                                        case PANIC_CHAR:
                                                                if (console_security & CONS_PANIC_OK){
                                                                        sp->spclrxchars++; 
                                                                        drv_shutdown(SD_PANIC,AD_QUERY);
                                                                        continue; 
                                                                } 
                                                                break;
                                        
                                                        default:
                                                                break;
                                                } 
                                        } 

                                        /* 
                                         * If !CREAD or !ISOPEN discard data.
                                         */ 

                                        if (!(tp->t_cflag & CREAD) || !(tp->t_state & ISOPEN)) { 
                                                sp->droprxcread++; 
                                                continue; 
                                        } 

                                        /* 
                                         * LSR holds parity/frame/overrun errors, and BREAK event
                                         * detection, and XHRE/XSRE flags.
                                         */

                                        c |= (ushort_t) (lsr << 8); 

                                        /* 
                                         * Test the S/W flow control and if set test the 
                                         * input character against the VSTART/VSTOP, as long
                                         * as the VSTART/VSTOP are _not_ _POSIX_VDISABLE (0);
                                         * if so their function is disabled, _but_ the character 
                                         * is still discarded.
                                         */

                                        if ( tp->t_iflag & IXON )  {

                                                uchar_t         cb = c & 0xFF;  
                                
                                                /* 
                                                 * VSTART/VSTOP special character functions can be 
                                                 * disabled by setting the characters (in t_cc[])
                                                 * to ASCII NUL. If c is NUL, don't bother testing
                                                 * since a match is ignored anyway. 
                                                 */

                                                if (cb == tp->t_cc[VSTOP] && cb != _POSIX_VDISABLE) {

                                                        sp->rcvdvstop++; 
                                                        asyc->flags |= OBLK_SWFC; 
                                                        continue;

                                                } 
                                                
                                                if (cb == tp->t_cc[VSTART] && cb != _POSIX_VDISABLE ) {
                                                
                                                        sp->rcvdvstart++; 
                                                        asyc->flags &= ~OBLK_SWFC; 
                                                        continue;       
                                                }

                                                /* 
                                                 * IXANY allows any character to restart, (but still
                                                 * filter VSTART characters from data. 
                                                 */

                                                if ( tp->t_iflag & IXANY ) {

                                                        sp->rcvdixany++; 
                                                        asyc->flags &= ~OBLK_SWFC; 

                                                } 
                                        } 
                        
                                                
                                        /* 
                                         * Save the data in the active ISR buffer.
                                         */

                                        sp->rxrealdata++; 

                                        if ( ap->ibuf1_active ) { 
                                                
                                                ASSERT(!ap->iget1); 
                                                ASSERT(ap->iput1 < ISIZE); 

                                                if (ap->iput1 == (ISIZE - 1)) { 
                                                        sp->softrxovrn1++;
                                                } else { 
                                                        ap->ibuf1[ap->iput1++] = c; 
                                                } 

                                        } else {        

                                                ASSERT(!ap->iget2); 
                                                ASSERT(ap->iput2 < ISIZE); 

                                                if (ap->iput2 == (ISIZE - 1)) { 
                                                        sp->softrxovrn2++;
                                                } else {
                                                        ap->ibuf2[ap->iput2++] = c; 
                                                }

                                        } 

                                        /* 
                                         * Do we need to turn input flow control off, 
                                         * ie. request remote quench ? If we have less 
                                         * free space than low water and we aren't ISTOPped
                                         * already.
                                         */     

                                        if ((IBUFFREE(ap) < asyc_iblowat) && 
                                                                        !(asyc->flags & ISTOP)) {       /* L002 */      

                                                /*
                                                 * Verify hardware flow control status.
                                                 */

                                                if (xtp->x_hflag & (RTSXOFF|DTRXOFF)) {

                                                        mcr = inb(asyc->asyc_mcr);      
#ifdef ASYC_DEBUG 
                                                        if (asyc_debug & DSP_FLWCTL)
                                                                cmn_err(CE_NOTE,"asycintr: msr 0x%x", msr); 
#endif 

                                                        if (xtp->x_hflag & RTSXOFF)
                                                                mcr &= ~MCR_RTS; 
                                                        if (xtp->x_hflag & DTRXOFF)
                                                                mcr &= ~MCR_DTR; 
                                                        outb(asyc->asyc_mcr, mcr); 
                                                        asyc->flags |= ISTOP;
#ifdef ASYC_DEBUG                       
                                                } else { 
                                                        mcr = inb(asyc->asyc_mcr);      
                                                        ASSERT(mcr & MCR_RTS);
                                                        ASSERT((mcr & MCR_DTR) || (tp->t_cflag & CLOCAL));
#endif 
                                                }

                                                /*
                                                 * SW flow control, if we are transmitting set a flag 
                                                 * to send on next Tx FIFO fill. If Tx empty send a 
                                                 * VSTOP character. 
                                                 */

                                                if (tp->t_iflag & IXOFF) { 
                                                        asyc->flags &= ~IGOPEND; 
                                                        asyc->flags |= ISTOPPEND; 
                                                        if ( !TXBUSY(tp) ) 
                                                                asyc_txsvc(tp);
                                                } 

                                                /* 
                                                 * End of per character processing.
                                                 */

                                        }       /*L002*/
        
                                }       

                                /* 
                                 * End of input character processing. Get 
                                 * another character , if there are any.
                                 */

                                continue; 

                        } else if (irqid == ISR_TxRDY) { 

                                if (!(tp->t_state & ISOPEN))
                                                continue; 

                                drv_setparm(SYSXINT, 1);
                                sp->txdatasvcreq++; 

                                /* 
                                 * BUSY means that the timeout has not yet fired, 
                                 * so if the state is correct (XHRE && !XSRE) clear 
                                 * timeout and decide whether to schedule idle or 
                                 * restart Tx chain. 
                                 */

                                if (tp->t_state & BUSY) {

                                        ASSERT(ap->txtout >= 0);
                                
                                        GET_LSR(lsr); 

                                        if (lsr & LSR_XHRE) {
                        
                                        sp->txdatasvcreq++; 

                                        /* 
                                         * Whether late or on time, clear the timeout.
                                         */ 

                                                ap->txtout = -1; 

                                                /* 
                                                 * Check if tasks requiring idle transmitter are
                                                 * pending. 
                                                 */

                                                if (asyc->flags & IDLETASK) {

                                                        /* 
                                                         * Call through the Tx timeout at the next 
                                                         * poll invocation.
                                                         */ 
                                                        if (lsr & LSR_XSRE) 
                                                                ap->txtout = 0; 
                                                        else
                                                                asyc_schedule_idle(asyc); 

                                                        continue;
                                                } 

                                                /* 
                                                 * No special tasks, so call the standard Tx chain 
                                                 * handler (sends data/flow control characters). 
                                                 */

                                                asyc_txsvc(tp); 
                                        }
                                        
                                        ++sp->badtxisrcalls;
                                }

                                /* 
                                 * End of TxRDY IRQ processing.
                                 */

                        } else if (irqid == ISR_MSTATUS)  {

                                sp->isrmodint++; 
                                msr = inb(asyc->asyc_msr);                      

                                /* 
                                 * Update modem control state.
                                 */

                                asyc_deltaDCD(tp, msr); 

                                /* 
                                 * Update hardware flow control state in local flags.
                                 * Test all 3 possible HW flow inputs: CTS,DSR,DCD. 
                                 * The next call to asyc_txsvc() will verify the states. 
                                 */

                                if (((xtp->x_hflag & CTSXON) && !(msr & MSR_CTS)) ||
                                        ((xtp->x_hflag & CDXON) && !(msr & MSR_DCD)))
                                        asyc->flags |= OBLK_HWFC; 
                                else 
                                        asyc->flags &= ~OBLK_HWFC; 

                        }

                } /* continue processing this UART */

        } /* processed all UARTs */

} /* ISR ends */

/* 
 * STATIC void 
 * asyc_deltaDCD(struct strtty *, uchar_t )
 *
 * Calling/Exit State:
 *      Called in txsvc() and asycintr() to update the carrier state.
 *      Sets tp->t_state CARR_ON flag. Needs to run at CLI/STI for this. 
 *      Send an M_HANGUP upstream, and flush queues if DCD drops, indicating 
 *      no more data. 
 * 
 */

STATIC void 
asyc_deltaDCD(struct strtty *tp, uchar_t msr)
{ 
        asyc_t          *asyc; 

        /* 
         * Process the DCD/CARR_ON/CLOCAL states.
         * asyc_clocal() returns effective CLOCAL state, ie. TRUE if CLOCAL 
         * set, false if not, except if in SCO compatibility mode, and not 
         * in WOPEN state : SCO mode ignores DCD dropping after open(2) 
         * has succeeded (incorrectly, but who's counting).
         */

        if ( !asyc_clocal(tp) ) {

                if ( msr & MSR_DCD ) { 
                                
                        /* 
                         * DCD on and WOPEN state => waiting for carrier, so set it
                         * and send signal. 
                         */

                        tp->t_state |= CARR_ON; 
                        if ( tp->t_state & WOPEN ) 
                                iasy_carrier(tp); 

                } else if ( tp->t_state & CARR_ON ) { 

                        /* 
                         * Carrier already detected, but DCD lost.
                         * DCD off and CARR_ON and !CLOCAL => M_HANGUP
                         */

                        tp->t_state &= ~CARR_ON; 

                        /* 
                         * Don't schedule the SIGHUP if we are already closed.
                         */

                        if (!(tp->t_state & (WOPEN|ISOPEN)))
                                return; 
                        
                        /*
                         * Check were not already scheduling the event, but havent
                         * had the poll service yet. If so, exit. 
                         */

                        asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id, tp)];

                        if (asyc->flags & DOSHUPTOUT) {
                                if (asyc->sighup_tp == 0) {
                                        asyc->sighup_tp = tp;
                                } 
                                return;
                        }

                        /* 
                         * L998 
                         * Cannot use itimeout(D2) at IPL9. Replace with 
                         * a call to the background poll handler. The iasy_hup()
                         * can be made ASAP, no need to time anything. The call
                         * sends an M_HANGUP upstream.
                         */ 

                        asyc->sighup_tp = tp;
                        asyc->flags |= DOSHUPTOUT; 

                } 
        }               
        return; 
}

/*
 * STATIC void
 * asyc_inchar(struct strtty *)
 *
 * Calling/Exit State:
 *      Called from asycpoll() to process data in idle buffer. 
 *      Called at splstr, but can be interrupted since only accesses
 *      the idle buffer.
 *      
 * Description:
 *      Process data in idle buffer, copy data to the STREAMS 
 *      buffers and send upstream if allowed. Processing is for the 
 *      parity/break (IGNPAR/IGNBRK),and all data where the upper byte 
 *  is set (this holds the LSR prior to the data read). Update 
 *      error count statistics. 
 *
 */

STATIC void
asyc_inchar(struct strtty *tp) 
{
        asycbuf_t               *ap;
        asyc_t                  *asyc;
        astats_t                *sp; 
        ushort_t                c, *idbuf; 
        uchar_t                 lsr0; 
        int                             iget = 0, iput; 
        boolean_t               saved_flag; 

        asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id, tp)];
        ap = asyc->asyc_bp;
        sp = asyc->asyc_stats; 

        /*
         * asyc_inchar() is only called if the last active buffer received
         * data, hence the current idle buffer must have data.
         * Set non-zero iget to indicate processing in progress.
         */

        saved_flag = ap->ibuf1_active; 

        if (ap->ibuf1_active) { 
                
                ASSERT(ap->iput2 && !ap->iget2); 

                idbuf = ap->ibuf2; 
                iput = ap->iput2; 
                ++ap->iget2; 

        } else { 

                ASSERT(ap->iput1 && !ap->iget1); 

                idbuf = ap->ibuf1; 
                iput = ap->iput1; 
                ++ap->iget1; 
        } 



        while ( iget != iput ) { 

                ASSERT(iget < iput); 

                /* 
                 * STREAMS buffer size left too low for further processing.
                 * (If the next data is a parity error/break condition it 
                 * may be translated to 3 characters (\377 \000 \ccc).
                 */

                if ( tp->t_in.bu_cnt < 3) {
                        if ( iasy_input( tp, L_BUF))
                                /* Failed to allocate a new STREAMS buffer */
                                break;          
                }
        
                /* Get next input character     */
                c = idbuf[iget++]; 

                /* 
                 * Update the error statistics; if any bits in upper byte are
                 * set this indicates an error, unless built under ASYC_DEBUG
                 */

                lsr0 = (c >> 8); 

                if ( lsr0 & LSR_OVRRUN ) 
                                sp->rxoverrun++; 

                /* 
                 * Make parity errors disappear if not checking input 
                 */

                if ((lsr0 & LSR_PARERR) && !(tp->t_iflag & INPCK)) { 
                                sp->rxperror++; 
                                lsr0 &= ~LSR_PARERR; 
                }       

                if ( lsr0 & (LSR_FRMERR|LSR_PARERR|LSR_BRKDET)) {

                        if (lsr0 & LSR_BRKDET) {

                                sp->rxbrkdet++; 

                                /* 
                                 * BREAK condition acts as a sequence point for all 
                                 * data received previously. The ldterm module handles
                                 * BRKINT, ISIG processing.
                                 */

                                if (!(tp->t_iflag & IGNBRK)) {

                                        iasy_input(tp, L_BUF);
                                        iasy_input(tp, L_BREAK);
                                }
                                continue;
                        }

                        /* 
                         * Either a framing error or a parity error
                         */
                        
                        if (lsr0 & LSR_FRMERR)
                                sp->rxfrmerror++; 

                        if (tp->t_iflag & IGNPAR)
                                        continue;
        
                        /* Mark parity errors as appropriate */
                        if (tp->t_iflag & PARMRK) {

                                /* 
                                 * Translate the Parity error , sequence is 
                                 *      char /ccc -> /377 /000 /ccc 
                                 */

                                PutInChar( tp, 0377);
                                PutInChar( tp, 0);
                                PutInChar( tp, c & 0377);
                                continue;

                        } else {

                                /* Send up a NULL character */
                                PutInChar( tp, 0);
                                continue;
                        }

                } else {

                        /* No errors; do good data checks */
                        if ( tp->t_iflag & ISTRIP) {
                                c &= 0177;
                        } else if (((c & 0377) == 0377) && (tp->t_iflag & PARMRK)) {
                                /* If marking parity errors, this character gets doubled */
                                PutInChar( tp, 0377);
                                PutInChar( tp, 0377);
                                continue;
                        }
                }

                /* Put single processed char upstream */
                PutInChar( tp, c);
        }

        /* 
         * Loop exit states are : 
         * (1) iput == iget -> finished processing all characters. 
         * (2) iget < iput  -> ran out of STREAMS buffer space, and 
         */ 

        if (iget < iput) {                      
                sp->droprxnobuf++; 
                sp->nobufchars += (iput - iget); 
        } 

        /*
         * Clear the idle buffer pointers to indicate the buffer can 
         * be reused. 
         */

        ASSERT(saved_flag == ap->ibuf1_active); 

        if (ap->ibuf1_active) 
                ap->iput2 = ap->iget2 = 0; 
        else 
                ap->iput1 = ap->iget1 = 0; 

        /* 
         * If we are not blocked, ship receive buffer upstream, full
         * or not.
         */

        iasy_input(tp, L_BUF);

        return; 
}

/*
 * STATIC void
 * asycpoll(void *)
 *
 * Calling/Exit State:
 *      Invoked by periodic itimeout(D3) at splstr. Interrupted context is 
 *      guaranteed to have been at splbase. Disables/restores IRQ processing 
 *      whilst in critical sections using ASYC_[CLI|STI]_BASE. 
 *
 * Description:
 *      Interface between STREAMS and CLI ISR code. Packetise any input data 
 *      and send upstream. Reload transmit data buffer if empty: either copy
 *      data into output buffer or set the pointers. 
 *
 */

/* ARGSUSED */
STATIC void
asycpoll(void * dummy)
{
        register struct strtty  *tp;
        asycbuf_t                               *ap;
        asyc_t                                  *asyc; 
        astats_t                                *sp; 
        struct termiox                  *xtp; 
        int                                             unit;
        boolean_t                               noinchar; 
        uchar_t                                 mcr, msr, lsr;

        /* 
         * Loop for all devices. 
         */

        for (unit = 0 ; unit < asyncfg; unit++ ) { 

                asyc = &asyctab[unit]; 
                ap = asyc->asyc_bp;
                sp = asyc->asyc_stats;
                tp = IASY_UNIT_TO_TP(asyc_id, unit); 
                xtp = &asy_xtty[IASY_TP_TO_UNIT(asyc_id, tp)];

                if (tp->t_state & ISOPEN) {

                        /* 
                         * L998
                         * Do schedule idle code : cannot call the asyc_schedule_idle 
                         * call from IPL 9, even though it used to. There is a possibility
                         * itimeout() will call kmem_zalloc() and this needs a lock we may 
                         * have interrupted at IPL 8. Also, same for the delta DCD code. 
                         */

                        ASYC_CLI_BASE(ap);      

                        if (asyc->flags & DOIDLETOUT) { 
                                clock_t         now;
                                if (asyc->endtime != 0) {
                                        if (drv_getparm(LBOLT,&now) < 0) {
                                                cmn_err(CE_WARN,"Cant get LBOLT time (poll)");
                                        } else if ((now - asyc->endtime) > 0){
                                                asyc->flags &= ~DOIDLETOUT;
                                                asyc->endtime = 0;
                                                asyc_idle_wrapper(asyc->tp);
                                        }       
                                } else {
                                        asyc->ticks -= asyc_pollticks;
                                        if (asyc->ticks < 0) {
                                                asyc->flags &= ~DOIDLETOUT;
                                                asyc->ticks = 0;
                                                asyc_idle_wrapper(asyc->tp);
                                        }
                                }
                        }


                        if (asyc->flags & DOSHUPTOUT) { 
                                asyc->flags &= ~DOSHUPTOUT; 
                                iasy_hup(asyc->sighup_tp); 
                                asyc->sighup_tp = 0; 
                        }

                        noinchar = B_FALSE;

                        /* 
                         * If the current buffer isn't empty, and upstream data 
                         * flow is not blocked, call asyc_inchar() 
                         */

                        if (ap->ibuf1_active ? ap->iput1 : ap->iput2) {

                                /* 
                                 * Data available: swap buffers and send idle buffer 
                                 * to post process routine. Ensure that the idle buffer 
                                 * is empty - PANIC if not under debugging.
                                 */

                                if (ap->ibuf1_active) { 
                                        if (ap->iget2 || ap->iput2) { /* Last buffer unfinished */
                                                cmn_err(CE_WARN,"asycpoll:Input processing too slow");
                                                sp->missedswap++ ; 
                                                noinchar = B_TRUE; 
                                        } else {
                                                ap->ibuf1_active = B_FALSE; 
                                        } 
                                } else { 
                                        if (ap->iget1 || ap->iput1) { 
                                                cmn_err(CE_WARN,"asycpoll:Input processing too slow");
                                                sp->missedswap++ ; 
                                                noinchar = B_TRUE; 
                                        } else {
                                                ap->ibuf1_active = B_TRUE; 
                                        } 
                                } 

                                /*
                                 * If the remote was stopped because of an internal buffer
                                 * overflow (not upstream request), clear it, and send a 
                                 * VSTART or set HWFC lines. 
                                 */ 

                                if ((asyc->flags & ISTOP) && !(tp->t_state & TBLOCK)) { 

                                        if ( tp->t_iflag & IXOFF ) { 
                                                asyc->flags |= IGOPEND; 
                                                asyc->flags &= ~ISTOPPEND; 
                                        }

                                        if (xtp->x_hflag & (RTSXOFF|DTRXOFF)) {
                                        
                                                mcr = inb(asyc->asyc_mcr);      
                                                mcr |= (MCR_RTS|MCR_DTR);
                                                outb(asyc->asyc_mcr, mcr); 
                                        } 
#ifdef ASYC_DEBUG 
                                        else { 
                                                mcr = inb(asyc->asyc_mcr);      
                                                ASSERT(mcr & MCR_RTS);
                                                ASSERT((mcr & MCR_DTR) || !(tp->t_cflag & CLOCAL)); 
                                        }       
#endif 
                                } 
        
                                ASYC_STI_BASE(ap); 

                                /* 
                                 * Process the characters found in the input buffer
                                 * if we didn't miss the swap. 
                                 */ 

                                if ( !noinchar )
                                        asyc_inchar(tp); 

                                ASYC_CLI_BASE(ap); 

                        }       /* Current device input processing */

                        /* 
                         * Check flow control sanity, move to the asycsetparam
                         * update routine, or ASYC_DEBUG maybe. Check that we can 
                         * restart stopped ports and stop ports.
                         */

                        if ((tp->t_iflag & IXON) && !(tp->t_iflag & IXANY) && 
                                (tp->t_cc[VSTART] == _POSIX_VDISABLE ))
                                cmn_err(CE_WARN,"No VSTART flow ctrl unit %d", unit); 

                        if ((tp->t_iflag & IXON) && (tp->t_cc[VSTOP] == _POSIX_VDISABLE))
                                cmn_err(CE_WARN,"No VSTOP flow ctrl unit %d", unit); 

                        /* 
                         * Check that we aren't blocked by an out of date flow 
                         * control mode (TCSETS race condition.)
                         */

                        if (!(tp->t_iflag & IXON))
                                asyc->flags &= ~OBLK_SWFC; 
                        
                        /* 
                         * If hardware flow control, then read line states to update, 
                         * remote may have cleared blocking state. 
                         */

                        if (!(xtp->x_hflag & (CTSXON|CDXON))) { 
                                asyc->flags &= ~OBLK_HWFC; 
                        } else { 
                                msr = inb(asyc->asyc_msr); 
                                if ((!(xtp->x_hflag & CTSXON) || (msr & MSR_CTS)) && 
                                        (!(xtp->x_hflag & CDXON) || (msr & MSR_DCD))) { 
                                        asyc->flags &= ~OBLK_HWFC; 
                                } else { 
                                        asyc->flags |= OBLK_HWFC; 
                                }
                        }

                        /* 
                         * Set primary state if currently idle, to flow controlled 
                         * state or vice versa: if TTSTOP'd then set to IDLE if flow 
                         * control has gone away.
                         */

                        if ((asyc->flags & OSTOP) && !(asyc->flags & OSTOPPEND)){

                                asyc->flags &= ~OSTOP; 
                                if (tp->t_state & TTSTOP)
                                        tp->t_state &= ~TTSTOP;

                        } else if (asyc->flags & OSTOPPEND) { 

                                        asyc->flags |= OSTOP; 
                                        if (TXIDLE(tp))
                                                tp->t_state |= TTSTOP;
                        }

#ifndef NOTXTO  

                        /* 
                         * Timeout can recover from lost TxRDY or failed itimeout(D3).
                         * The 1ary state is used to determine which actions should be 
                         * performed, and the UART THR/TSR states are verified.
                         * NB. Last pass entered with txtout == 0.
                         */
                
                        if ( ap->txtout > 0 ) {

                                ASSERT(tp->t_state & BUSY);     

                                --ap->txtout;

                        } else if (ap->txtout == 0) { 
                
                                /* 
                                 * If timeout is expired (not cancelled) then the
                                 * TxRDY has not been called. Also, the TxFIFO should
                                 * be empty, else timeout period was wrong.
                                 */

                                ASSERT(tp->t_state & BUSY);

                                GET_LSR(lsr);

                                ASSERT(lsr & LSR_XHRE); 
                                ASSERT(lsr & LSR_XSRE); 
        
                                sp->losttxrdyirq++; 

                                /* 
                                 * Now timeout fired, set the state back to IDLE 
                                 * and clear the timeout itself. 
                                 */ 

                                ap->txtout = -1;
                                tp->t_state &= ~BUSY; 

                                ASSERT( TXIDLE(tp) ); 

                        }

#endif /* NOTXTO */

                        /* 
                         * Call loadbuffer to top up the ISR obuf. Invoke when obuf
                         * reduced to low water or below. Hence, if possible, data is 
                         * pushed to ISR buffer as soon as possible. 
                         */

                        if ( OBUFCNT(ap) < ap->obuf_lowat )
                                asyc_loadtxbuffer(unit);                

                        /* 
                         * Start transmit from idle state. If data to send or tasks
                         * to perform, invoke idle or txsvc processing.
                         */

                        if (TXIDLE(tp)) { 

                                if (asyc->flags & IDLETASK) {
                                        sp->polltxcall++; 
                                        asyc_idle(tp); 
                                } else if (OBUFCNT(ap) && !(asyc->flags & OSTOPPEND)) { 
                                        sp->polltxcall++; 
                                        asyc_txsvc(tp);
                                } else if (tp->t_state & TTIOW){ 
                                        iasy_output(tp); 
                                }

                        }

                        ASYC_STI_BASE(ap); 

                } 

        } 

        return;
} 

/*
 * STATIC int
 * asycproc(struct strtty *, int) 
 *      low level device dependant operations
 *
 * Calling/Exit State:
 *      Called via ftab[] table from iasy module. 
 *      pl_t generally set to splstr on entry. 
 *      Preserves pl_t on exit.
 *      
 * Remarks:
 *      Interface undefined.
 *      
 */

STATIC int
asycproc(struct strtty *tp, int cmd)
{
        uchar_t                 lcr, msr;
        asyc_t                  *asyc;
        asycbuf_t               *ap;
        astats_t                *sp; 
        int                             ret = 0;
        uint_t                  unit; 
        struct termiox  *xtp; 
        ioaddr_t                io; 

        unit = IASY_TP_TO_UNIT(asyc_id, tp);
        asyc = &asyctab[unit]; 
        xtp = &asy_xtty[unit];
        io = asyc->asyc_dat; 
        ap = asyc->asyc_bp;
        sp = asyc->asyc_stats; 

        ASSERT( asyctab && ap); 

        /*
         * Lock out asycintr during asychronous writes to data.
         */

        ASYC_CLI_BASE(ap);

        switch (cmd) {
                
                /* 
                 * End of a BREAK status timed period. Clear BREAK state, 
                 * clear TIMEOUT state.
                 */

                case T_TIME:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_TIME tp 0x%x", tp);
#endif 
                        lcr = inb(asyc->asyc_lcr);

                        ASSERT(lcr & LCR_SETBREAK); 
                        ASSERT(tp->t_state & TIMEOUT);

                        outb(asyc->asyc_lcr, lcr & ~LCR_SETBREAK); 
                        tp->t_state &= ~TIMEOUT;
        
                        ASSERT(TXIDLE(tp)); 

                        break;

                /* 
                 * Flush the asyc owned WR(q) data: FIFO, strtty, ISR obuf
                 * May still be in BUSY state, as XSR not affected
                 */

                case T_WFLUSH:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_WFLUSH tp 0x%x", tp);
#endif 
                        outb(asyc->asyc_isr,FCR_FEN|FCR_TRST);
                        tp->t_out.bu_cnt = 0;
                        ap->oput = ap->oget = 0; 
                        break;

                /* 
                 * Allow data transmission, M_START. There may be several 
                 * halting causes, clear the M_START one. If this is the 
                 * sole cause, then, if TTSTOP is set, clear it. 
                 */

                case T_RESUME:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_RESUME tp 0x%x", tp);
#endif 
                        asyc->flags &= ~OBLK_MSTP; 

                        if (!(asyc->flags & OSTOPPEND)) { 
                                if (asyc->flags & OSTOP)
                                        asyc->flags &= ~OSTOP; 
                                if (tp->t_state & TTSTOP)
                                        tp->t_state &= ~TTSTOP;
                        }
                        break;

                /* 
                 * Stop data transmission, M_STOP message. If we are 
                 * already stopped, add our request flag to the others, 
                 * else, if transmitting, set our flag so that txsvc()
                 * can realise it, else set the flag. 
                 */

                case T_SUSPEND:         
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_SUSPEND tp 0x%x", tp);
#endif 
                        asyc->flags |= OBLK_MSTP; 

                        if (TXIDLE(tp)) { 
                                asyc->flags |= OSTOP; 
                                tp->t_state |= TTSTOP;  
                        } 
                        break;

                /* 
                 * Flush asyc owned RD(q): RxFIFO, strtty, ISR ibuf[12]
                 */

                case T_RFLUSH:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_RFLUSH tp 0x%x", tp);
#endif 

                        outb(asyc->asyc_isr,FCR_FEN|FCR_RRST); 
                        ap->iget1 = ap->iput1 = ap->iget2 = ap->iput2 = 0; 
                        tp->t_in.bu_cnt = IASY_BUFSZ;
                        tp->t_in.bu_ptr = tp->t_in.bu_bp->b_wptr;
                        break;

                /* 
                 * Restart remote by sending a VSTART char
                 */

                case T_UNBLOCK:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_UNBLOCK tp 0x%x", tp);
#endif 
                        /* 
                         * May have sent a VSTOP (ISTOP) or still be waiting (ISTOPPEND)
                         * either way, set IGOPEND and let the txsvc sort out the new 
                         * state, when it sends the character.
                         */

                        asyc->flags |= IGOPEND;
                        asyc->flags &= ~ISTOPPEND; 
                        tp->t_state &= ~TBLOCK; 

                        /* 
                         * Send data NOW if either blocked (ie. Tx idle due to flow 
                         * control), OR idle. 
                         */

                        if (!TXBUSY(tp) && !TXSPIN(tp))
                                asyc_txsvc(tp);

                        break;

                /* 
                 * Stop remote transmit by sending VSTOP char.
                 */

                case T_BLOCK:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_BLOCK tp 0x%x", tp);
#endif 
                        asyc->flags |= ISTOPPEND;
                        asyc->flags &= ~IGOPEND;

                        tp->t_state |= TBLOCK; 

                        /* 
                         * Send data NOW if either blocked (ie. Tx idle due to flow 
                         * control), OR idle. 
                         */

                        if (!TXBUSY(tp) && !TXSPIN(tp))
                                asyc_txsvc(tp);

                        break;

                /* 
                 * Send BREAK. Wait until the UART is IDLE then set the LCR_SETBREAK 
                 * bit and schedule a asycwakeup which terminates the BREAK state. 
                 */

                case T_BREAK:

#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_BREAK tp 0x%x", tp);
#endif 
                        /* 
                         * Inform the asyc_txsvc() that we wish to send a BREAK so 
                         * timeout the last Tx character. The timeout routine asyc_idle
                         * will detect the OBRKPEND and recall this routine with an 
                         * IDLE state.
                         */

                        if (!TXIDLE(tp)) {
                                asyc->flags |= OBRKPEND;
                        } else {

                                lcr = inb(asyc->asyc_lcr); 

                                ASSERT(!(lcr & LCR_SETBREAK));  
                
                                tp->t_state |= TIMEOUT;
                                outb(asyc->asyc_lcr, lcr | LCR_SETBREAK); 
                                ap->idletoid = itimeout(asycwakeup, (caddr_t)tp, HZ/4, plstr );
                        }
                        break; 

                /* 
                 * Called after a BREAK to restart transmit ? If already going 
                 * exit, otherwise refresh buffers and call the Tx service routine.
                 */

                case T_OUTPUT:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_OUTPUT tp 0x%x", tp);
#endif
                        if (OBUFCNT(ap) < ap->obuf_lowat)
                                asyc_loadtxbuffer(unit); 

                        if (TXIDLE(tp) && OBUFCNT(ap))
                                asyc_txsvc(tp); 

                        break; 
        
                /* 
                 * T_CONNECT is the request from iasy(7) to start the device.
                 * Also sets the UART parameters.
                 */ 

                case T_CONNECT:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_CONNECT tp 0x%x", tp);
#endif 
                        if (!(tp->t_state & (ISOPEN|WOPEN))) {
                                if ( asycparam(tp) < 0) {
                                        ret = EIO; 
                                        break; 
                                } 
                        }
                        
                        /* 
                         * Update the CARR_ON flag from the carrier state 
                         */

                        if (!asyc_clocal(tp)) {
                                msr = inb(asyc->asyc_msr); 
                                asyc_deltaDCD(tp,msr); 
                        } else { 
                                tp->t_state |= CARR_ON; 
                        }
                        break;

                /* 
                 * Drop modem connection, disassert DTR.
                 */

                case T_DISCONNECT:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_DISCONNECT tp 0x%x", tp);
#endif 
                        outb(asyc->asyc_mcr,MCR_OUT2|MCR_RTS);

                        break;

                /* 
                 * Set the UART serial parameters. Called when data has drained 
                 * (to last FIFO block) or as soon as this character has been sent.
                 */

                case T_PARM:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_PARM tp 0x%x", tp);
#endif 
                        /* 
                         * For cases where the user specifies split baud rates, fail
                         * the request before worrying about its timing. 
                         */

                        if (tpgetspeed(TCS_IN,tp) != tpgetspeed(TCS_OUT,tp)) { 
                                ret = EINVAL; 
                                break; 
                        }

                        /* 
                         * Caller's responsibility to ensure the state when 
                         * calling is suitable for a parameter setting.
                         * Can a parameter setting fail, after split speed removed?
                         */

                        if (tp->t_state & (TIMEOUT|BUSY)) { 
                                asyc->flags |= UARTPEND;
                                ret = 0;
                        } else { 
                                ret = asycparam(tp);
                        }
                        break;

                /* 
                 * Used by iasy driver to determine if there is any data waiting 
                 * to be written (blocked by anything or nothing). Include data 
                 * in Tx FIFO.
                 */

                case T_DATAPRESENT:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_DATAPRESENT tp 0x%x", tp);
#endif 
                        if ( OBUFCNT(ap) || (tp->t_out.bu_bp && tp->t_out.bu_cnt))
                                ret = ENOTEMPTY;
                        else 

#ifndef ASYC_SLACK_DPTEST
                        {
                                /* 
                                 * Is there data in the TxFIFO ? If we are BUSY and the  
                                 * Tx timeout is enabled, then yes (at least one byte).
                                 * asyc_idle_wrapper() BUSY until called.
                                 */ 

                                if (!(tp->t_state & BUSY))
                                        ret = 0;
                                else    
                                        ret = ENOTEMPTY;
                        }
#else 
                                ret = 0;
#endif 

                        break;

                /* 
                 * T_TRLVLn set the FIFO threshold values.
                 * User processes use SETUARTMODE ioctl(2).
                 * NB. Resets the FIFOs, flushing their contents first.
                 */

                case T_TRLVL1:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_TRLVL1 tp 0x%x", tp);
#endif 
                        outb (asyc->asyc_isr, 0x0);     
                        outb (asyc->asyc_isr, TRLVL1);
                        break;

                case T_TRLVL2:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_TRLVL2 tp 0x%x", tp);
#endif 
                        outb (asyc->asyc_isr, 0x0);
                        outb (asyc->asyc_isr, TRLVL2);
                        break;

                case T_TRLVL3:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_TRLVL3 tp 0x%x", tp);
#endif 
                        outb (asyc->asyc_isr, 0x0);
                        outb (asyc->asyc_isr, TRLVL3);
                        break;
        
                case T_TRLVL4:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_TRLVL4 tp 0x%x", tp);
#endif 
                        outb (asyc->asyc_isr, 0x0);
                        outb (asyc->asyc_isr, TRLVL4);
                        break;

        /* 
         * Check data structures are all clear in sensitive flags. 
         */

        case T_FIRSTOPEN:

#ifdef ASYC_DEBUG 
                if (asyc_debug & DSP_PROCS)
                        cmn_err(CE_WARN,"T_FIRSTOPEN tp 0x%x", tp);
#endif 

                ASSERT(!(asyc->flags & (IGOPEND|ISTOPPEND|ISTOP|OSTOPPEND|OSTOP)));
                ASSERT(!(tp->t_state & (BUSY|TIMEOUT|TTSTOP|TBLOCK))); 

                tp->t_state &= ~(BUSY|TIMEOUT|TTSTOP|TBLOCK); 
                asyc->flags &= CONDEV;  

                asyc->asyc_bp->txtout = -1; 
                asyc->asyc_bp->idletoid = 0; 

                ap->iget1 = ap->iput1 = 0; 
                ap->iget2 = ap->iput2 = 0; 

#ifdef MERGE386

                /* 
                 * For Merge processing, request privilege to access 
                 * serial USART ports, fail the open if unavailable.
                 */

                if (!portalloc(asyc->asyc_dat, asyc->asyc_dat + 0x7))
                        ret = EBUSY;

#endif /* MERGE386 */

                break;

        /* 
         * Called at various points in open(2) failure or close(2) succeed
         * but no obvious state rationale. In close(2) prior to clearing 
         * STREAMS buffer data, but has already drained. 
         */

        case T_LASTCLOSE:
#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_PROCS)
                                        cmn_err(CE_WARN,"T_LASTCLOSE tp 0x%x", tp);
#endif 

#ifdef MERGE386

                portfree(asyc->asyc_dat, asyc->asyc_dat + 0x7);

#endif /* MERGE386 */

                tp->t_state &= ~(BUSY|TIMEOUT|TTSTOP|TBLOCK|CARR_ON);
                asyc->flags &= CONDEV;
                xtp->x_hflag &= ~(RTSXOFF|DTRXOFF|CDXON|CTSXON);        

                asyc->asyc_bp->txtout = -1; 
                asyc->asyc_bp->idletoid = 0; 

                break;

        default:
                cmn_err(CE_WARN,"asycproc: bad command: %x", cmd);
                break; 

        }

        /* Unlock per port data */

        ASYC_STI_BASE(ap); 

        return(ret);
}

/* 
 * void schedule_idle(asycbuf_t *ap)
 * 
 * Calling/Exit:
 *      Called from asycintr when need to do task requiring a quiescent UART 
 *      transmitter. Called at IPL9, no t_state alterations allowed. 
 *      Also, not allowed to call itimeout() interrupt unsafe at IPL9.
 */

void 
asyc_schedule_idle(asyc_t       *asyc)
{
        asycbuf_t               *ap; 
        struct strtty   *tp;
        clock_t                 now;

        /* Check we're not in a schedule idle already   */
        if (asyc->flags & DOIDLETOUT)
                cmn_err(CE_WARN,"asyc: already in idle wait loop");


        /* 
         * Schedule the idle timeout to be after a single characters 
         * transmission time, or a single 'tick' if that is larger.
         */

        tp = &asy_tty[(asyc - asyctab)];
        ap = asyc->asyc_bp; 

        asyc->ticks = drv_usectohz(ap->txtchar) + TXTOFF;

        /*
         * If allowed to use the itimeout(D3) call do so. If it fails or we
         * are tuned not to, use asycpoll() as a timer. Set the DOIDLETOUT
         * flag. Try and use LBOLT, if that fails (how?) just dec a count.
         * If the end time is ever 0 (1 in 2^32 s) set to 1, 0 is reserved
         * for LBOLT get failure.
         */

        if (asyc_use_itimeout) {

                ap->idletoid = itimeout(&asyc_schedule_idle,tp,asyc->ticks,plstr);

                if (!ap->idletoid) {

                        cmn_err(CE_WARN,"asycintr: itimeout failed");
                
                } else {

                        /* OK, exit this scheduler      */
                        asyc->flags &= ~DOIDLETOUT; 
                        return;

                }

        }

        /* 
         * Failing itimeout(D3) and not allowed treated the same. Calc a 
         * end time if possible, else set endtime to 0, use ticks only.
         */

        if (drv_getparm(LBOLT,&now) < 0) {

                        cmn_err(CE_WARN,"Can't get LBOLT ticks");
                        asyc->endtime = 0;

        } else {
                        
                        /* Calculate end LBOLT time, ensure non zero    */
                        asyc->endtime = now + asyc->ticks + 1;
                        if (asyc->endtime == 0)
                                asyc->endtime = 1;

        }

        asyc->flags |= DOIDLETOUT; 
        asyc->tp = tp; 
        ap->idletoid = 1;

        return;

}

/* 
 * STATIC void 
 * asyc_txsvc(struct strtty *tp)
 *
 * Calling/Exit
 *      Called from asycintr() at BUSY, when correct IRQ (XHRE && !XSRE) to reload
 * data if no other IDLETASKs pending (else schedule idle). Called from poll()
 * when timeout has fired if IDLE and data and !IDLETASKs, called from OUTPUT 
 * if BREAK finished and IDLE. If TXIDLE() then at plstr/plbase and can set 
 * t_state else TXBUSY() at IPL9 and cannot set t_state.
 * 
 * Called at BUSY/IDLE, IPL9 or plstr. 
 */

STATIC void 
asyc_txsvc(struct strtty *tp)
{ 
        asyc_t                  *asyc; 
        ioaddr_t                io;     
        asycbuf_t               *ap; 
        astats_t                *sp; 
        int                             ccnt = 0, fifocnt ;  
        uchar_t                 lsr, msr ; 
        uint_t                  unit;
        struct termiox  *xtp; 
        ulong_t                 ticks;

#ifdef ASYC_DEBUG 
        if (asyc_debug & DSP_FUNCS)
                cmn_err(CE_NOTE,"asyc_txsvc");
#endif 

        unit = IASY_TP_TO_UNIT(asyc_id, tp);
        asyc = &asyctab[unit];
        xtp = &asy_xtty[unit];
        io = asyc->asyc_dat; 
        ap = asyc->asyc_bp;
        sp = asyc->asyc_stats; 
        fifocnt = asyc->txfifo; 

        /* 
         * May be BUSY or IDLE, depending on caller. We require a THRE state 
         * so that the transmit buffer capacity is known.
         */

#ifdef ASYC_DEBUG 

        GET_LSR(lsr);
        ASSERT(lsr & LSR_XHRE); 
        ASSERT(ap->txtout < 0); 

        // Causes _lots_ of ASSERTs on the PCTS testsuites
        // ASSERT(ap->idletoid == 0); 

#endif 

        /* 
         * Remote flow control data is sent whatever the flow control state 
         * of local transmitter.
         */

        if (asyc->flags & ISTOPPEND) { 

                ASSERT(!(asyc->flags & IGOPEND));

                outb(asyc->asyc_dat,tp->t_cc[VSTOP]);
                asyc->flags |= ISTOP; 
                asyc->flags &= ~ISTOPPEND; 
                ccnt++;
                sp->txflowchars++; 

        } else  if (asyc->flags & IGOPEND) { 

                ASSERT(!(asyc->flags & ISTOPPEND));

                outb(asyc->asyc_dat,tp->t_cc[VSTART]);
                asyc->flags &= ~(ISTOP|IGOPEND); 
                ccnt++;
                sp->txflowchars++; 

        } 

        /* 
         * Sample hardware flow control if enabled. This is safer (no 
         * problems with IRQs lost / MSR cleared ) than converting MSR
         * state to a state machine transition.
         * NB. No hardware flow control, no MSR read.
         */

        if ( xtp->x_hflag & (CTSXON|CDXON) )  { 

                msr = inb(asyc->asyc_msr); 

                if ((( xtp->x_hflag & CTSXON ) && !( msr & MSR_CTS )) || 
                        (( xtp->x_hflag & CDXON ) && !( msr & MSR_DCD ))) {
                        asyc->flags |= OBLK_HWFC; 
                        sp->notxhwfc++; 
                } 
        } else {
                asyc->flags &= ~OBLK_HWFC; 
        }

        /* 
         * Debug : verify that the DCD state is OK. This is handled by the 
         * MSR interrupt causing M_HANGUP messages, but we could quietly 
         * disable Tx during message passing, etc. The hangup will stop 
         * R/W and flush queues.
         */

#ifdef ASYC_DEBUG 

        msr = inb(asyc->asyc_msr); 
        ASSERT ( asyc_clocal(tp) || ( msr & MSR_DCD ) ); 

#endif 

        /* 
         * If transmit not blocked, load the TxFIFO with data characters, 
         * behind any remote flow controls. 
         */

        if (!(asyc->flags & OSTOPPEND)) { 

                sp->txdatachars -= ccnt; 
        
                while ((ccnt < fifocnt) && OBUFCNT(ap)) { 
                        outb(asyc->asyc_dat, ap->obuf[ap->oget]);
                        ap->oget = ONEXT(ap->oget);
                        ccnt++;
                }
        
        } 

        /* 
         * For asyc_txsvc() completed, set timeout period. If we were called 
         * from BUSY (IPL 9) then theres no change to the t_state, it we were
         * IDLE (asycpoll) then we are at plstr so can change. 
         */

        if (ccnt > 0) { 

                ASSERT(ap->txtoval[ccnt]);


#ifndef NOTXTO 
                ap->txtout = ap->txtoval[ccnt];
#else 
                ap->txtout = -1;
#endif 

                if (TXIDLE(tp))
                        tp->t_state |= BUSY; 

        } else {

                /* 
                 * No data to send, hence either flow controlled or none left, 
                 * if TXBUSY() set the EOTX timeout. 
                 */ 

                if (TXBUSY(tp))
                        asyc_schedule_idle(asyc);
                else if (TXIDLE(tp))
                        asyc_idle(tp);
        } 

        sp->txdatachars += ccnt; 
        return; 
} 

/*
 * STATIC int
 * asycparam(struct strtty *)
 *
 * Calling/Exit State:
 *      Called at STI, preserve.
 *
 * Description:
 *      Sets up the UART hardware and driver data to the settings in the 
 * strtty/termiox data structures set to the default/tuned value: now 
 * translate into UART control modes. Clear the Tx FIFO before we start
 * and the RxFIFO after doing change: the data OP / IP will be garbage
 * 
 */

STATIC int
asycparam(struct strtty *tp)
{
        speed_t                 s;      
        uchar_t                 x = 0, msr;
        uint_t                  unit = IASY_TP_TO_UNIT(asyc_id, tp); 
        struct termiox  *xtp = &asy_xtty[unit]; 
        asyc_t                  *ap = &asyctab[unit]; 
        ioaddr_t                io = uart_scantbl[unit].iobase; 

#ifdef ASYC_DEBUG 
        if (asyc_debug & DSP_FUNCS)
                cmn_err(CE_CONT,"!\nasycparam:"); 
#endif 

        /* Stop bits    */

        if ((tp->t_cflag & CSTOPB) == CSTOPB) 
                x |= LCR_STB;  

        /* Word size */

        switch (tp->t_cflag & CSIZE) { 
                case CS5: 
                        break ; 
                case CS6:
                        x |= LCR_BITS6;
                        break; 
                case CS7:
                        x |= LCR_BITS7;
                        break; 
                case CS8:
                        x |= LCR_BITS8;
                        break; 
        } 
                        
        if ( tp->t_cflag & PARENB )
                x |= LCR_PEN;
        if ( tp->t_cflag & PARODD )
                x &= ~LCR_EPS;
        else 
                x |= LCR_EPS;

#ifdef ASYC_DEBUG 
        if (asyc_debug & DSP_LCRCTL)
                cmn_err(CE_CONT, "!LCR = 0x%x",x); 
#endif 
        
        outb(ILCR(io), x);

        /* 
         * OUT2 line gates all UART interrupts, the PC architecture 
         * routes the UART IRQ via a buffer enabled by OUT2.
         */

        outb(IMCR(io), (MCR_DTR|MCR_RTS|MCR_OUT2)); 

        /* 
         * We have already failed requests with differing input/output 
         * speeds, so just call the function with TCS_ALL here. 
         */ 

        s = tpgetspeed(TCS_ALL,tp);

        /* 
         * Zero baud rate on a modem controlled device implies dropping the 
         * connection. It is an invalid setting for a non-modem device.
         */

        if (s == 0) { 

                if (!asyc_clocal(tp)) {

#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_DRVCTL)
                                cmn_err(CE_CONT, "!asycparam: Baud = 0, Dropping DTR line"); 
#endif 
                        outb(IMCR(io),(inb(IMCR(io)) & ~MCR_DTR)); 

                } else {
                        /* Bad parameter setting */
                        cmn_err(CE_WARN,"!asyc: 0 Baud invalid on CLOCAL port"); 
                        return(EINVAL);
                }

        } else { 

                /*
                 * Set the transmit timeout period (speed,txfifo,asyc_t).
                 * Pass in the asyc pointer, we may be called pre xxstart(D2) 
                 * in which case there's no asyctab[].
                 */

                asyc_settxtout(ap, s); 
                asyc_setspeed(tp, s); 
                outb(IMCR(io),(inb(IMCR(io)) | MCR_DTR)); 
        } 

        /* 
         * Enable all IRQs 
         */

        outb(IICR(io),(ICR_TIEN|ICR_SIEN|ICR_MIEN|ICR_RIEN));

        return (0);
}

/*
 * STATIC void
 * asycwakeup(struct strtty *) 
 *      End of BREAK output state.
 *
 * Calling/Exit State:
 *      Called by itimeout(D3) after BREAK delay expires, at plstr. 
 *
 * Description:
 *      It is used by the TCSBRK ioctl command.  After .25 sec
 *      timeout (see case BREAK in asycproc), this procedure is called.
 */

STATIC void
asycwakeup(struct strtty *tp)
{
        ASSERT(tp->t_state & TIMEOUT);
        asycproc(tp, T_TIME);
        ASSERT(TXIDLE(tp));
        return;
}


/* 
 * STATIC void
 * asyc_idle_wrapper (struct strtty *) 
 *
 * Calling/Exit State:
 *              Called by itimeout(D3) from asyc_schedule_idle() if BUSY state 
 * made good TxIRQ (XHRE && !XSRE), needing a last character timeout. 
 * 
 * Remarks: 
 *      Sets state (prior to calling asyc_idle() the idle handler) to make the 
 * calls from last character timeouts the same context as direct calls from 
 * asycintr() and asycpoll() when idle state detected. 
 */ 

STATIC void 
asyc_idle_wrapper(struct strtty *tp)
{ 
        uchar_t         lsr; 
        uint_t          unit = IASY_TP_TO_UNIT(asyc_id, tp);    
        asyc_t          *asyc = &asyctab[unit]; 
        asycbuf_t       *ap = asyc->asyc_bp; 

        /* 
         * Check state correct for call from asyc_txsvc() from asycintr(), 
         * ie. still in BUSY state, called by idle timeout, then clear them 
         * and set to TIMEOUT. 
         */ 

#ifdef ASYC_DEBUG 

        GET_LSR(lsr); 
        ASSERT(lsr & LSR_XSRE); 
        ASSERT(TXBUSY(tp));
        ASSERT(ap->idletoid); 
        ASSERT(ap->txtout < 0); 

#endif 

        /*
         * Call idle handler from CLI/STI, so that RMW of t_state OK. 
         * Initial idle wrapper from plbase context, from txsvc() or 
         * intr(). 
         */

        ASYC_CLI_BASE(ap);

        ap->idletoid = 0; 
        tp->t_state &= ~BUSY;

        ASSERT(TXIDLE(tp));
        
        asyc_idle(tp); 

        ASYC_STI_BASE(ap);

        return; 
}

/* 
 * STATIC void
 * asyc_idle (struct strtty *) 
 *
 * Calling/Exit State:
 *      From asyc_idle_wrapper() scheduled by asyc_txsvc() called by  asycintr()  
 *      which needed to perform idle tasks. 
 * 
 * Description:
 *      Search for idle tasks: blocked output, sending BREAK, sending a remote 
 *      flow character (VSTART/VSTOP), or running out of data, and perform them. 
 *      If nothing to do, could be end of data: leave in IDLE state, for restart.
 */

STATIC void 
asyc_idle(struct strtty *tp)
{ 
        uint_t          unit = IASY_TP_TO_UNIT(asyc_id, tp);    
        asyc_t          *asyc = &asyctab[unit]; 
        asycbuf_t       *ap = asyc->asyc_bp; 
        uchar_t         lsr; 


#ifdef ASYC_DEBUG 

        GET_LSR(lsr); 
        ASSERT(lsr & LSR_XSRE); 
        ASSERT(TXIDLE(tp)); 
        ASSERT(ap->idletoid == 0); 
        ASSERT(ap->txtout < 0); 

#endif 
        
        /* 
         * Perform idle tasks. If there is a flow control state pending, 
         * then only allow UART state setting, and remote flow control, 
         * as BREAK sending has same priority as data sending. 
         */


        if (asyc->flags & UARTPEND) { 
                asycparam(tp); 
                asyc->flags &= ~UARTPEND;
        } 

        /*  
         * If the OBLK_MSTP|OBLK_SWFC|OBLK_HWFC flags are set, set the state to 
         * TTSTOP, indicating no data output. 
         */
        
        if (asyc->flags & OSTOPPEND) {
                asyc->flags |= OSTOP; 
        }

        /* 
         * Send remote flow control characters: VSTART/VSTOP. The asyc_txsvc()
         * updates the asyc.flags field.
         */

        if (asyc->flags & (ISTOPPEND|IGOPEND)) { 
                asyc_txsvc(tp); 
        } 

        if (asyc->flags & OSTOP) 
                return; 

        if (asyc->flags & OBRKPEND) { 
                asycproc(tp, T_BREAK);
                asyc->flags &= ~OBRKPEND;
        }

        return; 

}

/*
 * STATIC void
 * asyc_setspeed (struct strtty *, speed_t)
 *      This procedure programs the baud rate generator.
 *
 * Calling/Exit State:
 *      Called by asycparam, plhi [ + PLOCK ]
 *
 * Remarks:
 *
 */

STATIC void
asyc_setspeed(struct strtty *tp, speed_t speed)
{
        ushort                  y;
        uchar_t                 x;
        ioaddr_t                io = uart_scantbl[IASY_TP_TO_UNIT(asyc_id,tp)].iobase; 

        if (speed == 0) { 
                cmn_err(CE_WARN,"asyc_setspeed:0 baudrate unit at io 0x%x",io); 
                return;
        } 
                
        x = inb(ILCR(io)); 
        outb(ILCR(io), x | LCR_DLAB);
        y = MAXBAUDRATE / speed ;  
        outb(IDAT(io), y & 0xff);
        outb(IICR(io), y >> 8);
        outb(ILCR(io), x);      
}

/* 
 * STATIC int 
 * asyc_chartime(speed_t baud) 
 *
 * Calling/Exit State:
 *      Called by asyc_setspeed, plhi [ + PLOCK ]
 * 
 * Remarks: 
 *      Get speed in usec per character - assume largest character 
 *      ie. 8 data + parity + start + 2 stop == 12 bits.
 * 
 */

STATIC uint_t
asyc_chartime ( speed_t baudrate ) 
{ 
        uint_t          us_char; 

        /* 
         * Baud rate is the bits/second rate, hence 1e6/ Baud 
         * gives the usecs/bit count, and 12e6/baud is the max 
         * character time. 
         */ 

        if ( baudrate == 0 ) 
                return (0xffffffff); 

        us_char = MAX_CHAR_K1 / baudrate; 

#ifdef ASYC_DEBUG 
                if (asyc_debug & DSP_TCHAR)
                        cmn_err(CE_NOTE,"!asyc_chartime: %d(0x%x) usec/char at %d Baud", 
                                                                        us_char, us_char, baudrate); 
#endif 
        return (us_char); 
} 

/*
 * STATIC void
 * asychwdep(queue_t *, mblk_t *, struct strtty *)
 *
 * Calling/Exit State:
 *      Called from iasy via ftab. Implements HW ioctls & accesses UART.
 *      Called at pl_t plstr, originally from iasyoput(D2). 
 *      Disables all interrupts during processing.
 * 
 * Remarks: 
 *      Handles both I_STR and TRANSPARENT ioctls. Each M_IOCTL, M_IOCDATA
 *      conntains id information and is handled separately, ie. no state is
 *      held between messages.
 */

STATIC void
asychwdep(queue_t *q, mblk_t *bp, struct strtty *tp)
{       
        struct iocblk   *ioc;
        asyc_t                  *asyc;
        asycbuf_t               *ap;
        int                     s, cmd, modval = 0, icnt ;
        uint_t                  id, unit ; 
        caddr_t                 uaddr;
        uchar_t                 msr, mcr;
        cred_t                  *credp;         
        struct copyreq  *cpreq;
        struct copyresp *cprv;
        struct termiox  *xtp;
        getasydat_t             *agp; 
        setasyc_t               *sdp;
        drvfunc_t               *dfp; 
        mblk_t                  *bpr; 
#ifdef ASYC_DEBUG 
        int                             curr_id = 0, curr_cmd = 0;
#endif

        /*
         * ioctl(2) requests are delivered as an M_IOCTL msg in the 
         * first data block (which holds the iocblk) and a bp->b_cont
         * 2ary block which is used to pass the data in strioctl(7) 
         * requests. Transparent ioctls have an ioc->ioc_count (data 
         * count) of -1, and require the driver to send the requisite 
         * M_COPYIN/M_COPYOUT to get/set the data in user context. The
         * address (in user space) at which the copy occurs is passed in 
         * the secondary data block (*(caddr_t *)bp->b_cont->rptr).
         */

        ioc = (struct iocblk *)bp->b_rptr;              /* ioctl command        */
        unit = IASY_TP_TO_UNIT(asyc_id, tp);    

        asyc = &asyctab[unit]; 
        ap = asyc->asyc_bp; 
        xtp = &asy_xtty[unit]; 

        /* 
         * I_STR ioctls all have an M_IOCTL and M_DATA, where the data 
         * is the argument passed in (M_DATA block is same type as the 
         * argument). For TRANSPARENT arguments, the M_DATA block holds 
         * the user address to M_COPYIN/M_COPYOUT the data: we make a 
         * copyin/copyout request; the M_IOCDATA returns the result from 
         * the stream head.
         */

        ASYC_CLI_BASE(ap); 

        switch (bp->b_datap->db_type) { 

                case M_IOCTL:

#ifdef ASYC_DEBUG
                if (asyc_debug & DSP_IOCTL)     
                        cmn_err(CE_NOTE, "!asychwdep: M_IOCTL"); 
#endif
                /*
                 * Secondary message block holds the ioctl data for I_STR ioctls
                 * or the user space address for TRANSPARENT ioctls. 
                 */

                if (!bp->b_cont)        
                        break; 
                /* 
                 * uaddr is used in all TRANSPARENT ioctl calls as the user space 
                 * address. It is held in the 2ary data. Save other data that is
                 * needed for M_IOCACK/M_COPYIN/M_COPYOUT.
                 */ 

                cmd     = ioc->ioc_cmd;
                id      = ioc->ioc_id; 
                credp   = ioc->ioc_cr;
                icnt    = ioc->ioc_count; 
                uaddr   = *((caddr_t *)bp->b_cont->b_rptr); 

#ifdef ASYC_DEBUG

                /*
                 * Debugging: check the driver is not currently processing an 
                 * ioctl; the curr_id, curr_cmd are set for the duration of an
                 * ioctl(2) processing.
                 */

                if (curr_id && id != curr_id) 
                        cmn_err(CE_WARN,"!Multiple M_IOCTLs cmd:0x%x,0x%x id:0x%x,0x%x",
                                                                                                cmd, curr_cmd,id, curr_id);
                
#endif 

#ifdef ASYC_DEBUG
                if (asyc_debug & DSP_IOCTL) {   
                        cmn_err(CE_NOTE, "!cmd 0x%x id 0x%x *cred 0x%x", cmd, id, credp ); 
                        if (icnt == TRANSPARENT)
                                cmn_err(CE_CONT,"!TRANSPARENT: uaddr 0x%x\n",uaddr); 
                        else 
                                cmn_err(CE_CONT,"!I_STR: arg %d bytes at 0x%x\n",icnt,uaddr); 
                } 
#endif

                switch (ioc->ioc_cmd) {

                /* 
                 * TIOCMGET : Get the modem/flow lines status. 
                 * Arg: int *tiocmflags 
                 */

                case TIOCMGET:          

                        msr = asyc->asyc_msrlaststate;
                        mcr = inb(asyc->asyc_mcr); 
        
                        if (mcr & MCR_DTR) 
                                modval |= TIOCM_DTR;            
                        if (mcr & MCR_RTS) 
                                modval |= TIOCM_RTS;            
        
                        if (msr & MSR_CTS)
                                modval |= TIOCM_CTS; 
                        if (msr & MSR_DCD)
                                modval |= TIOCM_CD; 
                        if (msr & MSR_DSR)
                                modval |= TIOCM_DSR; 
                        if (msr & MSR_RI)
                                modval |= TIOCM_RI; 

                        if (icnt == TRANSPARENT) {

                                bp->b_datap->db_type = M_COPYOUT;
                                credp = ioc->ioc_cr;

                                cpreq = (struct copyreq *)bp->b_rptr;
                                cpreq->cq_cmd = cmd ;
                                cpreq->cq_cr = credp; 
                                cpreq->cq_id = id;
                                cpreq->cq_addr = uaddr;
                                cpreq->cq_size = sizeof(int);

                                /* int argument - reuse M_DATA  */
                                *((int *) bp->b_cont->b_rptr) = modval; 
                                (void) putnext(RD(q), bp);

                        } else {

                                *((int *) bp->b_cont->b_rptr) = modval; 
                                bp->b_cont->b_wptr = bp->b_cont->b_rptr + sizeof(int);
                                ioc->ioc_count = sizeof(int);
                                bp->b_datap->db_type = M_IOCACK;
                                ioc->ioc_rval = 0;
                                ioc->ioc_error = 0;
                                (void) putnext(RD(q), bp);
                        }

                        ASYC_STI_BASE(ap); 
                        return; 
                        /* NOTREACHED */        
                        break;

                case TIOCMSET:

                        /* 
                         * TIOCMSET : set output lines according to input TIOCM flags.
                         * Arg: int * tiocmflags
                         */

                        if (ioc->ioc_count == TRANSPARENT) {

                                bp->b_datap->db_type = M_COPYIN;
                                
                                cpreq = (struct copyreq *)bp->b_rptr;
                                cpreq->cq_cmd = cmd;
                                cpreq->cq_cr = credp; 
                                cpreq->cq_id = id;
                                cpreq->cq_addr = uaddr;
                                cpreq->cq_size = sizeof(int);

                                freemsg(bp->b_cont); 
                                bp->b_cont = 0;

                                (void) putnext(RD(q), bp);

                        } else {
                                /*
                                 * TIOCMSET ioctls are anathema to hardware flow control. 
                                 * Explicitly disable the flow control if used. 
                                 */

                                if (xtp->x_hflag & (RTSXOFF|DTRXOFF)) 
                                        cmn_err(CE_WARN,"asyc: %d/%d TIOCMSET: disable HW flow ctl",
                                                                getemajor(tp->t_dev),geteminor(tp->t_dev));

                                if ( TIOCM_DIS_HWFC(unit) )
                                        xtp->x_hflag &= ~(RTSXOFF|DTRXOFF);     

                                mcr = inb(asyc->asyc_mcr); 
                                modval = *((int *) bp->b_cont->b_rptr); 

                                if (modval & TIOCM_DTR) 
                                        mcr |= MCR_DTR;         
                                else 
                                        mcr &= ~MCR_DTR;        

                                if (modval & TIOCM_RTS) 
                                        mcr |= MCR_RTS;         
                                else 
                                        mcr &= ~MCR_RTS;        

                                outb(asyc->asyc_mcr, mcr); 

                                bp->b_datap->db_type = M_IOCACK; 
                                ioc->ioc_count = ioc->ioc_rval =  ioc->ioc_error = 0;
                                freemsg(bp->b_cont); 
                                bp->b_cont = 0; 
                                (void) putnext(RD(q), bp);

                        }

                        ASYC_STI_BASE(ap); 
                        return;
                        /* NOTREACHED */        
                        break;


                /* 
                 * TIOCMBIS : Set flow control lines 
                 * Arg: int *tiocmflags.
                 */ 

                case TIOCMBIS:

                        if (ioc->ioc_count == TRANSPARENT) {

                                bp->b_datap->db_type = M_COPYIN;
                                
                                cpreq = (struct copyreq *)bp->b_rptr;
                                cpreq->cq_cmd = cmd;
                                cpreq->cq_cr = credp; 
                                cpreq->cq_id = id;
                                cpreq->cq_addr = uaddr;
                                cpreq->cq_size = sizeof(int);

                                freemsg(bp->b_cont); 
                                bp->b_cont = 0;                         

                                (void) putnext(RD(q), bp);

                        } else {

                                if (xtp->x_hflag & (RTSXOFF|DTRXOFF)) 
                                        cmn_err(CE_WARN,"asyc: %d/%d TIOCMSET: disable HW flow ctl",
                                                                getemajor(tp->t_dev),geteminor(tp->t_dev));

                                if ( TIOCM_DIS_HWFC(unit) )
                                        xtp->x_hflag &= ~(RTSXOFF|DTRXOFF);     

                                mcr = inb(asyc->asyc_mcr); 
                                modval = *((int *) bp->b_cont->b_rptr); 

                                if (modval & TIOCM_DTR) 
                                        mcr |= MCR_DTR;         

                                if (modval & TIOCM_RTS) 
                                        mcr |= MCR_RTS;         

                                outb(asyc->asyc_mcr, mcr); 

                                bp->b_datap->db_type = M_IOCACK; 
                                ioc->ioc_count = ioc->ioc_rval =  ioc->ioc_error = 0;
                                freemsg(bp->b_cont); 
                                bp->b_cont = 0; 
                                (void) putnext(RD(q), bp);
                        }

                        ASYC_STI_BASE(ap); 
                        return;
                        /* NOTREACHED */        
                        break;
                        

                /* 
                 * TIOCMBIC : Clear flow control lines 
                 * Arg: int *tiocmflags.
                 */ 

                case TIOCMBIC: 

                        if (ioc->ioc_count == TRANSPARENT) {

                                bp->b_datap->db_type = M_COPYIN;
                                
                                cpreq = (struct copyreq *)bp->b_rptr;
                                cpreq->cq_cmd = cmd;
                                cpreq->cq_cr = credp;
                                cpreq->cq_id = id;
                                cpreq->cq_addr = uaddr;
                                cpreq->cq_size = sizeof(int);

                                freemsg(bp->b_cont); 
                                bp->b_cont = 0; 

                                (void) putnext(RD(q), bp);

                        } else {

                                if (xtp->x_hflag & (RTSXOFF|DTRXOFF)) 
                                        cmn_err(CE_WARN,"asyc: %d/%d TIOCMSET: disable HW flow ctl",
                                                                getemajor(tp->t_dev),geteminor(tp->t_dev));

                                if ( TIOCM_DIS_HWFC(unit) )
                                        xtp->x_hflag &= ~(RTSXOFF|DTRXOFF);     

                                mcr = inb(asyc->asyc_mcr); 
                                modval = *((int *) bp->b_cont->b_rptr); 

                                if (modval & TIOCM_DTR) 
                                        mcr &= ~MCR_DTR;        

                                if (modval & TIOCM_RTS) 
                                        mcr &= ~MCR_RTS;        

                                outb(asyc->asyc_mcr, mcr); 

                                bp->b_datap->db_type = M_IOCACK; 
                                ioc->ioc_count = ioc->ioc_rval =  ioc->ioc_error = 0;
                                freemsg(bp->b_cont); 
                                bp->b_cont = 0; 
                                (void) putnext(RD(q), bp);
                        }

                        ASYC_STI_BASE(ap); 
                        return;
                        /* NOTREACHED */        
                        break;
                
                /* 
                 * The flushing of RD queues and draining of write queues has 
                 * been done by the time asychwdep() gets the request, treat 
                 * them all the same here and set the termiox data.
                 */

                case TCSETXF:   
                case TCSETXW:
                case TCSETX:

                        /* 
                         * TCSETX[WF] Set termiox to user specififed state. 
                         * iasy(7) takes care of the flush/drain requirements. 
                         * Arg: struct termiox *xtp; 
                         */

                        if (ioc->ioc_count == TRANSPARENT) {

                                bp->b_datap->db_type = M_COPYIN;
                                
                                cpreq = (struct copyreq *)bp->b_rptr;
                                cpreq->cq_cmd = cmd;
                                cpreq->cq_cr  = credp;
                                cpreq->cq_id = id;
                                cpreq->cq_addr = uaddr;
                                cpreq->cq_size = sizeof(struct termiox);

                                freemsg(bp->b_cont);
                                bp->b_cont = 0; 

                                (void) putnext(RD(q), bp);

                        } else {

                                /* 
                                 * No attempt to verify that the flags states are consistent
                                 * with anything. Caveat ioctler.
                                 */

                                bcopy( (caddr_t *) bp->b_cont->b_rptr, 
                                                (caddr_t *)     &asy_xtty[unit], sizeof(struct termiox)); 

                                bp->b_datap->db_type = M_IOCACK; 
                                ioc->ioc_count = ioc->ioc_rval =  ioc->ioc_error = 0;
                                freemsg(bp->b_cont); 
                                bp->b_cont = 0; 
                                (void) putnext(RD(q), bp);
                        }

                        ASYC_STI_BASE(ap); 
                        return;
                        /* NOTREACHED */        
                        break;

                case TCGETX: 

                        if (!(bpr = allocb(sizeof(struct termiox), BPRI_MED)))
                                break;

                        bcopy((caddr_t *)&asy_xtty[unit], (caddr_t *)bpr->b_rptr, 
                                        sizeof(struct termiox)); 

                        bpr->b_wptr += sizeof(struct termiox);

                        if (ioc->ioc_count == TRANSPARENT) {

                                /* Send message upstream for M_COPYOUT  */

                                bp->b_datap->db_type = M_COPYOUT;

                                cpreq = (struct copyreq *)bp->b_rptr;
                                cpreq->cq_cr    = credp;
                                cpreq->cq_cmd   = cmd;
                                cpreq->cq_id    = id;
                                cpreq->cq_addr  = uaddr; 
                                cpreq->cq_size  = sizeof(struct termiox);

                                freemsg(bp->b_cont);

                                bp->b_cont      = bpr; 

                                (void) putnext(RD(q), bp);

                        } else {

                                ioc->ioc_count = sizeof(struct termiox);
                                bp->b_datap->db_type = M_IOCACK;

                                freemsg(bp->b_cont); 
                                bp->b_cont = bpr; 

                                ioc->ioc_rval = ioc->ioc_error = 0;
                                (void) putnext(RD(q), bp);
                        }

                        ASYC_STI_BASE(ap); 
                        return; 
                        /* NOTREACHED */        
                        break;

                /*
                 * ASY_DRVFUNCS is a general purpose ioctl(2) that is used 
                 * to initiate varius internal driver operations, eg. tune
                 * Rx FIFO trigger levels, retrieve logging data, etc. 
                 */

                case ASY_DRVFUNCS: 

#ifdef ASYC_DEBUG
                        if (asyc_debug & DSP_IOCTL)
                                cmn_err(CE_NOTE,"ASY_DRVFUNCS bp 0x%x tp 0x%x", bp, tp );
#endif  

                        /* 
                         * Some driver functions require user data, others 
                         * take dummy arguments.
                         */

                        if (ioc->ioc_count == TRANSPARENT) {

                                bp->b_datap->db_type = M_COPYIN;
                                
                                cpreq = (struct copyreq *)bp->b_rptr;
                                cpreq->cq_cmd   = cmd;
                                cpreq->cq_cr    = credp; 
                                cpreq->cq_id    = id;
                                cpreq->cq_addr  = uaddr;
                                cpreq->cq_size  = sizeof(drvfunc_t);

                                freemsg(bp->b_cont); 
                                bp->b_cont = 0; 

                                (void) putnext(RD(q), bp);

                        } else {
                                
                                /* Data is user data */

                                dfp = (drvfunc_t *) bp->b_cont->b_rptr; 

                                /* 
                                 * Depending on the function specified the input
                                 * data is manipulated here.
                                 */

                                freemsg(bp->b_cont); 
                                bp->b_cont = 0; 

                                bp->b_datap->db_type = M_IOCACK; 
                                ioc->ioc_count = ioc->ioc_rval =  ioc->ioc_error = 0;
                                (void) putnext(RD(q), bp);

                        }

                        ASYC_STI_BASE(ap); 
                        return; 
                        /* NOTREACHED */        
                        break;  

                case ASY_SETUARTMODE:
                                
#ifdef ASYC_DEBUG
                        if (asyc_debug & DSP_IOCTL)
                                cmn_err(CE_NOTE,"!ASY_SETUARTMODE bp 0x%x tp 0x%x", bp, tp );
#endif  
                        if (ioc->ioc_count == TRANSPARENT) {

                                bp->b_datap->db_type = M_COPYIN;
                                
                                cpreq = (struct copyreq *)bp->b_rptr;
                                cpreq->cq_cmd   = cmd;
                                cpreq->cq_cr    = credp; 
                                cpreq->cq_id    = id;
                                cpreq->cq_addr  = uaddr;
                                cpreq->cq_size  = sizeof(setasyc_t);

                                freemsg(bp->b_cont); 
                                bp->b_cont = 0; 

                                (void) putnext(RD(q), bp);

                        } else {

                                /* 
                                 * Copy the passed in data to the driver 
                                 * variables.
                                 */

                                sdp = (setasyc_t *) bp->b_cont->b_rptr; 

                                asyc->rxfifo = sdp->set_rxfifo; 
                                asyc->txfifo = sdp->set_txfifo; 
                                ap->ibuf_lowat = sdp->set_rxlowat; 
                                ap->ibuf_hiwat = sdp->set_rxhiwat; 
                                asyc->pollfreq = sdp->set_pollfreq; 
                                asyc->compat = sdp->set_compat; 

                                freemsg(bp->b_cont); 
                                bp->b_cont = 0; 

                                bp->b_datap->db_type = M_IOCACK; 
                                ioc->ioc_count = ioc->ioc_rval =  ioc->ioc_error = 0;
                                (void) putnext(RD(q), bp);

                        }

                        ASYC_STI_BASE(ap); 
                        return; 
                        /* NOTREACHED */        
                        break;  

                case ASY_GETUARTDATA:

#ifdef ASYC_DEBUG
                        if (asyc_debug & DSP_IOCTL)
                                cmn_err(CE_NOTE,"!ASY_GETUARTDATA bp 0x%x tp 0x%x", bp, tp );
#endif  

                        if ((bpr = allocb(sizeof(getasydat_t), BPRI_MED)) == 0) 
                                break;

                        /* 
                         * Fill in data allocated : uart_scantbl[] setup data 
                         * and asyc_t / strtty operating data.
                         */

                        agp = (getasydat_t *)bpr->b_rptr; 

                        ASSERT(sizeof(getasydat_t) > sizeof(uart_data_t)); 

                        /*
                         * Block copy the uart scan table entry, fill in 
                         * settable parameters field individually.
                         */

                        bcopy((caddr_t *)&uart_scantbl[unit], (caddr_t *)&(agp->udata), 
                                                sizeof(uart_data_t)); 

                        agp->asydrv.set_rxfifo = asyc->rxfifo; 
                        agp->asydrv.set_txfifo = asyc->txfifo; 
                        agp->asydrv.set_rxlowat = ap->ibuf_lowat; 
                        agp->asydrv.set_rxhiwat = ap->ibuf_hiwat; 
                        agp->asydrv.set_pollfreq =      asyc->pollfreq; 
                        agp->asydrv.set_compat = asyc->compat; 

                        bpr->b_wptr += sizeof(getasydat_t);

                        if (icnt == TRANSPARENT) {
                                
                                bp->b_datap->db_type = M_COPYOUT;

                                cpreq = (struct copyreq *)bp->b_rptr;
                                cpreq->cq_cmd   = cmd;
                                cpreq->cq_cr    = credp; 
                                cpreq->cq_id    = id;
                                cpreq->cq_addr  = uaddr;
                                cpreq->cq_size  = sizeof(getasydat_t);

                                freemsg(bp->b_cont); 
                                bp->b_cont = bpr; 

                                (void) putnext(RD(q), bp);

                        } else {

                                freemsg(bp->b_cont); 
                                bp->b_cont = bpr; 

                                ioc->ioc_count = sizeof(getasydat_t);
                                bp->b_datap->db_type = M_IOCACK;
                                ioc->ioc_rval = ioc->ioc_error = 0;
                                (void) putnext(RD(q), bp);

                        }

                        ASYC_STI_BASE(ap); 
                        return; 
                        /* NOTREACHED */
                        break; 

                case ASY_GETSTATS :

#ifdef ASYC_DEBUG
                        if (asyc_debug & DSP_IOCTL)
                                cmn_err(CE_NOTE,"!ASY_GETSTATS ioctl");
#endif  
                        if ((bpr = allocb(sizeof(astats_t), BPRI_MED)) == 0) 
                                /* ENOMEM */
                                break;
                        
                        bcopy((caddr_t *) asyctab[unit].asyc_stats, 
                                        (caddr_t *)bpr->b_rptr, sizeof(astats_t)); 

                        /* 
                         * Zero all the fields on a read: accumulation is the 
                         * users responsibility. Perhaps we should add a time
                         * stamp. 
                         */ 

                        bzero ((caddr_t *)asyctab[unit].asyc_stats, sizeof(astats_t)); 

                        bpr->b_wptr += sizeof(astats_t);

                        if (ioc->ioc_count == TRANSPARENT) {
                                
                                bp->b_datap->db_type = M_COPYOUT;

                                cpreq = (struct copyreq *)bp->b_rptr;
                                cpreq->cq_cr    = credp; 
                                cpreq->cq_cmd   = cmd;
                                cpreq->cq_id    = id;
                                cpreq->cq_addr  = uaddr; 
                                cpreq->cq_size  = sizeof(astats_t);
                                
                                freemsg(bp->b_cont); 
                                bp->b_cont = bpr; 

                                (void) putnext(RD(q), bp);

                        } else {

                                freemsg(bp->b_cont); 
                                bp->b_cont = bpr; 

                                ioc->ioc_count = sizeof(astats_t);
                                bp->b_datap->db_type = M_IOCACK;
                                ioc->ioc_rval = ioc->ioc_error = 0;
                                (void) putnext(RD(q), bp);

                        }

                        ASYC_STI_BASE(ap); 
                        return; 
                        /* NOTREACHED */        
                        break; 

                /* 
                 * This is the unrecognised M_IOCTL type case: return an M_IOCNAK 
                 * packet to the stream head.
                 */

                default:

#ifdef ASYC_DEBUG 
                        if (asyc_debug & DSP_IOCTL) { 
                                cmn_err(CE_WARN,
                                        "!asychwdep: Unknown M_IOCTL 0x%x\n", ioc->ioc_cmd); 
                        } 
#endif 
                        bp->b_datap->db_type = M_IOCNAK ;
                        ioc->ioc_error = EINVAL ;
                        ioc->ioc_rval = -1; 
                        ioc->ioc_count = 0; 
                        (void) putnext(RD(q), bp);
                
                        ASYC_STI_BASE(ap); 
                        return; 
                        /* NOTREACHED */
                        break;

                }       /* ioc_cmd      */

                break;

        /* 
         * M_COPYIN/M_COPYOUT : for transparent ioctls which require data
         * from user space (holding ioctl arguments), ie. ioctl(2)s with 
         * arguments used to set kernel data. 
         */

        case M_IOCDATA:

#ifdef ASYC_DEBUG 
                if (asyc_debug & DSP_IOCTL)
                        cmn_err(CE_CONT,"!M_IOCACK received\n"); 
#endif 
                /* 
                 * Save data for the M_IOCACK response.
                 */

                cprv = (struct copyresp *)bp->b_rptr;
                cmd = cprv->cp_cmd;
                id  = cprv->cp_id;
                credp = cprv->cp_cr;

#ifdef ASYC_DEBUG 
                if (asyc_debug & DSP_IOCTL)
                        cmn_err(CE_CONT,"!cmd: 0x%x, id 0x%x rv 0x%x\n", 
                                                                                cmd, id, cprv->cp_rval); 
#endif 

                if (cprv->cp_rval == 0) { 

                        /*
                         * For M_COPYIN responses set the driver data to the 
                         * passed in data (as for non-transparent strioctls.
                         * For M_COPYOUT responses (eg. GET* type ioctls), the 
                         * default action (for rval == 0) of M_IOCACK, is 
                         * sufficient.
                         */

                        switch(cmd)             { 

                                case TIOCMSET:

                                        if ( TIOCM_DIS_HWFC(unit) )
                                                xtp->x_hflag &= ~(RTSXOFF|DTRXOFF);     

                                        mcr = inb(asyc->asyc_mcr);
                                        modval = *((int *) bp->b_cont->b_rptr); 

                                        if (modval & TIOCM_DTR) 
                                                mcr |= MCR_DTR;         
                                        else 
                                                mcr &= ~MCR_DTR;        

                                        if (modval & TIOCM_RTS) 
                                                mcr |= MCR_RTS;         
                                        else 
                                                mcr &= ~MCR_RTS;        

                                        outb(asyc->asyc_mcr, mcr); 
                                        break;

                                case TIOCMBIS:

                                        if ( TIOCM_DIS_HWFC(unit) )
                                                xtp->x_hflag &= ~(RTSXOFF|DTRXOFF);     

                                        mcr = inb(asyc->asyc_mcr); 
                                        modval = *((int *) bp->b_cont->b_rptr); 
        
                                        if (modval & TIOCM_DTR) 
                                                mcr |= MCR_DTR;         

                                        if (modval & TIOCM_RTS) 
                                                mcr |= MCR_RTS;         

                                        outb(asyc->asyc_mcr, mcr); 
                                        break;

                                case TIOCMBIC:

                                        if ( TIOCM_DIS_HWFC(unit) )
                                                xtp->x_hflag &= ~(RTSXOFF|DTRXOFF);     

                                        mcr = inb(asyc->asyc_mcr); 
                                        modval = *((int *) bp->b_cont->b_rptr); 

                                        if (modval & TIOCM_DTR) 
                                                mcr &= ~MCR_DTR;        

                                        if (modval & TIOCM_RTS) 
                                                mcr &= ~MCR_RTS;        

                                        outb(asyc->asyc_mcr, mcr); 
                                        break;

                                case TCSETX:
                                case TCSETXF:
                                case TCSETXW:

                                        bcopy((caddr_t *) bp->b_cont->b_rptr, 
                                        (caddr_t *)     &asy_xtty[unit], sizeof(struct termiox)); 
                                        break;

                                case ASY_SETUARTMODE:

                                        sdp = (setasyc_t *) bp->b_cont->b_rptr; 

                                        asyc->rxfifo = sdp->set_rxfifo; 
                                        asyc->txfifo = sdp->set_txfifo; 
                                        ap->ibuf_lowat = sdp->set_rxlowat; 
                                        ap->ibuf_hiwat = sdp->set_rxhiwat; 
                                        asyc->pollfreq = sdp->set_pollfreq; 
                                        asyc->compat = sdp->set_compat; 
                                        break; 
                
                                case ASY_DRVFUNCS: 

                                        dfp = (drvfunc_t *) bp->b_cont->b_rptr; 

                                        /* 
                                         * Process copied in data.
                                         */

                                        break; 

                                default:
                                        /* 
                                         * No especial action required - do the 
                                         * M_IOCACK as below.
                                         */
                                        break;
                        }

                        /* 
                         * For all M_IOCDATA responses (M_COPYIN/M_COPYOUT)
                         * send an M_IOCACK to unblock the caller. 
                         */

                        ioc->ioc_cmd = cmd ;
                        ioc->ioc_id = id ;
                        ioc->ioc_cr = credp ;

                        bp->b_datap->db_type = M_IOCACK; 

                        ioc->ioc_count = ioc->ioc_rval =  ioc->ioc_error = 0;
                        (void) putnext(RD(q), bp);

                } else {        /* cp->rval != 0 <=> M_COPY[IN|OUT] failed */

                        /* 
                         * M_COPYIN failure - return M_IOCNAK since 
                         * process is still blocked on initial request.
                         */

                        cmn_err(CE_WARN,
                                "asychwdep:copyreq failed:  tp: 0x%x, bp: 0x%x", tp, bp);

                        bp->b_datap->db_type = M_IOCNAK ;
                        ioc->ioc_error = EINVAL ;
                        ioc->ioc_rval = -1; 
                        ioc->ioc_count = 0; 
                        ioc->ioc_cmd = cmd; 
                        ioc->ioc_id = id; 
                        ioc->ioc_cr     = credp; 

                        (void) putnext(RD(q), bp);

                }

                ASYC_STI_BASE(ap); 
                return;
                /* NOTREACHED */
                break; 

        /* 
         * Not an M_IOCTL or M_IOCDATA message type. If Merge doesn't want
         * it just free the blocks: whatever wants it has missed it. 
         */

        default:
#ifdef MERGE386

                if (com_ppi_strioctl(q, bp, &asyc->mrg_data, ioc->ioc_cmd)) {
                        if (!(asyc->asyc_mrg_state & ASYC_MRG_ATTACH)) {
                                downgrade_asyc_ipl(asyc->asyc_vect, (void *)asycintr);
                                asyc->asyc_mrg_state |= ASYC_MRG_ATTACH;
                        }
                } else 
#endif 
                { 
                        /*
                         *+ An unknown message type, free blocks and return.
                         */

                        cmn_err(CE_WARN,"asyc:Bad msg mblk_t 0x%x strtty 0x%x", bp,tp);
                        freemsg(bp);
                        ASYC_STI_BASE(ap); 
                        return;
                        /* NOTREACHED */
                } 

        }       /* End of STREAMS message switch */

        ASYC_STI_BASE(ap); 

        return; 

}

/*
 *
 * debugger/console support routines.
 *
 *      System console can be assigned to a serial device by a bootstring or 
 * explicit setting of the console in sassugn.d/kernel. The OS boot loader
 * parses the bootstring
 *              console = iasy ( m, params ) 
 * and redirects I/O to the serial port specified by the minor number, after 
 * initialising the port to the protocol specified (96,N,8,1 default). 
 *       If the DCU is setup so that the serial devices are not in order of 
 * COM ports (IO bases 3F8, 2F8, 3E8, 2E8) , the console device mapping for 
 * the OS will not match boot's, and the console will be on a different port.
 * This will probably make the system unbootable. 
 * Since boot always maps console minors in COM1,2,3,4 compensate for the 
 * DCU/resmgr by using mapped minor device number in cn* routines.
 * Set up mapping when DCU(1M) data is retrieved.
 */

/*
 * STATIC dev_t
 * asyccnopen(minor_t, boolean_t, const char *)
 *
 * Calling/Exit State:
 *      Called from the iasycnopen(), calls iasyinit() if not already done.
 *      Allocates devices that were in the uart_scantbl[] at init_console(). 
 *
 * Remark:
 *      Can be used by kdb(1M) as well as the kernel console device.
 */

STATIC dev_t
asyccnopen(minor_t minor, boolean_t syscon, const char *params)
{
        extern major_t  iasy_major;     /* major num. of serial port device */
        int                             unit;
        int                     rval;
        uart_data_t             *up = uart_scantbl; 

        /* 
         * Set global consinit flag to indicate execution at early sysinit()
         * if we execute prior to asyc_asyinit(D2): kdb.rc / init_console.
         */

        if (!asyinitd) { 
                consinit = B_TRUE; 
                asyc_asyinit(); 
        } else 
                consinit = B_FALSE; 

        /*
         * Check that the minor specified is OK. Note that the 
         * test is against devices scanned for: we may not have 
         * run asycstart() if invoked by kdb.rc / console bootstring. 
         */ 

        if ((minor < asyc_sminor) ||
            (minor > (asyc_sminor + IASY_UNIT_TO_MINOR(asyc_nscdevs) - 1))){
                return NODEV;
        } 

        /* 
         * Verify that hardware is present. Uses the device in the 
         * space.c uart_scantbl[] as the minor/device mapping. If this 
         * is not the same as the dcu(1M) device cmn_err(). 
         */

        unit = IASY_MINOR_TO_UNIT((minor - asyc_sminor)); 
        ASSERT(unit < asyc_nscdevs); 

        /* 
         * The uart_scantbl[] has been setup at this point. We can use 
         * the minor derived unit to access the status fields. 
         */

        if (uart_scantbl[unit].status & (NOSTATE|HWERR)) {
                /*
                 *+ No port was found at the IO address, or the port
                 *+ failed the HW identification or loopback tests.
                 */
                consinit = B_FALSE; 
                return NODEV;
        }

        /*
         * Set port to specified parameters. Get unit, and setup. Don't 
         * setup the data buffers for the unit until necessary.
         */

        if (asyc_setconsole(minor, params) < 0) { 
                /* 
                 *+ Fatal error: cant set console.
                 */
                consinit = B_FALSE; 
                return NODEV; 
        } 

        /* 
         * Set to indicate console init; if syscon is not set, then we 
         * are being used by kdb(1M) not OS console. 
         */

        uart_scantbl[unit].status |= ( syscon ? SYSCON : KDBCON ); 

        /* 
         * If the terminal is being used as a debugger console by 
         * invoking 'newterm' after system boot, then the asycstart
         * routine has already run, and CONDEV will not have 
         * been set. We can set the CONDEV if the structure 
         * has been allocated, check that the base pointer is !NULL.
         */ 

        if ( asyctab ) 
                asyctab[unit].flags |= CONDEV ; 
        

        /* 
         * (Do not block for flow/modem control in sysinit).
         * Open should succeed if device is present at all. 
         */ 

        consinit = B_FALSE; 
        return (makedevice(iasy_major, minor));
}

/*
 * STATIC void
 * asyccnclose(minor_t, boolean_t)
 *
 * Calling/Exit State:
 *      - None
 * 
 * Remarks:
 */

STATIC void
asyccnclose(minor_t minor, boolean_t syscon)
{
        int             unit;

        unit = IASY_MINOR_TO_UNIT(minor - asyc_sminor);

        ASSERT(uart_scantbl[unit].status & (SYSCON|KDBCON));
        uart_scantbl[unit].status &= ~( syscon ? SYSCON : KDBCON ); 

        /* 
         * If no longer a system / debugger terminal, clear the 
         * CONDEV flag that enables special characters. 
         */

        if (asyctab && !(uart_scantbl[unit].status & (SYSCON|KDBCON))) 
                asyctab[unit].flags &= ~CONDEV;

        return;

}

/*
 * STATIC int 
 * asyccnputc(minor_t, int)
 *
 * Calling/Exit State:
 *      Return 1 on success, 0 for failure.
 *      Failure : flow blocked ?, no carrier on MODEM device.
 *
 * Description:
 *      Put a character out the console device serial port.
 *      Do not use interrupts.  If char is LF, put out LF, CR.
 */

STATIC int
asyccnputc(minor_t minor, int c)
{
        int             io, unit;               /* start i/o address */
        int             ccnt; 
        uchar_t         lsr0; 
        uart_data_t     *up = uart_scantbl;     

        unit = IASY_MINOR_TO_UNIT(minor - asyc_sminor);
        io = uart_scantbl[unit].iobase; 
        ASSERT(IASY_MINOR_TO_UNIT(minor) < asyc_nscdevs); 

#ifdef ASYC_DEBUG
        cmn_err(CE_NOTE,"!asyccnputc(), minor %d, char %c", minor, c); 
#endif 

        while (!((lsr0 = inb(ILSR(io))) & LSR_XHRE)) {

                uchar_t         msr; 

                /* 
                 * Depending on the minor specified, various control lines
                 * also require assertion. Determine which, and fail if 
                 * they are not set. Or hang ??
                 */ 

                msr = inb(IMSR(io)); 

                switch (MKHW_T(minor)) { 

                        case UWHWFC:                    /* DCD, CTS, DSR */
                                while (!((msr & MSR_CTS) && (msr & MSR_DSR))) 
                                                        ; 
                                /* FALLTHRU */

                        case SCOMDM:                    /* DCD only     */
                        case UWSWFC:
                                while (!(msr & MSR_DCD))
                                                        ; 
                                /* FALLTHRU */

                        case SCOTRM:                    /* Nothing      */ 
                        default:                                /* Error        */
                                break; 
                } 
        }
        
        /*
         * Put the character out and wait for the THR to go empty 
         * again: otherwise called to retransmit immediately.
         */

        outb(IDAT(io), c);
        
        while (!(inb(ILSR(io)) & LSR_XHRE)) 
                /* Timeout impracticable at sysinit()   */
                                ; 

        /* 
         * Same result, for OK or error (no carrier).
         */

        return (1);
}

/*
 * STATIC int
 * asyccngetc(minor_t)
 *      Get a character from the serial port with no interrupts.
 *
 * Calling/Exit State:
 *      If no character is available, return -1.
 *
 * Remarks:
 *      Run in polled mode, no interrupts.
 */

STATIC int
asyccngetc(minor_t minor)
{
        ioaddr_t        io;                     /* start i/o address */
        int             c, unit;
        uchar_t         lsr, msr; 
        uart_data_t     *up = uart_scantbl; 

#ifdef ASYC_DEBUG 

        /* Testing console handling routines    */
        static int off = 0; 
        if (asyc_debug & DSP_DOGETC) { 
                off = (off + 1)% 26;
                if (off == 0) 
                        return (-1);
                else 
                        return ('A' + off);
        }
#endif

        unit = IASY_MINOR_TO_UNIT(minor - asyc_sminor);
        io = uart_scantbl[unit].iobase; 

        ASSERT(IASY_MINOR_TO_UNIT(minor) < asyc_nscdevs); 

        /* 
         * Read line status for data received error status. If no data  
         * return -1, if the data read had a frame or parity error then 
         * disacrd it and look for more. Overrun errors are OK, since the  
         * data itself is correct (although we have lost >= 1 byte). 
         */

        while (1) { 

                lsr = inb(ILSR(io)); 

                if (!( lsr & LSR_RCA )) 
                        return (-1); 

                if (lsr & (LSR_FRMERR|LSR_PARERR) ) { 

                        (void) inb(IDAT(io)); 
                        continue; 

                } else { 
        
                        c = inb(IDAT(io));

#ifdef ASYC_FAILS_THIS_TEST

                        /* 
                         * If a minor using flow/modem control is specified then ensure 
                         * that required control lines are set. 
                         */ 

                        msr = inb(IMSR(io)); 

                        /* 
                         * Depending on the minor specified the modem carrier detect 
                         * line (DCD) may be required (indicates a remote connection).
                         * If this is not the case then the data input is invalid (it 
                         * originates locally) so we should discard/ignore it. DCD is 
                         * asynchronous to the FIFO contents - we should drain the FIFO 
                         * and keep returning -1 until the modem is connected. 
                         */ 

                        switch (MKHW_T(minor)) { 

                                /* ttyNNs/ttyNNh/ttyNNA ports are all modem control */

                                case UWHWFC:                    
                                case UWSWFC:
                                case SCOMDM:    

                                        if ( msr & MSR_DCD ) { 
                                                break ; 
                                        } else {  
                                                /* Drain FIFO (all CLOCAL data) */
                                                while ( inb(ILSR(io)) & LSR_RCA ) { 
                                                        inb (IDAT(io)); 
                                                } 
                                                return -1; 
                                        } 
                                        
                                /* ttyNNa port is CLOCAL        */

                                case SCOTRM:                    /* Nothing      */ 
                                        break; 

                                /* Unrecognised minor - kindness to strangers   */
        
                                default:
                                        break; 
                        } 

#endif 

                        /* 
                         * The console routines may be used by the kernel debugger 
                         * or the system console ( /dev/console ). Allow invocation
                         * of the debugger through the serial console special 
                         * character (Ctrl - K). If already in kdb(1M) the invocation 
                         * will succeed creating a kdb(1M) stack frame. The special 
                         * character is discarded.
                         */

#ifndef NODEBUGGER
                        if (c == DEBUGGER_CHAR) {
                                (*cdebugger)(DR_USER, NO_FRAME);
                                return (-1);    
                        }
#endif 

                        /* 
                         * Otherwise, return the data.
                         */
                        
                        return (c);
                        /* NOTREACHED */

                } /* No error in data */

        } /* while 1 */

        /* NOTREACHED */

}


/*
 * STATIC void
 * asyccnsuspend(minor_t minor)
 *      Suspend normal input processing in preparation for cngetc.
 *
 * Calling/Exit State:
 *      Called from the console handling routines to prepare the UART to 
 * do system console IO: this should be separate from any STREAMS R/W. Turn
 * interrupts off and wait until the TxFIFO has drained. 
 * 
 * Remarks:
 *
 */

STATIC void
asyccnsuspend(minor_t minor)
{
        ioaddr_t        io;     
        uchar_t         lcr,lsr;
        uint_t          unit = IASY_MINOR_TO_UNIT(minor - asyc_sminor); 
        uart_data_t     *up = uart_scantbl; 
        flags_t         efl; 


        io = uart_scantbl[unit].iobase;


        /* 
         * Block all interrupts whilst suspending console output. 
         */ 

        ASYC_CLI(efl); 

        /* 
         * Disable interrupts from UART and save interrupt state for 
         * resume. Ensure that we are not currently adjusting the Baud 
         * Rate: if so we are in trouble, as the UART has not been setup.
         */

        cons[unit].syscon_icr = inb(IICR(io)); 
        outb(IICR(io), 0);

        /* 
         * Reenable interrupts now we have disabled IRQ on UART 
         */ 
        
        while (!((lsr = inb(ILSR(io))) & LSR_XHRE)) { 
                /* 
                 * Wait for TxFIFO to drain. Ignore data in 
                 * RxFIFO. 
                 */ 
        } 

        /*
         * Ensure that there are no pending interrupts by reading 
         * the various registers.
         */

         inb(IMSR(io)); 
         inb(IISR(io)); 


         /* 
          * Drain the receiver FIFO 
          */
        
        while (inb(ILSR(io)) & LSR_RCA)
                (void) inb(IDAT(io)); 

        /* Reenable the interrupts from the rest of the system */

        ASYC_STI(efl); 

        return; 
}

/*
 * STATIC void
 * asyccnresume(minor_t minor)
 *      Resume normal input processing after cngetc.
 *
 * Calling/Exit State:
 *      None.
 * 
 * Remarks:
 *      Don't need to block interrupts in cnresume(), since UART IRQs have 
 * been disabled by the IER. 
 *
 */

/* ARGSUSED */
STATIC void
asyccnresume(minor_t minor)
{
        ioaddr_t        io; 
        uchar_t         lcr;
        uint_t          unit;
        uart_data_t     *up = uart_scantbl; 

        unit = IASY_MINOR_TO_UNIT(minor - asyc_sminor); 
        io = uart_scantbl[unit].iobase; 

        /* 
         * Restore the interrupt handling state to pre-cnsuspend. 
         * If the routines were setting up the UART their interruption 
         * will cause problems: retry or protect with LOCK/spl().
         */
                
        outb(IICR(io), cons[unit].syscon_icr);          /* Restore killed IRQs */

        return; 
}

/*
 * STATIC int
 * asyc_memalloc(void)
 *
 * Allocate memory for asyc struct depending on the type of 
 * init. 
 */

STATIC int
asyc_memalloc(void)
{

        if (asyctab) { 
                cmn_err(CE_WARN,"asyc_memalloc:asyctab already set 0x%x",asyctab);
                return 0; 
        } 

        asyctab = (asyc_t *)kmem_zalloc(asyncfg * sizeof(asyc_t), KM_NOSLEEP);
        if (!asyctab) 
                return(ENOMEM);

        asyc_bufp = (asycbuf_t *)kmem_zalloc (asyncfg * sizeof(asycbuf_t),
                                                                KM_NOSLEEP); 
        if (!asyc_bufp)
                return(ENOMEM);

        asyc_statp = (astats_t *)kmem_zalloc (asyncfg * sizeof(astats_t),
                                                                KM_NOSLEEP); 
        if (!asyc_statp)
                return(ENOMEM);


        return 0;
}

/* 
 * 
 * STATIC int   
 * asyc_parsebs(const char * parms, strtty * tp )
 * 
 * Calling/Exit State:
 * 
 * Remarks:
 *      Parse parameter string specified in console=iasy(m,p) bootstring.
 *      B<baud>|P<OE>|C<78>[T] eg. B57600|C8T into the asy_tty[] flags, 
 *      so that asycparam() can set them up.
 * 
 */

STATIC int      
asyc_parsebs(const char *pp, struct strtty *tp )
{ 
        char	*cp = (char *) pp; 
	int	baud = 0;

	SKIP_WS(cp)
	
	while (cp && *cp) {

		SKIP_WS(cp)

		FIND_KEY(cp)

		switch (*(cp++)) {

			case 'B': 
				baud = asyc_atoi(cp);
				switch(baud) {
					case 115:
						baud = 115200; 
						break;
					case 57:
						baud = 57600; 
						break;
					case 38:
						baud = 38400; 
						break;
					case 19:
						baud = 19200; 
						break;
					default:
						break;
				}

				tpsetspeed ( TCS_ALL, tp, asyc_atoi(cp) ); 
				break; 

			case 'C':
				tp->t_cflag &= ~CSTOPB; 
				if (*(cp+1) == 'T') 
					tp->t_cflag |= CSTOPB; 
                                
				tp->t_cflag &= ~CSIZE; 
				if ( *cp == '7' ) 
					tp->t_cflag |= CS7; 
				else if ( *cp == '8' ) 
					tp->t_cflag |= CS8; 
				else    
					return (-1); 

				break; 

			case 'P': 
				tp->t_cflag &= ~(PARENB|PARODD); 
				if ( *cp == 'E') 
					tp->t_cflag |= PARENB; 
				else if (*cp == 'O') 
					tp->t_cflag |= (PARENB|PARODD); 
				else 
					return (-1); 
				break; 

			default:
				return -1;
				/* NOTREACHED */
				break;
		}

			FIND_SEP(cp)

        }

        return (0); 

} 


/*
 * STATIC int 
 * asyc_atoi(char *num)
 *
 * Calling/Exit State:
 *
 * Remarks:
 *      Convert string of decimal digits into an integer. Decimal string 
 *      can terminate with any character not in 0-9 range.
 *
 */
                
STATIC int 
asyc_atoi(char *num)
{ 
        int     tot, mul; 
        char    *q = num; 

        /* Find end of number */
        while ((*q >= '0') && (*q <= '9')) 
                q++; 

        /* Any digits at all ?  */
        if (q == num)
                return (0); 

        /* Back to last digit   */
        q--; 
        tot = 0;        
        mul = 1; 
        do { 
                tot += ((*q - '0') * mul);
                mul *= 10; 
                q--;
        } while (q >= num); 

        return (tot); 
} 

/* 
 * STATIC boolean_t 
 * asyc_uartpresent(ioaddr_t io); 
 * 
 * Calling: 
 * 
 * Remarks: 
 * 
 */ 

STATIC boolean_t 
asyc_uartpresent(ioaddr_t io)
{ 
        uchar_t         isr0,mcr0; 

        /* 
         * Verify the unit is present. Do ICR echo test first. 
         */ 

        isr0 = inb(IICR(io));  

        outb(IICR(io), 0);      

        if (inb(IICR(io)) != 0) { 
#ifdef ASYC_DEBUG 
                if (!consinit)
                        cmn_err(CE_WARN,"!UART at %x , No ICR echo",io); 
#endif
                return (B_FALSE); 
        } 

        outb(IICR(io), isr0);   

        /* 
         * Do loopback of DTR/RTS/OUT1/OUT2 lines test. 
         */ 

        mcr0 = inb(IMCR(io));  

        outb(IMCR(io),(MCR_DTR|MCR_RTS|MCR_OUT1|MCR_OUT2|MCR_LBK)); 

        if ((inb(IMSR(io)) & 0xF0) != 0xF0) {
#ifdef ASYC_DEBUG 
                if (!consinit)
                        cmn_err(CE_WARN,"!UART at %x , No MCR->MSR echo",io); 
#endif
                outb(IMCR(io),mcr0); 
                return B_FALSE;
        } 

        outb(IMCR(io),mcr0); 

        return B_TRUE; 
        
} 

 /* 
 * STATIC int           
 * asyc_asytabinit(uint_t unit); 
 * 
 * Calling/Exit State:
 *      Requires that the asyc_iobase field set. Sets up the asyc_t and 
 *      asycbuf_t data for the dcu(1M) specified unit. Sets permanent data
 *      and data that is accessible through ioctl(2) only. Called from 
 *      asycstart() after asyc_dat & asyc_vect set, and after all presence 
 *      tests are complete.
 * 
 * Remarks:
 *      Initialise data in device's asytab[] entry.
 * 
 */

STATIC int              
asyc_asytabinit(uint_t unit)
{ 
        
        asyc_t          *ap;
        asycbuf_t       *bp;
        astats_t        *sp; 
        ioaddr_t        io; 
        uart_data_t     *up; 

        ap = &asyctab[unit]; 
        bp = &asyc_bufp[unit]; 
        sp = &asyc_statp[unit];

        up = &uart_scantbl[unit]; 

        /* 
         * Set the pointers to internal structures.
         */

        ap->asyc_bp = bp; 
        ap->asyc_stats = sp; 

        /* 
         * Set the IO addresses for the registers. Per link_unix.
         */
        io = ap->asyc_dat; 

        ap->asyc_isr = IISR(io); 
        ap->asyc_icr = IICR(io); 
        ap->asyc_lcr = ILCR(io); 
        ap->asyc_mcr = IMCR(io); 
        ap->asyc_lsr = ILSR(io); 
        ap->asyc_msr = IMSR(io); 

        /* 
         * Call the type detection to set the UART type. 
         */ 

        if ((ap->asyc_utyp == NOTYPE) && (up->uart != NOTYPE)) { 
                ap->asyc_utyp = up->uart; 
        } 
        
        switch (ap->asyc_utyp)  {

                case NS16550:
                        ap->txfifo = 16; ap->rxfifo = 14; 
                        break; 

                case ASY16650:
                        ap->txfifo = ap->rxfifo = 32; 
                        break; 

                case ASY16750:
                        ap->txfifo = ap->rxfifo = 64; 
                        break; 

                default:        
                /* UNKNOWN use vanilla features. */
                case NS8250:
                        ap->txfifo = ap->rxfifo = 1; 
                        break; 

                case NS16450:
                        ap->txfifo = ap->rxfifo = 1; 
                        break; 
        } 


        /* 
         * Set the buffer watermarks; choose reasonable defaults if the 
         * tunables are out of sensible range (10% - 90%).
         */ 
        
        if ((asyc_iblowat > 90) || (asyc_iblowat < 10)) { 
                cmn_err(CE_WARN,"Bad buffer low water mark (asyc_iblowat)"); 
                bp->ibuf_lowat = (60 * ISIZE )/100 ; 
        } else 
                bp->ibuf_lowat = (asyc_iblowat * (ISIZE - 1)) / 100 ; 

        if ((asyc_ibhiwat > 90) || (asyc_ibhiwat < 10)) { 
                cmn_err(CE_WARN,"Bad buffer high water mark (asyc_ibhiwat)"); 
                bp->ibuf_hiwat = (70 * ISIZE )/100 ; 
        } else 
                bp->ibuf_hiwat = (asyc_ibhiwat * (ISIZE - 1)) / 100 ; 

        bp->obuf_hiwat = ((asyc_obhiwat * (OBUFSZ - 1)) / 100) - 1; 
        bp->obuf_lowat = ((asyc_oblowat * (OBUFSZ - 1)) / 100) - 1; 

        /* 
         * Set the initial value of the timeout counter to -1, disabled.
         * Set the initial value of BREAK \0 tag to -1, disabled.
         */

        bp->txtout      = -1; 

        /* 
         * Set receiver FIFO trigger value.
         * Set transmitter burst FIFO load size 
         */

        if ((up->rxfifo > 0) && (up->rxfifo <= ap->rxfifo))
                ap->rxfifo = up->rxfifo; 

        if ((up->txfifo > 0) && (up->txfifo <= ap->txfifo))
                ap->txfifo = up->txfifo; 

#ifdef MERGE386 
        ap->mrg_data.baseport = ap->asyc_dat; 
#endif

        /* 
         * Set the msrlaststate variable to the current MSR: the 
         * ISR should track following MSR changes.
         */

        ap->asyc_msrlaststate = inb(IMSR(io)); 

        return (0); 
} 

/* 
 * 
 * STATIC int           
 * asyc_initporthw(uint_t unit)
 * 
 * Calling/Exit State:
 *      Expects the asyc_t asyc_dat field to be set.
 * 
 * Remarks:
 *      Initialise UART state : clear any residual bytes in the FIFO, clear
 *      any pending IRQ events. Set DTR/RTS/OUT2 (in console devices ).
 */

STATIC int              
asyc_initporthw(uint_t unit)
{ 
        ioaddr_t        io = asyctab[unit].asyc_dat; 
        int             ccnt = 0, rtg, ttg; 

        if (asyctab[unit].flags & CONDEV) { 
                cmn_err(CE_NOTE,"!asyc_initporthw: Unit %d is console", unit); 
                return (-1); 
        }       

		/* 
		 * Disable interrupt sources, prior to clearing the interrupt 
		 * sources. Clear lower nibble : IRQ source enable flags.
		 */

		outb(IICR(io),(inb(IICR(io)) &0xF0));

        /* 
         * Drain any pending FIFO events 
         */

        while ((inb(ILSR(io)) & LSR_RCA) && (ccnt++ < 20))      /* Per open(2) */
                (void)inb(IDAT(io)); 

        /* Dropped out of loop via counter limit -> error       */

        if (ccnt == 20) { 
                uart_scantbl[unit].status = HWERR; 
                cmn_err(CE_WARN,"asyc_initporthw: Drain FIFO HWERR"); 
                return (-1); 
        } 

        /* 
         * Set the UART FIFO trigger level and enable the FIFOs. The 
         * trigger levels are set to the highest possible values. Note that 
         * setting the FIFO reset/enable bits will generate a TxRDY IRQ.
         */

        if ( asyc_setFIFO ( unit ) < 0 ) { 
                cmn_err(CE_WARN,"asycparam: asyc_setFIFO failed, unit %d", unit ); 
        } 

        /* 
         * Read / clear ISR for line status register events and modem 
         * status register events.
         */ 
        
        (void) inb(ILSR(io));           
        (void) inb(IMSR(io));   

        /* 
         * Read the IIR register to clear any pending THRE IRQs
         * Clear the Tx FIFO, 
         */

        (void) inb(IISR(io)); 

        /* 
         * Enable DTR/RTS and IRQ line (in case of console). 
         */

        (void) outb(IMCR(io), (MCR_DTR|MCR_RTS|MCR_OUT2));      
        
        return (0); 
} 

/* 
 * 
 * STATIC int           
 * asyc_setconsole(minor_t minor, const char * parms);  
 * 
 * Calling/Exit State:
 *      Parse the parameter string argument and use it to set the UART mode 
 *      if valid, else fall back to the default parameters. Can be NULL ptr.
 *      Set the defaults from the uart_scantbl[] entry.
 * 
 * Remarks:
 *              The asyctab[] may not have been assigned at this point.    
 */

STATIC int              
asyc_setconsole(minor_t minor, const char * parms)
{ 
        uint_t                  unit = IASY_MINOR_TO_UNIT(minor - asyc_sminor); 
        struct strtty   *tp = &asy_tty[unit] ; 
        uart_data_t             *up = &uart_scantbl[unit]; 
        char                    *c, *pp;

#ifdef ASYC_DEBUG 
        if (asyc_debug & DSP_FUNCS)
                cmn_err(CE_CONT,"!asyc_setconsole\n");
#endif 

        if (!parms) { 
                tp->t_cflag = B9600|CS8|CLOCAL; 
        } else if (asyc_parsebs(parms, tp) < 0) { 
                tp->t_cflag = B9600|CS8|CLOCAL; 
        }

        /* 
         * Enable the FIFOs if possible.
         */
        
        if ( asyc_setFIFO(unit) < 0 ) { 
                if (!consinit)
                        cmn_err(CE_WARN,
                                "!asyc_setconsole: asyc_setFIFO failed on unit %d", unit); 
        }

        /* 
         * Call asycparam to set the strtty flags' state in the UART.
         */

        if ( asycparam(tp) < 0 ) { 

#ifdef ASYC_DEBUG 
                /* 
                 *+ Fatal error: cant set serial port mode.
                 */
                if (!consinit) 
                        cmn_err(CE_WARN,"!asyc_setconsole setmode FAIL");
#endif
                return (-1); 
        } 
        
        return (0); 
}       
        
/* 
 * 
 * STATIC int           
 * asyc_loopback(ioaddr_t io);
 * 
 * Calling/Exit State:
 *      Called from asyc_findports() - do a loopback test. Return 
 *      INIT for a good result, HWERR or ABSENT if test failed.
 * 
 * Remarks:
 *      Tests RTS/CTS and Tx->Rx data paths. 
 *      Leaves UART in non-loopback quiescent state,
 */

STATIC uart_state_t
asyc_loopback(ioaddr_t io)
{ 
        uchar_t         msr0, mcr0, rc;
        int             ccnt = 0; 
        clock_t         char_time;

        /* 
         * Set loopback mode on, set DTR/RTS, delay for MCR->MSR 
         */ 

        outb(IMCR(io),(MCR_LBK|MCR_RTS|MCR_DTR)); 


        /* 
         * Verify the CTS/RTS and DSR/DTR loopbacks are on. 
         */ 

        msr0 = inb(IMSR(io)); 

        if (!(msr0 & MSR_CTS) || !(msr0 & MSR_DSR)) {
                /* 
                 *+ Loopback failure: CTS = RTS, DSR = DTR, and DTR,RTS on.
                 */ 
#ifdef ASYC_DEBUG
                if (!consinit)
                        cmn_err(CE_WARN,"!RTS/DTR ON loopback error at %x",io); 
#endif
                        
                return (HWERR); 
        } 
        
        /* 
         * Disable RTS, DTR - expect CTS, DSR disassertion. 
         */
        
        outb(IMCR(io),MCR_LBK); 
        
        msr0 = inb(IMSR(io)); 

        if ((msr0 & MSR_CTS) || (msr0 & MSR_DSR)) {
                /* 
                 *+ Loopback failure: CTS = RTS, DSR = DTR, and DTR,RTS off.
                 */ 
#ifdef ASYC_DEBUG
                if (!consinit)
                        cmn_err(CE_WARN,"!RTS/DTR OFF loopback error at %x",io); 
#endif
                return (HWERR); 
        } 
        
        /* 
         * Loopback worked OK. Reset to quiescent state and exit.
         */ 

        outb(IMCR(io), 0);      /* ~DTR, ~RTS , ~OUT2, ~LBK     */
        return (INIT);          /* Passed test - UART OK        */ 

} 

/* 
 * 
 * STATIC uart_type_t
 * asyc_uarttype(ioaddr_t io)
 * 
 * Calling/Exit State:
 *      Called at device scan , xxinit() or console_init(). Disrupts UART
 *      operation. 
 * 
 * Remarks:
 *      Test for latest (most back-compatible first).
 *      Destroys FIFO state;
 *
 */ 

STATIC uart_type_t
asyc_uarttype(ioaddr_t io)
{ 
        uchar_t         lcr0, fcr0;


        /* 
         * TI's TL16C750 has a 64B FIFO mode: test for that first.
         * The write to FCR_64E requires the DLAB set.
         */

        lcr0 = inb(ILCR(io)); 
        outb(ILCR(io), (lcr0 | LCR_DLAB));
        outb(IFCR(io), (FCR_FEN|FCR_64E) ); 
        outb(ILCR(io), (lcr0 & ~LCR_DLAB));

        if ((inb(IISR(io)) & (ISR_FEN|ISR_64E)) == (ISR_FEN|ISR_64E)) { 
#ifdef ASYC_DEBUG
        if (asyc_debug & DSP_TYPE)
                cmn_err(CE_NOTE,"!asyc_uarttype: TI16C750 UART at %x",io);
#endif
                return (ASY16750);
        } 

        /* 
         * 16550 UARTs power up in 16450 mode. Attempt to set FIFO
         * control and check for FIFO state bit changes. 
         */ 

        outb(IFCR(io), FCR_FEN); 
        if ((inb(IISR(io)) & ISR_FEN) == ISR_FEN) {
#ifdef ASYC_DEBUG
        if (asyc_debug & DSP_TYPE)
                cmn_err(CE_NOTE,"!asyc_uarttype: 16550 UART at %x",io);
#endif
                outb(IFCR(io),0);       /* Disable FIFOs */
                return (NS16550);
        } 

        /* 
         * If bits not set, device is an 8250/16450 
         */

#ifdef ASYC_DEBUG
        if (asyc_debug & DSP_TYPE)
                cmn_err(CE_NOTE,"!asyc_uarttype: 16450/8250 UART at %x",io);
#endif
        return (NS16450); 
} 
        
/* 
 * STATIC int           
 * asyc_setFIFO(uint_t unit)
 * 
 * Calling/Exit State:
 * 
 * Remarks:
 *      If the trigger  level is set to 0 => disable FIFOs. 
 *      Can be called from pre/post start(D2): use uart_scantbl/asyctab[].
 */

STATIC int              
asyc_setFIFO(uint_t unit)
{ 
        uchar_t         fctl = 0;       
        uart_type_t     uarthw;
        ioaddr_t        io; 
        int                     rxfifo = -1, txfifo = -1; 
        uart_type_t     utype; 

        /* 
         * For console (early) calls, test unit against those scanned 
         * in the uart_scantbl[] (asyc_nscdevs), for later calls against 
         * the DCU configured (asyncfg) devices.
         * Ie. If Post DCU, asyncfg > 0 && unit >= asyncfg 
         * else (pre-DCU) (asyncfg == 0) && (unit > asyc_nscdevs) 
         */

        if ((asyncfg && (unit >= asyncfg)) || (unit > asyc_nscdevs)) {
                if (!consinit) { 
                        cmn_err(CE_WARN,"asyc_setFIFO: Bad unit %d: no FIFO setting", unit);
                        cmn_err(CE_WARN, "!asyc_setFIFO: asyncfg %d: asyc_nscdevs %d", 
                                                                                                        asyncfg, asyc_nscdevs); 
                }       
                return (-1);
        } 

        /* 
         * Verify the device has a FIFO to set, and check the 
         * value to set and the upper value allowed.
         * '750 can do 64,32,16,1 as well as '550 compat. => 
         * set to 64B FIFO mode and set as for 1,4,8,14
         */ 
        
        if (asyctab) { 

                /* 
                 * Use asyctab values, post DCU configuration at start(D2).
                 */

                io = asyctab[unit].asyc_dat; 
                rxfifo  = asyctab[unit].rxfifo; 
                txfifo  = asyctab[unit].txfifo; 
                utype = asyctab[unit].asyc_utyp; 

        } else { 

                /* 
                 * Use scantbl values at/before init(D2) time.
                 */

                io = uart_scantbl[unit].iobase; 
                rxfifo  = uart_scantbl[unit].rxfifo; 
                txfifo  = uart_scantbl[unit].txfifo; 
                utype = uart_scantbl[unit].uart; 

        } 

#ifdef ASYC_DEBUG 
        if (asyc_debug & DSP_FIFO)
                cmn_err(CE_CONT,"setFIFO Type: %d RxTL:%d, TxTL:%d, io: 0x%x\n", 
                                                                                                utype, rxfifo, txfifo,io ); 
#endif

        switch (utype) { 

                default:
                case NOTYPE:
                case NS8250 :
                case NS16450 :
                        if ((rxfifo > 1) || (txfifo > 1))
                                cmn_err(CE_WARN,"rxFIFO (%d)/txFIFO (%d) ~FIFO UART (unit %d)",
                                                        rxfifo, txfifo, unit ) ; 
                        if (!rxfifo || !txfifo )
                                cmn_err(CE_WARN," Zero rx (%d)/tx (%d) byte trigger (unit %d)",
                                                        rxfifo, txfifo, unit ) ; 
                        rxfifo = txfifo = 1; 
                        break; 

                /* 
                 * UARTs over 550 have different trigger level setting methods.
                 * Leave them as 16550 compatible until we have hardware to test 
                 * on.
                 */

                case NS16550:
                case ASY16650:
                case ASY16750:
                        switch (rxfifo) {       
                                case 1:
                                case 4:
                                case 8:
                                case 14:
                                        break; 
                                default: 
                                        /* FALLTHRU */
                                case 16: 
#ifdef ASYC_DEBUG 
                                        if (asyc_debug & DSP_FIFO)
                                                cmn_err(CE_WARN,"Set rx FIFO = 14, unit %d,io 0x%x",
                                                                        unit, io);
#endif 
                                        rxfifo = 14;  
                                        break; 
                        } 

                        if ( (txfifo < 0) || (txfifo > 17) ) { 
#ifdef ASYC_DEBUG 
                                if (asyc_debug & DSP_FIFO)
                                        cmn_err(CE_WARN,"Set tx to 15, was %d, unit %d, io 0x%x\n",
                                                                txfifo, unit, io); 
#endif 
                                txfifo = 15;  
                        } 
                        break; 
        } 

#ifdef ASYC_DEBUG 
                if (asyc_debug & DSP_FIFO)
                        cmn_err(CE_CONT,"\nasyc_setFIFO: set unit %d FIFO rx %d / tx %d\n", 
                                                                                unit, rxfifo, txfifo); 
#endif 

        switch (rxfifo) { 

                /* 
                 * Setting a depth of 0 disables the FIFOs
                 */
                case 0:         
                        outb(IFCR(io), 0); 
                        break; 

                case 1:
                        outb(IFCR(io), FCR_FE1);
                        break;

                case 16:
                        fctl = FCR_64E; 
                        /* FALLTHRU */

                case 4:
                        outb(IFCR(io), (fctl|FCR_FE1|FCR_RTRL)); 
                        break;

                case 32:
                        fctl = FCR_64E; 
                        /* FALLTHRU */

                case 8:
                        outb(IFCR(io), (fctl|FCR_FE1|FCR_RTRH)); 
                        break;

                case 64:
                        fctl = FCR_64E; 
                        /* FALLTHRU */

                case 14:
                        outb(IFCR(io), (fctl|FCR_FE1|FCR_RTRL|FCR_RTRH)); 
                        break;

                default: 
                        return(-1);     /* Bad selection for ANY UART */
                        /* NOTREACHED */
                        break;
        } 
        return (0); 
} 


/* 
 * STATIC boolean_t asyc_clocal ( strtty * tp )
 * 
 * Call/Exit:
 *      All over the place, wherever the driver actions depend on the 
 *      CLOCAL control mode. 
 *
 * Remarks:
 *      Called whenever the driver action is determined by the state of 
 *      the CLOCAL flag. Generally this is used to filter the state of 
 *      the DCD input line, set by MODEMs when a carrier is detected, ie. 
 *      when a remote system is accessible. 
 *      This function returns a boolean_t to simulate the CLOCAL state and
 *      bugs in the SCO Openserver driver, whereby the MODEM serial port 
 *      node allowed R/W to a MODEM after open(2) whatever the DCD state 
 *      was. ( XPG4 states that R/W to a port with !CLOCAL and no DCD line 
 *      (ie. a MODEM with no remote connection) should fail/set EIO.
 */

STATIC boolean_t 
asyc_clocal ( struct strtty *tp )
{ 
        int             unit = tp - &asy_tty[0] ;       

        /* 
         * Device really is a CLOCAL device, hence CLOCAL is always set. 
         */ 

        if ( tp->t_cflag & CLOCAL ) 
                return ( B_TRUE ); 

        /* 
         * Device is already OPEN, set to be compatible with SCO OSr5 
         * and thus should act as if CLOCAL set. Return CLOCAL. SCO 
         * mode ignores DCD when the device is open, but open(2) blocks.
         */

        if (( tp->t_state & ISOPEN ) && SCOMODEM_COMPAT(unit))  { 

                /* port is open and tunable is set */

#ifdef ASYC_DEBUG 
                if ( asyc_debug & DSP_SPACE )
                        cmn_err(CE_NOTE,
                                "Port tp = 0x%x in SCO modem compatibility mode",tp);
#endif 

                return ( B_TRUE ); 
        } 

        /* 
         * Else, port not yet open, or not set to be compatible, (and 
         * CLOCAL bit not set in mode) so return false: check DCD state.
         */

        return ( B_FALSE );
} 


/*
 * STATIC void asyc_loadtxbuffer ( uint_t unit )
 * 
 * Calling/Exit State:
 *              Called from asycpoll at CLI. asycpoll invoked bt itimeout(D3)
 *      hence interrupted IPL == plbase, and STREAMS runqueues not executing,
 *      thus STREAMS queues are consistent and call to iasy_output (getq) OK.
 *
 * Remarks:
 *              Pushes data downstream as far as tunables allow, ie. from the WR(q)
 *      to the ISR obuffer. Copy in blocks for speed, limit at high water marks 
 *      or rldcnt (loops) to prevent huge delays as startup fills obuf, since 
 *      CLI locks all IRQ sources.
 *              Note that calling iasy_output() when the STREAMS queue between iasy
 *      and asyc is empty (there is no data in the strtty buffer or the call would
 *      corrupt it) will cause a sleeping iasyclose() to continue. Don't call 
 *      iasy_output() whilst waiting for a close to complete unless all the data 
 *      in the output buffer has been sent: then asyc_idle() can do it. 
 */

STATIC void 
asyc_loadtxbuffer ( uint_t unit )
{ 
        asyc_t                  *asyc ; 
        asycbuf_t               *ap ; 
        struct strtty   *tp ; 
        int                             nloop = 0;                      
        int                             obtohw, oblinfree, ccnt;   

        tp = IASY_UNIT_TO_TP(asyc_id, unit); 
        asyc = &asyctab[unit]; 
        ap = asyc->asyc_bp; 

        /* 
         * Test exit condition of obuf reaches high water.
         */

        while ((obtohw = (ap->obuf_hiwat - OBUFCNT(ap))) > 0) { 

                ASSERT(asyc_rldcnt > 0); 

                if (!(++nloop < asyc_rldcnt)) { 
                        return; 
                }

                /* 
                 * Calculate the block size to copy, minimum of space to 
                 * high water, space to end of buffer, and bytes in STREAMS 
                 * M_DATA block. Since high water < full, and full is (oget-1)
                 * the high water value ensures we don't overwrite unread.
                 */

                ASSERT ( ap->oput < OBUFSZ ); 
                oblinfree = OBUFSZ - ap->oput; 

                /* 
                 * If we wrap before we fill the buffer
                 * take the data up to the wrap.
                 */

                ccnt = ((oblinfree < obtohw) ? oblinfree : obtohw) ; 
                ccnt = ((tp->t_out.bu_cnt < ccnt) ? tp->t_out.bu_cnt : ccnt);

#ifdef ASYC_DEBUG 
                if (asyc_debug & DSP_TXSMB)
                        cmn_err(CE_NOTE,"!ldtxb: tp %d linfree %d tohw %d ccnt %d", 
                                                tp->t_out.bu_cnt, oblinfree, obtohw, ccnt );
#endif 

                /* 
                 * If ccnt is 0, because the strtty t_out buffer is empty 
                 * then get another one. (Needs to be called at plstr from 
                 * interrupted IPL < plstr for STREAMS to be consistent).
                 */

                if (ccnt == 0) { 

                        /* 
                         * Pre copy, loop entry requires obuf is less than high 
                         * water, oput index updates modulus OBUFSZ always return 
                         * an index < OBUFSZ, hence the only possible source for 
                         * a character count of zero is an empty STREAMS buffer. 
                         * Check possibility of it being invalid too.
                         */

                        ASSERT(!tp->t_out.bu_cnt || !tp->t_out.bu_bp); 

                        /* 
                         * If TTIOW is set, the iasy(7) module close(2) has been 
                         * called and the driver is waiting for data to drain before
                         * setting device state to 0. Don't allow calls to iasy_output
                         * until all data has been sent, in this case. This will be 
                         * scheduled by asyc_idle() or an asycpoll() safety net timeout
                         * event (if the asyc_idle() cannot be scheduled. 
                         */

                        if (tp->t_state & TTIOW) 
                                return; 

                        /* 
                         * Call iasy_output to get more data (may return !CPRES ie. no
                         * data available) and exit if none available, else bump loop
                         * count and restart the loop.
                         */

                        if (iasy_output(tp) & CPRES) 
                                continue; 

                }

                /* 
                 * Partial/full copy from STREAMS buffer to ISR buffer.
                 * Update strtty pointers, and OBUF pointer.
                 */

                bcopy ((caddr_t) tp->t_out.bu_ptr, (caddr_t) &ap->obuf[ap->oput], ccnt); 
                /* 
                 * Verify update values for STREAMS message integrity. Bytes copied
                 * not greater than bytes in message, end of obuf, or msg wptr.  
                 */

                ASSERT(!(ccnt > tp->t_out.bu_cnt)); 
                ASSERT((ap->oput + ccnt - 1) < OBUFSZ); 
                ASSERT ( !tp->t_out.bu_bp ||    \
                        (tp->t_out.bu_bp->b_wptr >= (tp->t_out.bu_ptr + ccnt))); 


                tp->t_out.bu_cnt -= ccnt ; 
                tp->t_out.bu_ptr += ccnt ; 
                ap->oput = ((ap->oput + ccnt) % OBUFSZ); 

        } 

        /* 
         * Arrive here if the obuf is at high water. Try and top up the 
         * strtty buffer if it's empty.
         */

        if (!(tp->t_state & TTIOW))
                if (!tp->t_out.bu_cnt || !tp->t_out.bu_bp)
                        (void) iasy_output(tp);

        return;

}

/*
 * STATIC boolean_t asyc_chktunevars ( void )
 *
 * Calling/Exit State:
 *      Called by init(D2), no IRQs enabled.
 *      
 * Remarks:
 *      Ensures that tunable data is sensible and patch any garbage.
 */

STATIC boolean_t 
asyc_chktunevars ( void )
{ 
        boolean_t       changed = B_FALSE; 

        /* 
         * asyc_rldcnt > 0 
         * asyc_rldcnt < OBUFSZ (ISR OP buffer size)
         * Setting very low values will cause asyc_loadtxbuffer to 
         * exit after few bytes: inefficient, but possible.
         */

        if (asyc_rldcnt > OBUFSZ) { 
#ifdef ASYC_DEBUG 
                cmn_err(CE_WARN,"Limited asyc_rldcnt to OBUFSZ (%d)", OBUFSZ);
#endif 
                asyc_rldcnt = OBUFSZ; 
                changed = B_TRUE; 
        } 

        MK_VALID_VAR(asyc_obhiwat, 100, asyc_iblowat, 90); 
        MK_VALID_VAR(asyc_iblowat, asyc_obhiwat, 0, 60); 

        return (changed) ; 
}

/* 
 * STATIC void asyc_setxtout ( asyc_t *, speed_t s ) 
 * 
 * Calling/Exit State: 
 *      Nothing held. Called from asyc_txsvc() at CLI.
 *
 * Remarks:
 *      Set the txtout transmit timeout value in the asyc_t structure 
 *      from baud rate, Tx FIFO size, asycpoll frequency. Round to 
 *      decrement unit multiple. Store the per character time also.
 */ 

STATIC void 
asyc_settxtout(asyc_t *asyc, speed_t s)
{ 
        uint_t          tpoll; 
        ulong_t         totime, fpoll; 
        uint_t          tout, i;
        asycbuf_t       *ap = asyc->asyc_bp; 

#ifdef ASYC_DEBUG 
        if (asyc_debug & DSP_FUNCS)
                cmn_err(CE_NOTE,"asyc_setxtout");
#endif

        /*
         * May be called before asycstart(D2) allocates asyctab space.
         */

        if (!asyctab)
                return; 

        ap = asyc->asyc_bp; 

        /* 
         * asyc_chartime(baud) is us.char, txfifo is Tx FIFO depth.
         * totime = total tx duration in us. 
         * Max value = 110Baud on a 750 (64B FIFO) = ~7s
         */

        ap->txtchar = asyc_chartime(s); 

        /* 
         * Fill in the timeout delay array, so that imcomplete FIFO fills 
         * do not have to calculate their own timeouts every time. 
         */

        fpoll = (asyc_pollfreq != 0) ? asyc_pollfreq : 20000; 

        for (i = 0; i < NTOUTS; i++ ) { 

                totime = ap->txtchar * i;
                ap->txtoval[i] = (totime/fpoll) + TXTOFF + 1;

        }

        return; 
}

/* 
 * STATIC char * 
 * asyc_getUARTdevname(uart_type_t devid)
 * 
 * Calling/Exit State: 
 *      Called from asycstart. Gets/relinquishes no locks, no IPL changes.
 *
 * Remarks: 
 *      Convert the UART type field into a device string name.
 */

STATIC char * 
asyc_getUARTdevname(uart_type_t devid)
{ 
        switch (devid) { 
                case NOTYPE:
                        return("Unknown type");
                        /* NOTREACHED */
                        break; 
                case NS8250:
                        return("NS8250");       
                        /* NOTREACHED */
                        break; 
                case NS16450:
                        return("NS16450");      
                        /* NOTREACHED */
                        break; 
                case NS16550:
                        return("NS16550");      
                        /* NOTREACHED */
                        break; 
                case ASY16650:
                        return("ASY16650");     
                        /* NOTREACHED */
                        break; 
                case ASY16750:
                        return("ASY16750");     
                        /* NOTREACHED */
                        break; 
                default: 
                        return("Unknown type");
                        /* NOTREACHED */
                        break; 
        }
        /* NOTREACHED */
}

/* 
 * DEBUG routines 
 * 
 * Leave the display data routines (asyc_ (asyc/dump/fcstate/scan) ) in 
 * the driver : they only waste a little space, and they will be very 
 * useful for debugging in the preproduction stage. Define these as  
 * #ifdef ASYC_DEBUG_PROD, the rest are ASYC_DEBUG.
 */

#ifdef ASYC_DEBUG_PROD

/*
 * void
 * asyc_dump(uint_t)
 *
 * Calling/Exit State:
 *      None.
 * 
 * Remarks:
 *      For debugging in kdb(1M); invike from the kdb command line 
 * with 
 * <unit> asyc_dump 1 call 
 *
 */

void
asyc_dump (uint_t unit)
{
        struct strtty   *tp;
        asycbuf_t               *ap;
        asyc_t                  *asyc;
        astats_t                *sp; 
        
        if (unit >= asyncfg)
                return;

        tp = IASY_UNIT_TO_TP(asyc_id, unit);
        asyc = &asyctab[unit];
        ap = asyctab[unit].asyc_bp;
        sp = asyctab[unit].asyc_stats; 
        
        /* Structure addresses */
        cmn_err(CE_CONT, "\n\tUnit %d ",unit); 
        cmn_err(CE_CONT, "asyc = 0x%x\tasyc_aux = 0x%x",asyc,ap);
        cmn_err(CE_CONT, "\n\tasyc_stats = 0x%x\tstrtty = 0x%x\n" , sp, tp);

        /* ISR Buffer Data */
        cmn_err(CE_CONT, "\tibuf1_active = %s", ap->ibuf1_active?"TRUE":"FALSE");
        cmn_err(CE_CONT, "\n\tibuf1=0x%x",ap->ibuf1);
        cmn_err(CE_CONT, "\tiput1=0x%x, \tiget1=0x%x\n",
                        ap->iput1, ap->iget1);

        cmn_err(CE_CONT, "\tibuf2=0x%x", ap->ibuf2);
        cmn_err(CE_CONT, "\tiput2=0x%x, \tiget2=0x%x\n",
                        ap->iput2, ap->iget2);
        cmn_err(CE_CONT,"\tibuf_lowat = %d,\tibuf_hiwat = %d\n", 
                                                                ap->ibuf_lowat, ap->ibuf_hiwat ); 
        
        /* Output buffer data - downstream */

        cmn_err(CE_CONT, "\tt_out.bu_ptr=0x%x,\tt_out.bu_cnt=0x%x\n", 
                                tp->t_out.bu_ptr, tp->t_out.bu_cnt ); 

        cmn_err(CE_CONT, "\tt_out.bu_bp=0x%x\n",tp->t_out.bu_bp);

        cmn_err(CE_CONT,"\toput = 0x%x oget = 0x%x ocnt = 0x%x\n\t&obuf = 0x%x\n",
                                        ap->oput, ap->oget, OBUFCNT(ap), &ap->obuf[0] ); 

        cmn_err(CE_CONT,"\ttxtout = 0x%x, txtoval[8] = 0x%x char = %d us\n", 
                                        ap->txtout, ap->txtoval[8], ap->txtchar ); 

        cmn_err(CE_CONT,"\tobuf_lowat = %d, obuf_hiwat = %d, obrkidx = %d\n", 
                                                                ap->obuf_lowat, ap->obuf_hiwat, ap->obrkidx ); 

        /* Input buffer data - upstream */

        cmn_err(CE_CONT, "\tt_in.bu_ptr=0x%x,\tt_in.bu_cnt=0x%x\n", 
                                tp->t_in.bu_ptr, tp->t_in.bu_cnt ); 

        cmn_err(CE_CONT, "\tt_in.bu_bp=0x%x,\tinput chars=%d\n",
                                                        tp->t_in.bu_bp,IASY_BUFSZ - tp->t_in.bu_cnt);


        return;

}

/*
 * void 
 * asyc_ascyt ( unit ) 
 * 
 * Calling/Exit State: 
 *              None.
 * 
 * Remarks: 
 *      Displays the asyc_t data structure for a given unit. 
 * 
 */

#define _PASYCFLG( _a ) if( flg & (_a)) cmn_err(CE_CONT, " " #_a ); 
 
void 
asyc_asyc (uint_t unit) 
{ 

        asyc_t                  *ap;
        ulong_t                 flg, iob, irq; 
        int                             ioaddr_err = 0;

        if (unit >= asyncfg) { 

                /* Check unit sensible */

                cmn_err(CE_NOTE, "unit number %d >  configured units (asyncfg %d)", 
                                                unit, asyncfg); 

        } else { 

                /* Check pointer valid */

                if ( !(ap = &asyctab[unit]) ) { 
                        cmn_err (CE_WARN,"asyc_t pointer (asyctab[%d]) NULL", unit ); 
                        return ; 
                } 

                /* 
                 * Print operating mode flags 
                 */

                cmn_err(CE_CONT,"Unit %d",unit); 

                /* Check UART base COM1.4       */

                switch (ap->asyc_dat) { 
                        
                        case 0x3F8:
                                cmn_err(CE_CONT," COM1 " );
                                irq = 4; 
                                break; 

                        case 0x2F8:
                                cmn_err(CE_CONT," COM2"  );
                                irq = 3; 
                                break; 

                        case 0x3E8:
                                cmn_err(CE_CONT," COM3 " );
                                irq = 0;        /* No standard IRQ assignation */
                                break; 

                        case 0x2E8:
                                cmn_err(CE_CONT," COM4 " ); 
                                irq = 0;        /* No standard IRQ assignation */
                                break; 

                        default:
                                cmn_err(CE_WARN,"IO != COM1 to COM4 : 0%x\n",ap->asyc_dat);
                                irq = 0;        /* No standard IRQ assignation */
                                break; 
                } 

                iob = ap->asyc_dat; 

                /* 
                 * Check control/status registers as expected. Still not 
                 * necessarily an error if they diverge, but very likely 
                 * since no known UART exists with same register set in 
                 * different order to NS8250.
                 */

                if ( ap->asyc_icr != (iob + 1)) {
                        cmn_err(CE_CONT,"asyc_icr = 0x%x", ap->asyc_icr); 
                        ioaddr_err++; 
                } 
                if ( ap->asyc_isr != (iob + 2)) { 
                        cmn_err(CE_CONT,"asyc_isr = 0x%x", ap->asyc_isr); 
                        ioaddr_err++; 
                } 
                if ( ap->asyc_lcr != (iob + 3)) { 
                        cmn_err(CE_CONT,"asyc_lcr = 0x%x", ap->asyc_lcr ); 
                        ioaddr_err++; 
                } 
                if ( ap->asyc_mcr != (iob + 4)) { 
                        cmn_err(CE_CONT,"asyc_mcr = 0x%x", ap->asyc_mcr ); 
                        ioaddr_err++; 
                } 
                if ( ap->asyc_lsr != (iob + 5)) { 
                        cmn_err(CE_CONT,"asyc_lsr = 0x%x", ap->asyc_icr ); 
                        ioaddr_err++; 
                } 
                if ( ap->asyc_msr != (iob + 6)) { 
                        cmn_err(CE_CONT,"asyc_msr = 0x%x", ap->asyc_icr ); 
                        ioaddr_err++; 
                } 
                        
                if (!ioaddr_err)                
                        cmn_err(CE_CONT,"DAT/ICR/ISR/LCR/MCR/LSR/MSR addresses OK\n"); 
                
                cmn_err(CE_CONT,"flags: 0x%x : ", ap->flags);

                if (irq && irq != ap->asyc_vect) 
                                cmn_err(CE_WARN,"\nUnexpected IRQ (asyc_vect) %d\n",
                                                                                                                ap->asyc_vect); 
                else 
                                cmn_err(CE_CONT,"\nIRQ line = %d ", ap->asyc_vect); 

                cmn_err(CE_CONT," ( cm_intr_attach IRQ cookie = 0% )x\n",
                                                                                                        ap->asyc_irqmagic); 


                cmn_err(CE_CONT,"\t&asycbuf_t : 0x%x", ap->asyc_bp); 
                cmn_err(CE_CONT,"\t&asycstats_t : 0x%x\n", ap->asyc_bp); 

                cmn_err(CE_CONT,"\tasyc_utyp = 0x%x\n", ap->asyc_utyp ); 

                cmn_err(CE_CONT,"\tRx FIFO tlev = %d, Tx FIFO tlev = %d\n", 
                                                        ap->rxfifo, ap->txfifo); 
        } 

                return; 
} 

/* 
 * void 
 * asyc_uart(uint_t unit)
 * 
 * Not called. 
 * 
 * Remarks
 *      Displays UART configuration by reading the IO register states.
 */

void
asyc_uart (uint_t unit)
{ 
        ioaddr_t        iob; 
        uchar_t         dlab_lsb, dlab_msb, lcr, lsr, mcr, msr, ier; 
        int                     baud, dbits, sbits, parity ;

        iob = asyctab[unit].asyc_dat ; 
        lcr = inb(ILCR(iob)); 

        outb(ILCR(iob), lcr | 0x80 ); 
        dlab_lsb = inb(IDAT(iob));
        dlab_msb = inb(IICR(iob));
        outb(ILCR(iob), lcr ); 

        msr = inb(IMSR(iob)); 
        lsr = inb(ILSR(iob)); 
        mcr = inb(IMCR(iob)); 

        cmn_err(CE_CONT,"\tUART %d IO addr 0x%x\n",unit, iob); 

        baud = 115200 / ((dlab_msb << 8) + dlab_lsb); 
        dbits = ((lcr & 3) + 5); 
        sbits = ((lcr & 4) ? 2 : 1); 
        parity = ((lcr & 8) ? ((lcr & 16) ? 2 : 1) : 0) ; 

        cmn_err(CE_CONT,"\tBaud = %d, %d data bits, %d stop bits, %s parity", 
                                        baud, dbits, sbits, 
                                        (parity == 0) ? "No" : ((parity == 1) ? "Odd" : "Even" )); 

#define DFLG1(_f)       if (mcr & MCR_ ## _f ) cmn_err(CE_CONT," " #_f ); 

        cmn_err(CE_CONT, "\n\tMCR set flags: "); 
        DFLG1(DTR)
        DFLG1(RTS)
        DFLG1(OUT1)
        DFLG1(OUT2)
        DFLG1(LBK)

#define DFLG2(_f)       if (lsr & LSR_ ## _f ) cmn_err(CE_CONT," " #_f ); 

        cmn_err(CE_CONT, "\n\tLSR set flags: "); 
        DFLG2(RCA); 
        DFLG2(OVRRUN); 
        DFLG2(FRMERR); 
        DFLG2(PARERR); 
        DFLG2(BRKDET); 
        DFLG2(XHRE); 
        DFLG2(XSRE); 

#define DFLG3(_f)       if (msr & MSR_ ## _f ) cmn_err(CE_CONT," " #_f ); 

        cmn_err(CE_CONT, "\n\tMSR set flags: "); 
        DFLG3(DCD); 
        DFLG3(DDCD); 
        DFLG3(CTS); 
        DFLG3(DCTS); 
        DFLG3(DSR); 
        DFLG3(DDSR); 
        DFLG3(RI); 
        DFLG3(DRI); 

#define DFLG4(_f)       if (lcr & LCR_ ## _f ) cmn_err(CE_CONT," " #_f ); 

        cmn_err(CE_CONT, "\n\tLCR set flags: "); 
        DFLG4(SETBREAK); 

        cmn_err(CE_CONT,"\n");

        return; 
}

/*
 * void 
 * asyc_scan ( unit ) 
 * 
 * Not called.
 *
 * Remarks: 
 *      Displays the uart_scantbl data structure for a given unit. 
 */

void
asyc_scan ( unit ) 
{ 
        uart_data_t             *up; 
        

        if ( unit >=  asyc_nscdevs ) { 
                cmn_err ( CE_WARN, "unit %d > uart_scantbl size (%d)", 
                                                        unit, asyc_nscdevs - 1); 
                return ; 
        } 

        up = &uart_scantbl[unit]; 
        
        cmn_err(CE_CONT,"uart_scantbl[], unit %d\t", unit); 

        cmn_err(CE_CONT,"iobase = 0x%x ", up->iobase);  
        cmn_err(CE_CONT,"irqnum = 0x%x\n", up->irqnum);  
        cmn_err(CE_CONT,"uart (uart type) = %d ", up->uart);  
        cmn_err(CE_CONT,"status = %d ", up->status );  
        cmn_err(CE_CONT,"rxfifo = %d ", up->rxfifo);  
        cmn_err(CE_CONT,"txfifo = %d \n", up->txfifo);  
        cmn_err(CE_CONT,"compat = 0x%x", up->compat);  
        cmn_err(CE_CONT,"termios * = 0x%x ", up->termp );  
        cmn_err(CE_CONT,"termiox * = 0x%x\n", up->termxp );  

        return; 
} 

/*
 * void 
 * asyc_fcstate(unit)
 *
 * Calling/Exit State:
 *              None.
 * 
 * Remarks:
 *      Displays the state of flow control, configuration and the 
 * current driver state held in the strtty/asyc/asyc_aux flags.
 */ 

void 
asyc_fcstate(uint_t unit)
{ 
        struct strtty   *tp;
        asycbuf_t               *ap;
        asyc_t                  *asyc;
        astats_t                *sp; 
        struct termiox  *xp; 
        tcflag_t                flg; 
        
        if (unit >= asyncfg)
                return;

        tp = IASY_UNIT_TO_TP(asyc_id, unit);
        asyc = &asyctab[unit];
        ap = asyctab[unit].asyc_bp;
        sp = asyctab[unit].asyc_stats; 
        xp = &asy_xtty[unit]; 

#define _FLGDSP(_f)     if (flg & (_f)) cmn_err(CE_CONT, " "  #_f ) ;  

        /* strtty  */   
        cmn_err(CE_CONT, "\niflag = 0x%x",tp->t_iflag); 
        if (flg = tp->t_iflag) { 
                cmn_err(CE_CONT,"\nInput: "); 
                _FLGDSP( IGNBRK) 
                _FLGDSP( BRKINT)
                _FLGDSP( IGNPAR)
                _FLGDSP( PARMRK)
                _FLGDSP( INPCK)
                _FLGDSP( ISTRIP)
                _FLGDSP( INLCR)
                _FLGDSP( IGNCR)
                _FLGDSP( ICRNL)
                _FLGDSP( IUCLC)
                _FLGDSP( IXON)
                _FLGDSP( IXOFF)
                _FLGDSP( IMAXBEL)
                cmn_err(CE_CONT,"\n"); 

        } 

        cmn_err(CE_CONT, "\ncflag = 0x%x",tp->t_cflag); 
        if (flg = tp->t_cflag) {
                cmn_err(CE_CONT,"\nControl: "); 
                _FLGDSP( CSTOPB) 
                _FLGDSP( CREAD)         
                _FLGDSP( PARENB)
                _FLGDSP( PARODD)
                _FLGDSP( HUPCL)
                _FLGDSP( CLOCAL)
                cmn_err(CE_CONT,"\n"); 
        } 

        cmn_err(CE_CONT, "\nlflag = 0x%x",tp->t_lflag); 
        if (flg = tp->t_lflag) { 
                cmn_err(CE_CONT,"\nLocal : "); 
                _FLGDSP( ISIG)
                _FLGDSP( ICANON)
                _FLGDSP( NOFLSH)
                _FLGDSP( TOSTOP)
                _FLGDSP( ECHO)
                _FLGDSP( ECHOE)
                _FLGDSP( ECHOK)
                cmn_err(CE_CONT,"\n"); 
        } 

        cmn_err(CE_CONT, "\noflag = 0x%x",tp->t_oflag); 

        cmn_err(CE_CONT, "\nt_state = 0x%x",tp->t_state); 
        if (flg = tp->t_state) { 
                cmn_err(CE_CONT,"\ntty state: "); 
                _FLGDSP( TIMEOUT) 
                _FLGDSP( WOPEN)
                _FLGDSP( ISOPEN)
                _FLGDSP( TBLOCK)
                _FLGDSP( CARR_ON)
                _FLGDSP( BUSY)
                _FLGDSP( WIOC)
                _FLGDSP( WGETTY) 
                _FLGDSP( TTSTOP)
                _FLGDSP( EXTPROC)
                _FLGDSP( TACT)
                _FLGDSP( CLESC)
                _FLGDSP( RTO)
                _FLGDSP( TTIOW)
                _FLGDSP( TTXON)
                _FLGDSP( TTXOFF)
                cmn_err(CE_CONT,"\n"); 
        } 

        cmn_err(CE_CONT, "\nt_dstat = 0x%x",tp->t_dstat); 
        if (flg = tp->t_dstat) { 
                cmn_err(CE_CONT,"\ntty dstat : "); 
                _FLGDSP( EXCL_OTYPE)
        }       

        /* asyc */
        cmn_err(CE_CONT, "\nasyc.flags = 0x%x",asyc->flags); 
        if ( flg = asyc->flags ) { 
                cmn_err(CE_CONT,"\nflags: "); 
                _FLGDSP( CONDEV)
                _FLGDSP( ISTOP)
                _FLGDSP( ISTOPPEND)
                _FLGDSP( IGOPEND)
                _FLGDSP( OBRKPEND)
                _FLGDSP( UARTPEND)
                _FLGDSP( EOTXPEND)
                _FLGDSP( OSTOP)
                _FLGDSP( OBLK_SWFC)
                _FLGDSP( OBLK_HWFC)
                _FLGDSP( OBLK_MSTP)
                cmn_err(CE_CONT,"\n"); 
        } 

        /* termiox */
        cmn_err(CE_CONT, "\nxtty.x_hflag = 0x%x ",xp->x_hflag); 

        if ( flg = xp->x_hflag ) { 
                cmn_err(CE_CONT,"\ntermiox : "); 
                _FLGDSP( CTSXON)
                _FLGDSP( CDXON)
                _FLGDSP( DTRXOFF)
                _FLGDSP( RTSXOFF)
                cmn_err(CE_CONT,"\n"); 
        } 
        
        cmn_err(CE_CONT,"\n"); 
} 

/* 
 * void 
 * asyc_stats
 * 
 * Not called.
 * 
 * Remarks:
 *      Displays the current statistics gathered for the UART device.
 */


void 
asyc_stats ( uint_t unit )
{ 
        astats_t        *statsptr; 

        statsptr = asyctab[unit].asyc_stats; 

#define _SDSP(X)        printf ( "   "  #X "= %d", statsptr->X ) ; 
#define _SDSN(X)        printf ( "   "  #X "= %d\n", statsptr->X ) ; 

        _SDSP(  rxdataisr)
        _SDSP(  rawrxchars)
        _SDSP(  spclrxchars)
        _SDSN(  txdatasvcreq)
        _SDSP(  isrmodint)
        _SDSP(  isrnointid)

        _SDSP(  rcvdvstart)
        _SDSN(  rcvdvstop)
        _SDSP(  rcvdixany)

        _SDSP(  ierrcnt)
        _SDSP(  softrxovrn1)
        _SDSN(  softrxovrn2)
        _SDSP(  rxoverrun)
        _SDSP(  rxperror)
        _SDSP(  rxfrmerror)
        _SDSN(  rxbrkdet)

        _SDSP(  rxrealdata)

        _SDSP(  droprxcread)
        _SDSP(  droprxopen)
        _SDSN(  droprxnobuf)
        _SDSP(  nobufchars)

        _SDSP(  txdatachars)
        _SDSP(  nointrpending)

        _SDSN(  vstop_xhre_spin)
        _SDSP(  vstart_xhre_spin)
        _SDSP(  restart_xhre_spin)
        _SDSP(  rsteotpend)
        _SDSN(  badcarrsts) 

        _SDSP(  notxttstate )
        _SDSP(  notxhwfc )
        _SDSP(  notxswfc )
        _SDSN(  notxnodata )
        _SDSP(  notxqlockinuse )

        _SDSP( upstream_charcnt ) 
        _SDSP( upstream_calls ) 
        _SDSN( txstrqempty )
        _SDSP( nomsrdeltas )
        _SDSP( losttxrdyirq     )
        _SDSP( polltxcall )

        return ; 
} 

#endif  /* ASYC_DEBUG_PROD */

#ifdef ASYC_DEBUG 

/* 
 * int 
 * asyc_cmnerr(const char *, char * filename, int line); 
 *
 * Calling/Exit State:
 *      Used in local ASSERT() implementation. 
 * 
 * Remarks:
 *      Used in the ASSERT macro. Can PANIC, or send WARNING to osmlog.
 */ 

int
asyc_cmnerr(const char *string, char * filename, int line)
{
#ifdef ASYC_ASSERT_PANICS
        cmn_err(CE_PANIC,string,filename,line);
#else
        cmn_err(CE_WARN,string,filename,line);
#endif 
        return (1);
}

#endif  
