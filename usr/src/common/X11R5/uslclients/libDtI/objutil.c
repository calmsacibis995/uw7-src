#pragma ident	"@(#)libDtI:objutil.c	1.25"

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <FIconBox.h>
#include "DtI.h"

#ifndef MEMUTIL
extern char *strdup();
#endif /* MEMUTIL */

int
Dm__ObjectToIndex(flat, op)
Widget flat;
DmObjectPtr op;
{
	register int i;
	DmItemPtr ip;
	int num_items;

	XtSetArg(Dm__arg[0], XtNitems, &ip);
	XtSetArg(Dm__arg[1], XtNnumItems, &num_items);
	XtGetValues(flat, Dm__arg, 2);
	for (i=0; i < num_items; i++, ip++) {
		if ((ITEM_MANAGED(ip) != False) && (ITEM_OBJ(ip) == op))
			return(i);
	}
	return(OL_NO_ITEM);
}

int
Dm__ItemNameToIndex(ip, nitems, name)
register DmItemPtr ip;
int nitems;
char *name;
{
	register int i;

	for (i=0 ;i < nitems; i++, ip++)
		if (!strcmp(ITEM_LABEL(ip), name))
			return(i);
	return(OL_NO_ITEM);
}




void
DmCreateIconCursor(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	Pixmap		pixmap;
	Pixmap		pixmask;
	Cursor		cursor;
	Display *	dpy = XtDisplay(w);
	OlFlatDragCursorCallData *f_cursor =
			(OlFlatDragCursorCallData *)call_data;
	DmItemPtr	ip = ITEM_CD(f_cursor->item_data);
	DmObjectPtr	op = (DmObjectPtr)(ip->object_ptr);
	DmGlyphPtr	gp = op->fcp->glyph;
	XColor		fg;
	XColor		bg;
	static XColor	white;
	static XColor	black;
	static int	first = 1;

	if (first) {
		XColor junk;

		first--;
		XAllocNamedColor(dpy, DefaultColormapOfScreen(XtScreen(w)),
				"white", &white, &junk);
		XAllocNamedColor(dpy, DefaultColormapOfScreen(XtScreen(w)),
				"black", &black, &junk);
	}

	if (op->fcp->cursor) {
		pixmap  = op->fcp->cursor->pix;
		pixmask = op->fcp->cursor->mask;
		fg = black;
		bg = white;
	}
	else {
		GC		gc;
		XGCValues	values;

		pixmap = XCreatePixmap(dpy, RootWindowOfScreen(XtScreen(w)),
				(unsigned int)(gp->width),
				(unsigned int)(gp->height), 
				(unsigned int) 1);
		pixmask = pixmap;
		values.function	= GXcopyInverted;
		gc = XCreateGC(dpy, (Drawable)pixmap, GCFunction, &values);

		XCopyPlane(dpy, (gp->depth > 1) ? gp->mask : gp->pix,
			(Drawable)pixmap, gc, 0, 0,
			(unsigned int)(gp->width),
			(unsigned int)(gp->height),
			0, 0, (unsigned long)1);
		XFreeGC(dpy, gc);
		fg = white;
		bg = white;
	}

	/*
	 * Position of the cursor's hotspot relative to the origin of
	 * the icon.
	 */
	f_cursor->x_hot = ITEM_WIDTH(ip) / (Dimension)2;
	f_cursor->y_hot = gp->height / (Dimension)2;

	f_cursor->yes_cursor = XCreatePixmapCursor(dpy, pixmap, pixmask,
				&fg, &bg,
				gp->width / (Dimension)2,
				f_cursor->y_hot);

	f_cursor->static_cursor = False;

	if (!(op->fcp->cursor))
		XFreePixmap(dpy, pixmap);
}

