#ident	"@(#)debugger:libutil/common/create.C	1.10"
#include "Manager.h"
#include "ProcObj.h"
#include "Process.h"
#include "Program.h"
#include "PtyList.h"
#include "Proglist.h"
#include "Interface.h"
#include "utility.h"
#include "global.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

static void	destroy_create_session(int);
int		redir_io;	// no redir by default

int
create_process(const char *cmdline, int redirect, int follow, 
	int on_exec, Location *start_loc, const char *srcpath)
{
	Process	 	*process;
	Process		*first = 0;
	char		*cmd, *cend, *ncmd;
	int		errors = 0, len;
	int		redir;
	int		input = -1, output = -1;
	int		pfd[2];

	static int	create_id;
	static char	*oldargs;
	static char	old_redirect;

	switch(redirect)
	{
	default:
	case DEFAULT_IO:
		if (redir_io)
			redir = REDIR_PTY;
		else
			redir = 0;
		break;
	case REDIR_IO:
		redir = REDIR_PTY; 
		break;
	case DIRECT_IO:
		redir = 0;
		break;
	}

	if (!cmdline)
	{
		// recreate previous create session
		if (!oldargs)
		{
			printe(ERR_no_previous_create, E_ERROR);
			return 0;
		}

		destroy_create_session(create_id);
		len = strlen(oldargs); // used below
		if (redirect == DEFAULT_IO)
		{
			if (old_redirect == REDIR_IO)
				redir = REDIR_PTY;
			else if (old_redirect == DIRECT_IO)
				redir = 0;
		}
		printm(MSG_oldargs, oldargs);
	}
	else
	{
		len = strlen(cmdline);
		old_redirect = redirect;
		delete oldargs;
		oldargs = new(char[len +1]);
		strcpy(oldargs, cmdline);
		if (get_ui_type() == ui_gui)
			printm(MSG_oldargs, oldargs);
	}
	
	create_id++;
	pfd[0] = pfd[1] = -1;
	ncmd = new(char[len + 1]);
	strcpy(ncmd, oldargs);
	cend = ncmd + len;
	cmd = ncmd;

	// process pipeline
	// Parse cmdline; each time we get a non-quoted pipe
	// character, create a process for command up to that
	// point, setting up pipes for i/o redirection
	while((cmd < cend) && *cmd && !errors)
	{
		register char	*p;

		while(isspace(*cmd))
			cmd++;
		if (!*cmd)
			break; 
		p = cmd;
		while(*p)
		{
			switch(*p)
			{
			case '\\':
				// escape; eat it and next char
				p++;
				if (*p)
					p++;
				continue;

			case '\'':	
				// single quote; eat until next one
				do {
					p++;
				} while(*p && *p != '\'');
				if (*p)
					p++;
				continue;
			case '"':	
				// double quote; eat until next one
				do {
					p++;
				} while(*p && *p != '"');
				if (*p)
					p++;
				continue;
			case '|':
				// pipe
				*p = '\0';
				if (pipe(pfd) == -1)
				{
					printe(ERR_sys_pipe, E_ERROR, 
						strerror(errno));
					errors++;
					break;
				}
				redir |= REDIR_OUT;
				output = pfd[1];
				break;
			default:	
				p++;
				break;
			}
		}
		process = new Process();
		message_manager->reset_context(process);
		if (!process->create(cmd, proglist.next_proc(), input, 
			output, redir, create_id, on_exec, follow,
			start_loc, srcpath))
		{
			delete process;
			proglist.dec_proc();
			errors++;
			break;
		}
		if (!first)
			first = process;
		if (redir & REDIR_IN)
		{
			close(input);
		}
		else
			redir |= REDIR_IN;
		input = pfd[0];

		if (redir & REDIR_OUT)
		{
			redir &= ~REDIR_OUT;
			close(output);
			output = -1;
		}
		cmd = p + 1;
	}
	if (!errors && (redir & REDIR_PTY))
		printm(MSG_new_pty,
			process->program()->childio()->name());
	message_manager->reset_context(0);
	delete ncmd;
	if (errors)
	{
		destroy_create_session(create_id);
		printe(ERR_create_fail, E_ERROR);
		delete oldargs;
		oldargs = 0;
		return 0;
	}
	proglist.set_current(first, 0);
	return 1;
}

static void
destroy_create_session(int id)
{
	plist	*list;
	plist	*list_head;

	// processes only
	list_head = list = proglist.all_live_id(id);
	for (Process *p = (Process *)list++->p_pobj; p; 
		p = (Process *)list++->p_pobj)
	{
		destroy_process(p, 1);
	}
	proglist.prune();
	proglist.free_plist(list_head);
}
