/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:Dtm.h	1.251.2.3"

#ifndef __Dtm_h_
#define __Dtm_h_

/******************************file*header********************************

    Description:
	This file is the header for the desktop manager.

    Organization:
	I.	INCLUDES
	II.	TYPEDEF's, STRUCT's and ENUM's
	III.	#define's for BIT-FIELDS/OPTIONS
	IV.	#define's for INTEGER VALUES
	V.	#define's for STRINGS
	VI.	MACROS
*/

#include <stdio.h>
#include <sys/stat.h>
#include <sys/fstyp.h>

#include <buffutil.h>
#include <DtI.h>
#include <dtutil.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/ListGizmo.h>
#include <MGizmo/InputGizmo.h>

#include "CListGizmo.h"
#include "help.h"


/******************************************************************************
	II.  TYPDEF's, STRUCT's and ENUM's
*/

typedef int (*PFI)();

/* Enumeration types for File choices.  Note: order is important since
   used in table-driven code.
*/
typedef enum {
    DM_NO_OP,
    DM_COPY,
    DM_MOVE,
    DM_DELETE,
    DM_HARDLINK,
    DM_SYMLINK,
    DM_COPY_DEL,
    DM_CONV_D2U,
    DM_CONV_U2D,
    DM_BEGIN,
    DM_MKDIR,
    DM_RMDIR,
    DM_OVERWRITE,
    DM_NMCHG,
    DM_ENDOVER,
    DM_RENAME		/* pseudo-op which gets re-mapped to DM_MOVE */
} DmFileOpType;

/* Enumeration types for View choices.  Note that the order of the constants
   here must match the order of the buttons in the View menu and submenus.  See
   dtmsgstrs for label definitions and the appropriate "create" source file for
   the order of the callbacks.
*/
/* View Sort menu items */
typedef enum {
    DM_BY_TYPE,
    DM_BY_NAME,
    DM_BY_SIZE,
    DM_BY_TIME,
    DM_BY_POSITION	/* (the default)  Not strictly a menu item. */
} DmViewSortType;

/* Enumeration types for Customized View */
typedef enum {
    DM_SHOW_CV,
    DM_RESET_CV,
    DM_CANCEL_CV,
    DM_HELP_CV,
} DmCustomView;


/* View Format menu items */
typedef enum {
    DM_ICONIC,
    DM_NAME,
    DM_LONG
} DmViewFormatType;

/* Tree options menu items */
typedef enum {
    DM_SHOW_SUBS,
    DM_HIDE_SUBS,
    DM_SHOW_ALL_SUBS,
    DM_TREE_OPTION_SEP1,
    DM_BEGIN_HERE,
    DM_BEGIN_MAIN,
    DM_BEGIN_OTHER,
    DM_TREE_OPTION_SEP2,
    DM_BEGIN_PARENT
} DmTreeOption;

typedef enum {
    DM_IM_OPEN,
    DM_IM_PROPERTIES,
    DM_IM_DELETE,
    DM_IM_SHOW_SUBS,
    DM_IM_HIDE_SUBS,
    DM_IM_SHOW_ALL_SUBS,
    DM_IM_BEGIN_HERE,
    DM_IM_BEGIN_MAIN,
    DM_IM_BEGIN_OTHER,
    DM_IM_BEGIN_PARENT
} DmTreeIMOption;

/* defines used in FileGizmo Prompts */
typedef enum {
    FilePromptTask,
    FilePromptCancel,
    FilePromptHelp
} FileChoiceMenuItemIndex;

/* Types of client_data for MenuItems.

   None of the following values can be zero, because, if zero, then the
   gizmo code will copy the container's client_data into the item.
   NOTE: The constants must be defined in this order. See the code.
*/
typedef enum {
    DM_B_ANY = 1,
    DM_B_ONE_OR_MORE,
    DM_B_ONE,
    DM_B_UNDO,
    DM_B_TREE_OPTS,
    DM_B_NON_TREE_VIEW,
    DM_B_LINK
} DmMenuItemCDType;


/* The first few (4) fields in DmFmodeKeyRec and DmFnameKeyRec must be
   the same.

   An array of DmFmodeKeyRec structures contain the information about
   internal file classes. The DTM uses this information to determine the
   file class of a file.
*/
typedef struct dmFmodeKeyRec {
    const char *name;			/* name of class */
    DtAttrs	attrs;
    DmFileType	ftype;			/* DTM file type */
    DmFclassPtr	fcp;			/* ptr to actual entry */
					/* End of position-dependent fields */
    mode_t	fmt;			/* FMT part of mode from stat() */
    mode_t	perm;			/* PERM part of mode from stat() */
    dev_t	rdev;			/* rdev from stat() */
    DmGlyphPtr	small_icon;		/* for folders and exececutables
					   in "Tree" and "Name" format */
					/* 'short' fields go next */
					/* 'byte' fields go last */
} DmFmodeKeyRec, *DmFmodeKeyPtr;

/* The list of file classes read from the file database is stored in a 
   list of DmFnameKeyRec structure. Each structure stores information 
   that DTM uses to qualify files that belong to the file class.

   DmFclassFileRec must have the same 7 fields as this structure.
*/
typedef struct dmFnameKeyRec *DmFnameKeyPtr;
typedef struct dmFnameKeyRec {
    char *		name;		/* class name */
    DtAttrs		attrs;
    DmFileType		ftype;		/* file type */
    DmFclassPtr		fcp;		/* ptr to actual entry */
    unsigned short	level;		/* file level */
    DmFnameKeyPtr	next;		/* ptr to the next entry */
    DmFnameKeyPtr	prev;		/* ptr to the prev entry */
					/* End of position-dependent fields */
    char *		re;		/* compiled regular expression */
    char *		lre;		/* compiled link regular expression */
    char *		lfpath;	/* resolved link file path */
	int			fstype;	/* file system type */
					/* 'short' fields go next */
					/* 'byte' fields go last */
} DmFnameKeyRec;

