#ident	"@(#)debugger:libutil/common/callstack.C	1.24"

#include "FileEntry.h"
#include "Interface.h"
#include "Language.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proctypes.h"
#include "Proglist.h"
#include "Symtab.h"
#include "Source.h"
#include "Frame.h"
#include "Machine.h"
#include "Expr.h"
#include "Rvalue.h"
#include "TYPE.h"
#include "Parser.h"
#include "global.h"
#include "Tag.h"
#include "Buffer.h"
#include "NewHandle.h"
#include "utility.h"
#include "limits.h"
#include "str.h"
#include <signal.h>

static void show_call(ProcObj *, Frame *, int, Buffer *);


// new handler for stack routines - delete all allocated stack
// frames and print error message

static Frame *allocated_frames;
int	stack_count_down;

static void
handle_stack_memory_error()
{
	Frame *nf;
	if (allocated_frames)
		// leave top frame so ProcObj looks sane
		allocated_frames = (Frame *)allocated_frames->next();
	while(allocated_frames)
	{
		nf = allocated_frames;
		allocated_frames = (Frame *)allocated_frames->next();
		nf->unlink();
		delete(nf);
	}
	// restore old handler before printing message
	// in case it generates another memory error
	newhandler.restore_old_handler();
	printe(ERR_memory_allocation, E_ERROR);
	newhandler.return_to_saved_env(1);
}

// print callstack - if pc or sp are non-0 we allow user to specify
// initial pc and/or sp
int
print_stack(Proclist *procl, int how_many, char *cnt_var, int first,
	char *first_var, Iaddr pc, char *pc_var, Iaddr sp, char *sp_var)
{
	int	count;
	int	single = 1;
	int	ret = 1;
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head;
	Buffer	*buf = buf_pool.get();

	if (procl)
	{
		single = 0;
		list_head = list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else
	{
		pobj = proglist.current_object();
		list_head = 0;
	}
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		buf_pool.put(buf);
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}
	allocated_frames = 0;
	newhandler.install_user_handler(handle_stack_memory_error);
	if (setjmp(*newhandler.get_jmp_buf()) != 0)
	{
		// memory error - longjmp returns here 
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}
	sigrelse(SIGINT);
	do
	{
		Frame	*frame;
		int	intval;

		if (!pobj->state_check(E_RUNNING|E_DEAD))
		{
			ret = 0;
			continue;
		}


		printm(MSG_stack_header, pobj->obj_name(), pobj->prog_name());
		if (pc_var)
		{
			if (!parse_num_var(pobj, 0, pc_var, intval))
			{
				ret = 0;
				continue;
			}
			pc = (Iaddr)intval;
		}
		if (sp_var)
		{
			if (!parse_num_var(pobj, 0, sp_var, intval))
			{
				ret = 0;
				continue;
			}
			sp = (Iaddr)intval;
		}
		if (pc != 0 || sp != 0)
			frame = pobj->topframe(pc, sp);
		else
			frame = pobj->topframe();
		if (first_var)
		{
			if (!parse_num_var(pobj, frame, first_var, 
				first))
			{
				ret = 0;
				continue;
			}
		}
		if (cnt_var)
		{
			if (!parse_num_var(pobj, frame, cnt_var, 
				how_many))
			{
				ret = 0;
				continue;
			}
		}

		allocated_frames = frame;

		if (!stack_count_down)
		{
			// number stacks from 0 for most recent
			// to n for bottom of stack - default
			count = 0;
			if (first >= 0)
			{
				for(int i = 0; (i < first) && frame; i++)
				{
					// start with first rather than top
					frame = frame->caller();
					count++;
				}
			}
			if ( how_many == 0 )
				how_many = INT_MAX; // big enough
		}
		else
		{
			// number stacks from 0 for bottom to
			// n for most recent - compatibility mode
			count = count_frames(pobj);
				// count is 1 less than
				// number of frames

			if (first >= 0)
			{
				// start with first rather than top
				while(first < count)
				{
					frame = frame->caller();
					count--; 
				}
			}
			if ( how_many == 0 )
				how_many = count+1;	
				// big enough
		}

		while ( frame && how_many-- && 
			!prismember(&interrupt, SIGINT))
		{
			show_call(pobj, frame, count, buf);
			if (!stack_count_down)
				count++;
			else
				count--;
			frame = frame->caller();
		}
		if (!single)
			printm(MSG_newline);
		if (prismember(&interrupt, SIGINT))
			break;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));

	sighold(SIGINT);

	allocated_frames = 0;
	newhandler.restore_old_handler();

	if (list_head)
		proglist.free_plist(list_head);
	buf_pool.put(buf);
	return ret;
}

