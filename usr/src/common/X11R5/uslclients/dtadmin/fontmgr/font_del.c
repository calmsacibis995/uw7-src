#ident	"@(#)dtadmin:fontmgr/font_del.c	1.18.1.15"
/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_del.c
 */

#include <stdio.h>
#include <signal.h>
#include <Intrinsic.h>
#include <StringDefs.h>
#include <Shell.h>

#include <OpenLook.h>

#include <Gizmos.h>
#include <BaseWGizmo.h>
#include <PopupGizmo.h>
#include <MenuGizmo.h>
#include <StaticText.h>
#include <ModalGizmo.h>

#include <ScrolledWi.h>
#include <FList.h>
#include <fontmgr.h>


/*
 * external data
 */
extern Widget       app_shellW;		  /* application shell widget       */
extern ModalGizmo prompt;
extern char *xwin_home;
_OlArrayType(FamilyArray) family_data;

extern String LowercaseStr(String);
extern void ErrorCancelCB();
extern void HelpCB();
static void IntegrityDoneCB();
static void IntegrityCheckCB();
static void IntegrityCancelCB();
void IntegrityCheck();
static void DeleteCancelCB();
static void ConfirmCancelCB();
static void DoDeleteCB();
static void ApplyDeleteCB();
static void ResetDeleteCB();
static string_array_type _font_name, _xlfd, _selected_xlfd, _file_name;
static string_array_type _selected_dir;
static delete_type delete_info = { &_font_name, &_xlfd, &_selected_xlfd,
				   &_file_name, &_selected_dir};

static Widget ok_popup=0;
static Widget i_popup=0;

static HelpInfo help_integrity = { 0, "", HELP_PATH , TXT_HELP_INTEGRITY_SECTION};
static HelpInfo help_none_to_delete = { 0, "", HELP_PATH , TXT_HELP_NO_FONTS_TO_DELETE};
static HelpInfo help_delete = { 0, "", HELP_PATH , TXT_HELP_DEL_SECTION};
static HelpInfo help_confirm_delete = { 0, "", HELP_PATH };

static MenuItems delete_menu_item[] = {  
{ TRUE,TXT_DELETE_APPLY, ACCEL_DELETE_APPLY ,0, ApplyDeleteCB, (char*)&delete_info },
{ TRUE,TXT_RESET, ACCEL_DELETE_RESET ,0, ResetDeleteCB, (char*)&delete_info},
{ TRUE,TXT_CANCEL,ACCEL_DELETE_CANCEL,0, DeleteCancelCB, (char*)&delete_info },
{ TRUE,TXT_HELP_DDD,  ACCEL_DELETE_HELP  ,0, HelpCB, (char *)&help_delete },
{ NULL }
};
static MenuGizmo delete_menu = {0, "dm", "dm", delete_menu_item,
			 0, 0};
static PopupGizmo delete_popup = {0, "dp", TXT_FONT_DELETE, (Gizmo)&delete_menu,
	0, 0, 0, 0, 0, 0, 0 };

static MenuItems integrity_menu_item[] = {  
{ TRUE,TXT_CONTINUE, ACCEL_TXT_CONTINUE ,0, IntegrityCheckCB, NULL },
{ TRUE,TXT_CANCEL,ACCEL_DELETE_CANCEL,0, IntegrityCancelCB, NULL },
{ TRUE,TXT_HELP_DDD,  ACCEL_DELETE_HELP  ,0, HelpCB, (char *)&help_integrity },
{ NULL }
};


static MenuItems integrity_done_item[]= {
{ TRUE,TXT_OK, ACCEL_INTEGRITY_OK ,0, IntegrityDoneCB, NULL },
{ NULL }
};
static MenuGizmo integrity_done_menu = {0, "id", "id", integrity_done_item };
static MenuGizmo integrity_menu = {0, "ic", "ic", integrity_menu_item };
static PopupGizmo integrity_popup = {0, "dp", TXT_INTEGRITY,
	 (Gizmo)&integrity_menu};

static PopupGizmo integrity_done_popup = {0, "dp", TXT_INTEGRITY,
	 (Gizmo)&integrity_done_menu};

