#ident	"@(#)debugger:libexecon/common/PrObj.io.C	1.28"

#include "Interface.h"
#include "ProcObj.h"
#include "Process.h"
#include "Procctl.h"
#include "Proccore.h"
#include "Parser.h"
#include "Seglist.h"
#include "Segment.h"
#include "Source.h"
#include "Symbol.h"
#include "Instr.h"
#include "SrcFile.h"
#include "Frame.h"
#include "Symtab.h"
#include "FileEntry.h"
#include "str.h"
#include "sys/regset.h"
#include <string.h>
#if EXCEPTION_HANDLING
#include "Exception.h"
#endif

int
ProcObj::read( Iaddr addr, Stype stype, Itype & itype )
{
	Segment *seg;

	if ( !is_core() )
	{
		return process()->pctl->read( addr, &itype, 
			stype_size(stype) );
	}
	else if ( (seg = seglist->find_segment( addr )) == 0 )
	{
		return 0;
	}
	else
	{
		return seg->read( addr, stype, itype );
	}
}

int
ProcObj::write( Iaddr addr, Stype stype, const Itype & itype )
{
	if ( !is_core() )
	{
		return process()->pctl->write( addr, 
			(void *)&itype, stype_size(stype) );
	}
	return 0;
}

int
ProcObj::read( Iaddr addr, int len, char * buf )
{
	Segment *	seg;

	if ( !is_core() )
	{
		return process()->pctl->read( addr, buf, len );
	}
	else if ( (seg = seglist->find_segment( addr )) == 0 )
	{
		return 0;
	}
	else
	{
		return seg->read( addr, buf, len );
	}
}

int
ProcObj::write( Iaddr addr, void * buf, int len )
{
	if ( !is_core() )
	{
		return process()->pctl->write( addr, buf, len );
	}
	return 0;
}

void
ProcObj::stateinfo(const char *entryname,
	const char *filename, int vlevel, Execstate disp_state)
{
	const char	*spaceer = " ";
	int  		show_core = 0;

	switch( disp_state )
	{
	case es_stepped:
		if (vlevel < V_REASON)
			return;
		if (ecount != 0)
			printm(MSG_step_not_done, pobj_name);
		else
			printm(MSG_es_stepped, pobj_name);
		break;
	case es_off_lwp:
		// should only happen for grabbed processes
		// at startup
		if (vlevel < V_REASON)
			return;
		printm(MSG_es_halted_off_lwp, pobj_name);
		break;
	case es_halted:
		if (vlevel < V_REASON)
			return;
		printm(MSG_es_halted, pobj_name);
		break;

	case es_halted_thread_start:
		// separate message for benefit of GUI;
		// breakpoint at thread's "main" routine
		if (vlevel < V_REASON)
			return;
		printm(MSG_es_halted_thread_start, pobj_name);
		break;

	case es_signalled:
		printm(MSG_es_signal, latestsig, 
			signame(latestsig), pobj_name);
		break;

	case es_syscallent:
		printm(MSG_es_sysent, latesttsc, sysname(latesttsc),
			pobj_name);
		break;
	case es_syscallxit:
		printm(MSG_es_sysxit, latesttsc, sysname(latesttsc),
			pobj_name);
		break;

	case es_corefile:
	case es_core_off_lwp:
		if (vlevel < V_REASON)
			return;
		printm(MSG_es_core);
		show_core = 1;
		break;

	case es_breakpoint:
#if EXCEPTION_HANDLING
		if (flags & L_EH_THROWN)
		{
			printm(MSG_es_eh_thrown, eh_data->eh_trigger, pobj_name);
			break;
		}
		else if (flags & L_EH_CAUGHT)
		{
			printm(MSG_es_eh_caught, eh_data->eh_trigger, pobj_name);
			break;
		}
#endif // EXCEPTION_HANDLING
		// fall-through
	case es_watchpoint:
		if (strlen(latestexpr) > (size_t)55)
		{	
			// add a new-line to keep it legible
			spaceer = "\n";
		}

		printm(MSG_es_stop, latestexpr, spaceer, pobj_name);
		break;

	default:
		printe(ERR_internal, E_ERROR, 
			"ProcObj::stateinfo", __LINE__);
		break;
	}

	if (entryname && filename)
		printm(MSG_loc_sym_file, entryname, filename);
	else if (entryname)
		printm(MSG_loc_sym, entryname);
	else
		printm(MSG_loc_unknown);
	if (show_core)
	{
		// only on startup - corefile can't change state
		process()->core->core_state();
	}
}

