#pragma ident	"@(#)m1.2libs:Xm/RepType.c	1.2"
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
*  (c) Copyright 1991, 1992 HEWLETT-PACKARD COMPANY */

#include "XmI.h"
#include "RepTypeI.h"
#include "MessagesI.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MESSAGE1 _XmMsgRepType_0001
#define MESSAGE2 _XmMsgRepType_0002


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


/* INSTRUCTIONS to add a statically-stored representation type:
 *    (For dynamically allocated/created representation types, see the
 *     man page for XmRepTypeRegister).
 *
 *  1) Determine whether or not the numerical values of the representation
 *     type can be enumerated with consecutive numerical values beginning 
 *     with value zero.  If this is the case, continue with step 2).
 *
 *     If this is not the case, the representation type needs an extra
 *     array in the data structure to map the numerical resource value to
 *     the array position of the value name in the representation type data
 *     structures.  If the representation type must be mapped in this way,
 *     go to step 2M).
 *
 *  2) Define a static array of the names of the values for the
 *     representation type in RepType.c.  Use the representation type name,
 *     plus the suffix "Names" for the name of the array (see existing name
 *     arrays for an example).  The ordering of the value names in this
 *     array determines the numercial value of each name, beginning with
 *     zero and incrementing consecutively.
 *
 *  3) Add an enumeration symbol for the ID number of the representation
 *     type in the enum statement in the RepTypeI.h module.  There are
 *     two sections to the enumerated list; add the new type (using the
 *     XmRID prefix) to the FIRST section in ALPHABETICAL ORDER!!!
 *     The beginning of the first section can be identified by the
 *     assignment using XmREP_TYPE_STD_TAG.
 *
 *  4) Add an element to the static array of representation type data
 *     structures named "_XmStandardRepTypes".  Add the new element to
 *     the array in ALPHABETICAL ORDER (according to the name of the
 *     representation type).  Use the same format as the other elements
 *     in the array; the fields which are initialized with XtImmediate,
 *     FALSE, and 0 should be the same for all elements of the array.
 *
 *  5) You're done.  A generic "string to representation type" converter
 *     for the representation type that you just added will be automatically
 *     registered when all other Xm converters are registered.
 *
 ******** For "mapped" representation types: ********
 *
 *  2M) Define a static array of the numerical values for the
 *     representation type in RepType.c.  Use the enumerated symbols
 *     (generally defined in Xm.h) to initialize the array of numerical
 *     values.  Use the representation type name, plus the suffix "Map"
 *     for the name of the array (see existing map arrays for an example).
 *
 *  3M) Define a static array of the names of the values for the
 *     representation type in RepType.c.  Use the representation type name,
 *     plus the suffix "Names" for the name of the array (see existing name
 *     arrays for an example).  The ordering of the value names in this
 *     array determines the numercial value of each name, with the first
 *     element corresponding to the first element in the "Map" array, etc.
 *
 *  4M) Add an enumeration symbol for the ID number of the representation
 *     type in the enum statement in the RepTypeI.h module.  There are
 *     two sections to the enumerated list; add the new type (using the
 *     XmRID prefix) to the SECOND section in ALPHABETICAL ORDER!!!
 *     The beginning of the second section can be identified by the
 *     assignment using XmREP_TYPE_MAP_TAG.
 *
 *  5M) Add an element to the static array of representation type data
 *     structures named "_XmStandardMappedRepTypes".  Add the new element
 *     to the array in ALPHABETICAL ORDER (according to the name of the
 *     representation type).  Use the same format as the other elements
 *     in the array; the fields which are initialized with XtImmediate,
 *     FALSE, and 0 should be the same for all elements of the array.
 *
 *  6M) You're done.  A generic "string to representation type" converter
 *     for the representation type that you just added will be automatically
 *     registered when all other Xm converters are registered.
 */

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static String * CopyStringArray() ;
static Boolean ValuesConsecutive() ;
static XmRepType GetRepTypeRecord() ;
static unsigned int GetByteDataSize() ;
static void CopyRecord() ;
static Boolean ConvertRepType() ;
static Boolean ReverseConvertRepType() ;

#else

static String * CopyStringArray( 
                        String *StrArray,
#if NeedWidePrototypes
                        unsigned int NumEntries,
                        int NullTerminate,
                        int UppercaseFormat) ;
#else
                        unsigned char NumEntries,
                        Boolean NullTerminate,
                        Boolean UppercaseFormat) ;
#endif /* NeedWidePrototypes */
static Boolean ValuesConsecutive( 
                        unsigned char *values,
#if NeedWidePrototypes
                        unsigned int num_values) ;
#else
                        unsigned char num_values) ;
#endif /* NeedWidePrototypes */
static XmRepType GetRepTypeRecord( 
#if NeedWidePrototypes
                        int rep_type_id) ;
#else
                        XmRepTypeId rep_type_id) ;
#endif /* NeedWidePrototypes */
static unsigned int GetByteDataSize( 
                        XmRepType Record) ;
static void CopyRecord( 
                        XmRepType Record,
                        XmRepTypeEntry OutputEntry,
                        XtPointer *PtrDataArea,
                        XtPointer *ByteDataArea) ;
