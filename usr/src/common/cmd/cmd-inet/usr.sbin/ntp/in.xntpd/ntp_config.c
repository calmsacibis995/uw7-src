#ident "@(#)ntp_config.c	1.3"

/*
 * ntp_config.c - read and apply configuration information
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#ifndef SYS_WINNT
#if !defined(VMS)
#include <sys/wait.h>
#endif /* VMS */
#include <sys/time.h>
#else
#include <io.h>
#include <string.h>
#endif /* SYS_WINNT */

#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_unixtime.h"
#include "ntp_refclock.h"
#include "ntp_filegen.h"
#include "ntp_stdlib.h"

/*
 * These routines are used to read the configuration file at
 * startup time.  An entry in the file must fit on a single line.
 * Entries are processed as multiple tokens separated by white space
 * Lines are considered terminated when a '#' is encountered.  Blank
 * lines are ignored.
 */

/*
 * Configuration file name
 */
#ifndef	CONFIG_FILE
#ifndef SYS_WINNT
#define	CONFIG_FILE	"/etc/ntp.conf"
#else
#define CONFIG_FILE "%SystemRoot%\\NTP.INI"
#endif /* SYS_WINNT */
#endif	/* CONFIG_FILE */

/*
 * We understand the following configuration entries and defaults.
 *
 * peer [ addr ] [ version 3 ] [ key 0 ] [ minpoll 6 ] [ maxpoll 10 ]
 * server [ addr ] [ version 3 ] [ key 0 ] [ minpoll 6 ] [ maxpoll 10 ]
 * precision -7
 * broadcast [ addr ] [ version 3 ] [ key 0 ] [ ttl 1 ]
 * broadcastclient
 * multicastclient [224.0.1.1]
 * broadcastdelay 0.0102
 * authdelay 0.00842
 * restrict [ addr ] [ mask 255.255.255.0 ] ignore|noserve|notrust|noquery
 * driftfile file_name
 * keys file_name
 * statsdir /var/NTP/
 * filegen peerstats [ file peerstats ] [ type day ] [ link ]
 * clientlimit [ n ]
 * clientperiod [ 3600 ]
 * trustedkey [ key ]
 * requestkey [ key] 
 * controlkey [ key ]
 * trap [ addr ]
 * fudge [ addr ] [ stratum ] [ refid ] ...
 * pidfile [ ]
 * setvar [ ]
 * logfile logfile
 * logconfig [+|-|=][{sync|sys|peer|clock}{{,all}{info|statistics|events|status}}]...
 * enable auth|bclient|pll|pps|monitor|stats
 * disable auth|bclient|pll|pps|monitor|stats
 * phone ...
 */

/*
 * Types of entries we understand.
 */
#define CONFIG_UNKNOWN		0

#define	CONFIG_PEER		1
#define	CONFIG_SERVER		2
#define	CONFIG_PRECISION	3		/* (not used) */
#define	CONFIG_DRIFTFILE	4
#define	CONFIG_BROADCAST	5
#define	CONFIG_BROADCASTCLIENT	6
#define	CONFIG_AUTHENTICATE	7
#define	CONFIG_KEYS		8
#define	CONFIG_MONITOR		9		/* (not used) */
#define	CONFIG_AUTHDELAY	10
#define	CONFIG_RESTRICT		11
#define	CONFIG_BDELAY		12
#define	CONFIG_TRUSTEDKEY	13
#define	CONFIG_REQUESTKEY	14
#define	CONFIG_CONTROLKEY	15
#define	CONFIG_TRAP		16
#define	CONFIG_FUDGE		17
#define	CONFIG_RESOLVER		18		/* (not used) */
#define CONFIG_STATSDIR		19
#define CONFIG_FILEGEN		20
#define CONFIG_STATISTICS	21
#define	CONFIG_PIDFILE		22
#define CONFIG_SETVAR		23
#define CONFIG_CLIENTLIMIT	24
#define CONFIG_CLIENTPERIOD	25
#define CONFIG_MULTICASTCLIENT	26
#define CONFIG_ENABLE		27
#define CONFIG_DISABLE		28
#define CONFIG_PHONE		29
#define CONFIG_LOGFILE		30	
#define CONFIG_LOGCONFIG	31

#define	CONF_MOD_VERSION	1
#define	CONF_MOD_KEY		2
#define	CONF_MOD_MINPOLL	3
#define CONF_MOD_MAXPOLL	4
#define CONF_MOD_PREFER		5	
#define CONF_MOD_TTL		6
#define CONF_MOD_MODE		7

#define	CONF_RES_MASK		1
#define	CONF_RES_IGNORE		2
#define	CONF_RES_NOSERVE	3
#define	CONF_RES_NOTRUST	4
#define	CONF_RES_NOQUERY	5
#define	CONF_RES_NOMODIFY	6
#define	CONF_RES_NOPEER		7
#define	CONF_RES_NOTRAP		8
#define	CONF_RES_LPTRAP		9
#define	CONF_RES_NTPPORT	10
#define CONF_RES_LIMITED        11

#define	CONF_TRAP_PORT		1
#define	CONF_TRAP_INTERFACE	2

#define	CONF_FDG_TIME1		1
#define	CONF_FDG_TIME2		2
#define CONF_FDG_STRATUM	3
#define CONF_FDG_REFID		4
#define	CONF_FDG_FLAG1		5
#define	CONF_FDG_FLAG2		6
#define	CONF_FDG_FLAG3		7
#define	CONF_FDG_FLAG4		8

#define CONF_FGEN_FILE		1
#define CONF_FGEN_TYPE		2
#define CONF_FGEN_FLAG_LINK	3
#define CONF_FGEN_FLAG_NOLINK   4
#define CONF_FGEN_FLAG_ENABLE	5
#define CONF_FGEN_FLAG_DISABLE  6

/*
 * Translation table - keywords to function index
 */
struct keyword {
	char *text;
	int keytype;
};

/*
 * Command keywords
 */
static	struct keyword keywords[] = {
	{ "peer",		CONFIG_PEER },
	{ "server",		CONFIG_SERVER },
	{ "driftfile",		CONFIG_DRIFTFILE },
	{ "broadcast",		CONFIG_BROADCAST },
	{ "broadcastclient",	CONFIG_BROADCASTCLIENT },
	{ "multicastclient",	CONFIG_MULTICASTCLIENT },
	{ "authenticate",	CONFIG_AUTHENTICATE },
	{ "keys",		CONFIG_KEYS },
	{ "authdelay",		CONFIG_AUTHDELAY },
	{ "restrict",		CONFIG_RESTRICT },
	{ "broadcastdelay",	CONFIG_BDELAY },
	{ "trustedkey",		CONFIG_TRUSTEDKEY },
	{ "requestkey",		CONFIG_REQUESTKEY },
	{ "controlkey",		CONFIG_CONTROLKEY },
	{ "trap",		CONFIG_TRAP },
	{ "fudge",		CONFIG_FUDGE },
	{ "statsdir",		CONFIG_STATSDIR },
	{ "filegen",		CONFIG_FILEGEN },  
	{ "statistics",		CONFIG_STATISTICS },
	{ "pidfile",		CONFIG_PIDFILE },
	{ "setvar",		CONFIG_SETVAR },
	{ "clientlimit",	CONFIG_CLIENTLIMIT },
	{ "clientperiod",	CONFIG_CLIENTPERIOD },
	{ "enable",		CONFIG_ENABLE },
	{ "disable", 		CONFIG_DISABLE },
	{ "phone",		CONFIG_PHONE },
	{ "logfile",		CONFIG_LOGFILE },
	{ "logconfig",		CONFIG_LOGCONFIG },
	{ "",			CONFIG_UNKNOWN }
};

