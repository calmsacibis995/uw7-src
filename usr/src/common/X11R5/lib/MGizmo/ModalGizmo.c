#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ModalGizmo.c	1.11"
#endif

#include <stdio.h> 

#include <Xm/MessageB.h>
#include <Xm/DialogS.h>
#include <Xm/MwmUtil.h>

#include "Gizmo.h"
#include "MenuGizmoP.h"
#include "ModalGizmP.h"

static Gizmo		CreateModalGizmo();
static Gizmo		SetModalValueByName();
static Gizmo		GetTheMenuGizmo();
static void		FreeModalGizmo();
static void		MapModalGizmo();
static void		DumpModalGizmo();
static XtPointer	QueryModalGizmo();

GizmoClassRec ModalGizmoClass[] = {
	"ModalGizmo",
	CreateModalGizmo,	/* Create */
	FreeModalGizmo,		/* Free */
	MapModalGizmo,		/* Map */
	NULL,			/* Get */
	GetTheMenuGizmo,	/* Get Menu */
	DumpModalGizmo,		/* Dump */
	NULL,			/* Manipulate */
	QueryModalGizmo,	/* Query */
	SetModalValueByName,	/* Set by name */
	NULL			/* Attachment */
};

#define MAX(a,b) (a>b)?a:b

/*
 * FreeModalGizmo
 *
 * The FreeModalGizmo procedure is used free the ModalGizmo gizmo.
 */
static void 
FreeModalGizmo(ModalGizmoP *g)
{
	FreeGizmoArray(g->gizmos, g->numGizmos);
	FreeGizmo(CommandMenuGizmoClass, (Gizmo)(g->menu));
	FREE(g);
}

static void
Fix(
	ModalGizmoP *gp, MenuGizmo *menu, int index, int numItems,
	char *callback, char *label
)
{
	XmString	str;
	XtCallbackRec cb[] = {
		{(XtCallbackProc)NULL, (XtPointer)NULL},
		{(XtCallbackProc)NULL, (XtPointer)NULL}
	};

	/* Skip over buttons that aren't in standard menu */
	if (numItems > 3 && index != 0) {
		index += numItems-3;
	}
	str = XmStringCreateLocalized(
		GGT(menu->items[index].label)
	);
	cb[0].callback = (XtCallbackProc)(
		menu->items[index].callback
	);
	cb[0].closure = menu->items[index].clientData;
	XtVaSetValues(gp->box, callback, cb, label, str, NULL);
	XmStringFree(str);
}

static void
SetMnemonicValue(Widget w, ModalGizmo *g, DmMnemonicInfo *mneInfo, int n)
{
	unsigned char *	mne;
	KeySym		ks;
	Arg		arg[10];

	mne = (unsigned char *)GGT(g->menu->items[n].mnemonic);
	(*mneInfo)[n].mne = mne;
	(*mneInfo)[n].mne_len = strlen((char *)mne);
	(*mneInfo)[n].op = DM_B_MNE_ACTIVATE_BTN | DM_B_MNE_GET_FOCUS;
	(*mneInfo)[n].w = w;
	/* Set the mnemonic for this item */
	if (
		mne != NULL &&
		(((ks=XStringToKeysym((char *)mne))!=NoSymbol) ||
		(ks = (KeySym)(mne[0]))) &&
		strchr(GGT(g->menu->items[n].label), (char)(ks & 0xff)) != NULL
	) {
		XtSetArg(arg[0], XmNmnemonic, ks);
	}
	XtSetValues(w, arg, 1);
}

/*
 * CreateModalGizmo
 *
 * The CreateModalGizmo function is used to create the Widget tree
 */
