/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:oh/ifuncs.c	1.33.3.4"

#include <stdio.h>
#include <string.h>
#include "inc.types.h"		/* abs s14 */
#include <sys/stat.h>
#include "wish.h"
#include "but.h"
#include "typetab.h"
#include "ifuncdefs.h"
#include "partabdefs.h"
#include "obj.h"
#include "optabdefs.h"
#include "retcodes.h"
#include "windefs.h"
#include "var_arrays.h"
#include "terror.h"
#include "token.h"
#include "moremacros.h"
#include "message.h"
#include "sizes.h"
#include <unistd.h>

#define SUBSTLEN 2
#define INT_SUBSTLEN 4

struct ott_entry *name_to_ott();
/* a string containing the name of the variable in the ott that shows
   where a deleted object came from */
char Undel[] = "UNDELDIR";
static char *Arg;
/* dmd TCB */
static struct ott_entry *Ott;
extern char nil[];


/* a table of all of the functions that perform object operations */
int (*Function[MAX_IFUNCS])();
void docv();
char *getepenv();

IF_badfunc()
{
      (void)mess_err( gettxt(":118","That operation is not available in FACE") ); 
      /* abs s15 */
      return(FAIL);
}

int
IF_sh()
{ }

int
IF_rn(argv)
char *argv[];
{
	char msg[MESSIZ];
	char oldname[DNAMESIZ];
	struct ott_entry *entry, *path_to_ott();
	char	*bsd_path_to_title();

        char *i18n_string;
        int   i18n_length;

	if (path_isopen(argv[0], "rename", TRUE))
		return(FAIL);
	if (ckperms(parent(argv[0]), 02) == FAIL)
		return(FAIL);
	if ((entry = path_to_ott(argv[0])) == NULL)
		return(FAIL);
	strcpy(oldname, entry->name);
	if (ott_mv(entry, NULL, argv[1], TRUE) != FAIL) {

                i18n_string=gettxt(":119","%1$s renamed to %2$s"); 
                i18n_length=(int)strlen(i18n_string) - 2 * INT_SUBSTLEN;
                
		sprintf(msg, i18n_string, bsd_path_to_title(oldname,(MESS_COLS - i18n_length)/2), bsd_path_to_title(argv[1],(MESS_COLS - i18n_length)/2));
		mess_temp(msg);
		return(SUCCESS);
	} else {

                i18n_string=gettxt(":106","%s rename failed"); 
                i18n_length=(int)strlen(i18n_string) - SUBSTLEN;
                
		sprintf(msg, i18n_string, bsd_path_to_title(oldname,MESS_COLS - i18n_length));
		(void)mess_err(msg); 	/* abs s15 */
		return(FAIL);
	}
}

int
IF_cp(argv)
char *argv[];
{
	return(mv_or_cp(FALSE, argv));
}

int
IF_mv(argv)
char *argv[];
{
	return(mv_or_cp(TRUE, argv));
}

