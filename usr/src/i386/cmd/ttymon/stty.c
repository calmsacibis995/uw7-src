/*		copyright	"%c%" 	*/

#ident	"@(#)stty.c	1.3"

#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/termiox.h>
#include "stty.h"

extern char *getenv();
extern void exit();

static const char USAGE[] = ":1202:Usage: stty [-a|-g]\n"
				  "\t\t\tstty modes\n";
static int	pitt = 0;

static struct termios cb;
static struct termiox termiox;
static struct winsize winsize, owinsize;
static int term;
#ifdef MERGE386
long	sc_flag;
#endif

static void prmodes(), prcflag();

int
main(argc, argv)
char	*argv[];
{
	int i;
	char	*s_arg, *sttyparse();	/* s_arg: ptr to mode to be set */

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:stty");
	
	if ((term = get_ttymode(0, &cb, &termiox, &winsize)) < 0) {
		pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
		exit(2);
	}
	owinsize = winsize;
	if (argc == 1) {
		prmodes(0);
		exit(0);
	}
	if ((argc == 2) && (argv[1][0] == '-') && (argv[1][2] == '\0'))
	switch(argv[1][1]) {
		case 'a':
			prmodes(1);
			exit(0);
		case 'g':
			prencode();
			exit(0);
		default:
			(void) pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
			(void) pfmt(stderr, MM_ACTION, USAGE);
			exit(2);
	}
	if (s_arg = sttyparse(argc, argv, term, &cb, &termiox, &winsize)) {
		(void) pfmt(stderr, MM_ERROR, ":589:Unknown mode: %s\n", s_arg);
		exit(2);
	}

	if (set_ttymode(0, term, &cb, &termiox, &winsize, &owinsize) == -1) {
		pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
		exit(2);
	}
	exit(0);

	/*NOTREACHED*/
}

