#ident  "@(#)atdialer.c	1.7"
/*
 *	Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <utmp.h>
#include "rce.h"

#include <deflt.h>

#define DEFAULT_DIR	"/etc/uucp/default/"


/*
 * This table defines the modem setup/option strings
 */
struct mdm_cfg {
	char *param;
	char *val;
} mdm_cfg[] = {
#define	MDM_STTY	0
	"STTY", 	"-ORTSFL RTSFLOW CTSFLOW",
#define	MDM_SETUP	1
	"MDM_SETUP",	"AT&F0",
#define	MDM_OPTION	2
	"MDM_OPTION",	"",
#define	MDM_DIALCMD	3
	"MDM_DIALCMD",	"ATDT",
#define	MDM_ESCAPE	4
	"MDM_ESCAPE",	"+++",
#define	MDM_HANGUP	5
	"MDM_HANGUP",	"ATQ0H0",
#define	MDM_RESET	6
	"MDM_RESET",	"",
#define	MDM_DIALIN	7
	"MDM_DIALIN",	"ATS0=1",
#define	MDM_ATTN	8
	"MDM_ATTN",	"",
#define	MDM_DSBLESC	9
	"MDM_DSBLESC",	"ATS2=128",
#define	MDM_ONLINE	10
	"MDM_ONLINE",	"",
#define MDM_FAXBAUD	11
	"MDM_FAXBAUD",	"19200",
#define	MDM_ATSPEED	12
	"MDM_ATSPEED",	"0",
#define	MDM_CDS		13
	"MDM_CDS",	"AT+FAA=1;+FCR=1",
#define	MDM_QUIET	14
	"MDM_QUIET",	"ATQ1",
#define MDM_SPEAKER	15
	"MDM_SPEAKER",	"ATM0",
#define MDM_FAXCLASS	16
	"MDM_FAXCLASS","",
#define	MDM_NOTQUIET	17
	"MDM_NOTQUIET",	"ATQ0",
#define MDM_VERBOSE	18
	"MDM_VERBOSE",	"ATV1",
#define MDM_NOTVERBOSE	19
	"MDM_NOTVERBOSE",	"ATV0",
#define MDM_ECHO	18
	"MDM_ECHO",	"ATE1",
#define MDM_NOTECHO	19
	"MDM_NOTECHO",	"ATE0",
	NULL,		NULL,
};

/*
 * This table defines the list of recognised strings returned by the modem
 */
struct mdm_rtc {
	char *param;
	char *expect;
	speed_t baud;
} mdm_rtc[] = {
#define	RTC_OK		0
	"RTC_OK=",	"OK",	0,
#define	RTC_NOCARR	1
	"RTC_NOCARR=",	"NO CARRIER",	0,
#define	RTC_ERROR	2
	"RTC_ERROR",	"ERROR",	0,
#define	RTC_NOTONE	3
	"RTC_NOTONE=",	"NO DIAL",	0,
#define	RTC_BUSY		4
	"RTC_BUSY=",	"BUSY",	0,
#define	RTC_NOANS	5
	"RTC_NOANS=",	"NO ANSWER",	0,
#define	RTC_DATA		6
	"RTC_DATA=",	"DATA",	0,
#define	RTC_FAX		7
	"RTC_FAX=",	"FAX",	0,
#define RTC_CONNECT	8
	"RTC_CONNECT=",	"CONNECT",	0,
	"RTC_50=",	"not used",	B50,
	"RTC_75=",	"not used",	B75,
	"RTC_110=",	"not used",	B110,
	"RTC_134=",	"not used",	B134,
	"RTC_150=",	"not used",	B150,
	"RTC_200=",	"not used",	B200,
	"RTC_300=",	"not used",	B300,
	"RTC_1200=",	"not used",	B1200,
	"RTC_2400=",	"not used",	B2400,
	"RTC_4800=",	"not used",	B4800,
	"RTC_9600=",	"not used",	B9600,
	"RTC_19200=",	"not used",	B19200,
	"RTC_38400=",	"not used",	B38400,
	"RTC_57600=",	"not used",	B57600,
	"RTC_76800=",	"not used",	B76800,
	"RTC_115200=",	"not used",	B115200,
	"RTC_230400=",	"not used",	B230400,
	"RTC_460800=",	"not used",	B460800,
	"RTC_921600=",	"not used",	B921600,
	NULL,		NULL,	0,
#define RTC_ANY		255
};

