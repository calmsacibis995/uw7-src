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

$set 1

1 "2054-201 The arrow direction is not correct."


$set 2

1 "2054-202 Incorrect resize policy."
2 "2054-203 Incorrect dialog style."
3 "2054-204 Incorrect shadow type."
4 "2054-205 Null font list (no vendor shell default)"


$set 3

1 "2054-211 XmCascadeButton must have correct type of \n\
         XmRowColumnWidgetClass parent."
2 "2054-212 Only XmMENU_PULLDOWN XmRowColumnWidgets can be submenus."
$ Do not translate submenus.
3 "2054-213 MapDelay must be >= 0."
4 "2054-214 XmCascadeButtonGadget must have xmRowColumnWidgetClass \n\
         parent with rowColumnType XmMENU_PULLDOWN, XmMENU_POPUP, \n\
         XmMENU_BAR or XmMENU_OPTION."
5 "2054-xxx XtGrabPointer failed."
6 "2054-xxx XtGrabKeyboard failed."


$set 4

1 "2054-221 The dialog type must be XmDIALOG_COMMAND."
2 "2054-222 Invalid child type.\n\
         The Command widget does not have this child."
$ Do not translate Command.
3 "2054-223 Invalid XmString, check for invalid charset."
4 "2054-224 NULL or empty string passed in to CommandAppendValue."
5 "2054-225 mustMatch is always False for a Command widget."
$ Do not translate mustMatch, False or Command.   
6 "2054-226 historyMaxItems must be a positive integer greater than zero."


$set 5

1 "2054-231 Must call XmClipboardStartCopy() before XmClipboardCopy()"
2 "2054-232 Must call XmClipboardStartCopy() before XmClipboardEndCopy()"
3 "2054-233 Too many formats in XmClipboardCopy()"


$set 6

1 "2054-241 DialogShell widget only supports one rectObj child"
$ Do not transalate rectObj
2 "2054-242 gadgets aren't allowed in shell"


$set 7

1 "2054-251 Margin width or height cannot be negative."
2 "2054-252 Incorrect resize policy."


$set 8

1 "2054-261 Fraction base cannot be zero."
2 "2054-262 Incorrect form attachment type."
3 "2054-263 Cannot set constraints for non-resizable widget."
4 "2054-264 Attachment widget must not be null."
$ Do not translate Attachment
5 "2054-265 Circular dependency in Form children."
$ Do not translate Form.
6 "2054-266 Edge attached to a widget but no widget specified."
7 "2054-267 Bailed out of edge synchronization after 10,000 iterations\n\
         Check for contradictory constraints on the children of this form."
8 "2054-268 Attachment widget must be have same parent as widget."
$ do not translate Attachment


$set 9

1 "2054-272 Only one child should be inserted in a frame."
2 "2054-273 Invalid margin width."
3 "2054-274 Invalid margin height." 


$set 10

1 "2054-281 Invalid highlight thickness."
2 "2054-282 The unit type is incorrect."
3 "2054-283 Invalid shadow thickness."
4 "2054-284 Cannot set pixmap resource to unspecified."



$set 11

1 "2054-291 Invalid XmNlabeType " 
2 "2054-292 Invalid value in XmNalignment"
3 "2054-293 Invalid value in XmNstringDirection"
4 "2054-294 Invalid XmNlabelString - must be a compound string"
5 "2054-295 Invalid XmNacceleratorText - must be a compound string"


$set 12

1 "2054-300 List must have at least one visiable item."
2 "2054-301 Invalid Selection Policy."
3 "2054-302 Invalid Size Policy."
4 "2054-303 Invalid ScrollBar Display Policy."
5 "2054-304 Invalid String Direction."
6 "2054-305 Cannot change size policy after initialization."
7 "2054-306 Must set item count to non-negative value."
8 "2054-307 NULL font in SetValues ignored."
$ Do not translate SetValues
9 "2054-308 Invalid item(s) to delete."
10 "2054-309 No Horizontal Scrollbar to set."
11 "2054-310 Invalid Margin setting."
12 "2054-311 Invalid Spacing Value."
13 "2054-312 Cannot set items to NULL with non-zero item count."
14 "2054-313 Must set selected item count to non-negative value."
15 "2054-314 Cannot set selected items to NULL with non-zero item count."
16 "2054-315 Cannot set top position less than 1."
17 "2054-316 XmNitems and XmNitemCount mismatch!"
18 "2054-317 Cannot leave add mode in multiple selection."