int
ProcObj::writereg( RegRef regref, Stype stype, Itype & itype )
{
	if ( regaccess.writereg( regref, stype, itype ) == 0 )
	{
		return 0;
	}
	else if ( regref == REG_PC )
	{
		dot = pc = itype.iaddr;
	}
	return 1;
}

Iaddr
ProcObj::pc_value()
{
	Frame	*f = curframe();
	if (f == top_frame)
		return pc;
	else
		return f->pc_value();
}

int
ProcObj::setframe(Frame *frame)
{
	if ( !frame || !frame->valid() ) 
	{
		printe(ERR_internal, E_ERROR, "ProcObj::setframe", __LINE__);
		return 0;
	}
	cur_frame = frame;
	dot = frame->pc_value();
	find_cur_src(dot);
	return 1;
}

Frame *
ProcObj::curframe()
{
	if ( !cur_frame || !cur_frame->valid() )
		cur_frame = topframe();

	return cur_frame;
}

Frame *
ProcObj::topframe()
{
	if ( !top_frame || !top_frame->valid() ) 
	{	// if out of date
		while ( top_frame ) 
		{	// delete all frames
			Frame	*f;

			f = top_frame;
			top_frame = (Frame *)top_frame->next();
			delete f;
		}
		top_frame = new Frame(this);	// create new top frame
		cur_frame = top_frame;
	}
	return top_frame;
}

// create top frame - use supplied values for pc and/or 
// stack pointer
Frame *
ProcObj::topframe(Iaddr pc, Iaddr sp)
{
	while ( top_frame ) 
	{	// delete all frames
		Frame	*f;

		f = top_frame;
		top_frame = (Frame *)top_frame->next();
		delete f;
	}
	top_frame = new Frame(pc, sp, this);	// create new top frame
	cur_frame = top_frame;
	
	return top_frame;
}

int
ProcObj::print_map()
{
	if (state == es_corefile)
		return seglist->print_map(core);
	else
	{
		return seglist->print_map(process()->pctl);
	}
}


