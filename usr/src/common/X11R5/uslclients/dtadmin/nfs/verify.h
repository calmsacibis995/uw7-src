#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/verify.h	1.5"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	verify.h      input data validation header
 */

extern Boolean verifyLabel();
extern Boolean verifyRemotePath();
extern Boolean verifyLocalPath();
extern Boolean verifyHost();
extern Boolean verifyHost2();
extern Boolean verifyOptions();
extern Boolean verifyMountOptions();
extern Boolean verifyShareOptions();

#define VALID	True
#define INVALID	False

typedef enum _verifyMode
{
    localTabMode,
    localApplyMode,
    remoteMode

} verifyMode;

typedef struct _pathData
{
    char * path;
    verifyMode mode;
} pathData;
