#pragma ident	"@(#)m1.2libs:Xm/Messages.c	1.3"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */


/* Define _XmConst before including MessagesI.h, so that the
 * declarations will be in agreement with the definitions.
 */
#ifndef _XmConst
#if (defined(__STDC__) && __STDC__)  ||  !defined( NO_CONST )
#define _XmConst const
#else
#define _XmConst
#endif /* __STDC__ */
#endif /* _XmConst */

#include <Xm/XmP.h>
#include "MessagesI.h"


/**************** BaseClass.c ****************/

externaldef(messages) _XmConst char _XmMsgBaseClass_0000[] =
   "no context found for extension";

externaldef(messages) _XmConst char _XmMsgBaseClass_0001[] =
   "_XmPopWidgetExtData: no extension found with XFindContext";

externaldef(messages) _XmConst char _XmMsgBaseClass_0002[] =
   "XmFreeWidgetExtData is an unsupported routine";


/**************** BulletinB.c ****************/

externaldef(messages) _XmConst char _XmMsgBulletinB_0001[] =
   "Incorrect dialog style.";


/**************** CascadeB.c / CascadeBG.c ****************/

externaldef(messages) _XmConst char _XmMsgCascadeB_0000[] =
   "XmCascadeButton[Gadget] must have xmRowColumnWidgetClass parent with\n\
rowColumnType XmMENU_PULLDOWN, XmMENU_POPUP, XmMENU_BAR or XmMENU_OPTION.";

externaldef(messages) _XmConst char _XmMsgCascadeB_0001[] =
   "Only XmMENU_PULLDOWN XmRowColumnWidgets can be submenus.";

externaldef(messages) _XmConst char _XmMsgCascadeB_0002[] =
   "MapDelay must be >= 0.";

externaldef(messages) _XmConst char _XmMsgCascadeB_0003[] =
   "XtGrabPointer failed";


/**************** Command.c ****************/

externaldef(messages) _XmConst char _XmMsgCommand_0000[] =
   "Dialog type must be XmDIALOG_COMMAND.";

externaldef(messages) _XmConst char _XmMsgCommand_0001[] =
   "Invalid child type, Command widget does not have this child.";

externaldef(messages) _XmConst char _XmMsgCommand_0002[] =
   "Invalid XmString, check for invalid charset.";

externaldef(messages) _XmConst char _XmMsgCommand_0003[] =
   "NULL or empty string passed in to CommandAppendValue.";

externaldef(messages) _XmConst char _XmMsgCommand_0004[] =
   "mustMatch is always False for a Command widget.";

externaldef(messages) _XmConst char _XmMsgCommand_0005[] =
   "historyMaxItems must be a positive integer greater than zero.";


/**************** CutPaste.c ****************/

externaldef(messages) _XmConst char _XmMsgCutPaste_0000[] =
   "Must call XmClipboardStartCopy() before XmClipboardCopy()";

externaldef(messages) _XmConst char _XmMsgCutPaste_0001[] =
   "Must call XmClipboardStartCopy() before XmClipboardEndCopy()";

externaldef(messages) _XmConst char _XmMsgCutPaste_0002[] =
   "Too many formats in XmClipboardCopy()";

externaldef(messages) _XmConst char _XmMsgCutPaste_0003[] =
   "ClipboardBadDataType";

externaldef(messages) _XmConst char _XmMsgCutPaste_0004[] =
   "bad data type";

externaldef(messages) _XmConst char _XmMsgCutPaste_0005[] =
   "ClipboardCorrupt";

externaldef(messages) _XmConst char _XmMsgCutPaste_0006[] =
   "internal error - corrupt data structure";

externaldef(messages) _XmConst char _XmMsgCutPaste_0007[] =
   "ClipboardBadFormat";

externaldef(messages) _XmConst char _XmMsgCutPaste_0008[] =
   "Error - registered format length must be 8, 16, or 32";

externaldef(messages) _XmConst char _XmMsgCutPaste_0009[] =
   "Error - registered format name must be non-null";


/**************** DialogS.c ****************/

externaldef(messages) _XmConst char _XmMsgDialogS_0000[] =
   "DialogShell widget only supports one rectObj child";


/**************** DragBS.c ****************/

