#ident	"@(#)init.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident "@(#)init.c	1.25 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * A bunch of global variable declarations lie herein.
 *
 * All global externs are declared in def.h. All variables are initialized
 * here!
 *
 * !!!!!IF YOU CHANGE (OR ADD) IT HERE, DO IT THERE ALSO !!!!!!!!
 *
 */

#include	"def.h"
#include	<grp.h>
#include	<pwd.h>
#include	<utmp.h>
#include	<sys/utsname.h>

struct hdr header[] = {
	{	"",				FALSE,  0 },
	{	"Content-Length:",		TRUE,  15 },
	{	"Content-Type:",		FALSE, 13 },
	{	"Date:",			TRUE,   5 },
	{	"From ",			TRUE,   5 },
	{	">From ",			TRUE,   6 },
	{	"From:",			TRUE,   5 },
	{	"Received:",			FALSE,  9 },
	{	"Status:",			FALSE,  7 },
	{	"Subject:",			TRUE,   8 },
	{	"To:",				TRUE,   3 },
	{	"Encoding-Type:",		TRUE,  14 },
	{	"Mime-Version:",		TRUE,  14 },
};

int	Fflag = 0;			/* -F option: record messages into a file */
int	Hflag = 0;			/* -H option: print headers and exit */
char	*Tflag;				/* -T option: temp file for netnews */
int	UnUUCP = 0;			/* -U flag */
char	**altnames;			/* List of alternate names for user */
int	baud;				/* Output baud rate */
int	cond;				/* Current state of conditional exc. */
int	debug;				/* Debug flag set */
char	*domain;			/* domain name, such as system.com */
struct	message	*dot;			/* Pointer to current message */
int	edit;				/* Indicates editing a file */
const char	*editfile;		/* Name of file being edited */
int	escapeokay = 0;			/* Permit ~s even when reading files */
int	exitflg = 0;			/* -e for mail test */
int	fflag = 0;			/* -f: use file */
NODE	*fplist = NOFP;			/* list of FILE*'s maintained by my_fopen/my_fclose */
struct	grouphead *groups[HSHSIZE];	/* Pointer to active groups */
int	hflag;				/* Sequence number for network -h */
char	homedir[PATHSIZE];		/* Name of home directory */
char	*host;				/* Name of system (cluster name or uname) */
struct	ignret	*ignore[HSHSIZE];	/* Pointer to ignored fields */
int	image;				/* File descriptor for image of msg */
FILE	*input;				/* Current command input file */
int	intty;				/* True if standard input a tty */
int	ismail = TRUE;			/* default to program=mail */
MAILSTREAM	*itf;			/* Input temp file buffer */
int	lexnumber;			/* Number of TNUMBER from scan() */
char	lexstring[STRINGLEN];		/* String from TSTRING, scan() */
int	loading;			/* Loading user definitions */
char	*lockfname;			/* named used for locking in /var/mail */
#ifdef	VAR_SPOOL_MAIL
char	maildir[] = "/var/spool/mail/";	/* directory for mail files */
#else
# ifdef	USR_SPOOL_MAIL
char	maildir[] = "/usr/spool/mail/";	/* directory for mail files */
# else
#  ifdef preSVr4
char	maildir[] = "/usr/mail/";	/* directory for mail files */
#  else
char	maildir[] = "/var/mail/";	/* directory for mail files */
#  endif
# endif
#endif
char	mailname[PATHSIZE];		/* Name of /var/mail system mailbox */
char	*remotehost;			/* Name of remote folder system */
int	maxfiles;			/* Maximum number of open files */
struct	message	*message;		/* The actual message structure */
char	mm_cmd_default[] = "metamail -m mailx";	/* default metamail command */
int	msgCount;			/* Count of messages read in */
unsigned long ulgMsgCnt;
unsigned long ulgNewMsgCnt;
unsigned long ulgUnseenMsgCnt;
char	*mydomname;			/* My login id in user@system form */
gid_t	myegid;				/* User's effective gid */
uid_t	myeuid;				/* User's effective uid */
char	*mylocalname;			/* My login id in user@system.domain form */
char	myname[PATHSIZE];		/* My login id */
pid_t	mypid;				/* Current process id */
gid_t	myrgid;				/* User's real gid */
uid_t	myruid;				/* User's real uid */
int	newsflg = 0;			/* -I option for netnews */
char	noheader;			/* Suprress initial header listing */
int	noreset;			/* String resets suspended */
char	nosrc;				/* Don't source /etc/mail/mailx.rc */
int	numberstack[REGDEP];		/* Stack of regretted numbers */
char	origname[PATHSIZE];		/* Name of mailfile before expansion */
FILE	*otf;				/* Output temp file buffer */
int	outtty;				/* True if standard output a tty */
FILE	*pipef;				/* Pipe file we have opened */
char	*progname;			/* program name (argv[0]) */
char	*prompt = NOSTR;		/* prompt string */
int	_priv;				/* used by PRIV() macro */
int	rcvmode;			/* True if receiving mail */
int	readonly;			/* Will be unable to rewrite file */
int	regretp;			/* Pointer to TOS of regret tokens */
int	regretstack[REGDEP];		/* Stack of regretted tokens */
struct	ignret	*retain[HSHSIZE];	/* Pointer to retained fields */
int	retaincount = 0;		/* Number of retained fields. */
char	*rflag;				/* -r address for network */
int	sawcom;				/* Set after first command */
int	selfsent;			/* User sent self something */
int	senderr;			/* An error while checking */
int	sending;			/* TRUE==>sending mail; FALSE==>printing mail */
char	*sflag;				/* Subject given from non tty */
int	sourcing;			/* Currently reading variant file */
int	space;				/* Current maximum number of messages */
jmp_buf	srbuf;
struct strings stringdope[NSPACE];	/*
					 * The pointers for the string allocation routines,
					 * there are NSPACE independent areas.
					 * The first holds STRINGSIZE bytes, the next
					 * twice as much, and so on.
					 */