static void
prmodes(all)
{
	register m;
	speed_t ispeed, ospeed;

	ispeed = tcgetspeed(TCS_IN, &cb);
	ospeed = tcgetspeed(TCS_OUT, &cb);
	if (ispeed != ospeed) {
		prspeed("ispeed ", ispeed);
		prspeed("ospeed ", ospeed);
	} else
		prspeed("speed ", ospeed);

	if (!all) {
		if (m&PARENB) {
			if (m&PAREXT) {
				if (m&PARODD)
					(void) printf("markp ");
				else
					(void) printf("spacep ");
			} else {
				if (m&PARODD)
					(void) printf("oddp ");
				else
					(void) printf("evenp ");
			}
		} else
			(void) printf("-parity ");
		prcflag(cb.c_cflag, 0);
	}

	(void) printf("\n");

	if (term & WINDOW) {
		(void)printf("rows = %d; columns = %d;",
				winsize.ws_row, winsize.ws_col);
		(void)printf(" ypixels = %d; xpixels = %d;\n",
				winsize.ws_ypixel, winsize.ws_xpixel);
	}

	if ((cb.c_lflag&ICANON) == 0)
		(void) printf("min = %d; time = %d;\n",
				cb.c_cc[VMIN], cb.c_cc[VTIME]);
	if (all || cb.c_cc[VINTR] != CINTR)
		pit(cb.c_cc[VINTR], "intr", "; ");
	if (all || cb.c_cc[VQUIT] != CQUIT)
		pit(cb.c_cc[VQUIT], "quit", "; ");
	if (all || cb.c_cc[VERASE] != CERASE)
		pit(cb.c_cc[VERASE], "erase", "; ");
	if (all || cb.c_cc[VKILL] != CKILL)
		pit(cb.c_cc[VKILL], "kill", "; ");
	if (all || cb.c_cc[VEOF] != CEOF)
		pit(cb.c_cc[VEOF], "eof", "; ");
	if (all || cb.c_cc[VEOL] != CNUL)
		pit(cb.c_cc[VEOL], "eol", "; ");
	if (all || cb.c_cc[VEOL2] != CNUL)
		pit(cb.c_cc[VEOL2], "eol2", "; ");
	if (all || cb.c_cc[VSWTCH] != CSWTCH)
		pit(cb.c_cc[VSWTCH], "swtch", "; ");
	if (all || cb.c_cc[VSTART] != CSTART)
		pit(cb.c_cc[VSTART], "start", "; ");
	if (all || cb.c_cc[VSTOP] != CSTOP)
		pit(cb.c_cc[VSTOP], "stop", "; ");
	if (all || cb.c_cc[VSUSP] != CSUSP)
		pit(cb.c_cc[VSUSP], "susp", "; ");
	if (all || cb.c_cc[VDSUSP] != CDSUSP)
		pit(cb.c_cc[VDSUSP], "dsusp", "; ");
	if (all || cb.c_cc[VREPRINT] != CRPRNT)
		pit(cb.c_cc[VREPRINT], "rprnt", "; ");
	if (all || cb.c_cc[VDISCARD] != CFLUSH)
		pit(cb.c_cc[VDISCARD], "flush", "; ");
	if (all || cb.c_cc[VWERASE] != CWERASE)
		pit(cb.c_cc[VWERASE], "werase", "; ");
	if (all || cb.c_cc[VLNEXT] != CLNEXT)
		pit(cb.c_cc[VLNEXT], "lnext", "; ");
	if (pitt) (void) printf("\n");

	if (all) {
		m = cb.c_cflag;
		(void) printf("-parenb "+((m&PARENB)!=0));
		(void) printf("-parodd "+((m&PARODD)!=0));
		(void) printf("-parext "+((m&PAREXT)!=0));
		prcflag(m, 1);
		(void) printf("\n");
	}

	m = cb.c_iflag;
	if (all || (m&IGNBRK))
		(void) printf("-ignbrk "+((m&IGNBRK)!=0));
	if (all || (!(m&IGNBRK) && (m&BRKINT)))
		(void) printf("-brkint "+((m&BRKINT)!=0));
	if (all || !(m&INPCK))
		(void) printf("-inpck "+((m&INPCK)!=0));
	if (all || ((m&INPCK) && (m&IGNPAR)))
		(void) printf("-ignpar "+((m&IGNPAR)!=0));
	if (all || (m&PARMRK))
		(void) printf("-parmrk "+((m&PARMRK)!=0));
	if (all || !(m&ISTRIP))
		(void) printf("-istrip "+((m&ISTRIP)!=0));
	if (all || (m&INLCR))
		(void) printf("-inlcr "+((m&INLCR)!=0));
	if (all || (m&IGNCR))
		(void) printf("-igncr "+((m&IGNCR)!=0));
	if (all || (m&ICRNL))
		(void) printf("-icrnl "+((m&ICRNL)!=0));
	if (all || (m&IUCLC))
		(void) printf("-iuclc "+((m&IUCLC)!=0));
	if (all)
		(void) printf("\n");
	if (all || !(m&IXON))
		(void) printf("-ixon "+((m&IXON)!=0));
	if (all || ((m&IXON) && !(m&IXANY)))
		(void) printf("-ixany "+((m&IXANY)!=0));
	if (all || (m&IXOFF))
		(void) printf("-ixoff "+((m&IXOFF)!=0));
	if (all || (m&IMAXBEL))
		(void) printf("-imaxbel "+((m&IMAXBEL)!=0));
	if (all)
		(void) printf("\n");

	m = cb.c_oflag;
	if (all || !(m&OPOST))
		(void) printf("-opost "+((m&OPOST)!=0));
	if (all || (m&OPOST)) {
		if (all || (m&OLCUC))
			(void) printf("-olcuc "+((m&OLCUC)!=0));
		if (all || (m&ONLCR))
			(void) printf("-onlcr "+((m&ONLCR)!=0));
		if (all || (m&OCRNL))
			(void) printf("-ocrnl "+((m&OCRNL)!=0));
		if (all || (m&ONOCR))
			(void) printf("-onocr "+((m&ONOCR)!=0));
		if (all || (m&ONLRET))
			(void) printf("-onlret "+((m&ONLRET)!=0));
		if (all) {
			(void) printf("-ofill "+((m&OFILL)!=0));
			(void) printf("-ofdel "+((m&OFDEL)!=0));
		} else if (m&OFILL)
			(void) printf((m&OFDEL) ? "del-fill " : "nul-fill ");
		delay((m&CRDLY)/CR1, "cr");
		delay((m&NLDLY)/NL1, "nl");
		delay((m&TABDLY)/TAB1, "tab");
		delay((m&BSDLY)/BS1, "bs");
		delay((m&VTDLY)/VT1, "vt");
		delay((m&FFDLY)/FF1, "ff");
	}
	(void) printf("\n");

	m = cb.c_lflag;
	if (all || !(m&ISIG))
		(void) printf("-isig "+((m&ISIG)!=0));
	if (all || !(m&ICANON))
		(void) printf("-icanon "+((m&ICANON)!=0));
	if (all || (m&XCASE))
		(void) printf("-xcase "+((m&XCASE)!=0));
	(void) printf("-echo "+((m&ECHO)!=0));
	(void) printf("-echoe "+((m&ECHOE)!=0));
	(void) printf("-echok "+((m&ECHOK)!=0));
	if (all || (m&ECHONL))
		(void) printf("-echonl "+((m&ECHONL)!=0));
	if (all || (m&NOFLSH))
		(void) printf("-noflsh "+((m&NOFLSH)!=0));
	if (all)
		(void) printf("\n");
	if (all || (m&TOSTOP))
		(void) printf("-tostop "+((m&TOSTOP)!=0));
	if (all || (m&ECHOCTL))
		(void) printf("-echoctl "+((m&ECHOCTL)!=0));
	if (all || (m&ECHOPRT))
		(void) printf("-echoprt "+((m&ECHOPRT)!=0));
	if (all || (m&ECHOKE))
		(void) printf("-echoke "+((m&ECHOKE)!=0));
	if (all || (m&DEFECHO))
		(void) printf("-defecho "+((m&DEFECHO)!=0));
	if (all || (m&FLUSHO))
		(void) printf("-flusho "+((m&FLUSHO)!=0));
	if (all || (m&PENDIN))
		(void) printf("-pendin "+((m&PENDIN)!=0));
	if (all || (m&IEXTEN))
		(void) printf("-iexten "+((m&IEXTEN)!=0));
	(void) printf("\n");

	if (term & FLOW) {
		m = termiox.x_hflag;
		if (all || (m & RTSXOFF))
			(void) printf("-rtsxoff "+((m&RTSXOFF)!=0));
		if (all || (m & CTSXON))
			(void) printf("-ctsxon "+((m&CTSXON)!=0));
		if (all || (m & DTRXOFF))
			(void) printf("-dtrxoff "+((m&DTRXOFF)!=0));
		if (all || (m & CDXON))
			(void) printf("-cdxon "+((m&CDXON)!=0));
		if (all || (m & ISXOFF))
			(void) printf("-isxoff "+((m&ISXOFF)!=0));

		m = termiox.x_cflag;

		switch (m & XMTCLK) {
			case XCIBRG: (void)printf("xcibrg ");
				     break;
			case XCTSET: (void)printf("xctset ");
				     break;
			case XCRSET: (void)printf("xcrset ");
		}
		
		switch (m & RCVCLK) {
			case RCIBRG: (void)printf("rcibrg ");
				     break;
			case RCTSET: (void)printf("rctset ");
				     break;
			case RCRSET: (void)printf("rcrset ");
		}
		
		switch (m & TSETCLK) {
			case TSETCOFF: (void)printf("tsetcoff ");
				     break;
			case TSETCRBRG: (void)printf("tsetcrbrg ");
				     break;
			case TSETCTBRG: (void)printf("tsetctbrg ");
				     break;
			case TSETCTSET: (void)printf("tsetctset ");
				     break;
			case TSETCRSET: (void)printf("tsetcrset ");
		}
		
		switch (m & RSETCLK) {
			case RSETCOFF: (void)printf("rsetcoff ");
				     break;
			case RSETCRBRG: (void)printf("rsetcrbrg ");
				     break;
			case RSETCTBRG: (void)printf("rsetctbrg ");
				     break;
			case RSETCTSET: (void)printf("rsetctset ");
				     break;
			case RSETCRSET: (void)printf("rsetcrset ");
		}
	
		(void) printf("\n");
	}

#ifdef MERGE386
	if (all || (sc_flag & KB_ISSCANCODE)) {
		(void) printf("-isscancode "+((sc_flag&KB_ISSCANCODE)!=0));
		if (all || (sc_flag & KB_XSCANCODE))
			(void) printf("-xscancode"+((sc_flag&KB_XSCANCODE)!=0));
		(void) printf("\n");
	}
#endif
}

