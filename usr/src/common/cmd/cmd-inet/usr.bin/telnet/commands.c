#ident	"@(#)commands.c	1.6"

/*
 * Copyright (c) 1988, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)commands.c	8.4 (Berkeley) 5/30/95";
#endif /* not lint */

#if	defined(unix)
#include <sys/param.h>
#if	defined(CRAY) || defined(sysV88)
#include <sys/types.h>
#endif
#include <sys/file.h>
#else
#include <sys/types.h>
#endif	/* defined(unix) */
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef	CRAY
#include <fcntl.h>
#endif	/* CRAY */

#include <signal.h>
#include <netdb.h>
#include <ctype.h>
#include <pwd.h>
#include <varargs.h>
#include <errno.h>

#include <arpa/telnet.h>

#include "general.h"

#include "ring.h"

#include "externs.h"
#include "defines.h"
#include "types.h"

#if !defined(CRAY) && !defined(sysV88)
#include <netinet/in_systm.h>
# if (defined(vax) || defined(tahoe) || defined(hp300)) && !defined(ultrix)
# include <machine/endian.h>
# endif /* vax */
#endif /* !defined(CRAY) && !defined(sysV88) */
#include <netinet/ip.h>

#ifdef	INTL
#  include <locale.h>
#  include "telnet_msg.h"
   extern nl_catd catd;
#  define MN(num, str)		(num),(str)
#  define MNSTR(num, str)	MSGSTR((num),(str))
#else
#  define MSGSTR(num, str)	(str)
#  define MNSTR(num, str)	(str)
#  define MN(num, str)		(str)
#endif	/* INTL */

#ifndef	MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN 64
#endif	MAXHOSTNAMELEN

#if	defined(IPPROTO_IP) && defined(IP_TOS)
int tos = -1;
#endif	/* defined(IPPROTO_IP) && defined(IP_TOS) */

char	*hostname;
static char _hostname[MAXHOSTNAMELEN];

extern char *getenv();

extern int isprefix();
extern char **genget();
extern int Ambiguous();

static call();

typedef struct {
	char	*name;		/* command name */
#ifdef	INTL
	int	helpnum;	/* Message number of help string */
#endif
	char	*help;		/* help string (NULL for no help) */
	int	(*handler)();	/* routine which executes command */
	int	needconnect;	/* Do we need to be connected to execute? */
} Command;

static char line[256];
static char saveline[256];
static int margc;
static char *margv[20];

    static void
makeargv()
{
    register char *cp, *cp2, c;
    register char **argp = margv;

    margc = 0;
    cp = line;
    if (*cp == '!') {		/* Special case shell escape */
	strcpy(saveline, line);	/* save for shell command */
	*argp++ = "!";		/* No room in string to get this */
	margc++;
	cp++;
    }
    while (c = *cp) {
	register int inquote = 0;
	while (isspace(c))
	    c = *++cp;
	if (c == '\0')
	    break;
	*argp++ = cp;
	margc += 1;
	for (cp2 = cp; c != '\0'; c = *++cp) {
	    if (inquote) {
		if (c == inquote) {
		    inquote = 0;
		    continue;
		}
	    } else {
		if (c == '\\') {
		    if ((c = *++cp) == '\0')
			break;
		} else if (c == '"') {
		    inquote = '"';
		    continue;
		} else if (c == '\'') {
		    inquote = '\'';
		    continue;
		} else if (isspace(c))
		    break;
	    }
	    *cp2++ = c;
	}
	*cp2 = '\0';
	if (c == '\0')
	    break;
	cp++;
    }
    *argp++ = 0;
}

/*
 * Make a character string into a number.
 *
 * Todo:  1.  Could take random integers (12, 0x12, 012, 0b1).
 */

	static
special(s)
	register char *s;
{
	register char c;
	char b;

	switch (*s) {
	case '^':
		b = *++s;
		if (b == '?') {
		    c = b | 0x40;		/* DEL */
		} else {
		    c = b & 0x1f;
		}
		break;
	default:
		c = *s;
		break;
	}
	return c;
}

/*
 * Construct a control character sequence
 * for a special character.
 */
	static char *
control(c)
	register cc_t c;
{
	static char buf[5];
	/*
	 * The only way I could get the Sun 3.5 compiler
	 * to shut up about
	 *	if ((unsigned int)c >= 0x80)
	 * was to assign "c" to an unsigned int variable...
	 * Arggg....
	 */
	register unsigned int uic = (unsigned int)c;

	if (uic == 0x7f)
		return ("^?");
	if (c == (cc_t)_POSIX_VDISABLE) {
		return "off";
	}
	if (uic >= 0x80) {
		buf[0] = '\\';
		buf[1] = ((c>>6)&07) + '0';
		buf[2] = ((c>>3)&07) + '0';
		buf[3] = (c&07) + '0';
		buf[4] = 0;
	} else if (uic >= 0x20) {
		buf[0] = c;
		buf[1] = 0;
	} else {
		buf[0] = '^';
		buf[1] = '@'+c;
		buf[2] = 0;
	}
	return (buf);
}



/*
 *	The following are data structures and routines for
 *	the "send" command.
 *
 */

struct sendlist {
    char	*name;		/* How user refers to it (case independent) */
#ifdef	INTL
    int		helpnum;	/* Message number of help message */
#endif
    char	*help;		/* Help information (0 ==> no help) */
    int		needconnect;	/* Need to be connected */
    int		narg;		/* Number of arguments */
    int		(*handler)();	/* Routine to perform (for special ops) */
    int		nbyte;		/* Number of bytes to send this command */
    int		what;		/* Character to be sent (<0 ==> special) */
};


static int
	send_esc P((void)),
	send_help P((void)),
	send_docmd P((char *)),
	send_dontcmd P((char *)),
	send_willcmd P((char *)),
	send_wontcmd P((char *));

static struct sendlist Sendlist[] = {
    { "ao",
	MN(MSG_SENDL0, "Send Telnet Abort output"),
	    1, 0, 0, 2, AO },
    { "ayt",
	MN(MSG_SENDL1, "Send Telnet 'Are You There'"),
	    1, 0, 0, 2, AYT },
    { "brk",
	MN(MSG_SENDL2, "Send Telnet Break"),
	    1, 0, 0, 2, BREAK },
    { "break",
	MN(0, 0),
	    1, 0, 0, 2, BREAK },
    { "ec",
	MN(MSG_SENDL3, "Send Telnet Erase Character"),
	    1, 0, 0, 2, EC },
    { "el",
	MN(MSG_SENDL4, "Send Telnet Erase Line"),
	    1, 0, 0, 2, EL },
    { "escape",
	MN(MSG_SENDL5, "Send current escape character"),
	    1, 0, send_esc, 1, 0 },
    { "ga",
	MN(MSG_SENDL6, "Send Telnet 'Go Ahead' sequence"),
	    1, 0, 0, 2, GA },
    { "ip",
	MN(MSG_SENDL7, "Send Telnet Interrupt Process"),
	    1, 0, 0, 2, IP },
    { "intp",
	MN(0, 0),
	    1, 0, 0, 2, IP },
    { "interrupt",
	MN(0, 0),
	    1, 0, 0, 2, IP },
    { "intr",
	MN(0, 0),
	    1, 0, 0, 2, IP },
    { "nop",
	MN(MSG_SENDL8, "Send Telnet 'No operation'"),
	    1, 0, 0, 2, NOP },
    { "eor",
	MN(MSG_SENDL9, "Send Telnet 'End of Record'"),
	    1, 0, 0, 2, EOR },
    { "abort",
	MN(MSG_SENDL10, "Send Telnet 'Abort Process'"),
	    1, 0, 0, 2, ABORT },
    { "susp",
	MN(MSG_SENDL11, "Send Telnet 'Suspend Process'"),
	    1, 0, 0, 2, SUSP },
    { "eof",
	MN(MSG_SENDL12, "Send Telnet End of File Character"),
	    1, 0, 0, 2, xEOF },
    { "synch",
	MN(MSG_SENDL13, "Perform Telnet 'Synch operation'"),
	    1, 0, dosynch, 2, 0 },
    { "getstatus",
	MN(MSG_SENDL14, "Send request for STATUS"),
	    1, 0, get_status, 6, 0 },
    { "?",
	MN(MSG_SENDL15, "Display send options"),
	    0, 0, send_help, 0, 0 },
    { "help",
	MN(0, 0),
	    0, 0, send_help, 0, 0 },
    { "do",
	MN(0, 0),
	    0, 1, send_docmd, 3, 0 },
    { "dont",
	MN(0, 0),
	    0, 1, send_dontcmd, 3, 0 },
    { "will",
	MN(0, 0),
	    0, 1, send_willcmd, 3, 0 },
    { "wont",
	MN(0, 0),
	    0, 1, send_wontcmd, 3, 0 },
    { 0 }
};

#define	GETSEND(name) ((struct sendlist *) genget(name, (char **) Sendlist, \
				sizeof(struct sendlist)))

    static int
sendcmd(argc, argv)
    int  argc;
    char **argv;
{
    int count;		/* how many bytes we are going to need to send */
    int i;
    int question = 0;	/* was at least one argument a question */
    struct sendlist *s;	/* pointer to current command */
    int success = 0;
    int needconnect = 0;

    if (argc < 2) {
	printf(MSGSTR(MSG_NEED_SEND_ARG,
			"need at least one argument for 'send' command\n"));
	printf(MSGSTR(MSG_SEND_HELP, "'send ?' for help\n"));
	return 0;
    }
    /*
     * First, validate all the send arguments.
     * In addition, we see how much space we are going to need, and
     * whether or not we will be doing a "SYNCH" operation (which
     * flushes the network queue).
     */
    count = 0;
    for (i = 1; i < argc; i++) {
	s = GETSEND(argv[i]);
	if (s == 0) {
	    printf(MSGSTR(MSG_UNKNOWN_SEND_ARG,
			"Unknown send argument '%s'\n'send ?' for help.\n"),
			argv[i]);
	    return 0;
	} else if (Ambiguous(s)) {
	    printf(MSGSTR(MSG_AMBIGUOUS_SEND_ARG,
			"Ambiguous send argument '%s'\n'send ?' for help.\n"),
			argv[i]);
	    return 0;
	}
	if (i + s->narg >= argc) {
	    if (s->narg == 1)
		fprintf(stderr, MSGSTR(MSG_SEND_ONE_ARG,
	    "Need 1 argument to 'send %s' command.  'send %s ?' for help.\n"),
			s->name, s->name);
	    else
		fprintf(stderr, MSGSTR(MSG_SEND_N_ARGS,
	   "Need %d arguments to 'send %s' command.  'send %s ?' for help.\n"),
			s->narg, s->name, s->name);
	    return 0;
	}
	count += s->nbyte;
	if (s->handler == send_help) {
	    send_help();
	    return 0;
	}

	i += s->narg;
	needconnect += s->needconnect;
    }
    if (!connected && needconnect) {
	printf(MSGSTR(MSG_CONNECT_FIRST, "?Need to be connected first.\n"));
	printf(MSGSTR(MSG_SEND_HELP, "'send ?' for help\n"));
	return 0;
    }
    /* Now, do we have enough room? */
    if (NETROOM() < count) {
	printf(MSGSTR(MSG_NO_ROOM1, 
		"There is not enough room in the buffer TO the network\n"));
	printf(MSGSTR(MSG_NO_ROOM2,
		"to process your request.  Nothing will be done.\n"));
	printf(MSGSTR(MSG_NO_ROOM3,
		"('send synch' will throw away most data in the network\n"));
	printf(MSGSTR(MSG_NO_ROOM4,
		"buffer, if this might help.)\n"));
	return 0;
    }
    /* OK, they are all OK, now go through again and actually send */
    count = 0;
    for (i = 1; i < argc; i++) {
	if ((s = GETSEND(argv[i])) == 0) {
	    fprintf(stderr, MSGSTR(MSG_SEND_ERROR,
		"Telnet 'send' error - argument disappeared!\n"));
	    (void) quit();
	    /*NOTREACHED*/
	}
	if (s->handler) {
	    count++;
	    success += (*s->handler)((s->narg > 0) ? argv[i+1] : 0,
				  (s->narg > 1) ? argv[i+2] : 0);
	    i += s->narg;
	} else {
	    NET2ADD(IAC, s->what);
	    printoption("SENT", IAC, s->what);
	}
    }
    return (count == success);
}

    static int
send_esc()
{
    NETADD(escape);
    return 1;
}

    static int
send_docmd(name)
    char *name;
{
    return(send_tncmd(send_do, "do", name));
}

    static int
send_dontcmd(name)
    char *name;
{
    return(send_tncmd(send_dont, "dont", name));
}
    static int
send_willcmd(name)
    char *name;
{
    return(send_tncmd(send_will, "will", name));
}
    static int
send_wontcmd(name)
    char *name;
{
    return(send_tncmd(send_wont, "wont", name));
}

    int
