/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:wish/wdwmgmt.c	1.15.3.4"

#include <stdio.h>
#include <string.h>
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
extern	int	Vflag;

#define NMUDGES	4

extern char *gettxt();

static struct menu_line Mgmt_lines[NMUDGES] = {
        { "list",	"list all frames",	0},
        { "move",	"move a frame",		0},
        { "reshape",	"reshape a frame",	0},
        { NULL,		NULL,			0}
};

static struct i18n_menu_line {
        char *word;
        char *wordid;
        char *string;
        char *stringid;
        } I18n_Mgmt_lines[NMUDGES] = {
	               { "list",    ":334", "list all frames", ":335" },
	               { "move",    ":56", "move a frame",    ":336" },
	               { "reshape", ":337", "reshape a frame", ":338" },
};

static int flag=0;

static struct menu_line
mgmt_disp(n, ptr)
int n;
char *ptr;
{ 
        int i;

	if (n >= NMUDGES)
		n = NMUDGES - 1;
          if (flag == 0)
    	  {
            for(i=0;i<NMUDGES && Mgmt_lines[i].highlight != NULL ;i++) {
               Mgmt_lines[i].highlight = gettxt (I18n_Mgmt_lines[i].wordid,
	    				     I18n_Mgmt_lines[i].word);
               Mgmt_lines[i].lininfo = gettxt (I18n_Mgmt_lines[i].stringid,
					       I18n_Mgmt_lines[i].string);
            }
            flag = 1;
	  }
	return Mgmt_lines[n];
}

static int
mgmt_odsh(a, t)
struct actrec *a;
token t;
{
	int 	line;
	token	menu_stream();
	struct actrec *curar;

	t = menu_stream(t);
	if (t == TOK_ENTER || t == TOK_OPEN) {
		(void) menu_ctl(a->id, CTGETPOS, &line);
		curar = (struct actrec *)(a->odptr);
		switch (line) {
		case 0: /* list */
			list_create();
			break;
		case 1:	/* move */
			enter_wdw_mode(curar, FALSE);
			break;
		case 2: /* reshape */
			if (curar && (curar->flags & AR_NORESHAPE)) {
                    
				(void)mess_err( gettxt(":339","Forms cannot be reshaped") ); /* abs s15 */
				t = TOK_NOP;
			}
			else 
				enter_wdw_mode(curar, TRUE);
			break;
		}
		t = TOK_NOP;
	} else if (t == TOK_CANCEL) {
		ar_backup();
		t = TOK_NOP;
	} else if (t == TOK_NEXT)
		t = TOK_NOP;		/* filter out, see menu_stream */
	return t;
}

static int
mgmt_help(a)
struct actrec *a;
{
	if ( Vflag )
	    return(objop("OPEN", "TEXT", "$VMSYS/OBJECTS/Text.mfhelp", "frm-mgmt", gettxt(":340","Frame Management"), NULL));
	else
	    return(generic_help( gettxt(":340","Frame Management"), HT_FRM_MENU));
}

int
mgmt_create()
{
	char	*cmd;
	int	len;
	struct	actrec a, *ar, *ar_create(), *window_arg();
	extern	int Arg_count;
	extern	char *Args[];

	switch (Arg_count) {
	case 0:
	case 1:
		/*
		 * no arguments to frm-mgmt
		 *
		 * assume the current frame and prompt the user for 
		 * the command
		 */
		cmd = NULL;
		ar = ar_get_current();
		break;
	case 2:
		/*
		 * One argument to frm-mgmt
		 *
		 * This case is ambiguous, since the argument could be
		 * either one of the three commands "move" "reshape" or "list"
		 * or it could be a window path or number.  So, assume it 
		 * is a window if it isn't a command.  (Hope nobody tries this
		 * on a window named "list")
		 */
		len = strlen(Args[1]);
		if (strnCcmp(Args[1], "move", len) == 0 ||
			strnCcmp(Args[1], "reshape", len) == 0 ||
			strnCcmp(Args[1], "list", len) == 0) {
			cmd = Args[1];
			ar = ar_get_current();
		}
		else {
			cmd = NULL;
			if ((ar = window_arg(1, Args + 1, 1)) == NULL) {

                                io_printf( 'm', NULL,
                                           gettxt(":341","Unknown command or frame \"%s\" ignored"),
                                           Args[1]);

				return(FAIL);
			}
		}
		break;
	default:	
		/*
		 * Two arguments to frm-mgmt
		 *
		 * first arg is the command, the second is the frame
		 */
		len = strlen(Args[1]);
		if (strnCcmp(Args[1], "move", len) == 0 ||
		    strnCcmp(Args[1], "reshape", len) == 0) {
			cmd = Args[1];
			if ((ar = window_arg(1, Args + 2, 1)) == NULL) {

                                io_printf( 'm', NULL,
                                           gettxt(":342","Can't find frame \"%s\""),
                                           Args[2]);

				return(FAIL);
			}
		}
		else if (strnCcmp(Args[1], "list", len) == 0)  {
			cmd = Args[1];

			(void)mess_err( gettxt(":343","Arguments to \"list\" ignored") ); /* abs s15 */
		}
		else {

                                io_printf( 'm', NULL,
                                           gettxt(":344","Unknown command \"%s\" ignored"),
                                           Args[1]);

			return(FAIL);
		}
	}

	if (cmd == NULL) {
		/*
		 * if the command (list, reshape, move ...) is not specified
		 * then display a menu of available (frame management) commands
		 */ 
 

		a.id = (int) menu_make(-1, gettxt(":340","Frame Management"),
			VT_NONUMBER | VT_CENTER, VT_UNDEFINED, VT_UNDEFINED,
			0, 0, mgmt_disp, NULL);
		if (a.id == FAIL)
			return(FAIL);
		ar_menu_init(&a);
		a.fcntbl[AR_ODSH] = mgmt_odsh;
		a.fcntbl[AR_HELP] = mgmt_help;
		a.odptr = (char *) ar;
		a.flags = 0;
		return(ar_current(ar_create(&a), FALSE) ==     /* abs k15 */
		       NULL? FAIL : SUCCESS);
	}
	else if (strncmp(cmd, "list", strlen(cmd)) == 0) {
		/*
		 * if the command is "list" then generate a menu that
		 * will list all active frames
		 */
		list_create();
	}
	else if (strncmp(cmd, "move", strlen(cmd)) == 0) {
		/*
		 * if the command is "move" then enter "move" mode ...
		 */ 
		enter_wdw_mode(ar, FALSE);
	}
	else if (strncmp(cmd, "reshape", strlen(cmd)) == 0) {
		/*
		 * if the command is "reshape" then make sure the
		 * frame can be reshaped before performing the operation
		 */
		if (ar && (ar->flags & AR_NORESHAPE)) 

			(void)mess_err( gettxt(":339","Forms cannot be reshaped") ); /* abs s15 */
		else
			enter_wdw_mode(ar, TRUE);	/* reshape it */
	}
	else {

		(void)mess_err( gettxt(":345","Bad argument to frmmgmt: try list, move or reshape") ); 	/* abs s15 */
		return(FAIL);
	}
	return(SUCCESS);
}
