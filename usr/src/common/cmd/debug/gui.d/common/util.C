#ident	"@(#)debugger:gui.d/common/util.C	1.22"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

// Debug headers
#include "Vector.h"
#include "Buffer.h"
#include "UIutil.h"
#include "Machine.h"
#include "str.h"

// GUI headers
#include "Proclist.h"
#include "Dispatcher.h"
#include "Windows.h"
#include "Sender.h"
#include "UI.h"
#include "gui_msg.h"

static Buffer	bpool[BPOOL_SIZE];
Buffer_pool	buf_pool(bpool);

static Vector	vpool[VPOOL_SIZE];
Vector_pool	vec_pool(vpool);

int	Nsignals;
int	Nsyscalls;

int
alpha_comp(const void *s1, const void *s2)
{
	return strcmp(*(char **)s1, *(char **)s2);
}

const char **
get_signals(Order order)
{
	static char	**alpha_names;
	static char	**numeric_names;

	if (!alpha_names)
	{
		Message	*msg;
		int	i = 0;

		alpha_names = new char *[2*(NUMBER_OF_SIGS+1)];
		numeric_names = new char *[2*(NUMBER_OF_SIGS+1)];
		alpha_names[NUMBER_OF_SIGS] = numeric_names[NUMBER_OF_SIGS] = 0;

		// this loop assumes signals always come in numeric order
		dispatcher.query(0, 0, "help signames\n");
		while ((msg = dispatcher.get_response()) != 0)
		{
			char	*buf;
			Word	sig;

			Msg_id mid = msg->get_msg_id();
			if (mid == MSG_signame)
			{
				// help may know about more signals than are used on
				// the system (e.g., sigwaiting)
				if (i+1 == (NUMBER_OF_SIGS+1)*2)
					continue;

				msg->unbundle(sig, buf);
				alpha_names[i] = numeric_names[i] = makestr(buf);
				i++;
				alpha_names[i] = numeric_names[i] = new char[3];
				sprintf(numeric_names[i++], "%-2d", sig);
			}
			else if (mid == MSG_help_hdr_sigs
				|| mid == MSG_newline) // no action needed
				continue;
			else
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		}

		qsort(alpha_names, NUMBER_OF_SIGS, sizeof(char *[2]), alpha_comp);
		Nsignals = NUMBER_OF_SIGS;
	}

	return (order == Alpha) ? (const char **)alpha_names
				: (const char **)numeric_names;
}

const char **
get_syslist(Order order)
{
	static char	**alpha_names;
	static char	**numeric_names;
	static int	i = 0;

	if (!alpha_names)
	{
		Message	*msg;
		Vector	*vscratch = vec_pool.get();

		// this assumes signals come in numeric order
		vscratch->clear();
		dispatcher.query(0, 0, "help sysnames\n");
		while ((msg = dispatcher.get_response()) != 0)
		{
			char	*buf;
			Word	sys;

			Msg_id mid = msg->get_msg_id();
			if (mid == MSG_sys_name)
			{
				msg->unbundle(sys, buf);
				char *name = makestr(buf);
				char *number = new char[4];
				sprintf(number, "%-3d", sys);
				vscratch->add(&name, sizeof(char **));
				vscratch->add(&number, sizeof(char **));
				i++;
			}
			else if (mid == MSG_help_hdr_sys
				|| mid == MSG_newline)	// not displayed
				continue;
			else
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		}

		int slots = i*2;
		alpha_names = new char *[slots+1];
		numeric_names = new char *[slots+1];
		memcpy(alpha_names, vscratch->ptr(), vscratch->size());
		memcpy(numeric_names, vscratch->ptr(), vscratch->size());
		alpha_names[slots] = numeric_names[slots] = 0;
		qsort(alpha_names, i, sizeof(char *[2]), alpha_comp);
		Nsyscalls = i;
		vec_pool.put(vscratch);
	}

	return (order == Alpha) ? (const char **)alpha_names
				: (const char **)numeric_names;
}

Base_window *
Command_sender::get_window()
{
	return 0;
}

void
Command_sender::de_message(Message *)
{
}

void
Command_sender::cmd_complete()
{
}

void
interface_error(const char *func, int line, int quit)
{
	char	catid[sizeof(GUICATALOG) + sizeof(":1")];

	strcpy(catid, GUICATALOG);
	if (quit)
	{
		strcat(catid, ":3");
		fprintf(stderr, gettxt(catid,
			"Fatal error: Internal error in %s at line %d\n"),
			func, line);
		exit(1);
	}
	else
	{
		strcat(catid, ":4");
		fprintf(stderr, gettxt(catid,
			"Internal error in %s at line %d, displayed information may be suspect\n"),
			func, line);
	}
}

