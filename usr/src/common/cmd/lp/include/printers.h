/*		copyright	"%c%" 	*/


#ident	"@(#)printers.h	1.3"
#ident	"$Header$"

#if	!defined(_LP_PRINTERS_H)
#define	_LP_PRINTERS_H
#include <mac.h>

/*
 * Define the following to support administrator configurable
 * streams modules:
 */
#define	CAN_DO_MODULES	1	/* */

/**
 ** The disk copy of the printer files:
 **/

/*
 * There are 20 fields in the printer configuration file.
 */
#define PR_MAX		20
#define PR_BAN		0
#define PR_CPI		1
#define PR_CS		2
#define PR_ITYPES	3
#define PR_DEV		4
#define PR_DIAL		5
#define PR_RECOV	6
#define PR_INTFC	7
#define PR_LPI		8
#define PR_LEN		9
#define PR_LOGIN	10
#define PR_PTYPE	11
#define PR_REMOTE	12
#define PR_SPEED	13
#define PR_STTY		14
#define PR_WIDTH	15
#define PR_MODULES	16
#define	PR_RANGE	17
#define	PR_FF		18
#define	PR_USER		19

/**
 ** The internal flags seen by the Spooler/Scheduler and anyone who asks.
 **/

#define	PS_REJECTED	0x001
#define	PS_DISABLED	0x002
#define	PS_FAULTED	0x004
#define	PS_BUSY		0x008
#define	PS_LATER	0x010	/* Printer is scheduled for service */
#define	PS_REMOTE	0x020

/**
 ** The internal copy of a printer as seen by the rest of the world:
 **/

/*
 * A (char **) list is an array of string pointers (char *) with
 * a null pointer after the last item.
 */
typedef struct PRINTER {
	char   *name;		/* name of printer (redundant) */
	unsigned short banner;	/* banner page conditions */
	SCALED cpi;             /* default character pitch */
	char   **char_sets;     /* list of okay char-sets/print-wheels */
	char   **input_types;   /* list of types acceptable to printer */
	char   *device;         /* printer port full path name */
	level_t hilevel;	/* maximum range of the device */
	level_t lolevel;	/* minimum range of the device */
	char   *dial_info;      /* system name or phone # for dial-up */
	char   *fault_rec;      /* printer fault recovery procedure */
	char   *interface;      /* interface program full path name */
	SCALED lpi;             /* default line pitch */
	SCALED plen;            /* default page length */
	unsigned short login;	/* is/isn't a login terminal */
	char   *printer_type;   /* Terminfo look-up value (obsolete) */
	char   *remote;         /* remote machine!printer-name */
	char   *speed;          /* baud rate for connection */
	char   *stty;           /* space separated list of stty options */
	SCALED pwid;            /* default page width */
	char   *description;	/* comment about printer */
	FALERT fault_alert;	/* how to alert on printer fault */
	short  daisy;           /* 1/0 - printwheels/character-sets */
#if     defined(CAN_DO_MODULES)
	char   **modules;	/* streams modules to push */
#endif
	char   **printer_types; /* Terminfo look-up values */
	unsigned int nw_flags;	/* for NetWare printing */
	char *user;		/* run lpNet process as this user */

	/*
	 * Adding new members to this structure? Check out
	 * cmd/lpadmin/do_printer.c, where we initialize
	 * each new printer structure.
	 */
}			PRINTER;

/*
**  Default levels for hilevel and lolevel (respectively)
**  when defining a new remote printer.  They tell me that these
**  level values will never change (theoretically they cannot) and
**  looking them up each time we need them is a LOT of overhead.
**  So, here they are.
*/
#define	PR_SYS_PUBLIC		((level_t) 1)
#define	PR_SYS_PRIVATE		((level_t) 2)
#define	PR_SYS_RANGE_MAX	((level_t) 3)
#define	PR_SYS_RANGE_MIN	((level_t) 8)
#define	PR_DEFAULT_HILEVEL	PR_SYS_RANGE_MAX
#define	PR_DEFAULT_LOLEVEL	PR_SYS_RANGE_MIN

#define BAN_ALWAYS	0x01	/* user can't override banner */
#define BAN_OFF		0x02	/* don't print banner page */

#define LOG_IN		0x01	/* printer is login terminal */

#define FF_ALWAYS	0x01	/* user can't override form feed */
#define FF_OFF		0x02	/* no form feed character after job */

#define PCK_TYPE	0x0001	/* printer type isn't in Terminfo */
#define PCK_CHARSET	0x0002	/* printer type can't handle ".char_sets" */
#define PCK_CPI		0x0004	/* printer type can't handle ".cpi" */
#define PCK_LPI		0x0008	/* printer type can't handle ".lpi" */
#define PCK_WIDTH	0x0010	/* printer type can't handle ".pwid" */
#define PCK_LENGTH	0x0020	/* printer type can't handle ".plen" */
#define PCK_LOCALE	0x0040	/* print environment can't handle "locale" */

/*
 * The following PCK_... bits are only set by the Spooler,
 * when refusing a request.
 */
#define PCK_BANNER	0x1000	/* printer needs banner */

/*
 * Flags set by "putprinter()" for things that go wrong.
 */
#define BAD_REMOTE	0x0001	/* has attributes of remote and local */
#define BAD_INTERFACE	0x0002	/* no interface or can't read it */
#define BAD_DEVDIAL	0x0004	/* no device or dial information */
#define BAD_FAULT	0x0008	/* not recognized fault recovery */
#define BAD_ALERT	0x0010	/* has reserved word for alert command */
#define BAD_ITYPES	0x0020	/* multiple printer AND input types */
#define BAD_PTYPES	0x0040	/* multiple printer types, incl unknown */
#define BAD_DAISY	0x0080	/* printer types don't agree on "daisy" */

/*
 * A comma separated list of STREAMS modules to be pushed on an
 * opened port.
 */
#define	DEFMODULES	"ldterm"

/*
 * For print wheels:
 */

typedef struct PWHEEL {
	char   *name;		/* name of print wheel */
	FALERT alert;		/* how to alert when mount needed */
}			PWHEEL;

extern unsigned long	badprinter,
			ignprinter;

/**
 ** Various routines.
 **/

#if	defined(__STDC__)

PRINTER *	getprinter ( char * );

PWHEEL *	getpwheel ( char * );

char *		getdefault ( void );

int		putprinter ( char *, PRINTER *);
int		delprinter ( char * );
int		putdefault ( char * );
int		deldefault ( void );
int		putpwheel ( char * , PWHEEL * );
int		delpwheel ( char * );
int		okprinter ( char * , PRINTER * , int );
char *		getcpdefault ( void );
int		putcpdefault ( char * );

unsigned long	chkprinter ( char * , char * , char * , char * , char * , char * );

void		freeprinter ( PRINTER * );
void		freepwheel ( PWHEEL * );

#else

PRINTER *	getprinter();

PWHEEL *	getpwheel();

char *		getdefault();

int		putprinter(),
		delprinter(),
		putdefault(),
		deldefault(),
		putpwheel(),
		delpwheel(),
		okprinter();

unsigned long	chkprinter();
char *		getcpdefault();
int		putcpdefault();

void		freeprinter(),
		freepwheel();

#endif

/**
 ** Aliases (copies) of some important Terminfo caps.
 **/

#endif
