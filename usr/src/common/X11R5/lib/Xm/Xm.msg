#pragma ident	"@(#)m1.2libs:Xm/Xm.msg	1.2"
$
$
$
$
$
$   (C) COPYRIGHT International Business Machines Corp. 1992
$   All Rights Reserved
$   Licensed Materials - Property of IBM
$   US Government Users Restricted Rights - Use, duplication or
$   disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
$
$ Motif Release 1.2

$ This message catalog contains messages issued by Motif toolkit library.
$ Consult X-Window systems, OSF/MOTIF Programmer's Guide and OSF/MOTIF
$ Programmer's Reference Manual for technical terms if you have any
$ doubts about their meanings.
$ Do not translate variables such as %s, %d %s ... etc.
$ Do not translate Motif resource name, such as XmNlabelType.
$ Special terms with capital letter(s) should not be translated.
$ "False' and "True" are keywords. Do not translate.

$quote "

$set MS_AButton

MSG_AB_1 "2054-201 The arrow direction is not correct."


$set MS_BBoard        

MSG_BB_1 "2054-202 Incorrect resize policy."
MSG_BB_2 "2054-203 Incorrect dialog style."
MSG_BB_3 "2054-204 Incorrect shadow type."
MSG_BB_4 "2054-205 Null font list (no vendor shell default)"


$set MS_CButton

MSG_CB_1 "2054-211 XmCascadeButton must have correct type of \n\
         XmRowColumnWidgetClass parent."
MSG_CB_2 "2054-212 Only XmMENU_PULLDOWN XmRowColumnWidgets can be submenus."
$ Do not translate submenus.
MSG_CB_3 "2054-213 MapDelay must be >= 0."
MSG_CB_4 "2054-214 XmCascadeButtonGadget must have xmRowColumnWidgetClass \n\
         parent with rowColumnType XmMENU_PULLDOWN, XmMENU_POPUP, \n\
         XmMENU_BAR or XmMENU_OPTION."
MSG_CB_5 "2054-xxx XtGrabPointer failed."
MSG_CB_6 "2054-xxx XtGrabKeyboard failed."


$set MS_Command       

MSG_C_1 "2054-221 The dialog type must be XmDIALOG_COMMAND."
MSG_C_2 "2054-222 Invalid child type.\n\
         The Command widget does not have this child."
$ Do not translate Command.
MSG_C_3 "2054-223 Invalid XmString, check for invalid charset."
MSG_C_4 "2054-224 NULL or empty string passed in to CommandAppendValue."
MSG_C_5 "2054-225 mustMatch is always False for a Command widget."
$ Do not translate mustMatch, False or Command.   
MSG_C_6 "2054-226 historyMaxItems must be a positive integer greater than zero."


$set MS_CPaste

MSG_CP_1 "2054-231 Must call XmClipboardStartCopy() before XmClipboardCopy()"
MSG_CP_2 "2054-232 Must call XmClipboardStartCopy() before XmClipboardEndCopy()"
MSG_CP_3 "2054-233 Too many formats in XmClipboardCopy()"


$set MS_DShell

MSG_DS_1 "2054-241 DialogShell widget only supports one rectObj child"
$ Do not transalate rectObj
MSG_DS_2 "2054-242 gadgets aren't allowed in shell"


$set MS_DArea

MSG_DA_1 "2054-251 Margin width or height cannot be negative."
MSG_DA_2 "2054-252 Incorrect resize policy."


$set MS_FOrm

MSG_FO_1 "2054-261 Fraction base cannot be zero."
MSG_FO_2 "2054-262 Incorrect form attachment type."
MSG_FO_3 "2054-263 Cannot set constraints for non-resizable widget."
MSG_FO_4 "2054-264 Attachment widget must not be null."
$ Do not translate Attachment
MSG_FO_5 "2054-265 Circular dependency in Form children."
$ Do not translate Form.
MSG_FO_6 "2054-266 Edge attached to a widget but no widget specified."
MSG_FO_7 "2054-267 Bailed out of edge synchronization after 10,000 iterations\n\
         Check for contradictory constraints on the children of this form."
MSG_FO_8 "2054-268 Attachment widget must be have same parent as widget."
$ do not translate Attachment


$set MS_Frame

MSG_F_2 "2054-272 Only one child should be inserted in a frame."
MSG_F_3 "2054-273 Invalid margin width."
MSG_F_4 "2054-274 Invalid margin height." 