/*
 * "peer", "server", "broadcast" modifier keywords
 */
static	struct keyword mod_keywords[] = {
	{ "version",	CONF_MOD_VERSION },
	{ "key",	CONF_MOD_KEY },
	{ "minpoll",	CONF_MOD_MINPOLL },
	{ "maxpoll",	CONF_MOD_MAXPOLL },
	{ "prefer",	CONF_MOD_PREFER },
	{ "mode",	CONF_MOD_MODE },	/* reference clocks */
	{ "ttl",	CONF_MOD_TTL },		/* NTP peers */
	{ "",		CONFIG_UNKNOWN }
};

/*
 * "restrict" modifier keywords
 */
static	struct keyword res_keywords[] = {
	{ "mask",	CONF_RES_MASK },
	{ "ignore",	CONF_RES_IGNORE },
	{ "noserve",	CONF_RES_NOSERVE },
	{ "notrust",	CONF_RES_NOTRUST },
	{ "noquery",	CONF_RES_NOQUERY },
	{ "nomodify",	CONF_RES_NOMODIFY },
	{ "nopeer",	CONF_RES_NOPEER },
	{ "notrap",	CONF_RES_NOTRAP },
	{ "lowpriotrap", CONF_RES_LPTRAP },
	{ "ntpport",	CONF_RES_NTPPORT },
	{ "limited",    CONF_RES_LIMITED },
	{ "",		CONFIG_UNKNOWN }
};

/*
 * "trap" modifier keywords
 */
static	struct keyword trap_keywords[] = {
	{ "port",	CONF_TRAP_PORT },
	{ "interface",	CONF_TRAP_INTERFACE },
	{ "",		CONFIG_UNKNOWN }
};


/*
 * "fudge" modifier keywords
 */
static	struct keyword fudge_keywords[] = {
	{ "time1",	CONF_FDG_TIME1 },
	{ "time2",	CONF_FDG_TIME2 },
	{ "stratum",	CONF_FDG_STRATUM },
	{ "refid",	CONF_FDG_REFID },
	{ "flag1",	CONF_FDG_FLAG1 },
	{ "flag2",	CONF_FDG_FLAG2 },
	{ "flag3",	CONF_FDG_FLAG3 },
	{ "flag4",	CONF_FDG_FLAG4 },
	{ "",		CONFIG_UNKNOWN }
};


/*
 * "filegen" modifier keywords
 */
static	struct keyword filegen_keywords[] = {
	{ "file",	CONF_FGEN_FILE },
	{ "type",	CONF_FGEN_TYPE },
	{ "link",       CONF_FGEN_FLAG_LINK },
	{ "nolink",     CONF_FGEN_FLAG_NOLINK },
	{ "enable",     CONF_FGEN_FLAG_ENABLE },
	{ "disable",    CONF_FGEN_FLAG_DISABLE },
	{ "",		CONFIG_UNKNOWN }
};

/*
 * "type" modifier keywords
 */
static	struct keyword fgen_types[] = {
	{ "none",	FILEGEN_NONE  },
	{ "pid",	FILEGEN_PID   },
	{ "day",        FILEGEN_DAY   },
	{ "week",       FILEGEN_WEEK  },
	{ "month",      FILEGEN_MONTH },
	{ "year",	FILEGEN_YEAR  },
	{ "age",        FILEGEN_AGE   },
	{ "",		CONFIG_UNKNOWN}
};

/*
 * "enable", "disable" modifier keywords
 */
static struct keyword flags_keywords[] = {
	{ "auth",	PROTO_AUTHENTICATE },
	{ "bclient",	PROTO_BROADCLIENT },
	{ "pll",	PROTO_PLL },
	{ "pps",	PROTO_PPS },
	{ "monitor",	PROTO_MONITOR },
	{ "stats",	PROTO_FILEGEN },
	{ "",		CONFIG_UNKNOWN }
};

/*
 * "logconfig" building blocks
 */
struct masks {
  char         *name;
  unsigned long mask;
};

static struct masks logcfg_class[] = {
  { "sys",	NLOG_OSYS },
  { "peer",	NLOG_OPEER },
  { "clock",	NLOG_OCLOCK },
  { "sync",	NLOG_OSYNC },
  { (char *)0,	0 }
};

static struct masks logcfg_item[] = {
  { "info",		NLOG_INFO },
  { "allinfo",		NLOG_SYSINFO|NLOG_PEERINFO|NLOG_CLOCKINFO|NLOG_SYNCINFO },
  { "events",		NLOG_EVENT },
  { "allevents",	NLOG_SYSEVENT|NLOG_PEEREVENT|NLOG_CLOCKEVENT|NLOG_SYNCEVENT },
  { "status",		NLOG_STATUS },
  { "allstatus",	NLOG_SYSSTATUS|NLOG_PEERSTATUS|NLOG_CLOCKSTATUS|NLOG_SYNCSTATUS },
  { "statistics",	NLOG_STATIST },
  { "allstatistics",	NLOG_SYSSTATIST|NLOG_PEERSTATIST|NLOG_CLOCKSTATIST|NLOG_SYNCSTATIST },
  { "allclock",		(NLOG_INFO|NLOG_STATIST|NLOG_EVENT|NLOG_STATUS)<<NLOG_OCLOCK },
  { "allpeer",		(NLOG_INFO|NLOG_STATIST|NLOG_EVENT|NLOG_STATUS)<<NLOG_OPEER },
  { "allsys",		(NLOG_INFO|NLOG_STATIST|NLOG_EVENT|NLOG_STATUS)<<NLOG_OSYS },
  { "allsync",		(NLOG_INFO|NLOG_STATIST|NLOG_EVENT|NLOG_STATUS)<<NLOG_OSYNC },
  { "all",		NLOG_SYSMASK|NLOG_PEERMASK|NLOG_CLOCKMASK|NLOG_SYNCMASK },
  { (char *)0,	0 }
};

/*
 * Limits on things
 */
#define	MAXTOKENS	20	/* 20 tokens on line */
#define	MAXLINE		1024	/* maximum length of line */
#define MAXPHONE	5	/* maximum number of phone strings */
#define	MAXFILENAME	128	/* maximum length of a file name (alloca()?) */


/*
 * Miscellaneous macros
 */
#define	STRSAME(s1, s2)		(*(s1) == *(s2) && strcmp((s1), (s2)) == 0)
#define	ISEOL(c)		((c) == '#' || (c) == '\n' || (c) == '\0')
#define	ISSPACE(c)		((c) == ' ' || (c) == '\t')
#define	STREQ(a, b)		(*(a) == *(b) && strcmp((a), (b)) == 0)

/*
 * File descriptor used by the resolver save routines, and temporary file
 * name.
 */
static FILE *res_fp;
#ifndef SYS_WINNT
static char res_file[20];	/* enough for /tmp/xntpXXXXXX\0 */
#define	RES_TEMPFILE	"/tmp/xntpXXXXXX"
#else
static char res_file[MAX_PATH];
#endif /* SYS_WINNT */

/*
 * Definitions of things either imported from or exported to outside
 */
#ifdef DEBUG
extern int debug;
#endif
extern	char	*FindConfig();
	char	*progname;
	char	sys_phone[MAXPHONE][MAXDIAL]; /* ACTS phone numbers */
static	char	*xntp_options = "abc:dD:e:f:gk:l:mp:r:s:t:v:V:x";

