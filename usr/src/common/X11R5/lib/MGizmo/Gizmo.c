#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:Gizmo.c	1.10"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h> 
#include <DtI.h>
#include <Dt/Desktop.h>
#include <Xm/BulletinB.h>

#include "Gizmo.h"

	/* We don't need to worry about ARCHIVE because we only provide .so.
	 *
	 * We can place all related definitions here because only this
	 * file has references to libDt. When this assumption is no
	 * longer valid, we should move out this code, and place them
	 * in a header so that other C file(s) can easily do the same.
	 *
	 * And of course, dtsg_func_table will have to be a global rather
	 * than a static.
	 *
	 * Use Dtsg (Dt Stub in Gizmo) as prefix!
	 */
#if defined(DONT_USE_DT_FUNCS) && (defined(SVR4) || defined(sun)) /*dlopen etc*/

#include <dlfcn.h>

#define GET_DT_SYMS()		GetDtSyms()
#define DtsgEnqueueRequest	(*dtsg_func_table.enqueue_request)
#define DtsgSetAppId		(*dtsg_func_table.set_app_id)

typedef int    (*DtsgEnqueueRequest_type)(Screen *, Atom, Atom, Window,
								DtRequest *);
typedef Window (*DtsgSetAppId_type)(Display *, Window, char *);

typedef struct {
	DtsgEnqueueRequest_type	enqueue_request;
	DtsgSetAppId_type	set_app_id;
} DtsgFuncRec;

static  DtsgFuncRec		dtsg_func_table = { { NULL } };

	/* The code below is identical to libMDtI:DtiInitialize() */
static int
GetDtSyms(void)
{
#define NUM_DT_FUNC_NAMES	((int)(sizeof(dt_func_names) /		\
				       sizeof(dt_func_names[0])))

		/* Important: the order have to be in sync with DtsFuncRec */
	static const char * const	dt_func_names[] = {
		"DtEnqueueRequest",
		"DtSetAppId",
	};

	void *		handle;
	int		ret_code = 0;

		/* Should we do this? Should we dlclose() it afterward? */
	if (dtsg_func_table.enqueue_request)
		return(ret_code);

		/* Assume that the app will be linked with libMGizmo otherwise
		 * we will have to dlopen(libDt.so) if the else part was
		 * failed the first time. */
#ifdef RTLD_GLOBAL
	if ((handle = dlopen(NULL, RTLD_LAZY)) == (void *)NULL)
#else
	if ((handle = dlopen("libDt.so", RTLD_LAZY)) == (void *)NULL)
#endif
	{
		fprintf(stderr, "%s\n", dlerror());
		ret_code = 1;	/* dlopen problem */
	}
	else
	{
		typedef void	(*Func)(void);

		register int	i;
		Func *		func;

		func = (Func *)&dtsg_func_table.enqueue_request;
		for (i = 0; i < NUM_DT_FUNC_NAMES; i++)
		{
#define TAG		dt_func_names[i]

			if ((*func = (Func)dlsym(handle, TAG)) == NULL)
			{
					/* Either app didn't link with
					 * libDt or this Dt function name
					 * is changed */
				fprintf(stderr, "%s\n", dlerror());
				dlclose(handle);
				dtsg_func_table.enqueue_request = NULL;
				ret_code = 2; /* dlsym problem */
				break;
			}
			func++;
#undef TAG
		}
	}

	return(ret_code);
#undef NUM_DT_FUNC_NAMES
} /* end of GetDtSyms */

#else /* defined(DONT_USE_DT_FUNCS) && (defined(SVR4) || defined(sun)) */

#undef DONT_USE_DT_FUNCS	/* so that we can just use this symbol */

#define GET_DT_SYMS()		0
#define DtsgEnqueueRequest	DtEnqueueRequest
#define DtsgSetAppId		DtSetAppId

#endif /* defined(DONT_USE_DT_FUNCS) && (defined(SVR4) || defined(sun)) */

static char *		AppName		 = " ";
static char *		GizmoAppName	 = "*";	/* for Xdefaults */
static const char *	notfound	= "Message not found!!\n";

char * 
GetGizmoText(char *label)
{
	char *	p;
	char	c;

	if (label == NULL) {
		return (char *)notfound;
	}

	for (p = label; *p; p++) {
		if (*p == '\001') {
			c = *p;
			*p++ = 0;
			label = (char *)gettxt(label, p);
			*--p = c;
			break;
		}
	}
	return (label);
}

