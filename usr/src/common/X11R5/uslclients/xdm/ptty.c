/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xdm:ptty.c	1.2"

#include <sys/byteorder.h>
#include <termio.h>
#include <stropts.h>
#include <sys/stream.h>
#include <fcntl.h>
#include <utmpx.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <rx.h>


/* system calls */

extern	int	close();
extern	int	ioctl();
extern	int	getmsg();


/* externally defined routines */

extern	int	grantpt();
extern	int	unlockpt();
extern	char	*ptsname();
extern	char	*strncpy();



/* externally defined global variables */

int	Termios_received = 0; /* received termios structure flag */
struct termios	Termios;  /* termios buffer */

#define	RX_SLAVENAMELEN	32


/* locally defined global variables */

int	Ptty_open;	/* ptty open flag */
int	Ptty_fd;	/* master ptty fd */
int	Slaveptty_fd;	/* slave ptty fd */
char	Slaveptty_name[RX_SLAVENAMELEN]; /* slave ptty device file name */


/*
 * makepttypair()
 *
 * This function creates and opens a master/slave pair of pseudo ttys.
 * It returns 0 for success, -1 for failure
 *
 */

int
makepttypair()
{
	char	*ttyname;		/* file name of slave pseudo tty */
	pid_t	mypid = getpid();	/* my process id */
	struct passwd *pwp;		/* password file entry */

	Debug("makettypair\n");
	if ((Ptty_fd = open("/dev/ptmx", O_RDWR)) == -1) {
		Debug ("ptty: open ptmx failed, errno = %d\n", errno);
		return(-1);
	}
	if (grantpt(Ptty_fd) == -1) {
		Debug ("ptty: grantpt failed\n");
		return(-1);
	}
	if (unlockpt(Ptty_fd) == -1) {
		Debug ("ptty: unlockpt failed\n");
		return(-1);
	}
	if ((ttyname = ptsname(Ptty_fd)) == NULL) {
		Debug ("ptty: ptsname failed\n");
		return(-1);
	}
	(void) strncpy(Slaveptty_name, ttyname, RX_SLAVENAMELEN);
	if ((Slaveptty_fd = open(Slaveptty_name, O_RDWR)) == -1) {
		Debug ("ptty: could not open pts %s, errno = %d\n",
			 Slaveptty_name, errno);
		return(-1);
	}

	if (ioctl(Slaveptty_fd, I_PUSH, "ptem") == -1) {
		Debug ("ptty: push ptem failed\n");
		return(-1);
	}
	if (ioctl(Slaveptty_fd, I_PUSH, "ldterm") == -1) {
		Debug ("ptty: push ldterm failed\n");
		return(-1);
	}

	/*
	 *	Note that since the following ioctl() is performed
	 *	before the pckt module is pushed, it will not be sent back
	 *	to the client.  This is what we want.
	 *
	 */

	if (Termios_received)
		if (ioctl(Slaveptty_fd, TCSETS, &Termios) == -1) {
			Debug ("ptty: TCSETS failed\n");
			return(-1);
		}

	if (ioctl(Ptty_fd, I_PUSH, "pckt") == -1) {
		Debug ("ptty: push pckt failed\n");
		return(-1);
	}

	/* disable ldterm input processing on server end */
	if (ioctl(Ptty_fd, TIOCREMOTE, 1) == -1) {
		Debug ("ptty: ioctl(TIOCREMOTE) failed\n");
		return(-1);
	}

	Ptty_open = 1;

	Debug ("ptty: pttys created\n");

	return(0);
}
