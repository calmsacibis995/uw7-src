#ident	"@(#)la_launch.c	1.2"
#ident  "$Header$"

/*--------------------------------------------------------------------
** Filename : la_launch.c
**
** Description : This is the source file for the Launch_Application 
**               program.
**
** Functions : main
**             GetLaunchInfo
**             BuildLaunchButtons
**             LaunchCB
**             ResetCB
**             CancelCB
**             HelpCB
**             OkCB
**             ExitCB
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                       I N C L U D E S 
**------------------------------------------------------------------*/
#define OWNER_OF_STRINGS

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/RubberTile.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>
#include <Xol/Caption.h>
#include <Xol/TextField.h>
#include <Xol/FooterPane.h>
#include <X11/cursorfont.h>

#include <stdio.h>
#include <sys/utsname.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <locale.h>
#include <signal.h>
#include <errno.h>

void sig_alrm();
extern int errno;

#include "la_login.h"
#include "la_launch.h"

/*--------------------------------------------------------------------
**              I C O N    I N C L U D E    F I L E
**------------------------------------------------------------------*/
#include "remoteApplication.xpm"



/*--------------------------------------------------------------------
**                            D E F I N E S
**------------------------------------------------------------------*/
#define    MAX_ICON_FILE_LINE   120
#define    PERSONAL_EDITION     0x3e4

/*--------------------------------------------------------------------
**                  G L O B A L    V A R I A B L E S 
**------------------------------------------------------------------*/
Widget       top;
Display     *display;
Pixmap      *icon;

static void LoginFieldVerifyCB(Widget, XtPointer, XtPointer);
Widget      passwdEdit;

static XtActionsRec actions[] = {
    "CancelCB",  (XtActionProc) CancelCB,
    "HelpCB",    (XtActionProc) HelpCB,
};
String  transTable = "#override <Key>Escape: CancelCB()";

