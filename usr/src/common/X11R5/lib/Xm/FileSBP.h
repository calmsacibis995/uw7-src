#pragma ident	"@(#)m1.2libs:Xm/FileSBP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmFSelectP_h
#define _XmFSelectP_h

#include <Xm/SelectioBP.h>
#include <Xm/FileSB.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Defines for use in allocation geometry matrix. */

#define XmFSB_MAX_WIDGETS_VERT   10 

/* Bit locations for the state_flags bit field.
*/
#define XmFS_NO_MATCH		(1 << 0)
#define XmFS_IN_FILE_SEARCH	(1 << 1)
#define XmFS_DIR_SEARCH_PROC    (1 << 2)

/* Constraint part record for FileSelectionBox widget */

typedef struct _XmFileSelectionBoxConstraintPart
{
   char unused;
} XmFileSelectionBoxConstraintPart, * XmFileSelectionBoxConstraint;

/*  New fields for the FileSelectionBox widget class record  */

typedef struct
{
    XtPointer           extension;      /* Pointer to extension record */
} XmFileSelectionBoxClassPart;


/* Full class record declaration */

typedef struct _XmFileSelectionBoxClassRec
{
   CoreClassPart       core_class;
   CompositeClassPart  composite_class;
   ConstraintClassPart constraint_class;
   XmManagerClassPart  manager_class;
   XmBulletinBoardClassPart    bulletin_board_class;
   XmSelectionBoxClassPart    selection_box_class;
   XmFileSelectionBoxClassPart    file_selection_box_class;
} XmFileSelectionBoxClassRec;

externalref XmFileSelectionBoxClassRec xmFileSelectionBoxClassRec;


/* New fields for the FileSelectionBox widget record */

typedef struct
{
    XmString        directory;        /* directory specification */
    XmString        pattern;          /* file search pattern */
    Widget          dir_list_label;   /* directory list Label */
    XmString        dir_list_label_string;/* directory list label text */
    Widget          dir_list;         /* directory List */
    XmString *      dir_list_items;   /* items in directory List */
    int             dir_list_item_count;/* number of items in directory List */
    int             dir_list_selected_item_position;
    Widget          filter_label;     /* file search filter label */
    XmString        filter_label_string;/* filter label text */
    Widget          filter_text;      /* filter text entry field */
    XmString        dir_mask;         /* string in filter text entry field */
    XmString        no_match_string;  /* string in list when no file match */
    XmQualifyProc   qualify_search_data_proc; /* directory and mask routine */
    XmSearchProc    dir_search_proc;  /* change directory routine */
    XmSearchProc    file_search_proc; /* file search routine */
    unsigned char   file_type_mask;   /* mask for type of files in file list */
    Boolean         list_updated;     /* flag to indicate file list update   */
    Boolean         directory_valid ; /* flag to indicate valid new directory*/
    unsigned char   state_flags ;     /* internal flags to indicate state.   */

} XmFileSelectionBoxPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XmFileSelectionBoxRec
{
    CorePart	            core;
    CompositePart           composite;
    ConstraintPart          constraint;
    XmManagerPart           manager;
    XmBulletinBoardPart     bulletin_board;
    XmSelectionBoxPart      selection_box;
    XmFileSelectionBoxPart  file_selection_box;
} XmFileSelectionBoxRec;


/* Access macros */

#define FS_Directory( w) \
                (((XmFileSelectionBoxWidget)(w))->file_selection_box.directory)
#define FS_DirMask( w) \
                 (((XmFileSelectionBoxWidget)(w))->file_selection_box.dir_mask)
#define FS_DirListLabel( w) \
           (((XmFileSelectionBoxWidget)(w))->file_selection_box.dir_list_label)
#define FS_DirListLabelString( w) \
    (((XmFileSelectionBoxWidget)(w))->file_selection_box.dir_list_label_string)
#define FS_DirList( w) \
                 (((XmFileSelectionBoxWidget)(w))->file_selection_box.dir_list)
#define FS_DirListItems( w) \
           (((XmFileSelectionBoxWidget)(w))->file_selection_box.dir_list_items)
#define FS_DirListItemCount( w) \
      (((XmFileSelectionBoxWidget)(w))->file_selection_box.dir_list_item_count)
#define FS_FilterLabel( w) \
             (((XmFileSelectionBoxWidget)(w))->file_selection_box.filter_label)
#define FS_FilterLabelString( w) \
      (((XmFileSelectionBoxWidget)(w))->file_selection_box.filter_label_string)
#define FS_FilterText( w) \
              (((XmFileSelectionBoxWidget)(w))->file_selection_box.filter_text)
#define FS_Pattern( w) \
                  (((XmFileSelectionBoxWidget)(w))->file_selection_box.pattern)
#define FS_NoMatchString( w) \
          (((XmFileSelectionBoxWidget)(w))->file_selection_box.no_match_string)
#define FS_QualifySearchDataProc( w) (((XmFileSelectionBoxWidget) \
                             (w))->file_selection_box.qualify_search_data_proc)
#define FS_DirSearchProc( w) \
          (((XmFileSelectionBoxWidget)(w))->file_selection_box.dir_search_proc)
#define FS_FileSearchProc( w) \
         (((XmFileSelectionBoxWidget)(w))->file_selection_box.file_search_proc)
#define FS_RealDefaultButton( w) \
      (((XmFileSelectionBoxWidget)(w))->file_selection_box.real_default_button)
#define FS_FileTypeMask( w) \
           (((XmFileSelectionBoxWidget)(w))->file_selection_box.file_type_mask)
#define FS_ListUpdated( w) \
             (((XmFileSelectionBoxWidget)(w))->file_selection_box.list_updated)
#define FS_DirectoryValid( w) \
          (((XmFileSelectionBoxWidget)(w))->file_selection_box.directory_valid)
#define FS_StateFlags( w) \
              (((XmFileSelectionBoxWidget)(w))->file_selection_box.state_flags)
#define FS_DirListSelectedItemPosition( w) (((XmFileSelectionBoxWidget) w) \
                          ->file_selection_box.dir_list_selected_item_position)


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmFSelectP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
