#pragma ident	"@(#)dtm:f_prop.c	1.90.1.1"

/******************************file*header********************************

    Description:
	This file contains the source code for folder-window file property
	sheets.
*/
						/* #includes go here	*/
#include <errno.h>
#include <grp.h>
#include <libgen.h>
#include <locale.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/ContGizmo.h>
#include <MGizmo/ChoiceGizm.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/PopupGizmo.h>
#include <MGizmo/InputGizmo.h>
#include <MGizmo/ComboGizmo.h>
#include <Xm/Xm.h>
#include <Xm/Protocols.h>

#include <DtWidget/ComboBox.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures
*/
					/* private procedures		*/
static void		ApplyCB(Widget, XtPointer, XtPointer);
static void		CreatePermButtons(mode_t, int);
static DmFPropSheetPtr	CreateSheet(DmFolderWinPtr window, DmItemPtr ip);
static void		PopdownPropSheet(Widget, XtPointer, XtPointer);
static void		PopdownCB(Widget, XtPointer, XtPointer);
static void		ResetCB(Widget, XtPointer, XtPointer);
static mode_t		UpdatePermissions(DmFPropSheetPtr fpsptr, int type);
static void		VerifyCB(Widget, XtPointer, XtPointer);
static void         	HelpCB(Widget, XtPointer, XtPointer);
static void		ReclassCB(Widget, XtPointer, XtPointer);
static void		ComboListSelectCB(Widget, XtPointer, XtPointer);
static void		UpdatePropSheetCB(Widget, XtPointer, XtPointer);
					/* public procedures		*/
void			Dm__PopupFilePropSheet(DmFolderWinPtr, DmItemPtr);
void			Dm__PopupFilePropSheets(DmFolderWinPtr window);
DmFPropSheetPtr		GetNewSheet(DmFolderWinPtr window);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* property type for the comment field */
#define COMMENT		"Comment"

/* file permissions categories */
#define OWNER	1
#define	GROUP	2
#define	OTHER	3
#define	ACCESS	4

static Arg label_args[] = {
        {XmNalignment, XmALIGNMENT_BEGINNING},
};

static Arg combo_args[] = {
        {XmNalignment, XmALIGNMENT_BEGINNING},
};

static InputGizmo prop_name = {
	NULL, "prop_name", "", 24, NULL, NULL
};

static GizmoRec prop_name_rec[] =  {
	{ InputGizmoClass, &prop_name },
};

static LabelGizmo prop_name_label = {
        NULL,                   /* help */
        "pname_label",          /* widget name */
        TXT_FP_FILE_NAME,       /* caption label */
        False,                  /* align caption */
        prop_name_rec,            /* gizmo array */
        XtNumber(prop_name_rec), /* number of gizmos */
};

static LabelGizmo prop_link = {
        NULL,                   /* help */
        "plink",          	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        NULL,            	/* gizmo array */
        NULL,			/* number of gizmos */
};

static GizmoRec prop_link_rec[] =  {
	{ LabelGizmoClass, &prop_link, label_args, 1 },
};

static LabelGizmo prop_link_label = {
        NULL,                   /* help */
        "plink_label",         	/* widget name */
        "",       		/* caption label */
        False,                 	/* align caption */
        prop_link_rec,          /* gizmo array */
        XtNumber(prop_link_rec),/* number of gizmos */
};

static InputGizmo prop_owner = {
	NULL, "prop_owner", "", 24, NULL, NULL
};

static GizmoRec prop_owner_rec[] =  {
	{ InputGizmoClass, &prop_owner },
};

static LabelGizmo prop_owner_label = {
        NULL,                   /* help */
        "powner_label",          /* widget name */
        TXT_OWNER,       /* caption label */
        False,                  /* align caption */
        prop_owner_rec,            /* gizmo array */
        XtNumber(prop_owner_rec), /* number of gizmos */
};

static InputGizmo prop_group = {
	NULL, "prop_group", "", 24, NULL, NULL
};

static GizmoRec prop_group_rec[] =  {
	{ InputGizmoClass, &prop_group },
};

static LabelGizmo prop_group_label = {
        NULL,                   /* help */
        "pgroup_label",          /* widget name */
        TXT_GROUP,       /* caption label */
        False,                  /* align caption */
        prop_group_rec,            /* gizmo array */
        XtNumber(prop_group_rec), /* number of gizmos */
};


static LabelGizmo prop_modtime = {
        NULL,                   /* help */
        "pmodtime",          	/* widget name */
        "",       		/* caption label */
        False,                 /* align caption */
        NULL,            /* gizmo array */
        NULL,		/* number of gizmos */
};

static GizmoRec prop_modtime_rec[] =  {
	{ LabelGizmoClass, &prop_modtime, label_args, 1 },
};

static LabelGizmo prop_modtime_label = {
        NULL,                   /* help */
        "pmodtime_label",          	/* widget name */
        TXT_MODTIME,       		/* caption label */
        True,                 /* align caption */
        prop_modtime_rec,            /* gizmo array */
        XtNumber(prop_modtime_rec),		/* number of gizmos */
};

static MenuItems OwnerMenuItems[] = {
	{ True, TXT_READ_PERM, NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, False},
	{ True, TXT_WRITE_PERM, NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, False},
	{ True, TXT_EXEC_PERM, NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, False},
	{0}
};

MenuGizmo OwnerMenu=
	{ NULL, "ownermenu", "ownermenu", OwnerMenuItems, NULL, NULL, XmHORIZONTAL, 1};

ChoiceGizmo owner_choice = { NULL, "ownerchoice", &OwnerMenu, G_TOGGLE_BOX};

static GizmoRec owner_choice_rec[] =  {
	{ ChoiceGizmoClass, &owner_choice },
};
static LabelGizmo owner_choice_label = {
        NULL,                   		/* help */
        "owner_choice_label",          		/* widget name */
        TXT_OWNER_ACCESS,       		/* caption label */
        False,                 			/* align caption */
        owner_choice_rec,            		/* gizmo array */
        XtNumber(owner_choice_rec),		/* number of gizmos */
};

static MenuItems GroupMenuItems[] = {
	{ True, TXT_READ_PERM, NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, False},
	{ True, TXT_WRITE_PERM, NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, False},
	{ True, TXT_EXEC_PERM, NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, False},
	{0}
};

MenuGizmo GroupMenu=
	{ NULL, "groupmenu", "groupmenu", GroupMenuItems, NULL, NULL, XmHORIZONTAL, 1};

ChoiceGizmo group_choice = { NULL, "groupchoice", &GroupMenu, G_TOGGLE_BOX};

static GizmoRec group_choice_rec[] =  {
	{ ChoiceGizmoClass, &group_choice },
};

static LabelGizmo group_choice_label = {
        NULL,                   		/* help */
        "group_choice_label",          		/* widget name */
        TXT_GROUP_ACCESS,       		/* caption label */
        False,                 			/* align caption */
        group_choice_rec,            		/* gizmo array */
        XtNumber(group_choice_rec),		/* number of gizmos */
};

static MenuItems OtherMenuItems[] = {
	{ True, TXT_READ_PERM, NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, False},
	{ True, TXT_WRITE_PERM, NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, False},
	{ True, TXT_EXEC_PERM, NULL, I_TOGGLE_BUTTON, NULL, NULL, NULL, False},
	{0}
};