/*--------------------------------------------------------------------
** Function : main
**
** Description : This is the main function to the application
**               launch program.
**
** Parameters : argc and argv
**
** Return : 0 on Success
**------------------------------------------------------------------*/
int main( int argc, char **argv )
{
    XtAppContext    anyContext;
    Widget          text;
    Widget          launchCaption;
    Widget          launchEdit;
    Widget          passwdCaption;
    Widget          footer;
    Widget          footerStr;
    Widget          baseRubberTile; 
    Widget          launchControl;
    Widget          buttons;

    Pixel           backPix;

    unsigned char  *iconName;
    int             retCode; 
    unsigned char  *authTitle;
    unsigned char  *authText;
    unsigned char  *userID;
    unsigned char  *passwdLabel;
    unsigned char  *footerMsg;
    static userData buttonUserData;
    logItem        *launchButtons;
    XtTranslations translations;


    chdir( "/tmp" );

   	XtSetLanguageProc(NULL, NULL, NULL);
    OlToolkitInitialize( &argc, argv, NULL );

    top = XtAppInitialize( &anyContext, "My New App", NULL, 0,
                           &argc, argv, NULL, NULL, 0 );

    CopyInterStr( UC TXT_AUTH_TITLE, &authTitle, 0 );
    argv[0] = SC authTitle;
   	XtVaSetValues (top, XtNtitle,(XtArgVal) argv[0], 0);

    display = XtDisplay( top );

    timer_cursor = XCreateFontCursor( display, XC_watch );
 
    CopyInterStr( UC TXT_ICON_NAME, &iconName, 0 ); 
    icon = InitIcon( &top, remoteApplication_width, 
                     remoteApplication_height, 
                     remoteApplication_ncolors,
                     remoteApplication_chars_per_pixel, 
                     ( char * ) remoteApplication_colors,
                    ( char * ) remoteApplication_pixels, iconName, TRUE );
    XtFree( ( XtPointer ) iconName );

    XtAppAddActions (anyContext, actions, XtNumber (actions));

    if ( argc != 3 || 
      ( strcmp( argv[1], "-launchF" ) != 0 ) ||
      (( retCode= GetLaunchInfo( argv[2], &serverName, &applicationName ) 
      != SUCCESS )) ||
      ( isSystemSappingAType( PERSONAL_EDITION ) != NULL ) )
    {
        item            *okButton;
        unsigned char   *temp;
        unsigned char   *badFile;
        Widget           corruptFileWidg;
        
        if ( isSystemSappingAType( PERSONAL_EDITION ) != SUCCESS )
            CopyInterStr( UC TXT_NOT_SAPPING, &badFile, 0 );
        else 
            CopyInterStr( UC TXT_FILE_CORRUPT, &badFile, 0 );

        okButton = ( item * ) XtMalloc( sizeof( item ) ); 

        CopyInterStr( okButtonItems->label, &temp, 0 );
        okButton->label = temp;

        CopyInterStr( okButtonItems->mnemonic, &temp, 0 );
        okButton->mnemonic = ( unsigned char * )temp[0];
       
        okButton->select = okButtonItems->select;
        okButton->sensitive = okButtonItems->sensitive;

        baseRubberTile = XtVaCreateManagedWidget( "Base Rubber Tile",
                                                   rubberTileWidgetClass,
                                                   top,
                                                   XtNorientation, OL_VERTICAL,
                                                   ( String ) 0 );
        corruptFileWidg = XtVaCreateManagedWidget( "Corrupt File Message",
                                                    staticTextWidgetClass,
                                                    baseRubberTile, 
                                                    XtNstring, badFile,
                                                    XtNalignment, OL_CENTER,
                                                    ( String ) 0 );
        buttons = XtVaCreateManagedWidget( "OK Button",
                                            flatButtonsWidgetClass,
                                            baseRubberTile,
                                            XtNitems, okButton,
                                            XtNnumItems, numOkButtonItems,
                                            XtNitemFields, itemFields,
                                            XtNnumItemFields, numItemFields,
                                            XtNnoneSet, TRUE,
                                            XtNdefault, FALSE,
                                            ( String ) 0 );
        XtOverrideTranslations(top, XtParseTranslationTable(transTable ));
        translations = XtParseTranslationTable("#override <Key>F1: HelpCB()");
        XtOverrideTranslations(top, translations);
        OlAddCallback( top, XtNwmProtocol, ExitCB, NULL );
        XtRealizeWidget( top );
        XtAppMainLoop( anyContext ); 
         
    }
    /*--------------------------------------------------------------------
    ** If the command line was correct, then build authentication screen.
    **------------------------------------------------------------------*/
    CopyInterStr( UC TXT_AUTH_TXT,     &authText, 1, serverName );
    CopyInterStr( UC TXT_USER_ID,      &userID, 0 );
    CopyInterStr( UC TXT_PASSWD,       &passwdLabel, 0 );
    CopyInterStr( UC TXT_LOGIN_FOOTER, &footerMsg, 0 );

    launchControl = XtVaCreateManagedWidget( "Launch Control",
                                              controlAreaWidgetClass,
                                              top,
                                              XtNalignCaptions, TRUE,
                                              XtNlayoutType, OL_FIXEDCOLS,
                                              ( String ) 0 ); 
    text = XtVaCreateManagedWidget( "Header String",
                                     staticTextWidgetClass,
                                     launchControl,
                                     XtNstring, authText,
                                     ( String ) 0 );
    launchCaption = XtVaCreateManagedWidget( "Login Cap",
                                              captionWidgetClass,
                                              launchControl,
                                              XtNlabel, userID,
                                              XtNposition, OL_LEFT,
                                              ( String ) 0 );
    launchEdit = XtVaCreateManagedWidget( "Launch Edit",
                                           textFieldWidgetClass,
                                           launchCaption,
                                           XtNcharsVisible, MAX_LOGIN_CHARS,
                                           XtNwrapMode, OL_WRAP_OFF,
                                           XtNpreselect, TRUE,
                                           XtNlinesVisible, 1,
                                           ( String ) 0 );
    XtAddCallback (launchEdit, XtNverification,
                        LoginFieldVerifyCB,(XtPointer)NULL);
    passwdCaption = XtVaCreateManagedWidget( "Passwd Caption",
                                              captionWidgetClass,
                                              launchControl,
                                              XtNlabel, passwdLabel,
                                              XtNposition, OL_LEFT,
                                              ( String ) 0 );
    passwdEdit = XtVaCreateManagedWidget( "Passwd Edit",
                                           textFieldWidgetClass,
                                           passwdCaption,
                                           XtNcharsVisible, MAX_PASSWD_CHARS,
                                           XtNpreselect, TRUE,
                                           ( String ) 0 );
    XtVaGetValues( passwdEdit, XtNbackground, &backPix, ( String ) 0 );
    XtVaSetValues( passwdEdit, XtNfontColor, backPix, ( String ) 0 );

    footer = XtVaCreateManagedWidget( "Launch Footer",
                                       footerPanelWidgetClass,
                                       launchControl,
                                       ( String ) 0 );
    buttonUserData.server = serverName;
    buttonUserData.userid = launchEdit;
    buttonUserData.passwd = passwdEdit;
    BuildLaunchButtons( &launchButtons, &buttonUserData );
    
    buttons = XtVaCreateManagedWidget( "Launch Buttons",
                                        flatButtonsWidgetClass,
                                        footer,
                                        XtNitems, launchButtons,
                                        XtNnumItems, numLaunchButtonItems,
                                        XtNitemFields, logItemFields,
                                        XtNnumItemFields, numLogItemFields,
                                        XtNnoneSet, TRUE,
                                        XtNdefault, FALSE,
                                        ( String ) 0 );
    footerStr = XtVaCreateManagedWidget( "Launch Footer Str",
                                          staticTextWidgetClass,
                                          footer,
                                          XtNstring, footerMsg,
                                          ( String ) 0 );
    /*
     * Setup escape overrides so that we are Motif compliant
     */
    translations = XtParseTranslationTable(transTable);
    XtOverrideTranslations(launchEdit, translations);
    XtOverrideTranslations(passwdEdit, translations);
    XtOverrideTranslations(buttons, translations);

    translations = XtParseTranslationTable("#override <Key>F1: HelpCB()");
    XtOverrideTranslations(launchEdit, translations);
    XtOverrideTranslations(passwdEdit, translations);
    XtOverrideTranslations(buttons, translations);

    XtRealizeWidget( top );
    XtAppMainLoop( anyContext );
}



