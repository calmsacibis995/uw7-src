#ident	"@(#)debugger:libcmd/common/Help.C	1.17"

#include "Keyword.h"
#include "Parser.h"
#include "Interface.h"
#include "Scanner.h"
#include "TSClist.h"
#include "str.h"
#include "UIutil.h"
#include "Buffer.h"
#include "Machine.h"
#include <limits.h>
#include <locale.h>

extern const char *Help_msgs[];

// This enumeration must match the order of the help messages in cli.help[.thr]

enum HELP
{
	HELP_invalid,
	HELP_bang,	// SHELL
	HELP_alias,	// ALIAS
	HELP_stop,	// STOP
	HELP_break,	// BREAK
	HELP_cont,	// CONTINUE
	HELP_cancel,	// CANCEL
	HELP_create,	// CREATE
	HELP_delete,	// DELETE
	HELP_dis,	// DIS
	HELP_disable,	// DISABLE
	HELP_dump,	// DUMP
	HELP_enable,	// ENABLE
	HELP_event,	// EVENT
	HELP_grab,	// GRAB
	HELP_help,	// HELP
	HELP_if,	// IF
	HELP_input,	// INPUT
	HELP_jump,	// JUMP
	HELP_kill,	// KILL
	HELP_list,	// LIST
	HELP_map,	// MAP
	HELP_symbols,	// SYMBOLS
	HELP_print,	// PRINT
	HELP_ps,	// PS
	HELP_pwd,	// PWD
	HELP_quit,	// QUIT
	HELP_release,	// RELEASE
	HELP_run,	// RUN
	HELP_script,	// SCRIPT
	HELP_set,	// SET
	HELP_signal,	// SIGNAL
	HELP_step,	// STEP
	HELP_halt,	// HALT
	HELP_syscall,	// SYSCALL
	HELP_regs,	// REGS
	HELP_stack,	// STACK
	HELP_while,	// WHILE
	HELP_cd,	// CD
	HELP_change,	// CHANGE
	HELP_export,	// EXPORT
	HELP_fc,	// FC
	HELP_logoff,	// LOGOFF
	HELP_logon,	// LOGON
	HELP_onstop,	// ONSTOP
	HELP_rename,	// RENAME
	HELP_percent_file,
	HELP_percent_follow,
	HELP_percent_frame,
	HELP_percent_func,
	HELP_percent_lang,
	HELP_percent_loc,
	HELP_percent_list_file,
	HELP_percent_list_line,
	HELP_percent_line,
	HELP_percent_mode,
	HELP_percent_path,
	HELP_percent_proc,
	HELP_percent_program,
	HELP_percent_prompt,
	HELP_percent_thisevent,
	HELP_percent_lastevent,
	HELP_assoc,
	HELP_block,
	HELP_format,
	HELP_location,
	HELP_proclist,
	HELP_redir,
	HELP_pattern,
	HELP_regexp,
	HELP_uservars,
	HELP_percent_db_lang,
	HELP_percent_global_path,
	HELP_percent_num_bytes,
	HELP_percent_num_lines,
	HELP_percent_result,
	HELP_percent_verbose,
	HELP_percent_wait,
	HELP_stop_expr,
	HELP_percent_redir,
	HELP_expr,
	HELP_scope,
	HELP_whatis,
#ifdef DEBUG_THREADS
	HELP_program,
	HELP_process,
	HELP_thread,
	HELP_percent_thread,
	HELP_percent_thread_change,
#endif
	HELP_cplusplus,
	HELP_percent_dis_mode,
	HELP_percent_frame_numbers,
#if EXCEPTION_HANDLING
	HELP_exception,
	HELP_percent_eh_object,
#endif
	HELP_percent_stack_bounds,
	HELP_functions,
	HELP_last
};