MenuGizmo OtherMenu=
	{ NULL, "othermenu", "othermenu", OtherMenuItems, NULL, NULL, XmHORIZONTAL, 1};

ChoiceGizmo other_choice= { NULL, "otherchoice", &OtherMenu, G_TOGGLE_BOX};

static GizmoRec other_choice_rec[] =  {
	{ ChoiceGizmoClass, &other_choice },
};

static LabelGizmo other_choice_label = {
        NULL,                   /* help */
        "other_choice_label",          	/* widget name */
        TXT_OTHER_ACCESS,       		/* caption label */
        False,                 /* align caption */
        other_choice_rec,            /* gizmo array */
        XtNumber(other_choice_rec),		/* number of gizmos */
};

static LabelGizmo prop_class = {
        NULL,                   /* help */
        "pclass",          	/* widget name */
        "",       		/* caption label */
        False,                 /* align caption */
        NULL,            /* gizmo array */
        NULL,		/* number of gizmos */
};

static MenuItems ClassMenuItems[] = {
	{ True, TXT_UPDATE, NULL, I_PUSH_BUTTON, NULL, ReclassCB, NULL, False},
	{0}
};

static ComboBoxGizmo prop_combo = {
		NULL,					/* help */
		"class_combo",			/* widget name */
		NULL,					/* default item */
		NULL,					/* items              (fill in later) */
		0,						/* number of items    (fill in later) */
		0						/* visible item count (fill in later) */
};

MenuGizmo class_menu=
	{ NULL, "class_menu", "classmenu", ClassMenuItems, NULL, NULL, XmHORIZONTAL, 1};

static GizmoRec prop_class_subrec[] =  {
	{ ComboBoxGizmoClass, &prop_combo },
	{ CommandMenuGizmoClass, &class_menu },
};

ContainerGizmo prop_cont = {
		NULL,					/* help */
		"prop_cont",			/* widget name */
		G_CONTAINER_FORM,		/* container type */
		0,						/* width */
		0,						/* height */
		prop_class_subrec,		/* gizmo array */
		XtNumber(prop_class_subrec)/* number of gizmos */
};

static GizmoRec prop_class_rec[] =  {
	{ ContainerGizmoClass, &prop_cont, label_args, 1 },
};

static LabelGizmo prop_class_label = {
        NULL,                   /* help */
        "pclass_label",          	/* widget name */
        TXT_ICON_CLASS,       		/* caption label */
        False,                 /* align caption */
        prop_class_rec,            /* gizmo array */
        XtNumber(prop_class_rec),		/* number of gizmos */
};

static InputGizmo prop_comment = {
	NULL, "prop_comment", "", 24, NULL, NULL
};

static GizmoRec prop_comment_rec[] =  {
	{ InputGizmoClass, &prop_comment },
};

static LabelGizmo prop_comment_label = {
        NULL,                   /* help */
        "pcomment_label",          /* widget name */
        TXT_COMMENTS,       /* caption label */
        False,                  /* align caption */
        prop_comment_rec,            /* gizmo array */
        XtNumber(prop_comment_rec), /* number of gizmos */
};

static GizmoRec PropertyPreRec[] = {
	{LabelGizmoClass, &prop_name_label},
	{LabelGizmoClass, &prop_link_label},
	{LabelGizmoClass, &prop_owner_label},
	{LabelGizmoClass, &prop_group_label},
	{LabelGizmoClass, &prop_modtime_label},
	{LabelGizmoClass, &owner_choice_label},
	{LabelGizmoClass, &group_choice_label},
	{LabelGizmoClass, &other_choice_label},
	{LabelGizmoClass, &prop_class_label},
	{LabelGizmoClass, &prop_comment_label},
};

static ContainerGizmo PropertyContainer = {
        NULL, "PropertyContainer", G_CONTAINER_RC,
        NULL, NULL, PropertyPreRec, XtNumber (PropertyPreRec),
};

static GizmoRec PropertyRec[] = {
	{ContainerGizmoClass, &PropertyContainer}
};

/* Define the menu */

static MenuItems MenubarItems[] = {
        MENU_ITEM( TXT_APPLY,   TXT_M_APPLY,    ApplyCB ),
        MENU_ITEM( TXT_Reset,   TXT_M_Reset,    ResetCB ),
        MENU_ITEM( TXT_CANCEL,  TXT_M_CANCEL,   PopdownPropSheet),
        MENU_ITEM( TXT_HELP,    TXT_M_HELP,     HelpCB ),
        { NULL }
};
MENU_BAR("PropMenubar", Menubar, NULL, 0, 2);        /* default: Apply */

static PopupGizmo PropertyWindow = {
        NULL, 					/* help */
        "prop_popup",				/* widget name */
        TXT_EDIT_PROP_TITLE, 			/* title */
        &Menubar,           			/* menu */
        PropertyRec, 				/* gizmo array */
        XtNumber(PropertyRec),    		/* number of gizmos */
	NULL,					/* No footer for file props */
};


static MenuItems DosMenuItems[] = {
	{ True, TXT_READ_ONLY, NULL, I_RADIO_BUTTON, NULL, NULL, NULL, False},
	{ True, TXT_READ_WRITE, NULL, I_RADIO_BUTTON, NULL, NULL, NULL, False},
	{0}
};
MenuGizmo DosMenu=
	{ NULL, "dosmenu", "dosmenu", DosMenuItems, NULL, NULL, XmHORIZONTAL, 1};

ChoiceGizmo dos_choice = { NULL, "doschoice", &DosMenu, G_TOGGLE_BOX};
static GizmoRec dos_choice_rec[] =  {
	{ ChoiceGizmoClass, &dos_choice },
};


static LabelGizmo dos_access_label = {
        NULL,                   		/* help */
        "owner_choice_label",          		/* widget name */
        TXT_ACCESS,       		/* caption label */
        False,                 			/* align caption */
        dos_choice_rec,            		/* gizmo array */
        XtNumber(dos_choice_rec),		/* number of gizmos */
};

static GizmoRec DosPropertyPreRec[] = {
	{LabelGizmoClass, &prop_name_label},
	{LabelGizmoClass, &prop_owner_label},
	{LabelGizmoClass, &prop_modtime_label},
	{LabelGizmoClass, &dos_access_label},
	{LabelGizmoClass, &prop_class_label},
	{LabelGizmoClass, &prop_comment_label},
};

static ContainerGizmo DosPropertyContainer = {
        NULL, "PropertyContainer", G_CONTAINER_RC,
        NULL, NULL, DosPropertyPreRec, XtNumber (DosPropertyPreRec),
};

static GizmoRec DosPropertyRec[] = {
	{ContainerGizmoClass, &DosPropertyContainer}
};
static PopupGizmo DosPropertyWindow = {
        NULL, 					/* help */
        "prop_popup",				/* widget name */
        TXT_EDIT_PROP_TITLE, 			/* title */
        &Menubar,           			/* menu */
        DosPropertyRec, 				/* gizmo array */
        XtNumber(DosPropertyRec),    		/* number of gizmos */
	NULL,					/* No footer for file props */
};



/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    GetNewSheet- function allocates a new file property structure.
	Since more than one file property sheets are allowed, for
	optimization purpose we are reusing the file property structure.
	Available struct is marked with prop_num == -1
