/*		copyright	"%c%" 	*/

/*LINTLIBRARY*/

/* from file Syscalls.c */
# include	<fcntl.h>
# include	"lp.set.h"
# include	"lp.h"

_Access ( char * s, int i )
{
 static _returned_value;
 return _returned_value;
}
_Chdir ( char * s )
{
 static _returned_value;
 return _returned_value;
}
_Chmod ( char * s, int i )
{
 static _returned_value;
 return _returned_value;
}
_Chown (char * s, int i, int j )
{
 static _returned_value;
 return _returned_value;
}
_Close ( int i )
{
 static _returned_value;
 return _returned_value;
}
_Creat ( char * s, int i )
{
 static _returned_value;
 return _returned_value;
}
_Fcntl ( int i, int j, ...)
{
 static _returned_value;
 return _returned_value;
}
_Fstat ( int i, struct stat * st )
{
 static _returned_value;
 return _returned_value;
}
_Link ( char * s1, char * s2)
{
 static _returned_value;
 return _returned_value;
}
_Mknod ( char * s, int i, int j )
{
 static _returned_value;
 return _returned_value;
}
_Open ( char * s, int i, ... )
{
 static _returned_value;
 return _returned_value;
}
_Read ( int i, char * s, unsigned j )
{
 static _returned_value;
 return _returned_value;
}
_Stat ( char * s, struct stat * st )
{
 static _returned_value;
 return _returned_value;
}
_Unlink ( char * s )
{
 static _returned_value;
 return _returned_value;
}
_Wait ( int * i )
{
 static _returned_value;
 return _returned_value;
}
_Write ( int i, char * s, unsigned j )
{
 static _returned_value;
 return _returned_value;
}

/* from file addlist.c */

int addlist ( char *** plist, char * item )
{
    static int _returned_value;
    return _returned_value;
}

/* from file appendlist.c */

int appendlist ( char *** plist, char * item )
{
    static int _returned_value;
    return _returned_value;
}


/**
 ** putalert () - WRITE ALERT TO FILES
 **/
int putalert ( char * parent, char * name,
	       FALERT * alertp )
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** getalert () - EXTRACT ALERT FROM FILES
 **/
FALERT * getalert ( char * parent, char * name )
{
    static FALERT * _returned_value;
    return _returned_value;
}

/**
 ** delalert () - DELETE ALERT FILES
 **/
int delalert ( char * parent, char * name )
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** envlist () - PRINT OUT ENVIRONMENT LIST SAFELY
 **/

/**
 ** printalert () - PRINT ALERT DESCRIPTION
 **/
void printalert ( FILE * fp, FALERT * alertp, int isfault )
{
}

/* from file charset.c */
/**
 ** search_cslist () - SEARCH CHARACTER SET ALIASES FOR CHARACTER SET
 **/
char * search_cslist ( char * item, char ** list )
{
    static char * _returned_value;
    return _returned_value;
}

/* from file cs_strcmp.c */
int cs_strcmp ( char * s1, char * s2)
{
    static int
_returned_value;
    return _returned_value;
}

/* from file cs_strncmp.c */
int cs_strncmp ( char * s1, char * s2, int n )
{
    static int
_returned_value;
    return _returned_value;
}

/* from file dellist.c */

/**
 ** dellist () - REMOVE ITEM FROM (char **) LIST
 **/
int dellist ( char *** plist, char * item )
{
    static int _returned_value;
    return _returned_value;
}

/* from file dashos.c */
/**
 ** dashos () - PARSE -o OPTIONS, (char *) --> (char **)
 **/
char ** dashos ( char * o )
{
    static char ** _returned_value;
    return _returned_value;
}

/* from file dirs.c */

/**
 ** mkdir_lpdir ()
 **/
int mkdir_lpdir ( char * path, int mode )
{
    static int _returned_value;
    return _returned_value;
}


/**
 ** duplist () - DUPLICATE A LIST OF STRINGS
 **/
char ** duplist ( char ** src )
{
    static char ** _returned_value;
    return _returned_value;
}