// WARNING: this table is dependent on the order of the
// command enumeration
static const enum HELP help[] = {
	HELP_invalid,		// NoOp
	HELP_invalid,		// REDIR
	HELP_invalid,		// CMDLIST
	HELP_bang,		// SHELL
	HELP_invalid,		// SAVE
	HELP_alias,		// ALIAS
	HELP_break,		// BREAK
	HELP_cancel,		// CANCEL
	HELP_cd,		// CD
	HELP_change,		// CHANGE
	HELP_cont,		// CONTINUE
	HELP_create,		// CREATE
	HELP_delete,		// DELETE
	HELP_dis,		// DIS
	HELP_disable,		// DISABLE
	HELP_dump,		// DUMP
	HELP_enable,		// ENABLE
	HELP_event,		// EVENT
#if EXCEPTION_HANDLING
	HELP_exception,		// EXCEPTION
#endif
	HELP_export,		// EXPORT
	HELP_fc,		// FC
	HELP_functions,		// FUNCTIONS
	HELP_grab,		// GRAB
	HELP_halt,		// HALT
	HELP_help,		// HELP
	HELP_if,		// IF
	HELP_input,		// INPUT
	HELP_jump,		// JUMP
	HELP_kill,		// KILL
	HELP_list,		// LIST
	HELP_logoff,		// LOGOFF
	HELP_logon,		// LOGON
	HELP_map,		// MAP
	HELP_onstop,		// ONSTOP
	HELP_invalid,		// PENDING - gui only
	HELP_invalid,		// PFILES - gui only
	HELP_invalid,		// PPATH - gui only
	HELP_print,		// PRINT
	HELP_ps,		// PS
	HELP_pwd,		// PWD
	HELP_quit,		// QUIT
	HELP_regs,		// REGS
	HELP_release,		// RELEASE
	HELP_rename,		// RENAME
	HELP_run,		// RUN
	HELP_script,		// SCRIPT
	HELP_set,		// SET
	HELP_signal,		// SIGNAL
	HELP_stack,		// STACK
	HELP_step,		// STEP
	HELP_stop,		// STOP
	HELP_symbols,		// SYMBOLS
	HELP_syscall,		// SYSCALL
	HELP_invalid,		// VERSION
	HELP_whatis,		// WHATIS
	HELP_while,		// WHILE
	HELP_last		// must be last
};


struct Topic {
	const char	*key;
	enum HELP	help;
};

static Topic topic[] = {
	"%db_lang",	HELP_percent_db_lang,
	"%dis_mode",	HELP_percent_dis_mode,
#if EXCEPTION_HANDLING
	"%eh_object",	HELP_percent_eh_object,
#endif
	"%file",	HELP_percent_file,
	"%follow",	HELP_percent_follow,
	"%frame",	HELP_percent_frame,
	"%frame_numbers",HELP_percent_frame_numbers,
	"%func",	HELP_percent_func,
	"%global_path",	HELP_percent_global_path,
	"%lang",	HELP_percent_lang,
	"%lastevent",	HELP_percent_lastevent,
	"%line",	HELP_percent_line,
	"%list_file",	HELP_percent_list_file,
	"%list_line",	HELP_percent_list_line,
	"%loc",		HELP_percent_loc,
	"%mode",	HELP_percent_mode,
	"%num_bytes",	HELP_percent_num_bytes,
	"%num_lines",	HELP_percent_num_lines,
	"%path",	HELP_percent_path,
	"%proc",	HELP_percent_proc,
	"%program",	HELP_percent_program,
	"%prompt",	HELP_percent_prompt,
	"%redir",	HELP_percent_redir,
	"%result",	HELP_percent_result,
	"%stack_bounds", HELP_percent_stack_bounds,
	"%thisevent",	HELP_percent_thisevent,
#ifdef DEBUG_THREADS
	"%thread",	HELP_percent_thread,
	"%thread_change",HELP_percent_thread_change,
#endif
	"%verbose",	HELP_percent_verbose,
	"%wait",	HELP_percent_wait,
	"C++",		HELP_cplusplus,
	"assoccmd",	HELP_assoc,
	"block",	HELP_block,
	"expr",		HELP_expr,
	"format",	HELP_format,
	"location",	HELP_location,
	"pattern",	HELP_pattern,
#ifdef DEBUG_THREADS
	"process",	HELP_process,
#endif
	"proclist",	HELP_proclist,
#ifdef DEBUG_THREADS
	"program",	HELP_program,
#endif
	"redirection",	HELP_redir,
	"regexp",	HELP_regexp,
	"scope",	HELP_scope,
	"signames",	HELP_last,		// special
	"stop_expr",	HELP_stop_expr,
	"sysnames",	HELP_last,		// special
#ifdef DEBUG_THREADS
	"thread",	HELP_thread,
#endif
	"uservars",	HELP_uservars,
	0,		HELP_invalid
};

