#ident	"@(#)kern-i386at:svc/name.cf/Space.c	1.9.6.1"
#ident	"$Header$"

/*
 * This file contains machine dependent information.
 */

#include <sys/types.h>
#include <sys/utsname.h>
#include <config.h>

/*
 * ibcs2 conformance restricts the value assigned to the constant SYSNAME
 * to 8 characters or less.
 */
#define SYSNAME	"UnixWare"	/* utsname.sysname */
#define NODE	"unix"		/* [x]utsname.nodename, hostname */
#define REL	"5"		/* [x]utsname.release */
#define VER	"7"		/* [x]utsname.version */
#define MACH	"486"		/* [x]utsname.machine */

#define XSYS	"UNIX_SV"	/* xutsname.sysname */
#define RESRVD	""		/* xutsname.reserved */
#define ORIGIN	1		/* xutsname.sysorigin */
#define OEMNUM	0		/* xutsname.sysoem */
#define SERIAL	0		/* xutsname.sysserial */

struct utsname	utsname = {
	SYSNAME,		/* sysname */
	NODE,			/* nodename - may change via setuname */
	REL,			/* release - may change at boot time */
	VER,			/* version - may change at boot time */
	MACH			/* machine - will change at boot time */
};

struct xutsname xutsname = {
	XSYS,			/* sysname */
	NODE,			/* nodename - may change via setuname */
	REL,			/* release - may change at boot time */
	VER,			/* version - may change at boot time */
	MACH,			/* machine - will change at boot time */
	RESRVD,			/* reserved */
	ORIGIN,			/* sysorigin */
	OEMNUM,			/* sysoem */
	SERIAL			/* sysserial */
};

/* sysinfo information */
char architecture[SYS_NMLN] =	"IA32";
char srpc_domain[SYS_NMLN] =	"";
char hostname[SYS_NMLN] =	NODE;		/* may change via setuname */
char initfile[SYS_NMLN] =	INITFILE;	/* tunable */
char osbase[] =			"UNIX_SVR5";
char osprovider[] =		"SCO";
char *bustypes =		"AT";		/* default value only */

/* UnixWare 2.x compatibility strings for sysinfo and scoinfo */
char o_architecture[] =		"x86at";
char o_hw_provider[] =		"SCO";
char o_machine[] =		"i386";
char o_sysname[] =		"UNIX_SV";
char *o_bustype =		"AT";		/* default value only */