char	*stringstack[REGDEP];		/* Stack of regretted strings */
char	tempEdit[PATHSIZE];			/* ???? */
char	tempMail[PATHSIZE];			/* ???? */
char	tempMesg[PATHSIZE];			/* ???? */
char	tempQuit[PATHSIZE];			/* ???? */
char	tempSet[PATHSIZE];			/* ???? */
char	tempZedit[PATHSIZE];			/* ???? */
char	tempResid[PATHSIZE];		/* temp file in :saved */
int	tflag = 0;			/* -t use To: fields to get recipients */
uid_t	uid;				/* The invoker's user id */
static struct utimbuf	utimeb;		/* holds time stamp for mailbox */
struct utimbuf	*utimep = &utimeb;	/* pointer to time stamp for mailbox */
struct	var	*variables[HSHSIZE];	/* Pointer to active var list */

#ifdef SVR4ES
eucwidth_t	wp;
int	maxeucw;
#endif

const	char
	appended[] = "[Appended]",
	appendedid[] = ":240",
	ateof[] = ":239:At EOF\n",
	badchdir[] = ":246:Cannot change directory to %s: %s\n",
	badchmod[] = ":535:Cannot change mode of %s: %s\n",
	badexec[] = ":12:Cannot execute %s: %s\n",
	badopen[] = ":2:Cannot open %s: %s\n",
	badread1[] = ":263:Cannot read: %s\n",
	badrename[] = ":532:Cannot rename %s to %s: %s\n",
	badtmpopen[] = ":646:Cannot open temporary file: %s\n",
	badwrite1[] = ":264:Cannot write: %s\n",
	badwrite[] = ":260:Cannot write %s: %s\n",
	binarysize[] = ":243:%s binary/%ld\n",
	cmdfailed[] = ":262:\"%s\" failed\n",
	errmsg[] = ":248:%s: %s\n",
	failed[] = ":238:%s() failed: %s\n",
	failed_metamail_tmp[] = ":531:Failed to create temp file for metamail use: %s\n",
	filedothexist[] = ":259:%s: file exists\n",
	forwardbeginid[] = ":512",
	forwardbegin[] = "-------------------- begin forwarded message --------------------",
	forwardendid[] = ":513",
	forwardend[] = "-------------------- end of forwarded message --------------------",
	hasinterrupted[] = ":254:Interrupt\n",
	hasnomail[] = ":141:No mail.\n",
	hasnomailfor[] = ":250:No mail for %s\n",
	heldmsgs[] = ":257:Held %d messages in %s\n",
	heldonemsg[] = ":256:Held 1 message in %s\n",
	illegalmsglist[] = ":252:Illegal use of \"message list\"\n",
	inappropmsg[] = ":255:%d: Inappropriate message\n",
	msgbcc[] = "Bcc: ",
	msgbccid[] = ":268",
	msgcc[] = "Cc: ",
	msgccid[] = ":267",
	msgsubject[] = "Subject: ",
	msgsubjectid[] = ":265",
	msgto[] = "To: ",
	msgtoid[] = ":266",
	newfile[] = "[New file]",
	newfileid[] = ":241",
	newmailarrived[] = ":251:New mail has arrived.\n",
	noapplicmsgs[] = ":253:No applicable messages\n",
	nofieldignored[] = ":244:No fields currently being ignored.\n",
	nofieldretained[] = ":504:No fields currently being retained.\n",
	nohelp[] = ":245:No help just now.\n",
	nomatchingif[] = ":261:\"%s\" without matching \"if\"\n",
	nomem[] = ":10:Out of memory: %s\n",
	textsize[] = ":242:%s %ld/%ld\n",
	toolongtoedit[] = ":258:Too long to edit\n",
	unexpectedEOF[] = ":249:(Unexpected end-of-file).\n",
	usercont[] = "(continue)\n",
	usercontid[] = ":247",
	usercontinue[] = ":247:(continue)\n";