void
InitializeGizmos(char *applicationName)
{
	GizmoAppName = (char *)MALLOC(strlen(applicationName) + 2);
	strcpy(GizmoAppName, applicationName);
	strcat(GizmoAppName, "*");

	AppName = GGT(applicationName);

	(void)GET_DT_SYMS();	/* should we care about return code */
}

void
ManipulateGizmo(GizmoClass gizmoClass, Gizmo g, ManipulateOption option)
{
	if (gizmoClass->manipulateFunction) {
		(gizmoClass->manipulateFunction)(g, option);
	}
}

static void
MessageReceived(Widget w, XtPointer clientData, XtPointer callData)
{
	exit(0);
}

Widget
InitializeGizmoClient(
	char *name, char *class,
	XrmOptionDescRec *options, Cardinal numOptions,
	int *argc, char *argv[], 
	XtPointer base, XtResourceList resources, Cardinal numResources,
	ArgList args, Cardinal numArgs, char *resourceName
)
{
	Widget		shell;

	InitializeGizmos(name);

	shell = XtInitialize(name, class, options, numOptions, argc, argv);

	if (resourceName) {
		(void)DtsgSetAppId(
			XtDisplay(shell), XtWindow(shell), resourceName
		);
	}

	XtGetApplicationResources(
		shell, base, resources, numResources, args, numArgs
	);
	return shell;
}

/*
 * GizmoMainLoop
 */
void
GizmoMainLoop(
	void (*inputCB)(),
	XtPointer inputData, void (*otherCB)(), XtPointer otherData
)
{
	extern XtAppContext	_XtDefaultAppContext();
	XEvent			event;
	XtAppContext		appContext = _XtDefaultAppContext();

	while (True) {
		XtAppNextEvent(appContext, &event);
		if (
			inputCB &&
			(event.type == ButtonPress || event.type == KeyPress)
		) {
			(*inputCB)(
				XtWindowToWidget(
					event.xany.display, event.xany.window
				), otherData, &event
			);
		}
		else if (otherCB) {
			(*otherCB)(
				XtWindowToWidget(
					event.xany.display, event.xany.window
				), otherData, &event
			);
		}
		XtDispatchEvent(&event);
	}
}

/*
 * FreeGizmo
 */

void 
FreeGizmo(GizmoClass gizmoClass, Gizmo g)
{
	if (gizmoClass->freeFunction) {
		(gizmoClass->freeFunction)(g);
	}
}

/*
 * CreateGizmo
 */
Gizmo
CreateGizmo(
	Widget parent, GizmoClass gizmoClass, Gizmo g,
	ArgList arg, int num
)
{
	return (Gizmo)((gizmoClass->createFunction)(parent, g, arg, num));
}

/*
 * CreateGizmoArray
 */
GizmoArray
CreateGizmoArray(Widget parent, GizmoArray gizmos, int numGizmos)
{
	GizmoRec *	g;
	GizmoRec *	gr;
	GizmoRec *	retg;

	if (gizmos == NULL || numGizmos < 1) {
		return NULL;
	}
	gr = (GizmoArray) MALLOC(sizeof(GizmoRec) * numGizmos);
	retg = gr;
	for (g=gizmos; g<&gizmos[numGizmos]; g++){
		gr->gizmoClass = g->gizmoClass;
		gr->args = g->args;
		gr->numArgs = g->numArgs;
		gr->gizmo = CreateGizmo(
			parent, g->gizmoClass, g->gizmo, g->args, g->numArgs
		);
		gr += 1;
	}
	return retg;
}

Gizmo
SetGizmoValueByName(
	GizmoClass gizmoClass, Gizmo g, char *name, char *value
)
{
	if (gizmoClass->setByNameFunction != NULL) {
		return (gizmoClass->setByNameFunction)(g, name, value);
	}
	else {
		return NULL;
	}
}

Gizmo
SetGizmoArrayByName(GizmoArray gizmos, int numGizmos, char *name, char *value)
{
	GizmoArray	gp;
	Gizmo		g;

	if (numGizmos > 0) {
		for (gp=gizmos; gp<&gizmos[numGizmos]; gp++) {
			g = SetGizmoValueByName(
				gp->gizmoClass, gp->gizmo, name, value
			);
			if (g != NULL) {
				return g;
			}
		}
	}
	return NULL;
}