send_tncmd(func, cmd, name)
    void	(*func)();
    char	*cmd, *name;
{
    char **cpp;
    extern char *telopts[];
    register int val = 0;

    if (isprefix(name, "help") || isprefix(name, "?")) {
	register int col, len;

	printf(MSGSTR(MSG_SEND_USAGE, "Usage: send %s <value|option>\n"), cmd);
	printf(MSGSTR(MSG_SEND_VALUE, "\"value\" must be from 0 to 255\n"));
	printf(MSGSTR(MSG_SEND_VALID, "Valid options are:\n\t"));

	col = 8;
	for (cpp = telopts; *cpp; cpp++) {
	    len = strlen(*cpp) + 3;
	    if (col + len > 65) {
		printf("\n\t");
		col = 8;
	    }
	    printf(" \"%s\"", *cpp);
	    col += len;
	}
	printf("\n");
	return 0;
    }
    cpp = (char **)genget(name, telopts, sizeof(char *));
    if (Ambiguous(cpp)) {
	fprintf(stderr, MSGSTR(MSG_AMBIGUOUS_ARG,
		"'%s': ambiguous argument ('send %s ?' for help).\n"),
					name, cmd);
	return 0;
    }
    if (cpp) {
	val = cpp - telopts;
    } else {
	register char *cp = name;

	while (*cp >= '0' && *cp <= '9') {
	    val *= 10;
	    val += *cp - '0';
	    cp++;
	}
	if (*cp != 0) {
	    fprintf(stderr, MSGSTR(MSG_SEND_UNKNOWN_ARG,
			"'%s': unknown argument ('send %s ?' for help).\n"),
					name, cmd);
	    return 0;
	} else if (val < 0 || val > 255) {
	    fprintf(stderr, MSGSTR(MSG_SEND_BAD_VALUE,
			"'%s': bad value ('send %s ?' for help).\n"),
					name, cmd);
	    return 0;
	}
    }
    if (!connected) {
	printf(MSGSTR(MSG_CONNECT_FIRST, "?Need to be connected first.\n"));
	return 0;
    }
    (*func)(val, 1);
    return 1;
}

    static int
send_help()
{
    struct sendlist *s;	/* pointer to current command */
    for (s = Sendlist; s->name; s++) {
	if (s->help)
	    printf("%-15s %s\n", s->name, MNSTR(s->helpnum, s->help));
    }
    return(0);
}

/*
 * The following are the routines and data structures referred
 * to by the arguments to the "toggle" command.
 */

    static int
lclchars()
{
    donelclchars = 1;
    return 1;
}

    static int
togdebug()
{
#ifndef	NOT43
    if (net > 0 &&
	(SetSockOpt(net, SOL_SOCKET, SO_DEBUG, debug)) < 0) {
	    perror("setsockopt (SO_DEBUG)");
    }
#else	/* NOT43 */
    if (debug) {
	if (net > 0 && SetSockOpt(net, SOL_SOCKET, SO_DEBUG, 0, 0) < 0)
	    perror("setsockopt (SO_DEBUG)");
    } else
	printf("Cannot turn off socket debugging\n");
#endif	/* NOT43 */
    return 1;
}


    static int
togcrlf()
{
    if (crlf) {
	printf(MSGSTR(MSG_SEND_CR_LF,
		"Will send carriage returns as telnet <CR><LF>.\n"));
    } else {
	printf(MSGSTR(MSG_SEND_CR_NUL,
		"Will send carriage returns as telnet <CR><NUL>.\n"));
    }
    return 1;
}

int binmode;

    static int
togbinary(val)
    int val;
{
    donebinarytoggle = 1;

    if (val >= 0) {
	binmode = val;
    } else {
	if (my_want_state_is_will(TELOPT_BINARY) &&
				my_want_state_is_do(TELOPT_BINARY)) {
	    binmode = 1;
	} else if (my_want_state_is_wont(TELOPT_BINARY) &&
				my_want_state_is_dont(TELOPT_BINARY)) {
	    binmode = 0;
	}
	val = binmode ? 0 : 1;
    }

    if (val == 1) {
	if (my_want_state_is_will(TELOPT_BINARY) &&
					my_want_state_is_do(TELOPT_BINARY)) {
	    printf(MSGSTR(MSG_ALREADY_IN_BIN_MODE,
		"Already operating in binary mode with remote host.\n"));
	} else {
	    printf(MSGSTR(MSG_NEGOTIATE_BIN_MODE,
		"Negotiating binary mode with remote host.\n"));
	    tel_enter_binary(3);
	}
    } else {
	if (my_want_state_is_wont(TELOPT_BINARY) &&
					my_want_state_is_dont(TELOPT_BINARY)) {
	    printf(MSGSTR(MSG_ALREADY_IN_ASCII_MODE,
		"Already in network ascii mode with remote host.\n"));
	} else {
	    printf(MSGSTR(MSG_NEGOTIATE_ASCII_MODE,
		"Negotiating network ascii mode with remote host.\n"));
	    tel_leave_binary(3);
	}
    }
    return 1;
}

    static int
togrbinary(val)
    int val;
{
    donebinarytoggle = 1;

    if (val == -1)
	val = my_want_state_is_do(TELOPT_BINARY) ? 0 : 1;

    if (val == 1) {
	if (my_want_state_is_do(TELOPT_BINARY)) {
	    printf(MSGSTR(MSG_ALREADY_RX_BINARY,
		"Already receiving in binary mode.\n"));
	} else {
	    printf(MSGSTR(MSG_NEGOTIATE_BINARY_IP,
		"Negotiating binary mode on input.\n"));
	    tel_enter_binary(1);
	}
    } else {
	if (my_want_state_is_dont(TELOPT_BINARY)) {
	    printf(MSGSTR(MSG_ALREADY_RX_ASCII,
		"Already receiving in network ascii mode.\n"));
	} else {
	    printf(MSGSTR(MSG_NEGOTIATE_ASCII_IP,
		"Negotiating network ascii mode on input.\n"));
	    tel_leave_binary(1);
	}
    }
    return 1;
}

    static int
togxbinary(val)
    int val;
{
    donebinarytoggle = 1;

    if (val == -1)
	val = my_want_state_is_will(TELOPT_BINARY) ? 0 : 1;

    if (val == 1) {
	if (my_want_state_is_will(TELOPT_BINARY)) {
	    printf(MSGSTR(MSG_ALREADY_TX_BINARY,
		"Already transmitting in binary mode.\n"));
	} else {
	    printf(MSGSTR(MSG_NEGOTIATE_BINARY_OP,
		"Negotiating binary mode on output.\n"));
	    tel_enter_binary(2);
	}
    } else {
	if (my_want_state_is_wont(TELOPT_BINARY)) {
	    printf(MSGSTR(MSG_ALREADY_TX_ASCII,
		"Already transmitting in network ascii mode.\n"));
	} else {
	    printf(MSGSTR(MSG_NEGOTIATE_ASCII_OP,
		"Negotiating network ascii mode on output.\n"));
	    tel_leave_binary(2);
	}
    }
    return 1;
}


static int togglehelp P((void));
#if	defined(AUTHENTICATION)
extern int auth_togdebug P((int));
#endif
#ifdef	ENCRYPTION
extern int EncryptAutoEnc P((int));
extern int EncryptAutoDec P((int));
extern int EncryptDebug P((int));
extern int EncryptVerbose P((int));
#endif	/* ENCRYPTION */

struct togglelist {
    char	*name;		/* name of toggle */
#ifdef	INTL
    int		t_helpnum;	/* Message number of t_help message */
#endif
    char	*t_help;	/* toggle help message */
#ifdef	INTL
    int		e_helpnum;	/* Message number of e_help message */
#endif
    char	*e_help;	/* enable help message */
#ifdef	INTL
    int		d_helpnum;	/* Message number of d_help message */
#endif
    char	*d_help;	/* disable help message */
    int		(*handler)();	/* routine to do actual setting */
    int		*variable;
#ifdef	INTL
    int		willnum;
#endif
    char	*will_actionexplanation;
#ifdef	INTL
    int		wontnum;
#endif
    char	*wont_actionexplanation;
};

static struct togglelist Togglelist[] = {
    { "autoflush",
MN(MSG_TL0, "toggle flushing of output when sending interrupt characters"),
MN(MSG_TL1, "enable flushing of output when sending interrupt characters"),
MN(MSG_TL2, "disable flushing of output when sending interrupt characters"),
	    0,
		&autoflush,
MN(MSG_TL3, "Will flush output when sending interrupt characters"),
MN(MSG_TL4, "Won't flush output when sending interrupt characters") },
    { "autosynch",
MN(MSG_TL5, "toggle automatic sending of interrupt characters in urgent mode"),
MN(MSG_TL6, "enable automatic sending of interrupt characters in urgent mode"),
MN(MSG_TL7, "disable automatic sending of interrupt characters in urgent mode"),
	    0,
		&autosynch,
MN(MSG_TL8, "Will send interrupt characters in urgent mode"),
MN(MSG_TL9, "Won't send interrupt characters in urgent mode") },
#if	defined(AUTHENTICATION)
    { "autologin",
MN(MSG_TL10, "toggle automatic sending of login and/or authentication info"),
MN(MSG_TL11, "enable automatic sending of login and/or authentication info"),
MN(MSG_TL12, "disable automatic sending of login and/or authentication info"),
	    0,
		&autologin,
MN(MSG_TL13, "Will send login name and/or authentication information"),
MN(MSG_TL14, "Won't send login name and/or authentication information") },
    { "authdebug",
MN(MSG_TL15, "toggle Toggle authentication debugging"),
MN(MSG_TL16, "enable Toggle authentication debugging"),
MN(MSG_TL17, "disable Toggle authentication debugging"),
	    auth_togdebug,
		0,
MN(MSG_TL18, "Will print authentication debugging information"),
MN(MSG_TL19, "Won't print authentication debugging information") },
#endif
#ifdef	ENCRYPTION
    { "autoencrypt",
MN(MSG_TL20, "toggle automatic encryption of data stream"),
MN(MSG_TL21, "enable automatic encryption of data stream"),
MN(MSG_TL22, "disable automatic encryption of data stream"),
	    EncryptAutoEnc,
		0,
MN(MSG_TL23, "Will automatically encrypt output"),
MN(MSG_TL24, "Won't automatically encrypt output") },
    { "autodecrypt",
MN(MSG_TL25, "toggle automatic decryption of data stream"),
MN(MSG_TL26, "enable automatic decryption of data stream"),
MN(MSG_TL27, "disable automatic decryption of data stream"),
	    EncryptAutoDec,
		0,
MN(MSG_TL28, "Will automatically decrypt input"),
MN(MSG_TL29, "Won't automatically decrypt input") },
    { "verbose_encrypt",
MN(MSG_TL30, "toggle Toggle verbose encryption output"),
MN(MSG_TL31, "enable Toggle verbose encryption output"),
MN(MSG_TL32, "disable Toggle verbose encryption output"),
	    EncryptVerbose,
		0,
MN(MSG_TL33, "Will print verbose encryption output"),
MN(MSG_TL34, "Won't print verbose encryption output") },
    { "encdebug",
MN(MSG_TL35, "toggle Toggle encryption debugging"),
MN(MSG_TL36, "enable Toggle encryption debugging"),
MN(MSG_TL37, "disable Toggle encryption debugging"),
	    EncryptDebug,
		0,
MN(MSG_TL38, "Will print encryption debugging information"),
MN(MSG_TL39, "Won't print encryption debugging information") },
#endif	/* ENCRYPTION */
    { "skiprc",
MN(MSG_TL40, "toggle don't read ~/.telnetrc file"),
MN(MSG_TL41, "enable don't read ~/.telnetrc file"),
MN(MSG_TL42, "disable don't read ~/.telnetrc file"),
	    0,
		&skiprc,
MN(MSG_TL43, "Will skip reading of ~/.telnetrc file"),
MN(MSG_TL44, "Won't skip reading of ~/.telnetrc file") },
    { "binary",
MN(MSG_TL45, "toggle sending and receiving of binary data"),
MN(MSG_TL46, "enable sending and receiving of binary data"),
MN(MSG_TL47, "disable sending and receiving of binary data"),
	    togbinary,
		0,
		    0 },
    { "inbinary",
MN(MSG_TL50, "toggle receiving of binary data"),
MN(MSG_TL51, "enable receiving of binary data"),
MN(MSG_TL52, "disable receiving of binary data"),
	    togrbinary,
		0,
		    0 },
    { "outbinary",
MN(MSG_TL55, "toggle sending of binary data"),
MN(MSG_TL56, "enable sending of binary data"),
MN(MSG_TL57, "disable sending of binary data"),
	    togxbinary,
		0,
		    0 },
    { "crlf",
MN(MSG_TL60, "toggle sending carriage returns as telnet <CR><LF>"),
MN(MSG_TL61, "enable sending carriage returns as telnet <CR><LF>"),
MN(MSG_TL62, "disable sending carriage returns as telnet <CR><LF>"),
	    togcrlf,
		&crlf,
		    0 },
    { "crmod",
MN(MSG_TL65, "toggle mapping of received carriage returns"),
MN(MSG_TL66, "enable mapping of received carriage returns"),
MN(MSG_TL67, "disable mapping of received carriage returns"),
	    0,
		&crmod,
MN(MSG_TL68, "Will map carriage return on output"),
MN(MSG_TL69, "Won't map carriage return on output") },
    { "localchars",
MN(MSG_TL70, "toggle local recognition of certain control characters"),
MN(MSG_TL71, "enable local recognition of certain control characters"),
MN(MSG_TL72, "disable local recognition of certain control characters"),
	    lclchars,
		&localchars,
MN(MSG_TL73, "Will recognize certain control characters"),
MN(MSG_TL74, "Won't recognize certain control characters") },
    { " ", MN(0, ""), MN(0, ""), MN(0, ""), 0 },	/* empty line */
#if	defined(unix) && defined(TN3270)
    { "apitrace",
MN(MSG_TL75, "toggle (debugging) toggle tracing of API transactions"),
MN(MSG_TL76, "enable (debugging) toggle tracing of API transactions"),
MN(MSG_TL77, "disable (debugging) toggle tracing of API transactions"),
	    0,
		&apitrace,
MN(MSG_TL78, "Will trace API transactions"),
MN(MSG_TL79, "Won't trace API transactions") },
    { "cursesdata",
MN(MSG_TL80, "toggle (debugging) toggle printing of hexadecimal curses data"),
MN(MSG_TL81, "enable (debugging) toggle printing of hexadecimal curses data"),
MN(MSG_TL82, "disable (debugging) toggle printing of hexadecimal curses data"),
	    0,
		&cursesdata,
MN(MSG_TL83, "Will print hexadecimal representation of curses data"),
MN(MSG_TL84, "Won't print hexadecimal representation of curses data") },
#endif	/* defined(unix) && defined(TN3270) */
    { "debug",
MN(MSG_TL85, "toggle debugging"),
MN(MSG_TL86, "enable debugging"),
MN(MSG_TL87, "disable debugging"),
	    togdebug,
		&debug,
MN(MSG_TL88, "Will turn on socket level debugging"),
MN(MSG_TL89, "Won't turn on socket level debugging") },
    { "netdata",
MN(MSG_TL90, "toggle printing of hexadecimal network data (debugging)"),
MN(MSG_TL91, "enable printing of hexadecimal network data (debugging)"),
MN(MSG_TL92, "disable printing of hexadecimal network data (debugging)"),
	    0,
		&netdata,
MN(MSG_TL93, "Will print hexadecimal representation of network traffic"),
MN(MSG_TL94, "Won't print hexadecimal representation of network traffic") },
    { "prettydump",
MN(MSG_TL95, "toggle output of \"netdata\" to user readable format (debugging)"),
MN(MSG_TL96, "enable output of \"netdata\" to user readable format (debugging)"),
MN(MSG_TL97, "disable output of \"netdata\" to user readable format (debugging)"),
	    0,
		&prettydump,
MN(MSG_TL98, "Will print user readable output for \"netdata\""),
MN(MSG_TL99, "Won't print user readable output for \"netdata\"") },
    { "options",
MN(MSG_TL100, "toggle viewing of options processing (debugging)"),
MN(MSG_TL101, "enable viewing of options processing (debugging)"),
MN(MSG_TL102, "disable viewing of options processing (debugging)"),
	    0,
		&showoptions,
MN(MSG_TL103, "Will show option processing"),
MN(MSG_TL104, "Won't show option processing") },
#if	defined(unix)
    { "termdata",
MN(MSG_TL105, "toggle (debugging) toggle printing of hexadecimal terminal data"),
MN(MSG_TL106, "enable (debugging) toggle printing of hexadecimal terminal data"),
MN(MSG_TL107, "disable (debugging) toggle printing of hexadecimal terminal data"),
	    0,
		&termdata,
MN(MSG_TL108, "Will print hexadecimal representation of terminal traffic"),
MN(MSG_TL109, "Won't print hexadecimal representation of terminal traffic") },
#endif	/* defined(unix) */
    { "?",
MN(0, 0),
MN(0, 0),
MN(0, 0),
	    togglehelp },
    { "help",
MN(0, 0),
MN(0, 0),
MN(0, 0),
	    togglehelp },
    { 0 }
};

    static int
