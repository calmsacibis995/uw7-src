#ident	"@(#)debugger:libmotif/common/Resources.C	1.6"

// Read resource data base and initialize resource
// class so rest of gui can access the values

#include "Toolkit.h"
#include "Resources.h"
#include "Proclist.h"
#include "Windows.h"
#include "Syms_pane.h"
#include "UI.h"
#include "Src_pane.h"

#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

Resources	resources;

// converts name in place
static char *
cvt_to_upper(char *str)
{
	register char	*ptr = str;
	while(*ptr)
	{
		*ptr = toupper(*ptr);
		ptr++;
	}
	return str;
}

struct res_action {
	const char 	*name;
	int		action;
};

// list must be sorted alphabetically
static const res_action  acts[] =
{
	{ "BEEP",	(int)A_beep },
	{ "MESSAGE",	(int)A_message },
	{ "NONE",	(int)A_none },
	{ "RAISE",	(int)A_raise },
	{ 0,		(int)A_none },
};

// list must be sorted alphabetically
static const res_action	levs[] = 
{
	{ "PROCESS",	PROCESS_LEVEL },
	{ "PROGRAM",	PROGRAM_LEVEL },
#ifdef DEBUG_THREADS
	{ "THREAD",	THREAD_LEVEL },
#endif
	{ 0,		0 },
};

// list must be sorted alphabetically
static const res_action	stypes[] = 
{
	{ "DEBUGGER",	SYM_debugger },
	{ "FILE",	SYM_file },
	{ "GLOBAL",	SYM_global },
	{ "LOCAL",	SYM_local },
	{ "USER",	SYM_user },
	{ 0,		0 },
};

#ifdef DEBUG_THREADS

// list must be sorted alphabetically
static const res_action	tacts[] = 
{
	{ "BEEP",	TCA_beep },
	{ "NONE",	0 },
	{ "STOP",	TCA_stop },
	{ 0,		0 },
};
#endif

// list must be sorted alphabetically
static const res_action	dis_modes[] = 
{
	{ "NOSOURCE",	DISMODE_DIS_ONLY },
	{ "SOURCE",	DISMODE_DIS_SOURCE },
	{ 0,		0 },
};

static int
str_to_act(const res_action *tab, int defact, char *str, char *resource)
{
	const res_action	*a;

	str = cvt_to_upper(str);

	for(a = tab; a->name; a++)
	{
		int	i = strcmp(str, a->name);
		if (i == 0)
			return a->action;
		else if (i < 0)
			break;
	}
	display_msg(E_ERROR, GE_resource, str, resource);
	return defact;
}

static int
str_to_types(const res_action *tab, int deftype, char *str, char *resource)
{
	char			*next;
	const res_action	*s;
	int			type = 0;
	int			none = 0;

	str = cvt_to_upper(str);
	next = str;
	while(next && *next)
	{
		str = next;
		next = strchr(str, ',');
		if (next)
		{
			*next = 0;
			next++;
		}
		for(s = tab; s->name; s++)
		{
			int	i = strcmp(str, s->name);
			// "NONE" illegal with other types
			if (i == 0)
			{
				if (strcmp(str, "NONE") == 0)
				{
					if (type != 0)
					{
						display_msg(E_ERROR,
							GE_resource_none,
							resource);
						return deftype;
					}
					none = 1;
				}
				else if (none)
				{
					display_msg(E_ERROR,
						GE_resource_none,
						resource);
					return deftype;
				}
				else
					type |= s->action;
				break;
			}
			else if (i < 0)
			{
				display_msg(E_ERROR, GE_resource, str,
					resource);
				return deftype;
			}
		}
	}
	return type;
}

struct DebugResources {
	char	*config_dir;
	char	*output_action;
	char	*event_action;
	char	*command_level;
	char	*event_level;
	char	*symbols;
	char	*dis_mode;
#ifdef DEBUG_THREADS
	char	*thread_action;
#endif
	Boolean	source_no_wrap;
	Boolean	iconic;
};