/* DmFclassFileRec structure contains info about each class file.
   Note:This structure is in the same link list as the DmFnameKeyRec
	structure. Thus, the first 6 fields must be the same. Check the
	DM_B_CLASSFILE bit in attrs to distinguish the two structures.
*/
typedef struct dmFclassFileRec *DmFclassFilePtr;
typedef struct dmFclassFileRec {
    char *		name;		/* user specified file name */
    DtAttrs		attrs;		/* encoded info */
    DmFileType		ftype;		/* file type */
    DmFclassFilePtr	next_file;	/* ptr to the next file */
    unsigned short	level;		/* file level */
    DmFnameKeyPtr	next;		/* ptr to the next entry */
    DmFnameKeyPtr	prev;		/* ptr to the prev entry */
					/* End of position-dependent fields */
    char *		fullpath;	/* fullpath of file */
    time_t		last_read_time;
					/* 'short' fields go next */
					/* 'byte' fields go last */
} DmFclassFileRec;

/*			*** FILE OPERATIONS ***				*/

/* reason to invoke client proc during file manipulation operation */
typedef enum {
    DM_INIT_VAL = 1,
    DM_DONE,
    DM_OPRBEGIN,
    DM_ERROR,
    DM_REPORT_PROGRESS,
    DM_OVRWRITE
} DmProcReason;

/* define for strndup originally in Xol/strutil.c */
#define strndup(str,cnt) Dt__strndup(str,cnt)

/* container properties */
#define FOLDERSIZE	"size"

/* used in dm_util.c:DmMakeFolderTitle and f_create.c:DmOpenFolderWindow() */
#define UUCP_RECEIVE_PATH		"/spool/uucppublic/receive/"
#define NETWARE_PATH			"/.NetWare"
#define DISK_A				"/Disk_A"
#define DISK_B				"/Disk_B"
#define CD_ROM				"/CD-ROM"

#define NETWARE_SERVER			"NetWare Server"
#define NETWARE_SERVER_LINK		"NetWare Server Link"
#define NETWARE_SERVER_CLASS(D)		(DmGetNetWareServerClass())

/* format used to display (localized) icon label */
#define ICON_LABEL_FORMAT	"%s (%s)"

/* file classification used by file manipulation routines */
#define DM_NO_FILE	0
#define DM_IS_DIR	1
#define	DM_IS_FILE	2
#define	DM_IS_SPECIAL	3
#define DM_IS_SYMLINK	4
#define DM_IS_DOSDIR	5
#define	DM_IS_DOSFILE	6
#define DM_IS_S5DIR	7
#define	DM_IS_S5FILE	8
#define	DM_IS_S5SPECIAL	9
#define DM_PS_DIR	10
#define DM_PS_S5DIR	11
#define DM_PS_DOSDIR	12

/* 
 * Bits used in source info in DmFileOpCDRec.
 * (The LSBs are used to store source type)
 */
#define SRC_B_SKIP_OVERWRITE	( 1 << 8 )
#define SRC_B_ERROR		( 1 << 9 )
#define SRC_B_NMCHG		( 1 << 10 )
#define SRC_B_IGNORE		( SRC_B_SKIP_OVERWRITE | SRC_B_ERROR )
#define SRC_TYPE_MASK		( SRC_B_SKIP_OVERWRITE - 1 )

/* 'attrs' bits for file operation.  (OVERWRITE is also used as 'option') */
#define DM_B_DIR_EXISTS		( OVERWRITE << 1 )
#define DM_B_DIR_NEEDED_4FILES	( DM_B_DIR_EXISTS << 1 )
#define DM_B_ADJUST_TARGET	( DM_B_DIR_NEEDED_4FILES << 1 )
#define DM_B_FILE_OP_STOPPED	( DM_B_ADJUST_TARGET << 1 )

typedef void (*DmClientProc)(DmProcReason reason, XtPointer client_data,
			     XtPointer call_data, char * str1, char * str2);

/* The FileOpInfo structure contains "Input" and "output" parameters
   describing the file operation.  Fields marked with '@' represent input to
   DmDoFileOp.
*/
typedef struct dmFileOpInfoRec {
    DmFileOpType	type;		/* @ file operation type */
    DtAttrs		attrs;		/*   target "type" & internal flags */
    DtAttrs		options;	/* @ REPORT_PROGRESS, OVERWRITE, etc */
    char *		target_path;	/* @ target of the operation */
    char *		src_path;	/* @ current directory */
    DtAttrs *		src_info;	/*   info about each source item */
    char **		src_list;	/* @ list of files to operate on */
    int			src_cnt;	/* @ size of src_list & src_info */
    struct dmFolderWinRec * src_win;	/* @ source win of file op (if any) */
    struct dmFolderWinRec * dst_win;	/* @ dest win of file op (if any) */
    int			cur_src;	/*   source item being processed */
    int			task_cnt;	/*   # of subtasks completed */
    int			error;		/*   error (if any) */
					/*   'short' fields next */
    WidePosition	x;		/* @ x position of drop */
    WidePosition	y;		/* @ y position of drop */
					/*   'byte' fields last */
} DmFileOpInfoRec, *DmFileOpInfoPtr;

/* The TaskList structure stores information about a sub-task to be 
   executed by a background work proc.  A series of sub-tasks may be
   needed to perform a file opr.   The TaskInfo structure (below) represents
   the file opr and contains the head of the list of sub-tasks.
*/
typedef struct dmTaskListRec {
    char *		source;		/* source file name for sub-task */
    char *		target;		/* destination for sub-task */
    DmFileOpType	type;		/* operation type */
    struct dmTaskListRec * next;	/* next task in the list */
} DmTaskListRec, *DmTaskListPtr;

/* The DmTaskInfo structure stores information about an operation
   initiated from a folder window, WB etc.
*/
typedef struct dmTaskInfoListRec {
    struct dmTaskInfoListRec * next;
    struct dmTaskInfoListRec * prev;
    DmTaskListPtr	task_list;
    DmTaskListPtr	cur_task;
    int			rfd;		/* read fd for incomplete copy opr. */
    int			wfd;		/* write fd for incomplete copy opr. */
    DmClientProc	client_proc;
    XtPointer		client_data;
    DmFileOpInfoPtr	opr_info;
					/* 'short' fields go next */
					/* 'byte' fields last */
} DmTaskInfoListRec, *DmTaskInfoListPtr;

