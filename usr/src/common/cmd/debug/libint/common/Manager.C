#ident	"@(#)debugger:libint/common/Manager.C	1.5"

#include	<stdio.h>
#include	<stdarg.h>
#include	<stdlib.h>
#include	<limits.h>

#include	"Interface.h"
#include	"Input.h"
#include	"Msgtab.h"
#include	"Manager.h"
#include	"Parser.h"
#include	"global.h"
#include	"UIutil.h"
#include	"ProcObj.h"

#include	"libint.h"

MessageManager	cli_manager;
MessageManager	*message_manager = &cli_manager;

// write the debugger's output to the current output file
// print out an error label if it is an error message
// return the number of bytes printed as an upper bound for log_msg

int
MessageManager::send_msg(Msg_id mtype, Severity sev ...)
{
	va_list		ap;
	const char	*fmt;
	int		len;

	if ((fmt = Mtable.format(mtype)) == 0)	// no output
		return 0;

	if (mtype == MSG_prompt || mtype == MSG_input_line)
		PrintaxSpeakCount = 0;
	else
	{
		if (PrintaxGenNL)
		{
			fprintf(curoutput->fp, "\n");
			if (log_file)
				fprintf(log_file, "\n");
			PrintaxGenNL = 0;
		}
		PrintaxSpeakCount++;
	}

#ifndef NOCHECKS
	if (Mtable.msg_class(mtype) != MSGCL_error && sev > E_NONE)
		interface_error("MessageManager::send_msg", __LINE__);
#endif // NOCHECKS

	va_start(ap, sev);
	if (sev > E_NONE)
	{
		if (sev >= E_ERROR) 
			last_error = 1;
		fputs(get_label(sev), stderr);
	}

	if (Mtable.msg_class(mtype) != MSGCL_error)
		len = vfprintf(curoutput->fp, fmt, ap);
	else
		len = vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (mtype == MSG_prompt)
		fflush(stdout);
	return len;
}

// read and execute a single command
int
MessageManager::docommand()
{
	const char	*line;

	InputPrompt = PRI_PROMPT;
	if ((line = GetLine()) == 0)
		return 0;

	if (log_file && strncmp(line, "logoff", 6) != 0)
		fputs(line, log_file);

	// InputEcho is true if reading a script without -q
	if (InputEcho())
		printf("%s%s", Pprompt, line);

	parse_and_execute(line);
	return 1;
}

void
MessageManager::sync_request()
{
}

void
MessageManager::reset_context(ProcObj *)
{
}

// a query needs an immediate response
// the response is an integer, one of a list of values given in the query
int
MessageManager::query(Msg_id mtype, int yorn_answer ...)
{
	va_list		ap;
	int		response = 0;
	const char	*line;
	const char	*fmt = Mtable.format(mtype);
	int		len;

#ifndef NOCHECKS
	if (!fmt)
		interface_error("MessageManager::query", __LINE__, 1);
#endif // NOCHECKS

	// print out the question and a prompt, then wait for the
	// user to type in a valid answer
	processing_query = 1;
	va_start(ap, yorn_answer);
	len = vfprintf(curoutput->fp, fmt, ap);
	va_end(ap);
	fflush(stdout);
	InputPrompt = MORE_PROMPT;

	if ((line = GetLine()) == 0)
	{
		processing_query = 0;
		return -1;
	}

	if (log_file)
	{
		va_start(ap, mtype);
		log_msg(len, mtype, E_NONE, ap);
		fputs(line, log_file);
	}

	if (yorn_answer)
	{
		if (strcmp(line, "yes\n") == 0
			|| strcmp(line, "YES\n") == 0
			|| strcmp(line, "Y\n") == 0
			|| strcmp(line, "y\n") == 0
			|| strcmp(line, "1\n") == 0)
			response = 1;
	}
	else
	{
		char	*p = 0;
		response = (int)strtol(line, &p, 10);
		if (response < 0 || response == LONG_MAX || (response == 0 && p == line))
		{
			p = strchr(line, '\n');
			if (p)
			{
				*p = '\0';
			}
			printe(ERR_bad_query_answer, E_ERROR, line);
			response = -1;
		}
	}

	if (InputEcho())
		printf("%s%s", Pprompt, line);

	processing_query = 0;
	return response;
}
