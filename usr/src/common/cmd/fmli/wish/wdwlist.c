/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:wish/wdwlist.c	1.9.3.3"

#include <stdio.h>
#include "wish.h"
#include "token.h"
#include "slk.h"
#include "actrec.h"
#include "terror.h"
#include "ctl.h"
#include "menudefs.h"
#include "vtdefs.h"
#include "moremacros.h"
#include "helptext.h"	/* dmd s15 */

extern	menu_id menu_make();
extern	int 	Vflag;	/* dmd s15 */

static int
list_ctl(a, cmd, arg1, arg2, arg3, arg4, arg5, arg6)
struct actrec *a;
int cmd, arg1, arg2, arg3, arg4, arg5, arg6;
{
	struct actrec *menline_to_ar();

	if (cmd == CTGETARG) {
		int line;
		struct actrec *rec, *menline_to_ar();

		(void) menu_ctl(a->id, CTGETPOS, &line);
		rec = menline_to_ar(line);
		**((char ***)(&arg1)) = strsave(rec->path);
		return(SUCCESS);
	} else
		return(menu_ctl(a->id, cmd, arg1, arg2, arg3, arg4, arg5, arg6));
}

static token
list_odsh(a, t)
struct actrec *a;
token t;
{
	int line;
	struct actrec *menline_to_ar();
	extern int Arg_count;

	t = menu_stream(t);

	if ((t == TOK_OPEN || t == TOK_ENTER) && Arg_count <= 1) {
		(void) menu_ctl(a->id, CTGETPOS, &line);
		ar_current(menline_to_ar(line), TRUE); /* abs k15 */
		t = TOK_NOP;
	} else if (t == TOK_CANCEL) {
		ar_backup();
		t = TOK_NOP;
	} else if (t == TOK_NEXT)
		t = TOK_NOP;	/* returned when new item is navigated to */
	return(t);
}


static int
list_help(a)
struct actrec *a;
{
/* dmd s15
 * added Vflag and help when not Vflag for Open Frames
 */
	if (Vflag)
	    return(objop("OPEN", "TEXT", "$VMSYS/OBJECTS/Text.mfhelp", "T.m.frmlist", gettxt(":332","Frame List"), NULL));
	else
	    return(generic_help( gettxt(":333","Open Frames"), HT_FRM_LIST));
}

int
list_create()
{
	struct actrec a;
	vt_id vid;

	struct menu_line ar_menudisp();

	a.id = (int) menu_make(-1, gettxt(":333","Open Frames"), VT_NONUMBER | VT_CENTER, 
		VT_UNDEFINED, VT_UNDEFINED, 0, 0, ar_menudisp, NULL);
	ar_menu_init(&a);
	a.fcntbl[AR_CTL] = list_ctl;
	a.fcntbl[AR_HELP] = list_help;
	a.fcntbl[AR_ODSH] = (int (*)())list_odsh; /* added cast  abs 9/9/88 */
	a.flags = 0;
 
	return(ar_current(ar_create(&a), FALSE)); /* abs k15 */
}