*/
DmFPropSheetPtr
GetNewSheet(DmFolderWinPtr window)
{

    DmFPropSheetPtr fpsptr, last;

    if (window->fpsptr == NULL)
    {
	fpsptr = (DmFPropSheetPtr)CALLOC(1, sizeof(DmFPropSheetRec));
	window->fpsptr = fpsptr;
	fpsptr->next = NULL;

	return(fpsptr);
    }

    for(fpsptr = window->fpsptr; fpsptr->next; fpsptr = fpsptr->next)
	if(fpsptr->prop_num == -1)
	    return(fpsptr);

    /* check for the last one, this way last entry is handy in
       case we need at next lines
       */
    if (fpsptr->prop_num == -1)
	return(fpsptr);

    last = fpsptr;
    fpsptr = (DmFPropSheetPtr)CALLOC(1, sizeof(DmFPropSheetRec));
    last->next = fpsptr;
    fpsptr->next = NULL;
    return(fpsptr);

}				/* End of GetNewSheet */


/****************************procedure*header*****************************
    CreatePermButtons() function creates permission buttons for owner,
	group and other categories and sets appropriate state based on the
	permission on the file.
*/
static void
CreatePermButtons(mode_t mode, int type)
{
    switch(type)
    {
    case OWNER:
	OwnerMenuItems[0].set = (mode & S_IRUSR ? True : False);
	OwnerMenuItems[1].set = (mode & S_IWUSR ? True : False);
	OwnerMenuItems[2].set = (mode & S_IXUSR ? True : False);
	break;
    case GROUP:
	GroupMenuItems[0].set = (mode & S_IRGRP ? True : False);
	GroupMenuItems[1].set = (mode & S_IWGRP ? True : False);
	GroupMenuItems[2].set = (mode & S_IXGRP ? True : False);
	break;
    case OTHER:
	OtherMenuItems[0].set = (mode & S_IROTH ? True : False);
	OtherMenuItems[1].set = (mode & S_IWOTH ? True : False);
	OtherMenuItems[2].set = (mode & S_IXOTH  ? True : False);
	break;
    }

}				/* End of CreatePermButtons */

static int
FindClass(char **list, int cnt, char *name)
{
	char **pp;

	for (pp=list; cnt && *pp; pp++, cnt--)
		if (!strcmp(*pp, name))
			return(pp - list);
	return(-1);
}

static void
FreeClassList(char **list, int cnt)
{
	char **pp;

	if (list) {
		for (pp=list; cnt && *pp; cnt--, pp++)
			free(*pp);

		free(list);
	}
}				/* End of FreeClassList */

/*
 * This flag indicates whether to use strcoll for non-C locale and strcmp
 * for C locale.
 */
static int use_strcoll;

/*
 * This function inserts a new entry to the class list. We can't build
 * the list and then sort it, because there are two parallel arrays.
 * Returns:
 *	-1	duplicate entry
 *	 0	OK
 *	 1	error
 */
static int
InsertList(char **list1, char **list2, char *name1, char *name2)
{
	int result;

	for (; *list1; list1++, list2++) {
		if (use_strcoll) {
			char *tmp1 = strdup(name1);
			char *tmp2 = strdup(*list1);

			result = strcoll(tmp1, tmp2);
			free(tmp2);
			free(tmp1);
		}
		else {
			result = strcmp(name1, *list1);
		}

		if (!result)
			/* class is already in the list */
			return(-1);

		if (result < 0)
			/* found the insertion point */
			break;
	} /* for */

	/* duplicate the names first */
	name1 = strdup(name1);
	name2 = strdup(name2);

	if (!name1 || !name2)
		return(1);

	if (*list1) {
		/* found the insertion pt. in the middle of the list */
		char *tmp1;
		char *tmp2;

		/* insert the entry and shift the rest of the list */
		do {
			tmp1 = *list1;
			tmp2 = *list2;
			*list1++ = name1;
			*list2++ = name2;
			name1 = tmp1; /* use as tmp var */
			name2 = tmp2; /* use as tmp var */
		} while (*list1);

		*list1 = name1;
		*list2 = name2;
	}
	else {
		/* entry not found. Insert it at the end of the list */
		*list1 = name1;
		*list2 = name2;
	} /* else */

	return(0); /* success */
}

/*
 * BuildClassList builds an array of class names.
 * If ftype == 0, it means all classes.
 */
static char **
BuildClassList(DmFileType ftype, char ***intClassListPtr, int *numItems)
{
	register DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);
	DmFmodeKeyPtr fmkp = DESKTOP_FMKP(Desktop);
	char **classList; /* class list with nice names */
	char **intClassList; /* class list with internal names */
	char **clp, **iclp; /* temp vars */
	char *name;
	int cnt;
	int ret;
	DmFileType extratype;

	/* prepare for the worst */
	*numItems = 0;
	*intClassListPtr = NULL;

	/* count the number of classes */
	for (cnt=0; fnkp; fnkp=fnkp->next) {
		if (fnkp->attrs & (DM_B_NEW | DM_B_CLASSFILE |
				 DM_B_OVERRIDDEN | DM_B_INACTIVE_CLASS))
			continue;
		if (!ftype || !(fnkp->ftype) || (fnkp->ftype == ftype))
			cnt++;
	}

	/*
	 * If the type is data, add one for exec. And vice versa. This
	 * is done to make it easy for user to switch from data to exec.
	 */
	if (ftype == DM_FTYPE_EXEC) {
		extratype = DM_FTYPE_DATA;
		cnt++;
	}
	else if (ftype == DM_FTYPE_DATA) {
		extratype = DM_FTYPE_EXEC;
		cnt++;
	}
	else
		extratype = 0;
	
	/* Do we really need to loop? isn't it always one? */
	for (; fmkp->name; fmkp++) { /* internal classes */
		if (!ftype || !(fmkp->ftype) || (fmkp->ftype == ftype))
			cnt++;
	}


	if (!(classList = calloc(sizeof(char *), cnt)))
		return(NULL);
	if (!(intClassList = calloc(sizeof(char *), cnt))) {
		free(classList);
		return(NULL);
	}

	/* set use_strcoll for sorting the list */
	if (strcmp(setlocale(LC_COLLATE, NULL), "C"))
		use_strcoll = True;
	else
		use_strcoll = False;

	for (fnkp=DESKTOP_FNKP(Desktop); fnkp; fnkp=fnkp->next) {
		if (fnkp->attrs & (DM_B_NEW | DM_B_CLASSFILE |
							 DM_B_OVERRIDDEN | DM_B_INACTIVE_CLASS))
			continue;

		if (ftype && (fnkp->ftype) && (fnkp->ftype != ftype))
			continue;

		name = DtGetProperty(&(fnkp->fcp->plist), CLASS_NAME, NULL);
		if (name)
			name = GetGizmoText(name);
		else
			name = fnkp->name; /* default */

		ret = InsertList(classList, intClassList, name, fnkp->name);
		if (ret > 0) {
			/* one of the mallocs failed */
			FreeClassList(classList, cnt);
			FreeClassList(intClassList, cnt);
			return(NULL);
		}
		if (ret == -1)
			cnt--; /* duplicate entry */
	} /* for */

	/* add internal classes to the list */
	for (fmkp=DESKTOP_FMKP(Desktop); fmkp->name; fmkp++) {
		if (ftype && (fmkp->ftype) && (fmkp->ftype != ftype) &&
			(!extratype || (fmkp->ftype != extratype)))
			continue;

		name = DtGetProperty(&(fmkp->fcp->plist), CLASS_NAME, NULL);
		if (name)
			name = GetGizmoText(name);
		else
			name = (char *)(fmkp->name); /* default */

		ret = InsertList(classList, intClassList, name, (char*)(fmkp->name));
		if (ret > 0) {
			/* one of the mallocs failed */
			FreeClassList(classList, cnt);
			FreeClassList(intClassList, cnt);
			return(NULL);
		}
		if (ret == -1)
			cnt--; /* duplicate entry */
	} /* for */

	*numItems = cnt;
	*intClassListPtr = intClassList;

	return(classList);
}				/* End of BuildClassList */