void
DmInitObjType(w, op)
Widget w;
DmObjectPtr op;
{
	Screen *scrn = XtScreen(w);
	char *p;
	char *inst_iconfile; /* iconfile from instance property */
	char *iconfile = NULL;
	int special_icon = 0;

	if (op->fcp->attrs & DM_B_FREE)
		/* already initialized with a special glyph */
		return;

	/* get ICONFILE property */
	inst_iconfile = DmGetObjProperty(op, ICONFILE, NULL);
	p = DtGetProperty(&(op->fcp->plist), ICONFILE, NULL);

	/* a special icon */
	if (inst_iconfile) {
		if (p)
			special_icon = strcmp(inst_iconfile, p);
		else
			special_icon++;
	}

	/* put in guards against ICONFILE missing */
	if (p && op->fcp->glyph == NULL) {
		/* first check */
		iconfile = Dm__expand_sh(p, Dm__ExpandObjProp, (XtPointer)op);
		if (!strchr(p, '%')) {
			/* no variable substitution */
			op->fcp->glyph = DmGetPixmap(scrn, iconfile);
			if (op->fcp->glyph == NULL) {
				op->fcp->glyph  = DmGetPixmap(scrn, NULL);
				op->fcp->cursor = DmGetCursor(scrn, NULL);
			}
			else {
			    if (*iconfile != '/')
				op->fcp->cursor = DmGetCursor(scrn, iconfile);
			    else
				op->fcp->cursor = NULL;
			}
		}
		else {
			/* has variable substitution */
			char *dflt_icon;

			dflt_icon = DtGetProperty(&(op->fcp->plist),
					 DFLTICONFILE, NULL);
			if (dflt_icon) {
				op->fcp->glyph  = DmGetPixmap(scrn, dflt_icon);
				if (*dflt_icon != '/')
				 op->fcp->cursor = DmGetCursor(scrn, dflt_icon);
				else
				 op->fcp->cursor = NULL;
			}
			if (op->fcp->glyph == NULL) {
				op->fcp->glyph  = DmGetPixmap(scrn, NULL);
				op->fcp->cursor = DmGetCursor(scrn, NULL);
			}
			op->fcp->attrs |= DM_B_VAR;
		}
	}

	if ((op->fcp->attrs & DM_B_VAR) || special_icon) {
		DmFclassPtr fcp;
		DmGlyphPtr gp;

		if (special_icon) {
			if (iconfile) {
				free(iconfile);
				iconfile = NULL;
			}

			p = inst_iconfile;
		}

		if (iconfile == NULL)
			iconfile = Dm__expand_sh(p, Dm__ExpandObjProp,
						 (XtPointer)op);

		if (gp = DmGetPixmap(scrn, iconfile)) {
			/* got a special icon */
			fcp = DmNewFileClass(op->fcp->key);
			fcp->plist   = op->fcp->plist;
			fcp->attrs   = op->fcp->attrs;
			fcp->attrs  |= DM_B_FREE;
			fcp->attrs  &= ~DM_B_VAR;
			fcp->glyph   = gp;
			fcp->key     = op->fcp->key;
			op->fcp      = fcp;
			if (*iconfile != '/')
				fcp->cursor = DmGetCursor(scrn, iconfile);
			else
				fcp->cursor = NULL;
		}
	}

	/* In case no ICONFILE was defined */
	if (op->fcp->glyph == NULL) {
		op->fcp->glyph  = DmGetPixmap(scrn, NULL);
		op->fcp->cursor = DmGetCursor(scrn, NULL);
	}

	if (iconfile)
		free(iconfile);
}

DtPropPtr
DmFindObjProperty(op, attrs)
DmObjectPtr op;
DtAttrs attrs;
{
	static DmObjectPtr _op = NULL;
	static DtPropPtr _pp = NULL;
	static DtAttrs _attrs;
	static int instance = 1;
	static int first = 1;

	if (op) {
		_op      = op;
		_attrs   = attrs;
		_pp      = NULL;
		instance = 1;
		first    = 1;
	}

	if (instance) {
		if (first) {
			_pp = DtFindProperty(&(_op->plist), _attrs);
			first = 0;
		}
		else {
			_pp = DtFindProperty(NULL, _attrs);
		}
		if (_pp == NULL) {
			instance = 0;
			first = 1;
		}
	}

	if (!instance) {
		if (first) {
			_pp = DtFindProperty(&(_op->fcp->plist), _attrs);
			first = 0;
		}
		else {
			_pp = DtFindProperty(NULL, _attrs);
		}
	}
	return(_pp);
}

char *
DmGetObjProperty(op, name, attrs)
DmObjectPtr op;
char *name;
DtAttrs *attrs;
{
	char *ret = NULL;

	if (op) {
		ret = DtGetProperty(&(op->plist), name, attrs);
		if (!ret && op->fcp)
			ret = DtGetProperty(&(op->fcp->plist), name, attrs);
	}

	return(ret);
}

void
DmSetObjProperty(op, name, value, attrs)
DmObjectPtr op;
char *name;
char *value;
DtAttrs attrs;
{
	DtSetProperty(&(op->plist), name, value, attrs);
}

char *
Dm__ExpandObjProp(name, client_data)
char *name;
XtPointer client_data;
{
	DmObjectPtr op = (DmObjectPtr)client_data;
	char *p;
	char *real_path;
	char *real_name;
	char *free_this = NULL;

	p = DmGetObjProperty(op, name, NULL);
	if (!p && (name[0] && (name[1] == '\0'))) {
		/* one char variable */
		switch(*name) {
		case 'F':
			free_this = p = strdup(DmObjPath(op));
			break;
		case 'f':
			p = op->name;
			break;
		case 'L':
			if (DmResolveSymLink(DmObjPath(op), &real_path, &real_name))
				free_this = p = strdup(DmMakePath(real_path, real_name));
			else
				free_this = p = strdup(DmObjPath(op));
			break;
		case 'l':
			if (DmResolveSymLink(DmObjPath(op), &real_path, &real_name))
				p = real_name;
			else
				p = op->name;
			break;
		default:
			return(NULL);
		}
	}

	if (p) {
		p = Dm__expand_sh(p, Dm__ExpandObjProp, client_data);
		if (free_this)
			free(free_this);
		return(p);
	}
	else
		return(NULL);
}

char *
DmGetObjectName(op)
DmObjectPtr op;
{
	char *p;

	if (p = DmGetObjProperty(op, ICONLABEL, NULL))
		return(p);
	else
		return(op->name);
}

char *
DmGetObjectTitle(DmObjectPtr op)
{
	return DmGetObjProperty(op, "_ICONTITLE", NULL);
}

char *
Dt__Dm__expand_sh(char *p, XtPointer op)
{
	Dm__expand_sh(p, Dm__ExpandObjProp, (XtPointer)op);
}
