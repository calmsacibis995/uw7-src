#ident "@(#)refclock_gpsvme.c	1.2"

/*
 * refclock_gpsvme.c  NTP clock driver for the TrueTime GPS-VME
 * R. Schmidt, Time Service, US Naval Obs.  res@tuttle.usno.navy.mil
 * 
 * The refclock type has been defined as 16 (until new id assigned). 
 * These DEFS are included in the Makefile:
 *      DEFS= -DHAVE_TERMIOS -DSYS_HPUX=9
 *      DEFS_LOCAL=  -DREFCLOCK
 *      CLOCKDEFS=   -DGPSVME
 *  The file map_vme.c does the VME memory mapping, and includes vme_init().
 *  map_vme.c is HP-UX specific, because HPUX cannot mmap() device files! Boo!
 *  The file gps.h   provides TrueTime register info. 
 */
#if defined(REFCLOCK) && defined(GPSVME) 
#include <stdio.h>
#include <syslog.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>

#include "gps.h"
#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_refclock.h"
#include "ntp_unixtime.h"
#include "ntp_stdlib.h"
#include "/etc/conf/h/io.h"

/* GLOBAL STUFF BY RES */

#include <time.h>

#define PRIO    120               /* set the realtime priority */
#define NREGS 7                    /* number of registers we will use */

extern int init_vme();        /* This is just a call to map_vme() */
                              /* It doesn't have to be extern */
unsigned short  *greg[NREGS]; /* GPS registers defined in gps.h */
void *gps_base;               /* Base address of GPS VME card returned by */
                              /* the map_vme() call */
extern caddr_t map_vme ();   
extern void unmap_vme();      /* Unmaps the VME space */

 struct vmedate {               /* structure needed by ntp */
         unsigned short year; /* *tptr is a pointer to this */
         unsigned short doy;
         unsigned short hr;
         unsigned short mn;
         unsigned short sec;
         unsigned long frac;
         unsigned short status;
         };

struct vmedate *get_gpsvme_time();

/* END OF STUFF FROM RES */

/*
 * Definitions
 */
#define MAXUNITS 2              /* max number of VME units */
#define BMAX  50        /* timecode buffer length */

/*
 * VME interface parameters. 
 */
#define VMEPRECISION    (-21)      /* precision assumed (1 us) */
#define USNOREFID       "USNO\0"  /* Or whatever? */
#define VMEREFID        "GPS"   /* reference id */
#define VMEDESCRIPTION  "GPS" /* who we are */
#define VMEHSREFID      0x7f7f1001 /* 127.127.16.01 refid hi strata */

                   /* I'm using clock type 16 until one is assigned */
                   /* This is set also in vme_control, below        */


#define GMT             0       /* hour offset from Greenwich */

/*
 * Imported from ntp_timer module
 */
extern u_long current_time;     /* current time (s) */

/*
 * Imported from ntpd module
 */
extern int debug;               /* global debug flag */

/*
 * VME unit control structure.
 */
struct vmeunit {
        struct peer *peer;      /* associated peer structure */
        struct refclockio io;   /* given to the I/O handler */
        struct vmedate vmedata; /* data returned from vme read */
        l_fp lastrec;           /* last local time */
        l_fp lastref;           /* last timecode time */
        char lastcode[BMAX];    /* last timecode received */
        u_short lencode;        /* length of last timecode */
        u_long lasttime;        /* last time clock heard from */
        u_short unit;           /* unit number for this guy */
        u_short status;         /* clock status */
        u_short lastevent;      /* last clock event */
        u_short year;           /* year of eternity */
        u_short day;            /* day of year */
        u_short hour;           /* hour of day */
        u_short minute;         /* minute of hour */
        u_short second;         /* seconds of minute */
        u_long usec;            /* microsecond of second */
        u_long yearstart;       /* start of current year */
        u_short leap;           /* leap indicators */
        /*
         * Status tallies
         */
        u_long polls;           /* polls sent */
        u_long noreply;         /* no replies to polls */
        u_long coderecv;        /* timecodes received */
        u_long badformat;       /* bad format */
        u_long baddata;         /* bad data */
        u_long timestarted;     /* time we started this */
};

/*
 * Data space for the unit structures.  Note that we allocate these on
 * the fly, but never give them back.
 */
static struct vmeunit *vmeunits[MAXUNITS];
static u_char unitinuse[MAXUNITS];

/*
 * Keep the fudge factors separately so they can be set even
 * when no clock is configured.
 */
static l_fp fudgefactor[MAXUNITS];
static u_char stratumtouse[MAXUNITS];
static u_char sloppyclockflag[MAXUNITS];

/*
 * Function prototypes
 */
