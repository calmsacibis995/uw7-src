/*		copyright	"%c%" 	*/

#ident	"@(#)parms.h	1.2"
#ident  "$Header$"

/* go through this carefully, configuring for your site */

/* Owner of setud files running on behalf of uucp.  Needed in case
 * root runs uucp and euid is not honored by kernel.
 * GID is needed for some chown() calls.
 * Also used if guinfo() cannot find the current users ID in the
 * password file.
 */
#define UUCPUID		(uid_t) 5	/* */
#define UUCPGID		(gid_t) 5	/* */

/* define GRPCHK if you want to restrict the ability to read */
/* Systems file stuff by way of the DEBUG flags based on a group id range */
/* ex: if (GRPCHK(getgid()) no_secrets(); */
#define GRPMIN	(gid_t) 2	/* */
#define GRPMAX	(gid_t) 10	/* */
#define GRPCHK(gid)	( gid >= GRPMIN && gid <= GRPMAX ? 1 : 0 )	/* */
/* #define GRPCHK(gid)	1	/* Systems info is not protected from DEBUG */

/* definitions for the types of networks and dialers that are available */
/* used to depend on STANDALONE, but now done at runtime via Sysfiles	*/
#define DATAKIT		/* define DATAKIT if datakit is available. */
/* #define TCP		/* TCP (bsd systems) */
/* #define SYTEK	/* for sytek network */

#define TLI		/* for Transport Layer Interface networks */
#define TLIS		/* for Transport Layer Interface networks */
#define DIAL801	/* 801/212-103 auto dialers */

/* define DUMB_DN if your dn driver (801 acu) cannot handle '=' */
/* #define DUMB_DN /*  */

/*
 * Define protocols that are to be linked into uucico:
 *
 * The following table shows which protocols and networks work well
 * together.  The g protocol works over noisy links.  The e protocol
 * assumes that the underlying network provides an error free communications
 * channel that transfers the data in sequence without duplication.  The
 * d protocols makes the same assumptions as the e protocol, but in addition
 * it does Datakit specific ioctl's.  The g protocol is always included in
 * uucico.  To include the other protocols, 1) insure that the symbol from
 * the Symbol column is defined in this file and 2) include the file from
 * the File comlumn in the definition of PROTOCOLS in uucp.mk.
 *
 * Prot.
 * Letter Symbol       File	Applicable Media
 *
 *   g	  none	       -	-
 *   e	  E_PROTOCOL   eio.c	TCP, TLI, and DATAKIT.
 *   d	  D_PROTOCOL   dio.c	DATAKIT
 *   x	  X_PROTOCOL   xio.c	-
 *
 * The next six lines conditionally define the protocol symbols for d
 * and e protocols based on the networks that were chosen above.  For the
 * x protocol you must explicitly define X_PROTOCOL.
 */

#ifdef DATAKIT		/* Should include D protocol for Datakit. */
#define D_PROTOCOL
#endif /* DATAKIT */

#if defined TCP || defined TLI || defined DATAKIT
#define E_PROTOCOL	/* Include e protocol. */
#endif	/* TCP || TLI || DATAKIT */

/* #define X_PROTOCOL /* define X_PROTOCOL to use the xio protocol */

#define MAXCALLTRIES	2	/* maximum call attempts per Systems file line */

/* define DEFAULT_BAUDRATE to be the baud rate you want to use when both */
/* Systems file and Devices file allow Any */
#define DEFAULT_BAUDRATE "9600"	/* */

/*define permission modes for the device */
#define M_DEVICEMODE (mode_t) 0600	/* MASTER device mode */
#define S_DEVICEMODE (mode_t) 0600	/* SLAVE device mode */
#define R_DEVICEMODE (mode_t) 0600	/* default mode to restore */

/* UUSTAT_TBL - this is the maximum number of machines that
 * status may be needed at any instant.
 * If you are not concerned with memory for a seldom used program,
 * make it very large.
 * This number is also used in uusched for its machine table -- it has
 * the same properties as the one in uustat.
 */

/* #define UUSTAT_TBL 1000		/* big machine with lots of traffic */
#define UUSTAT_TBL 200

/* initial wait time after failure before retry */
#define RETRYTIME 300		/* 5 minutes */
/* MAXRETRYTIME is for exponential backoff  limit.
 * NOTE - this should not be 24 hours so that
 * retry is not always at the same time each day
 */
#define MAXRETRYTIME 82800	/* 23 hours */
#define ASSERT_RETRYTIME 86400	/* retry time for ASSERT errors */

/*  This is the path that will be used for uuxqt command executions */
#define PATH	"PATH=/usr/bin " /* */

/*  This is the set of default commands that can be executed */
/*  if non is given for the system name in PERMISSIONS file */
/*  It is a colon separated list as in PERMISSIONS file */
#define DEFAULTCMDS	"rmail"	/* standard default command list */

/* define NOSTRANGERS if you want to reject calls from systems which
 * are not in your Systems file.   If defined, NOSTRANGERS should be the name
 * of the program to execute when such a system dials in.  The argument
 * to said program will be the name of said system.  Typically this is a shell
 * procedure that sends mail to the uucp administrator informing them of an
 * attempt to communicate by an unknown system.
 * NOTE - if this is defined, it can be overridden by the administrator
 * by making the command non-executable.  (It can be turned on and off
 * by changing the mode of the command.)
 */
#define NOSTRANGERS	"/usr/lib/uucp/remote.unknown"	/* */

/* define GETTY to be the pathname of a non-STREAMS equivalent
 * of 'ttymon' if your system supports non-STREAMS (i.e. clist)
 * port/network drivers. It is expected to have the same interface
 * as the pre-SVR4 'getty' program.
 */
/*#define GETTY	"/sbin/getty"				/* */

/* define LIMITS to be the name of a file which contains information
 * about the number of simultaneous uucicos,uuxqts, and uuscheds
 * that are allowed to run. If it is not defined, then there may be
 * "many" uucicos, uuxqts, and uuscheds running.
 */
#define LIMITS		"/etc/uucp/Limits"		/* */

/* define USRSPOOLLOCKS if you like your lock files in /var/spool/locks
 * be sure other programs such as 'cu' and 'ct' know about this
 *
 * WARNING: if you do not define USRSPOOLLOCKS, then $LOCK in 
 * uudemon.clean must be changed.
 */
#define USRSPOOLLOCKS  /* define to use /var/spool/locks for LCK files */

/* define BIGCMDLINE if the current size of the uux command line
 * is not large enough.
 */
#define BIGCMDLINE	/* */
