#ident	"@(#)dl_issap.c	1.2"
#ident	"@(#)dl_issap.c	2.1 %Q"
#ident  "$Header$"

/*--------------------------------------------------------------------
** Filename : dl_issap.c
**
** Description : This file contains a function to check to see if 
**               a machine is sapping a a particular sap type.
**
** Functions : IsSap
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                        I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <stdlib.h>
#include <dirent.h>

#include "dl_common.h"
#include "dl_protos.h"


/*--------------------------------------------------------------------
**                        D E F I N E S
**------------------------------------------------------------------*/
#define     SAP_OUT_DIR       "/var/spool/sap/out"


/*--------------------------------------------------------------------
** Function : IsSap
**
** Parameters : long  sapFile - The number of the sap file.
**
** Return : TRUE if the file exists
**          FALSE if the file doesn't exist
**------------------------------------------------------------------*/
int IsSap( long sapFile )
{
    int                retCode = FAILURE;
    DIR               *Dp;
    struct dirent     *dirPtr;

    Dp = opendir( SAP_OUT_DIR );
    for ( dirPtr = readdir( Dp ); dirPtr != NULL; dirPtr = readdir( Dp ) )
        if ( ( strtol( dirPtr->d_name, NULL, 0 ) ) == sapFile )
            retCode = SUCCESS;
    return( retCode );
}