$set 13

1 "2054-321 The Menu Bar cannot be changed to NULL."
2 "2054-322 The Command Window cannot be changed to NULL."
3 "2054-323 Negative margin value ignored."


$set 14

1 "2054-331 MenuShell widgets must have a xmRowColumnWidgetClass child"
2 "2054-332 Attempting to manage an incomplete menu."


$set 15

1 "2054-341 Invalid Dialog Type."
2 "2054-342 Invalid Default Button Type."
3 "2054-343 Invalid Alignment Type."
4 "2054-344 Invalid Child Type."
5 "2054-345 PushButton Id cannot be changed directly."
6 "2054-346 Use XmNdefaultButtonType to set MessageBox default button."


$set 16

1 "2054-354 Invalid minimum value, must be greater than zero."
2 "2054-355 Invalid maximum value, must be greater than zero."
3 "2054-356 Invalid minimum/maximum value, minimum value must be \n\
         smaller than the maximum value."
4 "2054-357 Constraints do not allow appropriate sizing."
5 "2054-358 Too few parameters."
6 "2054-359 Invalid 1st parameter."


$set 17

1 "2054-361 fontList is not defined"
$ Do not translate fontList.


$set 18

1 "2054-371 Must be a vendor shell" 
2 "2054-372 Protocol manager already exists"
3 "2054-373 More protocols than I can handle"


$set 19

1 "2054-381 Not enough memory" 


$set 20
$ In this set, do not translate the word that starts with "XmN or Xm".

1 "2054-401 Attempt to set width to zero.\n\
         Set to default value 16."
2 "2054-402 Attempt to set width to zero.\n\
         The value is ignored."
3 "2054-403 Attempt to set height to zero.\n\
         Set to default value 16."
4 "2054-404 Attempt to set height to zero.\n\
         The value is ignored."
5 "2054-405 XmNhelpWidget not used by PopUps.\n\
         It is set to NULL."
6 "2054-406 XmNhelpWidget not used by Pulldowns.\n\
         It is set to NULL."
7 "2054-407 XmNhelpWidget not used by Option menus.\n\
         It is set to NULL."
8 "2054-408 XmNhelpWidget not used by Work Areas.\n\
         It is set to NULL."
9 "2054-409 Unknown value of XmNrowColumnType.\n\
         It is set to WorkArea."
10 "2054-410 Widget hierarchy not appropriate for this \n\
         XmNrowColumnType.  It is set to WorkArea."
11 "2054-411 Attempt to change XmNrowColumnType after initialization.\n\
         The value is ignored."
12 "2054-412 Unknown value of XmNorientation.\n\
         The default value is used."
13 "2054-413 Attempt to set XmNorientation to unknown value.\n\
         The value is ignored."
14 "2054-414 Unknown value of XmNpacking.\n\
         The default value is used."
15 "2054-415 Attempt to set XmNpacking to unknown value.\n\
         The value is ignored."
16 "2054-416 Unknown value of XmNentryAlignment.\n\
         The default value is used."
17 "2054-417 Attempt to set XmNentryAlignment to unknown value.\n\
         The value is ignored."
18 "2054-418 Attempt to set XmNisHomogenous to FALSE for \n\
         a RowColumn widget of type XmMENU_BAR.\n\
         The value is ignored."
19 "2054-419 Attempt to change XmNentryClass for a RowColumn \n\
         widget of type XmMENU_BAR.  The value is ignored."
20 "2054-420 Attempt to change XmNwhichButton via XtSetValues \n\
         for a RowColumn widget of type XmMENU_PULLDOWN.\n\
         The value is ignored."
21 "2054-421 Attempt to change XmNmenuPost via XtSetValues for \n\
         a RowColumn widget of type XmMENU_PULLDOWN.\n\
         The value is ignored."