/**
 ** open_lpfile () - OPEN AND LOCK A FILE; REUSE STATIC BUFFER
 ** close_lpfile () - CLOSE FILE; RELEASE STATIC BUFFER
 **/

/*VARARGS2*/
FILE * open_lpfile ( char * path, char * type, mode_t mode )
{
    static FILE * _returned_value;
    return _returned_value;
}
int close_lpfile ( FILE * fp )
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** chown_lppath ()
 **/
int chown_lppath ( char * path )
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** rmfile () - UNLINK FILE BUT NO COMPLAINT IF NOT THERE
 **/
int rmfile ( char * path )
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** loadline () - LOAD A ONE-LINE CHARACTER STRING FROM FILE
 **/
char * loadline ( char * path )
{
    static char * _returned_value;
    return _returned_value;
}

/**
 ** loadstring () - LOAD A CHARACTER STRING FROM FILE
 **/
char * loadstring ( char * path )
{
    static char * _returned_value;
    return _returned_value;
}

/**
 ** dumpstring () - DUMP CHARACTER STRING TO FILE
 **/
int dumpstring ( char * path, char * str )
{
    static int _returned_value;
    return _returned_value;
}

/* from file freelist.c */
/**
 ** freelist () - FREE ALL SPACE USED BY LIST
 **/
void freelist ( char ** list )
{
}


/**
 ** getlist () - CONSTRUCT LIST FROM STRING
 **/
char ** getlist ( char * str, char * ws,
		  char * hardsep )
{
    static char ** _returned_value;
    return _returned_value;
}

/**
 ** unq_strdup ()
 **/

/* from file getname.c */
char * getname ( void )
{
    static char * _returned_value;
    return _returned_value;
}

/* from file getpaths.c */

# undef	getpaths
# undef	getadminpaths

void getpaths ( void )
{
}
void getadminpaths ( char * admin )
{
}

/**
 ** getprinterfile () - BUILD NAME OF PRINTER FILE
 **/
char * getprinterfile ( char * name, char * component )
{
    static char * _returned_value;
    return _returned_value;
}

/**
 ** getclassfile () - BUILD NAME OF CLASS FILE
 **/
char * getclassfile ( char * name )
{
    static char * _returned_value;
    return _returned_value;
}

/**
 ** getfilterfile () - BUILD NAME OF FILTER TABLE FILE
 **/
char * getfilterfile ( char * table )
{
    static char * _returned_value;
    return _returned_value;
}

/* from file getspooldir.c */
char * getspooldir ( void )
{
    static char * _returned_value;
    return _returned_value;
}

/* from file isterminfo.c */

/**
 ** isterminfo () - SEE IF TYPE IS IN TERMINFO DATABASE
 **/
int isterminfo ( char * type )
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** _isterminfo ()
 **/

/* from file lenlist.c */
/**
 ** lenlist () - COMPUTE LENGTH OF LIST
 **/
int lenlist ( char ** list )
{
    static int _returned_value;
    return _returned_value;
}


/**
 ** makepath () - CREATE PATHNAME FROM COMPONENTS
 **/

/*VARARGS0*/
char * makepath ( char * s, ...)
{
    static char * _returned_value;
    return _returned_value;
}


/**
 ** makestr () - CONSTRUCT SINGLE STRING FROM SEVERAL
 **/

/*VARARGS0*/
char * makestr ( char * s, ...)
{
    static char * _returned_value;
    return _returned_value;
}

/* from file mergelist.c */

/**
 ** mergelist () - ADD CONTENT OF ONE LIST TO ANOTHER
 **/
int mergelist ( char *** dstlist, char ** srclist )
{
    static int _returned_value;
    return _returned_value;
}

/* from file printlist.c */
/**
 ** printlist_setup () - ARRANGE FOR CUSTOM PRINTING
 ** printlist_unsetup () - RESET STANDARD PRINTING
 **/
void printlist_setup ( char * prefix, char * suffix,
		       char * sep, char * newline )
{
}
void printlist_unsetup ( void )
{
}

/**
 ** printlist () - PRINT LIST ON OPEN CHANNEL
 **/