static Boolean ConvertRepType( 
                        Display *disp,
                        XrmValue *args,
                        Cardinal *n_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;
static Boolean ReverseConvertRepType( 
                        Display *disp,
                        XrmValue *args,
                        Cardinal *n_args,
                        XrmValue *from,
                        XrmValue *to,
                        XtPointer *converter_data) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static String AlignmentNames[] =
{   "alignment_beginning", "alignment_center", "alignment_end"
    } ;
static String AnimationStyleNames[] =
{   "drag_under_none", "drag_under_pixmap", "drag_under_shadow_in",
    "drag_under_shadow_out", "drag_under_highlight"
    };
static String ArrowDirectionNames[] =
{   "arrow_up", "arrow_down", "arrow_left", "arrow_right"
    } ;
static String AttachmentNames[] =
{   "attach_none", "attach_form", "attach_opposite_form", "attach_widget",
    "attach_opposite_widget", "attach_position", "attach_self"
    } ;
static String AudibleWarningNames[] =
{   "none", "bell"
    } ;
static String BlendModelNames[] =
{   "blend_all", "blend_state_source", "blend_just_source", "blend_none"
    } ;
#define ChildHorizontalAlignmentNames   AlignmentNames

static char *ChildPlacementNames[] =
{   "place_top", "place_above_selection", "place_below_selection"
    } ;
static char *ChildTypeNames[] =
{   "frame_generic_child", "frame_workarea_child", "frame_title_child"
    } ;
static String ChildVerticalAlignmentNames[] =
{   "alignment_baseline_top", "alignment_center", "alignment_baseline_bottom",
    "alignment_widget_top", "alignment_widget_bottom"
    } ;
static String CommandWindowLocationNames[] =
{   "command_above_workspace", "command_below_workspace"
    } ;
static String DialogTypeNames[] =
{   "dialog_template", "dialog_error", "dialog_information", "dialog_message",
    "dialog_question", "dialog_warning", "dialog_working"
    } ;
static String DragInitiatorProtocolStyleNames[] =
{   "drag_none", "drag_drop_only", "drag_prefer_preregister",
    "drag_preregister", "drag_prefer_dynamic", "drag_dynamic",
    "drag_prefer_receiver"
	};
static String DragReceiverProtocolStyleNames[] =
{   "drag_none", "drag_drop_only", "drag_prefer_preregister",
    "drag_preregister", "drag_prefer_dynamic", "drag_dynamic"
	};
static String DropSiteActivityNames[] =
{   "drop_site_active", "drop_site_inactive"
	};
static String DropSiteTypeNames[] =
{   "drop_site_simple",   "drop_site_composite"
	};
static String EditModeNames[] =
{   "multi_line_edit", "single_line_edit"
    } ;
static String IconAttachmentNames[] =
{   "attach_north_west", "attach_north", "attach_north_east", "attach_east",
    "attach_south_east", "attach_south", "attach_south_west", "attach_west",
    "attach_center", "attach_hot"
    } ;
static String ListSizePolicyNames[] =
{   "variable", "constant", "resize_if_possible"
    } ;
static String MultiClickNames[] =
{   "multiclick_discard", "multiclick_keep"
    } ;
static String NavigationTypeNames[] =
{   "none", "tab_group", "sticky_tab_group", "exclusive_tab_group"
    } ;
static String ProcessingDirectionNames[] =
{   "max_on_top", "max_on_bottom", "max_on_left", "max_on_right"
    } ;
static String ResizePolicyNames[] =
{   "resize_none", "resize_grow", "resize_any"
    } ;
static String RowColumnTypeNames[] =
{   "work_area", "menu_bar", "menu_pulldown", "menu_popup", "menu_option"
    } ;
static String ScrollBarDisplayPolicyNames[] =
{   "static", "as_needed"
    } ;
static String ScrollBarPlacementNames[] =
{   "bottom_right", "top_right", "bottom_left", "top_left"
    } ;
static String ScrollingPolicyNames[] =
{   "automatic", "application_defined"
    } ;
static String SelectionPolicyNames[] =
{   "single_select", "multiple_select", "extended_select", "browse_select"
    } ;
static String SelectionTypeNames[] =
{   "dialog_work_area", "dialog_prompt", "dialog_selection", "dialog_command",
    "dialog_file_selection"
    } ;
static String SeparatorTypeNames[] = 
{   "no_line", "single_line", "double_line", "single_dashed_line",
    "double_dashed_line", "shadow_etched_in", "shadow_etched_out",
    "shadow_etched_in_dash", "shadow_etched_out_dash"
    } ;
static String StringDirectionNames[] =
{   "string_direction_l_to_r", "string_direction_r_to_l"
    } ;
static String TearOffModelNames[] =
{   "tear_off_enabled", "tear_off_disabled"
    } ;
static String UnitTypeNames[] =
{   "pixels", "100th_millimeters", "1000th_inches", "100th_points",
    "100th_font_units"
    } ;
static String UnpostBehaviorNames[] =
{   "unpost", "unpost_and_replay"
    } ;
static String VerticalAlignmentNames[] =
{   "alignment_baseline_top", "alignment_center", "alignment_baseline_bottom",
    "alignment_contents_top", "alignment_contents_bottom"
    } ;
static String VisualPolicyNames[] =
{   "variable", "constant"
    } ;


#define NUM_NAMES( list )        (sizeof( list) / sizeof( String))

static XmRepTypeRec _XmStandardRepTypes[] =
{   
    {   XtImmediate, XmRID_ALIGNMENT,
        NUM_NAMES( AlignmentNames), FALSE, 0,
        XmRAlignment, AlignmentNames
        },
    {   XtImmediate, XmRID_ANIMATION_STYLE,
        NUM_NAMES( AnimationStyleNames), FALSE, 0,
        XmRAnimationStyle, AnimationStyleNames
        },
    {   XtImmediate, XmRID_ARROW_DIRECTION,
        NUM_NAMES( ArrowDirectionNames), FALSE, 0,
        XmRArrowDirection, ArrowDirectionNames
        },
    {   XtImmediate, XmRID_ATTACHMENT,
        NUM_NAMES( AttachmentNames), FALSE, 0,
        XmRAttachment, AttachmentNames
        },
    {   XtImmediate, XmRID_AUDIBLE_WARNING,
        NUM_NAMES( AudibleWarningNames), FALSE, 0,
        XmRAudibleWarning, AudibleWarningNames
        },
    {   XtImmediate, XmRID_BLEND_MODEL,
        NUM_NAMES( BlendModelNames), FALSE, 0,
        XmRBlendModel, BlendModelNames
        },
    {   XtImmediate, XmRID_CHILD_HORIZONTAL_ALIGNMENT,
        NUM_NAMES( ChildHorizontalAlignmentNames), FALSE, 0,
        XmRChildHorizontalAlignment, ChildHorizontalAlignmentNames
        },
    {   XtImmediate, XmRID_CHILD_PLACEMENT,
        NUM_NAMES( ChildPlacementNames), FALSE, 0,
        XmRChildPlacement, ChildPlacementNames
        },
    {   XtImmediate, XmRID_CHILD_TYPE,
        NUM_NAMES( ChildTypeNames), FALSE, 0,
        XmRChildType, ChildTypeNames
        },
    {   XtImmediate, XmRID_CHILD_VERTICAL_ALIGNMENT,
        NUM_NAMES( ChildVerticalAlignmentNames), FALSE, 0,
        XmRChildVerticalAlignment, ChildVerticalAlignmentNames
        },
    {   XtImmediate, XmRID_COMMAND_WINDOW_LOCATION,
        NUM_NAMES( CommandWindowLocationNames), FALSE, 0,
        XmRCommandWindowLocation, CommandWindowLocationNames
        },
    {   XtImmediate, XmRID_DIALOG_TYPE,
        NUM_NAMES( DialogTypeNames), FALSE, 0,
        XmRDialogType, DialogTypeNames
        },
    {   XtImmediate, XmRID_DRAG_INITIATOR_PROTOCOL_STYLE,
        NUM_NAMES( DragInitiatorProtocolStyleNames), FALSE, 0,
        XmRDragInitiatorProtocolStyle, DragInitiatorProtocolStyleNames
        },
    {   XtImmediate, XmRID_DRAG_RECEIVER_PROTOCOL_STYLE,
        NUM_NAMES( DragReceiverProtocolStyleNames), FALSE, 0,
        XmRDragReceiverProtocolStyle, DragReceiverProtocolStyleNames
        },
    {   XtImmediate, XmRID_DROP_SITE_ACTIVITY,
        NUM_NAMES( DropSiteActivityNames), FALSE, 0,
        XmRDropSiteActivity, DropSiteActivityNames
        },
    {   XtImmediate, XmRID_DROP_SITE_TYPE,
        NUM_NAMES( DropSiteTypeNames), FALSE, 0,
        XmRDropSiteType, DropSiteTypeNames
        },
    {   XtImmediate, XmRID_EDIT_MODE,
        NUM_NAMES( EditModeNames), FALSE, 0,
        XmREditMode, EditModeNames
        },
    {   XtImmediate, XmRID_ICON_ATTACHMENT,
        NUM_NAMES( IconAttachmentNames), FALSE, 0,
        XmRIconAttachment, IconAttachmentNames
        },
    {   XtImmediate, XmRID_LIST_SIZE_POLICY,
        NUM_NAMES( ListSizePolicyNames), FALSE, 0,
        XmRListSizePolicy, ListSizePolicyNames
        },
    {   XtImmediate, XmRID_MULTI_CLICK,
        NUM_NAMES( MultiClickNames), FALSE, 0,
        XmRMultiClick, MultiClickNames
        },
    {   XtImmediate, XmRID_NAVIGATION_TYPE,
        NUM_NAMES( NavigationTypeNames), FALSE, 0,
        XmRNavigationType, NavigationTypeNames
        },
    {   XtImmediate, XmRID_PROCESSING_DIRECTION,
        NUM_NAMES( ProcessingDirectionNames), FALSE, 0,
        XmRProcessingDirection, ProcessingDirectionNames
        },
    {   XtImmediate, XmRID_RESIZE_POLICY,
        NUM_NAMES( ResizePolicyNames), FALSE, 0,
        XmRResizePolicy, ResizePolicyNames
        },
    {   XtImmediate, XmRID_ROW_COLUMN_TYPE,
        NUM_NAMES( RowColumnTypeNames), FALSE, 0,
        XmRRowColumnType, RowColumnTypeNames
        },
    {   XtImmediate, XmRID_SCROLL_BAR_DISPLAY_POLICY,
        NUM_NAMES( ScrollBarDisplayPolicyNames), FALSE, 0,
        XmRScrollBarDisplayPolicy, ScrollBarDisplayPolicyNames
        },
    {   XtImmediate, XmRID_SCROLL_BAR_PLACEMENT,
        NUM_NAMES( ScrollBarPlacementNames), FALSE, 0,
        XmRScrollBarPlacement, ScrollBarPlacementNames
        },
    {   XtImmediate, XmRID_SCROLLING_POLICY,
        NUM_NAMES( ScrollingPolicyNames), FALSE, 0,
        XmRScrollingPolicy, ScrollingPolicyNames
        },
    {   XtImmediate, XmRID_SELECTION_POLICY,
        NUM_NAMES( SelectionPolicyNames), FALSE, 0,
        XmRSelectionPolicy, SelectionPolicyNames
        },
    {   XtImmediate, XmRID_SELECTION_TYPE,
        NUM_NAMES( SelectionTypeNames), FALSE, 0,
        XmRSelectionType, SelectionTypeNames
        },
    {   XtImmediate, XmRID_SEPARATOR_TYPE,
        NUM_NAMES( SeparatorTypeNames), FALSE, 0,
        XmRSeparatorType, SeparatorTypeNames
        },
    {   XtImmediate, XmRID_STRING_DIRECTION,
        NUM_NAMES( StringDirectionNames), FALSE, 0,
        XmRStringDirection, StringDirectionNames
        },
    {   XtImmediate, XmRID_TEAR_OFF_MODEL,
        NUM_NAMES( TearOffModelNames), FALSE, 0,
        XmRTearOffModel, TearOffModelNames
        },
    {   XtImmediate, XmRID_UNIT_TYPE,
        NUM_NAMES( UnitTypeNames), FALSE, 0,
        XmRUnitType, UnitTypeNames
        },
    {   XtImmediate, XmRID_UNPOST_BEHAVIOR,
        NUM_NAMES( UnpostBehaviorNames), FALSE, 0,
        XmRUnpostBehavior, UnpostBehaviorNames
        },
    {   XtImmediate, XmRID_VERTICAL_ALIGNMENT,
        NUM_NAMES( VerticalAlignmentNames), FALSE, 0,
        XmRVerticalAlignment, VerticalAlignmentNames
        },
    {   XtImmediate, XmRID_VISUAL_POLICY,
        NUM_NAMES( VisualPolicyNames), FALSE, 0,
        XmRVisualPolicy, VisualPolicyNames
        }
    } ;

static String DefaultButtonTypeNames[] =
{   "dialog_none", "dialog_cancel_button", "dialog_ok_button",
    "dialog_help_button"
    } ;
static unsigned char DefaultButtonTypeMap[] = 
{   XmDIALOG_NONE, XmDIALOG_CANCEL_BUTTON, XmDIALOG_OK_BUTTON,
    XmDIALOG_HELP_BUTTON
    } ;
static String DialogStyleNames[] =
{   "dialog_modeless", "dialog_work_area", "dialog_primary_application_modal",
    "dialog_application_modal", "dialog_full_application_modal",
    "dialog_system_modal"
    } ;
static unsigned char DialogStyleMap[] =
{   XmDIALOG_MODELESS, XmDIALOG_WORK_AREA, XmDIALOG_PRIMARY_APPLICATION_MODAL,
    XmDIALOG_APPLICATION_MODAL, XmDIALOG_FULL_APPLICATION_MODAL,
    XmDIALOG_SYSTEM_MODAL
    } ;
static String FileTypeMaskNames[] =
{   "file_directory", "file_regular", "file_any_type"
    } ;
static unsigned char FileTypeMaskMap[] = 
{   XmFILE_DIRECTORY, XmFILE_REGULAR, XmFILE_ANY_TYPE
    } ;
static String IndicatorTypeNames[] =
{   "n_of_many", "one_of_many"
    } ;
static unsigned char IndicatorTypeMap[] = 
{   XmN_OF_MANY, XmONE_OF_MANY
    } ;
static String LabelTypeNames[] =
{   "pixmap", "string"
    } ;
static unsigned char LabelTypeMap[] = 
{   XmPIXMAP, XmSTRING
    } ;
static String OrientationNames[] =
{   "vertical", "horizontal"
    } ;
static unsigned char OrientationMap[] = 
{   XmVERTICAL, XmHORIZONTAL
    } ;
static String PackingNames[] =
{   "pack_tight", "pack_column", "pack_none"
    } ;
static unsigned char PackingMap[] =
{   XmPACK_TIGHT, XmPACK_COLUMN, XmPACK_NONE
    } ;
static String ShadowTypeNames[] =
{   "shadow_etched_in", "shadow_etched_out", "shadow_in", "shadow_out"
    } ;
static unsigned char ShadowTypeMap[] = 
{   XmSHADOW_ETCHED_IN, XmSHADOW_ETCHED_OUT, XmSHADOW_IN, XmSHADOW_OUT
    } ;
static String WhichButtonNames[] =
{   "button1", "1", "button2", "2", "button3", "3", "button4", "4", 
    "button5", "5"
    } ;
static unsigned char WhichButtonMap[] = 
{   Button1, Button1, Button2, Button2, Button3, Button3, Button4, Button4,
    Button5, Button5
    } ;

static XmRepTypeMappedRec _XmStandardMappedRepTypes[] = 
{   
    {   XtImmediate, XmRID_DEFAULT_BUTTON_TYPE,
        NUM_NAMES( DefaultButtonTypeNames), FALSE, 0,
        XmRDefaultButtonType, DefaultButtonTypeNames, DefaultButtonTypeMap
        },
    {   XtImmediate, XmRID_DIALOG_STYLE,
        NUM_NAMES( DialogStyleNames), FALSE, 0,
        XmRDialogStyle, DialogStyleNames, DialogStyleMap
        },
    {   XtImmediate, XmRID_FILE_TYPE_MASK,
        NUM_NAMES( FileTypeMaskNames), FALSE, 0,
        XmRFileTypeMask, FileTypeMaskNames, FileTypeMaskMap
        },
    {   XtImmediate, XmRID_INDICATOR_TYPE,
        NUM_NAMES( IndicatorTypeNames), FALSE, 0,
        XmRIndicatorType, IndicatorTypeNames, IndicatorTypeMap
        },
    {   XtImmediate, XmRID_LABEL_TYPE,
        NUM_NAMES( LabelTypeNames), FALSE, 0,
        XmRLabelType, LabelTypeNames, LabelTypeMap
        },
    {   XtImmediate, XmRID_ORIENTATION,
        NUM_NAMES( OrientationNames), FALSE, 0,
        XmROrientation, OrientationNames, OrientationMap
        },
    {   XtImmediate, XmRID_PACKING,
        NUM_NAMES( PackingNames), FALSE, 0,
        XmRPacking, PackingNames, PackingMap
        },
    {   XtImmediate, XmRID_SHADOW_TYPE,
        NUM_NAMES( ShadowTypeNames), FALSE, 0,
        XmRShadowType, ShadowTypeNames, ShadowTypeMap
        },
    {   XtImmediate, XmRID_WHICH_BUTTON,
        NUM_NAMES( WhichButtonNames), FALSE, 0,
        XmRWhichButton, WhichButtonNames, WhichButtonMap
        }
    } ;


static XmRepTypeListDataRec RepTypeLists[] =
{   
    {   _XmStandardRepTypes,
        (sizeof( _XmStandardRepTypes) / sizeof( XmRepTypeRec)),
        sizeof( XmRepTypeRec),
        XmREP_TYPE_STD_TAG
        },
    {   (XmRepType) _XmStandardMappedRepTypes,
        (sizeof( _XmStandardMappedRepTypes) / sizeof( XmRepTypeMappedRec)),
        sizeof( XmRepTypeMappedRec),
        XmREP_TYPE_STD_MAP_TAG
        },
    {   NULL,
        0,
        sizeof( XmRepTypeRec),
        XmREP_TYPE_RT_TAG
        },
    {   NULL,
        0,
        sizeof( XmRepTypeMappedRec),
        XmREP_TYPE_RT_MAP_TAG
        }
    } ;

#define NUM_LISTS       (sizeof( RepTypeLists) / sizeof( XmRepTypeListDataRec))
#define NUM_SORTED_LISTS        2
#define RUN_TIME_LIST           2
#define RUN_TIME_MAPPED_LIST    3



static String *
#ifdef _NO_PROTO
CopyStringArray( StrArray, NumEntries, NullTerminate, UppercaseFormat)
        String *StrArray ;
        unsigned char NumEntries ;
        Boolean NullTerminate ;
        Boolean UppercaseFormat ;
#else
CopyStringArray(
        String *StrArray,
#if NeedWidePrototypes
        unsigned int NumEntries,
        int NullTerminate,
        int UppercaseFormat)
#else
        unsigned char NumEntries,
        Boolean NullTerminate,
        Boolean UppercaseFormat)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{   
            unsigned int AllocSize = 0 ;
            unsigned int Index ;
            char **TmpStr ;
            char *NextString ;
            char *SrcPtr ;

    Index = 0 ;
    while(    Index < NumEntries    )
    {   
        AllocSize += strlen( StrArray[Index++]) + 1 ;
        } 
    AllocSize += NumEntries * sizeof( String) ;

    if(    NullTerminate    )
    {   AllocSize += sizeof( String *) ;
        } 
    if(    UppercaseFormat    )
    {   AllocSize += NumEntries << 1 ;
        } 
    TmpStr = (char **) XtMalloc( AllocSize) ;

    NextString = (char *) (TmpStr + NumEntries) ;

    if(    NullTerminate    )
    {   NextString += sizeof( String *) ;
        } 
    Index = 0 ;
    if(    UppercaseFormat    )
    {   
        while(    Index < NumEntries    )
        {   
            SrcPtr = StrArray[Index] ;
            TmpStr[Index] = NextString ;
            *NextString++ = 'X' ;
            *NextString++ = 'm' ;
            while ((*NextString++ =
		    ((unsigned char)islower((unsigned char)*SrcPtr)) ?
		    toupper( (unsigned char)*SrcPtr++) : *SrcPtr++) != '\0')
		   { } 
            ++Index ;
            } 
        } 
    else
    {   while(    Index < NumEntries    )
        {   
            SrcPtr = StrArray[Index] ;
            TmpStr[Index] = NextString ;
            while(    (*NextString++ = *SrcPtr++) != '\0'    ){ } 
            ++Index ;
            } 
        } 
    if(    NullTerminate    )
    {   TmpStr[Index] = NULL ;
        } 
    return( TmpStr) ;
    } 