// display reason for stopping, current location and source line
// or disassembly
// if showstate != es_invalid, use that state for display instead
// of ProcObj::state
int
ProcObj::show_current_location( int showsrc, Execstate showstate ,
	int talk_ctl )
{
	Source		source;
	Symtab 		*symtab;
	Symbol		symbol;
	Symbol		entry;
	SrcFile 	*srcfile;
	char 		*s;
	const char	*fname;
	long		line = 0;
	Execstate	disp_state;
	int		inst_size;
	char		*dis;
	Iaddr		offset = 0;
	int		vlevel;
	const FileEntry	*fentry = 0;
	Iaddr		pc_val = curframe()->pc_value();

	static char	*spaces[] = { "", " ", "  ", "   ", "    "};

	if (talk_ctl >= 0)
		vlevel = talk_ctl;
	else
		vlevel = verbosity;

	if (!vlevel)
		return 1;

	disp_state = (showstate == es_invalid) ? state : showstate;

	if ((symtab = find_symtab( pc_val )) == 0)
	{
		// no symbolic information
		if (vlevel >= V_EVENTS)
			stateinfo(0, 0, vlevel, disp_state);
		dis = disassemble( pc_val, 0, 0, 0, inst_size );
		if (dis)
			printm(MSG_disassembly, pc_val, dis);
		return 1;
	}

	entry = symtab->find_symbol( pc_val );
	fname = entry.isnull() ? 0 : symbol_name(entry);
	if (fname && (vlevel >= V_EVENTS || showsrc))
	{
		if (symtab->find_source(pc_val, symbol) && symbol.source(source))
			fentry = source.pc_to_stmt(pc_val, line);
	}

	const char *file_name = fentry ? fentry->get_qualified_name(symbol)
					: symbol.name();

	if (vlevel >= V_EVENTS)
		stateinfo(fname, file_name, vlevel, disp_state);

	if (!fname || symbol.isnull() || (line == 0) || !showsrc)
	{
		offset = pc_val - entry.pc(an_lopc);
		dis = disassemble( pc_val, 1, fname, offset, inst_size );
		if (dis)
		{
			if (line != 0)
			{
				int	sz = 4;
				long	l = line;
				while(sz > 0 && l >= 10)
				{
					l /= 10;
					sz--;
				}
				printm(MSG_dis_line, line, spaces[sz], pc_val, dis);
			}
			else
				printm(MSG_disassembly, pc_val, dis);
		}
		return 1;
	}

	else if ((srcfile = find_srcfile(this, fentry)) == 0
		|| (s = srcfile->line( line )) == 0)
		printm(MSG_line_no_src, line);
	else
	{
		if (get_ui_type() == ui_gui)
			printm(MSG_source_file, srcfile->filename());
		printm(MSG_line_src, line, s );
	}

	return 1;
}

// is addr in a text segment
int
ProcObj::in_text( Iaddr addr)
{
        return seglist->in_text(addr);
}

int
ProcObj::set_pc(Iaddr addr)
{
	if (!regaccess.set_pc(addr))
		return 0;
	dot = pc = addr;
	state = es_halted;
	remove(bk_hoppt);
	remove(bk_destpt);
	epoch++;
	find_cur_src();
	return 1;
}

// save register state so debugger can later restore after
// a function call
int
ProcObj::save_registers()
{
	gregset_t	*gregs;
	fpregset_t	*fpregs;

	if (!saved_gregs)
		saved_gregs = new gregset_t;
	if ((gregs = read_greg()) == 0)
	{
		printe(ERR_read_reg, E_ERROR, pobj_name);
		return 0;
	}
	memcpy((char *)saved_gregs, (char *)gregs, sizeof(gregset_t));
	if ((fpregs = read_fpreg()) != 0)
	{
		// read_fpreg can return 0 if float regs not used
		if (!saved_fpregs)
			saved_fpregs = new fpregset_t;
		memcpy((char *)saved_fpregs, (char *)fpregs, 
			sizeof(fpregset_t));
	}
	else
	{
		delete saved_fpregs;
		saved_fpregs = 0;
	}
	return 1;
}

// restore register state after a function call
int
ProcObj::restore_registers()
{
	if (!saved_gregs)
	{
		printe(ERR_internal, E_ERROR, 
			"ProcObj::restore_registers", __LINE__);
		return 0;
	}
	if (!write_greg(saved_gregs) ||
		(saved_fpregs && !write_fpreg(saved_fpregs)))
	{
		printe(ERR_write_reg, E_ERROR, pobj_name);
		return 0;
	}
	++epoch;
	regaccess.update();
	return 1;
}

// null base class versions to support cfront 1.2

int
ProcObj::in_stack( Iaddr )
{
        return 0;
}

Iaddr
ProcObj::end_stack()
{
        return 0;
}

gregset_t *
ProcObj::read_greg()
{
	return 0;
}

fpregset_t *
ProcObj::read_fpreg()
{
	return 0;
}

int
ProcObj::write_greg(gregset_t *)
{
	return 0;
}

int
ProcObj::write_fpreg(fpregset_t *)
{
	return 0;
}