/*
 * CreateSheet() function the file property sheet for the item
 * pointed to bt 'ip'.
 */
static DmFPropSheetPtr
CreateSheet(DmFolderWinPtr window, DmItemPtr ip)
{

    static int prop_num = 1;
    Widget	popup_shell;
    DmFPropSheetPtr	fpsptr;
    int	owner;
    struct passwd *pw;
    struct group *gr;
    XtArgVal *p;
    char *name;
    char *alt_name;
    int len;
    char nlinks[32];
    char buffer[BUFSIZ];
    Boolean unmanage_link = True;
    int ftype;
    DtAttrs attrs;
    Boolean reclass;
    Widget w;

    fpsptr		= GetNewSheet(window);
    fpsptr->prop_num	= prop_num++;
    fpsptr->flag	= False;
    fpsptr->window	= window;
    fpsptr->item	= ip;


    prop_name.text = fpsptr->prev = strdup((ITEM_OBJ(ip))->name);

/* PWF - DO WE NEED TO SUPPORT ALTNAME NOW THAT ICONLABEL IS GOING AWAY
		Assumuption: No longer a need to support AltName
*/

   ftype = FILEINFO_PTR(ip)->fstype;
    /* display link info */
    if (ftype != DOSFS_ID(Desktop)) {
	    if (ITEM_OBJ(ip)->attrs & DM_B_SYMLINK) {
			/* display symbolic link info */
			int len;

			len = readlink(DmObjPath(ITEM_OBJ(ip)), buffer, BUFSIZ);
			buffer[len] = '\0'; /* readlink doesn't do this */
			prop_link_label.label = TXT_FPROP_SYMLINK;
			prop_link.label = buffer;
			unmanage_link = False;
	    } else if (ITEM_OBJ(ip)->ftype != DM_FTYPE_DIR) {
			/* display hard link info */

			sprintf(nlinks, "%d", FILEINFO_PTR(ip)->nlink);
			prop_link_label.label = TXT_FPROP_HARDLINK;
			prop_link.label = nlinks;
			unmanage_link = False;

	    }
    	    /*
     	     * create file, group input fileds
	     * do not need to worry about 'gr' being NULL  here
	     */
	    if (! (gr = getgrgid(FILEINFO_PTR(ip)->gid)) ) {
		sprintf(Dm__buffer, "%d", FILEINFO_PTR(ip)->gid);
		name = Dm__buffer;
	    } else
		name = gr->gr_name;

	    prop_group.text = fpsptr->prev_grp = strdup(name);
     }

    /*
     * create file, owner input fields
     * do not need to worry about 'pw' being NULL here
     */
    if (! (pw = getpwuid(FILEINFO_PTR(ip)->uid)) ) {
	sprintf(Dm__buffer, "%d", FILEINFO_PTR(ip)->uid);
	name = Dm__buffer;
    } else
	name = pw->pw_name;

    prop_owner.text 	= fpsptr->prev_own = strdup(name);


    /*
     * create static text for modification time - watch out for ctime()
     * return string
     */
    (void)strftime(Dm__buffer, sizeof(Dm__buffer), TIME_FORMAT,
	     localtime(&FILEINFO_PTR(ip)->mtime));

    prop_modtime.label 	= Dm__buffer;

    if (ftype != DOSFS_ID(Desktop)) {
	    /* create permission buttons, dimming 'em as appropriate */
	    CreatePermButtons(FILEINFO_PTR(ip)->mode, OWNER);
	    CreatePermButtons(FILEINFO_PTR(ip)->mode, GROUP);
	    CreatePermButtons(FILEINFO_PTR(ip)->mode, OTHER);

    } else {
	/* Dosfs file */
	if (FILEINFO_PTR(ip)->mode & S_IWOTH) {
		DosMenuItems[0].set = False;
		DosMenuItems[1].set = True;
	} else {
		DosMenuItems[0].set = True;
		DosMenuItems[1].set = False;
	}
   }

	/* check/set sensitivity of reclass button */
    reclass = True;
    if (DmGetObjProperty(ITEM_OBJ(ip), OBJCLASS, &attrs)) {
    	if (attrs & DT_PROP_ATTR_LOCKED)
    		reclass = False;
    }

	prop_combo.defaultItem = DmObjClassName(ITEM_OBJ(ip));

    if (reclass) {
    	/* build class item list */
	/* For now, display the full list, because a filtered list may not
	 * have the new class in it after the Reclass button is hit.
	 */
    	prop_combo.items = BuildClassList(0,
/*    			((DmFnameKeyPtr)(ITEM_OBJ(ip)->fcp->key))->ftype,*/
    						&(fpsptr->intClassList),
    						&prop_combo.numItems);
    	prop_combo.visible = 4;
    	fpsptr->list_count = prop_combo.numItems;
    }
    else {
    	/* create an empty list */
    	prop_combo.numItems = 1;
    	prop_combo.items = malloc(sizeof(char *) * 1);
    	prop_combo.items[0] = strdup(prop_combo.defaultItem);
    	prop_combo.visible = 1;
    	fpsptr->intClassList = NULL;
    	fpsptr->list_count = 0;
    } /* else */
	ClassMenuItems[0].sensitive = reclass;
	ClassMenuItems[0].clientData = (char *)fpsptr;

	/* save initial internal class name */
	fpsptr->prev_class = ((DmFnameKeyPtr)(ITEM_OBJ(ip)->fcp->key))->name;

    /* create comments field  */
    fpsptr->prev_cmt 	= DmGetObjProperty(ITEM_OBJ(ip), COMMENT, NULL);
    fpsptr->prev_cmt 	= (fpsptr->prev_cmt) ?
					strdup(fpsptr->prev_cmt) : strdup("");
    prop_comment.text	= fpsptr->prev_cmt;

    ((MenuGizmo *)(PropertyWindow.menu))->clientData = (char *) fpsptr;

    if (ftype != DOSFS_ID(Desktop)) {
    	if (fpsptr->shell = CreateGizmo(window->shell, PopupGizmoClass,
					&PropertyWindow, NULL, 0)) {
		Widget classButton, classCombo;

		classButton = (Widget)QueryGizmo(PopupGizmoClass,
					fpsptr->shell, GetGizmoWidget,
					"class_menu");
		classCombo = (Widget)QueryGizmo(PopupGizmoClass,
					fpsptr->shell, GetGizmoWidget,
					"class_combo");
		XtSetArg(Dm__arg[0], XmNleftAttachment, XmATTACH_WIDGET);
		XtSetArg(Dm__arg[1], XmNleftWidget, classCombo);
		XtSetValues(classButton, Dm__arg, 2);
	}
    }
    else
    	fpsptr->shell 	= CreateGizmo(window->shell, PopupGizmoClass,
					&DosPropertyWindow, NULL, 0);
    popup_shell 	= GetPopupGizmoShell(fpsptr->shell);

    /*  We will handle Close requests ourself.  */
    XmAddWMProtocolCallback( popup_shell,
			    XA_WM_DELETE_WINDOW(XtDisplay(popup_shell)),
			    PopdownPropSheet, (XtPointer) fpsptr ) ;
    XtSetArg(Dm__arg[0], XmNdeleteResponse, XmDO_NOTHING);
    XtSetValues(popup_shell, Dm__arg, 1 );
    XtAddCallback(popup_shell, XmNpopdownCallback, PopdownCB,
		  (XtPointer) fpsptr);

    /* register for context-sensitive help */
    XtAddCallback(GetPopupGizmoRowCol(fpsptr->shell), XmNhelpCallback, HelpCB,
	(XtPointer)fpsptr);

    if (ftype == DOSFS_ID(Desktop)) {
	Widget prop_owner = (Widget)QueryGizmo(PopupGizmoClass,fpsptr->shell,
						  GetGizmoWidget, "prop_owner");
	XtSetSensitive(prop_owner, False);
    } else {
	if (!(owner = (getuid() == FILEINFO_PTR(ip)->uid))) {
		/* set textfields sensitivity based on access() and uid */
		Widget prop_name = (Widget)QueryGizmo(PopupGizmoClass,
					fpsptr->shell, GetGizmoWidget,
					"prop_name");
		Widget prop_owner = (Widget)QueryGizmo(PopupGizmoClass,
					fpsptr->shell, GetGizmoWidget,
					"prop_owner");
		Widget prop_group = (Widget)QueryGizmo(PopupGizmoClass,
					fpsptr->shell, GetGizmoWidget,
					"prop_group");
		Widget prop_cmt = (Widget)QueryGizmo(PopupGizmoClass,
					fpsptr->shell, GetGizmoWidget,
					"prop_comment");
		XtSetSensitive(prop_name,  False);
		XtSetSensitive(prop_owner, False);
		XtSetSensitive(prop_group, False);
		XtSetSensitive(prop_cmt,   False);

	}

	/*
	 * Cannot change value of file name from Found Window
	 */
	if ((window->attrs & DM_B_FOUND_WIN) 
		&& (owner = (getuid() == FILEINFO_PTR(ip)->uid))) {

		Widget prop_name = (Widget)QueryGizmo(PopupGizmoClass,
					fpsptr->shell, GetGizmoWidget,
					"prop_name");
		XtSetSensitive(prop_name,  False);
	}

    	if (unmanage_link) {
		Widget prop_link = (Widget)QueryGizmo(PopupGizmoClass,
					fpsptr->shell, GetGizmoWidget,"plink");
		Widget prop_link_lab = (Widget)QueryGizmo(PopupGizmoClass,
						fpsptr->shell, GetGizmoWidget,
						"plink_label");

		XtUnmanageChild(prop_link_lab);
		XtUnmanageChild(prop_link);
    	}
    }

    /* make the class container gizmo into 2 columns */
	w = (Widget)QueryGizmo(PopupGizmoClass, fpsptr->shell, GetGizmoWidget,
					"prop_cont");
	XtSetArg(Dm__arg[0], XmNnumColumns, 2);
	XtSetArg(Dm__arg[1], XmNorientation, XmVERTICAL);
	XtSetArg(Dm__arg[2], XmNpacking, XmPACK_COLUMN);
	XtSetValues(w, Dm__arg, 3);

	/* attach select callback to class combo list */
	w = (Widget)QueryGizmo(PopupGizmoClass, fpsptr->shell, GetGizmoWidget,
					"class_combo");
	w = XtNameToWidget(w, "*List");	/* UNDOCUMENTED ASSUMPTION!!! */
	XtSetArg(Dm__arg[0], XmNselectionPolicy, XmSINGLE_SELECT);
	XtSetValues(w, Dm__arg, 1);
	XtAddCallback(w, XmNsingleSelectionCallback, ComboListSelectCB,
			(char *)fpsptr);

    return(fpsptr);
}


