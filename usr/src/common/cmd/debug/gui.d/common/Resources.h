#ifndef _RESOURCES_H
#define _RESOURCES_H

#ident	"@(#)debugger:gui.d/common/Resources.h	1.4"

#include "ResourcesP.h"
#include "Windows.h"
#include "UI.h"

class Resources {
	RESOURCE_TOOLKIT_SPECIFICS
private:
	const char	*config_dir;
	Action		output_action;
	Action		event_action;
	int		event_level;
	int		command_level;
	int		symbol_types;
	int		dis_mode;
#ifdef DEBUG_THREADS
	int		thread_action;
#endif
	Boolean		iconic;
	Boolean		source_no_wrap;
public:
			Resources();
			~Resources() {}
	void		initialize();
	const char	*get_config_dir() { return config_dir; }
	Action		get_output_action() { return output_action; }
	Action		get_event_action() { return event_action; }
	int		get_command_level() { return command_level; }
	int		get_event_level() { return event_level; }
	int		get_symbol_types() { return symbol_types; }
	Boolean		get_iconic()	{ return iconic; }
	int		get_dis_mode() { return dis_mode; }
	Boolean		get_source_no_wrap() { return source_no_wrap; }
#ifdef DEBUG_THREADS
	int		get_thread_action() { return thread_action; }
#endif
};

extern Resources	resources;

#endif
