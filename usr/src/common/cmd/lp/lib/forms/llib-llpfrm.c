/*		copyright	"%c%" 	*/

/*LINTLIBRARY*/

/*	from file delform.c */

# include	"lp.h"
# include	"form.h"

int delform(char *formname)
{
    static int _returned_value;
    return _returned_value;
}

int formtype(char *formname)
{
    static int _returned_value;
    return _returned_value;
}

/*	from file getform.c */
int getform ( char * form, FORM * formp, FALERT * alertp,
	      FILE ** alignfilep )
{
    static int _returned_value;
    return _returned_value;
}

/*	from file putform.c */
int putform( char * form, FORM * formp, FALERT *alertp,
	     FILE ** alignfilep)
{
    static int _returned_value;
    return _returned_value;
}
FILE *formfopen( char *formname, char *file, char *type,
		 int dircreat)
{
    static FILE * _returned_value;
    return _returned_value;
}

/*	from file rdform.c */
int addform( char *form_name, FORM *formp, FILE *usrfp)
{
    static int _returned_value;
    return _returned_value;
}
int chform( char *form_name, FORM *cform, FILE *usrfp)
{
    static int _returned_value;
    return _returned_value;
}
int chfalert( FALERT *new, FALERT *old)
{
    static int _returned_value;
    return _returned_value;
}
int nscan( char *s, SCALED *var)
{
    static int _returned_value;
    return _returned_value;
}
void
stripnewline( char *line)
{
}
char * mkcmd_usr ( char * cmd )
{
    static char *  _returned_value;
    return _returned_value;
}
char * getformdir ( char * formname, int creatdir)
{
    static char *  _returned_value;
    return _returned_value;
}
int scform ( char * form_nm, FORM * formp, FILE *fp,
	     short LP_FORM, int * opttag )
{
    static int _returned_value;
    return _returned_value;
}

/*	from file wrform.c */
int wrform( char * name, FORM * formp, FILE * fp,
	    int (*error_handler)(int, int, int), int * which_set)
{
    static int _returned_value;
    return _returned_value;
}
int wralign( FILE *where, FILE *fp)
{
    static int _returned_value;
    return _returned_value;
}
void wralert( FALERT *alertp)
{
}
int printcmt( FILE *where, char *comment)
{
    static int _returned_value;
    return _returned_value;
}