/* UpdatePermissions() function gets called from ApplyCB().
   It updates the permission for the category indicated by 'type'.
   Note, that the mode change is not enough., additional info. about
   file type, sticky bit etc. needs to be restored as well from the original
   mode. The later is done by the caller.
*/
static mode_t
UpdatePermissions(DmFPropSheetPtr fpsptr, int type)
{

	mode_t 		mode = 0;
    	char *		owner_choice_value;
    	char *		group_choice_value;
    	char *		other_choice_value;
    	char *		dos_choice_value;

	Gizmo		owner_choice_gizmo;
	Gizmo		group_choice_gizmo;
	Gizmo		other_choice_gizmo;
	Gizmo		dos_choice_gizmo;

	switch(type) {
		case OWNER:
			owner_choice_gizmo = QueryGizmo(PopupGizmoClass,
								fpsptr->shell,
								GetGizmoGizmo,
								"ownerchoice");
    			ManipulateGizmo(ChoiceGizmoClass,
						(Gizmo)owner_choice_gizmo,
						GetGizmoValue);
    			owner_choice_value = (char *) QueryGizmo(
							ChoiceGizmoClass,
							owner_choice_gizmo,
							GetGizmoCurrentValue,
							NULL);
			if (owner_choice_value[0] == 'x')
				mode = S_IRUSR;
			if (owner_choice_value[1] == 'x')
				mode |= S_IWUSR;
			if (owner_choice_value[2] == 'x')
				mode |= S_IXUSR;
			break;
		case GROUP:
			group_choice_gizmo = QueryGizmo(PopupGizmoClass,
								fpsptr->shell,
								GetGizmoGizmo,
								"groupchoice");
    			ManipulateGizmo(ChoiceGizmoClass,
						(Gizmo)group_choice_gizmo,
						GetGizmoValue);
    			group_choice_value = (char *) QueryGizmo(
							ChoiceGizmoClass,
							group_choice_gizmo,
							GetGizmoCurrentValue,
							NULL);
			if (group_choice_value[0] == 'x')
				mode = S_IRGRP;
			if (group_choice_value[1] == 'x')
				mode |= S_IWGRP;
			if (group_choice_value[2] == 'x')
				mode |= S_IXGRP;
			break;

		case OTHER:
			other_choice_gizmo = QueryGizmo(PopupGizmoClass,
								fpsptr->shell,
								GetGizmoGizmo,
								"otherchoice");
    			ManipulateGizmo(ChoiceGizmoClass,
						(Gizmo)other_choice_gizmo,
						GetGizmoValue);
    			other_choice_value = (char *) QueryGizmo(
							ChoiceGizmoClass,
							other_choice_gizmo,
							GetGizmoCurrentValue,
							NULL);
			if (other_choice_value[0] == 'x')
				mode = S_IROTH;
			if (other_choice_value[1] == 'x')
				mode |= S_IWOTH;
			if (other_choice_value[2] == 'x')
				mode |= S_IXOTH;
			break;
		case ACCESS:
			dos_choice_gizmo = QueryGizmo(PopupGizmoClass,
								fpsptr->shell,
								GetGizmoGizmo,
								"doschoice");
    			ManipulateGizmo(ChoiceGizmoClass,
						(Gizmo)dos_choice_gizmo,
						GetGizmoValue);
    			dos_choice_value = (char *) QueryGizmo(
							ChoiceGizmoClass,
							dos_choice_gizmo,
							GetGizmoCurrentValue,
							NULL);
			if (dos_choice_value[1] == 'x')
				mode = (S_IRWXU | S_IRWXG | S_IRWXO);
			else
				mode = (S_IRUSR | S_IRGRP | S_IROTH) ;
			break;
		}
	return(mode);
}

