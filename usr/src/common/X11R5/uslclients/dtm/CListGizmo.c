#pragma ident	"@(#)dtm:CListGizmo.c	1.29"

#include <Xm/ScrolledW.h>
#include <MGizmo/Gizmo.h>
#include "Dtm.h"
#include "extern.h"
#include "CListGizmo.h"

static Gizmo     CreateCListGizmo();
static void      FreeCListGizmo();
static XtPointer QueryCListGizmo();
static Widget 	 CListAttachment();

GizmoClassRec CListGizmoClass[] = {
	"CListGizmo",
	CreateCListGizmo,	/* Create	 */
	FreeCListGizmo,		/* Free		 */
	NULL,			/* Map		 */
	NULL,			/* Get		 */
	NULL,			/* Get Menu	 */
	NULL,			/* Dump	         */
	NULL,			/* Manipulate	 */
	QueryCListGizmo,	/* Query	 */
	NULL,			/* SetByFunction */
	CListAttachment		/* Get Attachment Widget */
};

static void
FreeCListGizmo (gizmo)
CListGizmo *	gizmo;
{
	int	i;

	XtFree(gizmo->name);
	XtFree(gizmo->name);

	DmCloseContainer(gizmo->cp, DM_B_NO_FLUSH);
	FREE((void *)gizmo);
}

static void
DmAddClass(Widget w, DmContainerPtr cp, char *name, DmFclassPtr fcp,
	Boolean viewable)
{
	DmObjectPtr op;

	if (!name)
		name = DmClassName(fcp);

	if ((op = Dm__NewObject(cp, name)) == NULL)
		return;

	if (viewable == False) {
		op->attrs |= DM_B_HIDDEN;
		op->fcp = fcp;
	}
	else {
		op->fcp = fcp;
		DmInitObjType(w, op);
	}
}

/*
 * Checks if a class should be included.
 * fmkp represents DmFnameKeyPtr too.
 */
static void
IncludeClass(Widget parent, CListGizmo *g, DmFmodeKeyPtr fmkp)
{
	/* include overridden file classes */
	if (!(g->overridden) && (fmkp->attrs & DM_B_OVERRIDDEN))
			return;

	/* If no data file or folder, check req_prop.  If req_prop
	 * is set and the property is not found, exclude the file type.
	 */
	if (g->req_prop && strcmp(fmkp->name, "DIR") &&
	    strcmp(fmkp->name, "DATA") &&
	    (DtGetProperty(&(fmkp->fcp->plist), g->req_prop, NULL) == NULL))
				return;

	DmAddClass(parent, g->cp, NULL, fmkp->fcp, True);
}

static Gizmo
CreateCListGizmo(Widget parent, CListGizmo *g, Arg arglist, int num_args)
{
	Dimension width, height, vpad, hpad;
	CListGizmo *clist;
	DmObjectPtr op;
	DmItemPtr ip;
	int n = 0;

	if ((clist = (CListGizmo *)CALLOC(1, sizeof(CListGizmo))) == NULL)
		return(NULL);

	if ((clist->cp =
	  (DmContainerPtr)calloc(1, sizeof(DmContainerRec))) == NULL)
		return(NULL);

	clist->cp->count    = 1;
	clist->cp->num_objs = 0;
	clist->name         = strdup(g->name);
	clist->req_prop     = g->req_prop ? strdup(g->req_prop) : NULL;
	clist->sys_class    = g->sys_class;
	clist->xenix_class  = g->xenix_class;
	clist->usr_class    = g->usr_class;
	clist->width        = g->width;
	clist->selectProc   = g->selectProc;
	clist->exclusives   = g->exclusives;
	clist->noneset      = g->noneset;
	clist->overridden   = g->overridden;
	clist->file         = g->file;
	
	if (g->sys_class != False) {
	  DmFmodeKeyPtr fmkp = DESKTOP_FMKP(Desktop);

	  for (; fmkp->ftype != DM_FTYPE_SEM; fmkp++)
	    IncludeClass(parent, clist, fmkp);
	}
	if (g->sys_class == False && g->xenix_class != False) {
	  DmFmodeKeyPtr fmkp = DESKTOP_FMKP(Desktop);

	  for (; fmkp->ftype != DM_FTYPE_UNK; fmkp++)
	    IncludeClass(parent, clist, fmkp);
	}

	if (g->usr_class != False) {
	  DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);

	  for (; fnkp; fnkp = fnkp->next)
	    if (fnkp->attrs & DM_B_CLASSFILE)
	    {
	      if (g->file)
	        /* Add fnkp entries as hidden objects. Used by Icon Setup. */
		DmAddClass(parent, clist->cp, fnkp->name, (DmFclassPtr)fnkp,
			False);
	      else
	        continue;
	    } else
	      IncludeClass(parent, clist, (DmFmodeKeyPtr)fnkp);
	}

	/* create scrolled window */
	XtSetArg(Dm__arg[0], XmNscrollingPolicy,        XmAPPLICATION_DEFINED);
	XtSetArg(Dm__arg[1], XmNvisualPolicy,           XmVARIABLE);
	XtSetArg(Dm__arg[2], XmNscrollBarDisplayPolicy, XmSTATIC);
	XtSetArg(Dm__arg[3], XmNshadowThickness,        2);

	clist->swinWidget = XtCreateManagedWidget("swin",
		xmScrolledWindowWidgetClass, parent, Dm__arg, 4);

	n = 0;
	XtSetArg(Dm__arg[n], XmNdrawProc, DmDrawIcon); n++;
	XtSetArg(Dm__arg[n], XmNmovableIcons,  False); n++;
	XtSetArg(Dm__arg[n], XmNexclusives, g->exclusives); n++;
	XtSetArg(Dm__arg[n], XmNnoneSet, g->noneset); n++;
	XtSetArg(Dm__arg[n], XmNselectProc, g->selectProc); n++;
	XtSetArg(Dm__arg[n], XmNgridHeight, GRID_HEIGHT(Desktop) / 2); n++;
	XtSetArg(Dm__arg[n], XmNgridWidth, GRID_WIDTH(Desktop) / 2);n++;
	XtSetArg(Dm__arg[n], XmNgridRows, 1); n++;
	XtSetArg(Dm__arg[n], XmNgridColumns, 6); n++;

	clist->boxWidget = DmCreateIconContainer(clist->swinWidget,
			DM_B_CALC_SIZE | DM_B_NO_INIT,
			Dm__arg, n,
			clist->cp->op, clist->cp->num_objs,
			&(clist->itp), clist->cp->num_objs,
			NULL);

	LayoutCListGizmo(clist, False);
	return((Gizmo)clist);
}

