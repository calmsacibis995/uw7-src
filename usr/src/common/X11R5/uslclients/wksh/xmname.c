
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:xmname.c	1.1"

#include	<errno.h>
#include	"defs.h"
#include	"sym.h"
#include	"builtins.h"
#include	"test.h"
#include	"timeout.h"
#include	"history.h"
#include	"wextra.h"
#include	"xmextra.h"

#define CONSTCHAR (const char *)

#define VALPTR(x)	((char*)x)

WK_EXTRA_FUNCS
WK_TK_EXTRA_FUNCS

const struct name_value wk_functions[] = {
	WK_EXTRA_TABLE
	WK_TK_EXTRA_TABLE
	{ e_nullstr, NULL, 0 }
};

const struct name_value wk_variables[] = {
	WK_EXTRA_VAR
	WK_TK_EXTRA_VAR
	{ e_nullstr, NULL, 0 }
};

const struct name_value wk_aliases[] = {
	WK_EXTRA_ALIAS
	WK_TK_EXTRA_ALIAS
	{ e_nullstr, NULL, 0 }
};

wk_libinit()
{
	funcload(wk_functions);
	varload(wk_variables);
	aliasload(wk_aliases);
}