static	void
ReclassCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFPropSheetPtr	fpsptr = (DmFPropSheetPtr)client_data;
    DmFolderWinPtr	window = (DmFolderWinPtr)fpsptr->window;
    char *fullsrc;
    Widget classWidget; /* class combo widget */
    XmString itemString;
    DmItemPtr ip;
    int newclass;
    DmFclassPtr save_fcp;
    char *save_value;
    DtAttrs save_attrs;

    ip = DmObjNameToItem((DmWinPtr)window, fpsptr->prev);
    if (!ip)
    	return;
	save_value = DtGetProperty(&(ITEM_OBJ(ip)->plist),
					OBJCLASS, &save_attrs);

    /*
     * Remove _CLASS property before retyping the object.
     * Shouldn't need to check for LOCKED attribute, because the update
     * button will be disabled in that case.
     */
    DtSetProperty(&(ITEM_OBJ(ip)->plist), OBJCLASS, NULL, 0);

    /* retype the object */
    save_fcp = ITEM_OBJ(ip)->fcp;
    DmSetFileClass(ITEM_OBJ(ip));

    /* update class label */
    newclass = FindClass(fpsptr->intClassList, fpsptr->list_count,
    			((DmFnameKeyPtr)(ITEM_OBJ(ip)->fcp->key))->name);
	classWidget = QueryGizmo(PopupGizmoClass, fpsptr->shell,
					GetGizmoWidget, "class_combo");
	XtSetArg(Dm__arg[0], XmNselectedPosition, newclass);
	XtSetValues(classWidget, Dm__arg, 1);

	ITEM_OBJ(ip)->fcp = save_fcp;
	fpsptr->update = True;

	/* restore _CLASS */
	DtSetProperty(&(ITEM_OBJ(ip)->plist), OBJCLASS, save_value, save_attrs);
}

static	void
ComboListSelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFPropSheetPtr	fpsptr = (DmFPropSheetPtr)client_data;
	XmListCallbackStruct *cbs = (XmListCallbackStruct *)call_data;
	Widget combo;

    XtSetArg(Dm__arg[0], XmNselectedPosition, cbs->item_position - 1);
    XtSetArg(Dm__arg[1], XmNtopItemPosition, cbs->item_position - 1);

	combo = QueryGizmo(PopupGizmoClass, fpsptr->shell, GetGizmoWidget,
				"class_combo");
    XtSetValues(combo, Dm__arg, 2);

    fpsptr->update = False; /* turn off update flag */
}