#ifdef SYS_WINNT
extern HANDLE ResolverThreadHandle;
#endif

/*
 * Function prototypes
 */
static  unsigned long get_pfxmatch P((char **, struct masks *));
static  unsigned long get_match P((char *, struct masks *));
static  unsigned long get_logmask P((char *));
static	int	gettokens	P((FILE *, char *, char **, int *));
static	int	matchkey	P((char *, struct keyword *));
static	int	getnetnum	P((char *, struct sockaddr_in *, int));
static	void	save_resolve	P((char *, int, int, int, int, int, int, u_long));
static	void	do_resolve_internal P((void));
static	void	abort_resolve	P((void));
#if !defined(VMS)
static	RETSIGTYPE catchchild	P((int));
#endif /* VMS */

/*
 * get_pfxmatch - find value for prefixmatch
 * and update char * accordingly
 */
static unsigned long
get_pfxmatch(s, m)
     char ** s;
     struct masks *m;
{
  while (m->name) {
    if (strncmp(*s, m->name, strlen(m->name)) == 0) {
      *s += strlen(m->name);
      return m->mask;
    } else {
      m++;
    }
  }
  return 0;
}

/*
 * get_match - find logmask value
 */
static unsigned long
get_match(s, m)
     char *s;
     struct masks *m;
{
  while (m->name) {
    if (strcmp(s, m->name) == 0) {
      return m->mask;
    } else {
      m++;
    }
  }
  return 0;
}

/*
 * get_logmask - build bitmask for ntp_syslogmask
 */
static unsigned long
get_logmask(s)
     char *s;
{
  char *t;
  unsigned long offset;
  unsigned long mask;

  t = s;
  offset = get_pfxmatch(&t, logcfg_class);
  mask   = get_match(t, logcfg_item);

  if (mask)
    return mask << offset;
  else
    syslog(LOG_ERR, "logconfig: illegal argument %s - ignored", s);

  return 0;
}

/*
 * getstartup - search through the options looking for a debugging flag
 */
void
getstartup(argc, argv)
	int argc;
	char *argv[];
{
	int errflg;
	int c;
	extern int ntp_optind;
	extern char *ntp_optarg;

#ifdef DEBUG
	debug = 0;		/* no debugging by default */
#endif

	/*
	 * This is a big hack.  We don't really want to read command line
	 * configuration until everything else is initialized, since
	 * the ability to configure the system may depend on storage
	 * and the like having been initialized.  Except that we also
	 * don't want to initialize anything until after detaching from
	 * the terminal, but we won't know to do that until we've
	 * parsed the command line.  Do that now, crudely, and do it
	 * again later.  Our ntp_getopt() is explicitly reusable, by the
	 * way.  Your own mileage may vary.
	 *
	 * This hack is even called twice (to allow complete logging to file)
	 */
	errflg = 0;
	progname = argv[0];

	/*
	 * Decode argument list
	 */
	while ((c = ntp_getopt(argc, argv, xntp_options)) != EOF)
		switch (c) {
#ifdef DEBUG
		case 'd':
			++debug;
		break;
		case 'D':
			debug = atol(ntp_optarg);
			printf("Debug1: %s -> %x = %d\n", ntp_optarg, debug, debug);
		break;
#else
		case 'd':
		case 'D':
		  syslog(LOG_ERR, "xntpd not compiled with -DDEBUG option - no DEBUG support");
		  fprintf(stderr, "xntpd not compiled with -DDEBUG option - no DEBUG support");
	          ++errflg;
                break;
#endif
		case '?':
			++errflg;
			break;

		case 'l':
			{
				extern int syslogit;
				extern FILE *syslog_file;
				FILE *new_file;

				new_file = fopen(ntp_optarg, "a");
				if (new_file != NULL) {
 					NLOG(NLOG_SYSINFO)
					  syslog(LOG_NOTICE, "logging to file %s", ntp_optarg);
					if (syslog_file != NULL &&
					    fileno(syslog_file) != fileno(new_file))
						(void)fclose(syslog_file);

				    	syslog_file = new_file;
			                syslogit = 0;
				}
				else
					syslog(LOG_ERR,
					       "Cannot open log file %s",
					       ntp_optarg);
			}
                        break;

		default:
			break;
		}
	
	if (errflg || ntp_optind != argc) {
		(void) fprintf(stderr, "usage: %s [ -abdgmx ] [ -c config_file ] [ -e e_delay ]\n", progname);
		(void) fprintf(stderr, "\t\t[ -f freq_file ] [ -k key_file ] [ -l log_file ]\n");
		(void) fprintf(stderr, "\t\t[ -p pid_file ] [ -r broad_delay ] [ -s statdir ]\n");
		(void) fprintf(stderr, "\t\t[ -t trust_key ] [ -v sys_var ] [ -V default_sysvar ]\n");
		exit(2);
	}
	ntp_optind = 0;		/* reset ntp_optind to restart ntp_getopt */

#ifdef DEBUG
	if (debug) {
#ifdef NTP_POSIX_SOURCE
                static char buf[BUFSIZ];
                setvbuf(stdout, buf, _IOLBF, BUFSIZ);
#else
		setlinebuf(stdout);
#endif
        }
#endif
}

/*
 * getconfig - get command line options and read the configuration file
 */