static char *system_font[]= { 
    "-lucida-",
    "-lucidatypewriter-medium-r-",
    "-helvetica-",
    "-courier-medium-r-",
    "-open look cursor-",
    "-open look glyph-",
    "-clean-",
    "-fixed-"
};


/*
 * return the length of the string that includes 'num' of 'char'
 */
static int
strnchar(String str,char ch, int num)
{
    String p = str;

    for ( ; *p; p++) {
	if (*p == ch)
	    num--;
	if (num <= 0)
	    break;
    }
    return p - str;
}


/*
 * returns TRUE if a match is found
 * returns FALSE otherwise
 */
static Boolean
MatchFileWithFont(info, font_path, file_name, font_name)
     delete_type *info;
     char *font_path;
     char *file_name;
     char *font_name;
{
    int i, j, str_len;
    char full_path_name[MAX_PATH_STRING];
    char body_name[MAX_STRING];

    for (i=0; i<info->selected_xlfd->n_strs; i++) {
	if (info->bitmap)
	    str_len = strlen(font_name);
	else
	    str_len = strnchar(font_name, DELIM, 9);

	if (strncmp( LowercaseStr(font_name), info->selected_xlfd->strs[i],
		    str_len) == STR_MATCH) {
	    sprintf(full_path_name, "%s/%s", font_path, file_name);
	    InsertStringDB(info->file_name, full_path_name);

	    /* if outline font then delete afm file and pfb file */
	    if (!info->bitmap) {
		sscanf( file_name, "%[^.]", body_name);
		sprintf(full_path_name, "%s/%s/%s.afm", font_path,
			AFM_DIR, body_name);
		InsertStringDB(info->file_name, full_path_name);
		sprintf(full_path_name, "%s/%s.pfb", font_path, body_name);
		InsertStringDB(info->file_name, full_path_name);
	    }

	    /* keep a record of modified directories */
	    for( j=0; j<info->selected_dir->n_strs; j++) {
		if (strcmp(info->selected_dir->strs[j],font_path) == STR_MATCH)
		    break;
	    }
	    if (j >= info->selected_dir->n_strs)
		InsertStringDB(info->selected_dir, font_path);
	    
	    return TRUE;
	}
    }
    return FALSE;

} /* end of MatchFileWithFont */


static void
DeleteFontFiles(delete_type *info)
{
    int i;

    /* remove the font files from the disk */
    for(i=0; i<info->file_name->n_strs; i++) {
	unlink( info->file_name->strs[i]);
    }
} /* DeleteFontFiles */


/*
 * insert the family names that the user selected into an array
 */
static void
GetSelectedFonts(info)
    delete_type *info;
{
    Boolean selected;
    int i;

    for(i= info->font_name->n_strs - 1; i>=0; i--) {
	OlVaFlatGetValues(info->font_list, i, XtNset, &selected, 0);
	if (selected) {
	    InsertStringDB(info->selected_xlfd, info->xlfd->strs[i]);
	}
    }
} /* GetSelectedFonts */


/*
 * update the fonts.dir, the server, and us
 */
