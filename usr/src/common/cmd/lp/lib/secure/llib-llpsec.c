/*		copyright	"%c%" 	*/

#ident	"@(#)llib-llpsec.c	1.2"
#ident	"$Header$"

/* LINTLIBRARY */

# include	"secure.h"

/**
 ** getsecure() - EXTRACT SECURE REQUEST STRUCTURE FROM DISK FILE
 **/

SECURE * getsecure ( char * file )
{
    static SECURE * _returned_value;
    return _returned_value;
}

/**
 ** putsecure() - WRITE SECURE REQUEST STRUCTURE TO DISK FILE
 **/

int putsecure ( char * file, SECURE * secbufp )
{
    static int _returned_value;
    return _returned_value;
}

void freesecure ( SECURE * secbufp )
{
}