static  void    vme_init        P(());
static  int     vme_start       P((u_int, struct peer *));
static  void    vme_shutdown    P((int));
static  void    vme_report_event        P((struct vmeunit *, int));
static  void    vme_receive     P((struct recvbuf *));
static  void    vme_poll        P((int unit, struct peer *));
static  void    vme_control     P((u_int, struct refclockstat *, struct refclockstat *));
static  void    vme_buginfo     P((int, struct refclockbug *));

/*
 * Transfer vector
 */
struct  refclock refclock_gpsvme = {
        vme_start, vme_shutdown, vme_poll,
        vme_control, vme_init, vme_buginfo, NOFLAGS
};

int fd_vme;  /* file descriptor for ioctls */
int regvalue;

/*
 * vme_init - initialize internal vme driver data
 */
static void
vme_init()
{
        register int i;
        /*
         * Just zero the data arrays
         */
         /*
        bzero((char *)vmeunits, sizeof vmeunits);
        bzero((char *)unitinuse, sizeof unitinuse);
        */

        /*
         * Initialize fudge factors to default.
         */
        for (i = 0; i < MAXUNITS; i++) {
                fudgefactor[i].l_ui = 0;
                fudgefactor[i].l_uf = 0;
                stratumtouse[i] = 0;
                sloppyclockflag[i] = 0;
        }
}

/*
 * vme_start - open the VME device and initialize data for processing
 */
static int
vme_start(unit, peer)
        u_int unit;
        struct peer *peer;
{
        register struct vmeunit *vme;
        register int i;
        int dummy;
        char vmedev[20];

        /*
         * Check configuration info.
         */
        if (unit >= MAXUNITS) {
                syslog(LOG_ERR, "vme_start: unit %d invalid", unit);
                return (0);
        }
        if (unitinuse[unit]) {
                syslog(LOG_ERR, "vme_start: unit %d in use", unit);
                return (0);
        }

        /*
         * Open VME device
         */
#ifdef DEBUG

        printf("Opening  VME DEVICE \n");
#endif
        init_vme();   /* This is in the map_vme.c external file */

        /*
         * Allocate unit structure
         */
        if (vmeunits[unit] != 0) {
                vme = vmeunits[unit];   /* The one we want is okay */
        } else {
                for (i = 0; i < MAXUNITS; i++) {
                        if (!unitinuse[i] && vmeunits[i] != 0)
                                break;
                }
                if (i < MAXUNITS) {
                        /*
                         * Reclaim this one
                         */
                        vme = vmeunits[i];
                        vmeunits[i] = 0;
                } else {
                        vme = (struct vmeunit *)
                            emalloc(sizeof(struct vmeunit));
                }
        }
        bzero((char *)vme, sizeof(struct vmeunit));
        vmeunits[unit] = vme;

        /*
         * Set up the structures
         */
        vme->peer = peer;
        vme->unit = (u_short)unit;
        vme->timestarted = current_time;

        vme->io.clock_recv = vme_receive;
        vme->io.srcclock = (caddr_t)vme;
        vme->io.datalen = 0;
        vme->io.fd = fd_vme;

        /*
         * All done.  Initialize a few random peer variables, then
         * return success. Note that root delay and root dispersion are
         * always zero for this clock.
         */
        peer->precision = VMEPRECISION;
        peer->rootdelay = 0;
        peer->rootdispersion = 0;
        peer->stratum = stratumtouse[unit];
            memcpy( (char *)&peer->refid, USNOREFID,4);

            /* peer->refid = htonl(VMEHSREFID); */

        unitinuse[unit] = 1;
        return (1);
}


/*
 * vme_shutdown - shut down a VME clock
 */
static void
vme_shutdown(unit)
        int unit;
{
        register struct vmeunit *vme;

        if (unit >= MAXUNITS) {
                syslog(LOG_ERR, "vme_shutdown: unit %d invalid", unit);
                return;
        }
        if (!unitinuse[unit]) {
                syslog(LOG_ERR, "vme_shutdown: unit %d not in use", unit);
                return;
        }

        /*
         * Tell the I/O module to turn us off.  We're history.
                 */
         unmap_vme();
        vme = vmeunits[unit];
        io_closeclock(&vme->io);
        unitinuse[unit] = 0;
}

/*
 * vme_report_event - note the occurance of an event
 *
 * This routine presently just remembers the report and logs it, but
 * does nothing heroic for the trap handler.
 */
static void
vme_report_event(vme, code)
        struct vmeunit *vme;
        int code;
{
        struct peer *peer;
        
        peer = vme->peer;
        if (vme->status != (u_short)code) {
                vme->status = (u_short)code;
                if (code != CEVNT_NOMINAL)
                        vme->lastevent = (u_short)code;
                syslog(LOG_INFO,
                    "clock %s event %x", ntoa(&peer->srcadr), code);
        }
}


