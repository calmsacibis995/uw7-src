#pragma ident	"@(#)m1.2libs:Mrm/Mrminit.c	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif

/*
*  (c) Copyright 1989, 1990, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */

/*
 *++
 *  FACILITY:
 *
 *      UIL Resource Manager (URM):
 *
 *  ABSTRACT:
 *
 *	This contains only the top-level routine MrmIntialize. It can be 
 *	modified by vendors as needed to add or remove widgets being \
 *	initialized for URM facilities. This routine is normally accessible to
 *	and used by an application at runtime to access URM facilities.
 *
 *--
 */


/*
 *
 *  INCLUDE FILES
 *
 */


#include <Mrm/MrmAppl.h>
#include <Mrm/Mrm.h>

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>
#include <X11/ShellP.h>
#include <X11/VendorP.h>
#include <X11/RectObjP.h>

#ifdef DXM_V11
#include <DXm/DXmHelpB.h>
#include <DXm/DXmHelpBp.h>
#endif

#include <Xm/XmP.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/LabelGP.h>
#include <Xm/LabelP.h>
#include <Xm/BulletinB.h>
#include <Xm/BulletinBP.h>
#include <Xm/RowColumn.h>
#include <Xm/RowColumnP.h>
#include <Xm/ArrowB.h>
#include <Xm/ArrowBG.h>
#include <Xm/ArrowBGP.h>
#include <Xm/ArrowBP.h>
#include <Xm/AtomMgr.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/CascadeBGP.h>
#include <Xm/CascadeBP.h>
#include <Xm/SelectioBP.h>
#include <Xm/SelectioB.h>
#include <Xm/Command.h>
#include <Xm/CommandP.h>
#include <Xm/CutPaste.h>
#include <Xm/CutPasteP.h>
#include <Xm/DialogS.h>
#include <Xm/DialogSP.h>
#include <Xm/DrawingA.h>
#include <Xm/DrawingAP.h>
#include <Xm/DrawnB.h>
#include <Xm/DrawnBP.h>
#include <Xm/FileSB.h>
#include <Xm/FileSBP.h>
#include <Xm/Form.h>
#include <Xm/FormP.h>
#include <Xm/Frame.h>
#include <Xm/FrameP.h>
#include <Xm/List.h>
#include <Xm/ListP.h>
#include <Xm/MainW.h>
#include <Xm/MainWP.h>
#include <Xm/MenuShell.h>
#include <Xm/MenuShellP.h>
#include <Xm/MessageB.h>
#include <Xm/MessageBP.h>
#include <Xm/PanedW.h>
#include <Xm/PanedWP.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/PushBGP.h>
#include <Xm/PushBP.h>
#include <Xm/SashP.h>
#include <Xm/Scale.h>
#include <Xm/ScaleP.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrollBarP.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrolledWP.h>
#include <Xm/SeparatoG.h>
#include <Xm/SeparatoGP.h>
#include <Xm/Separator.h>
#include <Xm/SeparatorP.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/TextP.h>
#include <Xm/TextInP.h>
#include <Xm/TextOutP.h>
#include <Xm/TextStrSoP.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/ToggleBGP.h>
#include <Xm/ToggleBP.h>

/*
 *
 *  TABLE OF CONTENTS
 *
 *	MrmInitialize			Initialize URM internals before use
 *
 */


/*
 *
 *  DEFINE and MACRO DEFINITIONS
 *
 */



/*
 *
 *  EXTERNAL VARIABLE DECLARATIONS
 *
 */

/*
 *
 *  GLOBAL VARIABLE DECLARATIONS
 *
 */


/*
 *
 *  OWN VARIABLE DECLARATIONS
 *
 */

/*
 * The following flag is set to indicate successful URM initialization
 */
static	Boolean	urm__initialize_complete = FALSE;


void MrmInitialize ()

/*
 *++
 *  PROCEDURE DESCRIPTION:
 *
 *	MrmInitialize must be called in order to prepare an application to
 *	use URM widget fetching facilities. It initializes the internal data
 *	structures (creating the mapping from class codes to the creation
 *	routine for each builtin widget class) which URM needs in order to 
 *	successfully perform type conversion on arguments, and successfully 
 *	access widget creation facilities. MrmInitialize must be called before
 *	any widgets are	created, whether by URM's fetch mechanisms or directly
 *	by the application. It may be called before or after XtInitialize, and
 *	multiple calls after the first one are benign (no-ops).
 *
 *  FORMAL PARAMETERS:
 *
 *  IMPLICIT INPUTS:
 *
 *  IMPLICIT OUTPUTS:
 *
 *  FUNCTION VALUE:
 *
 *  SIDE EFFECTS:
 *
 *--
 */

