#ident	"@(#)wksh:xksrc/extra.h	1.1"

/*	Copyright (c) 1990, 1991 AT&T and UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T    */
/*	and UNIX System Laboratories, Inc.			*/
/*	The copyright notice above does not evidence any       */
/*	actual or intended publication of such source code.    */


#ifdef TEST_CODE

#define BASE_EXTRA_FUNCS \
int do_libload(); \
int do_cmdload(); \
int do_sizeof(); \
int do_findsym(); \
int do_field_get(); \
int do_finddef(); \
int do_deflist(); \
int do_define(); \
int do_structlist(); \
int do_field_comp(); \
int do_deref(); \
int do_call(); \
int do_struct(); \
int do_typedef(); \
int do_symbolic(); \

#define BASE_EXTRA_TABLE \
	{ "call", VALPTR(do_call), N_BLTIN|BLT_FSUB }, \
	{ "field_comp", VALPTR(do_field_comp), N_BLTIN|BLT_FSUB }, \
	{ "field_get", VALPTR(do_field_get), N_BLTIN|BLT_FSUB }, \
	{ "libload", VALPTR(do_libload), N_BLTIN|BLT_FSUB }, \
	{ "cmdload", VALPTR(do_cmdload), N_BLTIN|BLT_FSUB }, \
	{ "sizeof", VALPTR(do_sizeof), N_BLTIN|BLT_FSUB }, \
	{ "findsym", VALPTR(do_findsym), N_BLTIN|BLT_FSUB }, \
	{ "finddef", VALPTR(do_finddef), N_BLTIN|BLT_FSUB }, \
	{ "deflist", VALPTR(do_deflist), N_BLTIN|BLT_FSUB }, \
	{ "define", VALPTR(do_define), N_BLTIN|BLT_FSUB }, \
	{ "structlist", VALPTR(do_structlist), N_BLTIN|BLT_FSUB }, \
	{ "deref", VALPTR(do_deref), N_BLTIN|BLT_FSUB }, \
	{ "struct", VALPTR(do_struct), N_BLTIN|BLT_FSUB }, \
	{ "typedef", VALPTR(do_typedef), N_BLTIN|BLT_FSUB }, \
	{ "symbolic", VALPTR(do_symbolic), N_BLTIN|BLT_FSUB }, \

#define BASE_EXTRA_VAR_DECL \
	extern char *xk_ret_buf; \
	extern struct Bfunction xk_prdebug; \

#define BASE_EXTRA_VAR \
	"RET",	(char*)(&xk_ret_buf),	N_FREE|N_INDIRECT|N_BLTNOD, \
	"PRDEBUG",	(char*)(&xk_prdebug),	N_FREE|N_INTGER|N_BLTNOD, \

#define BASE_EXTRA_ALIAS \
	"args",		"setargs \"$@\"",	N_FREE|N_EXPORT,

#define EXTRA_FUNCS BASE_EXTRA_FUNCS
#define EXTRA_TABLE BASE_EXTRA_TABLE
#define EXTRA_VAR_DECL BASE_EXTRA_VAR_DECL
#define EXTRA_VAR BASE_EXTRA_VAR
#define EXTRA_ALIAS BASE_EXTRA_ALIAS

#else /* not TEST_CODE, normal ksh */
#define EXTRA_FUNCS
#define EXTRA_TABLE
#define EXTRA_VAR_DECL
#define EXTRA_VAR
#define EXTRA_ALIAS
#endif /* TEST_CODE */
