#ident	"@(#)ksh93:src/cmd/ksh93/data/keywords.c	1.1"
#pragma prototyped
#include	<hash.h>
#include	"shtable.h"
#include	"shlex.h"
#include	"FEATURE/options"

/*
 * table of reserved words in shell language
 * This list must be in in ascii sorted order
 */

const Shtable_t shtab_reserved[] =
{
		"!",		NOTSYM,
		"[[",		BTESTSYM,
		"case",		CASESYM,
		"do",		DOSYM,
		"done",		DONESYM,
		"elif",		ELIFSYM,
		"else",		ELSESYM,
		"esac",		ESACSYM,
		"fi",		FISYM,
		"for",		FORSYM,
		"function",	FUNCTSYM,
		"if",		IFSYM,
		"in",		INSYM,
		"select",	SELECTSYM,
		"then",		THENSYM,
		"time",		TIMESYM,
		"until",	UNTILSYM,
		"while",	WHILESYM,
		"{",		LBRACE,
		"}",		RBRACE,
		"",		0,
};

const char	e_endoffile[]	= "end of file";
const char	e_endoffile_id[]	= ":117";
const char	e_newline[]	= "newline";
const char	e_newline_id[]	= ":118";