22 "2054-422 Attempt to set XmNpostMenu to an illegal value.\n\
         The value is ignored."
23 "2054-423 Attempt to change XmNshadowThickness for a RowColumn \n\
         widget not of type XmMENU_PULLDOWN or XmMENU_POPUP.\n\
         The value is ignored."
24 "2054-024 Attempt to change XmNorientation for a RowColumn \n\
         widget of type XmMENU_OPTION.\n\
         The value is ignored."
25 "2054-425 Attempt to add wrong type child to a menu \n\
         (i.e. RowColumn) widget"
26 "2054-426 Attempt to add wrong type child to a homogeneous \n\
         RowColumn widget"
27 "2054-427 Attempt to change XmNisHomogeneous for a RowColumn \n\
	 widget of type XmMENU_OPTION ignored"
28 "2054-xxx Tear off enabled on a shared menupane is allowed \n\
	 but not recommended."
29 "2054-xxx Illegal mnemonic character.  Could not convert X KEYSYM \n\
	 to a keycode."


$set 21

1 "2054-431 The scale minumum value is greater than or \n\
         equal to the scale maximum value."
2 "2054-432 The specified scale value is less than \n\
         the minimum scale value."
3 "2054-433 The specified scale value is greater than \n\
         the maximum scale value."
4 "2054-434 Incorrect orientation."
5 "2054-435 Incorrect processing direction."
6 "2054-436 Invalid highlight thickness."
7 "2054-437 Invalid scaleMultiple; greater than (max - min)"
8 "2054-438 Invalid scaleMultiple; less than zero"
9 "2054-xxx (Maximum - minimum) cannot be greater than \n\
	INT_MAX divided by 2.  Miminimum has been set to zero. \n\
	 Maximum may have been set to (INT_MAX/2)."



$set 22

1 "2054-441 The scrollbar minimum value is greater than\n\
         or equal to the scrollbar maximum value."
2 "2054-442 The specified slider size is less than 1"
3 "2054-443 The specified scrollbar value is less than\n\
         the minimum scrollbar value."
4 "2054-444 The specified scrollbar value is greater than\n\
         the maximum scrollbar value minus the scrollbar slider size."
5 "2054-445 Incorrect orientation."
6 "2054-446 Incorrect processing direction."
7 "2054-447 The scrollbar increment is less than 1."
8 "2054-448 The scrollbar page increment is less than 1."
9 "2054-449 The scrollbar initial delay is less than 1."
10 "2054-450 The scrollbar repeat delay is less than 1."
11 "2054-451 Error in context manager; scrollbar backgrounds\n\
         cannot be set correctly"
12 "2054-452 Error in context manager; scrollbar foregrounds\n\
         cannot be set correctly"
13 "2054-453 Specified slider size is greater than the scrollbar\n\
         maximum value minus the scrollbar minimum value." 


$set 23

1 "2054-461 Invalid ScrollBar Display policy."
2 "2054-462 Invalid Scrolling Policy."
3 "2054-463 Invalid Visual Policy."
4 "2054-464 Invalid placement policy."
5 "2054-466 Cannot change scrolling policy after initialization."
6 "2054-467 Cannot change visual policy after initialization."
7 "2054-468 Cannot set AS_NEEDED scrollbar policy with a\n\
         visual policy of VARIABLE."
$ Do not translate AS_NEEDED or VARIABLE.
8 "2054-469 Cannot change scrollbar widget in AUTOMATIC mode."
$ Do not translate AUTOMATIC.
9 "2054-470 Cannot change clip window"
10 "Cannot set visual policy of CONSTANT in APPLICATION_DEFINED mode." 
$ Do not translate CONSTANT or APPLICATION_DEFINED.
11 "2053-xxx Wrong parameters passed to the function."


$set 24

1 "2054-481 Incorrect dialog type."
2 "2054-482 Dialog type cannot be modified."
$ Do not translate Dialog.
3 "2054-483 Only one work area child allowed."
4 "2054-484 Invalid child type."


$set 25

1 "2054-491 Invalid separator type."
2 "2054-492 Invalid orientation."


$set 26