void
getconfig(argc, argv)
	int argc;
	char *argv[];
{
	register int i;
	int c;
	int errflg;
	int peerversion;
	int minpoll;
	int maxpoll;
	int ttl;
	u_long peerkey;
	int peerflags;
	int hmode;
	struct sockaddr_in peeraddr;
	struct sockaddr_in maskaddr;
	FILE *fp;
	char line[MAXLINE];
	char *(tokens[MAXTOKENS]);
	int ntokens;
	int tok;
	struct interface *localaddr;
	char *config_file;
#ifdef SYS_WINNT
	LPTSTR temp;
	char config_file_storage[MAX_PATH];
#endif
	struct refclockstat clock;
	extern int ntp_optind;
	extern char *ntp_optarg;
	extern char *Version;
	extern u_long info_auth_keyid;
	extern int allow_set_backward;
	extern int correct_any;
	FILEGEN *filegen;

	/*
	 * Initialize, initialize
	 */
	errflg = 0;
#ifdef DEBUG
	debug = 0;
#endif	/* DEBUG */
#ifndef SYS_WINNT
	config_file = CONFIG_FILE;
#else
	temp = CONFIG_FILE;
	if (!ExpandEnvironmentStrings((LPCTSTR)temp, (LPTSTR)config_file_storage, (DWORD)sizeof(config_file_storage))) {
		syslog(LOG_ERR, "ExpandEnvironmentStrings failed: %m\n");
		exit(1);
	}
	config_file = config_file_storage;
#endif /* SYS_WINNT */
	progname = argv[0];
	res_fp = NULL;
	allow_set_backward = TRUE;
	correct_any = FALSE;
	memset((char *)sys_phone, 0, sizeof(sys_phone));
	ntp_syslogmask = NLOG_SYNCMASK; /* set more via logconfig */

	/*
	 * install a non default variable with this daemon version
	 */
	(void) sprintf(line, "daemon_version=\"%s\"", Version);
	set_sys_var(line, strlen(line)+1, RO);

	/*
	 * Decode argument list
	 */
	while ((c = ntp_getopt(argc, argv, xntp_options)) != EOF) {
		switch (c) {
		case 'a':
			proto_config(PROTO_AUTHENTICATE, 1);
			break;

		case 'b':
			proto_config(PROTO_BROADCLIENT, 1);
			break;

		case 'c':
			config_file = ntp_optarg;
			break;

		case 'd':
#ifdef DEBUG
			debug++;
#else
			errflg++;
#endif	/* DEBUG */
			break;
		case 'D':
#ifdef DEBUG
			debug = atol(ntp_optarg);
			printf("Debug2: %s -> %x = %d\n", ntp_optarg, debug, debug);
#else
			errflg++;
#endif	/* DEBUG */
		break;

		case 'e':
			do {
				l_fp tmp;
				
				if (!atolfp(ntp_optarg, &tmp)) {
					syslog(LOG_ERR,
			"command line encryption delay value %s undecodable",
					    ntp_optarg);
					errflg++;
				} else if (tmp.l_ui != 0) {
					syslog(LOG_ERR,
			"command line encryption delay value %s is unlikely",
					    ntp_optarg);
					errflg++;
				} else {
					proto_config(PROTO_AUTHDELAY, tmp.l_f);
				}
			} while (0);
			break;
			
		case 'f':
			stats_config(STATS_FREQ_FILE, ntp_optarg);
			break;

		case 'g':
			correct_any = TRUE;
			break;

		case 'k':
			getauthkeys(ntp_optarg);
			break;

		case 'l':	/* already done at pre-scan */
			break;

		case 'm':
			proto_config(PROTO_MULTICAST_ADD, htonl(INADDR_NTP));
			break;

		case 'p':
			stats_config(STATS_PID_FILE, ntp_optarg);
			break;

		case 'r':
			do {
				l_fp tmp;
				
				if (!atolfp(ntp_optarg, &tmp)) {
					syslog(LOG_ERR,
			"command line broadcast delay value %s undecodable",
					    ntp_optarg);
				} else if (tmp.l_ui != 0) {
					syslog(LOG_ERR,
			 "command line broadcast delay value %s is unlikely",
					    ntp_optarg);
				} else {
					proto_config(PROTO_BROADDELAY, tmp.l_f);
				}
			} while (0);
			break;
			
		case 's':
			stats_config(STATS_STATSDIR, ntp_optarg);
			break;
			
		case 't':
			do {
				u_long tkey;
				
				tkey = atol(ntp_optarg);
				if (tkey <= 0 || tkey > NTP_MAXKEY) {
					syslog(LOG_ERR,
				"command line trusted key %s is unlikely",
					    ntp_optarg);
				} else {
					authtrust(tkey, 1);
				}
			} while (0);
			break;
			
		case 'v':
		case 'V':
			set_sys_var(ntp_optarg, strlen(ntp_optarg)+1,
			    RW | ((c == 'V') ? DEF : 0));
			break;

		case 'x':
			allow_set_backward = FALSE;
			break;
			
		default:
			errflg++;
			break;
		}
	}
	
	if (errflg || ntp_optind != argc) {
		(void) fprintf(stderr, "usage: %s [ -abdgmx ] [ -c config_file ] [ -e e_delay ]\n", progname);
		(void) fprintf(stderr, "\t\t[ -f freq_file ] [ -k key_file ] [ -l log_file ]\n");
		(void) fprintf(stderr, "\t\t[ -p pid_file ] [ -r broad_delay ] [ -s statdir ]\n");
		(void) fprintf(stderr, "\t\t[ -t trust_key ] [ -v sys_var ] [ -V default_sysvar ]\n");
		exit(2);
	}

	if ((fp = fopen(FindConfig(config_file), "r")) == NULL) {
		/*
		 * Broadcast clients can sometimes run without
		 * a configuration file.
		 */
		return;
	}

	while ((tok = gettokens(fp, line, tokens, &ntokens))
	       != CONFIG_UNKNOWN) {
		switch(tok) {
		case CONFIG_PEER:
		case CONFIG_SERVER:
		case CONFIG_BROADCAST:
			if (tok == CONFIG_PEER)
				hmode = MODE_ACTIVE;
			else if (tok == CONFIG_SERVER)
				hmode = MODE_CLIENT;
			else
				hmode = MODE_BROADCAST;
			
			if (ntokens < 2) {
				syslog(LOG_ERR,
				   "No address for %s, line ignored",
				    tokens[0]);
				break;
			}
			
			if (!getnetnum(tokens[1], &peeraddr, 0)) {
				errflg = -1;
			} else {
				errflg = 0;
				
				if (
#ifdef REFCLOCK
				    !ISREFCLOCKADR(&peeraddr) &&
#endif
				    ISBADADR(&peeraddr)) {
					syslog(LOG_ERR,
					    "attempt to configure invalid address %s",
					    ntoa(&peeraddr));
					break;
				}
			}
			
			peerversion = NTP_VERSION;
			minpoll = NTP_MINDPOLL;
			maxpoll = NTP_MAXDPOLL;
			peerkey = 0;
			peerflags = 0;
			ttl = 0;
			for (i = 2; i < ntokens; i++)
				switch (matchkey(tokens[i], mod_keywords)) {
				case CONF_MOD_VERSION:
					if (i >= ntokens-1) {
						syslog(LOG_ERR,
						    "peer/server version requires an argument");
						errflg = 1;
						break;
					}
					peerversion = atoi(tokens[++i]);
					if ((u_char)peerversion > NTP_VERSION
					    || (u_char)peerversion < NTP_OLDVERSION) {
						syslog(LOG_ERR,
						    "inappropriate version number %s, line ignored",
						    tokens[i]);
						errflg = 1;
					}
					break;
					
				case CONF_MOD_KEY:
					if (i >= ntokens-1) {
						syslog(LOG_ERR,
						    "key: argument required");
						errflg = 1;
						break;
					}
					peerkey = atol(tokens[++i]);
					peerflags |= FLAG_AUTHENABLE;
					break;

				case CONF_MOD_MINPOLL:
					if (i >= ntokens-1) {
						syslog(LOG_ERR,
						    "minpoll: argument required");
						errflg = 1;
						break;
					}
					minpoll = atoi(tokens[++i]);
					if (minpoll < NTP_MINPOLL)
						minpoll = NTP_MINPOLL;
					break;

				case CONF_MOD_MAXPOLL:
					if (i >= ntokens-1) {
						syslog(LOG_ERR,
						    "maxpoll: argument required"
);
						errflg = 1;
						break;
					}
					maxpoll = atoi(tokens[++i]);
					if (maxpoll > NTP_MAXPOLL)
						maxpoll = NTP_MAXPOLL;
					break;

				case CONF_MOD_PREFER:
					peerflags |= FLAG_PREFER;
					break;

				case CONF_MOD_TTL:
					if (i >= ntokens-1) {
						syslog(LOG_ERR,
						    "ttl: argument required");
						errflg = 1;
						break;
					}
					ttl = atoi(tokens[++i]);
					break;

				case CONF_MOD_MODE:
					if (i >= ntokens-1) {
						syslog(LOG_ERR,
						    "mode: argument required");
						errflg = 1;
						break;
					}
					ttl = atoi(tokens[++i]);
					 break;

				case CONFIG_UNKNOWN:
					errflg = 1;
					break;
				}
			if (minpoll > maxpoll) {
				syslog(LOG_ERR, "config error: minpoll > maxpoll");
				errflg = 1;
			}
			if (errflg == 0) {
				if (peer_config(&peeraddr,
				    (struct interface *)0, hmode, peerversion, 
				    minpoll, maxpoll, peerflags, ttl, peerkey)
				    == 0) {
					syslog(LOG_ERR,
					    "configuration of %s failed",
					    ntoa(&peeraddr));
				}
			} else if (errflg == -1) {
				save_resolve(tokens[1], hmode, peerversion,
				    minpoll, maxpoll, peerflags, ttl, peerkey);
			}
			break;
			
		case CONFIG_DRIFTFILE:
			if (ntokens >= 2)
				stats_config(STATS_FREQ_FILE, tokens[1]);
			else
				stats_config(STATS_FREQ_FILE, (char *)0);
			break;
	
		case CONFIG_PIDFILE:
			if (ntokens >= 2)
				stats_config(STATS_PID_FILE, tokens[1]);
			else
				stats_config(STATS_PID_FILE, (char *)0);
			break;
				
		case CONFIG_LOGFILE: {
			extern int syslogit;
			extern FILE *syslog_file;

			if (ntokens >= 2) {
				FILE *new_file;
				new_file = fopen(tokens[1], "a");
				if (new_file != NULL) {
 					NLOG(NLOG_SYSINFO) /* conditional if clause for conditional syslog */
					  syslog(LOG_NOTICE, "logging to file %s", tokens[1]);
					if (syslog_file != NULL &&
					    fileno(syslog_file) != fileno(new_file))
						(void)fclose(syslog_file);

				    	syslog_file = new_file;
			                syslogit = 0;
				}
				else
					syslog(LOG_ERR,
					       "Cannot open log file %s",
					       tokens[1]);
			}
			else
				syslog(LOG_ERR, "logfile needs one argument");

			}
			break;

		case CONFIG_LOGCONFIG:
			{
			  for (i = 1; i < ntokens; i++)
			    {
			      int add = 1;
			      int equals = 0;
			      char * s = &tokens[i][0];

			      switch (*s)
				{
				case '+':
				case '-':
				case '=':
				  add = *s == '+';
				  equals = *s == '=';
				  s++;
				  break;

				default:
				  break;
				}
			      if (equals) {
				ntp_syslogmask = get_logmask(s);
			      } else {				
				if (add) {
				  ntp_syslogmask |= get_logmask(s);
				} else {
				  ntp_syslogmask &= ~get_logmask(s);
				}
			      }
#ifdef DEBUG
			      if (debug)
				printf("ntp_syslogmask = 0x%08lx (%s)\n", ntp_syslogmask, tokens[i]);
#endif
			    }
			}
                        break;

		case CONFIG_BROADCASTCLIENT:
			proto_config(PROTO_BROADCLIENT, 1);
			break;
			
		case CONFIG_MULTICASTCLIENT:
			if (ntokens > 1) {
				for (i = 1; i < ntokens; i++) {
					if (getnetnum(tokens[i], &peeraddr, 1))
						proto_config(PROTO_MULTICAST_ADD,
						    peeraddr.sin_addr.s_addr);
				}
			} else
				proto_config(PROTO_MULTICAST_ADD, htonl(INADDR_NTP));
			break;

		case CONFIG_AUTHENTICATE:
			errflg = 0;
			if (ntokens >= 2) {
				if (STREQ(tokens[1], "yes"))
					proto_config(PROTO_AUTHENTICATE, 1);
				else if (STREQ(tokens[1], "no"))
					proto_config(PROTO_AUTHENTICATE, 0);
				else
					errflg++;
			} else {
				errflg++;
			}
			
			if (errflg)
				syslog(LOG_ERR,
				    "should be `authenticate yes|no'");
			break;
			
		case CONFIG_KEYS:
			if (ntokens >= 2) {
				getauthkeys(tokens[1]);
			}
			break;
			
		case CONFIG_AUTHDELAY:
			if (ntokens >= 2) {
				l_fp tmp;
				
				if (!atolfp(tokens[1], &tmp)) {
					syslog(LOG_ERR,
					    "authdelay value %s undecodable",
					    tokens[1]);
				} else if (tmp.l_ui != 0) {
					syslog(LOG_ERR,
					    "authdelay value %s is unlikely",
					    tokens[1]);
				} else {
					proto_config(PROTO_AUTHDELAY, tmp.l_f);
				}
			}
			break;

		case CONFIG_RESTRICT:
			if (ntokens < 2) {
				syslog(LOG_ERR, "restrict requires an address");
				break;
			}
			if (STREQ(tokens[1], "default"))
				peeraddr.sin_addr.s_addr = htonl(INADDR_ANY);
			else if (!getnetnum(tokens[1], &peeraddr, 1))
				break;
			
			/*
			 * Use peerversion as flags, peerkey as mflags.  Ick.
			 */
			peerversion = 0;
			peerkey = 0;
			errflg = 0;
			maskaddr.sin_addr.s_addr = ~0;
			for (i = 2; i < ntokens; i++) {
				switch (matchkey(tokens[i], res_keywords)) {
				case CONF_RES_MASK:
					if (i >= ntokens-1) {
						syslog(LOG_ERR,
						    "mask keyword needs argument");
						errflg++;
						break;
					}
					i++;
					if (!getnetnum(tokens[i], &maskaddr, 1))
						errflg++;
					break;
					
				case CONF_RES_IGNORE:
					peerversion |= RES_IGNORE;
					break;
				
				case CONF_RES_NOSERVE:
					peerversion |= RES_DONTSERVE;
					break;
				
				case CONF_RES_NOTRUST:
					peerversion |= RES_DONTTRUST;
					break;
				
				case CONF_RES_NOQUERY:
					peerversion |= RES_NOQUERY;
					break;
					
				case CONF_RES_NOMODIFY:
					peerversion |= RES_NOMODIFY;
					break;
					
				case CONF_RES_NOPEER:
					peerversion |= RES_NOPEER;
					break;
					
				case CONF_RES_NOTRAP:
					peerversion |= RES_NOTRAP;
					break;
					
				case CONF_RES_LPTRAP:
					peerversion |= RES_LPTRAP;
					break;
					
				case CONF_RES_NTPPORT:
					peerkey |= RESM_NTPONLY;
					break;
					
				case CONF_RES_LIMITED:
					peerversion |= RES_LIMITED;
					break;
					
				case CONFIG_UNKNOWN:
					errflg++;
					break;
				}
			}
			if (SRCADR(&peeraddr) == htonl(INADDR_ANY))
				maskaddr.sin_addr.s_addr = 0;
			if (!errflg)
				restrict(RESTRICT_FLAGS, &peeraddr, &maskaddr,
				    (int)peerkey, peerversion);
			break;
			
		case CONFIG_BDELAY:
			if (ntokens >= 2) {
				l_fp tmp;
				
				if (!atolfp(tokens[1], &tmp)) {
					syslog(LOG_ERR,
					    "broadcastdelay value %s undecodable",
					    tokens[1]);
				} else if (tmp.l_ui != 0) {
					syslog(LOG_ERR,
					    "broadcastdelay value %s is unlikely",
					    tokens[1]);
				} else {
					proto_config(PROTO_BROADDELAY, tmp.l_f);
				}
			}
			break;
			
		case CONFIG_TRUSTEDKEY:
			for (i = 1; i < ntokens; i++) {
				u_long tkey;
				
				tkey = atol(tokens[i]);
				if (tkey == 0) {
					syslog(LOG_ERR,
					    "trusted key %s unlikely",
					    tokens[i]);
				} else {
					authtrust(tkey, 1);
				}
			}
			break;
			
		case CONFIG_REQUESTKEY:
			if (ntokens >= 2) {
				u_long rkey;
				
				if (!atouint(tokens[1], &rkey)) {
					syslog(LOG_ERR,
					    "%s is undecodable as request key",
					    tokens[1]);
				} else if (rkey == 0) {
					syslog(LOG_ERR,
					    "%s makes a poor request keyid",
					    tokens[1]);
				} else {
#ifdef DEBUG
					if (debug > 3)
						printf(
						    "set info_auth_key to %lu\n", rkey);
#endif
					info_auth_keyid = rkey;
				}
			}
			break;
			
		case CONFIG_CONTROLKEY:
			if (ntokens >= 2) {
				u_long ckey;
				extern u_long ctl_auth_keyid;
				
				ckey = atol(tokens[1]);
				if (ckey == 0) {
					syslog(LOG_ERR,
					    "%s makes a poor control keyid",
					    tokens[1]);
				} else {
					ctl_auth_keyid = ckey;
				}
			}
			break;
			
		case CONFIG_TRAP:
			if (ntokens < 2) {
				syslog(LOG_ERR,
				    "no address for trap command, line ignored");
				break;
			}
			if (!getnetnum(tokens[1], &peeraddr, 1))
				break;
			
			/*
			 * Use peerversion for port number.  Barf.
			 */
			errflg = 0;
			peerversion = 0;
			localaddr = 0;
			for (i = 2; i < ntokens-1; i++)
				switch (matchkey(tokens[i], trap_keywords)) {
				case CONF_TRAP_PORT:
					if (i >= ntokens-1) {
						syslog(LOG_ERR,
						    "trap port requires an argument");
						errflg = 1;
						break;
					}
					peerversion = atoi(tokens[++i]);
					if (peerversion <= 0
					    || peerversion > 32767) {
						syslog(LOG_ERR,
						    "invalid port number %s, trap ignored",
						       tokens[i]);
						errflg = 1;
					}
					break;
					
				case CONF_TRAP_INTERFACE:
					if (i >= ntokens-1) {
						syslog(LOG_ERR,
						    "trap interface requires an argument");
						errflg = 1;
						break;
					}
					
					if (!getnetnum(tokens[++i],
					    &maskaddr, 1)) {
						errflg = 1;
						break;
					}
					
					localaddr = findinterface(&maskaddr);
					if (localaddr == NULL) {
						syslog(LOG_ERR,
						    "can't find interface with address %s",
						    ntoa(&maskaddr));
						errflg = 1;
					}
					break;
					
				case CONFIG_UNKNOWN:
					errflg++;
					break;
				}
			
			if (!errflg) {
				extern struct interface *any_interface;
				
				if (peerversion != 0)
					peeraddr.sin_port = htons(peerversion);
				else
					peeraddr.sin_port = htons(TRAPPORT);
				if (localaddr == NULL)
					localaddr = any_interface;
				if (!ctlsettrap(&peeraddr, localaddr, 0,
				    NTP_VERSION))
					syslog(LOG_ERR,
					    "can't set trap for %s, no resources",
					    ntoa(&peeraddr));
			}
			break;
		
		case CONFIG_FUDGE:
			if (ntokens < 2) {
				syslog(LOG_ERR,
				    "no address for fudge command, line ignored");
				break;
			}
			if (!getnetnum(tokens[1], &peeraddr, 1))
				break;
			
			if (!ISREFCLOCKADR(&peeraddr)) {
				syslog(LOG_ERR,
				    "%s is inappropriate address for the fudge command, line ignored",
				    ntoa(&peeraddr));
				break;
			}
			
			memset((char *)&clock, 0, sizeof clock);
			errflg = 0;
			for (i = 2; i < ntokens-1; i++) {
				switch (c = matchkey(tokens[i],
				    fudge_keywords)) {
				case CONF_FDG_TIME1:
					if (!atolfp(tokens[++i],
					    &clock.fudgetime1)) {
						syslog(LOG_ERR,
						    "fudge %s time1 value in error",
						    ntoa(&peeraddr));
						errflg = i;
						break;
					}
					clock.haveflags |= CLK_HAVETIME1;
					break;
					
				case CONF_FDG_TIME2:
					if (!atolfp(tokens[++i],
					    &clock.fudgetime2)) {
						syslog(LOG_ERR,
						    "fudge %s time2 value in error",
						    ntoa(&peeraddr));
						errflg = i;
						break;
					}
					clock.haveflags |= CLK_HAVETIME2;
					break;
					
				case CONF_FDG_STRATUM:
					if (!atoint(tokens[++i],
					    (long *)&clock.fudgeval1)) {
						syslog(LOG_ERR,
						    "fudge %s stratum value in error",
						    ntoa(&peeraddr));
						errflg = i;
						break;
					}
					clock.haveflags |= CLK_HAVEVAL1;
					break;
					
				case CONF_FDG_REFID:
					strncpy((char *)&clock.fudgeval2,
					    tokens[++i], 4);
					clock.haveflags |= CLK_HAVEVAL2;
					break;

				case CONF_FDG_FLAG1:
				case CONF_FDG_FLAG2:
				case CONF_FDG_FLAG3:
				case CONF_FDG_FLAG4:
					if (!atouint(tokens[++i], &peerkey)
					    || peerkey > 1) {
						syslog(LOG_ERR,
						    "fudge %s flag value in error",
						    ntoa(&peeraddr));
						errflg = i;
						break;
					}
					switch(c) {
					case CONF_FDG_FLAG1:
						c = CLK_FLAG1;
						clock.haveflags|=CLK_HAVEFLAG1;
						break;
					case CONF_FDG_FLAG2:
						c = CLK_FLAG2;
						clock.haveflags|=CLK_HAVEFLAG2;
						break;
					case CONF_FDG_FLAG3:
						c = CLK_FLAG3;
						clock.haveflags|=CLK_HAVEFLAG3;
						break;
					case CONF_FDG_FLAG4:
						c = CLK_FLAG4;
						clock.haveflags|=CLK_HAVEFLAG4;
						break;
					}
					if (peerkey == 0)
						clock.flags &= ~c;
					else
						clock.flags |= c;
					break;
					
				case CONFIG_UNKNOWN:
					errflg = -1;
					break;
				}
			}
			
#ifdef REFCLOCK
			/*
			 * If reference clock support isn't defined the
			 * fudge line will still be accepted and syntax
			 * checked, but will essentially do nothing.
			 */
			if (!errflg) {
				refclock_control(&peeraddr, &clock,
				    (struct refclockstat *)0);
			}
#endif
			break;

		case CONFIG_STATSDIR:
			if (ntokens >= 2) {
				stats_config(STATS_STATSDIR,tokens[1]);
			}
			break;
			
		case CONFIG_STATISTICS:
			for (i = 1; i < ntokens; i++) {
				filegen = filegen_get(tokens[i]);

				if (filegen == NULL) {
					syslog(LOG_ERR,
					    "no statistics named %s available",
					    tokens[i]);
					continue;
				}
#ifdef DEBUG
				if (debug > 3)
					printf("enabling filegen for %s statistics \"%s%s\"\n",
					    tokens[i], filegen->prefix, filegen->basename);
#endif
				filegen->flag |= FGEN_FLAG_ENABLED;
			}
			break;
			
		case CONFIG_FILEGEN:
			if (ntokens < 2) {
				syslog(LOG_ERR,
				    "no id for filegen command, line ignored");
				break;
			}
			
			filegen = filegen_get(tokens[1]);
			if (filegen == NULL) {
				syslog(LOG_ERR,
				    "unknown filegen \"%s\" ignored",
				    tokens[1]);
				break;
			}
			/*
			 * peerversion is (ab)used for filegen file (index)
			 * peerkey     is (ab)used for filegen type
			 * peerflags   is (ab)used for filegen flags
			 */
			peerversion = 0;
			peerkey =     filegen->type;
			peerflags =   filegen->flag;
			errflg = 0;
			
			for (i = 2; i < ntokens; i++) {
				switch (matchkey(tokens[i], filegen_keywords)) {
				case CONF_FGEN_FILE:
					if (i >= ntokens - 1) {
						syslog(LOG_ERR, 
						    "filegen %s file requires argument",
						    tokens[1]);
						errflg = i;
						break;
					}
					peerversion = ++i;
					break;
				case CONF_FGEN_TYPE:
					if (i >= ntokens -1) {
						syslog(LOG_ERR,
						    "filegen %s type requires argument",
						    tokens[1]);
						errflg = i;
						break;
					}
					peerkey = matchkey(tokens[++i], fgen_types);
					if (peerkey == CONFIG_UNKNOWN) {
						syslog(LOG_ERR,
						    "filegen %s unknown type \"%s\"",
						    tokens[1], tokens[i]);
						errflg = i;
						break;
					}
					break;

				case CONF_FGEN_FLAG_LINK:
					peerflags |= FGEN_FLAG_LINK;
					break;

				case CONF_FGEN_FLAG_NOLINK:
					peerflags &= ~FGEN_FLAG_LINK;
					break;

				case CONF_FGEN_FLAG_ENABLE:
					peerflags |= FGEN_FLAG_ENABLED;
					break;

				case CONF_FGEN_FLAG_DISABLE:
					peerflags &= ~FGEN_FLAG_ENABLED;
					break;
				}
			}
			if (!errflg) {
				filegen_config(filegen, tokens[peerversion],
				    (u_char)peerkey, (u_char)peerflags);
			}
			break;

		case CONFIG_SETVAR:
			if (ntokens < 2) {
				syslog(LOG_ERR,
				    "no value for setvar command - line ignored");
			} else {
		 		set_sys_var(tokens[1], strlen(tokens[1])+1, RW |
				    ((((ntokens > 2) && !strcmp(tokens[2],
				    "default"))) ? DEF : 0));
			}
			break;
			
		case CONFIG_CLIENTLIMIT:
			if (ntokens < 2) {
				syslog(LOG_ERR,
				    "no value for clientlimit command - line ignored");
			} else {
				u_long i;
				if (!atouint(tokens[1], &i) || !i) {
					syslog(LOG_ERR,
				           "illegal value for clientlimit command - line ignored");
				} else {
					extern u_long client_limit;
					char bp[80];

#ifdef DEBUG
					if (debug)
						sprintf(bp, "client_limit=%lu", i);
#endif
					set_sys_var(bp, strlen(bp)+1, RO);
					client_limit = i;
				}
			}
			break;

		case CONFIG_CLIENTPERIOD:
			if (ntokens < 2) {
				syslog(LOG_ERR,
				    "no value for clientperiod command - line ignored");
			} else {
				u_long i;

				if (!atouint(tokens[1], &i) || i < 64) {
					syslog(LOG_ERR,
					    "illegal value for clientperiod command - line ignored");
				} else {
					extern u_long client_limit_period;
					char bp[80];

					sprintf(bp, "client_limit_period=%ld", i);
					set_sys_var(bp, strlen(bp)+1, RO);
					client_limit_period = i;
				}
		 	}
			break;

		case CONFIG_ENABLE:
			for (i = 1; i < ntokens; i++) {
				int flag;

				flag = matchkey(tokens[i], flags_keywords);
				if (flag == CONFIG_UNKNOWN) {
					syslog(LOG_ERR,
					    "enable unknown flag %s",
					    tokens[i]);
					errflg = 1;
					break;
				}
				proto_config(flag, 1L);
			}
			break;

		case CONFIG_DISABLE:
			for (i = 1; i < ntokens; i++) {
				int flag;

				flag = matchkey(tokens[i], flags_keywords);
				if (flag == CONFIG_UNKNOWN) {
					syslog(LOG_ERR,
					    "disable unknown flag %s", 
					    tokens[i]);
					errflg = 1;
					break;
				}
				proto_config(flag, 0L);
			}
			break;

		case CONFIG_PHONE:
			for (i = 1; i < ntokens && i < MAXPHONE; i++) {
				(void)strncpy(sys_phone[i - 1],
				    tokens[i], MAXDIAL);
			}
			sys_phone[i - 1][0] = '\0';
			break;
		}
	}
	(void) fclose(fp);

	if (res_fp != NULL) {
		/*
		 * Need name resolution
		 */
		do_resolve_internal();
	}
}



