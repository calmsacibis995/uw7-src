#ident	"@(#)def.h	11.1"
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

#ident "@(#)def.h	1.34 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#ifdef USE_TERMIOS
# include <termios.h>
#else
# include <termio.h>
#endif
#include <setjmp.h>
#include <time.h>
#include <sys/stat.h>
#include "../libmail/maillock.h"
#include <ctype.h>
#include <errno.h>
#if !defined(preSVr4) || defined(sun)
# include <unistd.h>
#endif
#ifndef preSVr4
# include <stdlib.h>
# include <ulimit.h>
# include <wait.h>
# include <locale.h>
# include <sys/euc.h>
# ifdef SVR4ES
#  include <libw.h>
# endif
#else
/* arguments for access(2) defined in <unistd.h> */
# ifndef R_OK
#  define	R_OK	4	/* Test for Read permission */
#  define	W_OK	2	/* Test for Write permission */
#  define	X_OK	1	/* Test for eXecute permission */
#  define	F_OK	0	/* Test for existence of File */
# endif /* !R_OK */
#endif
#include <pwd.h>
#include "../c-client/mail.h"
#include "local.h"
#include "uparm.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 */

#ifdef preSVr4	/* support for void* */
# define VOID		char
#else
# define VOID		void
#endif

#define	SENDESC		'~'		/* Default escape for sending */
#define	NMLSIZE		1024		/* max names in a message list */
#define	PATHSIZE	1024		/* Size of pathnames throughout */
#define	HSHSIZE		19		/* Hash size for aliases and vars */
#define	LINESIZE	5120		/* max readable line width */
#define	STRINGSIZE	((unsigned) 128)/* Dynamic allocation units */
#define	MAXARGC		1024		/* Maximum list of raw strings */
#define	NOSTR		((char *) 0)	/* Nill string pointer */
#define	NOSTRPTR	((char **) 0)	/* Nill pointer to string pointer */
#define	NOINTPTR	((int *) 0)	/* Nill pointer */
#define	MAXEXP		25		/* Maximum expansion of aliases */

#define	equal(a, b)	(strcmp(a,b)==0)/* A nice function to string compare */
#define fopen(s,t)	my_fopen(s,t)	/* Keep a list of all opened files */
#define fclose(s)	my_fclose(s)	/* delete closed file from the list*/
#define	plural(n)	((n) == 1 ? "" : "s")

typedef enum {
	M_text,			/* content type is 7-bit ASCII */
	M_gtext,		/* content type is generic text */
	M_binary		/* content type is binary */
} t_Content;

struct message {
	off_t		m_offset;		/* offset in block of message */
	long		m_size;			/* Bytes in the message */
	long		m_lines;		/* Lines in the message */
	long		m_clen;			/* Content-Length: of the mesg body */
	short		m_flag;			/* flags, see below */
	char		*m_encoding_type;	/* Encoding-Type: of the mesg */
	char		*m_content_type;	/* Content-Type: of the mesg */
	t_Content	m_text;			/* M_text, M_gtext, M_binary */
	char		m_mime_version;		/* Mime-Version: header is present */
};

typedef struct fplst {
	FILE	*fp;
	struct	fplst	*next;
} NODE;

/*
 * flag bits.
 */

#define	MUSED		(1<<0)		/* entry is used, but this bit isn't */
#define	MDELETED	(1<<1)		/* entry has been deleted */
#define	MSAVED		(1<<2)		/* entry has been saved */
#define	MTOUCH		(1<<3)		/* entry has been noticed */
#define	MPRESERVE	(1<<4)		/* keep entry in sys mailbox */
#define	MMARK		(1<<5)		/* message is marked! */
#define	MODIFY		(1<<6)		/* message has been modified */
#define	MNEW		(1<<7)		/* message has never been seen */
#define	MREAD		(1<<8)		/* message has been read sometime. */
#define	MSTATUS		(1<<9)		/* message status has changed */
#define	MBOX		(1<<10)		/* Send this to mbox, regardless */