$set MS_Gadget

MSG_G_1 "2054-281 Invalid highlight thickness."
MSG_G_2 "2054-282 The unit type is incorrect."
MSG_G_3 "2054-283 Invalid shadow thickness."
MSG_G_4 "2054-284 Cannot set pixmap resource to unspecified."



$set MS_Label

MSG_L_1 "2054-291 Invalid XmNlabeType " 
MSG_L_2 "2054-292 Invalid value in XmNalignment"
MSG_L_3 "2054-293 Invalid value in XmNstringDirection"
MSG_L_4 "2054-294 Invalid XmNlabelString - must be a compound string"
MSG_L_5 "2054-295 Invalid XmNacceleratorText - must be a compound string"


$set MS_LIst

MSG_LI_0 "2054-300 List must have at least one visiable item."
MSG_LI_1 "2054-301 Invalid Selection Policy."
MSG_LI_2 "2054-302 Invalid Size Policy."
MSG_LI_3 "2054-303 Invalid ScrollBar Display Policy."
MSG_LI_4 "2054-304 Invalid String Direction."
MSG_LI_5 "2054-305 Cannot change size policy after initialization."
MSG_LI_6 "2054-306 Must set item count to non-negative value."
MSG_LI_7 "2054-307 NULL font in SetValues ignored."
$ Do not translate SetValues
MSG_LI_8 "2054-308 Invalid item(s) to delete."
MSG_LI_9 "2054-309 No Horizontal Scrollbar to set."
MSG_LI_10 "2054-310 Invalid Margin setting."
MSG_LI_11 "2054-311 Invalid Spacing Value."
MSG_LI_12 "2054-312 Cannot set items to NULL with non-zero item count."
MSG_LI_13 "2054-313 Must set selected item count to non-negative value."
MSG_LI_14 "2054-314 Cannot set selected items to NULL with non-zero item count."
MSG_LI_15 "2054-315 Cannot set top position less than 1."
MSG_LI_16 "2054-316 XmNitems and XmNitemCount mismatch!"
MSG_LI_17 "2054-317 Cannot leave add mode in multiple selection."


$set MS_MWindow

MSG_MW_1 "2054-321 The Menu Bar cannot be changed to NULL."
MSG_MW_2 "2054-322 The Command Window cannot be changed to NULL."
MSG_MW_3 "2054-323 Negative margin value ignored."


$set MS_MShell

MSG_MS_1 "2054-331 MenuShell widgets must have a xmRowColumnWidgetClass child"
MSG_MS_2 "2054-332 Attempting to manage an incomplete menu."


$set MS_MBox

MSG_MB_1 "2054-341 Invalid Dialog Type."
MSG_MB_2 "2054-342 Invalid Default Button Type."
MSG_MB_3 "2054-343 Invalid Alignment Type."
MSG_MB_4 "2054-344 Invalid Child Type."
MSG_MB_5 "2054-345 PushButton Id cannot be changed directly."
MSG_MB_6 "2054-346 Use XmNdefaultButtonType to set MessageBox default button."


$set MS_PWindow

MSG_PW_4 "2054-354 Invalid minimum value, must be greater than zero."
MSG_PW_5 "2054-355 Invalid maximum value, must be greater than zero."
MSG_PW_6 "2054-356 Invalid minimum/maximum value, minimum value must be \n\
         smaller than the maximum value."
MSG_PW_7 "2054-357 Constraints do not allow appropriate sizing."
MSG_PW_8 "2054-358 Too few parameters."
MSG_PW_9 "2054-359 Invalid 1st parameter."


$set MS_PWidget

MSG_PWI_1 "2054-361 fontList is not defined"
$ Do not translate fontList.


$set MS_PRotocol

MSG_PR_1 "2054-371 Must be a vendor shell" 
MSG_PR_2 "2054-372 Protocol manager already exists"
MSG_PR_3 "2054-373 More protocols than I can handle"


$set MS_PButton

MSG_PB_1 "2054-381 Not enough memory" 


$set MS_RColumn
$ In this set, do not translate the word that starts with "XmN or Xm".

MSG_RC_1 "2054-401 Attempt to set width to zero.\n\
         Set to default value 16."
MSG_RC_2 "2054-402 Attempt to set width to zero.\n\
         The value is ignored."
MSG_RC_3 "2054-403 Attempt to set height to zero.\n\
         Set to default value 16."
MSG_RC_4 "2054-404 Attempt to set height to zero.\n\
         The value is ignored."