externaldef(messages) _XmConst char _XmMsgDragBS_0000[] =
   "_MOTIF_DRAG_WINDOW has been destroyed";

externaldef(messages) _XmConst char _XmMsgDragBS_0001[] =
   "we aren't at the same version level";

externaldef(messages) _XmConst char _XmMsgDragBS_0002[] =
   "unable to open display";

externaldef(messages) _XmConst char _XmMsgDragBS_0003[] =
   "empty atoms table";

externaldef(messages) _XmConst char _XmMsgDragBS_0004[] =
   "empty target table";

externaldef(messages) _XmConst char _XmMsgDragBS_0005[] =
   "inconsistent targets table property";

externaldef(messages) _XmConst char _XmMsgDragBS_0006[] =
   "invalid target table index";


/**************** DragICC.c ****************/

externaldef(messages) _XmConst char _XmMsgDragICC_0000[] =
   "unknown dnd message type";

externaldef(messages) _XmConst char _XmMsgDragICC_0001[] =
   "we aren't at the same version level";


/**************** DragIcon.c ****************/

externaldef(messages) _XmConst char _XmMsgDragIcon_0000[] =
   "no geometry specified for dragIcon pixmap";

externaldef(messages) _XmConst char _XmMsgDragIcon_0001[] =
   "dragIcon with no pixmap created";

externaldef(messages) _XmConst char _XmMsgDragIcon_0002[] =
   "String to Bitmap converter needs Screen argument";


/**************** DragOverS.c ****************/

externaldef(messages) _XmConst char _XmMsgDragOverS_0000[] =
   "depth mismatch";

externaldef(messages) _XmConst char _XmMsgDragOverS_0001[] =
   "unknown icon attachment";

externaldef(messages) _XmConst char _XmMsgDragOverS_0002[] =
   "unknown drag state";

externaldef(messages) _XmConst char _XmMsgDragOverS_0003[] =
   "unknown blendModel";


/**************** DragUnder.c ****************/

externaldef(messages) _XmConst char _XmMsgDragUnder_0000[] =
   "unable to get dropSite window geometry";

externaldef(messages) _XmConst char _XmMsgDragUnder_0001[] =
   "invalid animationPixmapDepth";


/**************** Form.c ****************/

externaldef(messages) _XmConst char _XmMsgForm_0000[] =
   "Fraction base cannot be zero.";

externaldef(messages) _XmConst char _XmMsgForm_0002[] =
   "Circular dependency in Form children.";

externaldef(messages) _XmConst char _XmMsgForm_0003[] =
   "Bailed out of edge synchronization after 10,000 iterations.\n\
Check for contradictory constraints on the children of this form.";

externaldef(messages) _XmConst char _XmMsgForm_0004[] =
   "Attachment widget must have same parent as widget.";


/**************** GetSecRes.c ****************/

externaldef(messages) _XmConst char _XmMsgGetSecRes_0000[] =
   "getLabelSecResData: Not enough memory \n";


/**************** Label.c / LabelG.c ****************/

externaldef(messages) _XmConst char _XmMsgLabel_0003[] =
   "Invalid XmNlabelString - must be a compound string";

externaldef(messages) _XmConst char _XmMsgLabel_0004[] =
   "Invalid XmNacceleratorText - must be a compound string";


/**************** List.c ****************/

externaldef(messages) _XmConst char _XmMsgList_0000[] =
   "When changed, XmNvisibleItemCount must be at least 1.";

externaldef(messages) _XmConst char _XmMsgList_0005[] =
   "Cannot change XmNlistSizePolicy after initialization.";

externaldef(messages) _XmConst char _XmMsgList_0006[] =
   "When changed, XmNitemCount must be non-negative.";

externaldef(messages) _XmConst char _XmMsgList_0007[] =
   "Invalid item(s) to delete.";

externaldef(messages) _XmConst char _XmMsgList_0008[] =
   "XmNlistSpacing must be non-negative.";

externaldef(messages) _XmConst char _XmMsgList_0009[] =
   "Cannot set XmNitems to NULL when XmNitemCount is positive.";

externaldef(messages) _XmConst char _XmMsgList_0010[] =
   "When changed, XmNselectedItemCount must be non-negative.";

externaldef(messages) _XmConst char _XmMsgList_0011[] =
   "Cannot set XmNselectedItemCount to NULL when XmNselectedItemCount \
is positive.";

