#ident	"@(#)newvt.c	1.11"

/*
 *      Copyright (C) The Santa Cruz Operation, 1992-1995.
 *      Portions Copyright (C) Microsoft Corporation, 1984-1988.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation, and Microsoft Corporation
 *      and should be treated as Confidential.
 */

/*
 *	L000	5Feb97		rodneyh@sco.com
 *      - Changes for multiconsole support.
 *	  Change KIOCINFO handling to check for new MC return value, and make
 *	  new KDVTPATH ioctl
 *	L001	15Apr97		rodneyh@sco.com
 *	- Removed extraneous call to KDGETVTPATH left in by L000 (unmarked)
 *
 */ 

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <sys/vt.h>
#include <sys/termio.h>
#include <sys/termios.h>
#include <sys/stermio.h>
#include <sys/termiox.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>

extern int errno;

static char *	get_vtname();
static void	set_ttymode();
static void	get_ttymode();
static void	do_setttymode();

main(argc, argv)
int argc;
char *argv[];
{
	int	fd, option, wantvt = -1, errcnt = 0;
	long	vtno;
	char	*comm, *bname, *av0, prompt[11];
	char	*name;
	char	vtpref[VTNAMESZ], vtname[VTNAMESZ];
	char	vtpath[VTNAMESZ-5];			/* L000 */
	int	ttype;
	struct	vt_stat vtinfo;
	
	comm = (char *)NULL;
	bname = (char *)NULL;
	av0 = argv[0];
	while ((option = getopt(argc, argv, "e:n:")) != EOF) {
		switch (option) {
		case 'e':
			if (comm != (char *)NULL) {
				fprintf(stderr, "Multiple use of -e\n");
				errcnt++;
			}
			comm = optarg;
			break;
		case 'n':
			if (wantvt != -1) {
				fprintf(stderr, "Multiple use of -n\n");
				errcnt++;
			}
			wantvt = atoi(optarg);
			break;
		default:
			errcnt++;
			break;
		}
	}
	argc += optind;
	argv += optind;
	if (errcnt) {
		fprintf(stderr, "usage: newvt [-e command] [-n vt number]\n");
		exit(errcnt);
	}
	if ((fd = open("/dev/tty", O_RDWR)) == -1) {
		fprintf(stderr, "%s: unable to open /dev/tty\n", av0);
		exit(1);
	}
	while ((ttype = ioctl(fd, KIOCINFO, 0)) == -1) {
		if ((name = get_vtname(fd)) == NULL)
			break;
		close(fd);
		if ((fd = open(name, O_RDONLY)) == -1)
			break;
	}
	if (ttype == -1) {
		fprintf(stderr, "cannot execute %s from here\n", av0);
		exit(1);
	}

	if (ttype == 0x6B64)				/* "kd" */
		strcpy(vtpref, "/dev/");
	else if (ttype == 0x4D43){		/* Multiconsole, L000 begin */
		/*
		 * make the new KDVTPATH ioctl
		 */
		if(ioctl(fd, KDVTPATH, vtpath) == -1){
			perror("Can't get VT path");
			exit(1);
		}
		strcpy(vtpref, "/dev/");
		strcat(vtpref, vtpath);

	}	/* End if ttype == 0x4D43, L000 end */

	if (wantvt < 0) {
		ioctl(fd, VT_OPENQRY, &vtno);
		if (vtno < 0) {
			fprintf(stderr, "No vts available\n");
			exit(1);
		}
	} else {
		vtno = wantvt;
		ioctl(fd, VT_GETSTATE, &vtinfo);
		if (vtinfo.v_state & (1 << vtno)) {
			fprintf(stderr, "%svt%02ld is not available\n", vtpref, vtno);
			exit(1);
		}
	}
	sprintf(vtname, "%svt%02ld", vtpref, vtno);
	close(fd);
	close(2);
	close(1);
	close(0);
	if (fork())
		exit(0);	/* parent */
	putenv("TERM=AT386");
	setpgrp();   
	if (open(vtname, O_RDWR) == -1)
		exit(1);
	dup(0);
	dup(0);
	set_ttymode();
	sprintf(prompt,"PS1=VT %ld> ", vtno);
	putenv(prompt);
	fputs("\033c", stdout);
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	if (ioctl(0, VT_ACTIVATE, vtno) == -1)
        {
     		fprintf(stderr, "VT_ACTIVATE failed");
        }
        else
        {
     		if (ioctl(0, VT_WAITACTIVE, 0) == -1)
            		fprintf(stderr, "VT_WAITACTIVE failed");
        }

	if (comm != (char *)NULL) {
		system(comm);
	} else if ((comm = (char *)getenv("SHELL")) != (char *)NULL) {
		if ((bname = strrchr(comm, '/')) == (char *) NULL)
			bname = comm;
		else	/* skip past '/' */
			bname++;
		if (execl(comm, bname, 0) == -1)
			fprintf(stderr, "exec of %s failed\n", comm);
	} else if (execl("/bin/sh", "sh", 0) == -1)
		fprintf(stderr, "exec of /bin/sh failed\n");
	sleep(5);
	exit(1);

	/* NOTREACHED */
}