#define	H_CLEN		1		/* "Content-Length:"      */
#define	H_CTYPE		2		/* "Content-Type:"        */
#define	H_DATE		3		/* "Date:" 		  */
#define	H_FROM		4		/* "From " 		  */
#define	H_FROM1		5		/* ">From " 		  */
#define	H_FROM2		6		/* "From:" 		  */
#define	H_RECEIVED	7		/* "Received:"	 	  */
#define	H_STATUS	8		/* "Status:"		  */
#define	H_SUBJ		9		/* "Subject:" 		  */
#define	H_TO		10		/* "To:" 		  */
#define	H_ENCDTYPE	11		/* "Encoding-Type:"	  */
#define	H_MIME_VERSION	12		/* "Mime-Version:"	  */
#define H_CONT		13		/* Continuation of previous line */
#define H_NAMEVALUE	14		/* unrecognized "name: value" hdr line*/

/*
 * Format of the command description table.
 * The actual table is declared and initialized
 * in lex.c
 */

struct cmd {
	char	*c_name;		/* Name of command */
	int	(*c_func)();		/* Implementor of the command */
	short	c_argtype;		/* Type of arglist (see below) */
	short	c_msgflag;		/* Required flags of messages */
	short	c_msgmask;		/* Relevant flags of messages */
};

/* can't initialize unions */

#define	c_minargs c_msgflag		/* Minimum argcount for RAWLIST */
#define	c_maxargs c_msgmask		/* Max argcount for RAWLIST */

/*
 * Argument types.
 */

#define	MSGLIST	 0		/* Message list type */
#define	STRLIST	 1		/* A pure string */
#define	RAWLIST	 2		/* Shell string list */
#define	NOLIST	 3		/* Just plain 0 */
#define	NDMLIST	 4		/* Message list, no defaults */

#define	P	040		/* Autoprint dot after command */
#define	I	0100		/* Interactive command bit */
#define	M	0200		/* Legal from send mode bit */
#define	W	0400		/* Illegal when read only bit */
#define	F	01000		/* Is a conditional command */
#define	TX	02000		/* Is a transparent command */
#define	R	04000		/* Cannot be called from collect */

/*
 * Oft-used mask values
 */

#define	MMNORM		(MDELETED|MSAVED)/* Look at both save and delete bits */
#define	MMNDEL		MDELETED	/* Look only at deleted bit */

/*
 * Structure used to return a break down of a head
 * line
 */

struct headline {
	char	*l_from;	/* The name of the sender */
	char	*l_tty;		/* His tty string (if any) */
	char	*l_date;	/* The entire date string */
};

/*
 * Bits passed to puthead() and used in (struct name).n_type
 */
#define	GDEL	(1<<0)		/* This address removed from list */
#define	GNL	(1<<1)		/* Print blank line after header */
#define GOTHER	(1<<2)		/* Other header lines */
#define	GUFROM	(1<<3)		/* From [rmail] */
#define	GFROM	(1<<4)		/* From: */
#define	GTO	(1<<5)		/* To: */
#define	GCC	(1<<6)		/* Cc: */
#define	GBCC	(1<<7)		/* Bcc: */
#define	GDEFOPT	(1<<8)		/* Default-Options: */
#define GENCTYP	(1<<9)		/* Encoding-Type: */
#define	GSUBJECT (1<<10)	/* Subject: */
#define GCLEN	(1<<11)		/* Content-Length: */
#define GCTYPE	(1<<12)		/* Content-Type: */
#define GCTE	(1<<13)		/* Content-Transfer-Encoding: */
#define GMIMEV	(1<<14)		/* Mime-Version: */
#define GDATE	(1<<15)		/* Date: */
#define	GMASK	(GTO|GCC|GBCC|GDEFOPT|GENCTYP|GSUBJECT|GOTHER|GCLEN|GNL|GCTYPE|GCTE|GMIMEV|GDATE)
				/* Mask of all header lines but From's */
#define	GFULL	(GMASK|GFROM|GUFROM)
				/* Mask of all header lines */

/*
 * Structure used to pass about the current
 * state of the user-typed message header.
 */

struct header {
	char	*h_to;				/* Dynamic "To:" string */
	char	*h_subject;			/* Subject string */
	char	*h_cc;				/* Carbon copies string */
	char	*h_bcc;				/* Blind carbon copies */
	char	*h_date;			/* Date */
	char	*h_defopt;			/* Default options */
	char	*h_encodingtype;		/* Encoding-Type of message */
	char	*h_content_type;		/* Content-Type of message */
	char	*h_content_transfer_encoding;	/* Content-Transfer-Encoding of message */
	char	*h_mime_version;		/* Mime-Version of message */
	char	**h_others;			/* Other header lines */
	int	h_seq;				/* Sequence for optimization */
};