{

/*
 *  External Functions
 */

/*
 *  Local variables
 */

/*
 * Initialize only once
 */
if ( urm__initialize_complete ) return ;

/*
 * Initialize the class descriptors for all the known widgets.
 */
#ifdef DXM_V11
MrmRegisterClass
   (0, NULL, "DXmCreateHelpWidget", DXmCreateHelp,
    (WidgetClass)&dxmhelpwidgetclassrec);
#endif

MrmRegisterClass
   (0, NULL, "XmCreateArrowButton", XmCreateArrowButton,
    (WidgetClass)&xmArrowButtonClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateArrowButtonGadget", XmCreateArrowButtonGadget,
    (WidgetClass)&xmArrowButtonGadgetClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateBulletinBoard", XmCreateBulletinBoard,
     (WidgetClass)&xmBulletinBoardClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateBulletinBoardDialog", XmCreateBulletinBoardDialog,
     (WidgetClass)&xmBulletinBoardClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateCascadeButton", XmCreateCascadeButton,
     (WidgetClass)&xmCascadeButtonClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateCascadeButtonGadget", XmCreateCascadeButtonGadget,
     (WidgetClass)&xmCascadeButtonGadgetClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateCommand", XmCreateCommand,
     (WidgetClass)&xmCommandClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateDialogShell", XmCreateDialogShell,
     (WidgetClass)&xmDialogShellClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateDrawingArea", XmCreateDrawingArea,
     (WidgetClass)&xmDrawingAreaClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateDrawnButton", XmCreateDrawnButton,
     (WidgetClass)&xmDrawnButtonClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateFileSelectionBox", XmCreateFileSelectionBox,
     (WidgetClass)&xmFileSelectionBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateFileSelectionDialog", XmCreateFileSelectionDialog,
     (WidgetClass)&xmFileSelectionBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateForm", XmCreateForm,
     (WidgetClass)&xmFormClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateFormDialog", XmCreateFormDialog,
     (WidgetClass)&xmFormClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateFrame", XmCreateFrame,
     (WidgetClass)&xmFrameClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateLabel", XmCreateLabel,
     (WidgetClass)&xmLabelClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateLabelGadget", XmCreateLabelGadget,
     (WidgetClass)&xmLabelGadgetClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateList", XmCreateList,
     (WidgetClass)&xmListClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateScrolledList", XmCreateScrolledList,
     (WidgetClass)&xmListClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateMainWindow", XmCreateMainWindow,
     (WidgetClass)&xmMainWindowClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateMenuShell", XmCreateMenuShell,
     (WidgetClass)&xmMenuShellClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateMessageBox", XmCreateMessageBox,
     (WidgetClass)&xmMessageBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateMessageDialog", XmCreateMessageDialog,
     (WidgetClass)&xmMessageBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateErrorDialog", XmCreateErrorDialog,
     (WidgetClass)&xmMessageBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateInformationDialog", XmCreateInformationDialog,
     (WidgetClass)&xmMessageBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateQuestionDialog", XmCreateQuestionDialog,
     (WidgetClass)&xmMessageBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateTemplateDialog", XmCreateTemplateDialog,
     (WidgetClass)&xmMessageBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateWarningDialog", XmCreateWarningDialog,
     (WidgetClass)&xmMessageBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateWorkingDialog", XmCreateWorkingDialog,
     (WidgetClass)&xmMessageBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreatePushButton", XmCreatePushButton,
     (WidgetClass)&xmPushButtonClassRec);

MrmRegisterClass
    (0, NULL, "XmCreatePushButtonGadget", XmCreatePushButtonGadget,
     (WidgetClass)&xmPushButtonGadgetClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateRowColumn", XmCreateRowColumn,
     (WidgetClass)&xmRowColumnClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateWorkArea", XmCreateWorkArea,
     (WidgetClass)&xmRowColumnClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateRadioBox", XmCreateRadioBox,
     (WidgetClass)&xmRowColumnClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateMenuBar", XmCreateMenuBar,
     (WidgetClass)&xmRowColumnClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateOptionMenu", XmCreateOptionMenu,
     (WidgetClass)&xmRowColumnClassRec);

MrmRegisterClass
    (0, NULL, "XmCreatePopupMenu", XmCreatePopupMenu,
     (WidgetClass)&xmRowColumnClassRec);

MrmRegisterClass
    (0, NULL, "XmCreatePulldownMenu", XmCreatePulldownMenu,
     (WidgetClass)&xmRowColumnClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateScale", XmCreateScale,
     (WidgetClass)&xmScaleClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateScrolledWindow", XmCreateScrolledWindow,
     (WidgetClass)&xmScrolledWindowClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateScrollBar", XmCreateScrollBar,
     (WidgetClass)&xmScrollBarClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateSelectionBox", XmCreateSelectionBox,
     (WidgetClass)&xmSelectionBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateSelectionDialog", XmCreateSelectionDialog,
     (WidgetClass)&xmSelectionBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreatePromptDialog", XmCreatePromptDialog,
     (WidgetClass)&xmSelectionBoxClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateSeparator", XmCreateSeparator,
     (WidgetClass)&xmSeparatorClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateSeparatorGadget", XmCreateSeparatorGadget,
     (WidgetClass)&xmSeparatorGadgetClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateText", XmCreateText,
     (WidgetClass)&xmTextClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateTextField", XmCreateTextField,
     (WidgetClass)&xmTextClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateScrolledText", XmCreateScrolledText,
     (WidgetClass)&xmTextClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateToggleButton", XmCreateToggleButton,
     (WidgetClass)&xmToggleButtonClassRec);

MrmRegisterClass
    (0, NULL, "XmCreateToggleButtonGadget", XmCreateToggleButtonGadget,
     (WidgetClass)&xmToggleButtonGadgetClassRec);

MrmRegisterClass
    (0, NULL, "XmCreatePanedWindow", XmCreatePanedWindow,
     (WidgetClass)&xmPanedWindowClassRec);

/*
 * Initialization complete
 */
urm__initialize_complete = TRUE ;
return ;

}
