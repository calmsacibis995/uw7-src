#pragma comment(exestr, "@(#) ati.c 25.7 95/02/22 ")

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pwd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include "rce.h"
#include "uucp.h"

void logent(char *x, char *y){}
void assert(){}

#define MAXLINE		2048
#define PROBEBAUD 9600
#define DEBUG(l, f, s)	if (Debug >= l) fprintf(stderr, f, s)

char *strchr();
void alrmint(int);
void abortati(int);
char *vgets(unsigned char);
char * parsebuf(char *cmd, char *b);
struct baud *checkbaud(int n);
int setline(int *fd, struct baud *baud, char *acu);
void cleanup(int rc);

struct termios term;

int Debug;
int fd = -1;				/* file descriptor for acu	*/
int retcode = 0;			/* return code			*/
int high, low;
char *Bnptr;

/*
 * This table is used to convert user baudrates to 
 * their internal representation
 */
/*
 * At the moment we don't support these fast baud rates .... hack a little 
 */

struct baud {
	int rate;
	int value;
	int tout;	/* 100 ths per char */
} bauds[] = {
	1200,	B1200,	5,
	2400,	B2400,	2,
	4800,	B4800,	2,
	9600,	B9600,	2,
	19200,	B19200,	2,
	38400,	B38400,	2,
	57600,	B57600,	2,
	76800,	B76800,	2,
	115200, B115200,2,
	230400, B230400,2,
	460800,	B460800,2,
	921600,	B921600,2,
	0,	0,	0,
};

#define DEF_MD_SETUP "AT&F0X4Q0E1V1\r"

char *setup = DEF_MD_SETUP;

char *cmd;
int time_mul;	
char *acu; /* device to dial through	*/

main(int argc,char **argv)
{

	int errflag = 0;		/* set on errors		*/
	int timeout = 0;		/* how long to wait for alarm	*/
	extern int optind;
	extern char *optarg;
	int c;
	char *p;
	int i;
	struct baud *lowbaud = NULL, *highbaud = NULL, *probebaud = NULL;
	int atimax = 5;

	cmd = argv[0];

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
	signal(SIGTERM, SIG_DFL);

	/* Checkout the command line options */

	while ((c = getopt(argc, argv, "x:b:i:s:")) != EOF) {
		switch(c) {
		case 'b':
			probebaud = checkbaud(atoi(optarg));
			break;
		case 'x':
			Debug = atoi(optarg);
			break;
		case 'i':
			atimax = atoi(optarg);
			break;
		case 's':
			setup = optarg;
			break;
		case '?':
			errflag++;
			break;
		}
	}

	if (argc - optind != 1 || errflag) {
		fprintf(stderr,"Usage: %s [-x debug] [-b baud] devicename\n",
			cmd);
		exit(1);
	}

	if (Debug) {
		fprintf(stderr, "%s args ", cmd);
		for (c=0; c<argc; c++)  fprintf(stderr, ":%s", argv[c]);
		fprintf(stderr, "\n");
	}

	acu = argv[optind++];

	if (strchr(acu,'/') == NULL) {
		char buf[BUFSIZ];

		strcpy(buf,"/dev/");
		strcat(buf,acu);
		acu = strdup(buf);
		DEBUG(8,"opening %s\n",acu);
	}

	/* Try to lock the tty */

	if (mlock(LOCKPRE,acu)) {
		printf("Couldn't obtain lock for device %s\n", acu);
		exit(2);
	}

	/* Work out the available baud rates */

	if (!probebaud) {
		int workedonce = 0;
		
		highbaud = bauds;

		timeout = 15;

		for (i = 0; bauds[i].rate; i++) {
			signal(SIGINT, abortati);

			/* Must open with O_NDELAY set or the open may hang. */

			if ((fd = open(acu, O_RDWR | O_NDELAY)) < 0) {
				fprintf(stderr, "%s: Can't open device: %s\n",
					cmd, acu);
				delock(LOCKPRE,acu);
				exit(1);
			}

			if (setline(&fd, &bauds[i], acu)) {
				close(fd);
				continue;
			}

			signal(SIGALRM, alrmint);
			mdwrite(setup);
			if (mdread(bauds[i].tout, NULL) != 0) {
				if (workedonce) {
					close(fd);
					break;
				}
			} else {
				if (!lowbaud)
					lowbaud = &bauds[i];

				if (highbaud->rate < bauds[i].rate)
					highbaud = &bauds[i];

				workedonce = 1;
			}
			close(fd);
			nap(20);
		}

		if (!workedonce) {
			printf("Cannot access modem on any baud rate !\n");
			cleanup(1);
		}
		DEBUG(3, "%s: ", cmd);
		DEBUG(3, "available rates %d - ", lowbaud->rate);
		DEBUG(3, "%d\n", highbaud->rate);

		/* Since we aren't sure about flow control .... don't just
		 * use the hightest available baud rate ... use 9600 as
		 * a prefered value ... and if that is lower than the
		 * lowest supported .. use the lowest supported
		 */

		probebaud = lowbaud;
		if (probebaud->rate < PROBEBAUD && highbaud->rate > PROBEBAUD)
			probebaud = checkbaud(PROBEBAUD);
		DEBUG(6, "probe at %d\n", probebaud->rate);
	} 

	/*  Now set to highest baud rate */	

	signal(SIGINT, abortati);
	/* Must open with O_NDELAY set or the open may hang. */

	if ((fd = open(acu, O_RDWR | O_NDELAY)) < 0) {
		fprintf(stderr, "%s: Can't open device: %s\n", cmd, acu);
		delock(LOCKPRE,acu);
		exit(1);
	}

	if (setline(&fd, probebaud, acu)) {
		fprintf(stderr, "%s: Couldn't access modem at %d baud\n", cmd,
			probebaud->rate);
		close(fd);
		delock(LOCKPRE,acu);
		exit(1);

	}

	/* Timeout after 15 seconds if no response */

	timeout = 2;
	signal(SIGALRM, alrmint);

	/* Setup the modem */

	mdwrite(setup);
	if (mdread(timeout, NULL) != 0) {
		fprintf(stderr, "%s: Couldn't access modem at %d baud\n", cmd,
			probebaud->rate);
		cleanup(1);
	}

	timeout = 15;
	/* Do ATIx commands */
	if (highbaud == lowbaud) {
		printf("BAUD %d\n", probebaud->rate);
	} else {
		printf("BAUD %d-%d\n", lowbaud->rate, highbaud->rate);
	}
	doati(atimax, probebaud);
	cleanup(0);
}