/*			*** END OF FILE OPERATIONS ***			*/

/* This structure stores various options for filtering a view. */
typedef struct keylist {
    void *		key;
    struct keylist *	next;
} KeyList;


typedef struct dmFVFilter {
    char *	pattern;	/* filename pattern */
    DtAttrs	attrs;		/* view flags: DM_SHOW_DOT_FILES */
    Boolean     type[8]; 	/* Array of booleans of for 6 basic sys types */
} DmFVFilter, *DmFVFilterPtr;


/*	File Property Sheet-related structures			*/

/* File Property Sheet structure: list of these hangs off Folder window */
typedef struct _DmFPropSheetRec *DmFPropSheetPtr;
typedef struct _DmFPropSheetRec {
    DmFPropSheetPtr	next;		/* next property sheet */
    struct dmFolderWinRec * window;	/* folder window owner */
    Gizmo		shell;		/* popup shell gizmo*/
    DmItemPtr		item;		/* item within folder */
    char *              prev;           /* previous name string to restore */
    char *              prev_own;       /* previous own string to restore */
    char *              prev_grp;       /* previous grp string to restore */
    char *              prev_cmt;       /* previous cmt string to restore */
    char *              prev_class;     /* previous internal class name */
    char **		intClassList;	/* internal class name list */
    int			list_count;	/* class list count */
    short		prop_num;	/* serial prop. sheet # */
    Boolean		flag;		/* whether to popdown */
    Boolean		update;		/* update button was pressed last */
} DmFPropSheetRec;

/* This structure stores info about clients that want to be notified
   when a folder window is closed.
*/
typedef struct dmCloseNotifyRec {
    long		serial;	
    Window		client;
    Atom		replyq;
					/* 'short' fields go next */
    unsigned short	version;
					/* 'byte' fields go last */
} DmCloseNotifyRec, *DmCloseNotifyPtr;

typedef struct FolderViewRec {
    DmContainerPtr      cp;             /* ptr to container struct */
    DmItemPtr           itp;            /* ptr to item list */
    Cardinal            nitems;         /* number of items */
    Widget              box;            /* icon box widget */
    Widget              swin;           /* scrolled window widget */
    Widget              rubber;         /* views rubbertile widget BV */
    Widget              stext;          /* views title ie stext widget BV*/
    DmViewFormatType    view_type;      /* current view type */
    DmViewSortType      sort_type;      /* current sort type */
    void *              user_data;
} *DmFolderView, DmFolderViewRec;



/* This is the standard header that is in both the folder window and the
 * toolbox window structure.
 */
#define DM_WIN_HEADER(WINTYPE) \
    DtAttrs		attrs;		/* window type, etc. */		\
    void *		gizmo_shell;	/* "toplevel" gizmo */		\
    Widget		shell;		/* shell */			\
    Widget		swin;		/* scrolled window */		\
    XtIntervalId	busy_id;	/* timer for busy cursor */     \
    DmFolderViewRec	views[1];	/* folder views */		\
    WINTYPE		next;		/* next window */		\
					/* put 'short' fields next */

typedef struct dmWinRec *DmWinPtr;
typedef struct dmWinRec {
	DM_WIN_HEADER(DmWinPtr)
} DmWinRec;


#define DM_WIN_PATH(win)	( (win)->views[0].cp->path )
#define DM_WIN_USERPATH(win)	( (win)->user_path )
#define DM_WIN_ITEM(W, I)	((W)->views[0].itp + (I))
#define DM_OBJ_USERPATH(W,O) 	DmMakePath((W)->user_path, (O)->name)
#define DM_WB_PATH(D)		DM_WIN_PATH(DESKTOP_WB_WIN(D))
#define IS_WB_PATH(D, path)	( path && strcmp(path, DM_WB_PATH(D)) == 0 )
#define NUM_WB_OBJS(D)        (DESKTOP_WB_WIN(D)->views[0].cp->num_objs)
#define WB_IS_EMPTY(D)        (NUM_WB_OBJS(D) == 0)

/* A list of DmFolderWindowRec structures is used to store the information 
   about open folder windows.
*/
typedef struct dmFolderWinRec *DmFolderWinPtr;
typedef struct dmFolderWinRec {
    DM_WIN_HEADER(DmFolderWinPtr)

    Gizmo 	copy_prompt;		/* FileGizmoClass */
    Gizmo 	move_prompt;		/* FileGizmoClass */
    Gizmo 	link_prompt;		/* FileGizmoClass */
    Gizmo 	rename_prompt;		/* PopupGizmoClass */
    Gizmo 	convertd2u_prompt;	/* PopupGizmoClass */
    Gizmo 	convertu2d_prompt;	/* PopupGizmoClass */
    Gizmo 	folder_prompt;		/* FileGizmoClass */
    Gizmo 	customWindow;		/* PopupGizmo for customized view */
    Gizmo	finderWindow;		/* PopupGizmo for find utility */
    Gizmo	createWindow;		/* Popup Gizmo for New utility */
    Gizmo	overwriteNotice;	/* ModalGizmo for Overwrite notice */
    Gizmo	confirmDeleteNotice;	/* ModalGizmo for confirming deletes */
    Gizmo	nmchgNotice;
    DmFVFilter		filter;		/* view filter info */
    DmFPropSheetPtr	fpsptr;		/* list of file prop. sheets */
    DmTaskInfoListPtr	task_id;
    int			num_notify;	/* number of notify entries */
    DmCloseNotifyPtr	notify;		/* ptr to the list of notify entries */
    char *		user_path;	/* ptr to user-traversed pathname
					 * (before calling realpath(3)) */
    char *		root_dir;	/* "root" directory to limit access */
    
    /* 'short' fields go next */
    /* 'byte' fields go last */

    Boolean		saveFormat;	/* save format, size in .dtinfo */
    Boolean		filter_state;
} DmFolderWinRec, *DmFolderWindow;