static int
mv_or_cp(mv, argv)
bool mv;
char *argv[];
{
    char msg[MESSIZ];
    char oldname[DNAMESIZ], newname[DNAMESIZ];
    char *display, path[PATHSIZ];
    struct ott_entry *entry, *path_to_ott();
    extern char *Wastebasket;
    char *filename();
    char	*bsd_path_to_title();
    int plen;

    char *i18n_string;
    int   i18n_length;

    working(TRUE);
    if (mv && path_isopen(argv[0], mv ? "move" : "copy", TRUE))
	return(FAIL);
    if (mv && ckperms(parent(argv[0]), 02) == FAIL) {
	return(FAIL);
    }
    if (! mv && ckperms(argv[0], 04) == FAIL) {
	return(FAIL);
    }
    if (ckperms(argv[1], 02) == FAIL) {
	return(FAIL);
    }
    if ((entry = path_to_ott(argv[1])) == NULL)
	return(FAIL);
    if (!(entry->objmask & CL_DIR)) {

        if (mv != 0)
           i18n_string=gettxt(":120","%s is not a proper destination for move"); 
        else
           i18n_string=gettxt(":121","%s is not a proper destination for copy"); 

        i18n_length=(int)strlen(i18n_string) - SUBSTLEN;
                
	sprintf(msg, i18n_string, 
		bsd_path_to_title(argv[1], MESS_COLS-i18n_length) );

	(void)mess_err(msg);		/* abs s15 */
	return(FAIL);
    }
    if ((entry = path_to_ott(argv[0])) == NULL)
	return(FAIL);
    display = strsave(entry->display);
    strcpy(oldname, entry->name);
    if (argv[2] == NULL)
	strcpy(newname, entry->name);
    else
	strcpy(newname, filename(argv[2]));
    if (ott_mv(entry, argv[1], newname, mv) != FAIL) {
	sprintf(path, "%s/%s", argv[1], newname);
	if ((entry = path_to_ott(path)) == NULL)
	    return(FAIL);
	entry->display = display;
	ott_chg_display(entry);
	if (strncmp(path, Wastebasket, strlen(Wastebasket)) == 0)
	    (void) odi_putkey(entry, Undel, parent(argv[0]));
	else if (strncmp(argv[0], Wastebasket, strlen(Wastebasket)) == 0)
	    (void) odi_putkey(entry, Undel, NULL);
	utime(path, NULL);	/* Touch the file */
	ott_mtime(entry);

	if ( strcmp(oldname, newname) == 0 ) {

            if (mv != 0)
                  i18n_string=gettxt(":102","%1$s moved to the %2$s folder");
            else
                  i18n_string=gettxt(":103","%1$s copied to the %2$s folder");

            i18n_length=(int)strlen(i18n_string) - 2 * INT_SUBSTLEN;

	    sprintf(msg, i18n_string,
		bsd_path_to_title(oldname, (MESS_COLS-i18n_length)/2),
	        bsd_path_to_title(argv[1],(MESS_COLS-i18n_length )/2) );

	} else {

            if (mv != 0)
                  i18n_string=gettxt(":100","%1$s moved to the %2$s folder and named %3$s");
            else
                  i18n_string=gettxt(":101","%1$s copied to the %2$s folder and named %3$s");

            i18n_length=(int)strlen(i18n_string) - 3 * INT_SUBSTLEN;

	    sprintf(msg, i18n_string,
		bsd_path_to_title(oldname, (MESS_COLS-i18n_length)/3),
	        bsd_path_to_title(argv[1],(MESS_COLS-i18n_length )/3),
	        bsd_path_to_title(newname,(MESS_COLS - i18n_length)/3) );
	}

	mess_temp(msg);
	return(SUCCESS);
    } else {
        if (mv != 0)
           i18n_string=gettxt(":104","%1$s move failed to the %2$s folder");
        else
           i18n_string=gettxt(":105","%1$s copy failed to the %2$s folder");

        i18n_length=(int)strlen(i18n_string) - 2 * INT_SUBSTLEN;

	sprintf(msg, i18n_string,
	    bsd_path_to_title(oldname,(MESS_COLS-i18n_length)/2),
	    bsd_path_to_title(argv[1],(MESS_COLS-i18n_length)/2) );

	(void)mess_err(msg);		/* abs s15 */
	return(FAIL);
    }
}

int
IF_sc(argv)
char *argv[];
{
	if (scram(argv[0]) == FAIL)
		return(FAIL);
	return(SUCCESS);
}

int
IF_unsc(argv)
char *argv[];
{
	if (unscram(argv[0]) == FAIL)
		return(FAIL);
	return(SUCCESS);
}

int
IF_rm(argv)
char *argv[];
{
	token confirmW(), confirm(), t;
	struct ott_entry *ott;
	struct ott_entry *path_to_ott();
	struct ott_tab *paths_ott;
	struct ott_tab *ott_get();
	int ott_len;
	char path[PATHSIZ], buf[BUFSIZ], *filename(), *bsd_path_to_title();
	extern char *Filecabinet, *Wastebasket;

        char *i18n_string;
        int   i18n_length;

	Arg=strsave(argv[0]);

	if ((ott = path_to_ott(argv[0])) == NULL)
		return(FAIL);

	if (strcmp(argv[0], Filecabinet) == 0) {
		(void)mess_err( gettxt(":122","You are not allowed to delete your Filecabinet") ); /* abs s15 */
		return(FAIL);
	}

	if (strcmp(argv[0], Wastebasket) == 0) {
		if (path_isopen(argv[0], "delete", FALSE))
			return(FAIL);
		sprintf(buf, gettxt(":123","Press ENTER to empty your %s:"), filename(argv[0]));
		(void)mess_err( gettxt(":124","WARNING: You are about to permanently remove all objects in your WASTEBASKET") );		/* abs s15 */

		get_string(confirmW, buf, "", 0, FALSE, "delete", "delete");
		return(SUCCESS);
	}

	if (path_isopen(argv[0], "delete", TRUE)) {
		return(FAIL);
	}

/*
 *   The following if statement reads,
 *		if the object we are deleting is a directory and
 *		   we can get an ott for it and
 *		   the directory is not empty.
 */
	if ((ott->objmask & CL_DIR) && 
	   ((paths_ott = ott_get(argv[0], OTT_SALPHA, 0, 0, 0)) != NULL) &&
	    array_len(paths_ott->parents))
		(void)mess_err( gettxt(":125","WARNING: The folder you are about to delete is not empty") ); /* abs s15 */

        i18n_string=gettxt(":126","Press ENTER to delete %s:");
        i18n_length=(int)strlen(i18n_string) - SUBSTLEN;

	sprintf(buf, i18n_string,
	    bsd_path_to_title(filename(argv[0]), MESS_COLS - i18n_length));

	if (strncmp(argv[0], Wastebasket, strlen(Wastebasket)) == 0) {
		Ott=ott;
		get_string(confirm, buf, "", 0, FALSE, "delete", "delete");
		return(SUCCESS);
	}

	get_string(confirm, buf, "", 0, FALSE, "delete", "delete");
	return(SUCCESS);
}

