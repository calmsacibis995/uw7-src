#ident	"@(#)debugger:gui.d/common/main.C	1.18"

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>

#include "Msgtypes.h"
#include "Message.h"
#include "Msgtab.h"
#include "Transport.h"
#include "UIutil.h"
#include "Label.h"
#include "config.h"

#include "UI.h"
#include "Dispatcher.h"
#include "Resources.h"
#include "Windows.h"
#include "NewHandle.h"
#include "Path.h"

extern Transport	transport;
const char		*msg_internal_error;
int			exit_called;

void
internal_error(int sig)
{
	dispatcher.cleanup();
	write(2, msg_internal_error, strlen(msg_internal_error));
	exit(sig);
}

static void
new_handler()
{
	interface_error("new", __LINE__, 1);
}

static void
bye(int)
{
	if (!exit_called)
		// exit will be called as the result of
		// the quit_cb being called; avoid
		// deadlocks by calling again
		exit(0);
}

void
main(int argc, char **argv)
{
	(void) setlocale(LC_MESSAGES, "");
	newhandler.install_user_handler(new_handler);

	// initialize the three message catalogs
	labeltab.init();
	init_message_cat();
	Mtable.init();

	msg_internal_error = Mtable.format(ERR_cannot_recover);

	Boolean dump_core = FALSE;
	for (int i = 1; i < argc; ++i)
	{
		char *cp = argv[i];
		if (cp[0] == '-' && cp[1] == 'D' && cp[2] == '\0')
		{
			dump_core = TRUE;
			break;
		}
	}
	if (!dump_core)
	{
		signal(SIGQUIT, internal_error);
		signal(SIGILL, internal_error);
		signal(SIGTRAP, internal_error);
		signal(SIGIOT, internal_error);
		signal(SIGEMT, internal_error);
		signal(SIGFPE, internal_error);
		signal(SIGBUS, internal_error);
		signal(SIGSEGV, internal_error);
		signal(SIGSYS, internal_error);
		signal(SIGXCPU, internal_error);
		signal(SIGXFSZ, internal_error);
	}
	// exit gracefully when cli terminates
	signal(SIGUSR2, bye);	

	init_cwd();

	(void) init_gui("debug", "Debug", &argc, argv);

	if (!build_configuration())
		toolkit_main_loop();

	int sets = read_saved_layout();
	do {
		// always at least one window set
		Window_set	*ws = new Window_set();
		sets--;
	} while(sets > 0);

	// Main loop - never returns
	toolkit_main_loop();
}

// let the debugger know the gui is alive and well and ready to start
void
notify_debugger()
{
	write(fileno(stdout), "1", 1);
	dis_mode = set_debug_dis_mode(0, resources.get_dis_mode());
}
