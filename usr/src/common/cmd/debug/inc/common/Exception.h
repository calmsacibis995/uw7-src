#ifndef EXCEPTION_H
#define EXCEPTION_H
#ident	"@(#)debugger:inc/common/Exception.h	1.6"

#if EXCEPTION_HANDLING

#include "Iaddr.h"

class TYPE;
class ProcObj;
class Process;
class Breakpoint;
class Frame;

class Exception_data
{
	Breakpoint	*eh_bkpt;	// primary breakpoint
	Iaddr		eh_brk_addr;	// primary break point address
	Iaddr		eh_addr;	// address of _eh_debug
	Iaddr		eh_object;	// address of object thrown
	TYPE		*eh_type;	// type of object thrown
	const char	*eh_type_name;	// type name from run-time library
	const char	*eh_trigger;	// type causing an event to trigger
					// includes modifiers like const, etc, so
					// may differ from eh_type_name
	int		eh_type_flags;	// flags modifying eh_type_name
	char		eh_default_setting;
	char		eh_obj_valid;
	char		eh_type_valid;	// if type name not found, assumes "void *"
	char		eh_warning_issued;

	friend		ProcObj;
	friend		Process;

public:
			Exception_data(Iaddr info_addr, Iaddr brk_addr);
			Exception_data(Exception_data *, ProcObj *);
			~Exception_data();

			// access functions
	Iaddr		get_eh_object() { return eh_object; }
	TYPE		*get_eh_type()	{ return eh_type; }
	int		is_obj_valid()	{ return eh_obj_valid; }
	int		is_type_valid()	{ return eh_type_valid; }
	const char	*get_type_name()	{ return eh_trigger; }
	const char	*get_unmodified_type()	{ return eh_type_name; }
	void		set_defaults(int ignore, int flags)
				{
					if (ignore)
						eh_default_setting &= ~flags;
					else
						eh_default_setting |= flags;
				}

	int		setup_type(ProcObj *, Iaddr, int flags);
	int		evaluate_type(ProcObj *);
	void		issue_warning(ProcObj *, Frame *);
};

#endif // EXCEPTION_HANDLING

#endif // EXCEPTION_H