UpdateFonts( font_path, n_paths, bitmap)
    char **font_path;
    int n_paths;
    Boolean bitmap;
{
    char *ptr;
    char sys_cmd[MAX_PATH_STRING*4];
    int i, port;
    String derived_ps;
    char **fs_font_path;
    int fs_n_paths;

    /* update fonts.scale */
		derived_ps = (String) GetDerivedPS();
		sprintf(sys_cmd, "DERIVED_INSTANCE_PS='%s' %s/bin/mkfontscale",
		derived_ps ? derived_ps : "", xwin_home	);
	for (i=0; i<n_paths; i++) {
	    /* only mkfontscale TYPE1 fonts */
	    if (strstr(font_path[i], "ype1")) {
		strcat(sys_cmd, " ");
		strcat(sys_cmd, font_path[i]);
	    }
	}
		strcat(sys_cmd, " ");
		strcat(sys_cmd,xwin_home);
		strcat(sys_cmd,"/lib/fonts/type1");

#ifdef DEBUG
	fprintf(stderr,"MKFONTSCALE: %s\n", sys_cmd);
#endif
	system(sys_cmd);
	sprintf(sys_cmd, "/usr/bin/cp %s/lib/fonts/type1/fonts.scale %s/lib/fonts/type1/fonts.dir", xwin_home, xwin_home);

	system(sys_cmd);

	/* Becase of a bug in mkfontdir we have to do this.
	   The bug is, if fonts.scale has no xlfd entry,
	   mkfontdir will not update fonts.dir */
	for (i=0; i<n_paths; i++) {
	    sprintf(sys_cmd,
		    "/usr/bin/cp %s/fonts.scale %s/fonts.dir 2>/dev/null",
		    font_path[i], font_path[i]);
	    system(sys_cmd);
	}

    /* perform mkfontdir */
    sprintf(sys_cmd, "%s/bin/mkfontdir", xwin_home);
    for (i=0; i<n_paths; i++) {
		/* skip font server paths */
#ifdef DEBUG
	fprintf(stderr,"mkfontdir check font_path[i]=%s\n",font_path[i]);
#endif
	if (ptr=strstr(font_path[i], ":")) continue;
	strcat(sys_cmd, " ");
	strcat(sys_cmd, font_path[i]);
    }
#ifdef DEBUG
	fprintf(stderr,"MKFONTDIR: %s\n", sys_cmd);
#endif
    system(sys_cmd);

    sprintf(sys_cmd, "%s/bin/xset fp rehash", xwin_home);
    system(sys_cmd);

	/* send SIGUSR1 to each fontserver in the fontpath */
	/* this will cause the fontserver to reread it's configuration
		data, we need this to get the newly installed/deleted
		fonts recognized */

	fs_font_path = XGetFontPath(XtDisplay(app_shellW), &fs_n_paths);

	for (i=0; i<fs_n_paths; i++) {
		/* see if the fontpath element contains a fontserver
			address */
	    if (ptr=strstr(fs_font_path[i], ":")) {
		ptr++;
		if (ptr) {
			/* execute fs_sig1 with port number of fs */
			port = atoi(ptr);
			sprintf(sys_cmd, "/usr/X/adm/fsfpreset %d", port);
			system(sys_cmd);
			}
		}
	
	}

	XFreeFontPath(fs_font_path);
    UpdateMainView();

} /* end of UpdateFonts */

	
static void
DoDelete(delete_info)
    delete_type *delete_info;
{
    int i, read_count, fonts_in_dir;
    char **font_path;
    char scalable_dir[MAX_PATH_STRING];
    int n_paths;
    char dir_filename[MAX_PATH_STRING], file_name[MAX_STRING];
    char font_name[MAX_PATH_STRING];
    FILE *dir_file;


	/* get directory for scalable fonts */ 
	/* this directory is not in the font path directly since
		scalable fonts are now handled by the font server
		and we don't know the font server's font catalogue.
		However, we know we install fonts in the type1 directory
		so we can delete them from that directory */

    callRegisterHelp((Widget) delete_info->popup,
			ClientName, TXT_HELP_DEL_SECTION);
    sprintf(scalable_dir, "%s/lib/fonts/type1", xwin_home);
    BusyCursor(0);
    font_path = XGetFontPath(XtDisplay(delete_info->popup), &n_paths);
	/* need to add /usr/X/lib/fonts/type1 and /usr/X/lib/fonts/Type1
		to these lists to allow deletion of scalable fonts */
    for (i=0; i<n_paths; i++) readFontsDirFile(font_path[i],delete_info);
    readFontsDirFile(scalable_dir, delete_info);
    sprintf(scalable_dir, "%s/lib/fonts/Type1", xwin_home);
    readFontsDirFile(scalable_dir, delete_info);
    DeleteFontFiles(delete_info);
    UpdateFonts( delete_info->selected_dir->strs,
		delete_info->selected_dir->n_strs,
		delete_info->bitmap);
    XFreeFontPath(font_path);
    StandardCursor(0);
}

