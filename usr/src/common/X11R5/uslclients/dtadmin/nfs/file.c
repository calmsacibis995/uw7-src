#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/file.c	1.10"
#endif
/*
 * Module:      dtadmin: nfs  Graphical Administration of Network File Sharing
 * File:        file.c: filename input functions.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h> 

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Gizmo/FileGizmo.h>

#include "nfs.h"
#include "text.h"
#include "verify.h"
#include "notice.h"

extern Boolean removeWhitespace();

/* FIX: move findFolderCB and filePopupCB from localAdv.c and */
/* remoteProp.c here to reduce code duplication               */

extern Boolean
verifyLocalPath(char **value, Gizmo popup,  verifyMode mode,
		XtCallbackProc postVerifyCB)
{
    static noticeData ndata = { NULL, TXT_Create, CHR_Create,
				    NULL, NULL };
    static char * home = NULL;
    static int homelen;
    char *path = *value, *result;
    char  *absPath = NULL;
    static char buf[MAXPATHLEN+500];
    pathData *client_data;
    struct stat statBuffer;
    Widget shell = GetPopupGizmoShell(popup);

    if (path == NULL )
    {
        SetMessage(popup, TXT_EnterLocalPath, Popup);
        return INVALID;
    }

    (void)removeWhitespace(path);
    
    if (*path == EOS)
    {
        SetMessage(popup, TXT_EnterLocalPath, Popup);
        return INVALID;
    }

    if ( *path != '/')
    {
	if (home == NULL)
	{
	    if ((home = getenv("HOME")) == NULL)
	    {
		SetMessage(popup, TXT_EnterFullPath, Popup);
		return INVALID;
	    }
	    homelen = strlen(home);
	}
	if ((absPath = MALLOC(strlen(path) + homelen +2)) == NULL)
	    NO_MEMORY_EXIT();
	strcpy(absPath, home);
	strcat(absPath, "/");
	strcat(absPath, path);
	path = absPath;
	if (*value != NULL)
	    FREE(*value);
	*value = path;
    }

    /* FIX: expand shell meta-characters here */

    while (stat(path, &statBuffer) < 0)
    {
	DEBUG1("stat failed; errno =%d\n", errno);
	switch (errno)
	{
	case EINTR:
	    break;		/* try again */
	case EACCES:
	    SetMessage(popup, TXT_PathAccess, Notice);
	    return INVALID;
	    break;
	case ENOLINK:
	    /* FALL THRU */
	case EMULTIHOP:
	    SetMessage(popup, TXT_PathMustBeLocal, Notice);
	    return INVALID;
	    break;
	case ENOENT:
	    /* popup a notice to give user a chance to create a */
	    /* missing directory.  postVerifyCB must finish     */
	    /* whatever work calling routine would have done if */
	    /* we had returned VALID */ 
	    sprintf(buf, GetGizmoText(TXT_ConfirmCreate), path);
	    ndata.text = buf;
	    if ((client_data = (pathData *)MALLOC(sizeof(pathData))) == NULL)
		NO_MEMORY_EXIT();
	    client_data-> path = path;
	    client_data-> mode = mode;
	    ndata.client_data = (XtPointer)client_data;
	    ndata.callBack    = (XtPointer)postVerifyCB;
	    NoticeCB(shell, &ndata, NULL);
	    return INVALID;
	    break;
	default:
	    SetMessage(popup, TXT_BadPath, Notice);
	    return INVALID;
	    break;
	}
    }
    if (mode == remoteMode && ! S_ISDIR(statBuffer.st_mode))
    {
	sprintf(buf, TXT_DirectoryRequired, path);
	SetMessage(popup, buf, Notice);
	return INVALID;
    }
/** FIX: fill in conditional...
    if ( Path is on a remote system )
    {
	SetMessage(popup, TXT_PathMustBeLocal, Notice);
	return INVALID;
    }
**/    
    return VALID;
}
	