typedef struct dmDnDFileOpRec {
    OlDnDTriggerOperation	type;
    DmWinPtr			wp;
    Atom 			selection;
    Time			timestamp;
					/* 'short' fields next */
    Position			root_x;
    Position			root_y;
} DmDnDFileOpRec, *DmDnDFileOpPtr;

typedef struct dmDnDInfoRec {
    DmWinPtr			wp;		/* base window struct ptr */
    DmItemPtr			ip;		/* "current" item */
    DmItemPtr *			list_idx;	/* "current" item in list */
    DmItemPtr *			ilist;		/* item list */
    Atom			selection;	/* source selection */
    Time			timestamp;	/* timestamp of trigger msg */
    OlDnDTriggerOperation	opcode;		/* opcode (copy or move) */
    DtAttrs			attrs;		/* transaction attributes */
    XtPointer			user_data;	/* a hook for special info */
						/* Put 'short' fields next */
    Position			x;		/* position rel. to dst win */
    Position			y;		/* position rel. to dst win */
						/* Put 'byte' fields last */
} DmDnDInfoRec, *DmDnDInfoPtr;

/* Structure for appl resources */
typedef struct dmOptions {
    int			sync_interval;		/* in millisecs */
						/* 'short' fields go next */
    Dimension		grid_width;
    Dimension		grid_height;
						/* 'byte' fields go last */
    u_char		folder_cols;
    u_char		folder_rows;
    Boolean		kill_all_windows;	/* during exiting */
    Boolean		launch_from_cwd;	/* launch apps from CWD ? */
    Boolean		open_in_place;		/* default folder open style */
    Boolean		show_full_path;
    Boolean		show_hidden_files;
    Boolean		sync_remote_folders;
    u_char		tree_depth;		/* tree view default depth */
    String		label_separators;	/* icon label separators */
} DmOptions;

typedef struct dmFontListInfo{
    XmFontList font;
    Dimension	max_width;
    Dimension	max_height;
} DmFontListInfo;

typedef struct dmFSTypeInfo{
    int		num_fstypes;
    short	dos_id;
    short	nucfs_id;
    short	nucam_id;
    short	nfs_id;
} DmFSTypeInfo;

/* Used for File:New window */
typedef struct createInfo {
	int		flag;
	char		*fname;
	char		*template;
	DmItemPtr	itp;
	DmFolderWinPtr	fwp;
	CListGizmo	*clistG;
	Gizmo		listG;
	Gizmo		inputG;
	Widget		listW;
	Widget		patternW;
} CreateInfo;


typedef Bufferof(DmContainerPtr) DmContainerBuf;

/* DmDesktopRec structure contains all the global data associated with the
   desktop. If multiple (vitual) desktops are supported in the future, then
   several instances of this structure will be created.
*/
typedef struct dmDesktopRec {
    Widget		init_shell;	/* shell from Xt[App]Initialize() */
    DmFmodeKeyPtr	fmkp;		/* list of internal file classes */
    DmFnameKeyPtr	fnkp;		/* list of external file classes */
    DmFclassFilePtr	fcfp;		/* list of class files */
    DmFolderWindow	folders;	/* list of folder windows */
    DmContainerBuf	containers;	/* list of containers */
    DmFolderWindow	wb_win;		/* folder window pointer for wb */
    DmFolderWindow	icon_setup_win;	/* icon setup window */
    DmFolderWindow	help_desk;	/* folder window ptr for help desk */
    DmFolderWindow	top_folder;	/* DESKTOP HOME folder */
    DmFolderWindow	tree_win;	/* Tree view window */
    DmHelpAppPtr	help_info;	/* list of HM's app info */
    DtPropList		properties;	/* desktop property list */
    DmOptions		options;	/* desktop options */
    DmTaskInfoListPtr	current_task;	/* current task info. */	
    DmGlyphPtr		shortcut;	/* glyph for shortcut */
    Window		wb_icon_win;	/* wb icon window id */
    int			folder_help_id;	/* Help Appl ID for folders */
    int			wb_help_id;	/* Help Appl ID for wastebasket */
    int			ib_help_id;	/* Help Appl ID for icon binder */
    int			fmap_help_id;	/* Help Appl ID for folder map */
    int			icon_setup_help_id;/* help id for Icon Setup */
    int			desktop_help_id; /* Help Appl ID for Destiny window */
    char *		node_name;
    mode_t		umask;
    char *		home;		/* convenient to store $HOME */
    char *		login;		/* user's login id */
    char *		cwd;		/* current working directory */
    char *		dt_dir;		/* desktop directory */
    XtIntervalId	sync_timer;	/* so timer can be removed */
    DmFontListInfo	sansserif_fontinfo;  /* XmNsansSerifFamilyFontList */
    DmFontListInfo	monospaced_fontinfo; /* XmNmonospacedFamilyFontList */
    DmFontListInfo	serif_fontinfo;	     /* XmNserifFamilyFontList */
    struct stalecontainer *stale_containers; /* list of containers needing 
						updates*/
    DmFSTypeInfo	fstypes;	/* fs types recognized by dtm*/
    struct fsinfo	*fstbl;  	/* list of File system related info */
    Atom		dnd_targets[6];	/* DnD targets */
    char		*dpalette;	/* default palette */


    /* Put 'byte' fields last */

    Boolean		bg_not_regd;	/* XtAppProc() registered or not */
    u_char		num_visited;	/* Number of visited folders <max */
    Boolean		use_font_obj;	/* Motif supports font object ext */
    Boolean		C_locale;       /* True if C locale, false otherwise*/

} DmDesktopRec, *DmDesktopPtr;

extern DmDesktopPtr Desktop;	/* There is one global Destop struct */

