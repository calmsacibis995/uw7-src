#ident	"@(#)la_login.h	1.2"
#ident	"@(#)la_login.h	2.1 "
#ident  "$Header$"

/*--------------------------------------------------------------------
** Filename : la_login.h
**
** Description : This is the header file for la_launch.c
**------------------------------------------------------------------*/

/*--------------------------------------------------------------------
**                       D E F I N E S 
**------------------------------------------------------------------*/
#define    MAX_LOGIN_CHARS          20
#define    MAX_PASSWD_CHARS         20

/*--------------------------------------------------------------------
**                       T Y P E D E F S 
**------------------------------------------------------------------*/
typedef struct _logitem {
    XtPointer    label;
    XtPointer    mnemonic;
    XtPointer    select;
    Boolean      deflt;
    XtPointer    clientData;
} logItem;


typedef struct buttondata {
    unsigned char        *server;
    Widget                userid;
    Widget                passwd;
    Widget                popup;
    Pixmap               *icon;
} buttonData;
    

static char      *logItemFields[] = { XtNlabel, 
                                      XtNmnemonic, 
                                      XtNselectProc,
                                      XtNdefault,
                                      XtNclientData };
static int        numLogItemFields = XtNumber( logItemFields );
