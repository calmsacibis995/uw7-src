#ident	"@(#)dl_i18n.c	1.2"
#ident	"@(#)dl_i18n.c	2.1 "
#ident  "$Header$"

/*--------------------------------------------------------------------
** Filename : dl_i18n.c
**
** Description : This file contains a function that is useful for 
**               internationalizing code.
**
** Functions : CopyInterStr 
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>

#include "dl_fsdef.h"
#include "dl_common.h"

/*--------------------------------------------------------------------
**                            I N C L U D E S 
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "dl_protos.h"
#include "dl_fsdef.h"

/*--------------------------------------------------------------------
** Function : CopyInterStr
**
** Description : This function gets a string from an international
**               string, and copies it into a character array.
**
** Parameters : iStr - String to internationalize
**              buf  - buffer to put internationalized string in 
**              numArgs - strings to put in string with sprintf
**              variable number of arguments. Currently just use 1
**
** Note - temp should be freed by the calling application when 
**        necc.
**
** Return : None
**------------------------------------------------------------------*/
void  CopyInterStr( unsigned char *iStr, unsigned char **buf, 
                    int numArgs, ... )
{
    unsigned char      *temp = NULL;
    va_list   ap;
    int       length = 0;
    int       i;

    va_start( ap, numArgs );
 
    length += strlen( SC GetInternationalStr( iStr ) );
    for ( i = 0; i < numArgs; i++ )
        length += strlen( va_arg( ap, char * ) );
    length++;      /* For the terminating char */
    temp = ( unsigned char * ) XtMalloc ( length ); 
    va_start( ap, numArgs );
    if ( numArgs != 0 )
        sprintf( ( char * )temp, SC GetInternationalStr( iStr ), 
                              va_arg( ap, char * ) ); 
    else
        strcpy( ( char * )temp, SC GetInternationalStr( iStr ) );
    *buf = temp;
}