/*
 * set line for no echo and correct speed.
 */
int
setline(int *fd, struct baud *baud, char *acu)
{
	int c, errflag;

	DEBUG(3, "%s: ", cmd);
	DEBUG(3, "setting line to %d baud\n", baud->rate);

	errflag = tcgetattr(*fd, &term);
	if (errflag) {
		char buf[16];
		DEBUG(1, "%s: ", cmd);
		DEBUG(1, "tcgetattr error on %s", acu);
		DEBUG(1, " errno=%d\n", errno);
		cleanup(1);
	}

	term.c_cflag &= ~(CBAUD | HUPCL );
	term.c_cflag |= CLOCAL;
	term.c_iflag &= ~(IXON | IXOFF | IXANY);
	term.c_lflag &= ~ECHO;
	term.c_cc[VMIN] = '\1';
	term.c_cc[VTIME] = '\0';

	tcsetspeed(TCS_ALL,&term,baud->rate);

	errflag = tcsetattr(*fd, TCSANOW, &term);
	if (errflag) {
		char buf[16];
		DEBUG(1, "%s: ", cmd);
		DEBUG(1, "tcsetattr error on %s", acu);
		DEBUG(1, " errno=%d\n", errno);
		cleanup(1);
	}

	/* Check the baud rate was set as requested */
	errflag = tcgetattr(*fd, &term);
	if (errflag) {
		char buf[16];
		DEBUG(1, "%s: ", cmd);
		DEBUG(1, "tcgetattr error on %s", acu);
		DEBUG(1, " errno=%d\n", errno);
		cleanup(1);
	}

	if (tcgetspeed(TCS_OUT,&term) != baud->rate) {
		term.c_cflag |= HUPCL;
		tcsetattr(*fd, TCSANOW, &term);
		return 1;
	}

	/*
	 *  Reopen line with clocal so we can talk without carrier present
	 */

	c = *fd;
	if ((*fd = open(acu, O_RDWR)) < 0) {
		fprintf(stderr, "%s: Can't open device local: %s\n", cmd, acu);
		exit(1);
	}
	close(c);

	/* Make sure the line is dropped if when we next close the modem */

	errflag = tcgetattr(*fd, &term);
	if (errflag) {
		char buf[16];
		DEBUG(1, "%s: ", cmd);
		DEBUG(1, "tcgetattr error on %s", acu);
		DEBUG(1, " errno=%d\n", errno);
		cleanup(1);
	}

	term.c_cflag |= HUPCL;
	errflag = tcsetattr(*fd, TCSANOW, &term);
	if (errflag) {
		char buf[16];
		DEBUG(1, "%s: ", cmd);
		DEBUG(1, "tcsetattr error on %s", acu);
		DEBUG(1, " errno=%d\n", errno);
		cleanup(1);
	}

	return 0;
}

/*
 * Perform requested ati commands
 */
doati(int atimax, struct baud *baud)
{
	int i, ret;
	char cmd[MAXLINE], *buf, *line;
	int try = 0;

	mdflush();
	for (i = 0; i < atimax; i++) {
		sprintf(cmd, "ATI%d", i);
 retry:
		try++;
		DEBUG(9, "nap for %d ms\n", 10 * baud->tout * try);
		nap(5 * baud->tout * try);
		mdwrite(cmd);
		mdwrite("\r");
		nap(5 * baud->tout);
		ret = mdread(baud->tout, &buf);

		if (ret != 0 && try < 3)
			goto retry;
			
		/* Parse returned buffer */
		if (ret == 0) {
			parsebuf(cmd, buf);
		} else {
			printf("%s Error\n", cmd);
		}

		mdflush();
	}

	sprintf(cmd, "AT+FCLASS=?");
	nap(5 * baud->tout * try);
	mdwrite(cmd);
	mdwrite("\r");

	ret = mdread(baud->tout, &buf);
	if (ret == 0) {
		parsebuf(cmd, buf);
	} else {
		printf("%s Error\n", cmd);
	}
	
}

