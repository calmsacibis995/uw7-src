#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/notice.c	1.13"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	notice.c      notice box code
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>

#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>
#include <Xol/FList.h>
#include <Xol/PopupWindo.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include "text.h"
#include "nfs.h"
#include "notice.h"

void   ErrorNotice();
void   OKCB();
extern Boolean DeleteRemoteCB();
extern standardCursor();
static MenuItems  NoticeMenuItems[] =
   {
      {True, TXT_DoIt,	   CHR_DoIt },
      {True, TXT_Cancel,   CHR_Cancel },
      {True, TXT_Help,     CHR_Help },
      { 0 }
   };

extern MenuGizmo NoticeMenu =
   { NULL, "noticeMenu", "noticeMenu", NoticeMenuItems, NULL };

static MenuItems errnote_item[] = {
        { TRUE, TXT_OK,  CHR_OK, 0, OKCB },
        { NULL }
};

static  MenuGizmo errnote_menu = {0, "note", "note", errnote_item };
static  ModalGizmo errnote = {0, "warn", TXT_errLine, (Gizmo)&errnote_menu };

static ModalGizmo noticeG =
{
    NULL,
    "notice",
    FormalClientName,
    &NoticeMenu,
    "",
    NULL,
    0,
};

void
ErrorNotice (char *buf)
{
     extern NFSWindow *nfsw;
        if (!errnote.shell)
                CreateGizmo(GetBaseWindowShell(nfsw-> baseWindow), ModalGizmoClass, &errnote, NULL, 0);

        SetModalGizmoMessage(&errnote, buf);
        OlVaFlatSetValues(errnote_menu.child, 0, XtNclientData,
                      (XtArgVal)0, 0);
        MapGizmo(ModalGizmoClass, &errnote);
}

void
OKCB(Widget wid, XtPointer client_data, XtPointer call_data)
{

    BringDownPopup(errnote.shell);
}



extern Boolean
CreateNotice(XtPointer client_data)
{
    extern NFSWindow *nfsw;

    if (nfsw-> noticePopup == NULL)
    {
        nfsw-> noticePopup = &noticeG;
        CreateGizmo(GetBaseWindowShell(nfsw-> baseWindow),
                    ModalGizmoClass, nfsw-> noticePopup, NULL, 0);
    }
    return True;
}


extern void 
NoticeCB(Widget w, XtPointer client_data, XtPointer call_data) 
{
    extern NFSWindow *nfsw;
    noticeData       *data = (noticeData *)client_data;
    char 	     mnemonic;

    (void)CreateNotice(NULL);

    /* set the appropriate message and button label */

    SetModalGizmoMessage(nfsw-> noticePopup, GetGizmoText(data->text));
    mnemonic = *GetGizmoText(data-> mnemonic);
    OlVaFlatSetValues(NoticeMenu.child, NoticeDoIt,
		      XtNlabel, GetGizmoText(data-> label), 
		      XtNmnemonic, mnemonic,
		      XtNselectProc, data-> callBack,
		      XtNclientData, data-> client_data, NULL); 
    
    OlVaFlatSetValues(NoticeMenu.child, NoticeCancel,
		      XtNselectProc, data-> callBack,
		      XtNclientData, data-> client_data, NULL); 

    OlVaFlatSetValues(NoticeMenu.child, NoticeHelp,
		      XtNselectProc, data-> callBack, 
		      XtNclientData, data-> client_data, NULL); 

    MapGizmo(ModalGizmoClass, nfsw-> noticePopup);
    return;
}

extern void 
DeleteCB(Widget w, XtPointer client_data, XtPointer call_data) 
{
    OlFlatCallData * p          = (OlFlatCallData *)call_data;
    NFSWindow *     nfsw        = FindNFSWindow(w);
    ModalGizmo *     popup      = nfsw-> noticePopup;
    Widget	    shell	= GetModalGizmoShell(popup);
    Boolean	    result;

    DEBUG1("DeleteCB entry, index=%d\n", p-> item_index);
    switch ((noticeIndex)p-> item_index)
    {
    case NoticeDoIt:
	SetMessage(nfsw, "", Base);
	if (nfsw-> viewMode == ViewRemote)
	    result =  DeleteRemoteCB(w, client_data, ReDoIt_Confirm);
	else		
	    result = DeleteLocalCB(w, client_data, ReDoIt_Confirm);
	XtPopdown(shell);
	break;
    case NoticeCancel:
	SetMessage(nfsw, TXT_DeleteCanceled, Base);
	XtPopdown(shell);
	break;
    case NoticeHelp:
	if (nfsw-> viewMode == ViewRemote)
	    PostGizmoHelp(nfsw-> baseWindow-> shell,
			  &DeleteRemoteNoticeHelp);
	else
            PostGizmoHelp(nfsw-> baseWindow-> shell,
                          &DeleteLocalNoticeHelp);
	break;
    default:
	SetMessage(nfsw, "", Base);
	DEBUG0("default in DeleteCB taken!!!\n");
    }
}				/* end of DeleteCB */

