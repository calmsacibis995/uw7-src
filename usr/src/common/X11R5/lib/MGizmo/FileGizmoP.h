#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:FileGizmoP.h	1.4"
#endif

/* Edit this file with ts=4 */

#ifndef _FileGizmoP_h
#define _FileGizmoP_h

#include "FileGizmo.h"

#define NUM_ICONS	9

typedef struct _ListData {
	char *				names;		/* List of files */
	DmFileType			ftype;		/* DM_FTYPE_DIR, DM_FTYPE_EXEC, ... */
} ListData;

typedef struct _ListRec {
	struct _ListData *	data;		/* Names and file types */
	XmString *			xmsnames;	/* Localized list of files */
	int					cnt;		/* Number of files in list */
	int					size;		/* Size of the list (can be > cnt) */
	Widget				cw;			/* List widget */
} ListRec;

typedef struct _FileGizmoP {
	char *			name;				/* Name of the Gizmo */
	char *			directory;			/* Current directory */
	char *			prevDir;			/* Track prev dir for error recovery */
	char *			filter;				/* Value to filter dir entries */
	Gizmo			menu;				/* Pointer to menu info */
	struct _ListRec	dir;				/* Directories in current directory */
	struct _ListRec	files;				/* Files in the current directory */
	FolderType		dialogType;			/* FOLDERS_ONLY or FOLDERS_AND_FILES */
	GizmoArray		upperGizmos;		/* The gizmo list at top of list */
	int				numUpper;			/* Number of gizmos  in top list */
	GizmoArray		lowerGizmos;		/* The gizmo list at bottom of list */
	int				numLower;			/* Number of gizmos at bottom of */
	Widget			pathAreaWidget;		/* Path display area */
	Widget			pathAreaLabel;		/* Path display area */
	Widget			inFieldWidget;		/* Folders in: or Items In: */
	Widget			inputFieldWidget;	/* Input field */
	Widget			inputFieldLabel;	/* Input field */
	Widget			checkBoxWidget;		/* Show hiddden */
	Widget			dirWidget;			/* Subdirs scrolled window */
	Widget			fileWidget;			/* Filename scrolled window */
	Widget			filesFilteredLabel;	/* Files (n filtered out) */
	Widget			shell;				/* File shell */
	Widget			row_column;			/* only child of shell */
} FileGizmoP;

extern void	DmDrawNameIcon(Widget , XtPointer , XtPointer);

#endif /* _FileGizmoP_h */
