#ident	"@(#)debugger:libcmd/common/Keyword.C	1.20"

#include "Keyword.h"
#include "Parser.h"
#include "Scanner.h"
#include "str.h"

Keyword keywordtable[] = {
//	str		op		opts	arg_type[MAX_ARGS]

	"!",		SHELL,		0,	{ OP_REQ|OP_COMMAND, 0, 0, 0, 0 },
	"alias",	ALIAS,		"r",	{ 0, OP_NAME, OP_TOKENS, 0, 0 },
	"break",	BREAK,		0,	{ 0, 0, 0, 0, 0 },
	"cancel",	CANCEL,		"p:2",
		{ 0, OP_OPT|OP_PROCS, OP_LIST|OP_SIGNAL, 0, 0 },
	"cd",		CD,		0,	{ 0, OP_FILE, 0, 0, 0},
	"change",	CHANGE,		"eqvxp:2c:1",
		{ OP_OPT|OP_NUMBER, OP_OPT|OP_PROCS, OP_REQ|OP_EVENT, 
			OP_LIST|OP_SPECIAL, OP_CMD_BLOCK},
	"continue",	CONTINUE,	0,	{ 0, 0, 0, 0, 0 },
#ifdef SDE_SUPPORT
	"create",	CREATE,		"dref:2l:3s:4",
		{ OP_COMMAND, OP_OPT|OP_CHARSTAR, OP_OPT|OP_LOCATION, OP_OPT|OP_RAWSTRING, 0 },
#else
	"create",	CREATE,		"dref:2l:3",
		{ OP_COMMAND, OP_OPT|OP_CHARSTAR, OP_OPT|OP_LOCATION, 0, 0 },
#endif
	"delete",	DELETE,		"ap:2",
		{ 0, OP_OPT|OP_PROCS, OP_LIST|OP_EVENT, 0, 0 },
	"dis",		DIS,		"p:2c:3nfs",
		{ 0, OP_OPT|OP_PROCS, OP_OPT|OP_NUMBER, OP_LOCATION, 0 },
	"disable",	DISABLE,	"ap:2",
		{ 0, OP_OPT|OP_PROCS, OP_LIST|OP_EVENT, 0, 0 },
	"dump",		DUMP,		"p:2c:3b",
		{ 0, OP_OPT|OP_PROCS, OP_OPT|OP_NUMBER, 
			OP_REQ|OP_EXPR, 0 },
	"enable",	ENABLE,		"ap:2",
		{ 0, OP_OPT|OP_PROCS, OP_LIST|OP_EVENT, 0, 0 },
	"events",	EVENTS,		"p:2",
		{ 0, OP_OPT|OP_PROCS, OP_LIST|OP_EVENT, 0, 0 },
#if EXCEPTION_HANDLING
	"exception",	EXCEPTION,	"dip:2q",	
		{ 0, OP_OPT|OP_PROCS, OP_EH_TYPE, OP_EXPR, OP_CMD_BLOCK },
#endif
	"export",	EXPORT,		0,	{ 0, 0, OP_REQ|OP_UNAME, 0, 0 },
	"fc",		FC,		0,	{ 0, 0, OP_COMMAND, 0, 0 }, 
	"functions",	FUNCTIONS,	"sp:2f:3o:4",
		{ 0, OP_OPT|OP_PROCS, OP_OPT|OP_FILE, OP_OPT|OP_FILE, OP_CHARSTAR }, 
#ifdef SDE_SUPPORT
	"grab",		GRAB,		"s:1c:2l:3f:5",
		{ OP_OPT|OP_RAWSTRING, OP_OPT|OP_FILE, OP_OPT|OP_FILE, OP_REQ|OP_LIST|OP_FILE, OP_OPT|OP_CHARSTAR},
#else
	"grab",		GRAB,		"c:2l:3f:5",
		{ 0, OP_OPT|OP_FILE, OP_OPT|OP_FILE, OP_REQ|OP_LIST|OP_FILE, OP_OPT|OP_CHARSTAR},
#endif
	"halt",		HALT,		"p:2",	{ 0, OP_OPT|OP_PROCS, 0, 0, 0 },
	"help",		HELP,		0,	{ 0, 0, OP_CHARSTAR, 0, 0 },
	"if",		IF,		0,
		{OP_REQ|OP_EXPR, OP_REQ|OP_CMD_BLOCK, OP_CMD_BLOCK, 0, 0},
	"input",	INPUT,		"np:2r:3",	{ 0, OP_OPT|OP_CHARSTAR, OP_OPT|OP_CHARSTAR, OP_REQ|OP_STRING, 0 },
	"jump",		JUMP,		"p:2",
		{ 0, OP_OPT|OP_PROCS, OP_REQ|OP_LOCATION, 0, 0 },
	"kill",		KILL,		"p:2",
		{ 0, OP_OPT|OP_PROCS, OP_SIGNAL, 0, 0},
	"list",		LIST,		"p:2c:3",	
			{ 0, OP_OPT|OP_PROCS, OP_OPT|OP_NUMBER, OP_LOCATION, 0},
	"logon",	LOGON,		0,	{ 0, 0, OP_FILE, 0, 0},
	"logoff",	LOGOFF,		0,	{ 0, 0, 0, 0, 0},
	"map",		MAP,		"p:2",	{ 0, OP_OPT|OP_PROCS, 0, 0, 0 },
	"onstop",		ONSTOP,		"p:2",
		{ 0, OP_OPT|OP_PROCS, 0, OP_CMD_BLOCK, 0 },
	// gui only
	"pending",	PENDING,	"p:2",	{ 0, OP_OPT|OP_PROCS, 0, 0 },
	// gui only
	"pfiles",	PFILES,		"p:2",	{ 0, OP_OPT|OP_PROCS, 0, 0 },
	// gui only
	"ppath",	PPATH,		"p:2",	{ 0, OP_OPT|OP_PROCS, OP_REQ|OP_FILE, OP_FILE },
	"print",	PRINT,		"bp:2f:3v",
		{ 0, OP_OPT|OP_PROCS, OP_OPT|OP_STRING, OP_REQ|OP_LIST|OP_EXPR, 0},
	"ps",		PS,		"p:2",	{ 0, OP_OPT|OP_PROCS, 0, 0 },
	"pwd",		PWD,		0,	{ 0, 0, 0, 0, 0},
	"rename",	RENAME,		0,	
		{ 0, 0, OP_REQ|OP_FILE, OP_REQ|OP_FILE, 0},
	"quit",		QUIT,		0,	{ 0, 0, 0, 0, 0 },
	"regs",		REGS,		"p:2",	{ 0, OP_OPT|OP_PROCS, 0, 0, 0 },
	"release",	RELEASE,	"p:2s",	{ 0, OP_OPT|OP_PROCS, 0, 0, 0 },
	"run",		RUN,		"p:2bfru:3",	
		{ 0, OP_OPT|OP_PROCS, OP_OPT|OP_LOCATION, 0, 0 },
	"script",	SCRIPT,		"q",	{ 0, 0, OP_REQ|OP_FILE, 0, 0},
	"set",		SET,		"p:2v",
		{ 0, OP_OPT|OP_PROCS, OP_REQ|OP_SET_EXPR, OP_REQ|OP_LIST|OP_EXPR },
	"signal",	SIGNAL,		"diqp:2",
		{ 0, OP_OPT|OP_PROCS, OP_LIST|OP_SIGNAL, OP_CMD_BLOCK, 0 },
	"stack",	STACK,		"f:1p:2c:3a:4s:5",
		{ OP_OPT|OP_NUMBER, OP_OPT|OP_PROCS, OP_OPT|OP_NUMBER, 
			OP_OPT|OP_HEXNUM, OP_OPT|OP_HEXNUM},
	"step",		STEP,		"p:2c:3bofiq",
		{ 0, OP_OPT|OP_PROCS, OP_OPT|OP_NUMBER, 0, 0 },
	"stop",		STOP,		"qp:2c:1",
		{ OP_OPT|OP_NUMBER, OP_OPT|OP_PROCS, OP_STOP, OP_CMD_BLOCK, 0 },
	"syscall",	SYSCALL,	"qp:2exc:1",
		{ OP_OPT|OP_NUMBER, OP_OPT|OP_PROCS, OP_LIST|OP_SYSCALL, OP_CMD_BLOCK, 0 },
	"symbols",	SYMBOLS,		"p:2o:3n:4dfgltuv",
		{ 0, OP_OPT|OP_PROCS, OP_OPT|OP_FILE, OP_OPT|OP_FILE,
			OP_CHARSTAR},
	"version",	VERSION,	0,	{ 0, 0, 0, 0, 0 },
	"whatis",	WHATIS,		"p:2",	{ 0, OP_OPT|OP_PROCS, OP_REQ|OP_EXPR, 0, 0 },
	"while",	WHILE,		0,
		{ OP_REQ|OP_EXPR, OP_REQ|OP_CMD_BLOCK, 0, 0, 0 },
	0,		NoOp,		0,	{ 0, 0, 0, 0, 0 },
};

char *else_str;

static void
init_table()
{
	register Keyword *p = keywordtable;
	for ( ; p->str ; p++ )
		p->str = str(p->str);	// initialize str() table
	else_str = str("else");		// initialize "else" pointer
}

static int init_done = 0;
	
Keyword *
keyword( register const char *word )
{
	if ( !init_done )
	{
		init_table();
		init_done = 1;
	}

	register Keyword *p = keywordtable;
	for ( ; p->str ; p++ )
		if ( p->str == word )	// since keywords are in str() table
			return p;

	return 0;
}

Keyword *
keyword( register Op op )	// look up op
{
	if ( !init_done )
	{
		init_table();
		init_done = 1;
	}

	register Keyword *p = keywordtable;
	for ( ; p->str ; p++ )
		if( p->op == op )
			return p;

	return 0;
}