static Gizmo
CreateModalGizmo(Widget parent, ModalGizmo *g, Arg *args, int numArgs)
{
	MenuGizmo	copy;		/* Copy of menu */
	Widget		ok;
	Widget		cancel;
	Widget		help;
	ModalGizmoP *	gp;
	Arg		arg[100];
	int		i = 0;
	int		j;
	XmString	str;
	XmString	str1;
	long		dec;
	long		func;
	Dimension	width;
	Dimension	max = 0;
	XtWidgetGeometry	size;
	DmMnemonicInfo  mneInfo;

	gp = (ModalGizmoP *)CALLOC(1, sizeof(ModalGizmoP));

	gp->name = g->name;
	dec = MWM_DECOR_TITLE|MWM_DECOR_BORDER|MWM_DECOR_MENU;
	func = MWM_FUNC_MOVE|MWM_FUNC_CLOSE;
	XtSetArg(arg[i], XmNmwmDecorations, dec); i++;
	XtSetArg(arg[i], XmNmwmFunctions, func); i++;
	XtSetArg(arg[i], XmNallowShellResize, True); i++;
	gp->shell = XmCreateDialogShell(parent, "dialogShell", arg, i);

	i = 0;
	str1 = XmStringCreateLocalized(GGT(g->title));
	XtSetArg(arg[i], XmNdialogTitle, str1); i++;
	XtSetArg(arg[i], XmNdialogType, g->type); i++;
	XtSetArg(arg[i], XmNdialogStyle, g->style); i++;
	XtSetArg(arg[i], XmNautoUnmanage, g->autoUnmanage); i++;
	if (g->message != NULL) {
		str = XmStringCreateLtoR(
			GGT(g->message), XmFONTLIST_DEFAULT_TAG
		);
		XtSetArg(arg[i], XmNmessageString, str); i++;
	}
	i = AppendArgsToList(arg, i, args, numArgs);
	gp->box = XtCreateWidget(
		"messageBox", xmMessageBoxWidgetClass, gp->shell, arg, i
	);
	XmStringFree(str1);
	if (g->help != NULL) {
		GizmoRegisterHelp(gp->box, g->help);
	}
	if (g->message != NULL) {
		XmStringFree(str);
	}
	gp->numGizmos = g->numGizmos;
	gp->gizmos = CreateGizmoArray(
		gp->box, g->gizmos, g->numGizmos
	);

	/* Construct the menu from the default buttons and any new buttons */

	ok = XmMessageBoxGetChild(gp->box, XmDIALOG_OK_BUTTON);
	cancel = XmMessageBoxGetChild(gp->box, XmDIALOG_CANCEL_BUTTON);
	help = XmMessageBoxGetChild(gp->box, XmDIALOG_HELP_BUTTON);
	i = 0;
	if (g->menu != NULL) {
		for (i=0; g->menu->items[i].label; i++) {	/* count */
		}
		if (i > 2) {
			g->isHelp = False;
		}
		if (i > 0) {
			Fix(gp, g->menu, 0, i, XmNokCallback, XmNokLabelString);
		}
		if (i > 1) {
			char *	cb = XmNcancelCallback;
			char *	label = XmNcancelLabelString;

			if (g->isHelp == True) {
				cb = XmNhelpCallback;
				label = XmNhelpLabelString;
			}
			Fix(gp, g->menu, 1, i, cb, label);
		}
		if (i > 2) {
			Fix(
				gp, g->menu, 2, i,
				XmNhelpCallback, XmNhelpLabelString
			);
		}
		switch (i) {
			case 0: {
				XtUnmanageChild(ok);
				/* Fall thru */
			}
			case 1: {
				if (cancel != NULL) {
					XtUnmanageChild(cancel);
				}
				if (help != NULL) {
					XtUnmanageChild(help);
				}
				break;
			}
			case 2: {
				if (g->isHelp == False) {
					XtUnmanageChild(help);
				}
				else {
					XtUnmanageChild(cancel);
					cancel = help;
				}
				break;
			}
			case 3: {
				break;
			}
			default: {
				/* Copy the remaining buttons and */
				/* use these to create the remaining buttons */
				copy = *(g->menu);
				copy.items = (MenuItems *)CALLOC(
					1, sizeof(MenuItems)*(i-2)
				);
				for (j=1; j<i-2; j++) {
					copy.items[j-1] = g->menu->items[j];
				}
				gp->menu = _CreatePushButtons(
					gp->box, &copy, NULL, 0
				);
				FREE(copy.items);
				break;
			}
		}
		/* Finally, set the default button */
		if (g->menu->defaultItem == 0) {
			XtVaSetValues(
				gp->box,
				XmNdefaultButtonType, XmDIALOG_OK_BUTTON,
				NULL
			);
		}
		else {
			XtVaSetValues(
				gp->box,
				XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON,
				NULL
			);
		}
	}

	mneInfo = (DmMnemonicInfo)MALLOC(sizeof(DmMnemonicInfoRec)*i);

	/* The following code is here to fix a bug in the window manager. */
	/* Motif doesn't wrap buttons correctly that are too large for */
	/* the screen.  This code calculates the size of the menu at the */
	/* bottom of the popup and if it is larger than the screen then it */
	/* sets the size of the menu equal to the size of the screen.  This */
	/* seems to wrap the buttons correctly. */

	if (i > 0 && ok != NULL) {
		XtVaGetValues(ok, XmNwidth, &width, NULL);
		max = MAX(max, width);
		SetMnemonicValue(ok, g, &mneInfo, 0);
	}
	if (i > 1 && cancel != NULL) {
		XtVaGetValues(cancel, XmNwidth, &width, NULL);
		max = MAX(max, width);
		SetMnemonicValue(cancel, g, &mneInfo, i==2 ? 1 : i-2);
	}
	if (i > 2 && help != NULL) {
		XtVaGetValues(help, XmNwidth, &width, NULL);
		max = MAX(max, width);
		SetMnemonicValue(help, g, &mneInfo, i-1);
	}
	if (i > 3) {
		for (j=0; j<i-3; j++) {
			XtVaGetValues(
				gp->menu->items[j].button,
				XmNwidth, &width, NULL
			);
			max = MAX(max, width);
			SetMnemonicValue(
				gp->menu->items[j].button, g, &mneInfo, j+1
			);
		}
	}
	width = XDisplayWidth(XtDisplay(gp->box), 0);
	if (max*i > width*8/9) {
		XtVaSetValues(gp->box, XmNwidth, width*8/9, NULL);
	}

	XtManageChild(gp->box);
	if(i > 0) {
		DmRegisterMnemonic(gp->shell, mneInfo, i);
		FREE(mneInfo);
	}

	return gp;
}

