#pragma ident	"@(#)dtm:error.c	1.12"

/******************************file*header********************************

    Description:
	This file contains messages that must be "compiled in".  These
	are messages that are accessed via an error number.
*/
						/* #includes go here	*/
#include "dm_strings.h"

static char * FileOpErrMsgs[] = {
    0,				/*  0	no error */
    TXT_IsAFile,		/*  1	ERR_IsAFile */
    TXT_Rm,			/*  2	ERR_Rm */
    TXT_Read,			/*  3	ERR_Read */
    TXT_Write,			/*  4	ERR_write */
    TXT_NotAFile,		/*  5	ERR_NotAFile */
    TXT_Mkdir,			/*  6	ERR_Mkdir */
    TXT_Stat,			/*  7	ERR_Stat */
    TXT_OpenSrc,		/*  8	ERR_OpenSrc */
    TXT_OpenDst,		/*  9	ERR_OpenDst */
    TXT_Link,			/* 10	ERR_Link */
    TXT_Rename,			/* 11	ERR_Rename */
    TXT_ForeignLink,		/* 12	ERR_ForeignLink */
    TXT_NoDirHardlink,		/* 13	ERR_NoDirHardlink */
    TXT_MoveToSelf,		/* 14	ERR_MoveToSelf */
    TXT_CopyToSelf,		/* 15	ERR_CopyToSelf */
    TXT_LinkToSelf,		/* 16	ERR_LinkToSelf */
    TXT_MoveToDesc,		/* 17	ERR_MoveToDesc */
    TXT_CopyToDesc,		/* 18	ERR_CopyToDesc */
    TXT_ReadLink,		/* 19	ERR_ReadLink */
    TXT_OpenDir,		/* 20	ERR_OpenDir */
    TXT_CopySpecial,		/* 21	ERR_CopySpecial */
    TXT_TargetFS,		/* 22	ERR_TargetFS */
    TXT_NoSrc,			/* 23	ERR_NoSrc */
    TXT_MoveRoot,		/* 24	ERR_MoveRoot */
    TXT_MoveDesktop,		/* 25	ERR_MoveDesktop */
    TXT_MoveWastebasket,	/* 26	ERR_MoveWastebasket */
    TXT_OpenErr,		/* 27	ERR_OpenErr */
    TXT_CreateEntry,		/* 28	ERR_CreateEntry */
    TXT_NoDirConvert,		/* 29   ERR_NoDirConvert */
    TXT_ConvertSpecial,		/* 30   ERR_ConvertSpecial */
    TXT_Linktodos,		/* 31   ERR_Linktodos */
    TXT_OverwriteParentDir,	/* 32   ERR_OverwriteParentDir */
    TXT_ActiveMountPoint,	/* 33   ERR_ActiveMountPoint */
    TXT_MoveFailed,		/* 34   ERR_MoveFailed */
};
/****************************procedure*header*****************************
    StrError- emulates strerror(3C) to return pointer to error messages
*/
char *
StrError(int err)
{
    return(((err < 0) || (err >= sizeof(FileOpErrMsgs))) ? 0 :
	   FileOpErrMsgs[err]);
}