externaldef(messages) _XmConst char _XmMsgList_0012[] =
   "XmNtopItemPosition must be non-negative.";

externaldef(messages) _XmConst char _XmMsgList_0013[] =
   "XmNitems and XmNitemCount mismatch!";


/**************** MainW.c ****************/

externaldef(messages) _XmConst char _XmMsgMainW_0000[] =
   "The Menu Bar cannot be changed to NULL.";

externaldef(messages) _XmConst char _XmMsgMainW_0001[] =
   "The Command Window cannot be changed to NULL.";


/**************** Manager.c ****************/

externaldef(messages) _XmConst char _XmMsgManager_0000[] =
   "widget class %s has invalid CompositeClassExtension record";


/**************** MenuShell.c ****************/

externaldef(messages) _XmConst char _XmMsgMenuShell_0000[] =
   "MenuShell widgets must have a xmRowColumnWidgetClass child.";

externaldef(messages) _XmConst char _XmMsgMenuShell_0001[] =
   "Attempting to manage an incomplete menu.";


/**************** MessageB.c ****************/

externaldef(messages) _XmConst char _XmMsgMessageB_0003[] =
   "Invalid Child Type.";

externaldef(messages) _XmConst char _XmMsgMessageB_0004[] =
   "PushButton Id cannot be changed directly.";


/**************** NavigMap.c ****************/

externaldef(messages) _XmConst char _XmMsgNavigMap_0000[] =
   "_XmNavigate called with invalid direction";


/**************** PanedW.c ****************/

externaldef(messages) _XmConst char _XmMsgPanedW_0000[] =
   "Invalid minimum value, must be > 0.";

externaldef(messages) _XmConst char _XmMsgPanedW_0001[] =
   "Invalid maximum value, must be > 0.";

externaldef(messages) _XmConst char _XmMsgPanedW_0002[] =
   "Invalid minimum/maximum value, minimum must be < maximum.";

externaldef(messages) _XmConst char _XmMsgPanedW_0003[] =
   "Constraints do not allow appropriate sizing.";

externaldef(messages) _XmConst char _XmMsgPanedW_0004[] =
   "Too few parameters.";

externaldef(messages) _XmConst char _XmMsgPanedW_0005[] =
   "Invalid 1st parameter.";


/**************** Protocols.c ****************/

externaldef(messages) _XmConst char _XmMsgProtocols_0000[] =
   "must be a vendor shell";

externaldef(messages) _XmConst char _XmMsgProtocols_0001[] =
   "protocol mgr already exists";

externaldef(messages) _XmConst char _XmMsgProtocols_0002[] =
   "more protocols than I can handle";


/**************** Region.c ****************/

externaldef(messages) _XmConst char _XmMsgRegion_0000[] =
   "memory error";


/**************** ResConvert.c ****************/

externaldef(messages) _XmConst char _XmMsgResConvert_0000[] =
   "FetchUnitType: bad widget class";
externaldef(messages) _XmConst char _XmMsgResConvert_0001[] =
   "Improperly defined default list! exiting...";

/**************** RowColumn.c ****************/

externaldef(messages) _XmConst char _XmMsgRowColumn_0000[] =
   "Attempt to set width to zero ignored";

externaldef(messages) _XmConst char _XmMsgRowColumn_0001[] =
   "Attempt to set height to zero ignored";

externaldef(messages) _XmConst char _XmMsgRowColumn_0002[] =
   "XmNhelpWidget not used by PopUps: forced to NULL";

externaldef(messages) _XmConst char _XmMsgRowColumn_0003[] =
   "XmNhelpWidget not used by Pulldowns: forced to NULL";

externaldef(messages) _XmConst char _XmMsgRowColumn_0004[] =
   "XmNhelpWidget not used by Option menus: forced to NULL";

externaldef(messages) _XmConst char _XmMsgRowColumn_0005[] =
   "XmNhelpWidget not used by Work Areas: forced to NULL";

externaldef(messages) _XmConst char _XmMsgRowColumn_0007[] =
   "Widget hierarchy not appropriate for this XmNrowColumnType:\n\
defaulting to WorkArea";

externaldef(messages) _XmConst char _XmMsgRowColumn_0008[] =
   "Attempt to change XmNrowColumnType after initialization: ignored";