/*
 * gettokens - read a line and return tokens
 */
static int
gettokens(fp, line, tokenlist, ntokens)
	FILE *fp;
	char *line;
	char **tokenlist;
	int *ntokens;
{
	register char *cp;
	register int eol;
	register int ntok;
	register int quoted = 0;

	/*
	 * Find start of first token
	 */
again:
	while ((cp = fgets(line, MAXLINE, fp)) != NULL) {
		cp = line;
		while (ISSPACE(*cp))
			cp++;
		if (!ISEOL(*cp))
			break;
	}
	if (cp == NULL) {
		*ntokens = 0;
		return CONFIG_UNKNOWN;	/* hack.  Is recognized as EOF */
	}

	/*
	 * Now separate out the tokens
	 */
	eol = 0;
	ntok = 0;
	while (!eol) {
		tokenlist[ntok++] = cp;
		while (!ISEOL(*cp) && (!ISSPACE(*cp) || quoted))
			quoted ^= (*cp++ == '"');

		if (ISEOL(*cp)) {
			*cp = '\0';
			eol = 1;
		} else {		/* must be space */
			*cp++ = '\0';
			while (ISSPACE(*cp))
				cp++;
			if (ISEOL(*cp))
				eol = 1;
		}
		if (ntok == MAXTOKENS)
			eol = 1;
	}

	/*
	 * Return the match
	 */
	*ntokens = ntok;
	ntok = matchkey(tokenlist[0], keywords);
	if (ntok == CONFIG_UNKNOWN)
		goto again;
	return ntok;
}