/*
 * This routine exists to permit running newvt from xterm or xdm.
 * Attempt to determine what vt (if any) we are talking to.  Return
 * a pointer to the device pathname (such as "/dev/vt01"), or NULL
 * on failure (not running under X, display is not local, etc.).
 *
 * The pathname is in static storage and will be over-written
 * by the next call.
 */

#define DEV	"/dev/"
#define DEVSZ	(sizeof DEV - 1)
#define UNIX	"unix"
#define UNIXSZ	(sizeof UNIX - 1)

static char *
get_vtname(fd)
	int	fd;
{
	static	char name[DEVSZ + VTNAMESZ];
	struct	utsname utsbuf;
	FILE	*fp;
	char	*p, *q;
	size_t	len;

	/*
	 * If we're talking to a local xterm with the consem module
	 * pushed on the stream, a TIOCVTNAME ioctl will succeed and
	 * will give us the device name, e.g. "vt01".
	 * The current version of xterm can respond incorrectly to
	 * the TIOCVTNAME ioctl, giving a garbage string, so we check
	 * to make sure that the putative device actually exists.
	 * If not, try the hard way.
	 */

	strcpy(name, DEV);

	if (ioctl(fd, TIOCVTNAME, name + DEVSZ) >= 0 &&
		access(name, F_OK) == 0) {
#ifdef DEBUG
		fprintf(stderr, "TIOCVTNAME gives us \"%s\"\n", name + DEVSZ);
#endif
		return name;
	}

	/*
	 * We may be running under a local xterm without the consem
	 * module pushed, or we may have been invoked directly from xdm.
	 * If there is no $DISPLAY in the environment, or it does not
	 * refer to the local machine, we're out of luck.
	 * We consider it local if it is of the form ":*", "unix:*"
	 * or "<uname>:*", where <uname> is the name of this machine.
	 * This code was adapted from xterm:charproc.c.
	 *
	 * Note that the format of the $DISPLAY string may be extended
	 * in X11R6, with possible implications for this code.
	 */

	if ((p = getenv("DISPLAY")) == NULL || (q = strchr(p, ':')) == NULL)
		return NULL;

	if ((len = q - p) == 0)
		/* EMPTY */ ;
	else if (len == UNIXSZ && strncmp(p, UNIX, UNIXSZ) == 0)
		/* EMPTY */ ;
	else if (uname(&utsbuf) >= 0 &&
		len == strlen(utsbuf.nodename) &&
		strncmp(p, utsbuf.nodename, len) == 0)
		/* EMPTY */ ;
	else
		return NULL;

	/*
	 * $DISPLAY is in the environment and refers to this machine.
	 * Use xdpydev (which queries the X server) to determine what
	 * VT device the server is talking to.
	 */

#ifdef DEBUG
	fprintf(stderr, "trying xdpydev... ");
#endif
	if ((fp = popen("/usr/X/bin/xdpydev 2>/dev/null", "r")) == NULL ||
		fgets(name, sizeof name, fp) == NULL ||
		pclose(fp) != 0 ||
		(len = strlen(name)) <= 1 ||
		name[len - 1] != '\n') {
#ifdef DEBUG
		fprintf(stderr, "no luck\n");
#endif
		return NULL;
	}

	name[len - 1] = '\0';
#ifdef DEBUG
	fprintf(stderr, "got \"%s\"\n", name);
#endif
	return name;
}

