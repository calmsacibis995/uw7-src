/*		copyright	"%c%" 	*/

/*LINTLIBRARY*/

#include "access.h"

/*	from file allowed.c */

/**
 ** is_user_admin() - CHECK IF CURRENT USER IS AN ADMINISTRATOR
 **/

int is_user_admin ( void )
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** is_user_allowed() - CHECK USER ACCESS ACCORDING TO ALLOW/DENY LISTS
 **/
int is_user_allowed ( char * user, char ** allow,
		      char ** deny )
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** is_user_allowed_form() - CHECK USER ACCESS TO FORM
 **/
int is_user_allowed_form ( char * user, char * form )
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** is_user_allowed_printer() - CHECK USER ACCESS TO PRINTER
 **/
int is_user_allowed_printer ( char * user, char * printer)
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** is_form_allowed_printer() - CHECK FORM USE ON PRINTER
 **/
int is_form_allowed_printer ( char * form,  char * printer)
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** allowed() - GENERAL ROUTINE TO CHECK ALLOW/DENY LISTS
 **/
int allowed (char * item, char ** allow, char ** deny)
{
    static int  _returned_value;
    return _returned_value;
}

/*	from file change.c */

/**
 ** deny_user_form() - DENY USER ACCESS TO FORM
 **/
int deny_user_form ( char ** user_list,  char * form)
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** allow_user_form() - ALLOW USER ACCESS TO FORM
 **/
int allow_user_form (char ** user_list, char * form)
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** deny_user_printer() - DENY USER ACCESS TO PRINTER
 **/
int deny_user_printer (char ** user_list, char * printer)
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** allow_user_printer() - ALLOW USER ACCESS TO PRINTER
 **/
int allow_user_printer (char ** user_list, char * printer)
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** deny_form_printer() - DENY FORM USE ON PRINTER
 **/
int deny_form_printer (char ** form_list, char * printer)
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** allow_form_printer() - ALLOW FORM USE ON PRINTER
 **/
int allow_form_printer (char ** form_list, char * printer)
{
    static int  _returned_value;
    return _returned_value;
}

/*	from file dumpaccess.c */

/**
 ** dumpaccess() - DUMP ALLOW OR DENY LISTS
 **/
int dumpaccess (char * dir, char * name, char * prefix,
		char *** pallow, char *** pdeny)
{
    static int  _returned_value;
    return _returned_value;
}

/*	from file files.c */


/**
 ** getaccessfile() - BUILD NAME OF ALLOW OR DENY FILE
 **/
char *getaccessfile (char * dir, char * name,
		     char * prefix, char * base)
{
    static char * _returned_value;
    return _returned_value;
}

/*	from file loadaccess.c */
/**
 ** load_userform_access() - LOAD ALLOW/DENY LISTS FOR USER+FORM
 **/
int load_userform_access (char *form, char *** pallow, char *** pdeny)
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** load_userprinter_access() - LOAD ALLOW/DENY LISTS FOR USER+PRINTER
 **/
int load_userprinter_access (char * printer, char *** pallow,
			     char *** pdeny)
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** load_formprinter_access() - LOAD ALLOW/DENY LISTS FOR FORM+PRINTER
 **/
int load_formprinter_access (char * printer, char *** pallow,
			     char *** pdeny)
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** loadaccess() - LOAD ALLOW OR DENY LISTS
 **/
int loadaccess (char * dir, char * name, char * prefix,
		char *** pallow, char *** pdeny)
{
    static int  _returned_value;
    return _returned_value;
}