togglehelp()
{
    struct togglelist *c;

    for (c = Togglelist; c->name; c++) {
	if (c->t_help) {
	    if (*c->t_help)
		printf("%-15s %s\n", c->name, MNSTR(c->t_helpnum, c->t_help));
	    else
		printf("\n");
	}
    }
    printf("\n");
    printf("%-15s %s\n", "?", MSGSTR(MSG_DISPLAY_HELP_INFO,
				"display help information"));
    return 0;
}

    static void
settogglehelp(set)
    int set;
{
    struct togglelist *c;

    for (c = Togglelist; c->name; c++) {
	if (set) {
	    if (c->e_help) {
		if (*c->e_help)
		    printf("%-15s %s\n", c->name,
					MNSTR(c->e_helpnum, c->e_help));
		else
		    printf("\n");
	    }
	} else {
	    if (c->d_help) {
		if (*c->d_help)
		    printf("%-15s %s\n", c->name,
					MNSTR(c->d_helpnum, c->d_help));
		else
		    printf("\n");
	    }
	}
    }
}

#define	GETTOGGLE(name) (struct togglelist *) \
		genget(name, (char **) Togglelist, sizeof(struct togglelist))

    static int
toggle(argc, argv)
    int  argc;
    char *argv[];
{
    int retval = 1;
    char *name;
    struct togglelist *c;

    if (argc < 2) {
	fprintf(stderr, MSGSTR(MSG_NEED_TOGGLE_ARG,
	    "Need an argument to 'toggle' command.  'toggle ?' for help.\n"));
	return 0;
    }
    argc--;
    argv++;
    while (argc--) {
	name = *argv++;
	c = GETTOGGLE(name);
	if (Ambiguous(c)) {
	    fprintf(stderr, MSGSTR(MSG_AMBIGUOUS_TOGGLE_ARG,
			"'%s': ambiguous argument ('toggle ?' for help).\n"),
					name);
	    return 0;
	} else if (c == 0) {
	    fprintf(stderr, MSGSTR(MSG_UNKNOWN_TOGGLE_ARG,
			"'%s': unknown argument ('toggle ?' for help).\n"),
					name);
	    return 0;
	} else {
	    if (c->variable) {
		*c->variable = !*c->variable;		/* invert it */
		if (c->will_actionexplanation && *c->variable)
		    printf("%s.\n",
				MNSTR(c->willnum, c->will_actionexplanation));
		else if (c->wont_actionexplanation && !(*c->variable))
		    printf("%s.\n",
				MNSTR(c->wontnum, c->wont_actionexplanation));
	    }
	    if (c->handler) {
		retval &= (*c->handler)(-1);
	    }
	}
    }
    return retval;
}

/*
 * The following perform the "set" command.
 */

#ifdef	USE_TERMIO
struct termio new_tc = { 0 };
#endif

struct setlist {
    char *name;				/* name */
#ifdef	INTL
    int helpnum;			/* Message number of help message */
#endif
    char *help;				/* help information */
    void (*handler)();
    cc_t *charp;			/* where it is located at */
};

static struct setlist Setlist[] = {
#ifdef	KLUDGELINEMODE
    { "echo",
	MN(MSG_SETL0, "character to toggle local echoing on/off"),
	    0, &echoc },
#endif
    { "escape",
	MN(MSG_SETL1, "character to escape back to telnet command mode"),
	    0, &escape },
    { "rlogin",
	MN(MSG_SETL2, "rlogin escape character"),
	    0, &rlogin },
    { "tracefile",
	MN(MSG_SETL3, "file to write trace information to"),
	    SetNetTrace, (cc_t *)NetTraceFile},
    { " ", MN(0, "") },
    { " ",
	MN(MSG_SETL4, "The following need 'localchars' to be toggled true"),
	    0, 0 },
    { "flushoutput",
	MN(MSG_SETL5, "character to cause an Abort Output"),
	    0, termFlushCharp },
    { "interrupt",
	MN(MSG_SETL6, "character to cause an Interrupt Process"),
	    0, termIntCharp },
    { "quit",
	MN(MSG_SETL7, "character to cause an Abort process"),
	    0, termQuitCharp },
    { "eof",
	MN(MSG_SETL8, "character to cause an EOF "),
	    0, termEofCharp },
    { " ", MN(0, "") },
    { " ",
	MN(MSG_SETL9, "The following are for local editing in linemode"),
	    0, 0 },
    { "erase",
	MN(MSG_SETL10, "character to use to erase a character"),
	    0, termEraseCharp },
    { "kill",
	MN(MSG_SETL11, "character to use to erase a line"),
	    0, termKillCharp },
    { "lnext",
	MN(MSG_SETL12, "character to use for literal next"),
	    0, termLiteralNextCharp },
    { "susp",
	MN(MSG_SETL13, "character to cause a Suspend Process"),
	    0, termSuspCharp },
    { "reprint",
	MN(MSG_SETL14, "character to use for line reprint"),
	    0, termRprntCharp },
    { "worderase",
	MN(MSG_SETL15, "character to use to erase a word"),
	    0, termWerasCharp },
    { "start",
	MN(MSG_SETL16, "character to use for XON"),
	    0, termStartCharp },
    { "stop",
	MN(MSG_SETL17, "character to use for XOFF"),
	    0, termStopCharp },
    { "forw1",
	MN(MSG_SETL18, "alternate end of line character"),
	    0, termForw1Charp },
    { "forw2",
	MN(MSG_SETL18, "alternate end of line character"),
	    0, termForw2Charp },
    { "ayt",
	MN(MSG_SETL19, "alternate AYT character"),
	    0, termAytCharp },
    { 0 }
};

#if	defined(CRAY) && !defined(__STDC__)
/* Work around compiler bug in pcc 4.1.5 */
    void
_setlist_init()
{
#ifndef	KLUDGELINEMODE
#define	N 5
#else
#define	N 6
#endif
	Setlist[N+0].charp = &termFlushChar;
	Setlist[N+1].charp = &termIntChar;
	Setlist[N+2].charp = &termQuitChar;
	Setlist[N+3].charp = &termEofChar;
	Setlist[N+6].charp = &termEraseChar;
	Setlist[N+7].charp = &termKillChar;
	Setlist[N+8].charp = &termLiteralNextChar;
	Setlist[N+9].charp = &termSuspChar;
	Setlist[N+10].charp = &termRprntChar;
	Setlist[N+11].charp = &termWerasChar;
	Setlist[N+12].charp = &termStartChar;
	Setlist[N+13].charp = &termStopChar;
	Setlist[N+14].charp = &termForw1Char;
	Setlist[N+15].charp = &termForw2Char;
	Setlist[N+16].charp = &termAytChar;
#undef	N
}
#endif	/* defined(CRAY) && !defined(__STDC__) */

    static struct setlist *
getset(name)
    char *name;
{
    return (struct setlist *)
		genget(name, (char **) Setlist, sizeof(struct setlist));
}

    void
set_escape_char(s)
    char *s;
{
	if (rlogin != _POSIX_VDISABLE) {
		rlogin = (s && *s) ? special(s) : _POSIX_VDISABLE;
		printf(MSGSTR(MSG_RLOGIN_ESCAPE,
				"Telnet rlogin escape character is '%s'.\n"),
					control(rlogin));
	} else {
		escape = (s && *s) ? special(s) : _POSIX_VDISABLE;
		printf(MSGSTR(MSG_TELNET_ESCAPE,
			"Telnet escape character is '%s'.\n"), control(escape));
	}
}

    static int
setcmd(argc, argv)
    int  argc;
    char *argv[];
{
    int value;
    struct setlist *ct;
    struct togglelist *c;

    if (argc < 2 || argc > 3) {
	printf(MSGSTR(MSG_SET_FORMAT,
		"Format is 'set Name Value'\n'set ?' for help.\n"));
	return 0;
    }
    if ((argc == 2) && (isprefix(argv[1], "?") || isprefix(argv[1], "help"))) {
	for (ct = Setlist; ct->name; ct++)
	    printf("%-15s %s\n", ct->name, MNSTR(ct->helpnum, ct->help));
	printf("\n");
	settogglehelp(1);
	printf("%-15s %s\n", "?", MSGSTR(MSG_DISPLAY_HELP_INFO,
					"display help information"));
	return 0;
    }

    ct = getset(argv[1]);
    if (ct == 0) {
	c = GETTOGGLE(argv[1]);
	if (c == 0) {
	    fprintf(stderr, MSGSTR(MSG_SET_UNKNOWN_ARG,
			"'%s': unknown argument ('set ?' for help).\n"),
			argv[1]);
	    return 0;
	} else if (Ambiguous(c)) {
	    fprintf(stderr, MSGSTR(MSG_AMBIGUOUS_SET_ARG,
			"'%s': ambiguous argument ('set ?' for help).\n"),
			argv[1]);
	    return 0;
	}
	if (c->variable) {
	    if ((argc == 2) || (strcmp("on", argv[2]) == 0))
		*c->variable = 1;
	    else if (strcmp("off", argv[2]) == 0)
		*c->variable = 0;
	    else {
		printf(MSGSTR(MSG_SET_TOGGLE_FORMAT,
		"Format is 'set togglename [on|off]'\n'set ?' for help.\n"));
		return 0;
	    }
	    if (c->will_actionexplanation && *c->variable)
		printf("%s.\n", MNSTR(c->willnum, c->will_actionexplanation));
	    else if (c->wont_actionexplanation && !(*c->variable))
		printf("%s.\n", MNSTR(c->wontnum, c->wont_actionexplanation));
	}
	if (c->handler)
	    (*c->handler)(1);
    } else if (argc != 3) {
	printf(MSGSTR(MSG_SET_FORMAT,
		"Format is 'set Name Value'\n'set ?' for help.\n"));
	return 0;
    } else if (Ambiguous(ct)) {
	fprintf(stderr, MSGSTR(MSG_AMBIGUOUS_SET_ARG,
			"'%s': ambiguous argument ('set ?' for help).\n"),
			argv[1]);
	return 0;
    } else if (ct->handler) {
	(*ct->handler)(argv[2]);
	printf(MSGSTR(MSG_SET_TO,
		"%s set to \"%s\".\n"), ct->name, (char *)ct->charp);
    } else {
	if (strcmp("off", argv[2])) {
	    value = special(argv[2]);
	} else {
	    value = _POSIX_VDISABLE;
	}
	*(ct->charp) = (cc_t)value;
	printf(MSGSTR(MSG_CHARACTER_IS,
		"%s character is '%s'.\n"), ct->name, control(*(ct->charp)));
    }
    slc_check();
    return 1;
}

    static int
unsetcmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct setlist *ct;
    struct togglelist *c;
    register char *name;

    if (argc < 2) {
	fprintf(stderr, MSGSTR(MSG_NEED_UNSET_ARG,
	    "Need an argument to 'unset' command.  'unset ?' for help.\n"));
	return 0;
    }
    if (isprefix(argv[1], "?") || isprefix(argv[1], "help")) {
	for (ct = Setlist; ct->name; ct++)
	    printf("%-15s %s\n", ct->name, MNSTR(ct->helpnum, ct->help));
	printf("\n");
	settogglehelp(0);
	printf("%-15s %s\n", "?", MSGSTR(MSG_DISPLAY_HELP_INFO,
					"display help information"));
	return 0;
    }

    argc--;
    argv++;
    while (argc--) {
	name = *argv++;
	ct = getset(name);
	if (ct == 0) {
	    c = GETTOGGLE(name);
	    if (c == 0) {
		fprintf(stderr, MSGSTR(MSG_UNKNOWN_UNSET_ARG,
			"'%s': unknown argument ('unset ?' for help).\n"),
			name);
		return 0;
	    } else if (Ambiguous(c)) {
		fprintf(stderr, MSGSTR(MSG_AMBIGUOUS_UNSET_ARG,
			"'%s': ambiguous argument ('unset ?' for help).\n"),
			name);
		return 0;
	    }
	    if (c->variable) {
		*c->variable = 0;
		if (c->wont_actionexplanation)
		    printf("%s.\n",
				MNSTR(c->wontnum, c->wont_actionexplanation));
	    }
	    if (c->handler)
		(*c->handler)(0);
	} else if (Ambiguous(ct)) {
	    fprintf(stderr, MSGSTR(MSG_AMBIGUOUS_UNSET_ARG,
			"'%s': ambiguous argument ('unset ?' for help).\n"),
			name);
	    return 0;
	} else if (ct->handler) {
	    (*ct->handler)(0);
	    printf(MSGSTR(MSG_RESET_TO,
			"%s reset to \"%s\".\n"), ct->name, (char *)ct->charp);
	} else {
	    *(ct->charp) = _POSIX_VDISABLE;
	    printf(MSGSTR(MSG_CHARACTER_IS,
		"%s character is '%s'.\n"), ct->name, control(*(ct->charp)));
	}
    }
    return 1;
}

/*
 * The following are the data structures and routines for the
 * 'mode' command.
 */
#ifdef	KLUDGELINEMODE
extern int kludgelinemode;

    static int
dokludgemode()
{
    kludgelinemode = 1;
    send_wont(TELOPT_LINEMODE, 1);
    send_dont(TELOPT_SGA, 1);
    send_dont(TELOPT_ECHO, 1);
}
#endif

    static int
dolinemode()
{
#ifdef	KLUDGELINEMODE
    if (kludgelinemode)
	send_dont(TELOPT_SGA, 1);
#endif
    send_will(TELOPT_LINEMODE, 1);
    send_dont(TELOPT_ECHO, 1);
    return 1;
}

    static int
docharmode()
{
#ifdef	KLUDGELINEMODE
    if (kludgelinemode)
	send_do(TELOPT_SGA, 1);
    else
#endif
    send_wont(TELOPT_LINEMODE, 1);
    send_do(TELOPT_ECHO, 1);
    return 1;
}

    static int
dolmmode(bit, on)
    int bit, on;
{
    unsigned char c;
    extern int linemode;

    if (my_want_state_is_wont(TELOPT_LINEMODE)) {
	printf(MSGSTR(MSG_ENABLE_LINEMODE,
		"?Need to have LINEMODE option enabled first.\n"));
	printf(MSGSTR(MSG_MODE_HELP, "'mode ?' for help.\n"));
	return 0;
    }

    if (on)
	c = (linemode | bit);
    else
	c = (linemode & ~bit);
    lm_mode(&c, 1, 1);
    return 1;
}

    int
setmode(bit)
{
    return dolmmode(bit, 1);
}

    int
clearmode(bit)
{
    return dolmmode(bit, 0);
}

struct modelist {
	char	*name;		/* command name */
#ifdef	INTL
    int		helpnum;	/* Message number of help message */
#endif
	char	*help;		/* help string */
	int	(*handler)();	/* routine which executes command */
	int	needconnect;	/* Do we need to be connected to execute? */
	int	arg1;
};

extern int modehelp();

static struct modelist ModeList[] = {
    { "character",
	MN(MSG_MODEL0, "Disable LINEMODE option"),
	    docharmode, 1 },
#ifdef	KLUDGELINEMODE
    { "",
	MN(MSG_MODEL1, "(or disable obsolete line-by-line mode)"),
	    0 },
#endif
    { "line",
	MN(MSG_MODEL2, "Enable LINEMODE option"),
	    dolinemode, 1 },
#ifdef	KLUDGELINEMODE
    { "",
	MN(MSG_MODEL3, "(or enable obsolete line-by-line mode)"),
	    0 },
#endif
    { "", MN(0, ""), 0 },
    { "",
	MN(MSG_MODEL4, "These require the LINEMODE option to be enabled"),
	    0 },
    { "isig",
	MN(MSG_MODEL5, "Enable signal trapping"),
	    setmode, 1, MODE_TRAPSIG },
    { "+isig", MN(0, 0),			setmode, 1, MODE_TRAPSIG },
    { "-isig",
	MN(MSG_MODEL6, "Disable signal trapping"),
	    clearmode, 1, MODE_TRAPSIG },
    { "edit",
	MN(MSG_MODEL7, "Enable character editing"),
	    setmode, 1, MODE_EDIT },
    { "+edit", MN(0, 0),			setmode, 1, MODE_EDIT },
    { "-edit",
	MN(MSG_MODEL8, "Disable character editing"),
	    clearmode, 1, MODE_EDIT },
    { "softtabs",
	MN(MSG_MODEL9, "Enable tab expansion"),
	    setmode, 1, MODE_SOFT_TAB },
    { "+softtabs", MN(0, 0),			setmode, 1, MODE_SOFT_TAB },
    { "-softtabs",
	MN(MSG_MODEL10, "Disable tab expansion"),
	    clearmode, 1, MODE_SOFT_TAB },
    { "litecho",
	MN(MSG_MODEL11, "Enable literal character echo"),
	    setmode, 1, MODE_LIT_ECHO },
    { "+litecho", MN(0, 0),			setmode, 1, MODE_LIT_ECHO },
    { "-litecho",
	MN(MSG_MODEL12, "Disable literal character echo"),
	    clearmode, 1, MODE_LIT_ECHO },
    { "help", MN(0, 0),				modehelp, 0 },
#ifdef	KLUDGELINEMODE
    { "kludgeline", MN(0, 0),			dokludgemode, 1 },
#endif
    { "", MN(0, ""), 0 },
    { "?",
	MN(MSG_HELP_INFO, "Print help information"),
	    modehelp, 0 },
    { 0 },
};


    int
modehelp()
{
    struct modelist *mt;

    printf(MSGSTR(MSG_MODE_FORMAT,
		"format is:  'mode Mode', where 'Mode' is one of:\n\n"));
    for (mt = ModeList; mt->name; mt++) {
	if (mt->help) {
	    if (*mt->help)
		printf("%-15s %s\n", mt->name, MNSTR(mt->helpnum, mt->help));
	    else
		printf("\n");
	}
    }
    return 0;
}

#define	GETMODECMD(name) (struct modelist *) \
		genget(name, (char **) ModeList, sizeof(struct modelist))

    static int
modecmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct modelist *mt;

    if (argc != 2) {
	printf(MSGSTR(MSG_MODE_ARG_REQUIRED,
			"'mode' command requires an argument\n"));
	printf(MSGSTR(MSG_MODE_HELP, "'mode ?' for help.\n"));
    } else if ((mt = GETMODECMD(argv[1])) == 0) {
	fprintf(stderr, MSGSTR(MSG_UNKNOWN_MODE,
			"Unknown mode '%s' ('mode ?' for help).\n"), argv[1]);
    } else if (Ambiguous(mt)) {
	fprintf(stderr, MSGSTR(MSG_AMBIGUOUS_MODE,
			"Ambiguous mode '%s' ('mode ?' for help).\n"), argv[1]);
    } else if (mt->needconnect && !connected) {
	printf(MSGSTR(MSG_CONNECT_FIRST, "?Need to be connected first.\n"));
	printf(MSGSTR(MSG_MODE_HELP, "'mode ?' for help.\n"));
    } else if (mt->handler) {
	return (*mt->handler)(mt->arg1);
    }
    return 0;
}

/*
 * The following data structures and routines implement the
 * "display" command.
 */

    static int
display(argc, argv)
    int  argc;
    char *argv[];
{
    struct togglelist *tl;
    struct setlist *sl;

#define	dotog(tl)	if (tl->variable) { \
			    if (*tl->variable && tl->will_actionexplanation) { \
				printf("%s.\n", MNSTR(tl->willnum, tl->will_actionexplanation)); \
			    } else if (!(*tl->variable) && tl->wont_actionexplanation) { \
				printf("%s.\n", MNSTR(tl->wontnum, tl->wont_actionexplanation)); \
			    } \
			}

#define	doset(sl)   if (sl->name && *sl->name != ' ') { \
			if (sl->handler == 0) \
			    printf("%-15s [%s]\n", sl->name, control(*sl->charp)); \
			else \
			    printf("%-15s \"%s\"\n", sl->name, (char *)sl->charp); \
		    }

    if (argc == 1) {
	for (tl = Togglelist; tl->name; tl++) {
	    dotog(tl);
	}
	printf("\n");
	for (sl = Setlist; sl->name; sl++) {
	    doset(sl);
	}
    } else {
	int i;

	for (i = 1; i < argc; i++) {
	    sl = getset(argv[i]);
	    tl = GETTOGGLE(argv[i]);
	    if (Ambiguous(sl) || Ambiguous(tl)) {
		printf(MSGSTR(MSG_AMBIGUOUS_DISPLAY_ARG,
				"?Ambiguous argument '%s'.\n"), argv[i]);
		return 0;
	    } else if (!sl && !tl) {
		printf(MSGSTR(MSG_UNKNOWN_DISPLAY_ARG,
				"?Unknown argument '%s'.\n"), argv[i]);
		return 0;
	    } else {
		if (tl) {
		    dotog(tl);
		}
		if (sl) {
		    doset(sl);
		}
	    }
	}
    }
/*@*/optionstatus();
#ifdef	ENCRYPTION
    EncryptStatus();
#endif	/* ENCRYPTION */
    return 1;
#undef	doset
#undef	dotog
}

/*
 * The following are the data structures, and many of the routines,
 * relating to command processing.
 */

/*
 * Set the escape character.
 */
	static int
setescape(argc, argv)
	int argc;
	char *argv[];
{
	register char *arg;
	char buf[50];

	printf(MSGSTR(MSG_ESCAPE_DEPRECATED,
	    "Deprecated usage - please use 'set escape%s%s' in the future.\n"),
				(argc > 2)? " ":"", (argc > 2)? argv[1]: "");
	if (argc > 2)
		arg = argv[1];
	else {
		printf(MSGSTR(MSG_NEW_ESCAPE_CHAR, "new escape character: "));
		(void) fgets(buf, sizeof(buf), stdin);
		arg = buf;
	}
	if (arg[0] != '\0')
		escape = arg[0];
	if (!In3270) {
		printf(MSGSTR(MSG_ESCAPE_CHAR_IS,
		"Escape character is '%s'.\n"), control(escape));
	}
	(void) fflush(stdout);
	return 1;
}

    /*VARARGS*/
    static int
togcrmod()
{
    crmod = !crmod;
    printf(MSGSTR(MSG_CRMOD_DEPRECATED,
    	"Deprecated usage - please use 'toggle crmod' in the future.\n"));
    if (crmod)
	printf(MSGSTR(MSG_CRMOD_WILL_MAP,
		"Will map carriage return on output.\n"));
    else
	printf(MSGSTR(MSG_CRMOD_WONT_MAP,
		"Won't map carriage return on output.\n"));
    (void) fflush(stdout);
    return 1;
}

    /*VARARGS*/
    int
suspend()
{
#ifdef	SIGTSTP
    setcommandmode();
    {
	long oldrows, oldcols, newrows, newcols, err;

	err = (TerminalWindowSize(&oldrows, &oldcols) == 0) ? 1 : 0;
	(void) kill(0, SIGTSTP);
	/*
	 * If we didn't get the window size before the SUSPEND, but we
	 * can get them now (?), then send the NAWS to make sure that
	 * we are set up for the right window size.
	 */
	if (TerminalWindowSize(&newrows, &newcols) && connected &&
	    (err || ((oldrows != newrows) || (oldcols != newcols)))) {
		sendnaws();
	}
    }
    /* reget parameters in case they were changed */
    TerminalSaveState();
    setconnmode(0);
#else
    printf(MSGSTR(MSG_NO_SUSPEND,
		"Suspend is not supported.  Try the '!' command instead\n"));
#endif
    return 1;
}

