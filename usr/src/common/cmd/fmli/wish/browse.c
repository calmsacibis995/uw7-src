/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:wish/browse.c	1.19.3.5"

#include <stdio.h>
#include "wish.h"
#include "token.h"
#include "slk.h"
#include "actrec.h"
#include "terror.h"
#include "ctl.h"
#include	"moremacros.h"

#define BROWSE	1
#define PROMPT	2

/*
 * Caution: MAX_ARGS is defined in other files and should ultimately reside 
 * in wish.h
 */
#define MAX_ARGS	25	

extern int Arg_count;
extern char *Args[];
extern char *tok_to_cmd();	/* abs k16 */
/* the operation pending during Browse mode */
char *Pending_op;
/* dmd TCB */
static char *Pending_objtype, *Pending_argv[MAX_ARGS+2];
/* dmd TCB
struct slk *Pending_slks;
*/
/* dmd TCB */
static int Pending_type;

static char	name_string[] = "Enter the new object name: ";
static char	name_stringid[] =":358";
static char	desc_string[] = "Enter the new description: ";
static char	desc_stringid[] =":359";

/* a flag indicating whether or not FMLI is currently in browse mode */
int Browse_mode = 0;

enter_browse(op, objtype, argv)
char *op, *objtype, *argv[];
{
	register int i;
/* dmd TCB
	extern struct slk Browslk[], Defslk[];
*/

	Browse_mode++;
	save_browse(op, objtype, argv);
	setslks(NULL, 0);
	mess_temp(gettxt(":357",
	  "Open or navigate to the destination folder and press SELECT"));
	Pending_type = BROWSE;
}

enter_getname(op, objtype, argv)
char *op, *objtype, *argv[];
{
	register int i;
	token namevalid();

	save_browse(op, objtype, argv);
	Pending_type = PROMPT;
	get_string(namevalid, strCcmp(Pending_op, "redescribe") ? gettxt(name_stringid,name_string) : gettxt(desc_stringid,desc_string), "", 0, FALSE, Pending_op, Pending_op);
}

static token
namevalid(s, t)
char *s;
token t;
{
	register int i;
	char *errstr;

	if (t == TOK_CANCEL) {
	    if ( Browse_mode )
		glob_browse_cancel();
	    else
		Pending_op = NULL;
	    return TOK_NOP;
	}

	if (strCcmp(Pending_op, "create") == 0) {
		if (namecheck(Pending_argv[0], s, NULL, &errstr, TRUE) == FALSE) {
			get_string(namevalid, gettxt(name_stringid,name_string), "", 0, FALSE, Pending_op, Pending_op);
			(void)mess_err(errstr);		/* abs s15 */
			return TOK_NOP;
		}
	} else if (strCcmp(Pending_op, "redescribe") != 0) {
		if (namecheck(Pending_argv[1], s, Pending_objtype, &errstr, TRUE)==FALSE) {
			get_string(namevalid, gettxt(name_stringid,name_string), "", 0, FALSE, Pending_op, Pending_op);
			(void)mess_err(errstr);		/* abs s15 */
			return TOK_NOP;
		}
	}
/*
 *	Notice that redescribe falls thru the above if block without ever
 *	calling namecheck!
 */

	for (i = 0; Pending_argv[i]; i++)
		;
	Pending_argv[i] = strsave(s);
	Pending_argv[i+1] = NULL;
	glob_select();
	return(TOK_NOP);
}

glob_select()
{
	register int i, prevtype = Pending_type;
	bool canselect;

	if (Pending_type == BROWSE) {
		if (ar_ctl(ar_get_current(), CTISDEST, &canselect) == FAIL || !canselect) {
			(void)mess_err( gettxt(":283","This frame can not be used as a destination") ); /* abs s15 */
			return;
		}
		for (i = 0; Pending_argv[i]; i++)
			;

		Pending_argv[i] = strsave(ar_get_current()->path);
		Pending_argv[i+1] = NULL;
	}

	if (Pending_op)
	if (strcmp(Pending_op, "redescribe") == 0) {
		working(TRUE);
		redescribe(&Pending_argv[0]);
	} else if (strcmp(Pending_op, "create") == 0) {
		working(TRUE);
		Create_create(&Pending_argv[0]);
	} else {
		mess_perm(NULL);
		working(TRUE);
		(void) objopv(Pending_op, Pending_objtype, Pending_argv);
	}

	if (Pending_type == prevtype)
		glob_browse_cancel();
	ar_checkworld(TRUE);
}

