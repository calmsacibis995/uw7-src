#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:TextGizmo.c	1.3"
#endif

#include <Xm/Text.h>
#include <sys/euc.h>
#include "_wchar.h"
#include <ctype.h>
#include "Gizmo.h"
#include "TextGizmoP.h"

static Gizmo		CreateTextGizmo();
static void		FreeTextGizmo();
static void		DumpTextGizmo();
static XtPointer	QueryTextGizmo();
static Gizmo		SetTextGizmoValueByName();
static Widget		AttachmentWidget();
static void 		getwidth(eucwidth_t *);

GizmoClassRec TextGizmoClass[] = {
	"TextGizmo",
	CreateTextGizmo,
	FreeTextGizmo,
	NULL,		/* map */
	NULL,		/* get */
	NULL,		/* get menu */
	DumpTextGizmo,
	NULL,		/* manipulate */
	QueryTextGizmo,
	SetTextGizmoValueByName,
	AttachmentWidget	/* Widget for attachments in base window */
};

/*
 * FreeTextGizmo
 */
static void 
FreeTextGizmo(TextGizmoP *g)
{
	FREE(g);
}

/*
 * CreateTextGizmo
 */
static Gizmo
CreateTextGizmo(Widget parent, TextGizmo *g, Arg *args, int num)
{
	eucwidth_t 	euc;
	TextGizmoP *	text;
	Arg		arg[100];
	int		i;
	int		code_width, columns;
	XtCallbackRec	cb[] = {
		{(XtCallbackProc)NULL, (XtPointer)NULL},
		{(XtCallbackProc)NULL, (XtPointer)NULL}
	};

	text = (TextGizmoP *)CALLOC(1, sizeof(TextGizmoP));
	text->name = g->name;

	getwidth(&euc);
	if (euc._multibyte) {
		code_width = euc._eucw1 > euc._eucw2 ? euc._eucw1 : euc._eucw2;
		code_width = code_width > euc._eucw3 ? code_width : euc._eucw3;
		columns = g->columns / code_width;
	} else
		columns = g->columns;

	i = 0;
	XtSetArg(arg[i], XmNcolumns, columns); i++;
	XtSetArg(arg[i], XmNrows, g->rows); i++;
	XtSetArg(arg[i], XmNvalue, g->text); i++;
	XtSetArg(arg[i], XmNeditMode, XmMULTI_LINE_EDIT); i++;
	if (g->callback != NULL) {
		cb[0].callback = (XtCallbackProc)g->callback;
		cb[0].closure = (XtPointer)g->clientData;
		XtSetArg(arg[i], XmNactivateCallback, cb); i++;
	}
	if (g->flags&G_RDONLY) {
		XtSetArg(arg[i], XmNeditable, False); i++;
	}
	if (g->flags&G_NOSHADOW) {
		XtSetArg(arg[i], XmNshadowThickness, 0); i++;
	}
	if (g->flags&G_NOBORDER) {
		XtSetArg(arg[i], XmNhighlightThickness, 0); i++;
	}
	i = AppendArgsToList(arg, i, args, num);
	text->textWidget = XtCreateManagedWidget(
		text->name, xmTextWidgetClass, parent,
		arg, i
	);
	if (g->help != NULL) {
		GizmoRegisterHelp(text->textWidget, g->help);
	}

	return (Gizmo)text;
}

static void
DumpTextGizmo(TextGizmoP *g, int indent)
{
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf(stderr, "text(%s) = 0x%x 0x%x\n", g->name, g, g->textWidget);
}

static XtPointer
QueryTextGizmo(TextGizmoP *g, int option, char *name)
{
	if (!name || strcmp(name, g->name) == 0) {
		switch(option) {
			case GetGizmoWidget: {
				return (XtPointer)(g->textWidget);
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)g;
				break;
			}
			default: {
				return (XtPointer)NULL;
			}
		}
	}
	else {
		return (XtPointer)NULL;
	}
}

void
SetTextGizmoText(Gizmo g, char *text)
{
	Arg		arg[10];

	XtSetArg(arg[0], XmNvalue, text);
	XtSetValues(((TextGizmoP *)g)->textWidget, arg, 1);
}

static Gizmo
SetTextGizmoValueByName(TextGizmoP *g, char *name, char *value)
{
	if (!name || strcmp(name, g->name) == 0) {
		SetTextGizmoText((Gizmo)g, value);
		return (Gizmo)g;
	}
	else {
		return NULL;
	}
}

char *
GetTextGizmoText(Gizmo g)
{
	Arg	arg[10];
	char *	text;

	XtSetArg(arg[0], XmNvalue, &text);
	XtGetValues(((TextGizmoP *)g)->textWidget, arg, 1);
	return text;
}

static Widget
AttachmentWidget(TextGizmoP *g)
{
	return g->textWidget;
}

static void
getwidth(eucwidth_t *eucstruct)
{
	eucstruct->_eucw1 = eucw1;
	eucstruct->_eucw2 = eucw2;
	eucstruct->_eucw3 = eucw3;
	eucstruct->_multibyte = multibyte;
	if (_ctype[520] > 3 || eucw1 > 2)
		eucstruct->_pcw = sizeof(unsigned long);
	else
		eucstruct->_pcw = sizeof(unsigned short);
	eucstruct->_scrw1 = scrw1;
	eucstruct->_scrw2 = scrw2;
	eucstruct->_scrw3 = scrw3;
}