Component::~Component()
{
	// interesting work is done in derived classes, as needed
}

void *
Component::get_client_data()
{
	// used by derived classes for extending Component's data
	return 0;
}

Base_window *
Component::get_base()
{
	Component	*component = this;

	while (component && component->get_type() != WINDOW_SHELL)
		component = component->get_parent();

	if (!component)
		return 0;
	return (Base_window *)component->get_creator();
}

// do_vsprintf ensures that there is enough room to really do the vprintf
// before calling vprintf
char *
do_vsprintf(const char *format, va_list ap)
{
	static char		*buf;
	static size_t		bufsiz;

	va_list			ap2 = ap;
	register const char	*ptr;
	size_t			len = 1; // allow for '\0' at end

	for (ptr = format; *ptr; ptr++)
	{
		if (*ptr == '%')
		{
			switch(*++ptr)
			{
			case '%':
				len++;
				break;

			case 's':
				len += strlen(va_arg(ap, char *));
				break;

			case 'c':
			case 'd':
			case 'x':
			case 'X':
			case 'o':
			case 'i':
			case 'u':
			case 'p':
				len += MAX_LONG_DIGITS;	//  enough for maximum digits?
				(void) va_arg(ap, int);
				break;

			default:
				interface_error("do_vsprintf", __LINE__, 1);
				break;
			}
		}
		else
			len++;
	}

	if (len > bufsiz)
	{
		delete buf;
		bufsiz = len + 200;	// room to expand
		buf = new char[bufsiz];
	}

	vsprintf(buf, format, ap2);
	return buf;
}

static int msg_cat_available;

void
init_message_cat()
{
	// determine whether we have a catalog available -
	// otherwise always use default strings
	char	*default_string = "a";

	char buf[sizeof(GUICATALOG) + 10];

	sprintf(buf, "%s:%d", GUICATALOG, 1);
	msg_cat_available = (gettxt(buf, default_string) != default_string);
}

const char *
gm_format(Gui_msg_id msg)
{

	if ((msg <= GM_none) || (msg >= GM_last) || !gmtable[msg].string)
		interface_error("gm_format", __LINE__);

	register GM_tab	*mptr = &gmtable[msg];

	if (!mptr->set_local)
	{
		// not yet translated
		mptr->set_local = 1;
		if (msg_cat_available)
		{
			char buf[sizeof(GUICATALOG)+MAX_INT_DIGITS+1];
			// read the string from the message database 
			// and cache it
			sprintf(buf, "%s:%d", GUICATALOG, mptr->catindex);
			mptr->string = gettxt(buf, mptr->string);
		}
	}
	return mptr->string;
}

// gui-only messages are used to update state information and
// should never be displayed directly to the user
Boolean
gui_only_message(Message *m)
{
	switch(m->get_msg_id())
	{
	case MSG_proc_start:
	case MSG_set_language:
	case MSG_set_frame:
	case MSG_event_disabled:
	case MSG_event_enabled:
	case MSG_event_deleted:
	case MSG_event_changed:
	case MSG_bkpt_set:
	case MSG_bkpt_set_addr:
	case MSG_source_file:
	case MSG_new_context:
	case MSG_new_pty:
	case MSG_rename:
	case MSG_cmd_complete:
	case MSG_jump:
	case MSG_script_on:
	case MSG_script_off:
	case MSG_assoc_cmd:
	case MSG_proc_stop_fcall:
	case MSG_cd:
	case MSG_sync_response:
#if EXCEPTION_HANDLING
	case MSG_uses_eh:
#endif
		return TRUE;

	default:
		return FALSE;
	}
}

// free an array of strings
void
free_strings(const char **list, int total)
{
	for(int i = 0; i < total; ++i)
	{
		delete (void *)list[i];
	}
}

// set debugger's %dis_mode variable - return internal value
// for dis_mode;
int
set_debug_dis_mode(Command_sender *sender, int dis)
{
	const char	*dism;
	switch(dis)
	{
	default:
	case DISMODE_UNSPEC:
		// should only happen on startup
		return(DISMODE_DIS_ONLY);
	case DISMODE_DIS_ONLY:
		dism = "nosource";
		break;
	case DISMODE_DIS_SOURCE:
		dism = "source";
		break;
	}
	dispatcher.send_msg(sender, 0, "set %%dis_mode = %s\n", 
		dism);
	return(dis);
}