externaldef(messages) _XmConst char _XmMsgRowColumn_0015[] =
   "Attempt to set XmNisHomogenous to FALSE for a RowColumn widget of type \
XmMENU_BAR ignored";

externaldef(messages) _XmConst char _XmMsgRowColumn_0016[] =
   "Attempt to change XmNentryClass for a RowColumn widget of type \
XmMENU_BAR ignored";

externaldef(messages) _XmConst char _XmMsgRowColumn_0017[] =
   "Attempt to change XmNwhichButton via XtSetValues for a RowColumn widget \
of type XmMENU_PULLDOWN ignored";

externaldef(messages) _XmConst char _XmMsgRowColumn_0018[] =
   "Attempt to change XmNmenuPost via XtSetValues for a RowColumn widget \
of type XmMENU_PULLDOWN ignored";

externaldef(messages) _XmConst char _XmMsgRowColumn_0019[] =
   "Attempt to set XmNmenuPost to an illegal value ignored";

externaldef(messages) _XmConst char _XmMsgRowColumn_0020[] =
   "Attempt to change XmNshadowThickness for a RowColumn widget not of type \
XmMENU_PULLDOWN or XmMENU_POPUP ignored";

externaldef(messages) _XmConst char _XmMsgRowColumn_0022[] =
   "Attempt to add wrong type child to a menu (i.e. RowColumn) widget";

externaldef(messages) _XmConst char _XmMsgRowColumn_0023[] =
   "Attempt to add wrong type child to a homogeneous RowColumn widget";

externaldef(messages) _XmConst char _XmMsgRowColText_0024[] =
   "XtGrabKeyboard failed";

externaldef(messages) _XmConst char _XmMsgRowColumn_0025[] =
   "Attempt to change XmNisHomogeneous for a RowColumn widget of type \
XmMENU_OPTION ignored";

externaldef(messages) _XmConst char _XmMsgRowColumn_0026[] =
   "Tear off enabled on a shared menupane: allowed but not recommended";

externaldef(messages) _XmConst char _XmMsgRowColumn_0027[] =
   "Illegal mnemonic character;  Could not convert X KEYSYM to a keycode";


/**************** Scale.c ****************/

externaldef(messages) _XmConst char _XmMsgScale_0000[] =
   "The scale minumum value is greater than or equal to the scale maximum \
value.";

externaldef(messages) _XmConst char _XmMsgScale_0001[] =
   "The specified scale value is less than the minimum scale value.";

externaldef(messages) _XmConst char _XmMsgScale_0002[] =
   "The specified scale value is greater than the maximum scale value.";

externaldef(messages) _XmConst char _XmMsgScaleScrBar_0004[] =
   "Incorrect processing direction.";

externaldef(messages) _XmConst char _XmMsgScale_0005[] =
   "Invalid highlight thickness.";

externaldef(messages) _XmConst char _XmMsgScale_0006[] =
   "Invalid scaleMultiple; greater than (max - min)";

externaldef(messages) _XmConst char _XmMsgScale_0007[] =
   "Invalid scaleMultiple; less than zero";

externaldef(messages) _XmConst char _XmMsgScale_0008[] =
   "(Maximum - minimum) cannot be greater than INT_MAX / 2;\n\
minimum has been set to zero, maximum may have been set to (INT_MAX/2).";


/**************** Screen.c ****************/

externaldef(messages) _XmConst char _XmMsgScreen_0000[] =
   "icon screen mismatch";
externaldef(messages) _XmConst char _XmMsgScreen_0001[] =
  "Can't get XmScreen because I can't find an XmDisplay";


/**************** ScrollBar.c ****************/

externaldef(messages) _XmConst char _XmMsgScrollBar_0000[] =
   "The scrollbar minimum value is greater than or equal to\n\
the scrollbar maximum value.";

externaldef(messages) _XmConst char _XmMsgScrollBar_0001[] =
   "The specified slider size is less than 1";

externaldef(messages) _XmConst char _XmMsgScrollBar_0002[] =
   "The specified scrollbar value is less than the minimum\n\
scroll bar value.";

externaldef(messages) _XmConst char _XmMsgScrollBar_0003[] =
   "The specified scrollbar value is greater than the maximum\n\
scrollbar value minus the scrollbar slider size.";