static void
show_call(ProcObj *pobj, Frame *frame, int count, Buffer *buf)
{
	Iaddr	pc = frame->pc_value();
	Symtab	*symtab = pobj->find_symtab(pc);
	const char	*filename = 0;
	const FileEntry	*fentry = 0;
	long	line = 0;
	int	i = 0;
	int	assumed = 0;
	int	nbytes = frame->nargwds(assumed) * sizeof(int);
	int	done = 0;
	Frame	*caller = frame->caller();


	if (symtab)
	{
		Symbol	symbol;
		Source	source;

		if (symtab->find_source(pc, symbol))
		{
			if (symbol.source( source ))
				fentry = source.pc_to_stmt( pc, line );
			filename = fentry ? fentry->get_qualified_name(symbol)
					  : symbol.name();
		}

		Symbol		symbol2 = pobj->find_entry( pc );
		const char	*name = pobj->symbol_name(symbol2);
		if ( !name || !*name )
			name = "?";

		printm(MSG_stack_frame,
			(frame == pobj->curframe()) ? "*" : " ",
			(unsigned long)count, name);
		if ( (nbytes > 0) && caller) 
		{
			Symbol	arg = symbol2.child();
			Tag	tag = arg.tag();
			Language lang = current_context_language(pobj, frame);
			while ( !arg.isnull() ) 
			{
				if (prismember(&interrupt, SIGINT))
					break;
				if (tag == t_argument ) 
				{
					char	*val;
					Expr	*expr = new_expr( arg, pobj, lang );
					Rvalue	*rval = 0;
					if (frame->incomplete()
						|| !expr->eval( pobj, pc, frame )
					    	|| !expr->rvalue( rval ))
						val = "???" ;
					else
					{
						buf->clear();
						rval->print(buf, pobj,
						DEBUGGER_BRIEF_FORMAT);
						val = buf->size() ? (char *)*buf : "";
					}
					printm(MSG_stack_arg, (i++>0) ?
						", " : "",
						arg.name(), val);
					//
					// round to word boundary
					//
					if (rval && !rval->isnull())
						done+=ROUND_TO_WORD(rval->type()->size());
					else
						done+=ROUND_TO_WORD(sizeof(long));
					delete expr;
				}
				arg = arg.sibling();
				tag = arg.tag();
			}
		}
	}
	else
		printm(MSG_stack_frame,
			(frame == pobj->curframe()) ? "*" : " ",
			count, "?");

	if (caller && !frame->incomplete()) 
	{
		int	n = done/sizeof(int);
		int	first = 1;
		while ( done < nbytes ) 
		{
			if (first && assumed)
			{
				// number of args is a guess
				first = 0;
				printm( MSG_stack_arg3, (i++>0)?", ":"", 
					frame->argword(n++) );
			}
			else
				printm( MSG_stack_arg2, (i++>0)?", ":"", 
					frame->argword(n++) );
			done += sizeof(int);
		}
	}

	if ( filename && *filename && line) 
	{
		printm(MSG_stack_frame_end_1, filename, line);
	} 
	else 
	{
		printm(MSG_stack_frame_end_2, pc);
	}

}