static void
print_help(enum HELP h)
{
	static	int	has_messages = 0;

	if (!has_messages)
	{
		FILE	*help_file;
		char	help_path[PATH_MAX];
		char	*locale = setlocale(LC_MESSAGES, 0);

		has_messages = 1;
		strcpy(help_path, HELP_PATH1);
		strcat(help_path, locale);
		strcat(help_path, HELP_PATH2);
		if ((help_file = fopen(help_path, "r")) != NULL)
		{
			char	buf[BUFSIZ];
			int	in_message = 0;
			int	msg_num = 1;
			Buffer	*lbuf = buf_pool.get();

			lbuf->clear();
			while (fgets(buf, BUFSIZ, help_file) != NULL)
			{
				if (buf[0] == '+' && buf[1] == '+')
				{
					if (in_message == 1)
					{
						Help_msgs[msg_num++] = makestr((char *)*lbuf);
						lbuf->clear();
						in_message = 0;
					}
					else
						in_message = 1;
				}
				else if (in_message)
					lbuf->add(buf);
			}
			buf_pool.put(lbuf);
		}
	}
	printm(MSG_asis, Help_msgs[h]);
}

int
do_help( register char *p )
{
	int	nest = 0;
	char	line[(17 * 5) + 1];	// 5 items per line
	char	*buf;
	if ( !p || !*p ) 
	{	// list all commands and topics

		printm(MSG_help_hdr_commands);
		register int n = 0;
		buf = line;
		for ( register Keyword *k = keywordtable ; k->str ; ++k ) 
		{
			if ( help[k->op] == HELP_invalid )
				continue;
			if ( ++n%5 == 0 )
			{
				// end of line
				buf += sprintf(buf, "%-.16s", k->str);
				printm(MSG_help_topics, line);
				buf = line;
			}
			else
				buf += sprintf(buf, "%-16.16s", k->str);
		}
		if ( n%5 != 0 )
			printm(MSG_help_topics, line);
		printm(MSG_help_hdr_topics);
		n = 0;
		buf = line;
		for( Topic *t = topic ; t->key ; ++t ) 
		{
			if (t->help == HELP_invalid)
				continue;
			if ( ++n%5 == 0 )
			{
				// end of line
				buf += sprintf(buf, "%-.16s", t->key);
				printm(MSG_help_topics, line);
				buf = line;
			}
			else
				buf += sprintf(buf, "%-16.16s", t->key);
		}
		if ( n%5 != 0 )
			printm(MSG_help_topics, line);
		printm(MSG_help_get_alias);
		printm(MSG_help_get_help);
		return 1;
	} 
	p = str(p);
again:	Op op = NoOp;
	Keyword *k = keyword(p);
	if (k)
		op = k->op;
	if (op != NoOp) 
	{
		if ( (help[op] != HELP_invalid) &&
			(help[op] != HELP_last))
		{
			print_help(help[op]);
		} 
		else 
		{
			printe(ERR_help_no_help, E_ERROR, p);
			return 0;
		} 
	}
	else
	{
		Topic *t = topic;
		for( ; t->key ; ++t ) 
		{
			if ( !strcmp( p, t->key ) )
				break;
		}
		if (!t->key)
		{
			// maybe it's an alias...
			Token *tl = find_alias(p);
			if ( tl ) 
			{
				if ( ++nest >= 20 ) 
				{
					printe(ERR_alias_recursion, 
						E_ERROR, p);
					return 0;
			    	} 
				else 
				{
					char *pspace;
					Buffer	*buf = buf_pool.get();
					print_tok_list(tl, buf);
					char *pp = (char *)*buf;
					buf_pool.put(buf);
					printm(MSG_alias, p, pp);
					if ((pspace = strchr(pp, ' ')) != 0)
						*pspace = 0;
					p = str(pp);
					goto again;
				}
			} 
			else 
			{
				printe(ERR_help_bad_cmd, E_ERROR, p);
				return 0;
			}
		}
		else if ( (t->help != HELP_invalid) && 
			(t->help != HELP_last) )
		{
			print_help(t->help);
		} 
		else
		{	// special topic
			if ( !strcmp(t->key, "signames") ) 
			{
				printm(MSG_help_hdr_sigs);
				int n = 0;
				for ( int i = 1 ; i<=NUMBER_OF_SIGS ; i++ ) 
				{
					const char *s = signame(i);
					if ( s && *s ) 
					{
						printm(MSG_signame, i, s);
						if ( ++n%5 == 0 )
							printm(MSG_newline);
					}
			    	}
				if ( n%5 != 0 )
					printm(MSG_newline);
			}
			else if ( !strcmp(t->key, "sysnames") ) 
			{
#ifdef HAS_SYSCALL_TRACING
				printm(MSG_help_hdr_sys);
				dump_syscalls();
#else
				printe(ERR_op_not_supported_platform,
					E_ERROR);
				return 0;
#endif
			} 
			else 
			{
				printe(ERR_help_bad_cmd, E_ERROR,
					t->key);
				return 0;
			}
		}
	} 
	return 1;
}