/*
 * vme_receive - receive data from the VME device.
 *
 * Note: This interface would be interrupt-driven. We don't use that
 * now, but include a dummy routine for possible future adventures.
 */
static void
vme_receive(rbufp)
        struct recvbuf *rbufp;
{
}

/*
 * vme_poll - called by the transmit procedure
 */
static void
vme_poll(unit, peer)
        int unit;
        struct peer *peer;
{
        struct vmedate *tptr; 
        struct vmeunit *vme;
        l_fp tstmp;
        time_t tloc;
        struct tm *tadr;

        
        vme = (struct vmeunit *)emalloc(sizeof(struct vmeunit *));
        tptr = (struct vmedate *)emalloc(sizeof(struct vmedate *));

 
        if (unit >= MAXUNITS) {
                syslog(LOG_ERR, "vme_poll: unit %d invalid", unit);
                return;
        }
        if (!unitinuse[unit]) {
                syslog(LOG_ERR, "vme_poll: unit %d not in use", unit);
                return;
        }
        vme = vmeunits[unit];        /* Here is the structure */
        vme->polls++;

        tptr = &vme->vmedata; 
        
        if ((tptr = get_gpsvme_time()) == NULL ) {
                vme_report_event(vme, CEVNT_BADREPLY);
                return;
        }

        gettstamp(&vme->lastrec);
        vme->lasttime = current_time;

        /*
         * Get VME time and convert to timestamp format. 
         * The year must come from the system clock.
         */
/*
        time(&tloc);
        tadr = gmtime(&tloc);
        tptr->year = (unsigned short)(tadr->tm_year + 1900);
*/

        sprintf(vme->lastcode, 
            "%3.3d %2.2d:%2.2d:%2.2d.%.6d %1d\0",
            tptr->doy, tptr->hr, tptr->mn,
            tptr->sec, tptr->frac, tptr->status);

        record_clock_stats(&(vme->peer->srcadr), vme->lastcode);
        vme->lencode = (u_short) strlen(vme->lastcode);

        vme->day =  tptr->doy;
        vme->hour =   tptr->hr;
        vme->minute =  tptr->mn;
        vme->second =  tptr->sec;
        vme->usec =   tptr->frac;

#ifdef DEBUG
        if (debug)
                printf("vme: %3d %02d:%02d:%02d.%06ld %1x\n",
                    vme->day, vme->hour, vme->minute, vme->second,
                    vme->usec, tptr->status);
#endif
        if (tptr->status ) {       /*  Status 0 is locked to ref., 1 is not */
                vme_report_event(vme, CEVNT_BADREPLY);
                return;
        }

        /*
         * Now, compute the reference time value. Use the heavy
         * machinery for the seconds and the millisecond field for the
         * fraction when present. If an error in conversion to internal
         * format is found, the program declares bad data and exits.
         * Note that this code does not yet know how to do the years and
         * relies on the clock-calendar chip for sanity.
         */
        if (!clocktime(vme->day, vme->hour, vme->minute,
            vme->second, GMT, vme->lastrec.l_ui,
            &vme->yearstart, &vme->lastref.l_ui)) {
                vme->baddata++;
                vme_report_event(vme, CEVNT_BADTIME);
                syslog(LOG_ERR, "refclock_gpsvme: bad data!!");
                return;
        }
        TVUTOTSF(vme->usec, vme->lastref.l_uf);
        tstmp = vme->lastref;

        L_SUB(&tstmp, &vme->lastrec);
        vme->coderecv++;

        L_ADD(&tstmp, &(fudgefactor[vme->unit]));

        refclock_receive(vme->peer, &tstmp, GMT, 0,
            &vme->lastrec, &vme->lastrec, vme->leap);
}

/*
 * vme_control - set fudge factors, return statistics
 */
static void
vme_control(unit, in, out)
        u_int unit;
        struct refclockstat *in;
        struct refclockstat *out;
{
        register struct vmeunit *vme;

        if (unit >= MAXUNITS) {
                syslog(LOG_ERR, "vme_control: unit %d invalid)", unit);
                return;
        }

        if (in != 0) {
                if (in->haveflags & CLK_HAVETIME1)
                        fudgefactor[unit] = in->fudgetime1;
                if (in->haveflags & CLK_HAVEVAL1) {
                        stratumtouse[unit] = (u_char)(in->fudgeval1 & 0xf);
                        if (unitinuse[unit]) {
                                struct peer *peer;

                                /*
                                 * Should actually reselect clock, but
                                 * will wait for the next timecode
                                 */
                                vme = vmeunits[unit];
                                peer = vme->peer;
                                peer->stratum = stratumtouse[unit];
                                if (stratumtouse[unit] <= 1)
                                memcpy( (char *)&peer->refid, USNOREFID,4);
                                else
                                        peer->refid = htonl(VMEHSREFID);
                        }
                }
                if (in->haveflags & CLK_HAVEFLAG1) {
                        sloppyclockflag[unit] = in->flags & CLK_FLAG1;
                }
        }