#if	!defined(TN3270)

#ifdef	UW
#include <priv.h>
#include <sys/secsys.h>

/*
 * Must use fork() rather than vfork() in shell(), otherwise when the child
 * process clears its privileges it also clears those of the parent.
 */
#define vfork	fork

/*
 * If we are not the administrator, clear all privileges.
 */
void
clear_privs_non_admin()
{
    uid_t _loc_id_priv;

    if (((_loc_id_priv = secsys(ES_PRVID, 0)) < 0) ||
	(geteuid() != _loc_id_priv)) {
	    procprivl(CLRPRV, pm_max(P_ALLPRIVS), (priv_t)0);
    }
}
#endif	/* UW */

    /*ARGSUSED*/
    int
shell(argc, argv)
    int argc;
    char *argv[];
{
    long oldrows, oldcols, newrows, newcols, err;

    setcommandmode();

    err = (TerminalWindowSize(&oldrows, &oldcols) == 0) ? 1 : 0;
    switch(vfork()) {
    case -1:
	perror(MSGSTR(MSG_FORK_FAILED, "Fork failed\n"));
	break;

    case 0:
	{
	    /*
	     * Fire up the shell in the child.
	     */
	    register char *shellp, *shellname;
	    extern char *strrchr();

	    shellp = getenv("SHELL");
	    if (shellp == NULL)
		shellp = "/bin/sh";
	    if ((shellname = strrchr(shellp, '/')) == 0)
		shellname = shellp;
	    else
		shellname++;
#ifdef	UW
	    /*
	     * Prevent a user from gaining the inheritable privileges
	     * associated with the telnet binary.
	     */
	    clear_privs_non_admin();
#endif	/* UW */
	    if (argc > 1)
		execl(shellp, shellname, "-c", &saveline[1], 0);
	    else
		execl(shellp, shellname, 0);
	    perror("Execl");
	    _exit(1);
	}
    default:
	    (void)wait((int *)0);	/* Wait for the shell to complete */

	    if (TerminalWindowSize(&newrows, &newcols) && connected &&
		(err || ((oldrows != newrows) || (oldcols != newcols)))) {
		    sendnaws();
	    }
	    break;
    }
    return 1;
}
#else	/* !defined(TN3270) */
extern int shell();
#endif	/* !defined(TN3270) */

    /*VARARGS*/
    static
bye(argc, argv)
    int  argc;		/* Number of arguments */
    char *argv[];	/* arguments */
{
    extern int resettermname;

    if (connected) {
	(void) shutdown(net, 2);
	printf(MSGSTR(MSG_CONNECTION_CLOSED, "Connection closed.\n"));
	(void) NetClose(net);
	connected = 0;
	resettermname = 1;
#if	defined(AUTHENTICATION) || defined(ENCRYPTION)
	auth_encrypt_connect(connected);
#endif	/* defined(AUTHENTICATION) || defined(ENCRYPTION) */
	/* reset options */
	tninit();
#if	defined(TN3270)
	SetIn3270();		/* Get out of 3270 mode */
#endif	/* defined(TN3270) */
    }
    if ((argc != 2) || (strcmp(argv[1], "fromquit") != 0)) {
	longjmp(toplevel, 1);
	/* NOTREACHED */
    }
    return 1;			/* Keep lint, etc., happy */
}

/*VARARGS*/
quit()
{
	(void) call(bye, "bye", "fromquit", 0);
	Exit(0);
	/*NOTREACHED*/
}

/*VARARGS*/
	int
logout()
{
	send_do(TELOPT_LOGOUT, 1);
	(void) netflush();
	return 1;
}


/*
 * The SLC command.
 */

struct slclist {
	char	*name;
#ifdef	INTL
	int	helpnum;
#endif
	char	*help;
	void	(*handler)();
	int	arg;
};

static void slc_help();

struct slclist SlcList[] = {
    { "export", MN(MSG_SLCL0, "Use local special character definitions"),
						slc_mode_export,	0 },
    { "import", MN(MSG_SNCL1, "Use remote special character definitions"),
						slc_mode_import,	1 },
    { "check",	MN(MSG_SNCL2, "Verify remote special character definitions"),
						slc_mode_import,	0 },
    { "help",	MN(0, 0),			slc_help,		0 },
    { "?",	MN(MSG_HELP_INFO, "Print help information"),
						slc_help,		0 },
    { 0 },
};

    static void
slc_help()
{
    struct slclist *c;

    for (c = SlcList; c->name; c++) {
	if (c->help) {
	    if (*c->help)
		printf("%-15s %s\n", c->name, MNSTR(c->helpnum, c->help));
	    else
		printf("\n");
	}
    }
}

    static struct slclist *
getslc(name)
    char *name;
{
    return (struct slclist *)
		genget(name, (char **) SlcList, sizeof(struct slclist));
}

    static
slccmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct slclist *c;

    if (argc != 2) {
	fprintf(stderr, MSGSTR(MSG_NEED_SLC_ARG,
	    "Need an argument to 'slc' command.  'slc ?' for help.\n"));
	return 0;
    }
    c = getslc(argv[1]);
    if (c == 0) {
	fprintf(stderr, MSGSTR(MSG_UNKNOWN_SLC_ARG,
		"'%s': unknown argument ('slc ?' for help).\n"),
    				argv[1]);
	return 0;
    }
    if (Ambiguous(c)) {
	fprintf(stderr, MSGSTR(MSG_AMBIGUOUS_SLC_ARG,
		"'%s': ambiguous argument ('slc ?' for help).\n"),
    				argv[1]);
	return 0;
    }
    (*c->handler)(c->arg);
    slcstate();
    return 1;
}

/*
 * The ENVIRON command.
 */

struct envlist {
	char	*name;
#ifdef	INTL
	int	helpnum;
#endif
	char	*help;
	void	(*handler)();
	int	narg;
};

extern struct env_lst *
	env_define P((unsigned char *, unsigned char *));
extern void
	env_undefine P((unsigned char *)),
	env_export P((unsigned char *)),
	env_unexport P((unsigned char *)),
	env_send P((unsigned char *)),
#if defined(OLD_ENVIRON) && defined(ENV_HACK)
	env_varval P((unsigned char *)),
#endif
	env_list P((void));
static void
	env_help P((void));

struct envlist EnvList[] = {
    { "define",
	MN(MSG_ENVL0, "Define an environment variable"),
						(void (*)())env_define,	2 },
    { "undefine",
	MN(MSG_ENVL1, "Undefine an environment variable"),
						env_undefine,	1 },
    { "export",
	MN(MSG_ENVL2, "Mark an environment variable for automatic export"),
						env_export,	1 },
    { "unexport",
       MN(MSG_ENVL3, "Don't mark an environment variable for automatic export"),
						env_unexport,	1 },
    { "send",
	MN(MSG_ENVL4, "Send an environment variable"),
						env_send,	1 },
    { "list",
	MN(MSG_ENVL5, "List the current environment variables"),
						env_list,	0 },
#if defined(OLD_ENVIRON) && defined(ENV_HACK)
    { "varval",
	MN(MSG_ENVL6, "Reverse VAR and VALUE (auto, right, wrong, status)"),
						env_varval,    1 },
#endif
    { "help", MN(0, 0),				env_help,		0 },
    { "?",
	MN(MSG_HELP_INFO, "Print help information"),
						env_help,		0 },
    { 0 },
};

    static void
env_help()
{
    struct envlist *c;

    for (c = EnvList; c->name; c++) {
	if (c->help) {
	    if (*c->help)
		printf("%-15s %s\n", c->name, MNSTR(c->helpnum, c->help));
	    else
		printf("\n");
	}
    }
}

    static struct envlist *
getenvcmd(name)
    char *name;
{
    return (struct envlist *)
		genget(name, (char **) EnvList, sizeof(struct envlist));
}

env_cmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct envlist *c;

    if (argc < 2) {
	fprintf(stderr, MSGSTR(MSG_NEED_ENV_ARG,
	    "Need an argument to 'environ' command.  'environ ?' for help.\n"));
	return 0;
    }
    c = getenvcmd(argv[1]);
    if (c == 0) {
	fprintf(stderr, MSGSTR(MSG_UNKNOWN_ENV_ARG,
		"'%s': unknown argument ('environ ?' for help).\n"),
    				argv[1]);
	return 0;
    }
    if (Ambiguous(c)) {
	fprintf(stderr, MSGSTR(MSG_AMBIGUOUS_ENV_ARG,
		"'%s': ambiguous argument ('environ ?' for help).\n"),
    				argv[1]);
	return 0;
    }
    if (c->narg + 2 != argc) {
	if (c->narg + 2 < argc) {
	    if (c->narg == 1)
		fprintf(stderr, MSGSTR(MSG_ENV_ONLY_ONE_ARG,
"Need only 1 argument to 'environ %s' command.  'environ ?' for help.\n"),
			c->name);
	    else
		fprintf(stderr, MSGSTR(MSG_ENV_ONLY_N_ARGS,
"Need only %d arguments to 'environ %s' command.  'environ ?' for help.\n"),
			c->narg, c->name);
	} else {
	    if (c->narg == 1)
		fprintf(stderr, MSGSTR(MSG_ENV_ONE_ARG,
"Need 1 argument to 'environ %s' command.  'environ ?' for help.\n"),
			c->name);
	    else
		fprintf(stderr, MSGSTR(MSG_ENV_N_ARGS,
"Need %d arguments to 'environ %s' command.  'environ ?' for help.\n"),
			c->narg, c->name);
	}
	return 0;
    }
    (*c->handler)(argv[2], argv[3]);
    return 1;
}

struct env_lst {
	struct env_lst *next;	/* pointer to next structure */
	struct env_lst *prev;	/* pointer to previous structure */
	unsigned char *var;	/* pointer to variable name */
	unsigned char *value;	/* pointer to variable value */
	int export;		/* 1 -> export with default list of variables */
	int welldefined;	/* A well defined variable */
};

struct env_lst envlisthead;

	struct env_lst *
env_find(var)
	unsigned char *var;
{
	register struct env_lst *ep;

	for (ep = envlisthead.next; ep; ep = ep->next) {
		if (strcmp((char *)ep->var, (char *)var) == 0)
			return(ep);
	}
	return(NULL);
}

	void
env_init()
{
	extern char **environ;
	register char **epp, *cp;
	register struct env_lst *ep;
	extern char *strchr();

	for (epp = environ; *epp; epp++) {
		if (cp = strchr(*epp, '=')) {
			*cp = '\0';
			ep = env_define((unsigned char *)*epp,
					(unsigned char *)cp+1);
			ep->export = 0;
			*cp = '=';
		}
	}
	/*
	 * Special case for DISPLAY variable.  If it is ":0.0" or
	 * "unix:0.0", we have to get rid of "unix" and insert our
	 * hostname.
	 */
	if ((ep = env_find("DISPLAY"))
	    && ((*ep->value == ':')
		|| (strncmp((char *)ep->value, "unix:", 5) == 0))) {
		char hbuf[256+1];
		char *cp2 = strchr((char *)ep->value, ':');

		gethostname(hbuf, 256);
		hbuf[256] = '\0';
		cp = (char *)malloc(strlen(hbuf) + strlen(cp2) + 1);
		sprintf((char *)cp, "%s%s", hbuf, cp2);
		free(ep->value);
		ep->value = (unsigned char *)cp;
	}
	/*
	 * If USER is not defined, but LOGNAME is, then add
	 * USER with the value from LOGNAME.  By default, we
	 * don't export the USER variable.
	 */
	if ((env_find("USER") == NULL) && (ep = env_find("LOGNAME"))) {
		env_define((unsigned char *)"USER", ep->value);
		env_unexport((unsigned char *)"USER");
	}
	env_export((unsigned char *)"DISPLAY");
	env_export((unsigned char *)"PRINTER");
#ifdef	INTL
	env_export((unsigned char *)"LANG");
#endif
}

	struct env_lst *
env_define(var, value)
	unsigned char *var, *value;
{
	register struct env_lst *ep;

	if (ep = env_find(var)) {
		if (ep->var)
			free(ep->var);
		if (ep->value)
			free(ep->value);
	} else {
		ep = (struct env_lst *)malloc(sizeof(struct env_lst));
		ep->next = envlisthead.next;
		envlisthead.next = ep;
		ep->prev = &envlisthead;
		if (ep->next)
			ep->next->prev = ep;
	}
	ep->welldefined = opt_welldefined(var);
	ep->export = 1;
	ep->var = (unsigned char *)strdup((char *)var);
	ep->value = (unsigned char *)strdup((char *)value);
	return(ep);
}

	void
env_undefine(var)
	unsigned char *var;
{
	register struct env_lst *ep;

	if (ep = env_find(var)) {
		ep->prev->next = ep->next;
		if (ep->next)
			ep->next->prev = ep->prev;
		if (ep->var)
			free(ep->var);
		if (ep->value)
			free(ep->value);
		free(ep);
	}
}

	void
