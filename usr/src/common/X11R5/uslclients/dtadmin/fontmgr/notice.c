#ident	"@(#)dtadmin:fontmgr/notice.c	1.6"
/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_info.c
 */

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/FList.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Gizmos.h>
#include <MenuGizmo.h>
#include <ModalGizmo.h>
#include <PopupGizmo.h>
#include <RubberTile.h>
/*#include <InputGizmo.h>*/

#include <fontmgr.h>
/*
 * external data
 */

#define BUF 	400
#define INC	50
extern Widget app_shellW;
extern Widget base_shell;
static void NoticeCancelCB();
extern void HelpCB();

static string_array_type list_files;
static string_array_type *list= &list_files;
static char *ptr;
static Widget flat;

static HelpInfo help_info = {0, 0, HELP_PATH, TXT_HELP_MISSING_AFM_FILES_SECTION} ;

static MenuItems notice_menu_item[] = {
{ TRUE,TXT_OK,ACCEL_NOTICE_OK,0, NoticeCancelCB, 0 },
{ TRUE,TXT_HELP_DDD,  ACCEL_DELETE_HELP  ,0, HelpCB, (char *)&help_info },
{ NULL }
};

static MenuGizmo notice_menu = {0, "im", "im", notice_menu_item,
	0,0 };
 
static PopupGizmo notice = {0, "notice", FormalClientName,
	(Gizmo) &notice_menu, 
	 0,0, 0, 0, 0, 0, 0, 0, 0 };

void CreateNotice(info, missing_afm)
add_type *info;
int missing_afm;
{

	static Widget notice_popup=0;
        static Widget  control, stext1;
	char *filename;
	int len_alloc, len_used, this_len=0;
	char textmsg2[300];
    	char textmsg[300];
    	int i,found;


    /* if notice_popup doesn't exist, then create it */
	ptr = (char *) malloc(BUF);
	if (ptr == NULL) return;
	len_alloc = BUF;
	if (notice_popup == NULL) {
	sprintf(textmsg2, "%s", GetGizmoText(MISSING_AFM));
        notice_popup = CreateGizmo(app_shellW, PopupGizmoClass,
                              &notice, NULL, 0);

	XtVaGetValues(notice_popup, XtNupperControlArea, &control, 0);
	stext1 = XtVaCreateManagedWidget("stext1",
		staticTextWidgetClass, control,
		XtNalignment, OL_LEFT,
		XtNstring, textmsg2, 0);
	}	
	for (i=0,found=0; i < info->font_cnt; i++) {

	if (info->db[i].missing_afm == 1) {
		filename = strrchr(info->db[i].file_name, '/');
		if (!filename)
			filename = info->db[i].file_name;
		else
		filename++;
		
		sprintf(textmsg, "\t%s", filename);
		if (info->db[i].fontname_found != -1) {
			sprintf(textmsg2,"%s\t\t  (%s)\n", 
				textmsg,  info->font_name->strs[info->db[i].fontname_found]);
				strcpy(textmsg, textmsg2);
		} else {
			strcat(textmsg, "\n");
		}
		if (textmsg[0] != NULL)  {
			this_len = strlen(textmsg);
			if ((len_used + this_len) >= len_alloc) {
				len_alloc = len_alloc + INC;
				ptr = (char *) realloc(ptr, len_alloc);
				}
		if (ptr) strcat(ptr, textmsg);
		}		/* end if textmsg */

		found++;
		}		/* end if missing afm */

	}		/* end for */

	if (found) {
		flat=XtVaCreateManagedWidget("flat",
				staticTextWidgetClass,
				control,
				XtNstring, ptr, 0);
	    callRegisterHelp(notice_popup, ClientName, TXT_HELP_MISSING_AFM_FILES_SECTION);
		MapGizmo(PopupGizmoClass, &notice);
		}
}  /* end of StaticTextCB() */


static void
NoticeCancelCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{

    BringDownPopup((Widget) _OlGetShellOfWidget(w));
    callRegisterHelp(app_shellW, ClientName, TXT_HELP_MAIN_MENU);
    free(ptr);
    XtDestroyWidget(flat);
    

} /* end of CancelCB */