static Boolean
#ifdef _NO_PROTO
ValuesConsecutive( values, num_values)
        unsigned char *values ;
        unsigned char num_values ;
#else
ValuesConsecutive(
        unsigned char *values,
#if NeedWidePrototypes
        unsigned int num_values)
#else
        unsigned char num_values)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{   
    if(    values    )
    {   while(    num_values--    )
        {   if(    num_values != values[num_values]    )
            {   return( FALSE) ;
                } 
            } 
        } 
    return( TRUE) ;
    } 

XmRepTypeId
#ifdef _NO_PROTO
XmRepTypeRegister( rep_type, value_names, values, num_values)
        String rep_type ;
        String *value_names ;
        unsigned char *values ;
        unsigned char num_values ;
#else
XmRepTypeRegister(
        String rep_type,
        String *value_names,
        unsigned char *values,
#if NeedWidePrototypes
        unsigned int num_values)
#else
        unsigned char num_values)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{     
        XmRepType NewRecord ;

    if(    !num_values    )
    {   return( XmREP_TYPE_INVALID) ;
        } 

    if(    ValuesConsecutive( values, num_values)    )
    {   
            unsigned short NumRecords
                                    = RepTypeLists[RUN_TIME_LIST].num_records ;

        RepTypeLists[RUN_TIME_LIST].list = (XmRepType) XtRealloc( 
                              (char *) RepTypeLists[RUN_TIME_LIST].list,
                                  (sizeof( XmRepTypeRec) * (NumRecords + 1))) ;
        ++RepTypeLists[RUN_TIME_LIST].num_records ;

        NewRecord = &RepTypeLists[RUN_TIME_LIST].list[NumRecords] ;
        NewRecord->id = NumRecords | RepTypeLists[RUN_TIME_LIST].tag ;
        }
    else
    {       unsigned short NumRecords
                             = RepTypeLists[RUN_TIME_MAPPED_LIST].num_records ;
            XmRepTypeMappedRec *NewMapRecord ;
            unsigned char *ValArrayPtr ;
            unsigned char Index ;

        RepTypeLists[RUN_TIME_MAPPED_LIST].list = (XmRepType) XtRealloc( 
                              (char *) RepTypeLists[RUN_TIME_MAPPED_LIST].list,
                            (sizeof( XmRepTypeMappedRec) * (NumRecords + 1))) ;
        ++RepTypeLists[RUN_TIME_MAPPED_LIST].num_records ;

        NewMapRecord = (XmRepTypeMappedRec *) 
                         &RepTypeLists[RUN_TIME_MAPPED_LIST].list[NumRecords] ;
        NewMapRecord->id = NumRecords | RepTypeLists[RUN_TIME_MAPPED_LIST].tag ;
        ValArrayPtr = (unsigned char *) XtMalloc( sizeof( unsigned char)
                                                                * num_values) ;
        NewMapRecord->map = ValArrayPtr ;
        Index = 0 ;
        while(    Index++ < num_values    )
        {   *ValArrayPtr++ = *values++ ;
            } 
        NewRecord = (XmRepType) NewMapRecord ;
        } 
    NewRecord->mode_for_converter = XtImmediate ;
    NewRecord->num_values = num_values ;
    NewRecord->rep_type_name = strcpy( XtMalloc( strlen( rep_type) + 1),
                                                                    rep_type) ;
    NewRecord->value_names = CopyStringArray( value_names, num_values,
                                                                FALSE, FALSE) ;
    XtSetTypeConverter( XmRString, NewRecord->rep_type_name, ConvertRepType,
                          (XtConvertArgList) NewRecord, 1, XtCacheNone, NULL) ;
    NewRecord->reverse_installed = FALSE ;

    return( NewRecord->id) ;
    }