/*
 * Parse the output from the modem ..
 */
#define skipwhite(b) { while(*b && (*b == '\r' || *b == '\n')) b++; }
char *
parsebuf(char *cmd, char *b)
{
	char *start, *end;

	skipwhite(b);
	start = b;
	while (*b && *b != '\r' && *b != '\n')
		*b++;

	*b++ = '\0';

	DEBUG(7, "First line = %s\n", start);


	while (*b) {
		skipwhite(b);
		start = b;
		while (*b && *b != '\r' && *b != '\n')
			*b++;

		*b++ = '\0';

		/* If the line reads OK ... then skip it */

		if (strcmp(start, "OK") == 0)
			continue;

		DEBUG(7, "line = %s\n", start);

		printf("%s %s\n", cmd, start);
	}

	return(start);
}

/*
 * Perform external to internal mappings of baud rates.
 */
struct baud *
checkbaud(int n)
{
        int baudrate, i;

	for (i = 0; bauds[i].rate ; i++)
		if (bauds[i].rate == n)
			return(&bauds[i]);

	fprintf(stderr, "%s: Bad speed: %d\n", cmd, n);
	exit(1);
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
char *expect[] = {
	"OK",
	"ERROR",
	0,
};

mdread(int rtime, char **bufp)
{
	char c,**mp;
	int index;
	register char *bp;
	static char buf[MAXLINE];

	bp = buf;
	if (bufp)
		*bufp = (char *)(&buf);
	alarm(rtime);
	DEBUG(9, "timeout in %d seconds\n", rtime);
	DEBUG(6, "MODEM returned %s", "<<");
	while (read(fd, &c, 1) == 1) {
		nap(10);
		c &= 0177;
		if ((*bp = c) != '\0')
			*++bp = '\0';
		DEBUG(6,"%s",vgets(c));
		if (bp >= buf + MAXLINE) {
			alarm(0);
			DEBUG(6,">>-%s\n","FAIL");
			return(-1);
		}
		if (c == '\r' || c =='\n' ) 
			for (index=0; expect[index]; index++){
			if (substr (expect[index], buf) == 0) {
				alarm(0);
				DEBUG(6,">>-%s\n","OK");
				DEBUG(4,"got %s\n", expect[index]);
				DEBUG(6,"returning %d\n", index);
				return(index);	
			}
		}
	}
	alarm(0);
	DEBUG(6,">>-%s","FAIL");
	DEBUG(4, " no response\n", 0);
	return(-1);
}

/*  mdflush()
 *
 *  Function:	Flushes input clists for modem
 */
mdflush()
{
	ioctl(fd, TCFLSH, 0) ;
}

/*
 *  mdwrite(c)
 *
 *  Function:	Outputs the string pointed to by c to the ACU device.
 *
 *  Returns:	0 on completion.
 *		-1 on write errors.
 *
 */
mdwrite(register char *c)
{
	int err;
	/*
	 *  Give modem a chance to recover before writing.
	 */
	nap(200);
	DEBUG(6, "\rSent MODEM %s", "<<");
	while (*c) {
		DEBUG(6, "%s", vgets(*c));
		if ((err = write(fd, c, 1)) != 1) {
			char buf[MAXLINE];
			DEBUG(6, ">>-%s\n", "FAIL");
			DEBUG(1, "ACU write error (errno=%d)\n", errno);
			return(-1);
		}
		c++;
		nap(10);
	}
	DEBUG(6, ">>-%s\n", "OK");
	return(0);
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
substr(s, l)
	register char *s;
	register char *l;
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
 *  Returns:	No return.  Exits with status 1
 */
void
alrmint(int sig)
{
	DEBUG(4, "alrmint: Timeout waiting for acu\n", 0);
}

void
abortati(int sig)
{
	cleanup(1);
}
/*
 * On cleanup signal, if the port is still open, close it ... then exit
 */
void
cleanup(int rc)
{
	signal(SIGINT, SIG_IGN);
	nap(500);

	if (fd != -1) {
		DEBUG(9, "hanging up", 0);
		close(fd);
	}
	delock(LOCKPRE,acu);
	exit(rc);
}

/*
 * vgets - Format one character in "always printable" format (like cat -v)
 *         This procedure works even though it looks like it wouldn't
 */
#define	toprint(x)	((x)<' '?((x)+'@'):'?')
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
			c = toprint(c);			/* Make it printable */
		}
	}
	*pnt++ = c;
	*pnt = '\0';
	return(buffer);
}