static int term = 0;
#define	ASYNC	1
#define	TERMIOS	2

static void
set_ttymode()
{
	struct termio termio;
	struct termios termios;
	struct stio stermio;

	get_ttymode(&termio, &termios, &stermio);
	do_setttymode(&termio, &termios, &stermio);
}

static void
get_ttymode(termio, termios, stermio)
struct termio *termio;
struct termios *termios;
struct stio *stermio;
{
	int i, fd = 0;

	if(ioctl(fd, STGET, stermio) == -1) {
		term |= ASYNC;
		if(ioctl(fd, TCGETS, termios) == -1) {
			if(ioctl(fd, TCGETA, termio) == -1) {
				return;
			}
			termios->c_lflag = termio->c_lflag;
			termios->c_oflag = termio->c_oflag;
			termios->c_iflag = termio->c_iflag;
			termios->c_cflag = termio->c_cflag;
			for(i = 0; i < NCC; i++)
				termios->c_cc[i] = termio->c_cc[i];
		} else
			term |= TERMIOS;
	}
	else {
		termios->c_cc[7] = (unsigned)stermio->tab;
		termios->c_lflag = stermio->lmode;
		termios->c_oflag = stermio->omode;
		termios->c_iflag = stermio->imode;
	}

	termios->c_cc[VERASE] = '\b';
	termios->c_cc[VINTR] = CINTR;
	termios->c_cc[VMIN] = 1;
	termios->c_cc[VTIME] = 1;
	termios->c_cc[VEOF] = CEOF;
	termios->c_cc[VEOL] = CNUL;
	termios->c_cc[VKILL] = CKILL;
	termios->c_cc[VQUIT] = CQUIT;
	
	termios->c_cflag &= ~(CSIZE|PARODD|CLOCAL);
	termios->c_cflag |= (CS7|PARENB|CREAD);
	
	termios->c_iflag &= ~(IGNBRK|PARMRK|INPCK|INLCR|IGNCR|IUCLC|IXOFF);
	termios->c_iflag |= (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON|IXANY);

	termios->c_lflag &= ~(XCASE|ECHONL|NOFLSH|STFLUSH|STWRAP|STAPPL);
	termios->c_lflag |= (ISIG|ICANON|ECHO|ECHOE|ECHOK);
	
	termios->c_oflag &= ~(OLCUC|OCRNL|ONOCR|ONLRET|OFILL|OFDEL|
				NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY);
	termios->c_oflag |= (TAB3|OPOST|ONLCR);
}

static void
do_setttymode(termio, termios, stermio)
struct termio *termio;
struct termios *termios;
struct stio *stermio;
{
	int i, fd = 0;

	if (term & ASYNC) {
		if(term & TERMIOS)
			ioctl(fd, TCSETSW, termios);
		else {
			termio->c_lflag = termios->c_lflag;
			termio->c_oflag = termios->c_oflag;
			termio->c_iflag = termios->c_iflag;
			termio->c_cflag = termios->c_cflag;
			for(i = 0; i < NCC; i++)
				termio->c_cc[i] = termios->c_cc[i];
			ioctl(fd, TCSETAW, termio);
		}
	} else {
		stermio->imode = termios->c_iflag;
		stermio->omode = termios->c_oflag;
		stermio->lmode = termios->c_lflag;
		stermio->tab = termios->c_cc[7];
		ioctl(fd, STSET, stermio);
	}
}
