#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/notice.h	1.3"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	notice.h      notice box header
 */

extern void DeleteCB();

typedef struct _noticeData
{
    char          *text;
    char          *label;
    char          *mnemonic;
    XtPointer      callBack;	/* really XtCallBackProc */
    XtPointer	   client_data;
	
} noticeData;

typedef enum _noticeIndex
{
    NoticeDoIt, NoticeCancel, NoticeHelp
} noticeIndex;

extern MenuItems  NoticeMenuItems[];
extern MenuGizmo NoticeMenu;
extern ModalGizmo noticeG;