/* ApplyCB() function is a callback routine when the user clicks on
   an "Apply" button. It goes thru all the fields, validates new input
   if specified, and if valid, it initiates the change.
   Note the file name change is handles last, so if we had multiple filed
   change, we can take care of all other first, and then file renaming
   will save changing mode etc. later on.
   File name change may also involve additional work, such as, if the
   object was a directory and it/its descentdants are open, we may have
   to change the path.

   MORE: we need to retype the object when its permissions are changed.
   We need to update the views also.
*/
static	void
ApplyCB(Widget	w, XtPointer client_data, XtPointer call_data)
{
    DmFPropSheetPtr	fpsptr = (DmFPropSheetPtr)client_data;
    DmFolderWinPtr	window = (DmFolderWinPtr)fpsptr->window;
    char 		*fullsrc = NULL;
    char 		*fulldst = NULL;
    char		*new_str, *new_own, *new_grp;
    struct passwd *	pw;
    struct		group	*gr;
    mode_t		new_mode, old_mode;
    DmItemPtr		ip;
    int 		gchanged, ochanged;
    int 		ftype;
    int			pos; /* current value in class combo */
    Widget		tmpw;
    void		**item_list;

    /* Assume popdown first (Reset callback may have set this to True) */
	fpsptr->flag = False;

    /* check if file still exists */
    fullsrc = strdup(DmMakePath(window->views[0].cp->path, fpsptr->prev));
    if (access(fullsrc, F_OK) != 0) {
	DmVaDisplayStatus((DmWinPtr)window, True,
			  TXT_FILE_NOT_EXIST, fpsptr->prev);
	fpsptr->flag = True;			/* popup should stay up */
	goto out;
    }

    ip = DmObjNameToItem((DmWinPtr)window, fpsptr->prev);
    if (!ip || !ITEM_MANAGED(ip)) {
	DmVaDisplayStatus((DmWinPtr)window, True,
			  TXT_FILE_NOT_IN_VIEW, fpsptr->prev);
	fpsptr->flag = True;		/* popup should stay up */
	goto out;
    }

    ftype = FILEINFO_PTR(ip)->fstype;
    /* update permissions. we are not checking changes in individual
       perms. for the sake of optimization.
    */
    old_mode =  FILEINFO_PTR(ip)->mode;
    if (ftype != DOSFS_ID(Desktop)) {
	    new_mode = ((UpdatePermissions(fpsptr, OWNER)) |
			(UpdatePermissions(fpsptr, GROUP)) |
			(UpdatePermissions(fpsptr, OTHER)));
    } else
	    new_mode = UpdatePermissions(fpsptr, ACCESS);

    new_mode =  (old_mode & ~S_IAMB) | new_mode;

    if (new_mode != old_mode) {
    	if (chmod(fullsrc, new_mode) == 0)
    		FILEINFO_PTR(ip)->mode = new_mode;
	else {
		DmVaDisplayStatus((DmWinPtr)window, True, TXT_PROP_FAILED,
				  fullsrc);
		goto out;
	}
    }

    if (ftype != DOSFS_ID(Desktop)) {
    	/* now owner and group */
	new_own = GetInputGizmoText((Gizmo)QueryGizmo(PopupGizmoClass,
				     fpsptr->shell, GetGizmoGizmo,
				     "prop_owner"));
	new_grp = GetInputGizmoText((Gizmo)QueryGizmo(PopupGizmoClass,
				    fpsptr->shell, GetGizmoGizmo,
				    "prop_group"));

	if (ochanged = strcmp(fpsptr->prev_own, new_own)) {
		/* we have to validate new owner name, may not exist */
		if ( (pw = getpwnam(new_own)) != NULL ) {
		    XtFree(fpsptr->prev_own);
		    fpsptr->prev_own = strdup(new_own);
		    FILEINFO_PTR(ip)->uid = pw->pw_uid;
		} else {
		    char *retstr;
		    long uid = strtol(new_own, &retstr, 10);

		    if ((new_own != retstr) && (errno != ERANGE)) {
			FILEINFO_PTR(ip)->uid = uid;
		    } else {
			DmVaDisplayStatus((DmWinPtr)window, True,
					  TXT_INVALID_OWNER, new_own);
			fpsptr->flag = True;	/* popup should stay up */
			goto out;			/* return now */
		    }
		}
	    }

	if (gchanged = strcmp(fpsptr->prev_grp, new_grp)) {
		/* we have to validate new group, may not exist */
		if ( (gr = getgrnam(new_grp)) != NULL ) {
		    XtFree(fpsptr->prev_grp);
		    fpsptr->prev_grp = strdup(new_grp);
		    FILEINFO_PTR(ip)->gid = gr->gr_gid;
		} else {
		    char *retstr;
		    long gid = strtol(new_grp, &retstr, 10);

		    if ((new_grp != retstr) && (errno != ERANGE)) {
			FILEINFO_PTR(ip)->gid = gid;
		    } else {
			DmVaDisplayStatus((DmWinPtr)window, True,
					  TXT_INVALID_GROUP, new_grp);
			fpsptr->flag = True;	/* popup should stay up */
			goto out;		/* return now */
		    }
		}
	}

	if (ochanged || gchanged) {
		if (chown(fullsrc, FILEINFO_PTR(ip)->uid, FILEINFO_PTR(ip)->gid)
		    != 0) {
			struct stat statbuf;

			if (stat(fullsrc, &statbuf) == 0) {
				if (ochanged)
					FILEINFO_PTR(ip)->uid = statbuf.st_uid;
				if (gchanged)
					FILEINFO_PTR(ip)->gid = statbuf.st_gid;
			}
			DmVaDisplayStatus((DmWinPtr)window, True,
					TXT_PROP_FAILED, fullsrc);
			goto out;
		}
	}

    	/* no need to free new_own and new_grp */
    }
		
    /* comment field */
    new_str = GetInputGizmoText((Gizmo)QueryGizmo(PopupGizmoClass,fpsptr->shell,
			 	GetGizmoGizmo,"prop_comment"));
    if (strcmp(new_str, fpsptr->prev_cmt)) {
	XtFree(fpsptr->prev_cmt);
	fpsptr->prev_cmt = new_str;	/* no need to free */
	DmSetObjProperty(ITEM_OBJ(ip), COMMENT, strdup(fpsptr->prev_cmt), NULL);
    }

    /* update file name change 
     */
    new_str = GetInputGizmoText((Gizmo)QueryGizmo(PopupGizmoClass,fpsptr->shell,
			         GetGizmoGizmo, "prop_name"));
    if (strcmp(fpsptr->prev, new_str)) {
	DmObjectPtr op;

	/* we don't allow '/' char while renaming a file */
	if (strchr(new_str, '/')) {
	    DmVaDisplayStatus((DmWinPtr)window, True, TXT_NO_SLASH);
	    fpsptr->flag = True;	/* stay up */
	} else if (strlen(new_str) > Desktop->fstbl[ftype].f_maxnm) {
	    /* file name limit */
	    DmVaDisplayStatus((DmWinPtr)window, True, TXT_NAME_TOO_LONG,
			      Desktop->fstbl[ftype].f_maxnm);
	    fpsptr->flag = True;	/* stay up */
	} else {
		FREE(fpsptr->prev);
	    fpsptr->prev = new_str;

	    /* fulldst will be freed by file op */
	    fulldst = strdup(DmMakePath(window->views[0].cp->path, new_str));

	    item_list = DmOneItemList(fpsptr->item);
	    if (DmSetupFileOp(window, DM_RENAME, fulldst, item_list) != 0)
		/* error .. leave prop sheet posted */
		fpsptr->flag = True;
	    else
		if (!fpsptr->flag)
		/* make item sensitive if no other errors have occured */
		ExmVaFlatSetValues(window->views[0].box, 
				   fpsptr->item - DM_WIN_ITEM(window, 0),
				   XmNsensitive, True, NULL);
	    XtFree((void *) item_list);
	}
    }

    /* check class combo box */
	tmpw = QueryGizmo(PopupGizmoClass, fpsptr->shell, GetGizmoWidget,
				"class_combo");
	XtSetArg(Dm__arg[0], XmNselectedPosition, &pos);
	XtGetValues(tmpw, Dm__arg, 1);
	if (strcmp(fpsptr->intClassList[pos], fpsptr->prev_class)) {
		/* class changed */
		if (fpsptr->update)
		    /* remove _CLASS property before retyping the object */
    		DtSetProperty(&(ITEM_OBJ(ip)->plist), OBJCLASS, NULL, 0);
    	else {
    		/* class specified by the user */
    		DtSetProperty(&(ITEM_OBJ(ip)->plist), OBJCLASS,
    			fpsptr->intClassList[pos], DT_PROP_ATTR_DONTCHANGE);
    	} /* else */

	    /* retype the object */
	    DmRetypeObj(ITEM_OBJ(ip), True);
	}

out:
    XtFree(fullsrc);
    if (!fpsptr->flag)			/* DON'T stay up?? */
	PopdownPropSheet(w, client_data, call_data);

}				/* end of ApplyCB */

/****************************procedure*header*****************************
    PopdownCB- get called when sheet pops down: from pulling pin,
    Apply, and Cancel.  Free space, destroy
    sheet, and unbusy item.
*/
static void
PopdownCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFPropSheetPtr	sheet = (DmFPropSheetPtr)client_data;
    DmFolderWinPtr	window;
    DmItemPtr		ip;
    Widget 		shell = GetPopupGizmoShell(sheet->shell);
    Gizmo 		prev_gizmo;
    char 		*debug;

    if (sheet->shell == NULL)
	return;

    window = (DmFolderWinPtr)sheet->window;
    ip = DmObjNameToItem((DmWinPtr)window, sheet->prev);


    /* unregister callback for folder updates */
    DtRemoveCallback(&(window->views[0].cp->cb_list), UpdatePropSheetCB , 
		     (XtPointer) sheet);
    XtDestroyWidget(w);
    FreeGizmo(PopupGizmoClass, sheet->shell);


    sheet->shell = NULL;
    sheet->prop_num = -1;
	FREE(sheet->prev);

    /* free class list */
    FreeClassList(sheet->intClassList, sheet->list_count);

    /* unbusy associated icon in the window */
    if (ip && ITEM_MANAGED(ip))
    {
        Cardinal item_index = ip - window->views[0].itp;

    	ExmVaFlatSetValues(window->views[0].box,
			   item_index, XmNsensitive, True, NULL);
    }

}

/* ResetCB() function is a callback routine to reset the value in
   various fields once the user has made a change and clickedon the "Reset"
   button.
*/
static void
ResetCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmFPropSheetPtr	fpsptr = (DmFPropSheetPtr)client_data;
	DmFolderWinPtr	window = (DmFolderWinPtr)fpsptr->window;

	ManipulateGizmo(PopupGizmoClass, fpsptr->shell, ResetGizmoValue);

	fpsptr->flag = True;			/* popup should stay up */
}

/* VerifyCB() function is verification callback invoked by popup
   window widget to confirm whether it is ok to popdown the window.
*/
static void
VerifyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFPropSheetPtr	sheet = (DmFPropSheetPtr)client_data;

    if (sheet->flag)				/* should popup stay up? */
    {
	Boolean * ok = (Boolean *)call_data;

	*ok = sheet->flag = False;
    }
}

