#ident	"@(#)la_launch.h	1.3"

/* Modification History:
 *
 * 9-Oct-97	daveli
 *	Changed titles to "Launch Application"
 */
/*--------------------------------------------------------------------
** Filename : dl_launch.h
**
** Description : This header file contains definitions, typedefs, 
**               structs, etc. for the Launch_Application program.
**------------------------------------------------------------------*/

/*--------------------------------------------------------------------
**                         I N C L U D E S
**------------------------------------------------------------------*/
#include <stdarg.h>
#include "dl_fsdef.h"
#include "dl_common.h"
#include "dl_protos.h"

/*--------------------------------------------------------------------
**          I N T E R N A T I O N A L I Z A T I O N 
**------------------------------------------------------------------*/
#define    TXT_ICON_NAME     "launchappl:2" FS "Launch Application"
#define    TXT_LAUNCH        "launchappl:3" FS "Open"
#define    TXT_M_LAUNCH      "launchappl:4" FS "O"
#define    TXT_RESET         "launchappl:5" FS "Reset"
#define    TXT_M_RESET       "launchappl:6" FS "R"
#define    TXT_CANCEL        "launchappl:7" FS "Cancel"
#define    TXT_M_CANCEL      "launchappl:8" FS "C"
#define    TXT_HELP          "launchappl:9" FS "Help" 
#define    TXT_M_HELP        "launchappl:10" FS "H"
#define   TXT_AUTH_TITLE     "launchappl:11" FS "Launch Application"
#define   TXT_AUTH_TXT       "launchappl:12" FS "Enter your user ID and password for remote system %s"
#define   TXT_USER_ID        "launchappl:13" FS "Login ID"
#define   TXT_PASSWD         "launchappl:14" FS "Password"
#define   TXT_LOGIN_FOOTER   "launchappl:15" FS "Password entry will not be displayed as you type it."
#define   TXT_FILE_CORRUPT   "launchappl:16" FS "Cannot get server and application information from the file."
#define   TXT_OK_BUTTON      "launchappl:17" FS "Ok"
#define   TXT_OK_M_BUTTON    "launchappl:18" FS "O"
#define   TXT_UNABLE_TO_CONNECT "launchappl:19" FS "Unable to connect to %s"
#define   TXT_CANT_GET_XNAME "launchappl:20" FS "Unable to get xhost name"
#define   TXT_SECT_TAG_10    "launchappl:21" FS "10"
#define   TXT_NETWORK_ERROR  "launchappl:22" FS "A network error occurred while performing this function."
#define   TXT_NOT_SAPPING    "launchappl:23" FS "To execute this application you must first execute NetWare_Setup and turn peer-to-peer on."



/*--------------------------------------------------------------------
**                      P R O T O T Y P E S
**------------------------------------------------------------------*/
Pixmap  *InitIcon        ( Widget *, int, int, int, int,
                           char *, char *, unsigned char *, int );

void CopyInterStr        ( unsigned char *, unsigned char **, int, ... );

void     LaunchCB        ( Widget, XtPointer, XtPointer );
void     ResetCB         ( Widget, XtPointer, XtPointer );
void     CancelCB        ( Widget, XtPointer, XtPointer );
void     HelpCB          ( Widget, XtPointer, XtPointer );
static void ExitCB       ( Widget, XtPointer, XtPointer );

void     OkCB            ( Widget, XtPointer, XtPointer );

/*--------------------------------------------------------------------
**                          D E F I N E S
**------------------------------------------------------------------*/
#define          MAX_PID_LEN         20
#define          MAX_REXEC_ERR       201

#define          ERR_INSUFF_MEM      0x20

#define          WINDOW_X_POS        50
#define          WINDOW_Y_POS        50
#define          WINDOW_HEIGHT       220
#define          WINDOW_WIDTH        520

#define          REXEC_SOCKET        512


/*--------------------------------------------------------------------
**                         T Y P E D E F S 
**------------------------------------------------------------------*/
static char *itemFields[]={XtNlabel, XtNmnemonic, XtNselectProc, XtNsensitive};

static int numItemFields = XtNumber( itemFields );

typedef struct _session { 
    unsigned char *userID;
    unsigned char *passwd;
} Session;

typedef struct _userdata {
    unsigned char   *server;
    Widget  userid;
    Widget  passwd;
} userData;
    

#ifdef OWNER_OF_STRINGS
Session currSess = { NULL, NULL };
unsigned char   *serverName = NULL;
unsigned char   *applicationName   = ( unsigned char * )"Launch_Application";
char   *iconFile = { "/usr/X/lib/pixmaps/remoteApplication.xpm" };
char   *helpFile = { "Launch_Application/Open_Application.hlp" };
char   *helpDir  = { "/usr/X/lib/locale/C/help/Launch_Application" };
char *serverTag =  { "*Server-" };
char *appTag    =  { "*App-" };
#else
extern Session currSess;
extern unsigned char   *serverName;
extern unsigned char   *applicationName;
extern char   *iconFile;
extern char   *helpFile;
extern char   *helpDir;
extern char   *serverTag;
extern char   *appTag;
#endif


static logItem launchButtonItems[] = {
 { TXT_LAUNCH,    TXT_M_LAUNCH,    ( XtPointer ) LaunchCB,  TRUE,  NULL },
 { TXT_RESET,     TXT_M_RESET,     ( XtPointer ) ResetCB,   FALSE, NULL },
 { TXT_CANCEL,    TXT_M_CANCEL,    ( XtPointer ) CancelCB,  FALSE, NULL },
 { TXT_HELP,      TXT_M_HELP,      ( XtPointer ) HelpCB,    FALSE, NULL }
};

static int numLaunchButtonItems = 4;

static item okButtonItems[] = {
    { TXT_OK_BUTTON, TXT_OK_M_BUTTON, ( XtPointer ) OkCB, ( XtPointer ) TRUE }
};

static int numOkButtonItems = 1;

static int numButtonItems = 4;

Cursor timer_cursor;

/*--------------------------------------------------------------------
**                      P R O T O T Y P E S
*------------------------------------------------------------------*/
void GetServerName( unsigned char * );
void GenRandomTempFName( unsigned char ** );
void AppendToNameList( nameList **, unsigned char * );
