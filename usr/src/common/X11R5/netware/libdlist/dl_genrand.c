#ident	"@(#)dl_genrand.c	1.2"
#ident	"@(#)dl_genrand.c	2.1 "
#ident  "$Header$"

/*--------------------------------------------------------------------
** Filename : dl_genrand.c
**
** Description : This file contains a routine to generate a random 
**               filename
**
** Functions : GenRandomTempFName
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
** Function : GenRandomTempFName 
**
** Description : This function generates a random filename and returns
**               it to the user in the paramter provided.
**
** Parameters : unsigned char **fName - buffer to store created filename in.
**
** Return : None
**------------------------------------------------------------------*/
void GenRandomTempFName( unsigned char **fName )
{
    unsigned char *nameTemp   = { "tmp%dXXXXXX" }; 
    static int     uniquePart = 1;

    if ( ++uniquePart > 50000 )
        uniquePart = 1;
    *fName = ( unsigned char * ) XtMalloc( strlen( nameTemp ) + 7 + 1 );
    sprintf( *fName, nameTemp, uniquePart );
}