/*
 * GetTheMenuGizmo
 */
static Gizmo
GetTheMenuGizmo(ModalGizmoP *g)
{
	return g->menu;
}

/*
 * MapModalGizmo
 */
static void
MapModalGizmo(ModalGizmoP *g)
{
	XtManageChild(g->box);
	XtPopup(g->shell, XtGrabNone);
}

/*
 reate

 */
static XtPointer
QueryModalGizmo(ModalGizmoP *g, int option, char *name)
{
	XtPointer	value = NULL;

	if (!name || strcmp(name, g->name) == 0) {
		switch(option) {
			case GetGizmoWidget: {
				return (XtPointer)(g->shell);
			}
			case GetGizmoGizmo: {
				return (XtPointer)g;
			}
			default: {
				return (XtPointer)NULL;
			}
		}
	}
	else {
		if (g->menu) {
			value = QueryGizmo(
				CommandMenuGizmoClass, g->menu, option, name
			);
		}
		if (value) {
			return value;
		}
		else {
			return (XtPointer)QueryGizmoArray(
				g->gizmos, g->numGizmos, option, name
			);
		}
	}
}

/*
 * GetModalGizmoDialogBox
 */
Widget
GetModalGizmoDialogBox(Gizmo g)
{
	return ((ModalGizmoP *)g)->box;
}

/*
 * GetModalGizmoShell
 */
Widget
GetModalGizmoShell(Gizmo g)
{
	return ((ModalGizmoP *)g)->shell;
}

/*
 * SetModalGizmoMessage
 * Set the XmNmessageString resource for the modal gizmo
 */
void
SetModalGizmoMessage(Gizmo g, char *message)
{
	Arg		arg[1];
	XmString	str;
	ModalGizmoP *	gp = (ModalGizmoP *)g;

	if (gp->box != NULL) {
/*
		str = XmStringCreateLocalized(GGT(message));
*/
		str = XmStringCreateLtoR(
			GGT(message), XmFONTLIST_DEFAULT_TAG);
		XtSetArg(arg[0], XmNmessageString, str);
		XtSetValues(gp->box, arg, 1);
		XmStringFree(str);
	}
}

static Gizmo
SetModalValueByName(ModalGizmoP *g, char *name, char *value)
{
	return SetGizmoArrayByName(
		g->gizmos, g->numGizmos, name, value
	);
}

/*
 * DumpModalGizmo
 * Dump the contents of the modal gizmo to stderr
 */
static void
DumpModalGizmo(ModalGizmoP *g, int indent)
{
	fprintf(stderr, "%*s", indent*4, " ");
	fprintf(stderr, "modal(%s) = 0x%x 0x%x\n", g->name, g, g->box);
	DumpGizmoArray(g->gizmos, g->numGizmos, indent+1);
	DumpGizmo(CommandMenuGizmoClass, g->menu, indent+1);
}