env_export(var)
	unsigned char *var;
{
	register struct env_lst *ep;

	if (ep = env_find(var))
		ep->export = 1;
}

	void
env_unexport(var)
	unsigned char *var;
{
	register struct env_lst *ep;

	if (ep = env_find(var))
		ep->export = 0;
}

	void
env_send(var)
	unsigned char *var;
{
	register struct env_lst *ep;

	if (my_state_is_wont(TELOPT_NEW_ENVIRON)
#ifdef	OLD_ENVIRON
	    && my_state_is_wont(TELOPT_OLD_ENVIRON)
#endif
		) {
		fprintf(stderr, MSGSTR(MSG_CANT_SEND_ENV_OPT,
		    "Cannot send '%s': Telnet ENVIRON option not enabled\n"),
									var);
		return;
	}
	ep = env_find(var);
	if (ep == 0) {
		fprintf(stderr, MSGSTR(MSG_CANT_SEND_VAR,
			"Cannot send '%s': variable not defined\n"),
									var);
		return;
	}
	env_opt_start_info();
	env_opt_add(ep->var);
	env_opt_end(0);
}

	void
env_list()
{
	register struct env_lst *ep;

	for (ep = envlisthead.next; ep; ep = ep->next) {
		printf("%c %-20s %s\n", ep->export ? '*' : ' ',
					ep->var, ep->value);
	}
}

	unsigned char *
env_default(init, welldefined)
	int init;
{
	static struct env_lst *nep = NULL;

	if (init) {
		nep = &envlisthead;
		return;
	}
	if (nep) {
		while (nep = nep->next) {
			if (nep->export && (nep->welldefined == welldefined))
				return(nep->var);
		}
	}
	return(NULL);
}

	unsigned char *
env_getvalue(var)
	unsigned char *var;
{
	register struct env_lst *ep;

	if (ep = env_find(var))
		return(ep->value);
	return(NULL);
}

#if defined(OLD_ENVIRON) && defined(ENV_HACK)
	void
env_varval(what)
	unsigned char *what;
{
	extern int old_env_var, old_env_value, env_auto;
	int len = strlen((char *)what);

	if (len == 0)
		goto unknown;

	if (strncasecmp((char *)what, "status", len) == 0) {
		if (env_auto)
			printf(MSGSTR(MSG_VAR_VAL_AUTO,
		"VAR and VALUE are/will be determined automatically\n"));
		if (old_env_var == OLD_ENV_VAR)
			printf(MSGSTR(MSG_VAR_CORRECT,
				"VAR and VALUE set to correct definitions\n"));
		else
			printf(MSGSTR(MSG_VAR_REVERSE,
				"VAR and VALUE definitions are reversed\n"));
	} else if (strncasecmp((char *)what, "auto", len) == 0) {
		env_auto = 1;
		old_env_var = OLD_ENV_VALUE;
		old_env_value = OLD_ENV_VAR;
	} else if (strncasecmp((char *)what, "right", len) == 0) {
		env_auto = 0;
		old_env_var = OLD_ENV_VAR;
		old_env_value = OLD_ENV_VALUE;
	} else if (strncasecmp((char *)what, "wrong", len) == 0) {
		env_auto = 0;
		old_env_var = OLD_ENV_VALUE;
		old_env_value = OLD_ENV_VAR;
	} else {
unknown:
		printf(MSGSTR(MSG_VAR_UNKNOWN,
"Unknown \"varval\" command. (\"auto\", \"right\", \"wrong\", \"status\")\n"));
	}
}
#endif

#if	defined(AUTHENTICATION)
/*
 * The AUTHENTICATE command.
 */

struct authlist {
	char	*name;
#ifdef	INTL
	int	helpnum;
#endif
	char	*help;
	int	(*handler)();
	int	narg;
};

extern int
	auth_enable P((char *)),
	auth_disable P((char *)),
	auth_status P((void));
static int
	auth_help P((void));

struct authlist AuthList[] = {
    { "status",
	MN(MSG_AUTHL0, "Display current status of authentication information"),
						auth_status,	0 },
    { "disable",
	MN(MSG_AUTHL1, "Disable an authentication type ('auth disable ?' for more)"),
						auth_disable,	1 },
    { "enable",
	MN(MSG_AUTHL2, "Enable an authentication type ('auth enable ?' for more)"),
						auth_enable,	1 },
    { "help", MN(0, 0),				auth_help,	0 },
    { "?",
	MN(MSG_HELP_INFO, "Print help information"),
						auth_help,	0 },
    { 0 },
};

    static int
auth_help()
{
    struct authlist *c;

    for (c = AuthList; c->name; c++) {
	if (c->help) {
	    if (*c->help)
		printf("%-15s %s\n", c->name, MNSTR(c->helpnum, c->help));
	    else
		printf("\n");
	}
    }
    return 0;
}

auth_cmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct authlist *c;

    if (argc < 2) {
	fprintf(stderr, MSGSTR(MSG_NEED_AUTH_ARG,
	    "Need an argument to 'auth' command.  'auth ?' for help.\n"));
	return 0;
    }

    c = (struct authlist *)
		genget(argv[1], (char **) AuthList, sizeof(struct authlist));
    if (c == 0) {
	fprintf(stderr, MSGSTR(MSG_UNKNOWN_AUTH_ARG,
			"'%s': unknown argument ('auth ?' for help).\n"),
    				argv[1]);
	return 0;
    }
    if (Ambiguous(c)) {
	fprintf(stderr, MSGSTR(MSG_AMBIGUOUS_AUTH_ARG,
			"'%s': ambiguous argument ('auth ?' for help).\n"),
    				argv[1]);
	return 0;
    }
    if (c->narg + 2 != argc) {
	if (c->narg + 2 < argc) {
	    if (c->narg == 1)
		fprintf(stderr, MSGSTR(MSG_AUTH_ONLY_ONE_ARG,
	"Need only 1 argument to 'auth %s' command.  'auth ?' for help.\n"),
			c->name);
	    else
		fprintf(stderr, MSGSTR(MSG_AUTH_ONLY_N_ARGS,
	"Need only %d arguments to 'auth %s' command.  'auth ?' for help.\n"),
			c->narg, c->name);
	} else {
	    if (c->narg == 1)
		fprintf(stderr, MSGSTR(MSG_AUTH_ONE_ARG,
	"Need 1 argument to 'auth %s' command.  'auth ?' for help.\n"),
			c->name);
	    else
		fprintf(stderr, MSGSTR(MSG_AUTH_N_ARGS,
	"Need %d arguments to 'auth %s' command.  'auth ?' for help.\n"),
			c->narg, c->name);
	}
	return 0;
    }
    return((*c->handler)(argv[2], argv[3]));
}
#endif

#ifdef	ENCRYPTION
/*
 * The ENCRYPT command.
 */

struct encryptlist {
	char	*name;
#ifdef	INTL
	int	helpnum;
#endif
	char	*help;
	int	(*handler)();
	int	needconnect;
	int	minarg;
	int	maxarg;
};

extern int
	EncryptEnable P((char *, char *)),
	EncryptDisable P((char *, char *)),
	EncryptType P((char *, char *)),
	EncryptStart P((char *)),
	EncryptStartInput P((void)),
	EncryptStartOutput P((void)),
	EncryptStop P((char *)),
	EncryptStopInput P((void)),
	EncryptStopOutput P((void)),
	EncryptStatus P((void));
static int
	EncryptHelp P((void));

struct encryptlist EncryptList[] = {
    { "enable",
	MN(MSG_ENCRL0, "Enable encryption. ('encrypt enable ?' for more)"),
						EncryptEnable, 1, 1, 2 },
    { "disable",
	MN(MSG_ENCRL1, "Disable encryption. ('encrypt enable ?' for more)"),
						EncryptDisable, 0, 1, 2 },
    { "type",
	MN(MSG_ENCRL2, "Set encryption type. ('encrypt type ?' for more)"),
						EncryptType, 0, 1, 1 },
    { "start",
	MN(MSG_ENCRL3, "Start encryption. ('encrypt start ?' for more)"),
						EncryptStart, 1, 0, 1 },
    { "stop",
	MN(MSG_ENCRL4, "Stop encryption. ('encrypt stop ?' for more)"),
						EncryptStop, 1, 0, 1 },
    { "input",
	MN(MSG_ENCRL5, "Start encrypting the input stream"),
						EncryptStartInput, 1, 0, 0 },
    { "-input",
	MN(MSG_ENCRL6, "Stop encrypting the input stream"),
						EncryptStopInput, 1, 0, 0 },
    { "output",
	MN(MSG_ENCRL7, "Start encrypting the output stream"),
						EncryptStartOutput, 1, 0, 0 },
    { "-output",
	MN(MSG_ENCRL8, "Stop encrypting the output stream"),
						EncryptStopOutput, 1, 0, 0 },

    { "status",
	MN(MSG_ENCRL9, "Display current status of encryption information"),
						EncryptStatus,	0, 0, 0 },
    { "help", MN(0, 0),				EncryptHelp,	0, 0, 0 },
    { "?",
	MN(MSG_HELP_INFO, "Print help information"),
						EncryptHelp,	0, 0, 0 },
    { 0 },
};

    static int
EncryptHelp()
{
    struct encryptlist *c;

    for (c = EncryptList; c->name; c++) {
	if (c->help) {
	    if (*c->help)
		printf("%-15s %s\n", c->name, MNSTR(c->helpnum, c->help));
	    else
		printf("\n");
	}
    }
    return 0;
}

encrypt_cmd(argc, argv)
    int  argc;
    char *argv[];
{
    struct encryptlist *c;

    if (argc < 2) {
	fprintf(stderr, MSGSTR(MSG_NEED_ENCR_ARG,
	    "Need an argument to 'encrypt' command.  'encrypt ?' for help.\n"));
	return 0;
    }

    c = (struct encryptlist *)
		genget(argv[1], (char **) EncryptList, sizeof(struct encryptlist));
    if (c == 0) {
	fprintf(stderr, MSGSTR(MSG_UNKNOWN_ENCR_ARG,
			"'%s': unknown argument ('encrypt ?' for help).\n"),
    				argv[1]);
	return 0;
    }
    if (Ambiguous(c)) {
	fprintf(stderr, MSGSTR(MSG_AMBIGUOUS_ENCR_ARG,
			"'%s': ambiguous argument ('encrypt ?' for help).\n"),
    				argv[1]);
	return 0;
    }
    argc -= 2;
    if (argc < c->minarg || argc > c->maxarg) {
	if (c->minarg == c->maxarg) {
	    if (c->minarg < argc) {
		if (c->minarg == 1)
		    fprintf(stderr, MSGSTR(MSG_ENCR_ONLY_ONE_ARG,
"Need only 1 argument to 'encrypt %s' command.  'encrypt ?' for help.\n"),
				c->name);
		else
		    fprintf(stderr, MSGSTR(MSG_ENCR_ONLY_N_ARGS,
"Need only %d arguments to 'encrypt %s' command.  'encrypt ?' for help.\n"),
				c->minarg, c->name);
	    } else {
		if (c->minarg == 1)
		    fprintf(stderr, MSGSTR(MSG_ENCR_ONE_ARG,
"Need 1 argument to 'encrypt %s' command.  'encrypt ?' for help.\n"),
				c->name);
		else
		    fprintf(stderr, MSGSTR(MSG_ENCR_N_ARGS,
"Need %d arguments to 'encrypt %s' command.  'encrypt ?' for help.\n"),
				c->minarg, c->name);
	    }
	} else {
	    if (c->maxarg < argc) {
		fprintf(stderr, MSGSTR(MSG_ENCR_ONLY_NN_ARGS,
"Need only %d-%d arguments to 'encrypt %s' command.  'encrypt ?' for help.\n"),
		c->minarg, c->maxarg, c->name);
	    } else {
		fprintf(stderr, MSGSTR(MSG_ENCR_NN_ARGS,
"Need %d-%d arguments to 'encrypt %s' command.  'encrypt ?' for help.\n"),
		c->minarg, c->maxarg, c->name);
	    }
	}
	return 0;
    }
    if (c->needconnect && !connected) {
	if (!(argc && (isprefix(argv[2], "help") || isprefix(argv[2], "?")))) {
	    printf(MSGSTR(MSG_CONNECT_FIRST, "?Need to be connected first.\n"));
	    return 0;
	}
    }
    return ((*c->handler)(argc > 0 ? argv[2] : 0,
			argc > 1 ? argv[3] : 0,
			argc > 2 ? argv[4] : 0));
}
#endif	/* ENCRYPTION */

#if	defined(unix) && defined(TN3270)
    static void
filestuff(fd)
    int fd;
{
    int res;

#ifdef	F_GETOWN
    setconnmode(0);
    res = fcntl(fd, F_GETOWN, 0);
    setcommandmode();

    if (res == -1) {
	perror("fcntl");
	return;
    }
    printf(MSGSTR(MSG_OWNER_IS,"\tOwner is %d.\n"), res);
#endif

    setconnmode(0);
    res = fcntl(fd, F_GETFL, 0);
    setcommandmode();

    if (res == -1) {
	perror("fcntl");
	return;
    }
#ifdef notdef
    printf("\tFlags are 0x%x: %s\n", res, decodeflags(res));
#endif
}
#endif /* defined(unix) && defined(TN3270) */

/*
 * Print status about the connection.
 */
    /*ARGSUSED*/
    static