/*
 * Structure of namelist nodes used in processing
 * the recipients of mail and aliases and all that
 * kind of stuff.
 */

struct name {
	struct	name *n_link;		/* Singly linked list */
	short	n_type;			/* From which list it came */
	char	*n_name;		/* This person's address */
	char	*n_full;		/* Full name */
};

/*
 * Structure of a variable node.  All variables are
 * kept on a singly-linked list of these, rooted by
 * "variables"
 */

struct var {
	struct	var *v_link;		/* Forward link to next variable */
	char	*v_name;		/* The variable's name */
	char	*v_value;		/* And it's current value */
};

struct mgroup {
	struct	mgroup *ge_link;	/* Next person in this group */
	char	*ge_name;		/* This person's user name */
};

struct grouphead {
	struct	grouphead *g_link;	/* Next grouphead in list */
	char	*g_name;		/* Name of this group */
	struct	mgroup *g_list;		/* Users in group. */
};

/* conflicts with c-client/mail.h require NIL to be 0, not a NIL struct ptr.
 * comment it out here!
#define	NIL	((struct name *) 0)	/* The nil pointer for namelists */
#define	NONE	((struct cmd *) 0)	/* The nil pointer to command tab */
#define	NOVAR	((struct var *) 0)	/* The nil pointer to variables */
#define	NOGRP	((struct grouphead *) 0)/* The nil grouphead pointer */
#define	NOGE	((struct mgroup *) 0)	/* The nil group pointer */
#define	NOFP	((struct fplst *) 0)	/* The nil file pointer */

#define TRUE	1
#define FALSE	0

#define	LSIZE		BUFSIZ		/* maximum size of a line */
#define DEADPERM	0600		/* permissions of dead.letter */
#define TEMPPERM	0600		/* permissions of temp files */
#define MBOXPERM	0600		/* permissions of ~/mbox */

#ifndef	MFMODE
# define MFMODE		0660		/* create mode for `/var/mail' files */
#endif

/*
 * Structure of the hash table of ignored/retained header fields
 */
struct ignret {
	struct ignret	*i_link;	/* Next ignored/retained field in bucket */
	char		*i_field;	/* This ignored/retained field */
};

/* Default_display indicates whether to display this header line to the TTY */
/* when in default mode. Can be overridden via 'P' command at ? prompt */
struct hdr {
	char	*tag;
	int	default_display;
	int	length;
};

#ifdef preSVr4
struct utimbuf {
	time_t	actime;
	time_t	modtime;
};
#else
#  include	<utime.h>
#endif

/*
 * Token values returned by the scanner used for argument lists.
 * Also, sizes of scanner-related things.
 */

#define	TEOL		0		/* End of the command line */
#define	TNUMBER		1		/* A message number */
#define	TDASH		2		/* A simple dash */
#define	TSTRING		3		/* A string (possibly containing -) */
#define	TDOT		4		/* A "." */
#define	TUP		5		/* An "^" */
#define	TDOLLAR		6		/* A "$" */
#define	TSTAR		7		/* A "*" */
#define TPLUS		8		/* A '+' */

#define	REGDEP		2		/* Maximum regret depth. */
#define	STRINGLEN	64		/* Maximum length of string token */

/*
 * Constants for conditional commands.  These describe whether
 * we should be executing stuff or not.
 */

#define	CANY		0		/* Execute in send or receive mode */
#define	CRCV		1		/* Execute in receive mode only */
#define	CSEND		2		/* Execute in send mode only */

/*
 * VM/UNIX has a vfork system call which is faster than forking.  If we
 * don't have it, fork(2) will do . . .
 */

#ifndef VMUNIX
#define	vfork()	fork()
#endif

/*
 * The pointers for the string allocation routines,
 * there are NSPACE independent areas.
 * The first holds STRINGSIZE bytes, the next
 * twice as much, and so on.
 */

#define	NSPACE	25			/* Total number of string spaces */
struct strings {
	char	*s_topFree;		/* Beginning of this area */
	char	*s_nextFree;		/* Next alloctable place here */
	unsigned s_nleft;		/* Number of bytes left here */
};

#define PRIV(x)		(setgid(myegid), (_priv = (int)(x)), setgid(myrgid), _priv)

