#ident	"@(#)dl_errormsg.c	1.2"
#ident  "$Header$"

/*--------------------------------------------------------------------
** Filename : dl_errormsg.c
**
** Description : This file contains the definitions and functions to 
**               pop up an error message window.
**
** Functions : DestroyCB
**             shutdownErrorMsg
**             displayErrorMsg
**             GetErrorMsg
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                        I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Notice.h>
#include <Xol/FButtons.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "dl_fsdef.h"
#include "dl_msgs.h"
#include "dl_common.h"
#include "dl_protos.h"

/*--------------------------------------------------------------------
**          F U N C T I O N    P R O T O T Y E S  
**------------------------------------------------------------------*/
void shutdownErrorMsg ( Widget, XtPointer, XtPointer );

/*--------------------------------------------------------------------
**                        V A R I A B L E S 
**------------------------------------------------------------------*/


static    item ok_button[1];

/*--------------------------------------------------------------------
** Function : DestroyCB
**
** Description : 
**
** Parameters : As per callback routine
**
** Return : None
**------------------------------------------------------------------*/
void  DestroyCB ( Widget w, XtPointer client_data, XtPointer call_data )
{
    XtDestroyWidget ( w );
    XtVaSetValues ( ( Widget ) client_data, XtNbusy, False, NULL );
    XtFree( ( XtPointer )ok_button[0].label );
}


/*--------------------------------------------------------------------
** Function : shutdownErrorMsg
**
** Description : This function pops down the error msg window, when
**               the OK button is selected.
**
** Parameters : As per callback routines 
**
** Return : None
**------------------------------------------------------------------*/
void shutdownErrorMsg ( Widget w, XtPointer client_data, XtPointer call_data )
{
    XtVaSetValues ( ( Widget ) client_data, XtNbusy, False, 0 );
    XtPopdown ( ( Widget ) XtParent( XtParent ( w ) ) );
    XtFree( ( XtPointer )ok_button[0].label );
}


/*--------------------------------------------------------------------
** Function : displayErrorMsg 
**
** Description : This function displays an error msg in a 
**               noticeShellWidget.
**
** Parameters : Widget w       - Widget that initiated the call
**              unsigned char *errorMsg - The error message to display 
**                                        in the Notice shell.
** Return : None
**------------------------------------------------------------------*/
void displayErrorMsg ( Widget w, unsigned char *errorMsg )
{
    Widget        notice;
    Widget        buttons;
    Widget        flatButtons;
    Widget        textArea;
    Arg           args[1];
    static char   *itemFields[] = { XtNlabel, XtNmnemonic, XtNselectProc };
    static int    numItemFields = XtNumber ( itemFields );
    unsigned char *mnemonicChar;
	unsigned char *title;

    CopyInterStr( TXT_ERR_TITLE, &title, 0 );

    notice = XtVaCreatePopupShell ( "notice",
                                    noticeShellWidgetClass,
                                    XtParent ( w ),
                                    XtNstring, errorMsg,
                                    XtNtitle, title,
                                    XtNnoticeType, OL_ERROR,
                                    XtNemanateWidget, w,
                                    ( String ) 0 );
    XtFree( ( XtPointer )title );

    XtAddCallback ( notice, XtNpopdownCallback, DestroyCB, w );
    XtVaGetValues ( notice, XtNcontrolArea, &buttons, 0 );

    CopyInterStr( TXT_ERR_OK, &(ok_button[0].label), 0 );
    CopyInterStr( TXT_ERR_M_OK, &mnemonicChar, 0 );
    ok_button[0].mnemonic = ( XtPointer ) mnemonicChar[0];
    XtFree( ( XtPointer )mnemonicChar );
    ok_button[0].select = ( XtPointer )shutdownErrorMsg; 

    flatButtons = XtVaCreateManagedWidget ("OK",
                             flatButtonsWidgetClass,
                             buttons,
                             XtNitems, ok_button,
                             XtNnumItems, XtNumber ( ok_button ),
                             XtNitemFields, itemFields,
                             XtNnumItemFields, numItemFields,
                             XtNclientData, notice,
                             ( String ) 0 );
    XtSetArg( args[0], XtNdefault, TRUE );
    OlFlatSetValues( flatButtons, 0, args, 1 );
    XtVaGetValues ( notice, XtNtextArea, &textArea, 0 );
    XtSetArg( args[0], XtNalignment, OL_CENTER );
    XtSetValues( textArea, args, 1 );
    XtPopup ( notice, XtGrabExclusive );
}


/*--------------------------------------------------------------------
** Function : GetErrorMsg 
**
** Description : This functions checks to see if there is input in 
**               the error msg file, and if there is it copies that
**               into the input string ( after allocating memory ),
**               This function also deletes the error msg file.
**
** Parameters : fileName  - The name of the file where error 
**                          msgs are logged.
**              errorMsg  - Buffer where the error msg is 
**                          returned.        
**
** Return : None
**------------------------------------------------------------------*/
void GetErrorMsg( unsigned char *fileName, unsigned char **errorMsg )
{
    struct stat     errorStat;
    FILE            *errorFP = NULL;
    int             ret;

    ret = stat( ( char * )fileName, &errorStat );
    if ( ret != -1 &&  errorStat.st_size != 0 )
    {
        *errorMsg = ( unsigned char * ) XtMalloc( errorStat.st_size + 1 );
        errorFP = fopen( ( char * )fileName, "r" );
        fread( *errorMsg, 1, errorStat.st_size, errorFP );
        ( *errorMsg )[errorStat.st_size - 1] = '\0';
    }
    else
        *errorMsg = NULL;
    if ( errorFP )
        fclose( errorFP );
    unlink( fileName );
}