status(argc, argv)
    int	 argc;
    char *argv[];
{
    if (connected) {
	printf(MSGSTR(MSG_CONNECTED_TO, "Connected to %s.\n"), hostname);
	if ((argc < 2) || strcmp(argv[1], "notmuch")) {
	    int mode = getconnmode();

	    if (my_want_state_is_will(TELOPT_LINEMODE)) {
		printf(MSGSTR(MSG_OPERATING_LINEMODE,
			"Operating with LINEMODE option\n"));
		if (mode&MODE_EDIT)
		    printf(MSGSTR(MSG_LOCAL_LINE_EDIT, "Local line editing\n"));
		else
		    printf(MSGSTR(MSG_NO_LINE_EDIT, "No line editing\n"));

		if (mode&MODE_TRAPSIG)
		    printf(MSGSTR(MSG_LOCAL_SIGNALS,
			"Local catching of signals\n"));
		else
		    printf(MSGSTR(MSG_NO_SIGNALS, "No catching of signals\n"));

		slcstate();
#ifdef	KLUDGELINEMODE
	    } else if (kludgelinemode && my_want_state_is_dont(TELOPT_SGA)) {
		printf(MSGSTR(MSG_OBSOLETE_LINEMODE,
			"Operating in obsolete linemode\n"));
#endif
	    } else {
		printf(MSGSTR(MSG_SINGLE_CHAR_MODE, 
			"Operating in single character mode\n"));
		if (localchars)
		    printf(MSGSTR(MSG_SIGNALS_LOCALLY,
			"Catching signals locally\n"));
	    }
	    if (mode&MODE_ECHO)
		printf(MSGSTR(MSG_LOCAL_ECHO, "Local character echo\n"));
	    else
		printf(MSGSTR(MSG_REMOTE_ECHO, "Remote character echo\n"));

	    if (my_want_state_is_will(TELOPT_LFLOW))
		if (mode&MODE_FLOW)
		    printf(MSGSTR(MSG_LOCAL_FLOW, "Local flow control\n"));
		else
		    printf(MSGSTR(MSG_NO_FLOW, "No flow control\n"));
#ifdef	ENCRYPTION
	    encrypt_display();
#endif	/* ENCRYPTION */
	}
    } else {
	printf(MSGSTR(MSG_NO_CONNECTION, "No connection.\n"));
    }
#   if !defined(TN3270)
    printf(MSGSTR(MSG_ESCAPE_CHAR_IS,
	"Escape character is '%s'.\n"), control(escape));
    (void) fflush(stdout);
#   else /* !defined(TN3270) */
    if ((!In3270) && ((argc < 2) || strcmp(argv[1], "notmuch"))) {
	printf(MSGSTR(MSG_ESCAPE_CHAR_IS,
		"Escape character is '%s'.\n"), control(escape));
    }
#   if defined(unix)
    if ((argc >= 2) && !strcmp(argv[1], "everything")) {
	if (sigiocount == 1)
	    printf(MSGSTR(MSG_SIGIO_ONE, "SIGIO received 1 time.\n"));
	else
	    printf(MSGSTR(MSG_SIGIO_N,
		"SIGIO received %d times.\n"), sigiocount);
	if (In3270) {
	    printf(MSGSTR(MSG_PID_PGRP, "Process ID %d, process group %d.\n"),
					    getpid(), getpgrp(getpid()));
	    printf(MSGSTR(MSG_TERM_INPUT, "Terminal input:\n"));
	    filestuff(tin);
	    printf(MSGSTR(MSG_TERM_OUTPUT, "Terminal output:\n"));
	    filestuff(tout);
	    printf(MSGSTR(MSG_NET_SOCKET, "Network socket:\n"));
	    filestuff(net);
	}
    }
    if (In3270 && transcom) {
	printf(MSGSTR(MSG_TRANSPARENT,
		"Transparent mode command is '%s'.\n"), transcom);
    }
#   endif /* defined(unix) */
    (void) fflush(stdout);
    if (In3270) {
	return 0;
    }
#   endif /* defined(TN3270) */
    return 1;
}

#ifdef	SIGINFO
/*
 * Function that gets called when SIGINFO is received.
 */
ayt_status()
{
    (void) call(status, "status", "notmuch", 0);
}
#endif

unsigned long inet_addr();

    int
tn(argc, argv)
    int argc;
    char *argv[];
{
    register struct hostent *host = 0;
    struct sockaddr_in sin;
    struct servent *sp = 0;
    unsigned long temp;
    extern char *inet_ntoa();
#if	defined(IP_OPTIONS) && defined(IPPROTO_IP)
    char *srp = 0, *strrchr();
    unsigned long sourceroute(), srlen;
#endif
    char *cmd, *hostp = 0, *portp = 0, *user = 0;

    /* clear the socket address prior to use */
    memset((char *)&sin, 0, sizeof(sin));

    if (connected) {
	printf(MSGSTR(MSG_ALREADY_CONNECTED,
		"?Already connected to %s\n"), hostname);
	setuid(getuid());
	return 0;
    }
    if (argc < 2) {
	(void) strcpy(line, "open ");
	printf(MSGSTR(MSG_TO, "(to) "));
	(void) fgets(&line[strlen(line)], sizeof(line) - strlen(line), stdin);
	makeargv();
	argc = margc;
	argv = margv;
    }
    cmd = *argv;
    --argc; ++argv;
    while (argc) {
	if (isprefix(*argv, "?"))
	    goto usage;
	if (strcmp(*argv, "-l") == 0) {
	    --argc; ++argv;
	    if (argc == 0)
		goto usage;
	    user = *argv++;
	    --argc;
	    continue;
	}
	if (strcmp(*argv, "-a") == 0) {
	    --argc; ++argv;
	    autologin = 1;
	    continue;
	}
	if (hostp == 0) {
	    hostp = *argv++;
	    --argc;
	    continue;
	}
	if (portp == 0) {
	    portp = *argv++;
	    --argc;
	    continue;
	}
    usage:
	printf(MSGSTR(MSG_OPEN_USAGE,
		"usage: %s [-l user] [-a] host-name [[-]port]\n"), cmd);
	setuid(getuid());
	return 0;
    }
    if (hostp == 0)
	goto usage;

#if	defined(IP_OPTIONS) && defined(IPPROTO_IP)
    if (hostp[0] == '@' || hostp[0] == '!') {
	if ((hostname = strrchr(hostp, ':')) == NULL)
	    hostname = strrchr(hostp, '@');
	hostname++;
	srp = 0;
	temp = sourceroute(hostp, &srp, &srlen);
	if (temp == 0) {
	    herror(srp);
	    setuid(getuid());
	    return 0;
	} else if (temp == -1) {
	    printf(MSGSTR(MSG_BAD_SOURCE_ROUTE,
			"Bad source route option: %s\n"), hostp);
	    setuid(getuid());
	    return 0;
	} else {
	    sin.sin_addr.s_addr = temp;
	    sin.sin_family = AF_INET;
	}
    } else {
#endif
	temp = inet_addr(hostp);
	if (temp != (unsigned long) -1) {
	    sin.sin_addr.s_addr = temp;
	    sin.sin_family = AF_INET;
	    (void) strcpy(_hostname, hostp);
	    hostname = _hostname;
	} else {
	    host = gethostbyname(hostp);
	    if (host) {
		sin.sin_family = host->h_addrtype;
#if	defined(h_addr)		/* In 4.3, this is a #define */
		memmove((caddr_t)&sin.sin_addr,
				host->h_addr_list[0], host->h_length);
#else	/* defined(h_addr) */
		memmove((caddr_t)&sin.sin_addr, host->h_addr, host->h_length);
#endif	/* defined(h_addr) */
		strncpy(_hostname, host->h_name, sizeof(_hostname));
		_hostname[sizeof(_hostname)-1] = '\0';
		hostname = _hostname;
	    } else {
		herror(hostp);
		setuid(getuid());
		return 0;
	    }
	}
#if	defined(IP_OPTIONS) && defined(IPPROTO_IP)
    }
#endif
    if (portp) {
	if (*portp == '-') {
	    portp++;
	    telnetport = 1;
	} else
	    telnetport = 0;
	sin.sin_port = atoi(portp);
	if (sin.sin_port == 0) {
	    sp = getservbyname(portp, "tcp");
	    if (sp)
		sin.sin_port = sp->s_port;
	    else {
		printf(MSGSTR(MSG_BAD_PORT, "%s: bad port number\n"), portp);
		setuid(getuid());
		return 0;
	    }
	} else {
#if	!defined(htons)
	    u_short htons P((unsigned short));
#endif	/* !defined(htons) */
	    sin.sin_port = htons(sin.sin_port);
	}
    } else {
	if (sp == 0) {
	    sp = getservbyname("telnet", "tcp");
	    if (sp == 0) {
		fprintf(stderr, MSGSTR(MSG_UNKNOWN_SERVICE,
			"telnet: tcp/telnet: unknown service\n"));
		setuid(getuid());
		return 0;
	    }
	    sin.sin_port = sp->s_port;
	}
	telnetport = 1;
    }
    printf(MSGSTR(MSG_TRYING, "Trying %s...\n"), inet_ntoa(sin.sin_addr));
    do {
	net = socket(AF_INET, SOCK_STREAM, 0);
	setuid(getuid());
	if (net < 0) {
	    perror("telnet: socket");
	    return 0;
	}
#if	defined(IP_OPTIONS) && defined(IPPROTO_IP)
	if (srp && setsockopt(net, IPPROTO_IP, IP_OPTIONS, (char *)srp, srlen) < 0)
		perror("setsockopt (IP_OPTIONS)");
#endif
#if	defined(IPPROTO_IP) && defined(IP_TOS)
	{
# if	defined(HAS_GETTOS)
	    struct tosent *tp;
	    if (tos < 0 && (tp = gettosbyname("telnet", "tcp")))
		tos = tp->t_tos;
# endif
	    if (tos < 0)
		tos = 020;	/* Low Delay bit */
	    if (tos
		&& (setsockopt(net, IPPROTO_IP, IP_TOS,
		    (char *)&tos, sizeof(int)) < 0)
		&& (errno != ENOPROTOOPT))
		    perror(MSGSTR(MSG_SETSOCKOPT_IGNORED,
			"telnet: setsockopt (IP_TOS) (ignored)"));
	}
#endif	/* defined(IPPROTO_IP) && defined(IP_TOS) */

	if (debug && SetSockOpt(net, SOL_SOCKET, SO_DEBUG, 1) < 0) {
		perror("setsockopt (SO_DEBUG)");
	}

	if (connect(net, (struct sockaddr *)&sin, sizeof (sin)) < 0) {
#if	defined(h_addr)		/* In 4.3, this is a #define */
	    if (host && host->h_addr_list[1]) {
		int oerrno = errno;

		fprintf(stderr, MSGSTR(MSG_CONNECT_TO_ADDR,
			"telnet: connect to address %s: "),
						inet_ntoa(sin.sin_addr));
		errno = oerrno;
		perror((char *)0);
		host->h_addr_list++;
		memmove((caddr_t)&sin.sin_addr,
			host->h_addr_list[0], host->h_length);
		(void) NetClose(net);
		continue;
	    }
#endif	/* defined(h_addr) */
	    perror(MSGSTR(MSG_UNABLE_TO_CONNECT,
			"telnet: Unable to connect to remote host"));
	    return 0;
	}
	connected++;
#if	defined(AUTHENTICATION) || defined(ENCRYPTION)
	auth_encrypt_connect(connected);
#endif	/* defined(AUTHENTICATION) || defined(ENCRYPTION) */
    } while (connected == 0);
    cmdrc(hostp, hostname);
    if (autologin && user == NULL) {
	struct passwd *pw;

	user = getenv("USER");
	if (user == NULL ||
	    (pw = getpwnam(user)) && pw->pw_uid != getuid()) {
		if (pw = getpwuid(getuid()))
			user = pw->pw_name;
		else
			user = NULL;
	}
    }
    if (user) {
	env_define((unsigned char *)"USER", (unsigned char *)user);
	env_export((unsigned char *)"USER");
    }
    (void) call(status, "status", "notmuch", 0);
    if (setjmp(peerdied) == 0)
	telnet(user);
    (void) NetClose(net);
    ExitString(MSGSTR(MSG_CONN_CLOSED_FOREIGN,
	"Connection closed by foreign host.\n"),1);
    /*NOTREACHED*/
}

#define HELPINDENT (sizeof ("connect"))

static int	help();

