#ident	"@(#)debugger:libint/common/Output.C	1.6"

#include	<stdio.h>
#include	<stdarg.h>
#include	<string.h>

#include	"Interface.h"
#include	"global.h"
#include	"Msgtab.h"
#include	"UIutil.h"

#include	"libint.h"
#include	"Manager.h"

// top of stack of output files
OutPut		*curoutput;

// redirect output
void
pushoutfile(FILE *fp)
{
	OutPut *out = new OutPut;
	out->fp = fp;
	if (curoutput)
		out->prepend(curoutput);
	curoutput = out;
}

void
popout()
{
	OutPut *out;

	if (curoutput && curoutput->next())
	{	// all right to pop pushed files up to stdout
		out = curoutput;
		curoutput = (OutPut *)curoutput->next();
		out->unlink();
		delete out;
	}
	else 	// but not to pop the last entry
		printe(ERR_internal, E_FATAL, "popout", __LINE__);
}


// write a message to the log file - the message looks like
// the command line interface, even if the gui is running.
// log_msg puts out a comment character ('#') before each line of
// output so the log file can be reused as a script
// len gives the upper bound on the size of the message, which is
// used to ensure that the buffer passed to vsprintf is big enough

static void
write_log(int len, const char *fmt, Severity sev, va_list ap)
{
	static char	lastch = '\n';	// if last output ended in newline,
					// start off with '#'
	static char	*buf = 0;
	static int	max_len = 0;

	register char	*str;
	register char	c;

	// strlen redundant (but harmless) for cli but necessary for gui
	len += strlen(fmt) + 1;

	if (len > max_len)
	{
		max_len = len + 64;	// add a little extra for growth
		delete buf;
		buf = new char[max_len];
	}

	if (lastch == '\n')
		putc('#', log_file);
	if (sev > E_NONE)
		fputs(get_label(sev), log_file);

	if (vsprintf(buf, fmt, ap) >= max_len)
		interface_error("log_msg", __LINE__);

	// walk through the message and put a '#' after every new-line
	// this ensures that multi-line messages are properly commented
	str = buf;
	while((c = *str++) != '\0')
	{
		putc(c, log_file);
		if (c == '\n' && *str)
			putc('#', log_file);
	}
	lastch = *(str - 2);
}

void
log_msg(int len, Msg_id mid, Severity sev ...)
{
	va_list		ap;
	const char	*fmt;

	// don't log the output if it is being redirected
	// or if the message produces no output
	// note that error messages go to stderr and are not affected by
	// redirection
	if (!log_file || mid == MSG_prompt || mid == MSG_input_line
		|| (curoutput->fp != stdout && Mtable.msg_class(mid) != MSGCL_error)
		|| (fmt = Mtable.format(mid)) == 0)
		return;
	va_start(ap, sev);
	write_log(len, fmt, sev, ap);
	va_end(ap);
}

void
log_msg(int len, Msg_id mid, Severity sev, va_list ap)
{
	const char	*fmt;

	// don't log the output if it is being redirected
	// or if the message produces no output
	// note that error messages go to stderr and are not affected by
	// redirection
	if (!log_file || mid == MSG_prompt || mid == MSG_input_line
		|| (curoutput->fp != stdout && Mtable.msg_class(mid) != MSGCL_error)
		|| (fmt = Mtable.format(mid)) == 0)
		return;

	write_log(len, fmt, sev, ap);
}

#if DEBUG
void
dprintm(char *fmt ...)
{
	va_list		ap;

	if (PrintaxGenNL)
	{
		fprintf(curoutput->fp, "\n");
		if (log_file)
			fprintf(log_file, "\n");
		PrintaxGenNL = 0;
	}
	va_start(ap, fmt);
	if (log_file)
	{
		// we assume BUFSIZ is big enough - this is
		// only for -DDEBUG msgs
		write_log(BUFSIZ, fmt, E_NONE, ap);
		fflush(log_file);
	}
	else
	{
		PrintaxSpeakCount++;
		vfprintf(curoutput->fp, fmt, ap);
	}
	va_end(ap);
}
#endif