1 "2054-501 Invalid source, source ignored."
2 "2054-502 Invalid edit mode."
3 "2054-503 Text widget is editable, traversalOn must be true."
$ Do not translate traversalOn.
4 "2054-504 Can't find position in MovePreviousLine()."
$ Do not translate MovePreviousLine.
5 "2054-505 Invalid rows, must be > 0"
6 "2054-xxx XmFontListInitFontContext failed."
$ please do not translate XmFontListInitFontContext.
7 "2054-xxx XmFontListGetNextFont failed." 
$ please do not translate XmFontListGetNextFont.
8 "2054-xxx Character '%c' is not supported in font. It is discarded."
9 "2054-xxx Character '%s' is not supported in font. It is discarded."
10 "2054-xxx Cannot use a multibyte locale without a fontset. \n\
	The value is discarded."


$set 27

1 "2054-511 Invalid cursor position, must be >= 0."
2 "2054-512 Invalid columns, must be > 0."
3 "2054-513 traversalOn must always be true."
$ Do not translate traversalOn.
4 "2054-514 Invalid columns, must be >= 0."


$set 28

1 "2054-521 Indicator type should be either XmONE_OF_MANY or XmN_OF_MANY"


$set 29

1 "2054-531 Invalid value for navigation_type"
2 "2054-532 Wrong value in old for navigation_type!!"
3 "2054-533 Traversal bootstrap situation with bad parameters"
4 "2054-534 Attempt to traverse to new tab using bad parameters"
5 "2054-535 startWidget is not in child list"
$ Do not translate startWidget
6 "2054-536 Bad parameters to TraverseToChild"
$ Do not translate TraverseToChild.


$set 30

1 "2054-541 Invalid value for delete response"
2 "2054-542 Invalid XmNpreeditType, default to OverTheSpot"


$set 31

1 "2054-545 FetchUnitType: bad widget class"
2 "2054-xxx String to no-op conversion needs no extra parameters."
3 "2054-xxx FetchUnitType called without a widget to reference."
$ Do not translate FetchUnitType.


$set 32

1 "2054-551 Virtual bindings Initialize hasn't been called"



$set 33

1 "2054-561 Invalid color requested from _XmAccessColorData"
2 "2054-562 Cannot allocate colormap entry for background,\n\
         setting background to white"
3 "2054-563 Cannot parse given background color,\n\
         setting background to white"


$set 34

1 "2054-571 The specified Input Method faild to init : %s"
2 "2054-572 Cannot create the Input Method Object"
3 "2054-573 XmIMFocus invoked with NULL widget."
4 "2054-574 XmIMMove invoked without the Input Method focus."


$set 35

1 "OK"
$ "OK" as in "Go ahead"/"Confirmed to proceed as instructed"/"Fine"
2 "Cancel"
$ ... as in "Never mind"/"abort this operation"/"Stop current request"
3 "Selection"
$ ... as in "choosing from a list of items"
4 "Apply"
$ ... as in "To do something with the things selected"
5 "Help"
$ ... as in "Need more guidance on how to deal with this dialog box"
6 "Filter"
$ ... the noun "Filter", as in "a sifter/a filter/a qualifier to screen out
$ files with"
7 "Files"
$ ... as in "a list of the files"
8 "Directories"
$ ... as in "a list of the directories" 
9 "Items"
$ ... as in "a list of things to choose from"
10 "Filter"
$ ... the verb "Filter", as in "To screen out some files"
$ Note this has a slightly different semantics from MSG_Res_6.
$ In English, it is the same word, but MSG_Res_6 means "a sifter", and
$ 10 means "Push this button to filter out some files"
$ 6 is a label indicating "entry below is qualifier to filter out 
$ files with"

$ ##########################################################
$ The following message sets are added for MOTIF 1.2 support
$ ##########################################################


$set 36

1 "2054-xxx No context found for extension."
2 "2054-xxx _XmPopWidgetExtData: no extension found with XFindContext."
3 "2054-xxx XmFreeWidgetExtData is not a supported routine."
4 "2054-xxx getLabelSecResData: not enough memory."


$set 37

