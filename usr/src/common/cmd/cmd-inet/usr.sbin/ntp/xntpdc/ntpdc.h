#ident "@(#)ntpdc.h	1.3"

/*
 * ntpdc.h - definitions of interest to xntpdc
 */
#include "ntp_fp.h"
#include "ntp.h"
#include "ntp_request.h"
#include "ntp_string.h"
#include "ntp_malloc.h"

/*
 * Maximum number of arguments
 */
#define	MAXARGS	4

/*
 * Flags for forming descriptors.
 */
#define	OPT	0x80		/* this argument is optional, or'd with type */

#define	NO	0x0
#define	NTP_STR	0x1		/* string argument */
#define	UINT	0x2		/* unsigned integer */
#define	INT	0x3		/* signed integer */
#define	ADD	0x4		/* IP network address */

/*
 * Arguments are returned in a union
 */
typedef union {
	char *string;
	long ival;
	u_long uval;
	u_int32 netnum;
} arg_v;

/*
 * Structure for passing parsed command line
 */
struct parse {
	char *keyword;
	arg_v argval[MAXARGS];
	int nargs;
};

/*
 * xntpdc includes a command parser which could charitably be called
 * crude.  The following structure is used to define the command
 * syntax.
 */
struct xcmd {
	char *keyword;		/* command key word */
	void (*handler)	P((struct parse *, FILE *));	/* command handler */
	u_char arg[MAXARGS];	/* descriptors for arguments */
	char *desc[MAXARGS];	/* descriptions for arguments */
	char *comment;
};

extern	int	doquery	P((int, int, int, int, int, char *, int *, int *, char **, int));
extern	char *	nntohost	P((u_int32));
