#ifndef _Keyword_h
#define _Keyword_h
#ident	"@(#)debugger:libcmd/common/Keyword.h	1.6"

// The Keyword struct holds information about the debugger's commands -
// their names (str), option letters (opts) and arguments (arg_type)
// The opts string looks like a getopt(3C) string (letter or letter:
// for options that take an argument) with the addition of a number
// after the : to tell where (which Opnd in the Node) to put the argument
// for example "ap:2", -a takes no argument, -p takes an argument that
// goes in Node.second

#define	MAX_ARGS	5	// same as number of operands in a Node

//enum Op;
#include "Parser.h"

struct Keyword {
	const char	*str;		// command name
	Op		op;		// command type
	const char	*opts;		// valid options
	long		arg_type[MAX_ARGS];  // argument type, corresponds
					// to Opnd entries in a Node
};

extern Keyword keywordtable[];
extern char *else_str;

#ifndef __cplusplus
overload	keyword;
#endif
extern Keyword	*keyword(const char *);	// lookup keyword by name
extern Keyword	*keyword(Op);		// lookup keyword by op

// The Op_* defines are or'd together to form the entries in the
// arg_type array

#define	OP_UNSED	0
#define OP_REQ		0x00000001	// argument is required
#define OP_OPT		0x00000002	// comes from an option i.e. -p foo
					// instead of alias foo
#define OP_LIST		0x00000004	// takes a list of the same type of
					// arguments

// different argument types - only one of these allowed
#define OP_NAME		0x00000010
#define OP_TOKENS	0x00000020
#define OP_FILE		0x00000040
#define OP_LOCATION	0x00000080
#define OP_COMMAND	0x00000100	// shell type command with pipes, etc.
#define OP_SIGNAL	0x00000400
#define OP_NUMBER	0x00000800
#define OP_EXPR		0x00001000
#define OP_REGEXP	0x00002000
#define OP_SYSCALL	0x00004000
#define OP_CMD_BLOCK	0x00008000	// debugger command or block ({})
#define OP_EVENT	0x00010000
#define OP_PROCS	0x00020000
#define OP_STRING	0x00040000	// quoted string suitable to
					// pass to printf (escapes
					// resolved)
#define OP_STOP		0x00080000
#define OP_CHARSTAR	0x00100000
#define OP_SET_EXPR	0x00200000	// for set command
#define OP_SPECIAL	0x00400000	// requires special handling
#define OP_UNAME	0x00800000	// user-defined debug var
#define OP_HEXNUM	0x01000000	// hexadecimal number
#define OP_RAWSTRING	0x02000000	// string as typed (no quotes)
#if EXCEPTION_HANDLING
#define OP_EH_TYPE	0x04000000	// throw and/or catch for exception command
#endif

#define OP_TYPE_MASK	0xfffffff0

#endif // _Keyword_h