MSG_RC_5 "2054-405 XmNhelpWidget not used by PopUps.\n\
         It is set to NULL."
MSG_RC_6 "2054-406 XmNhelpWidget not used by Pulldowns.\n\
         It is set to NULL."
MSG_RC_7 "2054-407 XmNhelpWidget not used by Option menus.\n\
         It is set to NULL."
MSG_RC_8 "2054-408 XmNhelpWidget not used by Work Areas.\n\
         It is set to NULL."
MSG_RC_9 "2054-409 Unknown value of XmNrowColumnType.\n\
         It is set to WorkArea."
MSG_RC_10 "2054-410 Widget hierarchy not appropriate for this \n\
         XmNrowColumnType.  It is set to WorkArea."
MSG_RC_11 "2054-411 Attempt to change XmNrowColumnType after initialization.\n\
         The value is ignored."
MSG_RC_12 "2054-412 Unknown value of XmNorientation.\n\
         The default value is used."
MSG_RC_13 "2054-413 Attempt to set XmNorientation to unknown value.\n\
         The value is ignored."
MSG_RC_14 "2054-414 Unknown value of XmNpacking.\n\
         The default value is used."
MSG_RC_15 "2054-415 Attempt to set XmNpacking to unknown value.\n\
         The value is ignored."
MSG_RC_16 "2054-416 Unknown value of XmNentryAlignment.\n\
         The default value is used."
MSG_RC_17 "2054-417 Attempt to set XmNentryAlignment to unknown value.\n\
         The value is ignored."
MSG_RC_18 "2054-418 Attempt to set XmNisHomogenous to FALSE for \n\
         a RowColumn widget of type XmMENU_BAR.\n\
         The value is ignored."
MSG_RC_19 "2054-419 Attempt to change XmNentryClass for a RowColumn \n\
         widget of type XmMENU_BAR.  The value is ignored."
MSG_RC_20 "2054-420 Attempt to change XmNwhichButton via XtSetValues \n\
         for a RowColumn widget of type XmMENU_PULLDOWN.\n\
         The value is ignored."
MSG_RC_21 "2054-421 Attempt to change XmNmenuPost via XtSetValues for \n\
         a RowColumn widget of type XmMENU_PULLDOWN.\n\
         The value is ignored."
MSG_RC_22 "2054-422 Attempt to set XmNpostMenu to an illegal value.\n\
         The value is ignored."
MSG_RC_23 "2054-423 Attempt to change XmNshadowThickness for a RowColumn \n\
         widget not of type XmMENU_PULLDOWN or XmMENU_POPUP.\n\
         The value is ignored."
MSG_RC_24 "2054-024 Attempt to change XmNorientation for a RowColumn \n\
         widget of type XmMENU_OPTION.\n\
         The value is ignored."
MSG_RC_25 "2054-425 Attempt to add wrong type child to a menu \n\
         (i.e. RowColumn) widget"
MSG_RC_26 "2054-426 Attempt to add wrong type child to a homogeneous \n\
         RowColumn widget"
MSG_RC_27 "2054-427 Attempt to change XmNisHomogeneous for a RowColumn \n\
	 widget of type XmMENU_OPTION ignored"
MSG_RC_28 "2054-xxx Tear off enabled on a shared menupane is allowed \n\
	 but not recommended."
MSG_RC_29 "2054-xxx Illegal mnemonic character.  Could not convert X KEYSYM \n\
	 to a keycode."


$set MS_Scale

MSG_S_1 "2054-431 The scale minumum value is greater than or \n\
         equal to the scale maximum value."
MSG_S_2 "2054-432 The specified scale value is less than \n\
         the minimum scale value."
MSG_S_3 "2054-433 The specified scale value is greater than \n\
         the maximum scale value."
MSG_S_4 "2054-434 Incorrect orientation."
MSG_S_5 "2054-435 Incorrect processing direction."
MSG_S_6 "2054-436 Invalid highlight thickness."
MSG_S_7 "2054-437 Invalid scaleMultiple; greater than (max - min)"
MSG_S_8 "2054-438 Invalid scaleMultiple; less than zero"
MSG_S_9 "2054-xxx (Maximum - minimum) cannot be greater than \n\
	INT_MAX divided by 2.  Miminimum has been set to zero. \n\
	 Maximum may have been set to (INT_MAX/2)."



$set MS_SBar