int
blow_away(ott)
struct ott_entry *ott;
{
	char command[10*PATHSIZ + 30];
	struct ott_entry *ott_next_part();
	int len;

	len = sprintf(command, "/bin/rm -rf %s ", ott_to_path(ott));
	while (ott = ott_next_part(ott))
		len += sprintf(command+len, "%s ", ott_to_path(ott));
	(void) system(command);
}

int
IF_unrm(argv)
char *argv[];
{
	struct ott_entry *ott, *path_to_ott();
	char *path, *odi_getkey();
	extern char *Wastebasket;

	if ( strncmp(argv[0], Wastebasket, strlen(Wastebasket)) ||
	     ! strcmp(argv[0], Wastebasket) ) {

		(void)mess_err( gettxt(":127","Undelete can only be used on objects in your WASTEBASKET") ); /* abs s15 */

		return(FAIL);
	}
	if ((ott = path_to_ott(argv[0])) == NULL)
		return(FAIL);

	if ( ! ((path = odi_getkey(ott, Undel)) && *path )) {

		(void)mess_err( gettxt(":128","Unable to find previous folder, use MOVE") ); /* abs s15 */
		return(FAIL);
	}

	return(objop("move", NULL, argv[0], path, NULL));
}

int
IF_vi(argv)
char *argv[];
{
}

#define MAX_DESCRIP	24

int
redescribe(argv)
char *argv[];
{
	register int i, len;
	struct ott_entry *entry;
	char newdesc[MAX_DESCRIP+1]; /* + 1 to allow for NULL in sprintf */
	struct ott_entry *path_to_ott();

	char  *filename(), *bsd_path_to_title();

	if ((entry = path_to_ott(argv[0])) == NULL)
		return(FAIL);
	for (i = 1, len = 0; argv[i] && len < MAX_DESCRIP-1; i++)
		len += sprintf(&newdesc[len], "%.*s ", MAX_DESCRIP-len-1, argv[i]);
	newdesc[len-1] = '\0';
	if (strchr(newdesc, '|')) {

		(void)mess_err( gettxt(":129","The character '|' is not allowed in description, try again") ); /* abs s15 */

		return(FAIL);
	}
	if (strcmp(newdesc,"\0") == 0) {

		(void)mess_err( gettxt(":130","Null strings are not allowed in description, try again") ); /* abs s15 */

		return(FAIL);
	}


	entry->display = strsave(newdesc);
	(void) ott_chg_display(entry);

	mess_temp(nstrcat(bsd_path_to_title(filename(argv[0]),
	    MESS_COLS - 17 - (int)strlen(newdesc)),
	    " redescribed as ", newdesc, ".", NULL));
	return(SUCCESS);
}


int
ckperms(path, mode)
char *path;
mode_t mode;	/* EFT abs k16 */

{
    char	*bsd_path_to_title();

    char *i18n_string;
    int  i18n_length;

    if (access(path, 00) == FAIL) {
	mess_unlock();			/* abs s16 */

        i18n_string = gettxt(":131","%s does not exist");
        i18n_length = (int)strlen(i18n_string) - SUBSTLEN;

        io_printf( 'm', NULL, i18n_string,
                	bsd_path_to_title(path, MESS_COLS-i18n_length) );

	return(FAIL);
    }

    if (access(path, mode) == FAIL) {

	switch (mode) {
	case 01:
                i18n_string=gettxt(":132","You do not have permission to search %s"),
                i18n_length=(int)strlen(i18n_string) - SUBSTLEN;
                io_printf( 'm', NULL, i18n_string,
	              bsd_path_to_title(path, MESS_COLS-i18n_length) );

	case 02:
                i18n_string=gettxt(":133","You do not have permission to modify %s"),
                i18n_length=(int)strlen(i18n_string) - SUBSTLEN;
                io_printf( 'm', NULL, i18n_string,
	              bsd_path_to_title(path, MESS_COLS-i18n_length) );

	case 04:
                i18n_string=gettxt(":134","You do not have permission to read %s"),
                i18n_length=(int)strlen(i18n_string) - SUBSTLEN;
                io_printf( 'm', NULL, i18n_string,
	              bsd_path_to_title(path, MESS_COLS-i18n_length) );

	default:
                i18n_string=gettxt(":87","You do not have permission to access %s"),
                i18n_length=(int)strlen(i18n_string) - SUBSTLEN;
                io_printf( 'm', NULL, i18n_string,
	              bsd_path_to_title(path, MESS_COLS-i18n_length) );
	}

	return(FAIL);
    }
    return(SUCCESS);
}

