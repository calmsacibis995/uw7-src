#ident	"@(#)findvt.c	2.1"

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

static char *   get_vtname();
static void     set_ttymode();
static void     get_ttymode();
static void     do_setttymode();

main(argc, argv)
int argc;
char *argv[];
{
        int     fd, option, wantvt = -1, errcnt = 0;
        long    vtno;
        char    *comm, *bname, *av0, prompt[11];
        char    *name;
        char    vtpref[VTNAMESZ], vtname[VTNAMESZ];
        int     ttype;
        struct  vt_stat vtinfo;

        comm = (char *)NULL;
        bname = (char *)NULL;
        av0 = argv[0];

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
        if (ttype == 0x6B64)                            /* "kd" */
                strcpy(vtpref, "/dev/");
        else if (((ttype >> 8) & 0xFF) == 0x73)         /* SunRiver */
                sprintf(vtpref, "/dev/s%d", (ttype & 0xFF));

/* >>>>>>>> ioctl(fd, VT_OPENQRY, &vtno); */
	ioctl(fd, VT_OPENQRY, &vtno);
         if (vtno < 0) {
                  fprintf(stderr, "No vts available\n");
                 exit(1);
         }

       /* sprintf(vtname, "%svt%02ld", vtpref, vtno); */
       printf("%svt%02ld\n", vtpref, vtno);
        close(fd);
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
