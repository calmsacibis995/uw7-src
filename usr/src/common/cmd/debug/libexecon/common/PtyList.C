#ident	"@(#)debugger:libexecon/common/PtyList.C	1.10"

// Pseudo-Terminal records
// Contains the functions that are responsible for redirecting
// the I/O from subject processes created by the debugger.
// Programs created by the debugger may do I/O 
// through a pseudo-terminal.

#include "PtyList.h"
#include "Program.h"
#include "Interface.h"
#include "global.h"
#include "Proctypes.h"
#include "Proglist.h"
#include "ProcObj.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stropts.h>
#include <termio.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/ptms.h>

PtyInfo		*first_pty = 0;

PtyInfo::~PtyInfo()
{
	delete _name;
	if (first_pty == this)
		first_pty = (PtyInfo *) next();
	close(pty);
	unlink();
}

// set up the debugger side of an I/O channel.
// Grabs the master side of a pty and opens it, setting up appropriate
// magic.

PtyInfo::PtyInfo()
{
	int		ptyflags;
	const char	pname[] = "pts";

	pty = open("/dev/ptmx", O_RDWR);

	if (pty < 0)
	{
		pty = -1;
		printe(ERR_sys_pty_setup, E_ERROR, strerror(errno));
		return;
	}
	sigset(SIGCLD, SIG_DFL);
	if (grantpt(pty) == -1)
	{
		close(pty);
		pty = -1;
		printe(ERR_sys_pty_setup, E_ERROR, strerror(errno));
		return;
	}
	sigignore(SIGCLD);
	if (unlockpt(pty) == -1)
	{
		close(pty);
		pty = -1;
		printe(ERR_sys_pty_setup, E_ERROR, strerror(errno));
		return;
	}

	// set it up to hit us with SIGPOLL
	if ((ioctl(pty, I_SETSIG, (void *) (S_INPUT|S_HIPRI)) == -1)
		|| ((ptyflags = fcntl(pty, F_GETFL, 0)) == -1) )
	{
		printe(ERR_sys_pty_setup, E_ERROR, strerror(errno));
		close(pty);
		pty = -1;
		return;
	}

	// set up master so read returns -1 with EAGAIN if no data
	// available
	ptyflags |= O_NDELAY;
	if (fcntl(pty, F_SETFL, ptyflags) == -1) 
	{
		printe(ERR_sys_pty_setup, E_ERROR, strerror(errno));
		close(pty);
		pty = -1;
		return;
	}
	if (first_pty)
		prepend(first_pty);
	first_pty = this;
	count = 1;

	char *tmp = ptsname(pty);
	char *slashpos;

	if (slashpos = strrchr(tmp, '/'))
		tmp = slashpos + 1;

	_name = new char[strlen(tmp) + sizeof(pname)];
	if (strncmp(tmp, pname, sizeof(pname)-1) == 0)
	{
		strcpy(_name, tmp);
	}
	else
	{
		strcpy(_name, "pts");
		strcat(_name, tmp);
	}
}

void
redirect_childio(int fd)
{
	// To redirect the subject process IO and cause it to
	// properly get characters and interrupts we need to arrange
	// for the PTY to be its controlling terminal.
	// Executed by the child process

	struct	termio	modes;

	close(0);
	if (open(ptsname(fd), O_RDWR) < 0 ||
		ioctl(0, I_PUSH, "ptem") < 0 ||
		ioctl(0, I_PUSH, "ldterm") < 0)
		exit(1);

	// set up pseudo-term so it doesn't map 
	// NL to CR-NL on output and doesn't
	// map NL to CR on input

	if (ioctl(0, TCGETA, &modes) < 0)
		exit(1);
	
	modes.c_oflag &= ~ONLCR;
	modes.c_iflag &= ~INLCR;
	modes.c_lflag &= ~ECHO;

	if (ioctl(0, TCSETAW, &modes) < 0)
		exit(1);
	
	close(1);
	close(2);

	dup(0);		// stdout
	dup(0);		// stderr
}

// stop all processes after IO has been interrupted
static void
stop_all_procs()
{
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head;

	list_head = list = proglist.all_live(0);

	// allow stop directives to be interrupted in case
	// the stop never returns due to some deadlock
	// assumes the calling routine has released SIGINT
	// and will reblock it
	for(pobj = list++->p_pobj; pobj; pobj = list++->p_pobj)
	{
		pobj->stop();
	}
	prdelset(&interrupt, SIGINT);
	waitlist.clear();
	proglist.free_plist(list_head);
}

// Handler for SIGPOLL
// SIGPOLL, SIGINT and SIG_INFORM are blocked in the handler
void
FieldProcessIO(int sig)
{
	char	buf[BUFSIZ+1];
	int	go_on;
	int	io_interrupted = 0;

	for (PtyInfo *pty = first_pty; pty; pty = (PtyInfo *) pty->next()) 
	{
		int count;

		go_on = 1;
		while (go_on)
		{
			sigrelse(SIGINT);
			if (prismember(&interrupt, SIGINT))
			{
				prdelset(&interrupt, SIGINT);
				io_interrupted = 1;
				stop_all_procs();
			}
			sighold(SIGINT);
			count = read(pty->pt_fd(), buf, BUFSIZ);
			if (count < BUFSIZ)
				go_on = 0;

			if ((count > 0) && !io_interrupted)
			{
				char	*p = buf, *q;
				buf[count] = 0;
				while(count)
				{
					q = (char *)memchr(p, '\n',
						count);
					if (q)
					{
						char	save_ch;

						// print up through new-line
						save_ch = *++q;
						*q = '\0';
						printm(MSG_proc_output,
							pty->name(), p);
						*q = save_ch;

						count -= q - p;
						p = q;
					}
					else
					{
						printm(MSG_proc_output,
							pty->name(), p);
						break;
					}
				}
			}
			else if (count == -1 && errno != EAGAIN) 
			{
				printe(ERR_sys_pty_read, E_ERROR, 
					pty->name(), strerror(errno));
			}
			else
				break;
		}
	}
	praddset(&interrupt, sig);
	// restore original
}