void
fcn_init()
{
	int IF_dvi(), IF_dir_open(), IF_dmv(), IF_dcp(), IF_drn();
	int	IF_sp();
	int IF_aed(), IF_acv(), IF_apr(), IF_aed();
	int IF_omopen();
	int IF_helpopen();
	int IF_ofopen();
	int IF_exec_open();

/* general purpose operations */

	Function[IF_VI] = IF_vi;
	Function[IF_SH] = IF_sh;
	Function[IF_CP] = IF_cp;
	Function[IF_RN] = IF_rn;
	Function[IF_MV] = IF_mv;
	Function[IF_RM] = IF_rm;
	Function[IF_UNRM] = IF_unrm;
	Function[IF_SC] = IF_sc;
	Function[IF_UNSC] = IF_unsc;

/* operations specific to ascii files */

	Function[IF_ACV] = IF_acv;
	Function[IF_AED] = IF_aed;
	Function[IF_APR] = IF_apr;

/* operations specific to menu objects */

	Function[IF_MENOPEN] = IF_omopen;

/* operations specific to help objects */

	Function[IF_HLPOPEN] = IF_helpopen;

/* operations specific to form objects */

	Function[IF_FRMOPEN] = IF_ofopen;

/* operations specific to file folders */

	Function[IF_DED] = IF_dir_open;
	Function[IF_DVI] = IF_dvi;
	Function[IF_DMV] = IF_dmv;
	Function[IF_DCP] = IF_dcp;
	Function[IF_DRN] = IF_drn;

/* operations specific to executables */

	Function[IF_EED] = IF_exec_open;

/* illegal function */

	Function[IF_BADFUNC] = IF_badfunc;

	return;
}

static token
confirm(s, t)
char *s;
token t;
{
	extern char *Wastebasket;
	char buf[BUFSIZ], *filename(), *bsd_path_to_title();

        char *i18n_string;
        int   i18n_length;

        i18n_string=gettxt(":126","Press ENTER to delete %s:");
        i18n_length=(int)strlen(i18n_string) - SUBSTLEN;

	sprintf(buf, i18n_string,
	    bsd_path_to_title(filename(Arg),MESS_COLS - i18n_length));

	if (t == TOK_CANCEL)
		return TOK_NOP;

	if (t == TOK_SAVE && *s == NULL) {
		if (strncmp(Arg, Wastebasket, strlen(Wastebasket)) == 0) {
			blow_away(Ott);
			mess_temp(nstrcat("Object ", Ott->dname,
					" permanently removed from WASTEBASKET", NULL));
			ar_checkworld(TRUE);
			return TOK_NOP;
		}

		if (objop("move", NULL, Arg, Wastebasket, NULL) == FAIL) 
			return TOK_NOP;
		else
			ar_checkworld(TRUE);
	}
	else if (*s != NULL) {
		get_string(confirm, buf, "", 0, FALSE, "delete", "delete");

		(void)mess_err( gettxt(":135","Please re-enter value") ); /* abs s15 */
	}

	return TOK_NOP;
}

static token
confirmW(s, t)
char *s;
token t;
{
	extern char *Wastebasket;
	char buf[BUFSIZ], *filename();
	char command[PATHSIZ + 100];

	sprintf(buf, gettxt(":123","Press ENTER to empty your %s:"), filename(Arg));

	if (t == TOK_CANCEL)
		return TOK_NOP;

	if (t == TOK_SAVE && *s == NULL) {

		sprintf(command,"for i in %s/*; do /bin/rm -rf $i; done 1>/dev/null 2>/dev/null",Wastebasket);
		(void) system(command);
		(void)mess_err( gettxt(":136","All objects in WASTEBASKET have been permanently removed") ); /* abs s15 */
		ar_checkworld(TRUE);
	}
	else if (*s != NULL) {
		get_string(confirmW, buf, "", 0, FALSE, "delete", "delete");
		(void)mess_err( gettxt(":135","Please re-enter value") ); /* abs s15 */
	}

	return TOK_NOP;
}