void
FreeGizmoArray(GizmoArray old, int num)
{
	GizmoArray	gp;

	if (num > 0) {
		for (gp=old; gp<&old[num]; gp++) {
			FreeGizmo(gp->gizmoClass, gp->gizmo);
		}
		FREE(old);
	}
}

void 
DumpGizmo(GizmoClass gizmoClass, Gizmo g, int indent)
{
	if (gizmoClass->dumpFunction) {
		(gizmoClass->dumpFunction)(g, indent);
	}
}

void
DumpGizmoArray(GizmoArray gizmos, int numGizmos, int indent)
{
	GizmoArray gp;

	if (numGizmos > 0) {
		for (gp=gizmos; gp < &gizmos[numGizmos]; gp++) {
			DumpGizmo(gp->gizmoClass, gp->gizmo, indent);
		}
	}
}

void
MapGizmo(GizmoClass gizmoClass, Gizmo g)
{
	if (gizmoClass->mapFunction) {
		(gizmoClass->mapFunction)(g);
	}
}

XtPointer
QueryGizmo(GizmoClass gizmoClass, Gizmo g, QueryOption option, char *name)
{
	if (gizmoClass->queryFunction) {
		return (gizmoClass->queryFunction)(g, option, name);
	}
	else {
		return NULL;
	}
}

/*
 * QueryGizmoArray
 */
XtPointer
QueryGizmoArray(GizmoArray gizmos, int numGizmos, int option, char *name)
{
	GizmoArray	gp;
	XtPointer	value = NULL;

	if (numGizmos > 0) {
		for (gp=gizmos; value==NULL && gp<&gizmos[numGizmos]; gp++) {
			 value = QueryGizmo(
				gp->gizmoClass, gp->gizmo, option, name
			);
		}
		return value;
	}
	else {
		return NULL;
	}
}

/*
 * The AppendArgsToList function is used to merge two arg lists
 * by simply appending the second additional list to the original
 * list.  Note that the routine presumes that the original list array is 
 * large enough to accommodate the addition.
 */
Cardinal
AppendArgsToList(
	ArgList original, Cardinal numOriginal,
	ArgList additional, Cardinal numAdditional
)
{
	int	i;
	int	j = numOriginal;

	for (i=0; i<numAdditional; i++) {
		original[j++] = additional[i];
	}

	return numOriginal + numAdditional;

}

Pixmap
PixmapOfFile(Widget w, char *fname, int *iwid, int *iht)
{
	Screen *	screen = XtScreen(w);
	DmGlyphRec *	gp;
	Pixel		pixel;
	static GC	gc = NULL;
	XGCValues	v;

	typedef struct _PixmapCache {
		struct _PixmapCache *	next;
		char *			fname;
		Pixmap			pixmap;
		DmGlyphRec *		gp;
	} PixmapCache;

	typedef struct _PixmapSizeCache {
		struct _PixmapSizeCache *	next;
		Screen *			screen;
		Dimension			width;
		Dimension			height;
		PixmapCache *			pixmaps;
	} PixmapSizeCache;

	static PixmapSizeCache *	pixmapSizeCache = NULL;
	PixmapCache *			currentPixmap;
	PixmapSizeCache *		p;

	for (p=pixmapSizeCache; p!=NULL; p=p->next) {
		if (p->screen == screen) {
			break;
		}
	}
	if (p == NULL) {
		XIconSize *	iconSizeHint;
		int		count;

		p = (PixmapSizeCache *)MALLOC(sizeof(PixmapSizeCache));
		p->next = pixmapSizeCache;
		p->screen = screen;
		if (
			XGetIconSizes(
				DisplayOfScreen(screen),
				RootWindowOfScreen(screen), 
			 	&iconSizeHint, &count
			) && count > 0
		) {
			p->width = iconSizeHint->max_width;
			p->height = iconSizeHint->max_height;
			FREE(iconSizeHint);
		}
		else {
			p->width = 70;
			p->height = 70;
		}
		currentPixmap = p->pixmaps = (PixmapCache *)MALLOC(
			sizeof(PixmapCache)
		);
		currentPixmap->next = NULL;
		currentPixmap->fname = STRDUP(fname);
		currentPixmap->pixmap = (Pixmap)NULL;
		currentPixmap->gp = (DmGlyphRec *)NULL;
		/*
		 * find out how big to make pixmaps for this screen
		 */
	}
	else
	{
		for (
			currentPixmap=p->pixmaps;
			currentPixmap!=NULL; 
			currentPixmap = currentPixmap->next
		) {
			if (strcmp(currentPixmap->fname, fname) == 0) {
				break;
			}
		}
		if (currentPixmap == NULL) {
			currentPixmap = (PixmapCache *)MALLOC(
				sizeof(PixmapCache)
			);
			currentPixmap->next = p->pixmaps;
			currentPixmap->fname = STRDUP(fname);
			currentPixmap->pixmap = (Pixmap)NULL;
			currentPixmap->gp = (DmGlyphRec *)NULL;
			p->pixmaps = currentPixmap;
		}
		else {
			*iwid = p->width;
			*iht = p->height;
			return currentPixmap->pixmap;
		}
	}

	gp = currentPixmap->gp = DmGetPixmap(screen, fname);
	if (!gp) {
		gp = currentPixmap->gp = DmGetPixmap(screen, NULL);
	}
	(void)DmMaskPixmap(w, gp);
	if (gc == NULL) {
		gc = XCreateGC(DisplayOfScreen(screen), gp->pix, 0, &v);
	}
	ExmFIconDrawIcon(
		w, gp, gp->pix, gc, True, NULL, 0, 0
	);
	currentPixmap->pixmap = gp->pix;

	*iwid = p->width;
	*iht = p->height;

	return currentPixmap->pixmap;
}

