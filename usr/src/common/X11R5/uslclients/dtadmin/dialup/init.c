#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/init.c	1.12"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <X11/Shell.h>
#include <RubberTile.h>
#include <Form.h>
#include <StaticText.h>
#include "uucp.h"
#include "error.h"

extern Widget	InitButtons();
extern Widget	InitLists();
extern void	InitFooter();
extern void	InitDevice();
extern void	InitializeIcon();

extern Boolean IsSystemFile();

void
initialize()
{
	Widget form;
        Widget topButtons;

	/* Create icon window	*/
	InitializeIcon();
	

/*	SetValue(sf->toplevel, XtNallowShellResize, False);*/
	form = XtVaCreateManagedWidget(
		"form",
		rubberTileWidgetClass,
		sf->toplevel,
		XtNorientation, OL_VERTICAL,
		XtNweight, 	0,
		/*XtNshadowThickness, 2,*/
		(String)0
	);
	topButtons = InitButtons (form);
	SetValue(topButtons, XtNgravity, NorthWestGravity);
	SetValue(topButtons, XtNshadowThickness, 1);
	SetValue(topButtons, XtNweight,  0);
	InitLists (form);
	InitFooter (form);
		/*SAMC*/
	XtRealizeWidget(sf->toplevel);
	InitDevice ();
} /* initialize */

void
InitializeIcon()
{
	Pixmap		icon,
			iconmask;
	DmGlyphPtr	glyph;

	glyph = DmGetPixmap (SCREEN, "modem48.icon");
	if (glyph) {
		icon = glyph->pix;
		iconmask = glyph->mask;
	} else
		icon = iconmask = (Pixmap) 0;

	XtVaSetValues(sf->toplevel,
		XtNiconPixmap, (XtArgVal) icon,
		XtNiconMask, (XtArgVal) iconmask,
		XtNiconName, (XtArgVal) GGT(string_appName),
		0
	);
}

void
InitFooter(parent)
Widget parent;
{
	Boolean	sys = IsSystemFile(parent);
	Widget footer;

	footer = XtVaCreateManagedWidget(
		"footer",
		staticTextWidgetClass,
		parent,
		XtNstring,		sys?sf->filename:df->filename,
		XtNgravity,		NorthWestGravity,
		XtNxResizable,		True,
		XtNxAttachRight,	True,
		XtNyAttachBottom,	True,
		XtNyVaryOffset,		True,
		XtNweight,		0,
		(String)0
	);
	if (sys)
		sf->footer = footer;
	else
		df->footer = footer;
} /* InitFooter */