/*
 * Baud rate lookup table 
 */
struct baud {
	int rate;
	int value;
} bauds[] = {
	50,	B50,
	75,	B75,
	110,	B110,
	134,	B134,
	150,	B150,
	200,	B200,
	300,	B300,
	600,	B600,
	1200,	B1200,
	1800,	B1800,
	2400,	B2400,
	4800,	B4800,
	9600,	B9600,
	19200,	B19200,
	38400,	B38400,
	57600,	B57600,
	76800,	B76800,
	115200,	B115200,
	230400,	B230400,
	460800,	B460800,
	921600,	B921600,
	0,	0,
};


/*
 *  These defines are used to determine how long the dialer timeout
 *  should be.  MDPULSDLY can be changed, but MDPAUSDLY requires
 *  reprogramming modem register S8 to be effective.
 */
#define	MDPULSCHR	'P'
#define	MDPULSDLY	15
#define	MDPAUSCHR	','
#define	MDPAUSDLY	2

#define MDVALID		"0123456789PpTtRrSsWwXx,!@*#()-"
#define	DIAL_RETRY	3  
#define MAXLINE		255
#define DEBUG(l, f, s)	if (Debug >= l) fprintf(stderr, f, s)


char *strchr();
void alrmint(int);
void abort(int sig);
void mdwritecmd(int cmd);
void mdwrite(register char *c, int cr);
void mdwritechar(char c);
char *vgets(unsigned char c);
void translate(char *ttab, char *str);

struct termios term;
int Debug = 0;			/* set when debug flag is given	*/
int dialing;				/* set while modem is dialing	*/
int fd = -1;				/* file descriptor for acu	*/
char def_buffer[MAXLINE];
int atspeed = 0;
int callselection = 0;		/* Call selection requested */
int eintrok = 0;		/* If set don't exit if sys call gets EINTR */

