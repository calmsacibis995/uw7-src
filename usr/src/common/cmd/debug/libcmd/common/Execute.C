/*
 * $Copyright:
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libcmd/common/Execute.C	1.55"

#include "Event.h"
#include "Parser.h"
#include "global.h"
#include "utility.h"
#include "Interface.h"
#include "Location.h"
#include "Machine.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proctypes.h"
#include "Proglist.h"
#include "Dbgvarsupp.h"
#include "Shell.h"
#include "Input.h"
#include "Expr.h"
#include "Language.h"
#include "Link.h"
#include "Scanner.h"
#include "Keyword.h"
#include "TSClist.h"
#include "str.h"
#include "Path.h"
#include "Buffer.h"
#include "Manager.h"
#include "PtyList.h"
#include "Thread.h"
#include "Vector.h"

#include <sgs.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

class jbuf	: public Stack
{
public:
	sigjmp_buf	jb;
};

static jbuf		jmpstack;

int			vmode = V_ALL;	   // verbosity
int			wait_4_proc = WAIT; // synchronous by default
int			follow_mode = FOLLOW_ALL;
int			dis_mode = DIS_ONLY;
int			num_line = 10; // for dis and list
int			num_bytes = 256; // for dump
int			check_stack_bounds = 1;
#ifdef DEBUG_THREADS
int			thr_change = TCHANGE_STOP; 
#endif

FILE			*log_file;	// session logging
int			cmd_result;
int			last_error;
char			*original_dir;	// original working dir
static Proclist		pe(1);
static int		change_working_dir(char *);
static int		print_working_dir();

#define	LOOP_CONT	2
#define	LOOP_BREAK	3

int
execute( Node *n )
{
	static int	depth = 0;
	static int	ret;
	static const char *log_file_name;
	char		*strval = 0;

	Proclist	*procl;

	if (!n) 
	{
		cmd_result = 1;
		return 1;
	}

	if (!depth)
		ret = 1;
	++depth;

#if DEBUG
	if (debugflag & DBG_PARSER)
		n->dump();
#endif

	if ( (n->opts & DASH('p')) &&
		(n->optype[1] == PROCLIST) ) 
		procl = n->second.procs;
	else
		procl = 0;

	switch( n->op ) 
	{
	case ALIAS:
		if (n->second.cp)
		{
			if ((strval = parse_str_var(0, 0, n->second.cp)) == 0)
			{
				ret = 0;
				break;
			}
			if (strval != n->second.cp)
			{
				char *ptr = strval;
				if (!isalpha(*ptr) &&
					(*ptr != '_'))
				{
					printe(ERR_missing_id, E_ERROR,
						strval);
					ret = 0;
					break;
				}
				ptr++;
				while(*ptr)
				{
					if (!isalnum(*ptr) &&
						(*ptr != '_'))
					{
						printe(ERR_missing_id, E_ERROR, strval);
						ret = 0;
						break;
					}
					ptr++;
				}
				strval = str(strval);
			}

		}
		if ( n->opts & DASH('r') ) 
		{
			rm_alias(strval);
			break;
		} 
		if ( strval ) 
		{
			if ( n->third.tl ) 
			{
				// check for collisions and warn
				check_collisions( strval, ALIASES|COMMANDS);
				// change this alias
				make_alias(strval, n->third.tl);
			}
			else 
			{
				// print this alias
				print_alias(strval);
			}
		} 
		else 
		{
			// print all aliases
			print_alias(0);
		}
		break;
	case BREAK:
		if (jmpstack.is_empty())
		{
			printe(ERR_no_while, E_ERROR);
			ret = 0;
		}
		else
		{
			jbuf	*j;
			j = (jbuf *)jmpstack.pop();
			siglongjmp(j->jb, LOOP_BREAK);
		}
		break;
        case CANCEL:
	{
		ret = cancel_sig(procl, n->third.ilist);
		break;
	}
	case CD:
		ret = change_working_dir(parse_str_var(0, 0,
			n->second.cp));
		break;
	case CHANGE:
		{
			int		count;
			int		event;
			IntList		*il = n->third.ilist;
			int		set_quiet = -1;
			IntItem		*it = il->first();
			Systype		systype = NoSType;

			if (it->get_type() == it_int)
				event = it->get_ival();
			else if (!parse_num_var(0, 0, it->get_sval(), event))
				break;

			if (n->optype[0] == INT)
				count = n->first.i;
			else if (n->optype[0] == INT_VAR)
			{
				if (!parse_num_var(0, 0, n->first.cp, count))
				{
					ret = 0;
					break;
				}
			}
			else
				count = -1;
			if (n->opts & DASH('x'))
			{
				if (n->opts & DASH('e'))
					systype = Entry_exit;
				else
					systype = Exit;
			}
			else if (n->opts & DASH('e'))
				systype = Entry;
			if (n->opts & DASH('q'))
				set_quiet = 1;
			else if (n->opts & DASH('v'))
				set_quiet = 0;

			ret = change_event(event, procl, count, systype,
				set_quiet, n->fourth.cp, n->fifth.np);
		}
		break;
	case CMDLIST:
		execute( n->first.np );
		execute( n->second.np );
		break;
	case CONTINUE:
		if (jmpstack.is_empty())
		{
			printe(ERR_no_while, E_ERROR);
			ret = 0;
		}
		else
		{
			jbuf	*j;
			j = (jbuf *)jmpstack.pop();
			jmpstack.push((Link *)j);
			siglongjmp(j->jb, LOOP_CONT);

		}
		break;
        case CREATE:
		{
			int		redir = DEFAULT_IO;
			int		follow = follow_mode;
			int		on_exec = 0;
			Location	*l = 0;
			char		*path = 0;

			if (n->opts & DASH('d'))
				redir = DIRECT_IO;
			else if (n->opts & DASH('r'))
				redir = REDIR_IO;
			// undocumented option -e - stop
			// on exec - don't go past dynamic linker
			if (n->opts & DASH('e'))
				on_exec = 1;
			if (n->optype[2] == LOC)
				l = n->third.loc;
			if (n->opts & DASH('s'))
			{
				if ((path = parse_str_var(0, 0, n->fourth.cp)) == 0)
				{
					ret = 0;
					break;
				}
				path = makestr(path);
			}
			if (n->optype[1] == CHARSTAR)
			{
				char	*foll = parse_str_var(0, 0,
					n->second.cp);
				if (!foll)
				{
					ret = 0;
					break;
				}
				if (strcmp(foll, "none") == 0)
					follow = FOLLOW_NONE;
				else if (strcmp(foll, "all") == 0)
					follow = FOLLOW_ALL;
				else if (strcmp(foll, "procs") == 0)
					follow = FOLLOW_PROCS;
				else
				{
					printe(ERR_dashf_arg, E_ERROR);
					ret = 0;
					break;
				}
			}
			if (n->first.cp)
			{
				if ((strval = parse_str_var(0, 0, 
					n->first.cp)) == 0)
				{
					ret = 0;
					break;
				}
			}
			ret = create_process(strval, redir,
				follow, on_exec, l, path);
			delete path;
			break;
		}
	case DIS:
		{
			unsigned int	count = (unsigned int)num_line;
			char		*cnt_var = 0;
			int dmode = dis_mode;
			if (n->opts & DASH('n'))
				dmode = DIS_ONLY;
			else if (n->opts & DASH('s'))
				dmode = DIS_AND_SOURCE;

			if (n->optype[2] == INT)
				count = (unsigned int)n->third.i;
			else if (n->optype[2] == INT_VAR)
			{
				cnt_var = n->third.cp;
			}
			ret = disassem_cnt(procl, n->fourth.loc, count,
				cnt_var, (n->opts & DASH('f')), dmode);
		}
		break;
	case DELETE:
	case DISABLE:
	case ENABLE:
	{
		Event_op op;
		int	fail = 0;
		switch(n->op)
		{
		case DELETE:
			op = M_Delete;
			break;
		case DISABLE:
			op = M_Disable;
			break;
		case ENABLE:
			op = M_Enable;
			break;
		}

		if (n->opts & DASH('a'))
		{
			if (n->optype[2] == NODE)
			{
				Node	*n2 = n->third.np;
				int	etype;

				switch(n2->op) {
				case STOP:	etype = E_STOP; break;
				case SIGNAL:	etype = E_SIGNAL; break;
				case SYSCALL:	etype = E_SCALL; break;
				case ONSTOP:	etype = E_ONSTOP; break;
#if EXCEPTION_HANDLING
				case EXCEPTION:	etype = E_EH_EVENT; break;
#endif
				default:	fail = 1; break;
				}
				if (fail)
				{
					char	*cmd;
					switch(op)
					{
					case M_Delete:
						cmd = "delete";
						break;
					case M_Disable:
						cmd = "disable";
						break;
					case M_Enable:
						cmd = "enable";
						break;
					}
					printe(ERR_event_op_bad_cmd,
						E_ERROR, cmd);
					ret = 0;
					break;
				}
				ret = m_event.event_op(procl, etype, op);
			}
			else
				ret = m_event.event_op(procl, 0, op);
			break;
		}
		else if (n->optype[2] == INTLIST)
		{
			Vector	*v;
			int	cnt;
			int	*eid;

			if (procl)
			{
				printe(ERR_opt_combo_exec, E_ERROR,
					"-p", "an event list");
				ret = 0;
				break;
			}
			v = vec_pool.get();
			if (!parse_list(0, 0, n->third.ilist, v, 
				il_events))
			{
				vec_pool.put(v);
				ret = 0;
				break;
			}
			cnt = v->size()/sizeof(int);
			eid = (int *)v->ptr();
			for(int i = 0; i < cnt; i++, eid++)
			{
				if (*eid)
					if (!m_event.event_op(*eid, op))
						fail = 1;
			}
			ret = (fail == 0);
			vec_pool.put(v);
		}
		else 
		{
			char	*cmd;
			switch(op)
			{
			case M_Delete:
				cmd = "delete";
				break;
			case M_Disable:
				cmd = "disable";
				break;
			case M_Enable:
				cmd = "enable";
				break;
			}
			printe(ERR_event_op_list, E_ERROR, cmd);
			ret = 0;
		}
		break;
	}
	case DUMP:
		{
			int	count = num_bytes;
			char	*cnt_var = 0;
			if (n->optype[2] == INT)
				count = n->third.i;
			else if (n->optype[2] == INT_VAR)
			{
				cnt_var = n->third.cp;
			}
			ret = dump_raw(procl, n->fourth.cp, count,
				cnt_var, (n->opts & DASH('b')));
		}
		break;
	case EVENTS:
		if (n->optype[2] == INTLIST)
		{
			Vector	*v;
			int	cnt;
			int	*eid;

			printm(MSG_event_header_f);
			v = vec_pool.get();
			if (!parse_list(0, 0, n->third.ilist, v, 
				il_events))
			{
				vec_pool.put(v);
				ret = 0;
				break;
			}
			cnt = v->size()/sizeof(int);
			eid = (int *)v->ptr();
			for(int i = 0; i < cnt; i++, eid++)
			{
				if (*eid)
					ret = m_event.event_op(*eid, M_Display);
			}
			vec_pool.put(v);
		}
		else
		{
			static int	set;
			// if no proclist, set to %proc - event_op
			// assumes all procs in program for display
			if (!set)
			{
				pe.ptr[0] = str("%proc");
				set = 1;
			}
			if (procl)
				ret = m_event.event_op(procl, 0, M_Display);
			else
				ret = m_event.event_op(&pe, 0, M_Display);
		}

		break;
#if EXCEPTION_HANDLING
	case EXCEPTION:
		if (n->opts & DASH('d'))
		{
			// command was "exception -d ..."
			// set default exception disposition
			if (procl)
			{
				printe(ERR_option_mix, E_ERROR,
					"-p", "-d");
				ret = 0;
				break;
			}
			if (n->fourth.np)
			{
				printe(ERR_opt_change, E_ERROR, "-d");
				ret = 0;
				break;
			}
			if (n->fifth.np)
			{
				printe(ERR_no_cmd_with_option, E_ERROR,
					"-d");
				ret = 0;
				break;
			}
			if (n->optype[2] != INT)
			{
				display_default_eh_setting();
				ret = 1;
			}
			else
			{
				ret = set_default_eh_setting(n->third.i,
					((n->opts & DASH('i')) != 0));
			}
		}
		else if (n->optype[2] != INT)
		{
			if (n->optype[3] == CHARSTAR || n->optype[4] == CHARSTAR)
			{
				printe(ERR_missing_eh_type, E_ERROR);
				ret = 0;
			}
			else
			{
				// "exception" - display events
				ret = m_event.event_op(procl, E_EH_EVENT, 
					M_Display);
			}
		}
		else if (n->optype[3] != CHARSTAR)
		{
			// "exception throw" or "exception catch",
			// set disposition for current or listed processes
			ret = set_default_eh_setting(procl, n->third.i,
				((n->opts & DASH('i')) != 0));
		}
		else
		{
			// create an event
			if ((n->opts & DASH('i')) && n->fifth.np)
			{
				printe(ERR_no_cmd_with_option, E_ERROR,
					"-i");
				ret = 0;
				break;
			}
			ret = set_eh_event(procl, n->third.i,
				n->fourth.cp,
				n->fifth.np, 
				((n->opts & DASH('i')) != 0), 
				((n->opts & DASH('q')) != 0));
		}
		break;
#endif // EXCEPTION_HANDLING
	case EXPORT:
		Export_variable(n->third.cp);
		break;
	case FC:
		if (n->third.cp)
		{
			if ((strval = parse_str_var(0, 0, n->third.cp)) == 0)
			{
				ret = 0;
				break;
			}
			strval = makestr(strval);
		}
		ret = ksh_fc(strval);
		delete strval;
		break;
	case FUNCTIONS:
		ret = functions(procl, n->third.cp, n->fourth.cp, n->fifth.cp,
			(n->opts & DASH('s')));
		break;
        case GRAB:
	{
		char	*path = 0;
		if (n->opts & DASH('s'))
		{
			if ((path = parse_str_var(0,0, n->first.cp)) == 0)
			{
				ret = 0;
				break;
			}
			path = makestr(path);
		}
		if (n->opts & DASH('c'))
		{
			char	*core;
			Filelist *fl = n->fourth.files;
			if ((core = parse_str_var(0, 0, n->second.cp)) == 0)
			{
				ret = 0;
				break;
			}
			core = makestr(core);
			if ((strval = parse_str_var(0, 0, (*fl)[0])) == 0)
			{
				ret = 0;
				break;
			}
			if (n->opts & DASH('f'))
			{
				printe(ERR_opt_combo_exec, E_ERROR, "-f", "-c");
				ret = 0;
			}
			else if (n->opts & DASH('l'))
			{
				printe(ERR_opt_combo_exec, E_ERROR, "-l", "-c");
				ret = 0;
			}
			else 
				ret = grab_core(strval, core, path);
			delete core;
		}
		else
		{
			int	follow = follow_mode;

			if ((n->opts & DASH('l')) && 
				(n->fourth.files->count > 1))
			{
				printe(ERR_grab_dashl, E_ERROR);
				ret = 0;
				break;
			}
			if (n->optype[4] == CHARSTAR)
			{
				char	*foll = parse_str_var(0, 0,
					n->fifth.cp);
				if (!foll)
				{
					ret = 0;
					break;
				}
				if (strcmp(foll, "none") == 0)
					follow = FOLLOW_NONE;
				else if (strcmp(foll, "all") == 0)
					follow = FOLLOW_ALL;
				else if (strcmp(foll, "procs") == 0)
					follow = FOLLOW_PROCS;
				else
				{
					printe(ERR_dashf_arg, E_ERROR);
					ret = 0;
					break;
				}
			}
			if (n->third.cp)
			{
				if ((strval = parse_str_var(0, 0,
					n->third.cp)) == 0)
				{
					ret = 0;
					break;
				}
				strval = makestr(strval);
			}
			ret = grab_process(n->fourth.files,
				strval, follow, path);
			delete strval;
		}
		delete path;
	}
		break;
        case HALT:
		ret = stop_process( procl );
		break;	
        case HELP:
		ret = do_help( n->third.cp );
		break;
	case IF:
		{
			ProcObj	*pobj = proglist.current_object();
			Process	*proc = 0;

			// we only stop all if we have a thread -
			// the lower-level expr routines catch whether
			// the current object needs to be stopped - for
			// threads we must also have entire process stopped
			if (pobj)
			{
				if (pobj->obj_type() == pobj_thread)
				{
					proc = pobj->process();
					if (!proc->stop_all())
					{
						ret = 0;
						break;
					}
				}
			}

			char	*e = n->first.cp;
			Expr	*exp = new_expr(e, pobj);
			int	istrue;
			if (!exp->eval(pobj))
			{
				ret = 0;
				delete exp;
				break;
			}
			istrue = exp->exprIsTrue(pobj, 0);
			delete exp;
			if (proc)
			{
				if (!proc->restart_all())
				{
					ret = 0;
					break;
				}
			}
			if (istrue)
			{
				execute( n->second.np );
			}
			else if (n->optype[2] == NODE) // else part
			{
				execute( n->third.np );
			}
		}
		break;
	case INPUT:
	{
		char	*instr;
		if ((n->opts & DASH('p')) && (n->opts & DASH('r')))
		{
			printe(ERR_option_mix, E_ERROR, "p", "r");
			ret = 0;
			break;
		}
		if (n->third.cp)
		{
			if ((strval = parse_str_var(0, 0, n->third.cp)) == 0)
			{
				ret = 0;
				break;
			}
			strval = makestr(strval);
		}
		if ((instr = parse_str_var(0, 0, n->fourth.cp)) == 0)
		{
			ret = 0;
			break;
		}
		ret = input_pty(n->second.cp, strval, instr,
			((n->opts & DASH('n')) != 0));
		delete strval;
		break;
	}
	case JUMP:
		ret = jump(procl, n->third.loc);
		break;
        case KILL:
		{
			if (n->optype[2] == INTLIST)
			{
				ret = send_signal(procl, n->third.ilist);
			}
			else
			{
				ret = destroy_process(procl, 1);
			}
			break;
		}
	case LIST:
		{
			Location	*l = 0;
			char		*re = 0;
			int		count = -1;
			char		*cnt_var = 0;

			if (n->optype[2] == INT)
				count = n->third.i;
			else if (n->optype[2] == INT_VAR)
				cnt_var = n->third.cp;
			if (n->optype[3] == LOC)
				l = n->fourth.loc;
			else if (n->optype[3] == CHARSTAR)
				re = n->fourth.cp;
			ret = list_src(procl, count, cnt_var, l, re,
				((n->opts & DASH('d')) != 0));
			break;
		}
	case LOGOFF:
		if (log_file)
		{
			fclose(log_file);
			log_file = 0;
		}
		break;
	case LOGON:
		{
			const char *mode = "a";
			if (log_file)
			{
				printe(ERR_already_logging, E_ERROR);
				break;
			}
			if (n->third.cp)
			{
				delete log_file_name;
				if ((log_file_name = 
					parse_str_var(0, 0, n->third.cp)) == 0)
				{
					ret = 0;
					break;
				}
				log_file_name = makestr(log_file_name);
				mode = "w";
			}
			if (!log_file_name)
			{
				printe(ERR_no_log_file, E_ERROR);
				ret = 0;
				break;
			}
			if ((log_file = debug_fopen(log_file_name, mode)) == 0)
			{
				printe(ERR_cant_open, E_ERROR, log_file_name, strerror(errno));
				ret = 0;
				break;
			}
		}
		break;
	case MAP:
		ret = print_map(procl);
		break;
	case ONSTOP:
		if (n->optype[3] != NODE)
			ret = m_event.event_op(procl, E_ONSTOP, 
				M_Display);
		else
			ret = set_onstop(procl, n->fourth.np);
		break;
	case PENDING:
			// gui only
			ret = pending_sigs(procl);
			break;
	case PFILES:
			// gui only
			ret = print_files(procl);
			break;
	case PPATH:
			// gui only
			ret = print_path(procl, n->third.cp, n->fourth.cp);
			break;
	case PRINT:
		ret = print_expr(procl, n->third.cp, n->fourth.exp, 
			n->opts & DASH('v'), n->opts & DASH('b'));
		break;
        case PS:
		ret = proc_status(procl);
		break;
        case PWD:
		ret = print_working_dir();
		break;
        case QUIT:
                quitflag = 1;
		break;
	case REDIR:
	{
		FILE	*fp;
		int	success = 1;
		int	fopened = 0;

		if ((strval = parse_str_var(0, 0, n->second.cp)) == 0)
		{
			ret = 0;
			break;
		}
		switch ( n->third.i ) 
		{
		case RedirFile:
			if ((fp = debug_fopen(strval, "w")) == 0)
			{
				printe(ERR_cant_create_file, E_WARNING, n->second.cp, strerror(errno));
				success = 0;
			}
			else
			{
				pushoutfile(fp);
				fopened = 1;
			}
			break;
		case RedirAppend:
			if ((fp = debug_fopen(strval, "a")) == 0)
			{
				printe(ERR_cant_append,E_WARNING, n->second.cp, strerror(errno));
				success = 0;
			}
			else
			{
				pushoutfile(fp);
				fopened = 1;
			}
			break;
		case RedirPipe:
			if (get_ui_type() == ui_gui)
			{
				printe(ERR_not_supported_gui, E_ERROR);
				success = 0;
			}
			else if ((fp = popen(strval, "w")) == 0)
			{
				printe(ERR_sys_pipe, E_WARNING, 
					strerror(errno));
				success = 0;
			}
			else
				pushoutfile(fp);
			break;
		}
		if (success)
		{
			execute( n->first.np );
			popout();
			if (fopened)
				fclose(fp);
			else
				pclose(fp);
		}
		else
			ret = 0;
		break;
	}
	case REGS:
		ret = printregs(procl);
		break;
        case RELEASE:
		ret = release_proclist(procl, ((n->opts & DASH('s')) == 0));
		break;
	case RENAME:
	{
		char	*newname;
		if ((strval = parse_str_var(0, 0, n->third.cp)) == 0)
		{
			ret = 0;
			break;
		}
		strval = makestr(strval);
		if ((newname = parse_str_var(0, 0, n->fourth.cp)) == 0)
		{
			ret = 0;
			break;
		}
		ret = rename_prog(strval, newname);
		delete strval;
		break;
	}
        case RUN:
		{
			int rwait = wait_4_proc;
			if (n->opts & DASH('b'))
				rwait = NO_WAIT;
			else if (n->opts & DASH('f'))
				rwait = WAIT;
			if (n->opts & DASH('r'))
				ret = run ( procl, 1, n->third.loc, rwait );
			else
				ret = run ( procl, 0, n->third.loc, rwait );
		}
            break;
	case SAVE:
		execute( n->first.np );
		break;
        case SCRIPT:
	{
		int	save_depth;
		if ((strval = parse_str_var(0, 0, n->third.cp)) == 0)
		{
			ret = 0;
			break;
		}
		if (InputFile(strval, 0, ((n->opts & DASH('q')) == 0)))
		{
			printe(ERR_cant_open, E_ERROR, strval,
				strerror(errno));
			ret = 0;
			break;
		}
		if (get_ui_type() == ui_gui)
			printm(MSG_script_on);
		save_depth = depth;
		depth = 0;
		doscript();
		depth = save_depth;
		CloseInput();
		if (get_ui_type() == ui_gui)
			printm(MSG_script_off);
		break;
	}

	case SET:
		ret = set_val(procl, n->third.cp, n->fourth.exp, n->opts & DASH('v'));
		break;
	case SHELL:
		if ((strval = parse_str_var(0, 0, n->first.cp)) == 0)
		{
			ret = 0;
			break;
		}
		strval = makestr(strval);
		if (get_ui_type() == ui_gui)
		{
			printe(ERR_not_supported_gui, E_ERROR);
			ret = 0;
		}
		else
			ret = do_shell( strval, 1 );
		delete strval;
		break;
        case SIGNAL:
		if (n->opts & DASH('d'))
		{
			if (procl)
			{
				printe(ERR_option_mix, E_ERROR,
					"-p", "-d");
				ret = 0;
				break;
			}
			if (n->fourth.np)
			{
				printe(ERR_no_cmd_with_option, E_ERROR,
					"-d");
				ret = 0;
				break;
			}
			if (n->optype[2] != INTLIST)
			{
				display_default_signals();
				ret = 1;
			}
			else
			{
				ret = set_signal_defaults(n->third.ilist,
					((n->opts & DASH('i')) != 0));
			}
		}
		else if (n->optype[2] != INTLIST)
		{
			ret = m_event.event_op(procl, E_SIGNAL, 
				M_Display);
		}
		else
		{
			if ((n->opts & DASH('i')) && n->fourth.np)
			{
				printe(ERR_no_cmd_with_option, E_ERROR,
					"-i");
				ret = 0;
				break;
			}
			ret = set_signal(procl, n->third.ilist,
				n->fourth.np, 
				((n->opts & DASH('i')) != 0), 
				((n->opts & DASH('q')) != 0));
		}
		break;
	case STACK:
		{
			int	first = -1;
			Iaddr	pc = 0, sp = 0;
			int	count = 0;
			char	*cnt_var = 0, *pc_var = 0, *sp_var = 0,
					*first_var = 0;
			if (n->optype[0] == INT)
				first = n->first.i;
			else if (n->optype[0] == INT_VAR)
				first_var = n->first.cp;
			if (n->optype[2] == INT)
				count = n->third.i;
			else if (n->optype[2] == INT_VAR)
				cnt_var = n->third.cp;
			if (n->optype[3] == INT)
				pc = (Iaddr)n->fourth.i;
			else if (n->optype[3] == INT_VAR)
				pc_var = n->fourth.cp;
			if (n->optype[4] == INT)
				sp = (Iaddr)n->fifth.i;
			else if (n->optype[4] == INT_VAR)
				sp_var = n->fifth.cp;
			ret = print_stack(procl, count, cnt_var,
				first, first_var, pc, pc_var, sp,
				sp_var);
		}
		break;
        case STEP:
		{
			int count = 1;
			int swait = wait_4_proc;
			int talk = vmode;
			int level, where;
			char *cnt_var = 0;

			if ( n->optype[2] == INT )
			{
				count = n->third.i;
			}
			else if (n->optype[2] == INT_VAR)
			{
				cnt_var = n->third.cp;
			}
			if (n->opts & DASH('b'))
				swait = NO_WAIT;
			else if (n->opts & DASH('f'))
				swait = WAIT;
			if ( n->opts & DASH('q') ) 
				talk = 0;

			if ( n->opts & DASH('i') ) 
				level = STEP_INSTR;
			else
				level = STEP_STMT;
		
			if ( n->opts & DASH('o') )
				where = STEP_OVER;
			else
				where = STEP_INTO;

			ret = single_step(procl, 0, count, cnt_var,
				talk, level, where, swait );
			break;
		}
	case STOP:
		if ( n->optype[2] != EVENT_EXPR )
		{	// no locations
			ret = m_event.event_op(procl, E_STOP, 
				M_Display);
		}
		else
		{
			int	count = 0;
			char	*cnt_var = 0;
			if (n->optype[0] == INT)
				count = n->first.i;
			else if (n->optype[0] == INT_VAR)
				cnt_var = n->first.cp;
			ret = set_stop( procl, n->third.stop, n->fourth.np, 
				count, cnt_var,
				((n->opts & DASH('q')) != 0)); 
		}
	    	break;
	case SYSCALL:
#ifdef HAS_SYSCALL_TRACING
		if (n->optype[2] != INTLIST)
		{
			ret = m_event.event_op(procl, E_SCALL, 
				M_Display);
		}
		else 
		{
			Systype	stype;
			int	count = 0;
			char	*cnt_var = 0;
			if (n->optype[0] == INT)
				count = n->first.i;
			else if (n->optype[0] == INT_VAR)
				cnt_var = n->first.cp;

			if (n->opts & DASH('x'))
			{
				if (n->opts & DASH('e'))
					stype = Entry_exit;
				else
					stype = Exit;
			}
			else
				stype = Entry;
			ret = set_syscall(procl, n->third.ilist,
					stype, n->fourth.np, count, cnt_var,
					((n->opts & DASH('q')) != 0));
		}
#else
		printe(ERR_not_supported_platform, E_ERROR);
		ret = 0;
#endif
		break;
        case SYMBOLS:
		{
			int	mode = 0;

			if (n->opts & DASH('g'))
				mode |= SYM_GLOBALS;
			if (n->opts & DASH('f'))
				mode |= SYM_FILES;
			if (n->opts & DASH('l'))
				mode |= SYM_LOCALS;
			if (n->opts & DASH('u'))
				mode |= SYM_USER;
			if (n->opts & DASH('d'))
				mode |= SYM_BUILT_IN;
			if (n->opts & DASH('v'))
				mode |= SYM_VALUES;
			if (n->opts & DASH('t'))
				mode |= SYM_TYPES;

			// printing the locals is the default
			if (!(mode & (SYM_GLOBALS|SYM_FILES|SYM_USER|SYM_BUILT_IN)))
				mode |= SYM_LOCALS;
			ret = symbols(procl, n->third.cp, n->fifth.cp,
				n->fourth.cp, mode);
			break;
		}
	case VERSION:
			// gui only
			printm(MSG_version, prog_name, EDEBUG_PKG, EDEBUG_REL);
			break;

	case WHATIS:
			ret = whatis(procl, n->third.cp);
			break;

	case WHILE:
		{
			// We maintain a stack of jmp buffers
			// for use with break or continue commands.
			// Right before entering the while loop
			// we do a setjmp; break or continue will
			// do a longjmp back to that spot; if break,
			// just return, else re-enter loop.

			jbuf *j = new jbuf;
			char *e = n->first.cp;
			jmpstack.push((Link *)j);
			if (sigsetjmp(j->jb, 1) == LOOP_BREAK)
			{
				// break command
				delete j;
				break;
			}
			while(1)
			{
				sigrelse(SIGINT);
				if (prismember(&interrupt, SIGINT))
					break;
				ProcObj	*pobj = proglist.current_object();
				Process	*proc = 0;

				// we only stop all if we have a thread -
				// the lower-level expr routines catch whether
				// the current object needs to be stopped - for
				// threads we must also have entire process stopped
				if (pobj)
				{
					if (pobj->obj_type() == pobj_thread)
					{
						proc = pobj->process();
						if (!proc->stop_all())
						{
							ret = 0;
							break;
						}
					}
				}
				Expr 	*exp = new_expr(e, pobj);
				int	istrue;
				if (!exp->eval(pobj))
				{
					ret = 0;
					delete exp;
					break;
				}
				istrue = exp->exprIsTrue(pobj, 0);
				delete exp;
				if (proc)
				{
					if (!proc->restart_all())
					{
						ret = 0;
						break;
					}
				}
				if (istrue)
				{
					if (execute( n->second.np ) != 0)
					{
						ret = 0;
					}
				}
				else
				{
					break;
				}
			}
			sighold(SIGINT);
			// if no break , we must pop stack
			(void)jmpstack.pop();
			delete j;
		}
		break;
	case NoOp:
		break;
	default:
		printe(ERR_internal, E_FATAL, "execute", __LINE__);
	}

	if (n->op != CMDLIST && n->op != SAVE)
		message_manager->sync_request();
	--depth;
	if (!depth)
	{
		if (n->op != SAVE)
			delete n;
	}


	// ret is 0 if there was an error.
	// last_error is non-zero if there was an error.
	//    last_error is set if an error message was printed.
	// 0 should be returned if there was not an error.
	if (!ret && !last_error)
		cmd_result = 1;
	else
		cmd_result = last_error;

	return cmd_result;
}


static int
change_working_dir(char *newdir)
{
	// save original working dir
	if (!original_dir)
	{
		if (!current_dir && init_cwd() == 0)
		{
			printe(ERR_pwd_fail, E_ERROR, strerror(errno));
			return 0;
		}
		original_dir = makestr(current_dir);
	}
	if (!newdir)
		newdir = getenv("HOME");
	if (!newdir || !*newdir)
	{
		printe(ERR_no_new_dir, E_ERROR);
	}
	if ((chdir(newdir) != 0) || (init_cwd() == 0))
	{
		printe(ERR_cwd_fail, E_ERROR, newdir, strerror(errno));
		return 0;
	}
	pathage++;	// to invalidate cached source files
	if (get_ui_type() == ui_gui)
		printm(MSG_cd, newdir);
	return 1;
}

static int
print_working_dir()
{
	if (!current_dir && !init_cwd())
	{
		printe(ERR_pwd_fail, E_ERROR, strerror(errno));
		return 0;
	}
	printm(MSG_pwd, current_dir);
	return 1;
}

int
do_shell( const char *p, int update_last )
{
	const char	*cmd;
	static char	*oldcmd;
	PtyInfo		*pty = 0;
	ui_type		ut = get_ui_type();

	if (update_last)
	{
		// !! executes last shell cmd
		if (strcmp(p, "!") == 0)
		{
			if (!oldcmd)
			{
				printe(ERR_no_prev_cmd, E_ERROR);
				return 0;
			}
		} 
		else
		{
			delete oldcmd;
			oldcmd = new char[strlen(p) + 1];
			strcpy(oldcmd, p);
		}
		cmd = oldcmd;
	}
	else
		cmd = p;

	if (ut != ui_gui)
		restore_tty();
	pid_t pid = Shell( (char *)cmd, ut == ui_gui, pty);

	if ( pid == -1 ) 
	{
		delete pty;
		printe(ERR_fork_failed, E_ERROR, strerror(errno));
		return 0;
	}

	int status = 0;
	if ((ut == ui_gui) && pty)
		sigrelse(SIGPOLL);
	do
	{
		errno = 0;
		if (waitpid(pid, &status, 0) == pid)
			break;
	} while (errno != ECHILD);
		
	if (ut != ui_gui)
		debugtty();
	else
	{
		if (pty)
		{
			delete pty;
			sighold(SIGPOLL);
		}
	}

	if ( WIFSIGNALED(status))
	{			// signal
		unsigned sig = WTERMSIG(status);
		if (WCOREDUMP(status))
			printe(ERR_shell_cmd_core, E_ERROR, sig);
		else
			printe(ERR_shell_cmd, E_ERROR, sig);
	}
	return 1;
}

void
list_cmd(Node *n, Buffer *buf)
{
	static int	depth;
	static Buffer	*loc_buf;
	Keyword		*k;
	char		arg_ops[MAX_ARGS];
	int		i;

	if (!n) 
	{
		buf->clear();
		return;
	}
	if (!depth)
	{
		buf->clear();
		loc_buf = buf_pool.get();
	}

	++depth;
	for (int j = 0; j < MAX_ARGS; j++)
		arg_ops[j] = 0;

	// CMDLIST, REDIR, and SAVE don't have command strings
	if ((k = keyword(n->op)) != 0)
	{
		buf->add(k->str);
		buf->add(' ');
	}

	// print out options - temporarily skipping options that have
	// arguments
	if (n->opts)
	{
		int	hasopts = 0;
		int	nopts = n->opts;
		const char *p = k->opts;
		const char *r;
		while (*p)
		{
			if (*(p+1) == ':')
			{
				r = p+2;
				arg_ops[(*r - '0') -1] = *p;
				p = r+1;
			}
			else if (DASH(*p) & nopts)
			{
				if (!hasopts)
				{
					buf->add('-');
					hasopts = 1;
				}
				buf->add(*p++);
			}
			else
				p++;
		}
		if (hasopts)
			buf->add(' ');
	}
	switch( n->op ) 
	{
	case REDIR:
		list_cmd( n->first.np, buf );
		switch ( n->third.i ) 
		{
		case RedirFile:
			buf->add("> ");
			break;
		case RedirAppend:
			buf->add(">> ");
			break;
		case RedirPipe:
			buf->add("| ");
			break;
		}
		buf->add(n->second.cp);
		buf->add(' ');
		break;
	case SAVE:
		list_cmd( n->first.np, buf );
		break;
	case CMDLIST:
	{
		int nest = 0;
		if ( n->first.np )
		{
			if ( n->first.np->op == CMDLIST ) 
			{
				buf->add("{ ");
				++nest;
			}
			list_cmd( n->first.np, buf );
		} 
		else 
		{
			buf->add("{ } ");
		}
		if ( n->second.np ) 
		{
			buf->add("; ");
			list_cmd( n->second.np, buf );
		} 
		else if ( nest ) 
		{
			buf->add("} ");
		}
		break;
	}
	case LIST:
		if (n->optype[2] == LOC)
		{
			Location *l = n->third.loc;
			if ( l )
			{
				l->print(buf);
				buf->add(' ');
			}
		}
		else if (n->optype[2] == CHARSTAR)
		{
			char c;
			if (n->opts & DASH('d'))
				c = '/';
			else
				c = '?';
			buf->add(c);
			buf->add(n->third.cp);
			buf->add(c);
		}
		break;
#if EXCEPTION_HANDLING
	case EXCEPTION:
		if (n->optype[2] == INT)
		{
			if (n->third.i & E_THROW)
				buf->add("throw ");
			if (n->third.i & E_CATCH)
				buf->add("catch ");
		}
		if (n->optype[3] == CHARSTAR)
		{
			buf->add(n->fourth.cp);
			buf->add(' ');
		}
		if (n->optype[4] == NODE)
			list_cmd(n->fifth.np, buf);
		break;
#endif // EXCEPTION_HANDLING
	case NoOp:
		break;
	default:
		for (i = 0; i < MAX_ARGS; i++)
		{
			void	*opnd = n->getopnd(i+1);
			if (!opnd)
				continue;
			if (arg_ops[i] != 0)
			{
				buf->add('-');
				buf->add(arg_ops[i]);
				buf->add(' ');
			}

			switch(n->optype[i])
			{
			case NODE:
			{
				Node *sub_node = (Node *)opnd;
				if (sub_node->op != CMDLIST)
					buf->add("{ ");
				list_cmd(sub_node, buf);
				if (sub_node->op != CMDLIST)
					buf->add(" }");
				if ((n->op == IF) && (i == 1) && 
					(n->optype[2] == NODE))
						buf->add("else");
				break;
			}
			case CHARSTAR:
			case INT_VAR:
				buf->add((char *)opnd);
				break;
			case STRING_OPND:
				buf->add(fmtstring((char *)opnd));
				break;
			case INT:
				buf->add((int)opnd);
				break;
			case TOKEN:
				print_tok_list((Token *)opnd, loc_buf);
				buf->add((char *)*loc_buf);
				break;
			case PROCLIST:
				((Proclist *)opnd)->print(loc_buf);
				buf->add((char *)*loc_buf);
				break;
			case LOC:
				((Location *)opnd)->print(buf);
				break;
			case INTLIST:
				((IntList *)opnd)->print(loc_buf);
				buf->add((char *)*loc_buf);
				break;
			case EXPLIST:
				((Exp *)opnd)->print(loc_buf);
				buf->add((char *)*loc_buf);
				break;
			case EVENT_EXPR:
			{
				char *p = print_stop((StopEvent *)opnd);
				buf->add(p);
				delete(p);
				break;
			}
			default:	break;
			}
			buf->add(' ');
		}
		break;
	}
	--depth;
	if (!depth)
		buf_pool.put(loc_buf);
}