readFontsDirFile(pathname, delete_info)
char *pathname;
delete_type *delete_info;
{
    int i, read_count, fonts_in_dir;
    char **font_path;
    int n_paths;
    char dir_filename[MAX_PATH_STRING], file_name[MAX_STRING];
    char font_name[MAX_PATH_STRING];
    FILE *dir_file;


	strcpy(dir_filename, pathname);
	strcat(dir_filename, "/fonts.dir");
	dir_file = fopen(dir_filename, "r");
	if (!FileOK(dir_file))
	    return;
	read_count = fscanf(dir_file, "%d\n", &fonts_in_dir);
	if ((read_count == EOF) || (read_count != 1)) {
	    fclose(dir_file);
	    return;
	}
	while ((read_count = fscanf(dir_file, "%s %[^\n]\n", file_name,
				    font_name)) != EOF) {
	    if (read_count != 2) {
		break;
	    }
	    MatchFileWithFont(delete_info,
				  pathname, file_name, font_name);
	}
	fclose(dir_file);


} /* end of DoDelete */


static void
DoDeleteCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    delete_type *info = (delete_type *) client_data;

    BringDownPopup((Widget) _OlGetShellOfWidget(info->popup));
    BringDownPopup( prompt.shell);
  
    InformUser(TXT_DELETE_START);
    ScheduleWork( DoDelete, info, 9);

} /* end of DoDeleteCB */



static void CreateDeletePopup(w, info, bitmap, title)
    Widget w;
    delete_type *info;
    Boolean bitmap;
    String title;
{
    static String item_fields[] = {XtNlabel};
    Widget scroll_win, upper;
    int n;
    Arg largs[20];
    
    
    if (info->popup == NULL) {
	/* create the popup */
	info->popup = CreateGizmo(w, PopupGizmoClass,
			      &delete_popup, NULL, 0);

	XtVaGetValues( info->popup, XtNupperControlArea, &upper, 0);

	/*  Create the controls in the upper control area.  */
	n = 0;
	scroll_win = XtCreateManagedWidget("scroll_win",
		 scrolledWindowWidgetClass, upper, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNviewHeight, NUM_FAMILY_ITEMS);    n++; 
	XtSetArg(largs[n], XtNnumItems, info->font_name->n_strs);	n++;
	XtSetArg(largs[n], XtNitems, info->font_name->strs);		n++;
	XtSetArg(largs[n], XtNitemFields, item_fields);		 n++;
	XtSetArg(largs[n], XtNnumItemFields, XtNumber(item_fields)); n++;
	XtSetArg(largs[n], XtNexclusives, FALSE);		n++;
	info->font_list = XtCreateManagedWidget("families",
        flatListWidgetClass, scroll_win, largs, n);	
    }
    else {
	/* reset any previous selections */
	XtVaSetValues( info->font_list, XtNnumItems, 0, 0);

	n = 0;
	/* due to a bug in the flat list widget that causes the
	   scroll window to get larger, the XtNviewHeight has
	   to be specified */
	XtSetArg(largs[n], XtNviewHeight, NUM_FAMILY_ITEMS);    n++; 
	XtSetArg(largs[n], XtNnumItems, info->font_name->n_strs);	n++;
	XtSetArg(largs[n], XtNitems, info->font_name->strs);		n++;
	XtSetValues( info->font_list, largs, n);
    }

    XtVaSetValues(delete_popup.shell, XtNtitle, GetGizmoText(title), 0);
    callRegisterHelp(info->popup, ClientName, TXT_HELP_DEL_SECTION);
    MapGizmo(PopupGizmoClass, &delete_popup);

} /* end of CreateDeletePopup */


static Boolean
SystemFont( String xlfd)
{
    int i;

    for (i=0; i<XtNumber(system_font); i++) {
	if (strstr(xlfd, system_font[i]) != NULL)
	    return TRUE;
    }
    return FALSE;

} /* end of SystemFont */