main(int argc,char **argv)
{
	char *phone, *p, *acu;
	int errflag = 0;		/* set on errors		*/
	int timeout = 0;		/* how long to wait for alarm	*/
	int dial_retry = DIAL_RETRY;	/* dial retry count		*/
	int highbaud, lowbaud;		/* baud rate limits		*/
	extern int optind;
	extern char *optarg;
	int c;
	extern int fields();
	int option = 0;			/* Option string required */
	int inbound = 0;		/* Setup for incoming call (-h) */
	int rtc;
	int faxbaud;

	/*
	 *  Reenable all those signals we want to know about
	 */

	signal(SIGILL, SIG_DFL);
	signal(SIGIOT, SIG_DFL);
	signal(SIGEMT, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGSYS, SIG_DFL);
	signal(SIGTERM, abort);


	/*
	 * Find out the command line options for this program.
	 */
	while ((c = getopt(argc, argv, "cfhx:")) != EOF)
		switch(c) {
		case 'h':
			inbound++;
			break;
		case 'f':
			callselection++;
			break;
		case 'x':
			Debug = atoi(optarg);
			break;
		case '?':
			errflag++;
			break;
		}

	/*
	 * This is where argv(0) from the default directory is read in and
	 * is the part that really makes this whole thing work right for 
	 * more than one specific modem.
	 */
	read_defaults (argv[0]);

	if (Debug) {
		fprintf(stderr, "dialer args ");
		for (c=0; c<argc; c++)
			fprintf(stderr, ":%s", argv[c]);
		fprintf(stderr, "\n");
	}


	if (argc - optind != 3 && !(inbound && argc - optind == 2))
		errflag++;


	if (errflag) {
		if (inbound)
			fprintf(stderr,"Usage: %s -h [-f] [-x] devicename speed\n", argv[0]);
		else
			fprintf(stderr,"Usage: %s [-x] devicename number speed\n", argv[0]);
		exit(RCE_ARGS);
	}

	/*
	 * Get the device 
	 */
	acu = argv[optind++];
	if (strchr(acu,'/') == NULL) {
		char buf[BUFSIZ];

		strcpy(buf,"/dev/");
		strcat(buf,acu);
		acu = strdup(buf);
		DEBUG(8,"opening %s\n",acu);
	}

	/*
	 * Check the outgoinf phone number
	 */
	if (!inbound) {
		phone = argv[optind++];
		translate("=,-,", phone);
		if (strlen(phone) != strspn(phone, MDVALID)) {
			fprintf(stderr, "dial: Bad phone number %s\n", phone);
			exit(RCE_PHNO);
		}
		if (strlen(phone) != strcspn(phone, "Xx")) {
			option = 1;
		}
	}

	lowbaud = highbaud = checkbaud(atoi(argv[optind]));

	/* test for a range of baudrates */

	if ((p = strchr(argv[optind], '-')) != NULL) {
		*p++ = '\0';
		highbaud = checkbaud(atoi(p));
	}

	/*
	 * atspeed - the number of milliseconds to sleep between
	 * sending characters to the modem
	 */
	atspeed = atoi(mdm_cfg[MDM_ATSPEED].val);

	/*
	 *  Must open with O_NDELAY set or the open may hang.
	 */
	if ((fd = open(acu, O_RDWR | O_NDELAY)) < 0) {
		fprintf(stderr, "dial: Can't open device: %s\n", acu);
		exit(RCE_OPEN);
	}

	/*
	 * Make sure that the modem is on hook
	 */
	tcgetattr(fd, &term);
	tcsetspeed(TCS_ALL,&term,0);
	tcsetattr(fd, TCSANOW, &term);
	nap(2000);

	/*
	 * set line for no echo and correct speed.
	 */
	signal(SIGINT, abort);
	errflag = tcgetattr(fd, &term);

	term.c_iflag &= ~(IXON | IXOFF | IXANY);
	term.c_cflag &= ~(HUPCL); 

	tcsetspeed(TCS_ALL,&term,highbaud);

	term.c_lflag &= ~ECHO;
	term.c_cflag |= (CLOCAL);
	term.c_cc[VMIN] = '\1';
	term.c_cc[VTIME] = '\0';

	errflag = tcsetattr(fd, TCSANOW, &term);
	if (errflag) {
		DEBUG(1, "dial: tcsetattr error on %s\n", acu);
		cleanup(RCE_IOCTL);
	}
	/*
	 *  Reopen line with clocal so we can talk without carrier present
	 */
	c = fd;
	if ((fd = open(acu, O_RDWR)) < 0) {
		fprintf(stderr, "dial: Can't open device local: %s\n", acu);
		cleanup(RCE_OPEN);
	}
	close(c);

redial:
	hangup(15);

	signal(SIGALRM, alrmint);

	/* force modem to be in verbose mode */
	mdwritecmd(MDM_VERBOSE);
	mdread(15, RTC_OK);

	mdwritecmd(MDM_SETUP);
	mdflush();
	mdwritecmd(MDM_VERBOSE);
	mdread(15, RTC_OK);

	/* just in case the mdm setup had verbose off, set verbose on */
	mdwritecmd(MDM_VERBOSE);
	mdread(15, RTC_OK);

	if (*mdm_cfg[MDM_STTY].val)
		fields(mdm_cfg[MDM_STTY].val);
				/* If some of the previous settings
				 * are not desired they should change if
				 * defined in /etc/uucp/default/<dialer_name>
				 */
	
	errflag = tcsetattr(fd, TCSANOW, &term);
	if (errflag) {
		DEBUG(1, "dial: tcsetattr error on %s\n", acu);
		cleanup(RCE_IOCTL);
	}

	/*
	 * Set the speaker option
	 */
	mdwritecmd(MDM_SPEAKER);
	mdread(15, RTC_OK);

	if (inbound) {
		incoming();
		timeout = 0;
	} else
		timeout = outgoing(phone, option);

expect:
	DEBUG(6, "wait for connect - timeout %d\n", timeout);

	rtc = mdread(timeout, RTC_ANY);

	/*
	 * If we had a RTC_speed response, check it is within our
	 * specified range.
	 */
	if (mdm_rtc[rtc].baud) {
		 matchbaud(mdm_rtc[rtc].baud, lowbaud, highbaud);
	}

	switch (rtc) {
	case RTC_OK: 
	case RTC_NOCARR:
		cleanup(RCE_NOCARR);

	case RTC_ERROR:
		if (dial_retry--) 
			goto redial;
		cleanup(RCE_NULL);

	case RTC_NOTONE:
		cleanup(RCE_NOTONE);

	case RTC_BUSY:
		cleanup(RCE_BUSY);

	case RTC_NOANS:
		cleanup(RCE_ANSWER);
		
	case RTC_FAX:	/* Fax */
		/* Switch baud rate to MDM_FAXBAUD */

		if (*mdm_cfg[MDM_FAXBAUD].val) {
			faxbaud = checkbaud(atoi(mdm_cfg[MDM_FAXBAUD].val));
			tcsetspeed(TCS_ALL,&term,faxbaud);

			errflag = tcsetattr(fd, TCSANOW, &term);
			if (errflag) {
				DEBUG(1, "dial: tcsetattr error on %s\n", acu);
				cleanup(RCE_IOCTL);
			}
		}

		cleanup(RCE_FAXMODE);

	case RTC_DATA:	/* Data */
		DEBUG(6, "going online\n", 0);
		if (*mdm_cfg[MDM_ONLINE].val)
			mdwrite(mdm_cfg[MDM_ONLINE].val, 1);

		/* ... now expect the connect string .... */

		timeout=15;
		goto expect;

	case RTC_CONNECT:
		if (inbound) {
			mdflush();
			cleanup(RCE_DATAMODE);
		} else
			cleanup(0);

	default:
		cleanup(RCE_FAIL);
	}
}	