static XtResource	resrc[] = 
{
	{ "iconic", "Iconic", XtRBoolean, sizeof(Boolean),
		offsetof(DebugResources, iconic), XtRImmediate,
		(caddr_t)False },
	{ "config_directory", "Config_directory", XtRString, sizeof(String), 
		offsetof(DebugResources, config_dir),
		XtRString, (String)0 },
	{ "output_action", "Output_action", XtRString, sizeof(String), 
		offsetof(DebugResources, output_action),
		XtRString, (String)0 },
	{ "event_action", "Event_action", XtRString, sizeof(String), 
		offsetof(DebugResources, event_action),
		XtRString, (String)0 },
	{ "command_level", "Command_level", XtRString, sizeof(String), 
		offsetof(DebugResources, command_level),
		XtRString, (String)0 },
	{ "event_level", "Event_level", XtRString, sizeof(String), 
		offsetof(DebugResources, event_level),
		XtRString, (String)0 },
	{ "symbols", "Symbols", XtRString, sizeof(String), 
		offsetof(DebugResources, symbols),
		XtRString, (String)0 },
	{ "dis_mode", "Dis_mode", XtRString, sizeof(String), 
		offsetof(DebugResources, dis_mode),
		XtRString, (String)0 },
	{ "source_no_wrap", "Source_no_wrap", XtRBoolean, 
		sizeof(Boolean), offsetof(DebugResources, 
		source_no_wrap), XtRImmediate, (caddr_t)False },
#ifdef DEBUG_THREADS
	{ "thread_action", "Thread_action", XtRString, sizeof(String), 
		offsetof(DebugResources, thread_action),
		XtRString, (String)0 },
#endif
};

static XrmOptionDescRec options[] = 
{
	{"-config", "config_directory", XrmoptionSepArg, NULL },
	{"-iconic", "*iconic", XrmoptionNoArg, (caddr_t)"True" },
	{"-output_action", "output_action", XrmoptionSepArg, NULL },
	{"-event_action", "event_action", XrmoptionSepArg, NULL },
	{"-command_level", "command_level", XrmoptionSepArg, NULL },
	{"-event_level", "event_level", XrmoptionSepArg, NULL },
	{"-symbols", "symbols", XrmoptionSepArg, NULL },
	{"-dis_mode", "dis_mode", XrmoptionSepArg, NULL },
	{"-source_no_wrap", "source_no_wrap", XrmoptionNoArg, (caddr_t)"True" },
#ifdef DEBUG_THREADS
	{"-thread_action", "thread_action", XrmoptionSepArg, NULL },
#endif
};

Resources::Resources()
{
	config_dir = 0;
	output_action = A_raise;
	event_action = A_beep;
	event_level = PROGRAM_LEVEL;
	iconic = FALSE;
	symbol_types = SYM_local;
	dis_mode = DISMODE_UNSPEC;
	source_no_wrap = FALSE;
#ifdef DEBUG_THREADS
	thread_action = TCA_beep|TCA_stop;
	command_level = THREAD_LEVEL;
#else
	command_level = PROCESS_LEVEL;
#endif
}

void 
Resources::initialize()
{
	DebugResources	dbg;

	XtGetApplicationResources(base_widget, &dbg, resrc,
		XtNumber(resrc), NULL, 0);
	config_dir = dbg.config_dir;
	iconic = dbg.iconic;
	if (dbg.output_action)
		output_action = (Action)str_to_act(acts, 
			(int)output_action, dbg.output_action,
			"output_action");
	if (dbg.event_action)
		event_action = (Action)str_to_act(acts, 
			(int)event_action, dbg.event_action,
			"event_action");
	if (dbg.command_level)
		command_level = str_to_act(levs, command_level, 
			dbg.command_level, "command_level");
	if (dbg.event_level)
		event_level = str_to_act(levs, event_level, 
			dbg.event_level, "event_level");
	if (dbg.symbols)
		symbol_types = str_to_types(stypes, symbol_types,
			dbg.symbols, "symbols");
	if (dbg.dis_mode)
		dis_mode = str_to_types(dis_modes, dis_mode,
			dbg.dis_mode, "dis_mode");
	source_no_wrap = dbg.source_no_wrap;
#ifdef DEBUG_THREADS
	if (dbg.thread_action)
		thread_action = str_to_types(tacts, thread_action, 
				dbg.thread_action, "thread_action");
#endif
}

XrmOptionDescRec *
Resources::get_options(int &noptions)
{
	noptions = sizeof(options)/sizeof(XrmOptionDescRec);
	return options;
}
