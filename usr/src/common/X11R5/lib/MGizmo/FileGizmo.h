#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:FileGizmo.h	1.6"
#endif

/* Edit this file with ts=4 */

#ifndef _FileGizmo_h
#define _FileGizmo_h

typedef enum {
	FOLDERS_ONLY, FOLDERS_AND_FILES
} FolderType;

typedef enum {
    ABOVE_LIST, BELOW_LIST
} FGizmoPosition;

typedef struct _FileGizmo {
	HelpInfo *	help;			/* Help information */
	char *		name;			/* Name of the shell */
	char *		title;			/* Title of the window */
	MenuGizmo *	menu;			/* Pointer to menu info */
	GizmoArray	upperGizmos;	/* The gizmo list at top of list */
	int			numUpper;		/* Number of gizmos  in top list */
	GizmoArray	lowerGizmos;	/* The gizmo list at bottom of list */
	int 		numLower;		/* Number of gizmos at bottom of */
	char *		pathLabel;		/* Label on path field */
	char *		inputLabel;		/* Label on the input field */
	FolderType	dialogType;		/* FOLDERS_ONLY or FOLDERS_AND_FILES */
	char *		directory;		/* Current directory */
	FGizmoPosition	lower_gizmo_pos;	/* position of lower gizmos */
} FileGizmo;

extern GizmoClassRec FileGizmoClass[];

extern Widget	GetFileGizmoShell(Gizmo);
extern Widget	GetFileGizmoRowCol(Gizmo);
extern void		ExpandFileGizmoFilename(Gizmo);
extern char *	GetFilePath(Gizmo);
extern void 	SelectFileGizmoInputField(Gizmo g);
extern void 	SetFileGizmoInputField(Gizmo g, String value);
extern void		SetFileGizmoInputLabel(Gizmo g, String value);
extern void		ResetFileGizmoPath(Gizmo);

#endif /* _FileGizmo_h */