/*
 * Setup modem for incoming calls
 */
incoming()
{
	mdwritecmd(MDM_DSBLESC); /* disable escape */
	mdread(15, RTC_OK);

	mdwritecmd(MDM_DIALIN);	/* setup modem for dialin */
	mdread(15, RTC_OK);
		
	if (!callselection) {
		DEBUG(6, "Doing standard incoming setup\n", 0);

		/* Go quiet until we have a connection */
		mdwritecmd(MDM_QUIET);
		cleanup(0);
	}

	/* For auto ... send the Call Discrimination and Selection 
	 * string to the modmem.
	 */

	if (*mdm_cfg[MDM_FAXCLASS].val) {
		mdwritecmd(MDM_FAXCLASS);
		mdread(15, RTC_OK);
	}
	
	mdwritecmd(MDM_CDS);
	mdread(15, RTC_OK);
}

/*
 * Make an outgoing call over the modem
 */
outgoing(char *phone, int option)
{
	char command[MAXLINE];		/* modem command buffer		*/
	int timeout;
	char *p;

	/* This section is being added to allow putting specific options
	 * in the dialer to do things like control G-packet acking, or 
	 * other options that would be brand specific. It is optional and
	 * is used by putting an x or an X at the end of the phone number.
	 * if the phone number has the "Xx" at the end, then the mdm_option
	 * string will be sent to the modem.
	 */
	if (option) {
		DEBUG(6,"%s\n","Sending modem option string!");
		mdwritecmd(MDM_OPTION);
		nap(200);
		mdflush();
	}

	/*
	 *  Build up the phone number
	 */
	sprintf(command,"%s%s", mdm_cfg[MDM_DIALCMD].val, phone);

	/*
	 *  Set up a timeout for the connect.
	 *    Add in MDPAUSDLY seconds more for each pause character
	 *    Pulse dialing takes MDPULSDLY seconds longer too
	 *
	 *    This dialer computes a slightly extended delay since the
	 *    newer higher speed modems can take longer to negotiate a
	 *    common protocol.
	 */
	timeout = 12 * strlen(phone) + 25;
	for (p = phone; (p = strchr(p, MDPAUSCHR)) != NULL; p++)
		timeout += MDPAUSDLY;
	if (strchr(phone, MDPULSCHR) != NULL)
		timeout += MDPULSDLY;
	if (timeout < 45)
		timeout = 45;
	
	/* command string can only be 40 characters excluding "AT" */
	if (strlen(command) > 42)
		cleanup(RCE_PHNO);

	DEBUG(6,"%s\n","sending dial string");
	mdwrite(command, 1);

	dialing = 1;
	return timeout;
}