int
glob_browse_cancel()
{
	Browse_mode = 0;
	ar_setslks(ar_get_current()->slks, 0);
	if (Pending_op) {
		register int i;

		free(Pending_op);
		Pending_op = NULL;
		if (Pending_objtype) {
			free(Pending_objtype);
			Pending_objtype = NULL;
		}
		for (i = 0; Pending_argv[i]; i++) {
			free(Pending_argv[i]);
			Pending_argv[i] = NULL;
		}
	}
}

token
glob_mess_nosrc(t)
token t;
{
    bool b;
    char *cmd_name = tok_to_cmd(t); /* abs k16 */

    if  (cmd_name)		    /* abs k16 */

        switch(t) {

           case TOK_OPEN:
                  (void)mess_err( gettxt(":284","Can't open from this menu") );
                  break;

           case TOK_CLOSE:
                  (void)mess_err( gettxt(":285","Can't cancel from this menu") );
                  break;

           case TOK_CLEANUP:
                  (void)mess_err( gettxt(":286","Can't cleanup from this menu") );
                  break;

           case TOK_COPY:
                  (void)mess_err( gettxt(":287","Can't copy from this menu") );
                  break;

           case TOK_CREATE:
                  (void)mess_err( gettxt(":288","Can't create from this menu") );
                  break;

           case TOK_DELETE:
                  (void)mess_err( gettxt(":289","Can't delete from this menu") );
                  break;

           case TOK_DISPLAY:
                  (void)mess_err( gettxt(":290","Can't display from this menu") );
                  break;

           case TOK_ENTER:
                  (void)mess_err( gettxt(":291","Can't enter from this menu") );
                  break;

           case TOK_LOGOUT:
                  (void)mess_err( gettxt(":292","Can't exit from this menu") );
                  break;

           case TOK_HELP:
                  (void)mess_err( gettxt(":293","Can't help from this menu") );
                  break;

           case TOK_ORGANIZE:
                  (void)mess_err( gettxt(":294","Can't organize from this menu") );
                  break;

           case TOK_PRINT:
                  (void)mess_err( gettxt(":295","Can't print from this menu") );
                  break;

           case TOK_SREPLACE:
                  (void)mess_err( gettxt(":296","Can't redescribe from this menu") );
                  break;

           case TOK_REFRESH:
                  (void)mess_err( gettxt(":297","Can't refresh from this menu") );
                  break;

           case TOK_REPLACE:
                  (void)mess_err( gettxt(":298","Can't rename from this menu") );
                  break;

           case TOK_RUN:
                  (void)mess_err( gettxt(":299","Can't run from this menu") );
                  break;

           case TOK_SHOW_PATH:
                  (void)mess_err( gettxt(":300","Can't show path from this menu") );
                  break;

           case TOK_UNDELETE:
                  (void)mess_err( gettxt(":301","Can't undelete from this menu") );
                  break;

           case TOK_REREAD:
                  (void)mess_err( gettxt(":302","Can't update from this menu") );
                  break;

           case TOK_DONE:
                  (void)mess_err( gettxt(":303","Can't save from this menu") );
                  break;
           case TOK_RETURN:
                  (void)mess_err( gettxt(":304","Can't press enter in this menu") );
                  break;
           case TOK_CANCEL:
                  (void)mess_err( gettxt(":285","Can't cancel from this menu") );
                  break;
           default:
                  (void)mess_err( gettxt(":305","Command is not valid in this case") );

        }

        /* abs s15 */
    else
	beep();			    /* abs k16 */
    return(t);
}

static
save_browse(op, objtype, argv)
char *op, *objtype, *argv[];
{
	register int i;

	Pending_op = strsave(op);
	Pending_objtype = strsave(objtype);
/* dmd TCB
	Pending_slks = ar_get_current()->slks;
*/
	for (i = 0; argv[i]; i++)
		Pending_argv[i] = strsave(argv[i]);
	Pending_argv[i] = NULL;
}