/*--------------------------------------------------------------------
** Function : GetLaunchInfo
**
** Description : This function gets information from a file.
**
** Parameters : filePath   - get server and app from this file
**              serverName - the server name to launch app on 
**              appName    - the application to launch
**
** Return : SUCCESS
**          FAILURE
**------------------------------------------------------------------*/
int GetLaunchInfo( char *filePath, unsigned char **serverName, 
                 unsigned char **appName )
{
    FILE          *fp;
    int            retCode = SUCCESS;
    char          *line    = NULL;
    unsigned char *temp;
  
    fp = fopen( filePath, "r" );
    if ( fp != NULL )
    {
        line = XtMalloc( MAX_ICON_FILE_LINE );
        while( fgets( line, MAX_ICON_FILE_LINE - 1, fp ) != NULL )
        {
            if ( strstr( line, serverTag ) != NULL )
            {
              temp = UC XtMalloc( strlen( line )- strlen( serverTag ) + 1 );
              strcpy( SC temp, &(line[strlen( serverTag )]) );
              if (  temp[strlen( SC temp ) - 1] == '\n' )
                   temp[strlen( SC temp ) - 1] = '\0';
              *serverName = temp;
            }
            else if ( strstr( line, appTag ) != NULL )
            {
              temp = UC XtMalloc( strlen( line )- strlen( appTag ) + 1 );
              strcpy( SC temp, &(line[strlen( appTag )]) );
              if ( temp[strlen( SC temp ) - 1] == '\n' )
                  temp[strlen( SC temp ) - 1] = '\0';
              *appName = temp;
            }
        }
    }
    if ( serverName == NULL ||  appName == NULL )
        retCode = FAILURE;
    if ( line != NULL )
        XtFree( ( XtPointer ) line );
    return( retCode );
}



/*--------------------------------------------------------------------
** Function : BuildLaunchButtons
**
** Description : This function builds the buttons for the 
**               Launch_Application program.
**
** Parameters : buttons - structure to store the buttons in.
**              clientData - client data for each of the buttons 
**              
**
** Return : SUCCESS
**------------------------------------------------------------------*/
int BuildLaunchButtons( logItem **buttons, userData *clientData )
{
    logItem *itemPtr;
    int      i;

    *buttons = itemPtr = 
       ( logItem * ) XtMalloc ( sizeof( logItem ) * numLaunchButtonItems );
    for ( i = 0; i < numLaunchButtonItems; i++ )
    {
        unsigned char *temp;

        CopyInterStr( launchButtonItems[i].label, &temp, 0 );
        itemPtr->label = temp;
        CopyInterStr( launchButtonItems[i].mnemonic, &temp, 0 );
        itemPtr->mnemonic = ( XtPointer ) temp[0];
        itemPtr->select = launchButtonItems[i].select;
        itemPtr->deflt = launchButtonItems[i].deflt;
        itemPtr->clientData = ( XtPointer ) clientData;
        itemPtr++;
    }
}