/*
 *  mdread(rtime)
 *
 *  Function:	Reads from the ACU until it finds a valid response (found
 *		in modem msg structure) or times out after rtime seconds.
 *
 *  Returns:	The index in mdmsgs of the modem response found.
 *		-1 on timeout.
 *
 */
int
mdread(int rtime, int expect)
{
	char c,**mp;
	int index;
	register char *bp;
	char buf[MAXLINE];
	bp = buf;
	alarm(rtime);
	DEBUG(6, "MODEM returned %s", "<<");
	while (read(fd, &c, 1) == 1) {

		if (atspeed)
			nap(atspeed);

		c &= 0177;
		if ((*bp = c) != '\0')
			*++bp = '\0';
		DEBUG(6,"%s", vgets(c));
		if (bp >= buf + MAXLINE) {
			alarm(0);
			DEBUG(6,">>-%s\n","FAIL");
			cleanup(RCE_FAIL);
		}
		if (c == '\r' || c =='\n' )
			for (index=0; mdm_rtc[index].expect; index++){
			if (substr (mdm_rtc[index].expect,buf) == 0) {
				alarm(0);
				DEBUG(6,">>-%s\n", mdm_rtc[index].expect);
				if (expect != RTC_ANY && index != expect)
					cleanup(RCE_FAIL);
				return(index);	
			}
		}
	}
	alarm(0);
	DEBUG(6,">>-%s","FAIL");
	DEBUG(4, " no response\n", 0);
	if (eintrok)
		return(-1);
	cleanup(RCE_FAIL);
}

/*  mdflush()
 *
 *  Function:	Flushes input clists for modem
 */
mdflush()
{
	nap(200);
	ioctl(fd, TCFLSH, 0) ;
}

/*
 *  mdwritecmd(c)
 *
 *  Function:	Outputs the string pointed to by c to the ACU device.
 *
 *  Returns:	0 on completion.
 *		-1 on write errors.
 *
 */
void
mdwritecmd(int cmd)
{
	mdwrite(mdm_cfg[cmd].val, 1);
}

void
mdwrite(register char *c, int cr)
{
	/*
	 *  Give modem a chance to recover before writing.
	 */
	nap(200);
	DEBUG(6, "\rSent MODEM %s", "<<");
	while (*c) {
		mdwritechar(*c);
		c++;
	}
	if (cr)
		mdwritechar('\r');

	DEBUG(6, ">>-%s\n", "OK");
}

void
mdwritechar(char c)
{
	int err;

	if (atspeed)
		nap(atspeed);

	if ((err = write(fd, &c, 1)) != 1) {
		DEBUG(6, ">>-%s\n", "FAIL");
		DEBUG(1, "ACU write error (errno=%d)\n", errno);
		if (eintrok && errno == EINTR)
			return;
		cleanup(RCE_FAIL);
	}
	DEBUG(6, "%s", vgets(c));
}