static Command cmdtab[] = {
	{ "close",
	MN(MSG_CMDT0, "close current connection"),
					bye,		1 },
	{ "logout",
	MN(MSG_CMDT1, "forcibly logout remote user and close the connection"),
					logout,		1 },
	{ "display",
	MN(MSG_CMDT2, "display operating parameters"),
					display,	0 },
	{ "mode",
	MN(MSG_CMDT3, "try to enter line or character mode ('mode ?' for more)"),
					modecmd,	0 },
	{ "open",
	MN(MSG_CMDT4, "connect to a site"),
					tn,		0 },
	{ "quit",
	MN(MSG_CMDT5, "exit telnet"),
					quit,		0 },
	{ "send",
	MN(MSG_CMDT6, "transmit special characters ('send ?' for more)"),
					sendcmd,	0 },
	{ "set",
	MN(MSG_CMDT7, "set operating parameters ('set ?' for more)"),
					setcmd,		0 },
	{ "unset",
	MN(MSG_CMDT8, "unset operating parameters ('unset ?' for more)"),
					unsetcmd,	0 },
	{ "status",
	MN(MSG_CMDT9, "print status information"),
					status,		0 },
	{ "toggle",
	MN(MSG_CMDT10, "toggle operating parameters ('toggle ?' for more)"),
					toggle,		0 },
	{ "slc",
	MN(MSG_CMDT11, "change state of special characters ('slc ?' for more)"),
					slccmd,		0 },
#if	defined(TN3270) && defined(unix)
	{ "transcom",
	MN(MSG_CMDT12, "specify Unix command for transparent mode pipe"),
					settranscom,	0 },
#endif	/* defined(TN3270) && defined(unix) */
#if	defined(AUTHENTICATION)
	{ "auth",
	MN(MSG_CMDT13, "turn on (off) authentication ('auth ?' for more)"),
					auth_cmd,	0 },
#endif
#ifdef	ENCRYPTION
	{ "encrypt",
	MN(MSG_CMDT14, "turn on (off) encryption ('encrypt ?' for more)"),
					encrypt_cmd,	0 },
#endif	/* ENCRYPTION */
#if	defined(unix)
	{ "z",
	MN(MSG_CMDT15, "suspend telnet"),
					suspend,	0 },
#endif	/* defined(unix) */
	{ "!",
	MN(MSG_CMDT16, "invoke a subshell"),
#if	defined(TN3270)
					shell,		1 },
#else
					shell,		0 },
#endif
	{ "environ",
	MN(MSG_CMDT17, "change environment variables ('environ ?' for more)"),
					env_cmd,	0 },
	{ "?",
	MN(MSG_CMDT18, "print help information"),
					help,		0 },
	0
};

static Command cmdtab2[] = {
	{ "help",	MN(0, 0),	help,		0 },
	{ "escape",
	MN(MSG_CMDTT0, "deprecated command -- use 'set escape' instead"),
					setescape,	0 },
	{ "crmod",
	MN(MSG_CMDTT1, "deprecated command -- use 'toggle crmod' instead"),
					togcrmod,	0 },
	0
};


/*
 * Call routine with argc, argv set from args (terminated by 0).
 */

    /*VARARGS1*/
    static
call(va_alist)
    va_dcl
{
    va_list ap;
    typedef int (*intrtn_t)();
    intrtn_t routine;
    char *args[100];
    int argno = 0;

    va_start(ap);
    routine = (va_arg(ap, intrtn_t));
    while ((args[argno++] = va_arg(ap, char *)) != 0) {
	;
    }
    va_end(ap);
    return (*routine)(argno-1, args);
}


    static Command *
getcmd(name)
    char *name;
{
    Command *cm;

    if (cm = (Command *) genget(name, (char **) cmdtab, sizeof(Command)))
	return cm;
    return (Command *) genget(name, (char **) cmdtab2, sizeof(Command));
}

    void
command(top, tbuf, cnt)
    int top;
    char *tbuf;
    int cnt;
{
    register Command *c;

    setcommandmode();
    if (!top) {
	putchar('\n');
#if	defined(unix)
    } else {
	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
#endif	/* defined(unix) */
    }
    for (;;) {
	if (rlogin == _POSIX_VDISABLE)
		printf("%s> ", prompt);
	if (tbuf) {
	    register char *cp;
	    cp = line;
	    while (cnt > 0 && (*cp++ = *tbuf++) != '\n')
		cnt--;
	    tbuf = 0;
	    if (cp == line || *--cp != '\n' || cp == line)
		goto getline;
	    *cp = '\0';
	    if (rlogin == _POSIX_VDISABLE)
		printf("%s\n", line);
	} else {
	getline:
	    if (rlogin != _POSIX_VDISABLE)
		printf("%s> ", prompt);
	    if (fgets(line, sizeof(line), stdin) == NULL) {
		if (feof(stdin) || ferror(stdin)) {
		    (void) quit();
		    /*NOTREACHED*/
		}
		break;
	    }
	}
	if (line[0] == 0)
	    break;
	makeargv();
	if (margv[0] == 0) {
	    break;
	}
	c = getcmd(margv[0]);
	if (Ambiguous(c)) {
	    printf(MSGSTR(MSG_AMBIGUOUS_CMD, "?Ambiguous command\n"));
	    continue;
	}
	if (c == 0) {
	    printf(MSGSTR(MSG_INVALID_CMD, "?Invalid command\n"));
	    continue;
	}
	if (c->needconnect && !connected) {
	    printf(MSGSTR(MSG_CONNECT_FIRST, "?Need to be connected first.\n"));
	    continue;
	}
	if ((*c->handler)(margc, margv)) {
	    break;
	}
    }
    if (!top) {
	if (!connected) {
	    longjmp(toplevel, 1);
	    /*NOTREACHED*/
	}
#if	defined(TN3270)
	if (shell_active == 0) {
	    setconnmode(0);
	}
#else	/* defined(TN3270) */
	setconnmode(0);
#endif	/* defined(TN3270) */
    }
}

/*
 * Help command.
 */
	static
help(argc, argv)
	int argc;
	char *argv[];
{
	register Command *c;

	if (argc == 1) {
		printf(MSGSTR(MSG_CMDS_ABBREVIATED,
			"Commands may be abbreviated.  Commands are:\n\n"));
		for (c = cmdtab; c->name; c++)
			if (c->help) {
				printf("%-*s\t%s\n", HELPINDENT, c->name,
					MNSTR(c->helpnum, c->help));
			}
		return 0;
	}
	while (--argc > 0) {
		register char *arg;
		arg = *++argv;
		c = getcmd(arg);
		if (Ambiguous(c))
			printf(MSGSTR(MSG_AMBIGUOUS_HELP,
				"?Ambiguous help command %s\n"), arg);
		else if (c == (Command *)0)
			printf(MSGSTR(MSG_INVALID_HELP,
				"?Invalid help command %s\n"), arg);
		else
			printf("%s\n", MNSTR(c->helpnum, c->help));
	}
	return 0;
}

static char *rcname = 0;
static char rcbuf[128];

cmdrc(m1, m2)
	char *m1, *m2;
{
    register Command *c;
    FILE *rcfile;
    int gotmachine = 0;
    int l1 = strlen(m1);
    int l2 = strlen(m2);
    char m1save[64];

    if (skiprc)
	return;

    strcpy(m1save, m1);
    m1 = m1save;

    if (rcname == 0) {
	rcname = getenv("HOME");
	if (rcname)
	    strcpy(rcbuf, rcname);
	else
	    rcbuf[0] = '\0';
	strcat(rcbuf, "/.telnetrc");
	rcname = rcbuf;
    }

    if ((rcfile = fopen(rcname, "r")) == 0) {
	return;
    }

    for (;;) {
	if (fgets(line, sizeof(line), rcfile) == NULL)
	    break;
	if (line[0] == 0)
	    break;
	if (line[0] == '#')
	    continue;
	if (gotmachine) {
	    if (!isspace(line[0]))
		gotmachine = 0;
	}
	if (gotmachine == 0) {
	    if (isspace(line[0]))
		continue;
	    if (strncasecmp(line, m1, l1) == 0)
		strncpy(line, &line[l1], sizeof(line) - l1);
	    else if (strncasecmp(line, m2, l2) == 0)
		strncpy(line, &line[l2], sizeof(line) - l2);
	    else if (strncasecmp(line, "DEFAULT", 7) == 0)
		strncpy(line, &line[7], sizeof(line) - 7);
	    else
		continue;
	    if (line[0] != ' ' && line[0] != '\t' && line[0] != '\n')
		continue;
	    gotmachine = 1;
	}
	makeargv();
	if (margv[0] == 0)
	    continue;
	c = getcmd(margv[0]);
	if (Ambiguous(c)) {
	    printf(MSGSTR(MSG_AMBIGUOUS_COMMAND,
			"?Ambiguous command: %s\n"), margv[0]);
	    continue;
	}
	if (c == 0) {
	    printf(MSGSTR(MSG_INVALID_COMMAND,
			"?Invalid command: %s\n"), margv[0]);
	    continue;
	}
	/*
	 * This should never happen...
	 */
	if (c->needconnect && !connected) {
	    printf(MSGSTR(MSG_CONNECTED_FIRST,
		"?Need to be connected first for %s.\n"), margv[0]);
	    continue;
	}
	(*c->handler)(margc, margv);
    }
    fclose(rcfile);
}

#if	defined(IP_OPTIONS) && defined(IPPROTO_IP)

/*
 * Source route is handed in as
 *	[!]@hop1@hop2...[@|:]dst
 * If the leading ! is present, it is a
 * strict source route, otherwise it is
 * assmed to be a loose source route.
 *
 * We fill in the source route option as
 *	hop1,hop2,hop3...dest
 * and return a pointer to hop1, which will
 * be the address to connect() to.
 *
 * Arguments:
 *	arg:	pointer to route list to decipher
 *
 *	cpp: 	If *cpp is not equal to NULL, this is a
 *		pointer to a pointer to a character array
 *		that should be filled in with the option.
 *
 *	lenp:	pointer to an integer that contains the
 *		length of *cpp if *cpp != NULL.
 *
 * Return values:
 *
 *	Returns the address of the host to connect to.  If the
 *	return value is -1, there was a syntax error in the
 *	option, either unknown characters, or too many hosts.
 *	If the return value is 0, one of the hostnames in the
 *	path is unknown, and *cpp is set to point to the bad
 *	hostname.
 *
 *	*cpp:	If *cpp was equal to NULL, it will be filled
 *		in with a pointer to our static area that has
 *		the option filled in.  This will be 32bit aligned.
 *
 *	*lenp:	This will be filled in with how long the option
 *		pointed to by *cpp is.
 *
 */
	unsigned long
sourceroute(arg, cpp, lenp)
	char	*arg;
	char	**cpp;
	int	*lenp;
{
	static char lsr[44];
#ifdef	sysV88
	static IOPTN ipopt;
#endif
	char *cp, *cp2, *lsrp, *lsrep;
	register int tmp;
	struct in_addr sin_addr;
	register struct hostent *host = 0;
	register char c;

	/*
	 * Verify the arguments, and make sure we have
	 * at least 7 bytes for the option.
	 */
	if (cpp == NULL || lenp == NULL)
		return((unsigned long)-1);
	if (*cpp != NULL && *lenp < 7)
		return((unsigned long)-1);
	/*
	 * Decide whether we have a buffer passed to us,
	 * or if we need to use our own static buffer.
	 */
	if (*cpp) {
		lsrp = *cpp;
		lsrep = lsrp + *lenp;
	} else {
		*cpp = lsrp = lsr;
		lsrep = lsrp + 44;
	}

	cp = arg;

	/*
	 * Next, decide whether we have a loose source
	 * route or a strict source route, and fill in
	 * the begining of the option.
	 */
#ifndef	sysV88
	if (*cp == '!') {
		cp++;
		*lsrp++ = IPOPT_SSRR;
	} else
		*lsrp++ = IPOPT_LSRR;
#else
	if (*cp == '!') {
		cp++;
		ipopt.io_type = IPOPT_SSRR;
	} else
		ipopt.io_type = IPOPT_LSRR;
#endif

	if (*cp != '@')
		return((unsigned long)-1);

#ifndef	sysV88
	lsrp++;		/* skip over length, we'll fill it in later */
	*lsrp++ = 4;
#endif

	cp++;

	sin_addr.s_addr = 0;

	for (c = 0;;) {
		if (c == ':')
			cp2 = 0;
		else for (cp2 = cp; c = *cp2; cp2++) {
			if (c == ',') {
				*cp2++ = '\0';
				if (*cp2 == '@')
					cp2++;
			} else if (c == '@') {
				*cp2++ = '\0';
			} else if (c == ':') {
				*cp2++ = '\0';
			} else
				continue;
			break;
		}
		if (!c)
			cp2 = 0;

		if ((tmp = inet_addr(cp)) != -1) {
			sin_addr.s_addr = tmp;
		} else if (host = gethostbyname(cp)) {
#if	defined(h_addr)
			memmove((caddr_t)&sin_addr,
				host->h_addr_list[0], host->h_length);
#else
			memmove((caddr_t)&sin_addr, host->h_addr, host->h_length);
#endif
		} else {
			*cpp = cp;
			return(0);
		}
		memmove(lsrp, (char *)&sin_addr, 4);
		lsrp += 4;
		if (cp2)
			cp = cp2;
		else
			break;
		/*
		 * Check to make sure there is space for next address
		 */
		if (lsrp + 4 > lsrep)
			return((unsigned long)-1);
	}
#ifndef	sysV88
	if ((*(*cpp+IPOPT_OLEN) = lsrp - *cpp) <= 7) {
		*cpp = 0;
		*lenp = 0;
		return((unsigned long)-1);
	}
	*lsrp++ = IPOPT_NOP; /* 32 bit word align it */
	*lenp = lsrp - *cpp;
#else
	ipopt.io_len = lsrp - *cpp;
	if (ipopt.io_len <= 5) {		/* Is 3 better ? */
		*cpp = 0;
		*lenp = 0;
		return((unsigned long)-1);
	}
	*lenp = sizeof(ipopt);
	*cpp = (char *) &ipopt;
#endif
	return(sin_addr.s_addr);
}
#endif
