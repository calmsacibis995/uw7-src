/*		copyright	"%c%" 	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/pathott.c	1.10.3.5"

#include <stdio.h>
#include <string.h>
#include "inc.types.h"		/* abs s14 */
#include "typetab.h"
#include "sizes.h"
#include <unistd.h>

#define SUBSTLEN 2

extern int Vflag;

struct ott_entry *
path_to_ott(path)
char	*path;
{
    register char	*name;
    register struct ott_entry	*entry;
    struct ott_entry	*name_to_ott();
    struct ott_entry	*dname_to_ott();
    char	*filename();
    char	*parent();
    char	*nstrcat();
    char	*path_to_title();
    char	*path_to_full();
    char 	*bsd_path_to_title();

    char *i18n_string;
    int   i18n_length;


    if (make_current(parent(path)) == O_FAIL) {
	if (Vflag) {
            i18n_string = gettxt(":94","Could not open folder %s");
            i18n_length = (int)strlen(i18n_string) - SUBSTLEN;

            io_printf( 'm', NULL, i18n_string,
		  path_to_title(parent(path), NULL, (MESS_COLS-i18n_length) ) );

        }
	else
	    (void)mess_err( gettxt(":157","Command unknown, please try again") );
            /* abs s15 */

	    return(NULL);
    }
    if ((entry = name_to_ott(name = filename(path))) == NULL &&
	(entry = dname_to_ott(name)) == NULL) {
 /*
  * Backedup the changes to test the valid fmli name
  */
  /*
	if ( strncmp("Text", name, 4) == 0 ||
	     strncmp("Menu", name, 4) == 0 ||
	     strncmp("Form", name, 4) == 0 )    */
  /* Changed the message. Removed the word object  */
           
            i18n_string = gettxt(":158","Could not access %s");
            i18n_length = (int)strlen(i18n_string) - SUBSTLEN;

            io_printf( 'm', NULL, i18n_string, bsd_path_to_title(path_to_full(name), (MESS_COLS - i18n_length) ) ); /* ck p8 */

  /*
	else
	    mess_temp("Command unknown, please try again");   */
	return(NULL);
    }
    return(entry);
}
