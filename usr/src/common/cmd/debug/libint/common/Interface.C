#ident	"@(#)debugger:libint/common/Interface.C	1.20"

// Functions the debug engine calls to start and stop the GUI

#include	<errno.h>
#include	<string.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<signal.h>
#include	<fcntl.h>

#include	"Parser.h"
#include	"utility.h"
#include	"global.h"
#include	"Interface.h"
#include	"Transport.h"
#include	"UIutil.h"
#include	"utility.h"
#include	"Machine.h"

#include	"GManager.h"
#include	"libint.h"

// define the Interface type
Transport	*transport;
int		processing_query;

// PrintaxGenNL and PrintaxSpeakCount are used by the command line
// interface to determine when to put out an extra new-line.
// (They aren't used by the GUI).  The extra new-line is needed
// before an event notification to make sure the asynchrounous
// notification is clearly distinguishable
// PrintaxGenNL is set in debug_read to make MessageManager::send_msg put
// out a new-line.  PrintaxSpeakCount counts the messages sent since
// the last prompt - and tells debug_read if it should re-prompt

int		PrintaxGenNL = 0;   // starts off in non-asynch mode
int		PrintaxSpeakCount = 0;

static pid_t	ui_pid;

// Set up pipes going in both directions, and exec the ui process
// save the child pid for use by stop_interface
// There are other possible communications channels besides pipes - 
// using shared memory or some other mechanism requires changes to
// start_ui and the Transport class, but that should be about it.

static int
start_ui(const char *ui_name, char **options, const char *gui_path)
{
#ifdef NO_BIDIR_PIPES
	int	tocmd[2];
	int	fromcmd[2];
#else
	int	pipeline[2];
#endif
	char	*name;
	char	c;
	int	i;

	if (gui_path)
	{
		name = new char[strlen(gui_path)+strlen(ui_name)+2];
		sprintf(name, "%s/%s", gui_path, ui_name);
	}
	else
	{
		name = new char[strlen(debug_ui_path)+strlen(ui_name)+1];
		strcpy(name, debug_ui_path);
		strcat(name, ui_name);
	}

#ifdef NO_BIDIR_PIPES
	if ((access(name, X_OK) != 0) || (pipe(tocmd) < 0)
		|| (pipe(fromcmd) < 0))
		return 0;
#else
	if ((access(name, X_OK) != 0) || (pipe(pipeline) < 0))
		return 0;
#endif

	// the child process uses the pipes for stdin and stdout
	if((ui_pid = fork()) == 0)
	{
		options[0] = (char *)ui_name;

		// Set up the file desciptors so that the ui can be sure
		// to get the right ones.  The X gui uses the pipes for
		// stdin and stdout.
		if (ui_name == xui_name)
		{
#ifdef NO_BIDIR_PIPES
			(void) close( 0 );
			(void) fcntl( tocmd[0], F_DUPFD, 0 );
			(void) close( tocmd[0] );
			(void) close( 1 );
			(void) fcntl( fromcmd[1], F_DUPFD, 1 );
			(void) close( fromcmd[1] );
#else
			(void) close( 0 );
			(void) fcntl( pipeline[0], F_DUPFD, 0 );
			(void) close( 1 );
			(void) fcntl( pipeline[0], F_DUPFD, 1 );
			(void) close( pipeline[0] );
#endif
		}

#ifdef NO_BIDIR_PIPES
		(void) close( tocmd[1] );
		(void) close( fromcmd[0] );
#else
		(void) close( pipeline[1] );
#endif
	
		(void) execv( name, options );
		_exit(1);
	}

	if(ui_pid == (pid_t)-1)
		return  0;

#ifdef NO_BIDIR_PIPES
	(void) close( tocmd[0] );
	(void) close( fromcmd[1] );
#else
	(void) close( pipeline[0] );
#endif

	// try reading from pipe to make sure gui initialization
	// succeeded

	do {
		errno = 0;
#ifdef NO_BIDIR_PIPES
		if ((i = read(fromcmd[0], &c, 1)) == 1)
#else
		if ((i = read(pipeline[1], &c, 1)) == 1)
#endif
			break;
	} while(errno == EAGAIN || errno == EINTR);

	if (errno || i != 1)
		return 0;

	// initialize the transport layer with the pipes' file descriptors
	// in debugger, debug_read is used to read from the pipe,
	// so that it can
	// deal with signals (event notifications, etc.)
#ifdef NO_BIDIR_PIPES
	transport = new Transport(fromcmd[0], tocmd[1], 
		read_func, write, db_exit);
#else
	transport = new Transport(pipeline[1], pipeline[1], 
		read_func, write, db_exit);
#endif
	message_manager = new GUI_Manager;
	return 1;
}

#ifdef SDE_SUPPORT
// we have been started in server mode by a gui
static void
setup_as_server(int fd)
{
	transport = new Transport(fd, fd, read_func, write, db_exit);
	message_manager = new GUI_Manager;

	// let gui know we are ready
	write(fd, "1", 1);
}
#endif

static ui_type	interface_type;

ui_type
get_ui_type()
{
	return interface_type;
}

void
set_interface(const char *interface, char **options,
	const char *gui_path)
{
	char	*display;
#ifdef SDE_SUPPORT
	int	fd;
#endif

	// if -i was not specified on the command line, try to start the X ui
	// but if start_ui doesn't work, command line is ok (and already
	// initialized in Manager.C)
	if (!interface)
	{
		if ((display = getenv("DISPLAY")) != 0 && *display)
		{
			if (start_ui(xui_name, options, gui_path))
			{
				// in gui, by default i/o is always redirected
				interface_type = ui_gui;
				redir_io = 1;
				return;
			}
		}
	}
	
	// if the user explicity asks for the X ui, and start_ui doesn't
	// work, exit rather than using the command line interface
	// (could be running from a menu with no base xterm)
	else if (strcmp(interface, "x") == 0)
	{
		if (!start_ui(xui_name, options, gui_path))
		{
			printe(ERR_no_gui, E_FATAL, xui_name);
		}
		else
		{
			// in gui, by default i/o is always redirected
			interface_type = ui_gui;
			redir_io = 1;
			return;
		}
	}

#ifdef SDE_SUPPORT
	else if ((interface[0] == 's') &&
		(interface[1] == '=') &&
		((fd = atoi(&interface[2])) != 0))
	{
		// we have been started as an auxilliary server
		// by the gui - set up transport to receive
		// messages over the server pipes
		setup_as_server(fd);
		interface_type = ui_gui;
		redir_io = 1;
		return;
	}
#endif
	// only -ic, -ix, and -is are allowed
	else if (strcmp(interface, "c") != 0)
	{
		printe(ERR_bad_dashi, E_FATAL, interface);
	}
	interface_type = ui_cli;
}

void
stop_interface()
{
	if (ui_pid > 0)
		kill(ui_pid, SIGUSR2);
}

// There was a problem with the message passing routines
// Trying to use message passing routines to put out an error
// message could lead to infinite recursion, so just write an error and quit

void
interface_error(const char *func, int line, int quit)
{
	char	catid[sizeof(CATALOG) + sizeof(":4") + 1];

	strcpy(catid, CATALOG);
	if (quit)
	{
		strcat(catid, ":4");
		fprintf(stderr, gettxt(catid,
			"Fatal error: Internal error in %s at line %d\n"),
			func, line);
		db_exit(1);
	}
	else
	{
		strcat(catid, ":5");
		fprintf(stderr, gettxt(catid,
			"Error: Internal error in %s at line %d; contents of next message are suspect\n"),
			func, line);
	}
}
