#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:Gizmo.h	1.4"
#endif

#ifndef _Gizmo_h
#define _Gizmo_h

#define FS		"\001"
#define GGT		GetGizmoText

#include <DesktopP.h>
#include <Xm/Xm.h>

typedef void * Gizmo;

typedef struct _HelpInfo {
	char *		appTitle;
	char *		title;
	char *		filename;
	char *		section;
} HelpInfo;

typedef struct _GizmoClassRec {
	char *		name;
	Gizmo		(*createFunction)();
	void		(*freeFunction)();
	void		(*mapFunction)();
	Gizmo		(*getFunction)();
	Gizmo		(*getMenuFunction)();
	void		(*dumpFunction)();
	void		(*manipulateFunction)();
	XtPointer	(*queryFunction)();
	Gizmo		(*setByNameFunction)();
	Widget		(*attachmentFunction)();
} GizmoClassRec;

typedef GizmoClassRec * GizmoClass;

typedef struct _GizmoRec {
	GizmoClass	gizmoClass;
	Gizmo		gizmo;
	Arg *		args;
	Cardinal	numArgs;
} GizmoRec;

typedef GizmoRec *GizmoArray;

typedef enum {
	GetGizmoValue,		/* From widget to current value */
	ApplyGizmoValue,	/* From current value to previous value */
	ResetGizmoValue,	/* From previous value to current value */
	ReinitializeGizmoValue,	/* From initial value to current value */
} ManipulateOption;

typedef enum {
	GetGizmoCurrentValue,	/* Retrieve the gizmo current settings */
	GetGizmoPreviousValue,	/* Retrieve the gizmo previous settings */
	GetGizmoWidget,		/* Retrieve the gizmo Widget */
	GetGizmoGizmo,		/* Retrieve the named gizmo */
	GetItemWidgets		/* Retrieve the WidgetList of all the items */
} QueryOption;

extern Widget		GetAttachmentWidget(GizmoClass, Gizmo);
extern void		ManipulateGizmo(GizmoClass, Gizmo, ManipulateOption);
extern Gizmo		SetGizmoValueByName(GizmoClass, Gizmo, char *, char *);
extern char *		GetGizmoText(char *);
extern void		FreeGizmo(GizmoClass, Gizmo);
extern Gizmo		CreateGizmo(Widget, GizmoClass, Gizmo, ArgList, int);
extern XtPointer	QueryGizmo(GizmoClass, Gizmo, QueryOption, char *);
extern void		FreeGizmoArray(GizmoArray, int);
extern GizmoArray	CreateGizmoArray(Widget, GizmoArray, int);
extern void		DumpGizmo(GizmoClass, Gizmo, int);
extern void		DumpGizmoArray(GizmoArray, int, int);
extern Gizmo		SetGizmoArrayByName(GizmoArray, int, char *, char *);
extern Widget		InitializeGizmoClient(
				char *, char *, 
				XrmOptionDescRec *, Cardinal, int *, char **,
				XtPointer, XtResourceList, Cardinal,
				ArgList, Cardinal, char *
			);
extern void		GizmoMainLoop(
				void(*)(), XtPointer, void(*)(), XtPointer
			);
extern void		MapGizmo(GizmoClass, Gizmo);
extern Cardinal		AppendArgsToList(ArgList, Cardinal, ArgList, Cardinal);
extern Pixmap		PixmapOfFile(Widget, char *, int *, int *);
extern void		PostGizmoHelp(Widget, HelpInfo *);
extern void		GizmoRegisterHelp(Widget, HelpInfo *);
#endif /* _Gizmo_h */