/*--------------------------------------------------------------------
** Function : LaunchCB
**
** Description : Callback function to launch a remote application. 
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void  LaunchCB( Widget w, XtPointer clientData, XtPointer callData )
{
    char             *commandBuf = NULL;
    char             *command =
                      { "exec \"/usr/X/lib/app-defaults/.exportApps/%s\" %s:0 &" };
    char             *buf;
    char             *formatBuf;
    unsigned char    *errorStr = NULL;
    Arg               args[1];
    int               ioFD, errFD;
    int               retCode;
    unsigned char    *messageLabel;
    Cardinal          size;
    userData         *data;
    int               charsread; 
    int               localErrFD;
    char             *fileName = NULL;
    Boolean           appLaunched = FALSE;
  
    static char      *xhostDisplay = NULL;
    struct utsname    name;
    
    data = ( userData * )clientData; 

    /*----------------------------------------------------------
    ** Get display name for this machine, if we haven't already
    ** gotten it. Store it in a safe and static memory location.
    **--------------------------------------------------------*/ 
    retCode = AccessXhost( w, serverName, ADD_XHOST ); 
    if ( retCode != SUCCESS )
        goto EXIT_LAUNCH;
  
    if ( xhostDisplay == NULL )
    {
        retCode = uname( &name );
        if ( retCode == -1 )
        {
           CopyInterStr( UC TXT_CANT_GET_XNAME, &errorStr, 0 );
           displayErrorMsg( w, errorStr );
           goto EXIT_LAUNCH;
        }
        else
        {
           xhostDisplay = XtMalloc( strlen( name.nodename ) + 1 );
           strcpy( xhostDisplay, name.nodename );
        }
    }    
    XDefineCursor( XtDisplay( top ), XtWindow( top ), timer_cursor );
    XSync( XtDisplay( top ), False );

    /*------------------------------------------------------------------
    ** Now make the remote call to launch the application. We have 
    ** to pick up all potential errors on both sides of the connection.
    **----------------------------------------------------------------*/
    close( 2 );
    GenRandomTempFName( ( unsigned char ** )&fileName );
    mktemp( fileName );
    localErrFD = creat( fileName, O_RDWR );

    commandBuf = ( XtPointer ) XtMalloc( strlen( command ) +
                                         strlen( SC applicationName ) + 
                                         strlen( xhostDisplay ) + 1 );
    sprintf( commandBuf, command, applicationName, xhostDisplay );
    currSess.userID = UC OlTextFieldGetString( data->userid, &size );
    currSess.passwd = UC OlTextFieldGetString( data->passwd, &size );
    XtVaSetValues( data->passwd, XtNstring, NULL, ( String ) 0 );
    ioFD = rexec( &serverName, REXEC_SOCKET, currSess.userID, 
                  currSess.passwd, commandBuf, NULL );

    errorStr = ( XtPointer ) XtMalloc( MAX_REXEC_ERR );
    if ( ioFD == -1 )
    { 
        close( localErrFD );
        localErrFD = open( fileName, O_RDONLY ); 
        charsread = read( localErrFD, errorStr, MAX_REXEC_ERR - 1 );
        if ( charsread == -1 )
        {
            XtFree( ( XtPointer )errorStr );
            CopyInterStr( UC TXT_UNABLE_TO_CONNECT, &errorStr, 1, serverName );
        }
        else
            errorStr[charsread - 1] = '\0';
        if ( strlen( ( char * )errorStr ) == 0 )
        {
            XtFree( ( XtPointer ) errorStr );
            CopyInterStr( UC TXT_NETWORK_ERROR, &errorStr, 0 );
        }
        displayErrorMsg( w, errorStr );
    }
    else
    { 
        fcntl( ioFD, F_SETFL, O_NDELAY | O_NONBLOCK | O_APPEND );
        sleep( 7 );
		memset(errorStr,0,MAX_REXEC_ERR);
        charsread = read( ioFD, errorStr, 1 );

        if ( charsread >  0 )
        {
			/*
			 * Alright, got an error message back
			 * give him/her 5 sec to give me the rest.
			 * Can't hang around here forever
			 */
            int i = 1;
			int jj;

			signal(SIGALRM,sig_alrm);
			alarm(5);

            fcntl( ioFD, F_SETFL, O_APPEND );
            while( ( ( jj = read( ioFD, &(errorStr[i]), 1 ) ) != 0 ) &&
                 ( i < ( MAX_REXEC_ERR - 1 ) ) )
			{
				if ( jj == -1 &&  errno == EINTR )
				{
					/* Alarm poped, break from the read */
					/* Display what we have */
					break;
				}
                i++;
			}
    		(void) alarm(0);
			signal(SIGALRM,(void (*)())SIG_DFL);

            errorStr[i] = '\0';
            if ( strlen( ( char * )errorStr ) == 0 )
            {
                XtFree( ( XtPointer ) errorStr );
                CopyInterStr( UC TXT_NETWORK_ERROR, &errorStr, 0 );
            }
            if ( strlen((char *) errorStr ) > 2 )
            	displayErrorMsg( w, errorStr );
        }
        else appLaunched = TRUE;
    }