int printlist ( FILE * fp, char ** list )
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** q_print () - PRINT STRING, QUOTING SEPARATOR CHARACTERS
 **/

/* from file sdn.c */
/**
 ** printsdn () - PRINT A SCALED DECIMAL NUMBER NICELY
 **/
void printsdn_setup ( char * prefix, char * suffix,
		      char * newline )
{
}
void printsdn_unsetup ( void )
{
}
void printsdn ( FILE * fp, SCALED sdn )
{
}

/* from file sprintlist.c */
/**
 ** sprintlist () - FLATTEN (char **) LIST INTO (char *) LIST
 **/
char * sprintlist ( char ** list )
{
    static char * _returned_value;
    return _returned_value;
}

/* from file searchlist.c */
/**
 ** searchlist () - SEARCH (char **) LIST FOR ITEM
 **/
int searchlist ( char * item, char ** list )
{
    static int _returned_value;
    return _returned_value;
}

/* from file set_charset.c */

int set_charset ( char * char_set, int putout, char * type )
{
    static int _returned_value;
    return _returned_value;
}

/* from file set_pitch.c */

/**
 ** set_pitch ()
 **/
int set_pitch ( char * str, int which, int putout )
{
    static int _returned_value;
    return _returned_value;
}

/* from file set_size.c */

int set_size ( char * str, int which, int putout )
{
    static int _returned_value;
    return _returned_value;
}

/* from file sop.c */
/**
 ** sop_up_rest () - READ REST OF FILE INTO STRING
 **/
char * sop_up_rest ( FILE * fp, char * endsop )
{
    static char * _returned_value;
    return _returned_value;
}

/* from file strip.c */
/**
 ** strip () - STRIP LEADING AND TRAILING BLANKS
 **/
char * strip ( char * str )
{
    static char * _returned_value;
    return _returned_value;
}

/* from file syntax.c */
int syn_name ( char * str )
{
    static int _returned_value;
    return _returned_value;
}
int syn_type ( char * str )
{
    static int _returned_value;
    return _returned_value;
}
int syn_text ( char * str )
{
    static int _returned_value;
    return _returned_value;
}
int syn_comment ( char * str )
{
    static int _returned_value;
    return _returned_value;
}
int syn_machine_name ( char * str )
{
    static int _returned_value;
    return _returned_value;
}
int syn_option ( char * str )
{
    static int _returned_value;
    return _returned_value;
}

/* from file tidbit.c */
/**
 ** _Getsh () - GET TWO-BYTE SHORT FROM (char *) POINTER PORTABLY
 **/

/**
 ** tidbit () - TERMINFO DATABASE LOOKUP
 **/

/*VARARGS2*/
int tidbit ( char * term, char * cap, ...)
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** untidbit () - FREE SPACE ASSOCIATED WITH A TERMINFO ENTRY
 **/
void untidbit ( char * term )
{
}

/**
 ** open_terminfo_file () - OPEN FILE FOR TERM ENTRY
 **/

/* from file wherelist.c */
/**
 ** wherelist () - RETURN POINTER TO ITEM IN LIST
 **/
char ** wherelist ( char * item, char ** list )
{
    static char ** _returned_value;
    return _returned_value;
}

/* from file which.c */
/**
 ** isprinter () - SEE IF ARGUMENT IS A REAL PRINTER
 **/
int isprinter ( char * str )
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** isclass () - SEE IF ARGUMENT IS A REAL CLASS
 **/
int isclass ( char * str )
{
    static int _returned_value;
    return _returned_value;
}

/**
 ** isrequest () - SEE IF ARGUMENT LOOKS LIKE A REAL REQUEST
 **/
int isrequest ( char * str )
{
    static int _returned_value;
    return _returned_value;
}

int isnumber ( char * s )
{
    static int _returned_value;
    return _returned_value;
}

/*	from file next.c */

# if	defined(__STDC__)
char * next_x ( char * parent, long * lastdirp, unsigned int what )
#else
char *
next_x(parent, lastdirp, what)
char		*parent;
long		*lastdirp;
unsigned int	what;
#endif
{
    static char * _returned_value;
    return _returned_value;
}