/*
 *  substr(s, l)
 *
 *  Function:	Checks for the presence of the string pointed to by s
 *		somewhere within the string pointed to by l.
 *
 *  Returns:	0 if found.
 *		-1 if not found.
 */
substr( char *s, char *l)
{
	int len;
	len = strlen(s);	
	while ((l = strchr(l, *s)) != NULL) {
		if (strncmp(s, l, len) == 0)
			return(0);
		l++;
	}
	return(-1);
}

/*
 *  alrmint()
 *
 *  Function:	Catches alarm calls (signal 14) and exits.
 *
 *  Returns:	No return.  Exits with status RCE_TIMEOUT
 */
void
alrmint(int sig)
{
	DEBUG(4, "\nTimeout waiting for %s\n", dialing ? "carrier" : "acu");
	cleanup(RCE_TIMOUT);
}


/*
 *  cleanup(stat)
 *
 *  Function:	Closes device file and exits.
 *
 *  Returns:	No return.  Exits with status stat.
 */
cleanup(int code)
{
	/* Set clocal back */
	if (fd != -1) {
	        tcgetattr(fd, &term);
		term.c_cflag &= ~CLOCAL;
		term.c_cflag |= HUPCL;
		tcsetattr(fd, TCSANOW, &term);


		/* if we failed, drop DTR */
		if (code & RCE_FAIL) {	
			DEBUG(5, "Dropping DTR\n", 0);
		        tcgetattr(fd, &term);
			tcsetspeed(TCS_ALL,&term,0);
			tcsetattr(fd, TCSANOW, &term);
			sleep(5);
		}
	}
	DEBUG(6, "Exit with %d\n", code);
	exit(code);
}

/*
 * On abort signal, redo the serial ioctl and return control to calling
 * program with status.
 */
void
abort(int sig)
{
	signal(SIGINT, SIG_IGN);
	if (fd != -1) {
		DEBUG(5, "Dropping DTR", 0);
	        tcgetattr(fd, &term);
		term.c_cflag |= HUPCL;		/* make sure modem hangs up */
		tcsetattr(fd, TCSANOW, &term);
		close(fd);
	}
	exit(RCE_SIG);
}

/*
 *  hangup(htime)
 *
 *  Function:	Forces the modem to drop carrier and hang up the phone.
 *		Reads are allowed htime seconds before timeout.
 *
 */
void nil(int sig) {signal(SIGALRM, nil);}

hangup(int htime)
{
	int retry = 4, rcode = -1;
	void (*s)();

	DEBUG(4, "hangup - timeout %d\n", htime);

	s = signal(SIGALRM, nil);	/* alarms are non-fatal here */
	eintrok = 1;
	while (retry--  &&  rcode == -1) {
		mdwrite(mdm_cfg[MDM_ESCAPE].val, 0);
		/* Guard of 1 second minimum */
		nap(1500);
		/*
		 * MDESCAPE will return OK only if online,
		 * so ignore error return
		 */
		mdflush();
		mdwritecmd(MDM_NOTQUIET);
		mdflush();
		mdwritecmd(MDM_VERBOSE);
		mdflush();
		mdwritecmd(MDM_HANGUP);
		if (mdread(htime, RTC_ANY) == RTC_OK)
			rcode = 0;
		else {
			/* Guard of 1 second minimum */
			nap(1500);
		}
	}
	eintrok = 0;
	signal(SIGALRM, s);
}

getbaudindex(int n)
{
	int i;

	for (i = 0; bauds[i].value; i++)
		if (bauds[i].value == n)
			return(i);

	fprintf(stderr, "atdial: getbaudindex - bad speed : %d\n", n);

	cleanup(RCE_SPEED);
}
/*
 *  checkbaud(n)
 *
 *  Function:	Check for valid baud rates
 *
 *  Returns:	The baud rate in struct termio c_cflag fashion
 *
 */