        if (out != 0) {
                out->type = 16;  /*set  by RES  SHOULD BE CHANGED */
                out->haveflags
                    = CLK_HAVETIME1|CLK_HAVEVAL1|CLK_HAVEVAL2|CLK_HAVEFLAG1;
                out->clockdesc = VMEDESCRIPTION;
                out->fudgetime1 = fudgefactor[unit];
                out->fudgetime2.l_ui = 0;
                out->fudgetime2.l_uf = 0;
                out->fudgeval1 = (LONG)stratumtouse[unit];
                out->fudgeval2 = 0;
                out->flags = sloppyclockflag[unit];
                if (unitinuse[unit]) {
                        vme = vmeunits[unit];
                        out->lencode = vme->lencode;
                        out->lastcode = vme->lastcode;
                        out->timereset = current_time - vme->timestarted;
                        out->polls = vme->polls;
                        out->noresponse = vme->noreply;
                        out->badformat = vme->badformat;
                        out->baddata = vme->baddata;
                        out->lastevent = vme->lastevent;
                        out->currentstatus = vme->status;
                } else {
                        out->lencode = 0;
                        out->lastcode = "";
                        out->polls = out->noresponse = 0;
                        out->badformat = out->baddata = 0;
                        out->timereset = 0;
                        out->currentstatus = out->lastevent = CEVNT_NOMINAL;
                }
        }
}

/*
 * vme_buginfo - return clock dependent debugging info
 */
static void
vme_buginfo(unit, bug)
        int unit;
        register struct refclockbug *bug;
{
        register struct vmeunit *vme;

        if (unit >= MAXUNITS) {
                syslog(LOG_ERR, "vme_buginfo: unit %d invalid)", unit);
                return;
        }

        if (!unitinuse[unit])
                return;
        vme = vmeunits[unit];

        bug->nvalues = 11;
        bug->ntimes = 5;
        if (vme->lasttime != 0)
                bug->values[0] = current_time - vme->lasttime;
        else
                bug->values[0] = 0;
        bug->values[2] = (u_long)vme->year;
        bug->values[3] = (u_long)vme->day;
        bug->values[4] = (u_long)vme->hour;
        bug->values[5] = (u_long)vme->minute;
        bug->values[6] = (u_long)vme->second;
        bug->values[7] = (u_long)vme->usec;
        bug->values[9] = vme->yearstart;
        bug->stimes = 0x1c;
        bug->times[0] = vme->lastref;
        bug->times[1] = vme->lastrec;
}
/* -------------------------------------------------------*/
/* get_gpsvme_time()                                      */
/*  R. Schmidt, USNO, 1995                                */
/*  It's ugly, but hey, it works and its free             */

#include "gps.h"  /* defines for TrueTime GPS-VME */

#define PBIAS  193 /* 193 microsecs to read the GPS  experimentally found */

struct vmedate *get_gpsvme_time()
{
        struct vmedate  *time_vme;
        unsigned short set, hr, min, sec, ums, hms, status;
        int ret;
        char ti[3];

        long tloc ;
        time_t  mktime(),time();
        struct tm *gmtime(), *gmt;
        char  *gpsmicro;
        gpsmicro = (char *) malloc(7);  

        time_vme = (struct vmedate *)malloc(sizeof(struct vmedate ));
        *greg = (unsigned short *)malloc(sizeof(short) * NREGS);


/*  reference the freeze command address general register 1 */
        set = *greg[0];
/*  read the registers : */
/* get year */
        time_vme->year  = (unsigned short)  *greg[6];  
/* Get doy */
        time_vme->doy =  (unsigned short) (*greg[5] & MASKDAY);  
/* Get hour */
        time_vme->hr =  (unsigned short) ((*greg[4] & MASKHI) >>8);
/* Get minutes */
        time_vme->mn = (unsigned short)  (*greg[4] & MASKLO);
/* Get seconds */
        time_vme->sec = (unsigned short)  (*greg[3] & MASKHI) >>8;
        /* get microseconds in 2 parts and put together */
        ums  =   *greg[2];
        hms  =   *greg[3] & MASKLO;

        time_vme->status = (unsigned short) *greg[5] >>13;

/*  reference the unfreeze command address general register 1 */
        set = *greg[1];

        sprintf(gpsmicro,"%2.2x%4.4x\0", hms, ums);
        time_vme->frac = (u_long) gpsmicro;

/*      unmap_vme(); */

        if (!status) { 
                return ((void *)NULL);
                }
        else
                return (time_vme);
}
#endif 
