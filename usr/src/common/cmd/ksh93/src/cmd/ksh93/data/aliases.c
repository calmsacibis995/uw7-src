#ident	"@(#)ksh93:src/cmd/ksh93/data/aliases.c	1.1"
#pragma prototyped
#include	<ast.h>
#include	<signal.h>
#include	"FEATURE/options"
#include	"FEATURE/dynamic"
#include	"shtable.h"
#include	"name.h"

/*
 * This is the table of built-in aliases.  These should be exported.
 */

const struct shtable2 shtab_aliases[] =
{
#ifdef SHOPT_FS_3D
	"2d",		NV_NOFREE|NV_EXPORT,	"set -f;_2d",
#endif /* SHOPT_FS_3D */
	"autoload",	NV_NOFREE|NV_EXPORT,	"typeset -fu",
	"command",	NV_NOFREE|NV_EXPORT,	"command ",
	"fc",		NV_NOFREE|NV_EXPORT,	"hist",
	"float",	NV_NOFREE|NV_EXPORT,	"typeset -E",
	"functions",	NV_NOFREE|NV_EXPORT,	"typeset -f",
	"hash",		NV_NOFREE|NV_EXPORT,	"alias -t --",
	"history",	NV_NOFREE|NV_EXPORT,	"hist -l",
	"integer",	NV_NOFREE|NV_EXPORT,	"typeset -i",
	"nameref",	NV_NOFREE|NV_EXPORT,	"typeset -n",
	"nohup",	NV_NOFREE|NV_EXPORT,	"nohup ",
	"r",		NV_NOFREE|NV_EXPORT,	"hist -s",
	"redirect",	NV_NOFREE|NV_EXPORT,	"command exec",
	"times",	NV_NOFREE|NV_EXPORT,	"{ { time;} 2>&1;}",
	"type",		NV_NOFREE|NV_EXPORT,	"whence -v",
#ifdef SIGTSTP
	"stop",		NV_NOFREE|NV_EXPORT,	"kill -s STOP",
	"suspend", 	NV_NOFREE|NV_EXPORT,	"kill -s STOP $$",
#endif /*SIGTSTP */
	"",		0,			(char*)0
};