externaldef(messages) _XmConst char _XmMsgScrollBar_0004[] =
   "The scrollbar increment is less than 1.";

externaldef(messages) _XmConst char _XmMsgScrollBar_0005[] =
   "The scrollbar page increment is less than 1.";

externaldef(messages) _XmConst char _XmMsgScrollBar_0006[] =
   "The scrollbar initial delay is less than 1.";

externaldef(messages) _XmConst char _XmMsgScrollBar_0007[] =
   "The scrollbar repeat delay is less than 1.";

externaldef(messages) _XmConst char _XmMsgScrollBar_0008[] =
   "Specified slider size is greater than the scrollbar maximum\n\
value minus the scrollbar minimum value.";


/**************** ScrolledW.c ****************/

externaldef(messages) _XmConst char _XmMsgScrolledW_0004[] =
   "Cannot change scrolling policy after initialization.";

externaldef(messages) _XmConst char _XmMsgScrolledW_0005[] =
   "Cannot change visual policy after initialization.";

externaldef(messages) _XmConst char _XmMsgScrolledW_0006[] =
   "Cannot set AS_NEEDED scrollbar policy with a\nvisual policy of VARIABLE.";

externaldef(messages) _XmConst char _XmMsgScrolledW_0007[] =
   "Cannot change scrollbar widget in AUTOMATIC mode.";

externaldef(messages) _XmConst char _XmMsgScrolledW_0008[] =
   "Cannot change clip window";

externaldef(messages) _XmConst char _XmMsgScrolledW_0009[] =
   "Cannot set visual policy of CONSTANT in APPLICATION_DEFINED mode.";

externaldef(messages) _XmConst char _XmMsgScrollVis_0000[] =
   "Wrong parameters passed to the function.";

/**************** SelectioB.c ****************/

externaldef(messages) _XmConst char _XmMsgSelectioB_0001[] =
   "Dialog type cannot be modified.";

externaldef(messages) _XmConst char _XmMsgSelectioB_0002[] =
   "Invalid child type.";


/**************** Text.c ****************/

externaldef(messages) _XmConst char _XmMsgText_0000[] =
   "Invalid source, source ignored.";

externaldef(messages) _XmConst char _XmMsgText_0002[] =
   "Text widget is editable, Traversal_on must be true.";


/**************** TextF.c ****************/

externaldef(messages) _XmConst char _XmMsgTextF_0000[] =
   "Invalid cursor position, must be >= 0.";

externaldef(messages) _XmConst char _XmMsgTextF_0001[] =
   "Invalid columns, must be > 0.";

externaldef(messages) _XmConst char _XmMsgTextF_0002[] =
   "XmFontListInitFontContext Failed.";

externaldef(messages) _XmConst char _XmMsgTextF_0003[] =
   "XmFontListGetNextFont Failed.";

externaldef(messages) _XmConst char _XmMsgTextF_0004[] =
   "Character '%c', not supported in font.  Discarded.";

externaldef(messages) _XmConst char _XmMsgTextF_0005[] =
   "Traversal_on must always be true.";

externaldef(messages) _XmConst char _XmMsgTextF_0006[] =
   "Invalid columns, must be >= 0.";

externaldef(messages) _XmConst char _XmMsgTextFWcs_0000[] =
   "Character '%s', not supported in font.  Discarded.";

externaldef(messages) _XmConst char _XmMsgTextFWcs_0001[] =
   "Cannot use multibyte locale without a fontset.  Value discarded.";


/**************** TextIn.c ****************/

externaldef(messages) _XmConst char _XmMsgTextIn_0000[] =
   "Can't find position in MovePreviousLine().";


/**************** TextOut.c ****************/

externaldef(messages) _XmConst char _XmMsgTextOut_0000[] =
   "Invalid rows, must be > 0";


/**************** Vendor.c ****************/

externaldef(messages) _XmConst char _XmMsgVendor_0000[] =
   "invalid value for delete response";


/**************** VendorE.c ****************/

externaldef(messages) _XmConst char _XmMsgVendorE_0000[] =
   "String to noop conversion needs no extra arguments";

externaldef(messages) _XmConst char _XmMsgVendorE_0005[] =
   "FetchUnitType called without a widget to reference";