/* macros to access the global desktop structure */
#define DESKTOP_CUR_TASK(D)	( (D)->current_task )
#define DESKTOP_CWD(D)		( (D)->cwd )
#define DESKTOP_DIR(D)		( (D)->dt_dir )
#define DESKTOP_FMKP(D)		( (D)->fmkp )
#define DESKTOP_FNKP(D)		( (D)->fnkp )
#define DESKTOP_FOLDERS(D)	( (D)->folders )
#define DESKTOP_CONTAINERS(D)	( (D)->containers )
#define DESKTOP_HELP_DESK(D)	( (D)->help_desk )
#define DESKTOP_HELP_ID(D)	( (D)->desktop_help_id )
#define DESKTOP_HELP_INFO(D)	( (D)->help_info )
#define DESKTOP_HOME(D)		( (D)->home )
#define DESKTOP_LOGIN(D)	( (D)->login )
#define DESKTOP_MONOSPACED(D)	( (D)->monospaced_fontinfo )
#define DESKTOP_NODE_NAME(D)	( (D)->node_name )
#define DESKTOP_OPTIONS(D)	( (D)->options )
#define DESKTOP_PROPS(D)	( (D)->properties )
#define DESKTOP_SANS_SERIF(D)	( (D)->sansserif_fontinfo )
#define DESKTOP_SERIF(D)	( (D)->serif_fontinfo )
#define DESKTOP_SHELL(D)	( (D)->init_shell )
#define DESKTOP_SHORTCUT(D)	( (D)->shortcut )
#define DESKTOP_TOP_FOLDER(D)	( (D)->top_folder )
#define DESKTOP_TOP_TB(D)	( (D)->top_tb )
#define DESKTOP_UMASK(D)	( (D)->umask )
#define DESKTOP_WB_ICON(D)	( (D)->wb_icon_win )
#define DESKTOP_WB_WIN(D)	( (D)->wb_win )
#define DESKTOP_ICON_SETUP_WIN(D) ( (D)->icon_setup_win )
#define STALE_CONTAINERS(D)	( (D)->stale_containers )
#define SYNC_TIMER(D)		( (D)->sync_timer )
#define TREE_WIN(D)		( (D)->tree_win )
#define USE_FONT_OBJ(D)		( (D)->use_font_obj )
#define NUM_VISITED(D)		( (D)->num_visited )
#define WB_HELP_ID(D)		( (D)->wb_help_id )
#define BINDER_HELP_ID(D)	( (D)->ib_help_id )
#define FMAP_HELP_ID(D)		( (D)->fmap_help_id )
#define ICON_SETUP_HELP_ID(D)	( (D)->icon_setup_help_id )
#define FOLDER_HELP_ID(D)	( (D)->folder_help_id )

#define DESKTOP_SCREEN(D)	( XtScreen(DESKTOP_SHELL(D)) )
#define DESKTOP_DISPLAY(D)	( XtDisplay(DESKTOP_SHELL(D)) )

/* macros to access remote file system type ids recognized by dtm */
#define NUMFSTYPES(D)		( (D)->fstypes.num_fstypes )
#define DOSFS_ID(D)		( (D)->fstypes.dos_id )
#define NUCFS_ID(D)		( (D)->fstypes.nucfs_id )
#define NUCAM_ID(D)		( (D)->fstypes.nucam_id )
#define NFS_ID(D)		( (D)->fstypes.nfs_id )
#define IS_REMOTE(D,FS)		( (((FS) != -1) && \
				  (((FS) == NUCFS_ID((D))) || \
				   ((FS) == NUCAM_ID((D))) || \
				   ((FS) == NFS_ID((D))))))

/* macros to access desktop options */
#define FOLDER_COLS(D)		( DESKTOP_OPTIONS(D).folder_cols )
#define FOLDER_ROWS(D)		( DESKTOP_OPTIONS(D).folder_rows )
#define GRID_WIDTH(D)		( DESKTOP_OPTIONS(D).grid_width )
#define GRID_HEIGHT(D)		( DESKTOP_OPTIONS(D).grid_height )
#define KILL_ALL_WINDOWS(D)	( DESKTOP_OPTIONS(D).kill_all_windows )
#define LAUNCH_FROM_CWD(D)	( DESKTOP_OPTIONS(D).launch_from_cwd )
#define OPEN_IN_PLACE(D)	( DESKTOP_OPTIONS(D).open_in_place )
#define SHOW_FULL_PATH(D)	( DESKTOP_OPTIONS(D).show_full_path )
#define SHOW_HIDDEN_FILES(D)	( DESKTOP_OPTIONS(D).show_hidden_files )
#define SYNC_INTERVAL(D)	( DESKTOP_OPTIONS(D).sync_interval )
#define SYNC_REMOTE_FOLDERS(D)	( DESKTOP_OPTIONS(D).sync_remote_folders )
#define TREE_DEPTH(D)		( DESKTOP_OPTIONS(D).tree_depth )
#define LABEL_SEPS(D)		( DESKTOP_OPTIONS(D).label_separators )

/* macro to access DnD targets */
#define DESKTOP_DND_TARGETS(D)	( (D)->dnd_targets )

/* macro to check if in C locale */
#define IS_C_LOCALE(D)  ( (D)->C_locale )

/******************************************************************************
 *
 *	III.  #define's for BIT-FIELDS/OPTIONS
 */

/* Attributes for DmWinRec 'attrs' field
 *	The DM_B_*_WIN defines below are used to identify the type of window.
 *	This would normally be done by registering the 'Window' structure as
 *	client data but this cannot be done when menus are shared between
 *	folders, for instance.
 */
