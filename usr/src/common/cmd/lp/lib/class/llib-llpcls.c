/*		copyright	"%c%" 	*/

/*LINTLIBRARY*/

/*	from file delclass.c */
#include "class.h"

/**
 ** delclass() - WRITE CLASS OUT TO DISK
 **/
int delclass (char * name)
{
    static int  _returned_value;
    return _returned_value;
}

/*	from file freeclass.c */


/**
 ** freeclass() - FREE SPACE USED BY CLASS STRUCTURE
 **/
void freeclass (CLASS * clsbufp)
{
}

/*	from file getclass.c */
/**
 ** getclass() - READ CLASS FROM TO DISK
 **/
CLASS *getclass (char * name)
{
    static CLASS * _returned_value;
    return _returned_value;
}

/*	from file putclass.c */
/**
 ** putclass() - WRITE CLASS OUT TO DISK
 **/
int putclass (char * name, CLASS * clsbufp)
{
    static int  _returned_value;
    return _returned_value;
}