#ifdef __STDC__
#define ARGS(x)	x
#else
#define const /* nothing */
#define volatile /* nothing */
#define ARGS(x) (/* nothing */)
#endif

/* The following typedefs must be used in SVR4. Some Pre-SVR4 systems already have them. */
#ifdef preSVr4
# ifndef HAVE_GID_T
typedef int gid_t;
# endif
# ifndef HAVE_UID_T
typedef int uid_t;
# endif
# ifndef HAVE_MODE_T
typedef int mode_t;
# endif
# ifndef HAVE_PID_T
typedef int pid_t;
# endif
#endif

#ifdef SVR4ES
# include <pfmt.h>
#else
# define MM_ERROR	0
# define MM_HALT	1
# define MM_WARNING	2
# define MM_INFO	3
# define MM_NOSTD	0x100
# define MM_NOGET	0x200
# define MM_ACTION	0x400
# define MM_CONSOLE	0x800
# define MAXLABEL	25
# ifdef __STDC__
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif
extern int pfmt ARGS((FILE *, long, const char *, ...));
extern char *gettxt ARGS((const char *id, const char *msg));
#endif

/* from libmail */
extern	int bang_collapse ARGS((char *s));
extern int cascmp ARGS((const char *s1, const char *s2));
extern int casncmp ARGS((const char *s1, const char *s2, int n));
extern int copystream ARGS((FILE *infp, FILE *outfp));
extern const char *skipspace ARGS((const char *p));
extern const char *skiptospace ARGS((const char *p));
extern const char *mail_get_charset ARGS(());

/*
 * Forward declarations of routine types to keep lint and cc happy.
 */