#define DM_B_FOLDER_WIN		( 1 << 0 )
#define DM_B_TOP_FOLDER		( 1 << 1 )
#define DM_B_FOUND_WIN		( 1 << 2 )
#define DM_B_HELP_WIN		( 1 << 3 )
#define DM_B_HELPDESK_WIN	( 1 << 4 )
#define DM_B_TREE_WIN		( 1 << 5 )
#define DM_B_WASTEBASKET_WIN	( 1 << 6 )
#define DM_B_ICON_SETUP_WIN	( 1 << 7 )
#define DM_B_APPLICATIONS_WIN	( 1 << 8 )
#define DM_B_PREFERENCES_WIN	( 1 << 9 )
#define DM_B_DISKS_ETC_WIN	( 1 << 10 )
#define DM_B_ADMIN_TOOLS_WIN	( 1 << 11 )
#define DM_B_MAILBOX_WIN	( 1 << 12 )
#define DM_B_NETWORKING_WIN	( 1 << 13 )
#define DM_B_GAMES_WIN		( 1 << 14 )
#define DM_B_NETWARE_WIN	( 1 << 15 )
#define DM_B_WALLPAPER_WIN	( 1 << 16 )
#define DM_B_UUCP_INBOX_WIN	( 1 << 17 )
#define DM_B_DISKETTE_WIN	( 1 << 18 )
#define DM_B_STARTUP_ITEMS_WIN	( 1 << 19 )
#define DM_B_PERSONAL_CLASSES	( 1 << 20 )
#define DM_B_SYSTEM_CLASSES	( 1 << 21 )
#define DM_B_CDROM_WIN		( 1 << 22 )
						/* window "state": */
#define DM_B_SHOWN_MSG		( 1 << 27 )
#define DM_B_FILE_OP_BUSY	( 1 << 28 )
#define DM_B_BEING_DESTROYED	( 1 << 29 )


#define DM_B_BASE_WINDOW	( DM_B_FOLDER_WIN | DM_B_TOP_FOLDER |	\
				  DM_B_FOUND_WIN | DM_B_HELP_WIN |	\
				  DM_B_HELPDESK_WIN | DM_B_TREE_WIN |	\
				  DM_B_WASTEBASKET_WIN )


/* options to DmOpenFolderWindow() and DmOpenDir() 
 * (also passed through DmOpenObject to DmOpenFolderWindow)
 */
/* 
 *	WARNING: DM_B_INIT_FILEINFO and DM_B_SET_TYPE cannot conflict
 *	with DM_B_ADD_TO_END (DesktopP.h).  See DmAddObjectToContainer
 */
#define DM_B_INIT_FILEINFO	(1 << 0)
#define DM_B_SHOW_HIDDEN_FILES	(1 << 1)
#define DM_B_SET_TYPE		(1 << 2)
#define DM_B_TIME_STAMP		(1 << 3)
#define DM_B_READ_DTINFO	(1 << 4)
#define DM_B_PRIVATE_COPY       (1 << 5)  /* both OpenDir and CloseContainer*/
#define DM_B_DIRECTORY_ONLY     (1 << 6)  /* OpenDir passes to InitObj*/
#define DM_B_ROOTED_FOLDER	(1 << 7)  /* mark dir as root for folder */
#define DM_B_SET_CLASS_PROP	(1 << 8)  /* used by CreateFile() */
#define DM_B_OPEN_NEW		(1 << 9)  /* don't raise existing folderwin */
#define DM_B_OPEN_IN_PLACE	(1 << 10)  /* open in same folder */

/* options to DmCloseDir() & DmCloseContainer() */
#define DM_B_NO_FLUSH		(1 << 0)

/* DmReadDtInfo() options */
#define INTERSECT		(1 << 0)

/* options to DmWriteDtInfo() */
#define DM_B_PERM		(1 << 0)

/* options to DmAddShortcut() and DmUniqueLabel */
#define DM_B_DUPCHECK		(1 << 0)
#define DM_B_MOVE		(1 << 1)

/* options to DmDnDNewTransaction() */
#define DM_B_SEND_EVENT		(1 << 0)

/* options for updating a folder window */
#define DM_UPDATE_SRCWIN	( 1 << 0)
#define DM_UPDATE_DSTWIN	( 1 << 1)

/* options in DmFnameKeyRec and DmFmodeKeyRec structure */
#define DM_B_FILETYPE		(1 << 0)
#define DM_B_FILEPATH		(1 << 1)
#define DM_B_REGEXP		(1 << 2)
#define DM_B_SYS		(1 << 3)
#define DM_B_DELETED		(1 << 4)
#define DM_B_NEW		(1 << 5)
#define DM_B_VISUAL		(1 << 6)
#define DM_B_TYPING		(1 << 7)
#define DM_B_REPLACED		(1 << 8)
#define DM_B_NEW_NAME		(1 << 9)
#define DM_B_ACTION		(1 << 10)
#define DM_B_CLASSFILE		(1 << 11)
#define DM_B_BAD_CLASSFILE	(1 << 12)
#define DM_B_WRITE_FILE		(1 << 13)
#define DM_B_READONLY		(1 << 14)
#define DM_B_LREGEXP		(1 << 15)
#define DM_B_LFILEPATH		(1 << 16)
#define DM_B_OVERRIDDEN		(1 << 17)
#define DM_B_SYSTEM_CLASS 	(1 << 18)
#define DM_B_PERSONAL_CLASS 	(1 << 19)
#define DM_B_IN_NEW     	(1 << 20)
#define DM_B_TO_WB      	(1 << 21)
#define DM_B_PROG_TO_RUN	(1 << 22)
#define DM_B_PROG_TYPE		(1 << 23)
#define DM_B_TEMPLATE		(1 << 24)
#define DM_B_ICON_ACTIONS	(1 << 25)
#define DM_B_IGNORE_CLASS	(1 << 26)
#define DM_B_DONT_CHANGE	(1 << 27)
#define DM_B_DONT_DELETE	(1 << 28)
#define DM_B_LOCKED		(DM_B_DONT_CHANGE | DM_B_DONT_DELETE)
#define DM_B_TEMPLATE_CLASS	(1 << 29)
#define DM_B_INACTIVE_CLASS	(1 << 30)
#define DM_B_MANUAL_CLASSING_ONLY	((unsigned long)1 << 31)


/* options for layout routines (see DmComputeLayout)*/
#define UPDATE_LABEL		(1 << 1)   /* 0 for NONE, undefined for now */
#define	RESTORE_ICON_POS	(1 << 2)
#define	SAVE_ICON_POS		(1 << 3)
#define NO_ICONLABEL		(1 << 4)

