/*		copyright	"%c%" 	*/

/* LINTLIBRARY */

/*	from file chkprinter.c */
# include	"lp.h"
# include	"printers.h"

/**
 ** chkprinter() - CHECK VALIDITY OF PITCH/SIZE/CHARSET FOR TERMINFO TYPE
 **/
unsigned long chkprinter (char * type, char * cpi,
			  char * lpi, char * len,
			  char * wid, char * cs)
{
    static unsigned long  _returned_value;
    return _returned_value;
}

/*	from file default.c */

/**
 ** getdefault() - READ THE NAME OF THE DEFAULT DESTINATION FROM DISK
 **/
char * getdefault ( void )
{
    static char * _returned_value;
    return _returned_value;
}

/**
 ** putdefault() - WRITE THE NAME OF THE DEFAULT DESTINATION TO DISK
 **/
int putdefault ( char * dflt )
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** deldefault() - REMOVE THE NAME OF THE DEFAULT DESTINATION
 **/
int deldefault ( void )
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** getcpdefault() - READ THE NAME OF THE DEFAULT COPY MODE FROM DISK
 **/
char * getcpdefault ( void )
{
    static char * _returned_value;
    return _returned_value;
}

/**
 ** putcpdefault() - WRITE THE NAME OF THE DEFAULT COPY MODE TO DISK
 **/
int putcpdefault ( char * dflt )
{
    static int  _returned_value;
    return _returned_value;
}


/*	from file delprinter.c */

/**
 ** delprinter()
 **/
int delprinter ( char * name )
{
    static int  _returned_value;
    return _returned_value;
}


/*	from file freeprinter.c */
/**
 **  freeprinter() - FREE MEMORY ALLOCATED FOR PRINTER STRUCTURE
 **/
void freeprinter (PRINTER * pp)
{
}

/*	from file getprinter.c */

/**
 ** getprinter() - EXTRACT PRINTER STRUCTURE FROM DISK FILE
 **/
PRINTER * getprinter ( char * name )
{
    static PRINTER * _returned_value;
    return _returned_value;
}

/*	from file okprinter.c */

/**
 ** okprinter() - SEE IF PRINTER STRUCTURE IS SOUND
 **/
int okprinter (char * name, PRINTER * prbufp, int isput)
{
    static int  _returned_value;
    return _returned_value;
}

/*	from file printwheels.c */
/**
 ** getpwheel() - GET PRINT WHEEL INFO FROM DISK
 **/
PWHEEL * getpwheel (char * name)
{
    static PWHEEL * _returned_value;
    return _returned_value;
}

/**
 ** putpwheel() - PUT PRINT WHEEL INFO TO DISK
 **/
int putpwheel ( char * name, PWHEEL * pwheelp)
{
    static int  _returned_value;
    return _returned_value;
}

/**
 ** delpwheel() - DELETE PRINT WHEEL INFO FROM DISK
 **/
int delpwheel (char * name)
{
    static int  _returned_value;
    return _returned_value;
}

/**
 **  freepwheel() - FREE MEMORY ALLOCATED FOR PRINT WHEEL STRUCTURE
 **/
void freepwheel ( PWHEEL * ppw)
{
}

/*	from file putprinter.c */
/**
 ** putprinter() - WRITE PRINTER STRUCTURE TO DISK FILES
 **/
int putprinter (char * name, PRINTER * prbufp)
{
    static int  _returned_value;
    return _returned_value;
}