extern char		*addone ARGS((char*,char*));
extern char		*addto ARGS((char*,char*));
extern void		almostputc ARGS((int, FILE *, int));
extern void		alter ARGS((const char*));
extern int		alternates ARGS((char**));
extern void		announce ARGS((void));
extern int		any ARGS((int,const char*));
extern int		anyof ARGS((const char*,const char*));
extern int		argcount ARGS((char**));
extern void		assign ARGS((char*,const char*));
extern int		beditor ARGS((int*));
extern int		blankline ARGS((const char*));
extern int		btop ARGS((int*));
extern int		btype ARGS((int*));
extern int		Btype ARGS((int*));
extern int		bvisual ARGS((int*));
extern struct name	*cat ARGS((struct name*,struct name*));
extern FILE		*collect ARGS((struct header*, int*));
extern void		commands ARGS((void));
extern int		compstrs ARGS((const VOID *l, const VOID *r));
extern char		*copy ARGS((const char*,char*));
extern int		Copy ARGS((int*));
extern int		copycmd ARGS((char*));
extern int		deassign ARGS((char*));
extern int		delete ARGS((int*));
extern struct name	*delname ARGS((struct name*,const char*));
extern int		deltype ARGS((int*));
extern char		*detract ARGS((struct name*,int));
extern int		docomma ARGS((char*));
extern int		DoesNeedPortableNewlines  ARGS((char *));
extern int		dopipe ARGS((char*));
extern int		doPipe ARGS((char*));
extern int		echo ARGS((char*));
extern int		editor ARGS((int*));
extern int		edstop ARGS((void));
extern struct name	*elide ARGS((struct name*));
extern int		elsecmd ARGS((void));
extern int		endifcmd ARGS((void));
extern int		execute ARGS((char*,int));
extern const char	*expand ARGS((const char*));
extern struct name	*extract ARGS((char*,int));
extern FILE		*Fdopen ARGS((int,char*));
extern int		file ARGS((char**));
extern struct grouphead	*findgroup ARGS((char*));
extern void		findmail ARGS((void));
extern int		first ARGS((int,int));
extern void		flush ARGS((void));
extern int		folders ARGS((void));
extern int		followup ARGS((int*));
extern int		followupall ARGS((int*));
extern int		Followup ARGS((int*));
extern int		Followupall ARGS((int*));
extern int		forwardmail ARGS((char[]));
extern int		Forwardmail ARGS((char[]));
extern int		from ARGS((int*));
extern void		from64 ARGS((FILE *, FILE*, char **, int *, int));
extern void		fromqp ARGS((FILE *, FILE*, char **, int *));
extern off_t		fsize ARGS((FILE*));
extern const char	*Getf ARGS((const char*));
extern int		getfold ARGS((char*));
extern int		gethfield ARGS((FILE*,char*,long));
extern int		getline ARGS((char*,int,FILE*,int*));
extern int		getln ARGS((char *, int, FILE *));
extern int		getmsglist ARGS((const char*,int*,int));
extern int		getrawlist ARGS((char*,char**));
extern void		getrecf ARGS((char*,char*,int));
extern uid_t		getuserid ARGS((char*));
extern int		grabh ARGS((struct header*,int,int));
extern int		group ARGS((char**));
extern void		hangup ARGS((int));
extern int		hash ARGS((const char*));
extern char		*hcontents ARGS((char*));
extern int		headerp ARGS((char*));
extern int		headers ARGS((int*));
extern int		Headers ARGS((int*));
extern int		help ARGS((void));
extern char		*helppath ARGS((char*));
extern char		*hfield ARGS((char*,struct message*,char*(*)()));
extern void		holdsigs ARGS((void));
extern int		icequal ARGS((char*,char*));
extern int		ifcmd ARGS((char**));
extern int		igfield ARGS((char**));
extern void		inithost ARGS((void));
extern int		isdir ARGS((const char*));
extern int		isheader ARGS((char*,int*));
extern int		ishfield ARGS((char*,char*));
extern int		ishost ARGS((char*,char*));
extern int		isign ARGS((char*));
extern int		isignretained ARGS((char *field, struct ignret *ignretbuf[]));
extern int		ismbox ARGS((const char*));
extern t_Content	istext ARGS((unsigned char*,int,t_Content));
extern void		istrcpy ARGS((char*,char*));
extern void		lcwrite ARGS((const char*,FILE*,FILE*));
extern int		linecount ARGS((char*,int));
extern void		load ARGS((const char*));
extern int		lock ARGS((FILE*,char*,int));
extern void		lockmail ARGS((void));
extern const char	*lister ARGS((void));
extern int		mail ARGS((char**));
extern void		mail1 ARGS((struct header*,char*,int*));
extern int		mailt ARGS((void));
extern void		mapf ARGS((struct name*,char*));
extern int		mboxit ARGS((int*));
extern void		mechk ARGS((struct name*));
extern int		messize ARGS((int*));
extern int		my_fclose ARGS((FILE*));
extern FILE		*my_fopen ARGS((const char*,const char*));
extern int		name ARGS((struct message*));
extern char		*nameof ARGS((struct message*));
extern char		*netmap ARGS((char*,char*));
extern int		New ARGS((int*));
extern int		newfileinfo ARGS((void));
extern int		newmail ARGS((void));
extern int		next ARGS((int*));
extern void		nomemcheck ARGS((const VOID *));
extern int		npclose ARGS((FILE*));
extern FILE		*npopen ARGS((const char*,const char*));
extern int		null ARGS((char*));
extern int		outof ARGS((struct name*,struct header*,FILE*,off_t));
extern struct name	*outpre ARGS((struct name*));
extern const char	*pager ARGS((void));
extern int		panic ARGS((char*));
extern void		parse ARGS((char*,struct headline*,char*));
extern VOID		*pcalloc ARGS((unsigned));
extern int		pcmdlist ARGS((void));
extern int		pdot ARGS((void));
extern int		pghelpfile ARGS((const char *helpfile));
extern int		preserve ARGS((int*));
extern void		printgroup ARGS((char*));
extern void		printhead ARGS((int,int));
extern int		puthead ARGS((struct header*,FILE*,int,FILE*,int));
extern int		pversion ARGS((char*));
extern void		quit ARGS((void));
extern int		readline ARGS((FILE*,char*));
extern void		receipt ARGS((struct message*));
extern char		*reedit ARGS((char *subj));
extern void		relsesigs ARGS((void));
extern int		removefile ARGS((const char*));
extern int		respond ARGS((int*));
extern int		Respond ARGS((int*));
extern int		respondall ARGS((int*,int));
extern int		replysender ARGS((int*,int));
extern int		retainfield ARGS((char**));
extern int		rexit ARGS((int));
extern const char	*safeexpand ARGS((const char*));
extern VOID		*salloc ARGS((unsigned));
extern int		Save ARGS((int*));
extern int		savemail ARGS((const char*,int,struct header*,FILE*,off_t,int,int));
extern VOID		*srealloc ARGS((VOID*,unsigned));
extern int		samebody ARGS((const char*,const char*));
extern int		save ARGS((char*));
extern void		savedead ARGS((int));
extern char		*savestr ARGS((const char*));
extern int		schdir ARGS((const char*));
extern int		scroll ARGS((char*));
extern long		sendmsg ARGS((struct message*,FILE*,int,int,int,int));
extern int		sendm ARGS((char*));
extern int		Sendm ARGS((char*));
extern int		sendmail ARGS((char*));
extern int		Sendmail ARGS((char*));
extern int		set ARGS((char**));
extern int		setfile ARGS((const char*,int));
extern MAILSTREAM	*setinput ARGS((struct message*));
extern void		setptr ARGS((MAILSTREAM*));
extern void		set_metamail_env ARGS((void));
extern int		shell ARGS((char*));
extern int		showheaders ARGS((char[]));
extern void		sigchild ARGS((void));
#if !defined(SIG_HOLD) || defined(NEED_SIGRETRO)
extern void		(*sigset ARGS((int i, void(*f)(int)))) ARGS((int));
#endif
extern char		*skin ARGS((char*));
extern char		*snarf2 ARGS((char*,int*));
extern int		source ARGS((const char*));
extern char		*splice ARGS((char*,char*));
extern void		sreset ARGS((void));
extern void		stop ARGS((int));
extern int		stouch ARGS((int*));
extern int		swrite ARGS((char*));
extern struct name	*tailof ARGS((struct name*));
extern void		tinit ARGS((void));
extern void		to64 ARGS((FILE *, FILE *, int));
extern void		toqp ARGS((FILE *, FILE *));
extern int		top ARGS((int*));
extern void		touch ARGS((int));
extern struct name	*translate ARGS((struct name*));
extern int		type ARGS((int*));
extern int		Type ARGS((int*));
extern int		undelete ARGS((int*));
extern int		ungroup ARGS((char**));
extern int		unigfield ARGS((char**));
extern void		unlockmail ARGS((void));
extern char		**unpack ARGS((struct name*));
extern int		unretainfield ARGS((char**));
extern int		unset ARGS((char**));
extern int		unstack ARGS((void));
extern struct name	*usermap ARGS((struct name*));
extern char		*value ARGS((const char*));
extern char		*vcopy ARGS((const char*));
extern void		vfree ARGS((char *cp));
extern void		Verhogen ARGS((void));
extern int		visual ARGS((int*));
extern char		*yankword ARGS((char*,char*,int));
extern char		*xappend ARGS((char *mem, const char *newmem));
extern char		*xdup ARGS((const char *mem));
extern char 		*retrieve_message ARGS((register struct message *mp,
				STRINGLIST *slist,int part,int lines));