static void 
DisplayDeletePopup( w, delete_info, title)
    Widget w;
    delete_type *delete_info;
    String title;
{
    Boolean bitmap = delete_info->bitmap;
    int i,j,k;
    _OlArrayType(FamilyArray) *family_array = &family_data;
    _OlArrayType(StyleArray) * style_array;
    _OlArrayType(PSArray) * ps_array;
    font_type *font_info;
    char font_name[MAX_PATH_STRING];
    /* get and display font list */
    DeleteStringsDB( delete_info->font_name);
    DeleteStringsDB( delete_info->xlfd);
    DeleteStringsDB( delete_info->selected_xlfd);
    DeleteStringsDB( delete_info->file_name);
    DeleteStringsDB( delete_info->selected_dir);
    for(i=0; i<_OlArraySize(family_array); i++) {
	style_array = _OlArrayElement(family_array, i).l;
	for(j=0; j<_OlArraySize(style_array); j++) {
	    ps_array = _OlArrayElement(style_array, j).l;
	    for(k=0; k<_OlArraySize(ps_array); k++) {
		font_info = _OlArrayElement(ps_array, k).l;
		if (font_info->bitmap == bitmap) {
		    if (bitmap) {
			sprintf( font_name, "%s  %s  %s",
				_OlArrayElement(family_array, i).name,
				_OlArrayElement(style_array, j).style_name,
				_OlArrayElement(ps_array, k).ps);
		    } else {
			sprintf( font_name, "%s  %s",
				_OlArrayElement(family_array, i).name,
				_OlArrayElement(style_array, j).style_name);
		}
		    if (!bitmap || !SystemFont(font_info->xlfd_name)) {
			InsertStringDB( delete_info->font_name, font_name);
			InsertStringDB( delete_info->xlfd, font_info->xlfd_name);
		    }
		}
	    }
	}
    }

    if (delete_info->font_name->n_strs)
	CreateDeletePopup(w, delete_info, bitmap, title);
    else {
	/* no fonts to delete */
	if (delete_info->popup)
	    BringDownPopup((Widget) _OlGetShellOfWidget(delete_info->popup));
	if (bitmap) {
		/* show different help if no fonts to be deleted */
		PopupErrorMsg(TXT_NO_DELETABLE_BITMAP,
			ErrorCancelCB,
			NULL,
			TXT_DELETE,
			&help_none_to_delete,
			TXT_HELP_NO_FONTS_TO_DELETE);
	} else {
		/* show different help if no fonts to be deleted */
		PopupErrorMsg(TXT_NO_DELETABLE_OUTLINE,
			ErrorCancelCB,
			NULL,
			TXT_DELETE,
			&help_none_to_delete,
			TXT_HELP_NO_FONTS_TO_DELETE);
		}
    }
} /* end of DisplayDeletePopup */


void DisplayBitmapDeleteCB( w, client, call )
Widget	     w;
XtPointer    client;
XtPointer    call;
{
    delete_info.bitmap = True;
    help_delete.section = TXT_HELP_DEL_BITMAP;
    help_confirm_delete.section = TXT_HELP_DEL_BITMAP_WARN;
    DisplayDeletePopup( w, &delete_info, TXT_DEL_BITMAP);
}


void DisplayOutlineDeleteCB( w, client, call )
Widget	     w;
XtPointer    client;
XtPointer    call;
{
    delete_info.bitmap = False;
    help_delete.section = TXT_HELP_DEL_OUTLINE;
    help_confirm_delete.section = TXT_HELP_DEL_OUTLINE_WARN;
    DisplayDeletePopup( w, &delete_info, TXT_DEL_OUTLINE);

} /* end of DisplayOutlineDeleteCB */


static void
ResetDeleteCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    delete_type *delete_info = (delete_type *) client_data;

    int i;

    for(i=0; i< delete_info->font_name->n_strs; i++)
	/* unselect all items */
	OlVaFlatSetValues(delete_info->font_list, i,
			  XtNset, FALSE,
			  (String) 0);
    
} /* end of ResetDeleteCB */


static void
ConfirmCancelCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    delete_type *delete_info = (delete_type *) client_data;

    BringDownPopup((Widget) _OlGetShellOfWidget(delete_info->popup));
    callRegisterHelp(app_shellW, ClientName, TXT_HELP_MAIN_MENU);
    BringDownPopup( prompt.shell);

} /* end of ConfirmCancelCB */


