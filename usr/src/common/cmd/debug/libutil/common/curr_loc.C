#ident	"@(#)debugger:libutil/common/curr_loc.C	1.7"

#include "Interface.h"
#include "ProcObj.h"
#include "Frame.h"
#include "Iaddr.h"
#include "utility.h"
#include "Symbol.h"
#include "Source.h"
#include "Tag.h"
#include "FileEntry.h"

// return current compilation unit and source file,
// return current function and line, if available

int
current_loc(ProcObj *pobj, Frame *frame, Symbol &sym,
	const FileEntry **fentry, const char **funcname, long *line)
{
	Source	source;
	Iaddr	pc;

	sym.null();
	if (funcname)
		*funcname = 0;
	if (line)
		*line = 0;
	if (fentry)
		*fentry = 0;

	if ( pobj == 0 )
	{
		printe(ERR_internal, E_ERROR, "current_loc", __LINE__);
		return 0;
	}

	if (frame == 0)
		frame = pobj->topframe();

	pc = frame->pc_value();
	sym = pobj->find_entry(pc);
	if (!sym.isnull())
	{
		if (funcname)
			*funcname = pobj->symbol_name(sym);
	}
	else
		return 0;

	while(!sym.isnull() && sym.tag() != t_sourcefile)
	{
		// This loop goes infinite when the pobj is at the first
		// break (just after a create).  This test treats
		// the symptom but not the cause.
		Symbol parent = sym.parent();
		if (parent == sym) 
			sym.null();
		else 
			sym = sym.parent();
	}
	if (sym.isnull())
	{
		return 1;
	}
	if (fentry)
	{
		if (sym.source(source))
		{
			long	ll;
			*fentry = source.pc_to_stmt(pc, ll);
			if (line)
				*line = ll;
		}
		else
		{
			// file compiled w/o -g
			*fentry = pobj->create_fentry(sym.name());
		}
	}
	return 1;
}
