#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/verify.c	1.11"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	verify.c      input data validation
 *                            host validation is in hosts.c
*/

#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>

#include "nfs.h"
#include "verify.h"
#include "text.h"

extern Boolean removeWhitespace();

extern Boolean 
verifyLabel(char * label, Gizmo popup)
{
    char *start = label, *end;

    (void)removeWhitespace(label);

    if (label == NULL || *label == EOS)
    {
	SetMessage(popup, TXT_EnterLabel, Popup);
	return INVALID;
    }

    if (strpbrk(label, WHITESPACE) != NULL)
    {
	SetMessage(popup, TXT_WhiteInLabel, Notice);
	return INVALID;
    }
    return VALID;
}
extern Boolean 
verifyRemotePath(char * path, Gizmo popup)
{
    (void)removeWhitespace(path);

    if (path == NULL || *path == EOS)
    {
	SetMessage(popup, TXT_EnterRemotePath, Popup);
	return INVALID;
    }
    if ( *path != '/')
    {
	SetMessage(popup, TXT_BadRemotePath, Notice);
	return INVALID;
    }
    return VALID;
}
/*
extern Boolean 
verifyLocalPath(char * path, Gizmo popup, verifyMode why)
{
    (void)removeWhitespace(path);

    if (path && *path)
	return VALID;
    else
    {
	SetMessage(popup, TXT_EnterLocalPath, Popup);
	return INVALID;
    }
}
*/

extern Boolean 
verifyShareOptions(char *options, Gizmo popup)
{
    (void)removeWhitespace(options);

    if (options == NULL)
	return VALID;
    if (strpbrk(options, WHITESPACE) != NULL)
    {
	SetMessage(popup, TXT_SpaceNotAllowed, Notice);
	return INVALID;
    }
    /* FIX: call /usr/lib/fs/nfs/share -V */
    return VALID;
}

#define NFS_MOUNT_CMD	"/usr/lib/fs/nfs/mount -V -o "
#define DUMMY_ARGS	" a:/ /"
#define LEN_NFS_MOUNT_CMD 35	/* includes DUMMY_ARGS */

extern Boolean
verifyMountOptions(char *options, Gizmo popup)
{
    char * buf;
    int status;

    (void)removeWhitespace(options);

    if (options == NULL || *options == EOS)
	return VALID;
    if (strpbrk(options, " \t") != NULL)
    {
	SetMessage(popup, TXT_SpaceNotAllowed, Notice);
	return INVALID;
    }
    if ((buf = MALLOC(strlen(options) + LEN_NFS_MOUNT_CMD)) == NULL)
	NO_MEMORY_EXIT();
    strcpy(buf, NFS_MOUNT_CMD);
    strcat(buf, options);
    strcat(buf, DUMMY_ARGS);
    status = system(buf);
    FREE(buf);
    if (!WIFEXITED(status))
    {
	DEBUG1("could not verify Mount options; status=%x\n", status);
	SetMessage(popup, TXT_OptionsNotVerified, Notice);
	return VALID;
    }
    if (WEXITSTATUS(status) == 0)
	return VALID;
    else
    {
	SetMessage(popup, TXT_BadMountOptions, Notice);
	return INVALID;
    }
}