/****************************procedure*header*****************************
    HelpCB- displays help on file property sheet.
*/
static void
HelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DmFPropSheetPtr	fpsptr = (DmFPropSheetPtr)client_data;

	DmDisplayHelpSection(DmGetHelpApp(FOLDER_HELP_ID(Desktop)), NULL,
		FOLDER_HELPFILE, FOLDER_FILE_PROP_SECT);
	fpsptr->flag = True;			/* popup should stay up */
} /* end of HelpCB */

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    Dm__PopupFilePropSheets() function gets called from the "Properties.."
	callback routine. It goes thru the selected item, sets 'em busy and
	calls routine to create file property sheet. If the property sheet
	for a window was already up, it simply raises it and returns.
*/
void
Dm__PopupFilePropSheets(DmFolderWinPtr window)
{
    DmItemPtr	item;
    Cardinal	i;

    /* go thru' each selected item and see if prop. needs to be createed */
    for(i = 0, item = window->views[0].itp; i < window->views[0].nitems; i++, item++)
	if ((ITEM_MANAGED(item)) && (ITEM_SELECT(item)))
	    Dm__PopupFilePropSheet(window, item);

} /* end of Dm__PopupFilePropSheets */

/****************************procedure*header*****************************
    Dm__PopupFilePropSheet- create (if necessary) and popup a property sheet
	for the item pointed to by 'item'.
*/
void
Dm__PopupFilePropSheet(DmFolderWinPtr window, DmItemPtr item)
{
    Cardinal		indx = item - window->views[0].itp;
    DmFPropSheetPtr	sheet;
    Gizmo		popup;
    Widget		shell;

    /* If sheet is already up, raise it.  Otherwise, create it and popup it
       up.
    */
    for (sheet = window->fpsptr; sheet != NULL; sheet = sheet->next)
	if((sheet->prop_num != -1) &&
	   (!strcmp( sheet->prev, (ITEM_OBJ(item))->name)))
	{
	    shell = GetPopupGizmoShell(sheet->shell);
	    XRaiseWindow(XtDisplay(shell), XtWindow(shell));
	    return;
	}

    /* Sheet not found.  Create one and popup it up */

    /* Make item busy (and refresh it!) */
    ExmVaFlatSetValues(window->views[0].box, indx, XmNsensitive, False, NULL);

	/* update stat info on the item. */
	if (ITEM_OBJ(item)->objectdata)
		FREE(ITEM_OBJ(item)->objectdata);
	DmInitObj(ITEM_OBJ(item), NULL, DM_B_INIT_FILEINFO);

    sheet = CreateSheet(window, item);

    DtAddCallback(&window->views[0].cp->cb_list, UpdatePropSheetCB, sheet);

    MapGizmo(PopupGizmoClass, sheet->shell);


    /* XRaiseWindow(XtDisplay(fps), XtWindow(fps)); */

}				/* end of Dm__PopupFilePropSheet */


/****************************procedure*header*********************************
 *  PopdownPropSheet: Dismiss property sheet in response to Cancel Button
 *	or WM_DELETE_WINDOW request.  Prop sheets may be popped down in
 *	other ways, so data is freed in PopdownCB.
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
static void
PopdownPropSheet(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFPropSheetPtr	sheet = (DmFPropSheetPtr)client_data;
    Widget 		shell = GetPopupGizmoShell(sheet->shell);

    if (sheet->shell == NULL)
	return;

    BringDownPopup(shell);
} /* end of PopdownPropSheet */

/****************************procedure*header*********************************
 *  PopdownAllPropSheets: Dismiss property all property sheets for a folder
 *	Prop sheet data is freed in PopdownCB.
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
void
PopdownAllPropSheets(DmFolderWindow window)
{
    DmFPropSheetPtr	sheet;
    Widget 		shell;

    for (sheet = window->fpsptr; sheet != NULL; sheet = sheet->next)
	if(sheet->prop_num != -1)
	{
	    shell = GetPopupGizmoShell(sheet->shell);
	    PopdownPropSheet(shell, sheet, NULL);
	}
} /* end of PopdownAllPropSheets */
/****************************procedure*header*********************************
 *  DmBusyPropSheets: Busy all posted property sheets to prevent another 
 *		file-op (rename) from being kicked off 
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
void
DmBusyPropSheets(DmFolderWindow folder, Boolean busy)
{
    DmFPropSheetPtr	sheet;
    Widget 		shell;
    
    for (sheet = folder->fpsptr; sheet != NULL; sheet = sheet->next)
	if(sheet->prop_num != -1)
	{
	    shell = GetPopupGizmoShell(sheet->shell);
	    DmBusyWindow(shell, busy);
	}
} 	/* end of DmBusyPropSheets */

/*****************************************************************************
 *  	UpdatePropSheetCB: update prop-sheet in response to container/object 
 *			mods 
 *	INPUTS: widget is NULL (always)
 *		client_data is 
 *		call_data is DmContainerCallDataPtr (see Dt/DesktopP.h)
 *	OUTPUTS: none
 *	GLOBALS:
 *****************************************************************************/
static void
UpdatePropSheetCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmFPropSheetPtr	     sheet = (DmFPropSheetPtr) client_data;
    DmContainerCallDataPtr cp_data = (DmContainerCallDataPtr) call_data;
    DmContainerCBReason	    reason = cp_data->reason;
    DmFolderWinPtr	    folder = sheet->window;


    switch(reason){
#ifdef NOT_USED
	This code causes a coredump.  The problem seems to be caused
	by the XtDestroyWidget call in PopdownCB.

    case DM_CONTAINER_CONTENTS_CHANGED:
    {
	DmObjectPtr *obj;
	DmContainerChangedRec *changed = &cp_data->detail.changed;
	int i;
	if (changed->num_removed > 0 ){
	    for (obj = changed->objs_removed, i=0; i<changed->num_removed; 
		 i++, obj++){
		if (ITEM_OBJ(sheet->item) == (*obj)){
		    PopdownPropSheet(sheet->window->shell, sheet, NULL);
		}

	    }
	}
    }
	break;
#endif
    case DM_CONTAINER_OBJ_CHANGED:
    {
	DmContainerObjChangedRec *obj_changed = &cp_data->detail.obj_changed;
	DtAttrs obj_info = obj_changed->attrs;
	DmObjectPtr obj = obj_changed->obj;
	Gizmo name_giz;


	if (ITEM_OBJ(sheet->item) == (obj)){
	    if (obj_info & DM_B_OBJ_NAME){

		FREE(sheet->prev);
		sheet->prev = strdup(obj_changed->new_name);
		name_giz = (Gizmo)QueryGizmo(PopupGizmoClass,
						   sheet->shell, GetGizmoGizmo,
						   "prop_name");
		SetInputGizmoText(name_giz, obj_changed->new_name);
		/* FLH MORE: we really need a InitGizmoValue */
		ManipulateGizmo(InputGizmoClass, name_giz, GetGizmoValue);
		ManipulateGizmo(InputGizmoClass, name_giz, ApplyGizmoValue);

	    }
#ifdef NOT_USED
	    if (obj_info & DM_B_OBJ_TYPE){
		update the type field
	    }
#endif
	}
    }	/* DM_CONTAINER_OBJ_CHANGED */
	break;
    default:
	break;
    }
} /* end of UpdatePropSheetCB */