MSG_SB_1 "2054-441 The scrollbar minimum value is greater than\n\
         or equal to the scrollbar maximum value."
MSG_SB_2 "2054-442 The specified slider size is less than 1"
MSG_SB_3 "2054-443 The specified scrollbar value is less than\n\
         the minimum scrollbar value."
MSG_SB_4 "2054-444 The specified scrollbar value is greater than\n\
         the maximum scrollbar value minus the scrollbar slider size."
MSG_SB_5 "2054-445 Incorrect orientation."
MSG_SB_6 "2054-446 Incorrect processing direction."
MSG_SB_7 "2054-447 The scrollbar increment is less than 1."
MSG_SB_8 "2054-448 The scrollbar page increment is less than 1."
MSG_SB_9 "2054-449 The scrollbar initial delay is less than 1."
MSG_SB_10 "2054-450 The scrollbar repeat delay is less than 1."
MSG_SB_11 "2054-451 Error in context manager; scrollbar backgrounds\n\
         cannot be set correctly"
MSG_SB_12 "2054-452 Error in context manager; scrollbar foregrounds\n\
         cannot be set correctly"
MSG_SB_13 "2054-453 Specified slider size is greater than the scrollbar\n\
         maximum value minus the scrollbar minimum value." 


$set MS_SWindow

MSG_SW_1 "2054-461 Invalid ScrollBar Display policy."
MSG_SW_2 "2054-462 Invalid Scrolling Policy."
MSG_SW_3 "2054-463 Invalid Visual Policy."
MSG_SW_4 "2054-464 Invalid placement policy."
MSG_SW_6 "2054-466 Cannot change scrolling policy after initialization."
MSG_SW_7 "2054-467 Cannot change visual policy after initialization."
MSG_SW_8 "2054-468 Cannot set AS_NEEDED scrollbar policy with a\n\
         visual policy of VARIABLE."
$ Do not translate AS_NEEDED or VARIABLE.
MSG_SW_9 "2054-469 Cannot change scrollbar widget in AUTOMATIC mode."
$ Do not translate AUTOMATIC.
MSG_SW_10 "2054-470 Cannot change clip window"
MSG_SW_11 "Cannot set visual policy of CONSTANT in APPLICATION_DEFINED mode." 
$ Do not translate CONSTANT or APPLICATION_DEFINED.
MSG_SW_12 "2053-xxx Wrong parameters passed to the function."


$set MS_SBoX

MSG_SBX_1 "2054-481 Incorrect dialog type."
MSG_SBX_2 "2054-482 Dialog type cannot be modified."
$ Do not translate Dialog.
MSG_SBX_3 "2054-483 Only one work area child allowed."
MSG_SBX_4 "2054-484 Invalid child type."


$set MS_SEparator

MSG_SE_1 "2054-491 Invalid separator type."
MSG_SE_2 "2054-492 Invalid orientation."


$set MS_Text

MSG_T_2 "2054-501 Invalid source, source ignored."
MSG_T_3 "2054-502 Invalid edit mode."
MSG_T_4 "2054-503 Text widget is editable, traversalOn must be true."
$ Do not translate traversalOn.
MSG_T_5 "2054-504 Can't find position in MovePreviousLine()."
$ Do not translate MovePreviousLine.
MSG_T_6 "2054-505 Invalid rows, must be > 0"
MSG_T_7 "2054-xxx XmFontListInitFontContext failed."
$ please do not translate XmFontListInitFontContext.
MSG_T_8 "2054-xxx XmFontListGetNextFont failed." 
$ please do not translate XmFontListGetNextFont.
MSG_T_9 "2054-xxx Character '%c' is not supported in font. It is discarded."
MSG_T_10 "2054-xxx Character '%s' is not supported in font. It is discarded."
MSG_T_11 "2054-xxx Cannot use a multibyte locale without a fontset. \n\
	The value is discarded."


$set MS_TField

MSG_TF_1 "2054-511 Invalid cursor position, must be >= 0."
MSG_TF_2 "2054-512 Invalid columns, must be > 0."
MSG_TF_3 "2054-513 traversalOn must always be true."
$ Do not translate traversalOn.
MSG_TF_4 "2054-514 Invalid columns, must be >= 0."


$set MS_TButton

MSG_TB_1 "2054-521 Indicator type should be either XmONE_OF_MANY or XmN_OF_MANY"


$set MS_Traversal