/**************** Visual.c ****************/

externaldef(messages) _XmConst char _XmMsgVisual_0000[] =
   "Invalid color requested from _XmAccessColorData";

externaldef(messages) _XmConst char _XmMsgVisual_0001[] =
   "Cannot allocate colormap entry for default background";

externaldef(messages) _XmConst char _XmMsgVisual_0002[] =
   "Cannot parse default background color spec";


/**************** XmIm.c *******************/

externaldef(messages) _XmConst char _XmMsgXmIm_0000[] =
    "Cannot open input method - using XLookupString";

/*************** GeoUtils.c *******************/

externaldef(messages) _XmConst char _XmMsgGeoUtils_0000[] = 
    "failure of geometry request to \"almost\" reply";

/*************** DropSMgrI.c *******************/

externaldef(messages) _XmConst char _XmMsgDropSMgrI_0001[] =
    "Can't register a drop site which is a descendent of a simple drop site";
externaldef(messages) _XmConst char _XmMsgDropSMgrI_0002[] =
    "Can't create a discontiguous child list for a composite drop site.";
externaldef(messages) _XmConst char _XmMsgDropSMgrI_0003[] =
    "%s is not a drop site child of %s";

/*************** DragC.c *******************/

externaldef(messages) _XmConst char _XmMsgDragC_0001[] =
    "GenerateCallback does not expect XmCR_DROP_SITE_ENTER as a reason\n";
externaldef(messages) _XmConst char _XmMsgDragC_0002[] =
    "Invalid selection in DropConvertCallback\n";
externaldef(messages) _XmConst char _XmMsgDragC_0003[] =
    "We lost the drop selection\n";
externaldef(messages) _XmConst char _XmMsgDragC_0004[] =
    "XGrabPointer failed\n";
externaldef(messages) _XmConst char _XmMsgDragC_0005[] =
    "ExternalNotifyHandler: the callback reason is not acceptable\n";
externaldef(messages) _XmConst char _XmMsgDragC_0006[] =
    "XmDragStart must be called as a result of a button press or motion event\n";

/*************** DropSMgr.c *******************/

externaldef(messages) _XmConst char _XmMsgDropSMgr_0001[] =
    "Can't create drop sites which are children of a simple drop site.";
externaldef(messages) _XmConst char _XmMsgDropSMgr_0002[] =
    "Receiving Motion Events without an active drag context";
externaldef(messages) _XmConst char _XmMsgDropSMgr_0003[] =
    "Receiving operation changed without an active drag context.";
externaldef(messages) _XmConst char _XmMsgDropSMgr_0004[] =
    "Creating an active drop site with no drop proc.";
externaldef(messages) _XmConst char _XmMsgDropSMgr_0005[] =
    "Can\'t set rectangles or num rectangles of composite dropsites.";
externaldef(messages) _XmConst char _XmMsgDropSMgr_0006[] =
    "Registering a widget as a drop site out of sequence.\n\
Ancestors must be registered before any of their \n\
descendants are registered.";
externaldef(messages) _XmConst char _XmMsgDropSMgr_0007[] =
    "Can't register widget as a drop site more than once.";
externaldef(messages) _XmConst char _XmMsgDropSMgr_0008[] =
    "DropSite type may only be set at creation time.";
externaldef(messages) _XmConst char _XmMsgDropSMgr_0009[] =
    "Can't change rectangles of non-simple dropsite.";

/*************** Display.c ***************/

externaldef(messages) _XmConst char _XmMsgDisplay_0001[] =
    "Creating multiple XmDisplays for the same X display.  Only the\n\
first XmDisplay created for a particular X display can be referenced\n\
by calls to XmGetXmDisplay";
externaldef(messages) _XmConst char _XmMsgDisplay_0002[] =
    "Received TOP_LEVEL_LEAVE with no active DragContext";
externaldef(messages) _XmConst char _XmMsgDisplay_0003[] =
    "Cannot set XmDisplay class to a non-subclass of XmDisplay";

/*************** RepType.c ***************/

externaldef(messages) _XmConst char _XmMsgRepType_0001[] =
    "illegal representation type id";
externaldef(messages) _XmConst char _XmMsgRepType_0002[] =
    "illegal value (%d) for rep type XmR%s";