/* options for file operations-DmDoFileOp (internal ones must fit in u_char) */
#define OVERWRITE		( 1 << 0 )	/* Also used internally */
#define REPORT_PROGRESS		( 1 << 1 )
#define OPRBEGIN		( 1 << 2 )
#define	UNDO			( 1 << 3 )
#define	RMNEWDIR		( 1 << 4 )	/* during UNDO */
#define MULTI_PATH_SRCS		( 1 << 5 )
#define EXTERN_DND		( 1 << 6 )
#define EXTERN_SEND_DONE	( 1 << 7 )
#define DONT_OVERWRITE		( 1 << 8 )	/* return error instead */
#define NMCHG			( 1 << 9 )
#define FILE_NAMES		( 1 << 10 )

/* options for DmDnDInfoRec */
#define DM_B_TRANS_IN		( 1 << 1 )	/* incoming transaction */
#define DM_B_COPY_OP 		( 1 << 2 )	/* copy operation */
#define DM_B_DELETE  		( 1 << 3 )	/* delete object when done */

/* options for DmQueryFolderWindow */
#define DM_B_NOT_ROOTED		( 1 << 0 )	/* don't return rooted dirs */
#define DM_B_NOT_DESKTOP	( 1 << 1 )	/* don't return desktop win */


/******************************************************************************
	IV.  #define's for INTEGER VALUES
*/
#define MAX_VISITED_FOLDERS	7


/******************************************************************************
	V.  #define's for STRINGS
*/

#define DM_INFO_FILENAME	".dtinfo"

/* desktop properties */
#define ICONPATH		"_ICONPATH"
#define FILEDB_PATH		"FILEDB_PATH"
#define WBDIR			"WBDIR"
#define HDPATH			"HDPATH"
#define DESKTOPDIR		"DESKTOPDIR"
#define TEMPLATEDIR		"TEMPLATEDIR"
#define IGNORE_SYSTEM		"_IGNORE_SYSTEM"
#define DFLTPRINTCMD		"_DFLTPRINTCMD"

/* class properties */
#define REAL_PATH		"_REALPATH"
#define PATTERN			"_PATTERN"
#define LPATTERN		"_LPATTERN"
#define LFILEPATH		"_LFILEPATH"
#define TEMPLATE		"_TEMPLATE"
#define UNIQUE			"_UNIQUE"
#define CLASS_NAME		"_CLASSNAME"
#define ICON_LABEL		"_ICONLABEL"
#define SYSTEM			"_SYSTEM"
#define DISPLAY_IN_NEW 		"_DISPLAY_IN_NEW"
#define MOVE_TO_WB		"_MOVE_TO_WB"
#define PROG_TO_RUN		"_PROG_TO_RUN"
#define PROG_TYPE		"_PROG_TYPE"
#define FSTYPE			"_FSTYPE"

/* instance properties */
#define OBJCLASS		"_CLASS"
#define OBJCWD			"_CWD"

/* format used to convert time to a string */
#define TIME_FORMAT		"%X %a %x"

/******************************************************************************
	VI.  MACROS
*/

/*
 *  debugging stuff
 */
#ifdef DEBUG
#define debug(x)        (void)fprintf x
#define trace(x)        (void)fprintf(stderr,\
                    "%s: line = %d, file = %s\n",\
                    x, __LINE__, __FILE__)
#else
#define debug(x)
#define trace(x)
#endif

/* 
 Usage:
        _DPRINT3(stderr, "Popping fd %d from stack\n", s_c_fd);
*/

#ifdef FDEBUG
extern int Debug;
#define _DPRINT(n) (Debug < n) ? 1 : fprintf
#else
#define _DPRINT(n) (1) ? 1 : fprintf
#endif
#define _DPRINT1 _DPRINT(1)
#define _DPRINT3 _DPRINT(3)
#define _DPRINT5 _DPRINT(5)



/* Macros to get default width and height of folder window.  */
#define FOLDER_WINDOW_WIDTH(widget)	(68 * WidthOfScreen(		\
						XtScreen(widget)) / 100)

#define FOLDER_WINDOW_HEIGHT(widget)	(35 * HeightOfScreen(	\
						XtScreen(widget)) / 100)

#define IS_TREE_WIN(D, W)	( (W)->attrs & DM_B_TREE_WIN )
#define IS_FOUND_WIN(D, W)	( (W)->attrs & DM_B_FOUND_WIN )
#define IS_WB_WIN(D, W)		( (W)->attrs & DM_B_WASTEBASKET_WIN )

#define IS_SAME_PATH(p1, p2)	(IS_C_LOCALE(Desktop) ?        \
					(strcmp(p1,p2) == 0) : \
					(strcoll(p1,p2) == 0))
#define IS_NETWARE_PATH(p)	(IS_SAME_PATH(p,NETWARE_PATH))

/* Macro to find out what WB clean-up method is */
#define DM_WBIsByTimer(D)	( DmWBCleanUpMethod(D) == 0 )
#define DM_WBIsOnExit(D)		( DmWBCleanUpMethod(D) == 1 )
#define DM_WBIsImmediate(D)	( DmWBCleanUpMethod(D) == 2 )
#define DM_WBIsNever(D)		( DmWBCleanUpMethod(D) == 3 )

#define OBJ_CLASS_NAME(OP)	(((DmFnameKeyPtr)((OP)->fcp->key))->name)
#define OBJ_FILEINFO(OP)	((DmFileInfoPtr)((OP)->objectdata))
#define BUSY_FOLDER(F)		DmWorkingFeedback((F), 2000, False)
#define BUSY_DESKTOP(W)		DtLockCursor((W), 2000L, NULL, NULL, \
					DtGetBusyCursor(W))

/* Macro to check for "/" */
#define ROOT_DIR(path)	( (path[0] == '/') && (path[1] == '\0') )

/* check if the name is ".." */
#define IS_DOT_DOT_FILE(P)      ((P[0] == '.') &&                       \
                         (P[1] == '.') && (P[2] == '\0'))