MSG_TV_1 "2054-531 Invalid value for navigation_type"
MSG_TV_2 "2054-532 Wrong value in old for navigation_type!!"
MSG_TV_3 "2054-533 Traversal bootstrap situation with bad parameters"
MSG_TV_4 "2054-534 Attempt to traverse to new tab using bad parameters"
MSG_TV_5 "2054-535 startWidget is not in child list"
$ Do not translate startWidget
MSG_TV_6 "2054-536 Bad parameters to TraverseToChild"
$ Do not translate TraverseToChild.


$set MS_Vendor

MSG_V_1 "2054-541 Invalid value for delete response"
MSG_V_2 "2054-542 Invalid XmNpreeditType, default to OverTheSpot"


$set MS_VendorE

MSG_VE_1 "2054-545 FetchUnitType: bad widget class"
MSG_VE_2 "2054-xxx String to no-op conversion needs no extra parameters."
MSG_VE_3 "2054-xxx FetchUnitType called without a widget to reference."
$ Do not translate FetchUnitType.


$set MS_VKey

MSG_VK_1 "2054-551 Virtual bindings Initialize hasn't been called"



$set MS_VIsual

MSG_VI_1 "2054-561 Invalid color requested from _XmAccessColorData"
MSG_VI_2 "2054-562 Cannot allocate colormap entry for background,\n\
         setting background to white"
MSG_VI_3 "2054-563 Cannot parse given background color,\n\
         setting background to white"


$set MS_IM

MSG_IM_1 "2054-571 The specified Input Method faild to init : %s"
MSG_IM_2 "2054-572 Cannot create the Input Method Object"
MSG_IM_3 "2054-573 XmIMFocus invoked with NULL widget."
MSG_IM_4 "2054-574 XmIMMove invoked without the Input Method focus."


$set MS_RESOURCES

MSG_Res_1 "OK"
$ "OK" as in "Go ahead"/"Confirmed to proceed as instructed"/"Fine"
MSG_Res_2 "Cancel"
$ ... as in "Never mind"/"abort this operation"/"Stop current request"
MSG_Res_3 "Selection"
$ ... as in "choosing from a list of items"
MSG_Res_4 "Apply"
$ ... as in "To do something with the things selected"
MSG_Res_5 "Help"
$ ... as in "Need more guidance on how to deal with this dialog box"
MSG_Res_6 "Filter"
$ ... the noun "Filter", as in "a sifter/a filter/a qualifier to screen out
$ files with"
MSG_Res_7 "Files"
$ ... as in "a list of the files"
MSG_Res_8 "Directories"
$ ... as in "a list of the directories" 
MSG_Res_9 "Items"
$ ... as in "a list of things to choose from"
MSG_Res_10 "Filter"
$ ... the verb "Filter", as in "To screen out some files"
$ Note this has a slightly different semantics from MSG_Res_6.
$ In English, it is the same word, but MSG_Res_6 means "a sifter", and
$ MSG_Res_10 means "Push this button to filter out some files"
$ MSG_Res_6 is a label indicating "entry below is qualifier to filter out 
$ files with"

$ ##########################################################
$ The following message sets are added for MOTIF 1.2 support
$ ##########################################################


$set MS_BaseC        

MSG_BC_1 "2054-xxx No context found for extension."
MSG_BC_2 "2054-xxx _XmPopWidgetExtData: no extension found with XFindContext."
MSG_BC_3 "2054-xxx XmFreeWidgetExtData is not a supported routine."
MSG_BC_4 "2054-xxx getLabelSecResData: not enough memory."


$set MS_Display

MSG_DSP_1 "2054-xxx Creating multiple XmDisplays for the same X display.\n\
	Only the first XmDisplay created for a particular X display\n\
	can be referenced by calls to XmGetXmDisplay."
$ please do not translate XmDisplay or XmGetXmDisplay
MSG_DSP_2 "2054-xxx Received TOP_LEVEL_LEAVE with no active DragContext."
$ please do not translate TOP_LEVEL_LEAVE or DragContext
MSG_DSP_3 "2054-xxx Cannot set XmDisplay class to a non-subclass of XmDisplay."
$ please do not translate XmDisplay


$set MS_DragBS

MSG_DRB_1 "2054-xxx The _MOTIF_DRAG_WINDOW has been destroyed."
$ please do not translate _MOTIF_DRAG_WINDOW
MSG_DRB_2 "2054-xxx The protocol version levels do not match."
MSG_DRB_3 "2054-xxx Unable to open display."
MSG_DRB_4 "2054-xxx The atoms table is empty."
MSG_DRB_5 "2054-xxx The target table is empty."
MSG_DRB_6 "2054-xxx The target table has an inconsistent property."
MSG_DRB_7 "2054-xxx Invalid target table index."


