#ident	"@(#)ksh93:src/lib/libast/misc/state.c	1.1"
#pragma prototyped

static const char id[] = "\n@(#)ast (AT&T Bell Laboratories) 07/17/95\0\n";

#include <ast.h>

#undef	strcmp

#if _DLL_INDIRECT_DATA && !_DLL
static _Ast_info_t	ast_data =
#else
_Ast_info_t		ast =
#endif

{
	{ 0, 0 },
	0,
	0,
	0,
	0,
	0,
	0,
	strcmp
};

#if _DLL_INDIRECT_DATA && !_DLL
_Ast_info_t		ast = &ast_data;
#endif