static XmRepType
#ifdef _NO_PROTO
GetRepTypeRecord( rep_type_id )
     XmRepTypeId rep_type_id ;
#else
GetRepTypeRecord(
#if NeedWidePrototypes
     int rep_type_id)
#else
     XmRepTypeId rep_type_id)
#endif
#endif /* _NO_PROTO */
{   
    if(    rep_type_id != XmREP_TYPE_INVALID    )
    {   
        if(    XmREP_TYPE_STD( rep_type_id)    )
        {   return( &_XmStandardRepTypes[XmREP_TYPE_OFFSET( rep_type_id)]) ;
            } 
        else
        {   if(    XmREP_TYPE_STD_MAP( rep_type_id)    )
            {   return( (XmRepType) &_XmStandardMappedRepTypes[
                                           XmREP_TYPE_OFFSET( rep_type_id)]) ;
                }
            else
            {   if(    XmREP_TYPE_RT( rep_type_id)    )
                {   return( &RepTypeLists[RUN_TIME_LIST].list[
                                           XmREP_TYPE_OFFSET( rep_type_id)]) ;
                    }
                else
                {   if(    XmREP_TYPE_RT_MAP( rep_type_id)    )
                    {   return( &RepTypeLists[RUN_TIME_MAPPED_LIST].list[
                                           XmREP_TYPE_OFFSET( rep_type_id)]) ;
                        }
                    } 
                } 
            } 
        } 
    return( NULL) ;
    } 

