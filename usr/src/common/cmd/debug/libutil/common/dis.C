#ident	"@(#)debugger:libutil/common/dis.C	1.16"

#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proctypes.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "Symbol.h"
#include "Attribute.h"
#include "global.h"
#include "Source.h"
#include "Symbol.h"
#include "SrcFile.h"
#include <signal.h>

int
disassem_cnt( Proclist * procl, Location *l, unsigned int count, 
	char *cnt_var, int disp_func, int dis_mode)
{
	Iaddr	addr;
	Iaddr	hi = ~0;
	Iaddr	offset;
	Iaddr	cur_lo = ~0;
	Iaddr	cur_hi = 0;
	int 	single = 1;
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head = 0;
	int	ret = 1;
	Symbol	func;
	Symbol	file;
	Source	source;
	long	line;
	const char *name = 0;
	char	*dis;
	int	inst_size;
	SrcFile	*sf;

	static char	*spaces[] = { "", " ", "  ", "   ", "    "};

	if (procl)
	{
		single = 0;
		list_head = list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else
	{
		pobj = proglist.current_object();
	}
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}
	sigrelse(SIGINT);
	do
	{
		if (prismember(&interrupt, SIGINT))
			break;
		if (!pobj->state_check(E_RUNNING|E_DEAD))
		{
			ret = 0;
			continue;
		}
		if (l)
		{
			if (!get_addr(pobj, l, addr, st_func, func))
			{
				ret = 0;
				continue;
			}
		}
		else
			addr = pobj->dot_value();
		if (cnt_var)
		{
			int	val;
			if (!parse_num_var(pobj, pobj->curframe(),
				cnt_var, val))
			{
				ret = 0;
				continue;
			}
			count = (unsigned int)val;
		}
		printm(MSG_dis_header, pobj->obj_name(), pobj->prog_name());

		// determine bounds of current function
		func = pobj->find_entry(addr);
		if (!func.isnull())
		{
			cur_lo = func.pc(an_lopc);
			cur_hi = func.pc(an_hipc) - 1;
			name = pobj->symbol_name(func);
			file = func.parent();
			if (!file.isnull() && file.isSourceFile())
				file.source(source);
			else
				source.null();
		}
		if (disp_func)
		{
			// display code for a single function
			if (!func.isnull())
			{
				addr = cur_lo;
				hi = cur_hi;
				count = ~0;
			}
			else
				count = num_line;
		}
		unsigned int i = 0;
		while(addr <= hi && i < count)
		{
			if (prismember(&interrupt, SIGINT))
				break;

			if (addr < cur_lo || addr > cur_hi)
			{
				// new function - reset symbolic info
				func = pobj->find_entry(addr);
				if (!func.isnull())
				{
					cur_lo = func.pc(an_lopc);
					cur_hi = func.pc(an_hipc) - 1;
					name = pobj->symbol_name(func);
					offset = addr - cur_lo;
					file = func.parent();
					if (!file.isnull() && 
						file.isSourceFile())
						file.source(source);
					else
						source.null();
				}
				else
				{
					cur_lo = ~0;
					cur_hi = 0;
					name = 0;
					offset = 0;
				}
			}
			else
				offset = addr - cur_lo;
			if (name && !source.isnull())
			{
				const FileEntry *fentry = source.pc_to_stmt(addr, line, 0);
				if (fentry && dis_mode == DIS_AND_SOURCE)
				{
					// MORE - avoid multiple warnings
					if ((sf = find_srcfile(pobj, fentry)) == 0) 
						printe(ERR_no_source_info, 
							E_ERROR, file.name());
				}
				else
					sf = 0;
			}
			else
				line = 0;
			if (((dis = pobj->disassemble(addr, 1, name,
				offset, inst_size)) != 0) && (inst_size > 0))
			{
				// valid instruction
				if (line)
				{
					char	*ltext = 0;
					if (sf)
						ltext = sf->line((int)line);
					if (ltext)
					{
						printm(MSG_dis_src,
							line, ltext);
						printm(MSG_disassembly,
							addr, dis);
					}
					else
					{
						int	sz = 4;
						long	l = line;
						while(sz > 0 && l >= 10)
						{
							l /= 10;
							sz--;
						}
						printm(MSG_dis_line,
							line, spaces[sz],
							addr, dis);
					}
				}
				else
					printm(MSG_disassembly, addr,
						dis);
				addr = addr + (Iaddr)inst_size;
			}
			else if (dis != 0)
			{
				// bad opcode
				printm(MSG_disassembly, addr, dis);
				addr = addr + 1;
			}
			else
			{
				ret = 0;
				break;
			}
			i++;
		}
		pobj->set_dot(addr);
		if (!single)
			printm(MSG_newline);
	}
	while(!single && ((pobj = list++->p_pobj) != 0));

	if (list_head)
		proglist.free_plist(list_head);
	sighold(SIGINT);
	return ret;
}
