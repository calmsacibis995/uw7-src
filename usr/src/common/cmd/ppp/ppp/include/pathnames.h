#ifndef _PATHNAMES_H
#define _PATHNAMES_H

#ident	"@(#)pathnames.h	1.4"

/* pathname for unix domain socket */
#define	PPP_PATH "/etc/ppp.d/PPP_ADDR"

/* Where to find PSM support objects */
#define PSM_SO_PATH "/usr/lib/ppp/psm/"

/* The ppp device */
#define DEV_PPP "/dev/ppp"

/* The pcid device */
#define DEV_PCID "/dev/pcid"

/* The runtime support object */
#define PPPRT "/usr/lib/ppp/libppprt.so"

/* Log file */
#define DEFAULT_LOG "/var/adm/log/ppp.log"

/* Psuedo tty device */
#define DEV_PTM "/dev/ptmx"

/* Our internal ppp configuration file */
#define PPPCFG "/etc/ppp.d/.pppcfg"

/* Command used by pppd to load config */
#define PPPTALK_INIT "/usr/bin/ppptalk -l >/dev/null 2>&1"

/* Names for some environment variables */
#define PPPD_DEBUG "PPPD_DEBUG"	/* Set to debug the PPP Daemon */
#define ANVL_TESTS "ANVL"	/* Set when running the Anvl tests */

#endif /*_PATHNAMES_H*/