void
#ifdef _NO_PROTO
XmRepTypeAddReverse( rep_type_id )
     XmRepTypeId rep_type_id ;
#else
XmRepTypeAddReverse(
#if NeedWidePrototypes
     int rep_type_id)
#else
     XmRepTypeId rep_type_id)
#endif
#endif /* _NO_PROTO */
{     
        XmRepType Record = GetRepTypeRecord( rep_type_id) ;

    if(    Record  &&  !Record->reverse_installed    )
    {   
        XtSetTypeConverter( Record->rep_type_name, XmRString,
                              ReverseConvertRepType, (XtConvertArgList) Record,
                                                        1, XtCacheNone, NULL) ;
        Record->reverse_installed = TRUE ;
        } 
    return ;
    }

Boolean
#ifdef _NO_PROTO
XmRepTypeValidValue( rep_type_id, test_value, enable_default_warning )
     XmRepTypeId rep_type_id ;
     unsigned char test_value ;
     Widget enable_default_warning ;
#else
XmRepTypeValidValue(
#if NeedWidePrototypes
     int rep_type_id,
     unsigned int test_value,
#else
     XmRepTypeId rep_type_id,
     unsigned char test_value,
#endif
     Widget enable_default_warning)
#endif /* _NO_PROTO */
{
        XmRepType Record = GetRepTypeRecord( rep_type_id) ;

    if(    !Record    )
    {   if(    enable_default_warning    )

#ifdef I18N_MSG
        {   _XmWarning( enable_default_warning,
		catgets(Xm_catd,MS_RepType,MSG_REP_1, MESSAGE1)) ;
#else
        {   _XmWarning( enable_default_warning, MESSAGE1) ;
#endif

            } 
        } 
    else
    {   if(    XmREP_TYPE_MAPPED( rep_type_id)    )
        {   
            unsigned int Index = 0 ;
            unsigned int NumValues = Record->num_values ;
            unsigned char *ValidValues = ((XmRepTypeMappedRec *) Record)->map ;

            while(    Index < NumValues    )
            {   if(    ValidValues[Index] == test_value    )
                {   return( TRUE) ;
                    } 
                ++Index ;
                } 
            } 
        else
        {   if(    test_value < Record->num_values    )
            {   return( TRUE) ;
                } 
            } 
        if(    enable_default_warning    )
        {   
                char msg[256] ;


#ifdef I18N_MSG
            sprintf( msg, catgets(Xm_catd,MS_RepType,MSG_REP_2,
			MESSAGE2),
			test_value, Record->rep_type_name) ;
#else
            sprintf( msg, MESSAGE2, test_value,
                                                       Record->rep_type_name) ;
#endif

            _XmWarning( enable_default_warning, msg) ;
            } 
        }
    return FALSE ;
    }

static unsigned int
#ifdef _NO_PROTO
GetByteDataSize( Record )
        XmRepType Record ;
#else
GetByteDataSize(
        XmRepType Record)
#endif /* _NO_PROTO */
{   
            String * valNames ;
            unsigned int numVal ;
            register unsigned int Index ;
            unsigned int ByteDataSize ;

    ByteDataSize = strlen( Record->rep_type_name) + 1 ; /* For rep type name.*/

    valNames = Record->value_names ;
    numVal = Record->num_values ;
    Index = 0 ;
    while(    Index < numVal    )
    {   
        ByteDataSize += strlen( valNames[Index++]) + 1 ;/* For value names.*/
        } 
    ByteDataSize += numVal ;                        /* For array of values. */

    return( ByteDataSize) ;
    } 

static void
#ifdef _NO_PROTO
CopyRecord( Record, OutputEntry, PtrDataArea, ByteDataArea)
        XmRepType Record ;
        XmRepTypeEntry OutputEntry ;
        XtPointer *PtrDataArea ;
        XtPointer *ByteDataArea ;
#else
CopyRecord(
        XmRepType Record,
        XmRepTypeEntry OutputEntry,
        XtPointer *PtrDataArea,
        XtPointer *ByteDataArea)
#endif /* _NO_PROTO */
{   
            register String *PtrDataAreaPtr = (String *) *PtrDataArea ;
            register char *ByteDataAreaPtr = (char *) *ByteDataArea ;
            unsigned int NumValues = Record->num_values ;
            register char *NamePtr ;
            unsigned int ValueIndex ;

    OutputEntry->num_values = NumValues ;
    OutputEntry->reverse_installed = Record->reverse_installed ;
    OutputEntry->rep_type_id = Record->id ;

    OutputEntry->rep_type_name = ByteDataAreaPtr ;
    NamePtr = Record->rep_type_name ;
    while(    (*ByteDataAreaPtr++ = *NamePtr++) != '\0'    ){ } 

    OutputEntry->value_names = PtrDataAreaPtr ;
    ValueIndex = 0 ;
    while(    ValueIndex < NumValues    )
    {   
        *PtrDataAreaPtr++ = ByteDataAreaPtr ;
        NamePtr = Record->value_names[ValueIndex] ;
        while(    (*ByteDataAreaPtr++ = *NamePtr++) != '\0'    ){ }
        ++ValueIndex ;
        } 
    ValueIndex = 0 ;
    OutputEntry->values = (unsigned char *) ByteDataAreaPtr ;

    if(    XmREP_TYPE_MAPPED( Record->id)    )
    {   
        while(    ValueIndex < NumValues    )
        {   *((unsigned char *) ByteDataAreaPtr++) = ((XmRepTypeMappedRec *)
                                                   Record)->map[ValueIndex++] ;
            } 
        } 
    else
    {   while(    ValueIndex < NumValues    )
        {   *((unsigned char *) ByteDataAreaPtr++) = (unsigned char)
                                                                 ValueIndex++ ;
            } 
        } 
    *PtrDataArea = (XtPointer) PtrDataAreaPtr ;
    *ByteDataArea = (XtPointer) ByteDataAreaPtr ;
    return ;
    } 

XmRepTypeList
#ifdef _NO_PROTO
XmRepTypeGetRegistered()
#else
XmRepTypeGetRegistered( void )
#endif /* _NO_PROTO */
{
    /* In order allow the user to free allocated memory with a simple
    *    call to XtFree, the data from the record lists are copied into
    *    a single block of memory.  This can create alignment issues
    *    on some architectures.
    *  To resolve alignment issues, the data is layed-out in the memory
    *    block as shown below.
    *
    *   returned    ____________________________________________________
    *   pointer -> |                                                    |
    *              |   Array of XmRepTypeEntryRec structures            |
    *              |____________________________________________________|
    *              |                                                    |
    *              |   Pointer-size data (arrays of pointers to strings)|
    *              |____________________________________________________|
    *              |                                                    |
    *              |   Byte-size data (arrays of characters and values) |
    *              |____________________________________________________|
    *
    *  The XmRepTypeGetRegistered routine fills the fields of the
    *    XmRepTypeEntryRec with immediate values and with pointers
    *    to the appropriate arrays.  The entry->values field is set
    *    to point to an array of values in the byte-size data area,
    *    while the entry->value_names field is set to point to an
    *    array of pointers in the pointer-size data area.  This array
    *    of pointers is then set to point to character arrays in the
    *    byte-size data area.
    *  Since the first field of the XmRepTypeEntryRec is a pointer,
    *    it can be assumed that the section of arrays of pointers
    *    can be located immediately following the array of structures
    *    without concern for alignment.  Byte-size data is assumed
    *    to have no alignment requirements.
    */

            unsigned int TotalEntries = 1 ; /* One extra for null terminator.*/
            unsigned int PtrDataSize = 0 ;
            unsigned int ByteDataSize = 0 ;
            XmRepTypeList OutputList ;
            XmRepTypeList ListPtr ;
            XtPointer PtrDataPtr ;
            XtPointer ByteDataPtr ;
            unsigned int ListIndex ;

    ListIndex = 0 ;
    while(    ListIndex < NUM_LISTS    )
    {   
            XtPointer RecordPtr = (XtPointer) RepTypeLists[ListIndex].list ;
            unsigned int NumRecords = RepTypeLists[ListIndex].num_records ;
            unsigned int RecSize = RepTypeLists[ListIndex].rec_size ;
            unsigned int RecordIndex = 0 ;

        TotalEntries += NumRecords ;

        while(    RecordIndex < NumRecords    )
        {   
            PtrDataSize += ((XmRepType) RecordPtr)->num_values
                                                            * sizeof( String) ;
            ByteDataSize += GetByteDataSize( (XmRepType) RecordPtr) ;

            RecordPtr = (XtPointer) (((char *) RecordPtr) + RecSize) ;
            ++RecordIndex ;
            } 
        ++ListIndex ;
        } 
    OutputList = (XmRepTypeList) XtMalloc( PtrDataSize + ByteDataSize
                                + (TotalEntries * sizeof( XmRepTypeListRec))) ;
    ListPtr = OutputList ;
    PtrDataPtr = (XtPointer) (ListPtr + TotalEntries) ;
    ByteDataPtr = (XtPointer) (((char *) PtrDataPtr) + PtrDataSize) ;

    ListIndex = 0 ;
    while(    ListIndex < NUM_LISTS    )
    {   
            XtPointer RecordPtr = (XtPointer) RepTypeLists[ListIndex].list ;
            unsigned int NumRecords = RepTypeLists[ListIndex].num_records ;
            unsigned int RecSize = RepTypeLists[ListIndex].rec_size ;
            unsigned int RecordIndex = 0 ;

        while(    RecordIndex < NumRecords    )
        {   
            CopyRecord( (XmRepType) RecordPtr, ListPtr, &PtrDataPtr,
                                                                &ByteDataPtr) ;
            RecordPtr = (XtPointer) (((char *) RecordPtr) + RecSize) ;
            ++ListPtr ;
            ++RecordIndex ;
            } 
        ++ListIndex ;
        }
    ListPtr->rep_type_name = NULL ;

    return( OutputList) ;
    }

XmRepTypeEntry
#ifdef _NO_PROTO
XmRepTypeGetRecord( rep_type_id )
        XmRepTypeId rep_type_id ;
#else
XmRepTypeGetRecord(
#if NeedWidePrototypes
        int rep_type_id)
#else
        XmRepTypeId rep_type_id)