static void
IntegrityDoneCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    BringDownPopup( ok_popup);
}

static void
IntegrityCancelCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    BringDownPopup( i_popup);
}

static void
DeleteCancelCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    delete_type *delete_info = (delete_type *) client_data;

    BringDownPopup((Widget) _OlGetShellOfWidget(delete_info->popup));

} /* end of DeleteCancelCB */


static void
ApplyDeleteCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    delete_type *info = (delete_type *) client_data;

    GetSelectedFonts(info);
    if (info->selected_xlfd->n_strs)
	PromptUser(TXT_CONFIRM_MESS, DoDeleteCB, info,
               ConfirmCancelCB, info, &help_confirm_delete,
				TXT_DELETE,
				TXT_HELP_DEL_BITMAP);
    else {
	PopupErrorMsg(TXT_NONE_DELETE,
		ErrorCancelCB,
		NULL,
		TXT_DELETE,
		&help_delete,
		TXT_HELP_DEL_SECTION);
	
    }
} /* end of ApplyDeleteCB */


void 
IntegrityCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    char **font_path;
    int n_paths;
    static Widget msg=0;
    Widget upper;
    

    if (i_popup == NULL) {
	/* create the popup */
	i_popup = CreateGizmo(w, PopupGizmoClass,
			      &integrity_popup, NULL, 0);
	XtVaGetValues(i_popup, XtNupperControlArea, &upper, 0);
	msg = XtVaCreateManagedWidget("stext", 
		staticTextWidgetClass,
		upper,
		XtNtitle, GetGizmoText(TXT_INTEGRITY), 
		XtNstring, GetGizmoText(TXT_INTEGRITY_START),
		0);

	}
    XtVaSetValues(msg, 
		XtNstring, GetGizmoText(TXT_INTEGRITY_START),
		0);
    OlVaFlatSetValues(integrity_menu.child, 0,
		XtNselectProc, IntegrityCheckCB,
		XtNlabel, GetGizmoText(TXT_CONTINUE), 0);
	OlMoveFocus(integrity_menu.child, OL_IMMEDIATE, 0);
    callRegisterHelp(i_popup, ClientName, TXT_HELP_INTEGRITY_SECTION);
    MapGizmo(PopupGizmoClass, &integrity_popup);
    
}	

void 
IntegrityCheckCB(w)
Widget w;
{
	BringDownPopup(i_popup);
	ScheduleWork(IntegrityCheck, w , 5);

}

void
IntegrityCheck(w)
Widget w;
{
    char **font_path;
    static Widget msg2;
    Widget upper;
    int n_paths;


    if (ok_popup == NULL) {
	/* create the popup */
	ok_popup = CreateGizmo(w, PopupGizmoClass,
			      &integrity_done_popup, NULL, 0);
	XtVaGetValues(ok_popup, XtNupperControlArea, &upper, 0);
	msg2 = XtVaCreateManagedWidget("stext", 
		staticTextWidgetClass,
		upper,
		XtNstring, GetGizmoText(TXT_INTEGRITY_END),
		0);

	}
    XtVaSetValues(msg2, 
		XtNstring, GetGizmoText(TXT_INTEGRITY_END),
		0);

    callRegisterHelp(ok_popup, ClientName, TXT_HELP_INTEGRITY_SECTION);
    BusyCursor(0);
    font_path = XGetFontPath(XtDisplay(app_shellW), &n_paths);
    UpdateFonts( font_path, n_paths, FALSE);
    XFreeFontPath(font_path);
    MapGizmo(PopupGizmoClass, &integrity_done_popup);
    StandardCursor(0);

} /* end of IntegrityCB */


/*
 * search for X fontserver pid in /dev/X/fs.port#.pid file 
 */
static int
GetFontserverPid(char *filename)
{
    int pid=0;
    FILE *file;
    char buf[MAX_PATH_STRING];

    file = fopen(filename, "r");
    if (FileOK(file)) {
	while (fgets(buf, MAX_PATH_STRING, file) != NULL) {
		pid = atoi(buf);

	    }
	}
	fclose(file);
    return pid;
}