/* check if the name is either "." or ".." */
#define IS_DOT_DOT(P)	(P[0] == '.') &&			\
                         ((P[1] == '\0') ||			\
                         ((P[1] == '.') && (P[2] == '\0')))

/* macros used with DmObjectRec structure */
#define IS_EXEC(OP)		((OP)->ftype == DM_FTYPE_EXEC)




/* Macros to compute string metrics using icon_box font */
#define DmTextWidth(icon_box, item) DM_TextWidth((icon_box), (item))
#define DmTextHeight(icon_box, item) DM_TextHeight((icon_box), (item))
#define DmTextExtent(icon_box, item, width, height) \
    DM_TextExtent((icon_box), (item), (width), (height))

/* Macros for computing "row heights" of views with "fixed" heights.
   (Assumption: DM_FTYPE_DIR and DM_FTYPE_DATA small glyphs are same height.)
   + 2 ICON_PADDING is for frame around label; + 2-1/2 is for shortcut
   glyph and space between shortcut and the glyph.
*/
/* Height in NAME view is greater of "small" glyph (with space for link)  
 * and (max) font plus space for shadow and highlight to surround both 
 * glyph and text
 */
/* FLH MORE: make this resolution independent - use points */
#define LINK_PADDING (2 * ICON_PADDING + ICON_PADDING/2)
#define DM_NameRowHeight(icon_box) 						\
 (Dimension)(DM_Max((int)DmFontHeight(icon_box), 				\
 (int)(DmFtypeToFmodeKey(DM_FTYPE_DIR)->small_icon->height + LINK_PADDING))	\
  + 2 * PPART(icon_box).shadow_thickness 					\
  + 2 * PPART(icon_box).highlight_thickness)

/* Height in LONG view is greater of "small" glyph (plush space for link)
 * and (fixed) font plus space for shadow and highlight to surround both
 * glyph and text.  FLH_MORE: with font stored in icon_box, should be
 * equivalent to DM_NameRowHeight
 */
#define DM_LongRowHeight(icon_box) 				\
 (Dimension)(DM_Max((int)(DmFtypeToFmodeKey(DM_FTYPE_DIR)->	\
  small_icon->height + LINK_PADDING),				\
  (int)DmFontHeight(icon_box)) 					\
  + 2 * PPART(icon_box).shadow_thickness 			\
  + 2 * PPART(icon_box).highlight_thickness)


/* macro to beep */
#define DmBeep() XBell(DESKTOP_DISPLAY(Desktop), 0)

/* Macros to clear Footer areas */
#define DmClearStatus(window)	DmVaDisplayStatus(window, False, NULL);
#define DmClearState(window)	DmVaDisplayState(window, NULL);

/* Macros for sync timer */
#define Dm__AddSyncTimer(D)	SYNC_TIMER(D) = \
    XtAddTimeOut(SYNC_INTERVAL(D), Dm__SyncTimer, (XtPointer)(D))
#define Dm__RmSyncTimer(D)	XtRemoveTimeOut(SYNC_TIMER(D))

/* Macro to make an item label based on the view-type */
#define Dm__MakeItemLabel(item, view_type, maxlen) (view_type == DM_LONG) ? \
    DmGetLongName(item, (maxlen) + 3, False) : DmGetObjectName(ITEM_OBJ(item))


/* These macros are used to define MenuGizmo structures. */

#define MENU_ITEM(LABEL, MNEMONIC, SELECT_PROC)	\
    {						\
	(XtArgVal)True,	/* sensitive */		\
	LABEL,					\
	MNEMONIC,				\
	I_PUSH_BUTTON,	/* button type */	\
	NULL,		/* sub menu/resource */	\
	SELECT_PROC,				\
	NULL,		/* client_data */	\
	False,		/* set */		\
    }

#define MENU(NAME, ITEMS)	\
	static MenuGizmo ITEMS = {				\
		NULL,			/* help */			\
		NAME,			/* shell widget name */		\
		NULL,			/* title (implies pin) */	\
		ITEMS ## Items,		/* items */			\
		NULL,			/* default selectProc */	\
		NULL,			/* default clientData */	\
		XmVERTICAL,		/* layout type */		\
		1,			/* measure */			\
		0,			/* default item index */	\
	}

#define MENU_BAR(NAME, ITEMS, SELECT, DEFAULT_ITEM, CANCEL_ITEM)	\
	static MenuGizmo ITEMS = {					\
		NULL,			/* help */			\
		NAME,			/* shell widget name */		\
		NULL,			/* title (implies pin) */	\
		ITEMS ## Items,		/* items */			\
		SELECT,			/* default selectProc */	\
		NULL,			/* default clientData */	\
		XmHORIZONTAL,		/* layout type */		\
		1,			/* measure */			\
		DEFAULT_ITEM,		/* default item index */	\
		CANCEL_ITEM,		/* cancel item index */		\
	}

/* Define strings for special file system types recognized by dtm */
#define DOSFS	"dosfs"
#define NUCFS	"nucfs"
#define NUCAM	"nucam"
#define _NFS	"nfs"	/* NFS is defined to "NFS" in fsid.h */

#define	DOS_MAXPATH	13

struct fsinfo {
	char   f_fstype[FSTYPSZ];	/* file system */
	int    f_maxnm;			/* maximum file name length */
};

/*	
 *  DmValidateFname() function returns values: 
 */
typedef enum dmvpret {VALID, INVALCHAR, MAXNM, STATFAIL, 
		      DSTATFAIL, NOFILE} dmvpret_t;

extern char *doschar;

/* title of dtm modules used in on-line help */
extern const char *dtmgr_title;
extern const char *product_title;
extern const char *folder_title;


/* FolderWindow gizmo prompt info */

typedef struct folderPromptInfoRec {
    GizmoClass	class;
    Cardinal	gizmo_offset;
    Widget	(*get_gizmo_shell)(Gizmo);
} FolderPromptInfoRec, *FolderPromptInfoPtr;

extern const FolderPromptInfoRec FolderPromptInfo[];
extern const int NumFolderPromptInfo;

#endif /* __Dtm_h_ */