#endif
#endif /* _NO_PROTO */
{
            XmRepType Record = GetRepTypeRecord( rep_type_id) ;
            XmRepTypeEntry OutputRecord ;
            XtPointer PtrDataArea ;
            XtPointer ByteDataArea ;
            unsigned int PtrDataSize ;
            unsigned int ByteDataSize ;

    if(    Record    )
    {   
        PtrDataSize = Record->num_values * sizeof( String) ;
        ByteDataSize = GetByteDataSize( Record) ;

        OutputRecord = (XmRepTypeEntry) XtMalloc( sizeof( XmRepTypeEntryRec)
                                                + PtrDataSize + ByteDataSize) ;
        PtrDataArea = (XtPointer) (OutputRecord + 1) ;
        ByteDataArea = (XtPointer) (((char *) PtrDataArea) + PtrDataSize) ;

        CopyRecord( Record, OutputRecord, &PtrDataArea, &ByteDataArea) ;

        return( OutputRecord) ;
        } 
    return( NULL) ;
    }

XmRepTypeId
#ifdef _NO_PROTO
XmRepTypeGetId( rep_type)
        String rep_type ;
#else
XmRepTypeGetId(
        String rep_type)
#endif /* _NO_PROTO */
{
        int ListIndex ;
        XtPointer List ;
        int Index ;
        int NumInList ;
        unsigned short ListRecSize ;

    ListIndex = 0 ;
    while(    ListIndex < NUM_SORTED_LISTS    )
    {   
            int Lower = 0 ;
            int Upper = RepTypeLists[ListIndex].num_records - 1 ;
            int TestResult ;

        List = (XtPointer) RepTypeLists[ListIndex].list ;
        ListRecSize = RepTypeLists[ListIndex].rec_size ;

        while(    Upper >= Lower    )
        {   
            Index = ((Upper - Lower) >> 1) + Lower ;
            TestResult = strcmp( rep_type, 
                          ((XmRepType) (((char *) List) + ListRecSize * Index))
                                                             ->rep_type_name) ;
            if(    TestResult > 0    )
            {   Lower = Index + 1 ;
                } 
            else
            {   if(    TestResult < 0    )
                {   Upper = Index - 1 ;
                    } 
                else
                {   return( (XmRepTypeId) Index
                                               | RepTypeLists[ListIndex].tag) ;
                    } 
                }
            } 
        ++ListIndex ;
        }
    while(    ListIndex < NUM_LISTS    )
    {   
        List = (XtPointer) RepTypeLists[ListIndex].list ;
        NumInList = RepTypeLists[ListIndex].num_records ;
        ListRecSize = RepTypeLists[ListIndex].rec_size ;

        Index = 0 ;
        while(    Index < NumInList    )
        {   
            if(    !strcmp( rep_type, ((XmRepType) List)->rep_type_name)    )
            {   
                return( (XmRepTypeId) Index | RepTypeLists[ListIndex].tag) ;
                } 
            List = (XtPointer) (((char *) List) + ListRecSize) ;
            ++Index ;
            } 
        ++ListIndex ;
        } 

    return( XmREP_TYPE_INVALID) ;
    } 

