#ident	"@(#)debugger:libexecon/common/Exception.C	1.12"

#if EXCEPTION_HANDLING

#include "Breaklist.h"
#include "Buffer.h"
#include "Exception.h"
#include "Expr.h"
#include "Frame.h"
#include "Interface.h"
#include "ProcObj.h"
#include "TYPE.h"
#include "global.h"
#include "str.h"
#include "utility.h"

Exception_data::Exception_data(Iaddr info_addr, Iaddr brk_addr)
{
	eh_addr = info_addr;
	eh_brk_addr = brk_addr;
	eh_bkpt = 0;
	eh_default_setting = default_eh_setting;
	eh_obj_valid = 0;
	eh_object = 0;
	eh_type = 0;
	eh_type_valid = 0;
	eh_trigger = 0;
	eh_type_name = 0;
	eh_warning_issued = 0;
	eh_type_flags = 0;
}

Exception_data::Exception_data(Exception_data *odata, ProcObj *proc)
{
	eh_default_setting = odata->eh_default_setting;
	eh_brk_addr = odata->eh_brk_addr;
	if (eh_brk_addr)
		eh_bkpt = proc->set_bkpt(eh_brk_addr, 0, 0);
	eh_addr = odata->eh_addr;
	eh_obj_valid = odata->eh_obj_valid;
	eh_object = odata->eh_object;
	eh_type = odata->eh_type;
	eh_trigger = (odata->eh_trigger) ? makestr(odata->eh_trigger) : 0;
	eh_type_name = odata->eh_type_name;
	eh_type_valid = odata->eh_type_valid;
	eh_warning_issued = odata->eh_warning_issued;
	eh_type_flags = odata->eh_type_flags;
}

Exception_data::~Exception_data()
{
	delete eh_trigger;
	delete eh_type;
}

// called when a catch or throw is encountered to get the type name
// of the exception from the run-time library
int
Exception_data::setup_type(ProcObj *proc, Iaddr type_name_addr, int flags)
{
	char			*type_name;
	void			*addr;

	addr = (void *)type_name_addr;
	if ((type_name = get_program_string(proc, &addr)) == 0)
	{
		printe(ERR_proc_read, E_ERROR, proc->obj_name(), (Word)addr);
		return 0;
	}

	eh_type_name = str(type_name);
	eh_type_flags = flags;
	delete eh_type;
	eh_type = 0;
	eh_type_valid = 0;
	eh_warning_issued = 0;
	return 1;
}

// called at the throw-point or in the catch handler to evaluate the thrown type
int
Exception_data::evaluate_type(ProcObj *proc)
{
	int	ret = 1;
	int	err = 0;
	Buffer	*buf = buf_pool.get();
	buf->clear();

	Expr	*expr = new_expr(eh_type_name, proc, 0, 1, CPLUS);
	eh_type = expr->create_program_type(proc->curframe(), eh_type_flags, err);
	if (expr->print_type(buf))
	{
		// reset type name
		delete eh_trigger;
		eh_trigger = makestr((char *)*buf);
	}
	else
	{
		ret = 0;
	}

	delete expr;
	buf_pool.put(buf);
	eh_type_valid = !err;
	return (ret && !err);
}

void
Exception_data::issue_warning(ProcObj *pobj, Frame *frame)
{
	if (eh_warning_issued)
		return;

	Symbol		sym = pobj->find_entry(frame->pc_value());
	const char	*name = sym.name();
	if (!name || !*name)
		name = "?";
	printe(ERR_eh_type_assumed, E_WARNING, eh_type_name, name);
	eh_warning_issued = 1;
}

#endif	// EXCEPTION_HANDLING