void
LayoutCListGizmo(CListGizmo *g, Boolean new_list)
{
	DmItemPtr ip;
	int i;
	WidePosition x;
	Dimension height, vpad, hpad;

	if (!(g->boxWidget)) {
		return;
	}

	vpad = XmConvertUnits(g->boxWidget, XmVERTICAL, Xm100TH_POINTS,
		2 * 100, XmPIXELS);
	hpad = XmConvertUnits(g->boxWidget, XmHORIZONTAL, Xm100TH_POINTS,
		4 * 100, XmPIXELS);
	height = GRID_HEIGHT(Desktop) + vpad;

	/* update each item's x,y so that they all line up as one row */
	for (ip=g->itp, i=g->cp->num_objs, x=(WidePosition)vpad; i; i--, ip++)
		if (ITEM_MANAGED(ip) != False) {
			ip->x = (XtArgVal)x;
			ip->y = (XtArgVal)(height - ITEM_HEIGHT(ip));
			x += ITEM_WIDTH(ip) + hpad;
		}

	XtSetArg(Dm__arg[0], XmNitemsTouched, True);
	XtSetArg(Dm__arg[1], XmNitems,        g->itp);
	XtSetArg(Dm__arg[2], XmNnumItems,     g->cp->num_objs);
	XtSetValues(g->boxWidget, Dm__arg, 3);
}

void
ChangeCListItemLabel(g, idx, label)
CListGizmo *g;
int idx;
char *label;
{
	DmItemPtr ip = g->itp + idx;

	free(ITEM_LABEL(ip));
	ip->label = (XtArgVal)strdup(label);
	DmSizeIcon(g->boxWidget, ip);

	LayoutCListGizmo(g, False);
}

void
ChangeCListItemGlyph(g, idx)
CListGizmo *g;
int idx;
{
	if (g->boxWidget) {
		DmItemPtr ip = g->itp + idx;
		DmObjectPtr op = ITEM_OBJ(ip);
		Dimension width, height;

		width = ITEM_WIDTH(ip);
		height = ITEM_HEIGHT(ip);
		if (op->fcp->glyph) {
		  DmReleasePixmap(XtScreen(g->boxWidget),op->fcp->glyph);
		}

		/* reset attributes */
		op->fcp->attrs &= ~(DM_B_VAR | DM_B_FREE);
		DmInitObjType(g->boxWidget, op);
		DmSizeIcon(g->boxWidget, ip);

		if ((width != ITEM_WIDTH(ip)) || (height != ITEM_HEIGHT(ip)))
			LayoutCListGizmo(g, False);
		else
			ExmFlatRefreshItem(g->boxWidget, idx, True);
	}
}

static XtPointer
QueryCListGizmo(CListGizmo *gizmo, int option, char * name)
{
   if (!strcmp(name, gizmo->name)) {
      switch(option) {
         case GetGizmoWidget:
            return (XtPointer)(gizmo->swinWidget);
            break;
         case GetGizmoGizmo:
            return (XtPointer)(gizmo);
            break;
         default:
            return (XtPointer)(NULL);
            break;
      }
   }
   else
      return (XtPointer)(NULL);

} /* end of QuerySWinGizmo */


static Widget
CListAttachment(CListGizmo *g)
{
	return g->swinWidget;
}	/* end of CListAttachment */