String *
#ifdef _NO_PROTO
XmRepTypeGetNameList( rep_type_id, use_uppercase_format )
        XmRepTypeId rep_type_id ;
        Boolean use_uppercase_format ;
#else
XmRepTypeGetNameList(
#if NeedWidePrototypes
        int rep_type_id,
        int use_uppercase_format)
#else
        XmRepTypeId rep_type_id,
        Boolean use_uppercase_format)
#endif /* NeedWidePrototypes */
#endif
{
        XmRepType Record = GetRepTypeRecord( rep_type_id) ;

    if(    Record    )
    {   return( CopyStringArray( Record->value_names, Record->num_values,
                                                 TRUE, use_uppercase_format)) ;
        } 
    return( NULL) ;
    }

typedef struct { XmRepTypeId id ; } XmRepTypeIdRec ;

static Boolean
#ifdef _NO_PROTO
ConvertRepType( disp, args, n_args, from, to, converter_data )
        Display *disp ;
        XrmValue *args ;
        Cardinal *n_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
ConvertRepType(
        Display *disp,
        XrmValue *args,
        Cardinal *n_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{   

    char *in_str = (char *) (from->addr) ;
    XmRepTypeId RepTypeID = ((XmRepTypeIdRec *) args[0].addr)->id ;
    XmRepType Record = GetRepTypeRecord( RepTypeID) ;
    unsigned int NumValues = Record->num_values ;
    unsigned int Index = 0 ;
    static unsigned int EditModeBuffer ;
    static int WhichButtonBuffer ;
    char first, second;

 /*
  * Fix for 5330 - To ensure OS compatibility, always check a character with
  *                isupper before converting it with tolower.
  */
    if (isupper((unsigned char)in_str[0]))
        first = tolower((unsigned char)in_str[0]);
    else
        first = in_str[0];
    if (isupper((unsigned char)in_str[1]))
        second = tolower((unsigned char)in_str[1]);
    else
        second = in_str[1];
 
    if(    (first == 'x')  &&  (second == 'm')    )
    {   in_str += 2 ;
        } 
    while(    Index < NumValues    )
    {   
        if(    _XmStringsAreEqual( in_str, Record->value_names[Index])    )
        {   
            unsigned char OutValue = XmREP_TYPE_MAPPED( RepTypeID)
                                ? (((XmRepTypeMappedRec *) Record)->map[Index])
                                                    : ((unsigned char) Index) ;
            if(    (RepTypeID == XmRID_EDIT_MODE)
                || (RepTypeID == XmRID_WHICH_BUTTON)    )
            {   
                /* Assuming sizeof( unsigned int) == sizeof( int) */

                if(    to->addr    )
                {   
                    if(    to->size < sizeof( unsigned int)    )
                    {   
                        /* Insufficient space, so set needed size and return.*/
                        to->size = sizeof( unsigned int) ;
                        return( FALSE) ;
                        } 
                    }
                else
                {   to->addr = (RepTypeID == XmRID_EDIT_MODE)
                                        ? ((XPointer) &EditModeBuffer)
                                           : ((XPointer) &WhichButtonBuffer) ;
                    } 
                to->size = sizeof( unsigned int);
                *((unsigned int *) to->addr) = (unsigned int) OutValue ;
                } 
            else
            {   if(    to->addr    )
                {   
                    if(    to->size < sizeof( unsigned char)    )
                    {   
                        /* Insufficient space, so set needed size and return.*/
                        to->size = sizeof( unsigned char) ;
                        return( FALSE) ;
                        } 
                    }
                else
                {   to->addr = (XPointer) &Record->conversion_buffer ;
                    } 
                to->size = sizeof( unsigned char);
                *((unsigned char *) to->addr) = OutValue ;
                } 
            return( TRUE) ;
            }
        ++Index ;
        } 
    XtDisplayStringConversionWarning( disp, in_str, Record->rep_type_name) ;

    return( FALSE) ;
    }

static Boolean
#ifdef _NO_PROTO
ReverseConvertRepType( disp, args, n_args, from, to, converter_data )
        Display *disp ;
        XrmValue *args ;
        Cardinal *n_args ;
        XrmValue *from ;
        XrmValue *to ;
        XtPointer *converter_data ;
#else
ReverseConvertRepType(
        Display *disp,
        XrmValue *args,
        Cardinal *n_args,
        XrmValue *from,
        XrmValue *to,
        XtPointer *converter_data)
#endif /* _NO_PROTO */
{   

    XmRepTypeId RepTypeID = ((XmRepTypeIdRec *) args[0].addr)->id ;
    unsigned char in_value = ((RepTypeID == XmRID_EDIT_MODE)
                                          || (RepTypeID == XmRID_WHICH_BUTTON))
                           ? ((unsigned char) *((unsigned int *) (from->addr)))
                                          : *((unsigned char *) (from->addr)) ;
    XmRepType Record = GetRepTypeRecord( RepTypeID) ;
    unsigned short NumValues = Record->num_values ;
    char **OutValue = NULL ;
    char *params[2];
    Cardinal num_params = 2;


    if(    XmREP_TYPE_MAPPED( RepTypeID)    )
    {   
        unsigned short Index = 0 ;

        while(    Index < NumValues    )
        {   
            if(    in_value == ((XmRepTypeMappedRec *) Record)->map[Index]    )
            {   
                OutValue = (char **) &Record->value_names[Index] ;
                break ;
                }
            ++Index ;
            } 
        } 
    else
    {   if(    in_value < NumValues    )
        {   
            OutValue = (char **) &Record->value_names[in_value] ;
            } 
        } 
    if(    OutValue    )
    {   
        if(    to->addr    )
        {   
            if(    to->size < sizeof( char *)    )
            {   
                to->size = sizeof( char *) ;
                return( FALSE) ;
                } 
            *((char **) to->addr) = *OutValue ;
            }
        else
        {   to->addr = (XPointer) OutValue ;
            } 
        to->size = sizeof( char *) ;

        return( TRUE) ;
        } 
    params[0] = Record->rep_type_name;
    params[1] = (char *) ((int) in_value);
    XtAppWarningMsg( XtDisplayToApplicationContext( disp), "conversionError",
		    Record->rep_type_name, "XtToolkitError", 
		    "Cannot convert %s value %d to type String",
		    params, &num_params);
    return( FALSE) ;
    }

void
#ifdef _NO_PROTO
_XmRepTypeInstallConverters()
#else
_XmRepTypeInstallConverters( void )
#endif /* _NO_PROTO */
{   
        int ListNum = 0 ;

    while(    ListNum < NUM_LISTS    )
    {   
        XmRepType List = RepTypeLists[ListNum].list ;
        int NumRecords = RepTypeLists[ListNum].num_records ;
        int RecSize = RepTypeLists[ListNum].rec_size ;
        int Index = 0 ;

        while(    Index < NumRecords    )
        {   
#if 1
/* Maybe in Motif 1.3, this line will be removed so as to
 * always install the tear-off model converter.
 */
            if(    List->id != XmRID_TEAR_OFF_MODEL    )
#endif
            {   XtSetTypeConverter( XmRString, List->rep_type_name,
                                    ConvertRepType, (XtConvertArgList) List, 1,
                                                           XtCacheNone, NULL) ;
                } 
            List = (XmRepType) (((char *) List) + RecSize) ;
            ++Index ;
            } 
        ++ListNum ;
        } 
    return ;
    } 

void
#ifdef _NO_PROTO
XmRepTypeInstallTearOffModelConverter()
#else
XmRepTypeInstallTearOffModelConverter( void )
#endif /* _NO_PROTO */
{
  /* XmRepTypeInstallTearOffModelConverter convenience function.
   * Provide a way for the application to easily dynamically install the
   * TearOffModel converter.
   */
  XtSetTypeConverter( XmRString, XmRTearOffModel, ConvertRepType,
               (XtConvertArgList) GetRepTypeRecord(XmRID_TEAR_OFF_MODEL), 1,
                      XtCacheNone, NULL) ;
}