1 "2054-xxx Creating multiple XmDisplays for the same X display.\n\
	Only the first XmDisplay created for a particular X display\n\
	can be referenced by calls to XmGetXmDisplay."
$ please do not translate XmDisplay or XmGetXmDisplay
2 "2054-xxx Received TOP_LEVEL_LEAVE with no active DragContext."
$ please do not translate TOP_LEVEL_LEAVE or DragContext
3 "2054-xxx Cannot set XmDisplay class to a non-subclass of XmDisplay."
$ please do not translate XmDisplay


$set 38

1 "2054-xxx The _MOTIF_DRAG_WINDOW has been destroyed."
$ please do not translate _MOTIF_DRAG_WINDOW
2 "2054-xxx The protocol version levels do not match."
3 "2054-xxx Unable to open display."
4 "2054-xxx The atoms table is empty."
5 "2054-xxx The target table is empty."
6 "2054-xxx The target table has an inconsistent property."
7 "2054-xxx Invalid target table index."


$set 39

1 "2054-xxx GenerateCallback does not expect XmCR_DROP_SITE_ENTER as a reason."
$ please do not translate GenerateCallback or XmCR_DROP_SITE_ENTER
2 "2054-xxx Invalid selection in DropConvertCallback."
$ please do not translate DropConvertCallback
3 "2054-xxx The drop selection was lost."
4 "2054-xxx XGrabPointer failed."
5 "2054-xxx ExternalNotifyHandler: the callback reason is not acceptable."
$ please do not translate ExternalNotifyHandler
6 "2054-xxx XmDragStart must be called as a result of a button press."
$ please do not translate XmDragStart


$set 40

1 "2054-xxx Unknown drag and drop message type."


$set 41

1 "2054-xxx No geometry specified for dragIcon pixmap."
2 "2054-xxx dragIcon created with no pixmap."
3 "2054-xxx String to Bitmap converter needs Screen."
$ please do not translate dragIcon


$set 42

1 "2054-xxx Depth mismatch."
2 "2054-xxx Unknown icon attachment."
3 "2054-xxx Unknown drag state."
4 "2054-xxx Unknown blendModel."
$ please do not translate blendModel


$set 43

1 "2054-xxx Unable to get dropSite window geometry."
$ please do not translate dropSite
2 "2054-xxx Invalid animationPixmapDepth."
$ please do not translate animationPixmapDepth


$set 44

1 "2054-xxx Cannot create drop sites which are children of a simple drop site."
2 "2054-xxx Receiving Motion Events without an active drag context."
3 "2054-xxx Receiving operation changed without an active drag context."
4 "2054-xxx Creating an active drop site with no drop procedure."
5 "2054-xxx Cannot set rectangles or rectangle numbers of composite dropsites."
6 "2054-xxx Registering a widget as a drop site out of sequence.\n\
	Ancestors must be registered before any of their \n\
	descendants are registered."
7 "2054-xxx Cannot register widget as a drop site more than once."
8 "2054-xxx DropSite type may only be set at creation time."
$ please do not translate DropSite
9 "2054-xxx Cannot change rectangles of non-simple dropsite."
10 "2054-xxx Cannot register a drop site which is a descendent of a simple drop site."
11 "2054-xxx Cannot create a discontiguous child list for a composite drop site."
12 "2054-xxx %s is not a drop site child of %s."


$set 45

1 "2054-xxx Geometry request to \"almost\" reply failed."
$ please do not translate "almost"


$set 46

1 "2054-xxx Memory error."


$set 47

1 "2054-xxx Illegal representation type id."
2 "2054-xxx Illegal value (%d) for rep type XmR%s."
$ please do not translate XmR%s


$set 48

1 "2054-xxx FetchUnitType: bad widget class."


$set 49

1 "2054-xxx Icon screen mismatch."
2 "2054-xxx Cannot get XmScreen because XmDisplay was not found."


$set 50

1 "Could not allocate memory for color object data."
2 "Bad screen number from color server selection."


$set 51

1 "Incorrect alignment."
2 "Incorrect behavior."
3 "Incorrect fill mode."
4 "Incorrect string or pixmap position."
5 "Incorrect margin width or height."
6 "Incorrect shadow type."