checkbaud(int n)
{
	int i;

	for (i = 0; bauds[i].rate; i++)
		if (bauds[i].rate == n)
			return(bauds[i].rate);

	fprintf(stderr, "atdial: Bad speed: %d\n", n);
	cleanup(RCE_SPEED);
}

/*
 *  matchbaud(connect, high, low)
 *
 *  Function:	determine dialer return code based on connect, high, and low
 *		baud rates
 *
 *  Returns:	0		if connected baud == high baud
 *		Bxxxx		if low baud <= connected baud <= high baud
 *		Otherwise cleanup with RCE_SPEED
 */
matchbaud(cbaud, low, high)
int cbaud, low, high;
{
	DEBUG(6,"CBAUD= %d\n",cbaud);
	DEBUG(6,"LOWBAUD = %d\n",low);
	DEBUG(6,"HIGHBAUD = %d\n",high);
	if (cbaud == high)
		return(0);	      /* uucp/cu assume highest baud */
	if (low <= cbaud && cbaud <= high)
		return(cbaud);
	cleanup(RCE_SPEED);
}

FILE * open_defaults(char *progname, char *default_dir)
{
	char *p;

	strcpy(def_buffer,default_dir);
	if (( p =strrchr(progname,'/')) == NULL)
		strcat(def_buffer,progname);  /* assumes no pathname       */
	else
		strcat(def_buffer, p + 1); /* assumes '/' in pathname   */
	return(defopen(def_buffer));
}

/*************************************************************************
	The following procedure goes to the default file argv(0) and reads
	in all of the text that will be sent to and read from the modem.
**************************************************************************/
read_defaults (progname)
	char *progname;
{
	FILE *def_fp;
	int i, j;
	char *p;

	if( (def_fp =open_defaults(progname,DEFAULT_DIR)) == NULL ){
		fprintf(stderr, "%s required. \n",def_buffer);
		exit(RCE_NULL);	
	}

	/*
	 * Read the modem configuration
	 */
	for (i = 0; mdm_cfg[i].param; i++) {
		p = defread(def_fp,mdm_cfg[i].param);

		if (!p) {
			if (mdm_cfg[i].val)
				continue;

			fprintf(stderr, "%s required in %s\n", 
				mdm_cfg[i].param, def_buffer);
			exit(RCE_NULL);
		}
		mdm_cfg[i].val = strdup(p);
	}

	/*
	 * Read the expected result codes
	 */
	for (i = 0; mdm_rtc[i].param; i++) {
		p = defread(def_fp,mdm_rtc[i].param);

		if (!p) {
			if (mdm_rtc[i].expect)
				continue;

			fprintf(stderr, "%s required in %s\n", 
				mdm_rtc[i].param, def_buffer);
			exit(RCE_NULL);
		}
		mdm_rtc[i].expect = strdup(p);
	}
}

/*
 * vgets - Format one character in "always printable" format (like cat -v)
 *         This procedure works even though it looks like it wouldn't 
 */
#define	TOPRINT(x)	((x)<' '?((x)+'@'):'?')
char *
vgets(unsigned char c)
{
	static char buffer[10];
	char *pnt;

	pnt = buffer;
	if (iscntrl(c) || !isprint(c)) {
		if (!isascii(c)) {			/* Top bit is set */
			*pnt++ = 'M';
			*pnt++ = '-';
			c = toascii(c);			/* Strip it */
		}
		if (iscntrl(c)) {			/* Not printable */
			*pnt++ = '^';
			c = TOPRINT(c);			/* Make it printable */
		}
	}
	*pnt++ = c;
	*pnt = '\0';
	return(buffer);
}

/*
 * translate the pairs of characters present in the first
 * string whenever the first of the pair appears in the second
 * string.
 */
void
translate(char *ttab, char *str)
{
	char *s;
	
	for(;*ttab && *(ttab+1); ttab += 2)
		for(s=str;*s;s++)
			if(*ttab == *s)
				*s = *(ttab+1);
}