/*
 * matchkey - match a keyword to a list
 */
static int
matchkey(word, keys)
	register char *word;
	register struct keyword *keys;
{
	for (;;) {
		if (keys->keytype == CONFIG_UNKNOWN) {
			syslog(LOG_ERR,
			    "configure: keyword \"%s\" unknown, line ignored",
			    word);
			return CONFIG_UNKNOWN;
		}
		if (STRSAME(word, keys->text))
			return keys->keytype;
		keys++;
	}
}


/*
 * getnetnum - return a net number (this is crude, but careful)
 */
static int
getnetnum(num, addr, complain)
	char *num;
	struct sockaddr_in *addr;
	int complain;
{
	register char *cp;
	register char *bp;
	register int i;
	register int temp;
	char buf[80];		/* will core dump on really stupid stuff */
	u_int32 netnum;

/* XXX ELIMINATE replace with decodenetnum */
	cp = num;
	netnum = 0;
	for (i = 0; i < 4; i++) {
		bp = buf;
		while (isdigit(*cp))
			*bp++ = *cp++;
		if (bp == buf)
			break;

		if (i < 3) {
			if (*cp++ != '.')
				break;
		} else if (*cp != '\0')
			break;

		*bp = '\0';
		temp = atoi(buf);
		if (temp > 255)
			break;
		netnum <<= 8;
		netnum += temp;
#ifdef DEBUG
	if (debug > 3)
		printf("getnetnum %s step %d buf %s temp %d netnum %lu\n",
		    num, i, buf, temp, (u_long)netnum);
#endif
	}

	if (i < 4) {
		if (complain)
			syslog(LOG_ERR,
		    "configure: \"%s\" not valid host number, line ignored",
			    num);
#ifdef DEBUG
		if (debug > 3)
		        printf(
		    "configure: \"%s\" not valid host number, line ignored\n",
		            num);
#endif
		return 0;
	}

	/*
	 * make up socket address.  Clear it out for neatness.
	 */
	memset((char *)addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(NTP_PORT);
	addr->sin_addr.s_addr = htonl(netnum);
#ifdef DEBUG
	if (debug > 1)
		printf("getnetnum given %s, got %s (%lx)\n",
		    num, ntoa(addr), (u_long)netnum);
#endif
	return 1;
}


#if !defined(VMS)
/*
 * catchchild - receive the resolver's exit status
 */
static RETSIGTYPE
catchchild(sig)
int sig;
{
	/*
	 * We only start up one child, and if we're here
	 * it should have already exited.  Hence the following
	 * shouldn't hang.  If it does, please tell me.
	 */
#ifndef SYS_WINNT
	(void) wait(0);
#endif /* SYS_WINNT */
}
#endif /* VMS */


/*
 * save_resolve - save configuration info into a file for later name resolution
 */
static void
save_resolve(name, mode, version, minpoll, maxpoll, flags, ttl, keyid)
	char *name;
	int mode;
	int version;
	int minpoll;
	int maxpoll;
	int flags;
	int ttl;
	u_long keyid;
{
	if (res_fp == NULL) {
#ifndef SYS_WINNT
		(void) strcpy(res_file, RES_TEMPFILE);
#else
		/* no /tmp directory under NT */
		{
			DWORD len;
			if(!(len = GetTempPath((DWORD)MAX_PATH, (LPTSTR)res_file))) {
				syslog(LOG_ERR, "cannot get pathname for temporary directory: %m");
				return;
			}
			(void) strcat(res_file, "xntpdXXXXXX");
		}
#endif /* SYS_WINNT */
		(void) mktemp(res_file);
		res_fp = fopen(res_file, "w");
		if (res_fp == NULL) {
			syslog(LOG_ERR, "open failed for %s: %m", res_file);
			return;
		}
	}

#ifdef DEBUG
	if (debug) {
	       printf("resolving %s\n", name);
	}
#endif

	(void) fprintf(res_fp, "%s %d %d %d %d %d %d %lu\n", name, mode,
	    version, minpoll, maxpoll, flags, ttl, keyid);
}


/*
 * abort_resolve - terminate the resolver stuff and delete the file
 */
static void
abort_resolve()
{
	/*
	 * In an ideal world we would might reread the file and
	 * log the hosts which aren't getting configured.  Since
	 * this is too much work, however, just close and delete
	 * the temp file.
	 */
	if (res_fp != NULL)
		(void) fclose(res_fp);
	res_fp = NULL;

#if !defined(VMS)
	(void) unlink(res_file);
#else
	(void) delete(res_file);
#endif /* VMS */
}


#define	KEY_TYPE_MD5	4

/*
 * do_resolve_internal - start up the resolver function (not program)
 */
/*
 * On VMS, this routine will simply refuse to resolve anything.
 *
 * Possible implementation: keep `res_file' in memory, do async
 * name resolution via QIO, update from within completion AST.
 * I'm unlikely to find the time for doing this, though. -wjm
 */
static void
do_resolve_internal()
{
	int i;

#if !defined(VMS)
	extern u_long req_keyid;	/* request keyid */
	extern char *req_file;		/* name of the file with config info */
#endif
	extern u_long info_auth_keyid;

	if (res_fp == NULL) {
		/* belch */
		syslog(LOG_ERR,
		    "internal error in do_resolve_internal: res_fp == NULL");
		exit(1);
	}

	/* we are done with this now */
	(void) fclose(res_fp);
	res_fp = NULL;

#if !defined(VMS)
	/* find a keyid */
	if (info_auth_keyid == 0)
		req_keyid = 65535;
	else
		req_keyid = info_auth_keyid;

	/* if doesn't exist, make up one at random */
	if (!authhavekey(req_keyid)) {
		char rankey[9];
		struct timeval now;

		/* generate random key */
		GETTIMEOFDAY(&now, (struct timezone *)0);
		srand(now.tv_sec * now.tv_usec);
		for (i = 0; i < 8; i++)
			rankey[i] = (rand() % 255) + 1;
		rankey[8] = 0;
		authusekey(req_keyid, KEY_TYPE_MD5, rankey);
	}

	/* save keyid so we will accept config requests with it */
	info_auth_keyid = req_keyid;
	req_file = res_file;	/* set up pointer to res file */
#ifndef SYS_WINNT
	(void) signal_no_reset(SIGCHLD, catchchild);

	i = fork();
	if (i == 0) {
		/*
		 * this used to close everything
		 * I don't think this is necessary
		 */
		/*
		 * To the unknown commenter above:
		 * Well, I think it's better to clean up
		 * after oneself. I have had problems with
		 * refclock-io when intres was running - things
		 * where fine again when xntpintres was gone.
		 * So some systems react erratic at least.
		 *
		 *			Frank Kardel
		 *
		 * 94-11-16:
		 * Further debugging has proven that the above is
		 * absolutely harmful. The internal resolver
		 * is still in the SIGIO process group and the lingering
		 * async io information causes it to process requests from
		 * all file decriptor causing a race between the NTP daemon
		 * and the resolver. which then eats data when it wins 8-(.
		 * It is absolutly necessary to kill ane io associations 
		 * shared with the NTP daemon. I currently don't want
		 *
		 * we also block SIGIO (currently no portes means to
		 * disable the signal handle for IO).
		 *
		 * Thanks to wgstuken@informatik.uni-erlangen.de to notice
		 * that it is the xntp-resolver child running into trouble.
		 *
		 * THUS:
		 */

		closelog();
	        kill_asyncio(); 

		(void) signal_no_reset(SIGCHLD, SIG_DFL);
		ntp_intres();

		/*
		 * If we got here, the intres code screwed up.
		 * Print something so we don't die without complaint
		 */
		syslog(LOG_ERR, "call to ntp_intres lost");
		abort_resolve();
		exit(1);
	}
	if (i == -1) {
		syslog(LOG_ERR, "fork() failed, can't start ntp_intres");
		(void) signal_no_reset(SIGCHLD, SIG_DFL);
		abort_resolve();
	}
#else /* SYS_WINNT */
	{
		/* NT's equivalent of fork() is _spawn(), but the start point
		 * of the new process is an executable filename rather than
		 * a function name as desired here.
		 */
	    DWORD dwThreadId;
		fflush(stdout);
    	if (!(ResolverThreadHandle = CreateThread(
        	NULL,                                /* no security attributes  */
        	0,                                   /* use default stack size  */
        	(LPTHREAD_START_ROUTINE) ntp_intres, /* thread function         */
        	NULL,                                /* argument to thread function   */
        	0,                                   /* use default creation flags    */
        	&dwThreadId))) {                     /* returns the thread identifier */
				syslog(LOG_ERR, "CreateThread() failed, can't start ntp_intres");
				abort_resolve();
		}
	}
#endif /* SYS_WINNT */
#else /* VMS */
	syslog(LOG_ERR, 
	    "Name resolution not implemented for VMS - use numeric addresses");
	abort_resolve();
#endif /* VMS */
}
