/*		copyright	"%c%" 	*/

/*LINTLIBRARY*/

/* from file anyrequests.c */
# include	<sys/types.h>
# include	"requests.h"

/**
 ** anyrequests() - SEE IF ANY REQUESTS ARE ``QUEUED''
 **/
int anyrequests ()
{
    static int  _returned_value;
    return _returned_value;
}

/* from file freerequest.c */

/**
 ** freerequest() - FREE STRUCTURE ALLOCATED FOR A REQUEST STRUCTURE
 **/
void freerequest ( REQUEST * reqbufp )
{
}

/* from file getrequest.c */

/**
 ** getrequest() - EXTRACT REQUEST STRUCTURE FROM DISK FILE
 **/
REQUEST * getrequest (char * file)
{
    static REQUEST * _returned_value;
    return _returned_value;
}

/* from file mod32s.c */
char * mod32s ( char * file )
{
    static char *  _returned_value;
    return _returned_value;
}

/**
 ** getreqno() - GET NUMBER PART OF REQUEST ID
 **/
char * getreqno (char * req_id)
{
    static char * _returned_value;
    return _returned_value;
}

/* from file putrequest.c */
/**
 ** putrequest() - WRITE REQUEST STRUCTURE TO DISK FILE
 **/
int putrequest ( char * file, REQUEST * reqbufp)
{
    static int  _returned_value;
    return _returned_value;
}