static void
prcflag(m, all)
{
	if (all || ((m&PARENB) && !(m&CS7)) || (!(m&PARENB) && !(m&CS8)))
		(void) printf("cs%c ", '5'+(m&CSIZE)/CS6);
	if (all || (m&CSTOPB))
		(void) printf("-cstopb "+((m&CSTOPB)!=0));
	if (all || (m&HUPCL))
		(void) printf("-hupcl "+((m&HUPCL)!=0));
	if (all || !(m&CREAD))
		(void) printf("-cread "+((m&CREAD)!=0));
	if (all || (m&CLOCAL))
		(void) printf("-clocal "+((m&CLOCAL)!=0));
	if (all || (m&LOBLK))
		(void) printf("-loblk "+((m&LOBLK)!=0));
}

pit(what, itsname, sep)		/* print function for prmodes() */
unsigned char what;
char *itsname, *sep;
{

if (pitt && pitt % 4 == 0)
	(void) printf("\n");
pitt++;
(void) printf("%s", itsname);
if (what == _POSIX_VDISABLE) {
	(void) printf(" = <undef>%s", sep);
	return;
}
(void) printf(" = ");
if (what & 0200 && !isprint(what)) {
	(void) printf("-");
	what &= ~ 0200;
}
if (what == CINTR) {
	(void) printf("DEL%s", sep);
	return;
} else if (what < ' ') {
	(void) printf("^");
	what += '`';
}
(void) printf("%c%s", what, sep);
}

delay(m, s)
char *s;
{
if(m)
	(void) printf("%s%d ", s, m);
}

prspeed(c, s)
char *c;
speed_t s;
{
	(void) printf("%s%ld baud; ", c, s);
}

				/* print current settings for use with  */
prencode()				/* another stty cmd, used for -g option */
{
int i, last;
(void) printf("%x:%x:%x:%x:",cb.c_iflag,cb.c_oflag,cb.c_cflag,cb.c_lflag);
/* last control slot is unused */
last = NCCS - 2;

for(i = 0; i < last; i++)
	(void)printf("%x:", cb.c_cc[i]);

#ifdef MERGE386
(void)printf("%x:%x\n", cb.c_cc[last],sc_flag);
#else
(void)printf("%x\n", cb.c_cc[last]);
#endif

}