extern VOID		setflags(struct message *);

/*
 * These functions are defined in libmail.a
 */
extern int		delempty ARGS((mode_t, MAILSTREAM*));
extern char		*maildomain ARGS((void));
extern char		*mailsystem ARGS((int));
extern char		*Strerror ARGS((int));
extern int		substr ARGS((const char*, const char*));
extern void		trimnl ARGS((char *s));

/*
 * Standard functions from the C library.
 * These are all defined in <stdlib.h> and <wait.h> in SVr4.
 */
#ifdef preSVr4
extern long		atol();
extern char		*getcwd();
extern char		*calloc();
# ifdef lint
extern void		clearerr();
# endif
extern void		endpwent();
extern void		exit();
extern void		_exit();
extern char		*getenv();
extern unsigned short	getegid();
extern unsigned short	getgid();
extern unsigned short	geteuid();
extern unsigned short	getuid();
extern void		exit();
extern void		free();
extern long		lseek();
extern char		*malloc();
extern char		*mktemp();
extern void		qsort();
extern char		*realloc();
extern void		setpwent();
extern time_t		time();
extern long		ulimit();
extern int		utime();
extern int		wait();
#endif

/* versions of ctype.h macros that first guarantee that the character is unsigned */
#define Iscntrl(x) iscntrl((unsigned char)(x))
#define Isdigit(x) isdigit((unsigned char)(x))
#define Islower(x) islower((unsigned char)(x))
#define Isprint(x) isprint((unsigned char)(x))
#define Isspace(x) isspace((unsigned char)(x))