/*
 * LookupHelpType
 * 
 * The LookupHelpType function determines which desktop help type
 * is associated with the given section.  A null-string or the 
 * string "TOC" indicates that the type is DT_TOC_HELP, the string
 * "HelpDesk" indicates that the type is DT_OPEN_HELPDESK, otherwise the
 * type is set to DT_SECTION_HELP.
 */
static int
LookupHelpType(char *section)
{
	if (*section == '\0') {
		return (DT_TOC_HELP); /* backward compatibility */
	}
	else {
		if (strcmp(section, "TOC") == 0) {
			return DT_TOC_HELP;
		}
		else {
			if (strcmp(section, "HelpDesk") == 0) {
				return DT_OPEN_HELPDESK;
			}
			else {
				return DT_SECTION_HELP;
			}
		}
	}
}

/*
 * PostGizmoHelp
 * 
 * The PostGizmoHelp procedure is used to request the posting
 * of the help information for the given topic.  The HelpInfo
 * structure pointed to by help defines the topic.
 */
void
PostGizmoHelp(Widget w, HelpInfo *help)
{
	static DtDisplayHelpRequest	req;
	Display *			dpy;
	Screen *			scr;

	req.rqtype = DT_DISPLAY_HELP;
	req.serial = 0;
	req.version = 1;
	if (XtIsWidget(w) == False) {
		req.client = XtWindowOfObject(w);
		dpy = XtDisplayOfObject(w);
		scr = XtScreenOfObject(w);
	}
	else{
		req.client = XtWindow(w);
		dpy = XtDisplay(w);
		scr = XtScreen(w);
	}
	req.nodename = NULL;
	req.source_type = LookupHelpType(help->section);
	req.app_name = AppName;
	req.app_title = GGT(help->appTitle);
	req.title = GGT(help->title);
	req.help_dir = NULL;
	req.file_name = help->filename;
	req.sect_tag = help->section;

	(void)DtsgEnqueueRequest(
		scr, _HELP_QUEUE(dpy),
		_HELP_QUEUE(dpy), req.client, (DtRequest *)&req
	);
}

static void
GizmoHelpCB(Widget w, HelpInfo *help, XtPointer callData)
{
	PostGizmoHelp(w, help);
}

void
GizmoRegisterHelp(Widget w, HelpInfo *help)
{
	if (help->appTitle != NULL) {
		help->appTitle = GGT(help->appTitle);
	}
	if (help->title != NULL) {
		help->title = GGT(help->title);
	}
	XtAddCallback(
		w, XmNhelpCallback, (XtCallbackProc)GizmoHelpCB,
		(XtPointer)help
	);
}

Widget
GetAttachmentWidget(GizmoClass gizmoClass, Gizmo g)
{
	if (gizmoClass->attachmentFunction) {
		return (gizmoClass->attachmentFunction)(g);
	}
	return NULL;
}