EXIT_LAUNCH:
    if ( commandBuf != NULL )
        XtFree( ( XtPointer )commandBuf );
    if ( errorStr != NULL ) 
        XtFree( ( XtPointer )errorStr );
    if ( localErrFD )
    {
        close( localErrFD );
        unlink( fileName );
        XtFree( fileName );
    }
    XUndefineCursor( XtDisplay( top ), XtWindow( top ) );
    if ( appLaunched == TRUE )
        exit( 0 );
}



/*--------------------------------------------------------------------
** Function : ResetCB
**
** Description : Callback function to reset user id and password fields.
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void  ResetCB( Widget w, XtPointer clientData, XtPointer callData )
{
    userData *data;

    data = ( userData * ) clientData;
 
    XtVaSetValues( data->userid, XtNstring, NULL, ( String ) 0 );
    XtVaSetValues( data->passwd, XtNstring, NULL, ( String ) 0 );
}

/*--------------------------------------------------------------------
** Function : CancelCB
**
** Description : Callback function to exit application. 
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void  CancelCB( Widget w, XtPointer clientData, XtPointer callData )
{
    AccessXhost( w, NULL, CLEANUP_XHOST );
    exit( 0 );
}


/*--------------------------------------------------------------------
** Function : HelpCB
**
** Description : Callback function to display help about application. 
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void  HelpCB( Widget w, XtPointer clientData, XtPointer callData )
{
}


/*--------------------------------------------------------------------
** Function : OkCB
**
** Description : Callback function to exit screen when the icon file was
**               corrupted or the command line was incorrect.
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void  OkCB( Widget w, XtPointer clientData, XtPointer callData )
{
    exit( 0 );
}


/*--------------------------------------------------------------------
** Function : ExitCB
**
** Description : This function is called when the application is closed.
**               ( It is called at other times by the window manager,
**                 but this is the only event we care about ). We clean
**                 up xhost in this routine.
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
static void ExitCB( Widget w, XtPointer clientData, XtPointer callData )
{
    OlWMProtocolVerify    *wmData = ( OlWMProtocolVerify * ) callData;

    if ( wmData->msgtype == OL_WM_DELETE_WINDOW )
    {
        XCloseDisplay( XtDisplay( top ) );
        AccessXhost( w, NULL, CLEANUP_XHOST );
        exit( 0 );
    }
}

/******************************************************************
   Move to password field on "CR" in login field procedure 
 *******************************************************************/
static void
LoginFieldVerifyCB(Widget w, XtPointer client_data,XtPointer call_data)
{
    OlTextFieldVerify *tfv = ( OlTextFieldVerify *)call_data;
    Time            time;

    if ( tfv->reason == OlTextFieldReturn && strlen(tfv->string))
    {
        tfv->ok = False;
        time = XtLastTimestampProcessed(XtDisplay(w));
        if (OlCanAcceptFocus(passwdEdit, time))
            OlSetInputFocus(passwdEdit, RevertToNone, time);
    }
    if ( !(strlen(tfv->string)))
    {
        tfv->ok = False;
    }
}
/*--------------------------------------------------------------------
** Funciton : sig_alrm 
**
** Description : This function is called when the alarm timeout expires and
**               the SIGALRM handler has been pointed here. Used to terminate 
**               blocking reads, etc.
**          
**               CAUTION: Alarm handler is set to default. ( I know SVR4
**                        does it automatically, but who knows maybe they
**                        will fix it some day )
**
** Parameters : Should be SIGALRM
**
** Return : None
**------------------------------------------------------------------*/
void
sig_alrm(int signo )
{
	signal(SIGALRM,(void (*)())SIG_DFL);
	return;
}
