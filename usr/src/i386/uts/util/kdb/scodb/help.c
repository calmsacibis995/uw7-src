#ident	"@(#)kern-i386:util/kdb/scodb/help.c	1.1.1.2"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 * Modification History:
 *
 *	L000	nadeem@sco.com	1jun93
 *	- add some help text for debugger functions.
 *	L001	scol!harveyt	17 Aug 94
 *	- fixed help command with an argument to not hang the machine, but
 *	  to print the help for the given command only.
 */

#include	"dbg.h"

/*
 * Maximum size of each indented description line.
 */

#define MAX_DESCRIPT_SIZE	71

/*
 * Help text
 */

char *scodb_cmdhelp[] = {

/*
 * Alias
 */

"alias [<word> [<new command>]]",
"Set or list command aliases.",

/*
 * Breakpoint clear
 */

"bc [<address> | * | DR<n>]",
"Breakpoint clear.  The '*' argument is used to clear all breakpoints.  Debug \
registers are cleared with the 'DR<n>' form where <n> is 0 to 3.",

/*
 * Breakpoint list
 */

"bl",
"Breakpoint list - lists all breakpoints.",

/* 
 * Breakpoint set
 */

"bp <address>\n\
bp [ en | dis ] <address>\n\
bp [ [r|w|i][b|s|l] | x] <address> [<condition> <value>]",
"Breakpoint set.  The second form is used to (en)able or (dis)able a \
previously set breakpoint.  The third form is used to set debug \
breakpoints - the characters of the second argument indicate (r)ead, \
(w)rite, (i)o, and also (b)yte, (s)hort, (l)ong.  \
Alternatively, e(x)ecute can be specified.  For example, \"bp rl <address>\" \
breaks on the read of a long.",

/*
 * Change memory
 */

"c[b|s|l] <address>",
"Change memory.  Enters a mode which allows changing of memory.  Change \
memory by navigating to the appropriate word and \
entering hex digits.  The 'h' key moves to the previous word, 'j' moves to the \
next line, 'k' moves to the previous line, and 'l' moves to the next word.  \
<tab> also moves to the next word.  <backspace> moves left \
one digit and <space> moves right one digit.  \
<escape> allows entry of an expression which is evaluated and \
entered into the current word.  'u' un-does the last digit entered \
and 'U' un-does changes to the entire word.  <delete>, <enter> and 'q' \
all quit the mode.  The optional second character of the command \
indicates whether memory is to be displayed as (b)ytes, (s)horts or (l)ongs.",

/*
 * Dump memory
 */

"d[b|s|l|n] <address>",
"Dump memory.  Enters a mode which displays memory.  The 'j', <space> \
and <enter> keys move down a line, and 'k' moves up a line.  Control-U \
moves up half a screen and control-D moves down half a screen.  'q' \
and <delete> both quit the mode.  The optional second character of the \
command indicates whether memory is to be displayed as (b)ytes, \
(s)horts, (l)ongs or, in the case of 'n', longs which are displayed \
symbolically where possible.",

/*
 * Declare
 */

"declare <C declaration>",
"Give a system variable a type.  For example, \
\"declare struct proc practive\".  This would allow the evaluation of \
of \"practive->p_ppid\".",

/*
 * New debugger
 */

"newdebug",
"Change the debugger.",

/*
 * Process status
 */

"ps",
"Display process status.",

/*
 * Quit
 */

"q [<address>]",
"Quit debugger.  Optionally, quit to the address specified.",

/*
 * Quit if
 */

"quitif <expression>",
"If expression is true then quit.  Useful as part of the commands to be \
executed on a breakpoint.",

/*
 * Registers
 */

"r [-t]",
"Display system registers.  The optional '-t' argument to the command \
toggles between two different forms of register display.",

/*
 * Single Step
 */

"s [-r]",
"Single-step.  Enters a mode for single stepping.  Use the <space> key to \
step one instruction and 'r' to step till the next ret or iret instruction.  \
At a call instruction, use 'e' to (e)nter the function or 'j' to (j)ump over \
the function (ie: execute the function at full speed and stop after \
the call).  The optional '-r' argument to the command toggles on and off \
the automatic displaying of registers during the single stepping.",

/*
 * Stack backtrace
 */

"stack [-p <proc_slot_or_ptr>] [-l <lwp_ptr>] [-a <esp_to_try>]",
"Stack backtrace.  The optional '-p' argument can be used to specify a \
particular process - either as a proc slot number or as a pointer to \
the proc structure.  The optional '-l' argument can be used to specify a \
particular LWP - as a pointer to the lwp structure.  The optional '-a' \
argument can be used to specify a particular stack address to try \
dumping the stack from.",

/*
 * Structure dump
 */

"struct <structure name> <address>\n\
struct <structure name> [-> <follow-field>] [<display-fields...>] <address>",

"Display a structure.  The second form can be used to display only \
certain fields of the structure, for example \"struct proc p_pidp \
p_ppid practive\".  The '->' is used to specify a field that should \
be followed when displaying the next structure, for example \
\"struct proc -> p_next practive\".  The <follow-field> must always be \
specified before any <display-fields>.",

/*
 * Type
 */

"type <expression>",
"Show the type of a variable or expression.",

/*
 * Unassemble
 */

"{u | dis}  <address>\n\
{u | dis} mode [<mode keywords>]",
"Unassemble.  Enters a mode which unassembles memory.  The <space>, \
<enter> and 'j' keys move down a line, and 'k' moves up a line.  Control-U \
moves up half a screen and control-D moves down half a screen.  '/' \
and '?' search (forward and backward) for a string in the disassembly \
output, with 'n' and 'N' searching for the next and previous occurence of the \
string (the search is bounded within the current function).  'q' and \
<delete> both quit the mode.  The second form of the command is used to \
display or set the current disassembly settings.  The most commonly used \
keywords are 'binary' to display instruction bytes, and 'default' to \
restore default settings.  A '-' preceeding a keyword turns off that keyword.",

/*
 * Unalias
 */

"unalias [ * | <aliases...> ]",
"Unset alias(es).",

/*
 * Undeclare
 */

"undeclare [ * | <variables...> ]",
"Undeclare system variable(s).",

/*
 * Unset variable
 */

"unvar [ * | <variables...> ]",
"Remove debugger variable(s).",

/*
 * Set variable
 */

"var [<name> <initial value>]",
"Create and list debugger variables.",

/*
 * Help
 */

"{? | help}",
"This message.",

/*
 * History Editting
 */

"Command History Editting",
"vi-style history editting is invoked by pressing <escape>.",

/*
 * Expressions
 */

"Expressions",
"Expressions are in 'C' style and include most of the standard binary and \
unary operators.  Note that \"<var>\" is consequently the contents of a \
variable whereas \"&<var>\" is the address of a variable.  The latter \
form is generally required for those commands that expect an address \
argument.  Text symbols are an exception, with \"<func>\" and \"&<func>\" \
both being the address of a function.  The address argument \
passed to many of the debugger commands is actually evaluated \
as a full expression.  If command \
line input is not recognised as a valid command, then it is assumed to be \
an expression.  Thus, the contents of a variable can be examined by typing \
its symbol name on the command line, or an assignment can be made by \
typing \"<var>=<expr>\", both of which are just evaluated as standard \
'C' expressions.  Functions can be called with \"<func>(<args...>)\".",

(char *)0, (char *)0
};

/*
 * Print out a help description, indented by one tab, automatically printing
 * as many words as will fit on each line.  Calculating this within this
 * routine saves the effort of manually indenting the description strings
 * everytime they change.
 */

scodb_help_description(char *string)
{
	int n;
	char *p, *q, c;

	p = string;
	q = p;
	n = strlen(p);

	while (n > MAX_DESCRIPT_SIZE) {
		q = p + MAX_DESCRIPT_SIZE;
		while (q > p && *q != ' ')
			q--;
		c = *q;
		*q = '\0';
		printf("\t%s\n", p);
		*q = c;
		while (*q && *q == ' ')
			q++;
		n -= (q - p);
		p = q;
	}
	printf("\t%s\n\n", q);
}

/*
*	help command
*/
NOTSTATIC
c_help(c, v)
	int c;
	char **v;
{
	char **p;

	p = scodb_cmdhelp;

	while (*p) {
		/*
		 * Print out the command synopsis followed by the
		 * idented description.
		 */
		printf("%s\n\n", *p);
		scodb_help_description(*(p+1));
		p += 2;

	}

	return DB_CONTINUE;
}