$set MS_DragC

MSG_DRC_1 "2054-xxx GenerateCallback does not expect XmCR_DROP_SITE_ENTER as a reason."
$ please do not translate GenerateCallback or XmCR_DROP_SITE_ENTER
MSG_DRC_2 "2054-xxx Invalid selection in DropConvertCallback."
$ please do not translate DropConvertCallback
MSG_DRC_3 "2054-xxx The drop selection was lost."
MSG_DRC_4 "2054-xxx XGrabPointer failed."
MSG_DRC_5 "2054-xxx ExternalNotifyHandler: the callback reason is not acceptable."
$ please do not translate ExternalNotifyHandler
MSG_DRC_6 "2054-xxx XmDragStart must be called as a result of a button press."
$ please do not translate XmDragStart


$set MS_DragICC

MSG_DRI_1 "2054-xxx Unknown drag and drop message type."


$set MS_DragIcon

MSG_DI_1 "2054-xxx No geometry specified for dragIcon pixmap."
MSG_DI_2 "2054-xxx dragIcon created with no pixmap."
MSG_DI_3 "2054-xxx String to Bitmap converter needs Screen."
$ please do not translate dragIcon


$set MS_DragOver

MSG_DO_1 "2054-xxx Depth mismatch."
MSG_DO_2 "2054-xxx Unknown icon attachment."
MSG_DO_3 "2054-xxx Unknown drag state."
MSG_DO_4 "2054-xxx Unknown blendModel."
$ please do not translate blendModel


$set MS_DragUnder

MSG_DU_1 "2054-xxx Unable to get dropSite window geometry."
$ please do not translate dropSite
MSG_DU_2 "2054-xxx Invalid animationPixmapDepth."
$ please do not translate animationPixmapDepth


$set MS_DropS

MSG_DRS_1 "2054-xxx Cannot create drop sites which are children of a simple drop site."
MSG_DRS_2 "2054-xxx Receiving Motion Events without an active drag context."
MSG_DRS_3 "2054-xxx Receiving operation changed without an active drag context."
MSG_DRS_4 "2054-xxx Creating an active drop site with no drop procedure."
MSG_DRS_5 "2054-xxx Cannot set rectangles or rectangle numbers of composite dropsites."
MSG_DRS_6 "2054-xxx Registering a widget as a drop site out of sequence.\n\
	Ancestors must be registered before any of their \n\
	descendants are registered."
MSG_DRS_7 "2054-xxx Cannot register widget as a drop site more than once."
MSG_DRS_8 "2054-xxx DropSite type may only be set at creation time."
$ please do not translate DropSite
MSG_DRS_9 "2054-xxx Cannot change rectangles of non-simple dropsite."
MSG_DRS_10 "2054-xxx Cannot register a drop site which is a descendent of a simple drop site."
MSG_DRS_11 "2054-xxx Cannot create a discontiguous child list for a composite drop site."
MSG_DRS_12 "2054-xxx %s is not a drop site child of %s."


$set MS_GeoUtil

MSG_GEO_1 "2054-xxx Geometry request to \"almost\" reply failed."
$ please do not translate "almost"


$set MS_Region

MSG_RE_1 "2054-xxx Memory error."


$set MS_RepType

MSG_REP_1 "2054-xxx Illegal representation type id."
MSG_REP_2 "2054-xxx Illegal value (%d) for rep type XmR%s."
$ please do not translate XmR%s


$set MS_ResCvt

MSG_RSC_1 "2054-xxx FetchUnitType: bad widget class."


$set MS_Screen

MSG_SCR_1 "2054-xxx Icon screen mismatch."
MSG_SCR_2 "2054-xxx Cannot get XmScreen because XmDisplay was not found."


$set MS_ColObj

MSG_CO_1 "Could not allocate memory for color object data."
MSG_CO_2 "Bad screen number from color server selection."


$set MS_IconG

MSG_IG_1 "Incorrect alignment."
MSG_IG_2 "Incorrect behavior."
MSG_IG_3 "Incorrect fill mode."
MSG_IG_4 "Incorrect string or pixmap position."
MSG_IG_5 "Incorrect margin width or height."
MSG_IG_6 "Incorrect shadow type."
