#ident	"@(#)dtadmin:fontmgr/font_add.c	1.25.2.19"
/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_add.c
 */

#include <stdio.h>

#include <StringDefs.h>
#include <Intrinsic.h>

#include <OpenLook.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <ModalGizmo.h>
#include <PopupGizmo.h>
#include <RubberTile.h>
#include <InputGizmo.h>

#include <ScrolledWi.h>
#include <FList.h>
#include <Gauge.h>
#include <Caption.h>
#include <Stub.h>

#include <DtI.h>
#include <fontmgr.h>
#include <sys/time.h>
#include <sys/resource.h>

enum media_status { NOT_DOS, NOT_ATM, ATM, NOT_INSERTED, NO_SUCH_DEVICE };

/*
 * external data
 */
extern void ErrorCancelCB();
extern Widget       app_shellW;		  /* application shell widget       */
extern Widget base_shell;
extern Boolean install_allowed;
extern char *xwin_home;

extern    FILE *ForkWithPipe();

extern StandardCursor();
extern void HelpCB();
static int afm_missing;
static void AddCBContinue();

static void ExtractFontNames();
static void ReadDosFile();
static void CheckFileType();
static int Check4Subdirs();
static int Check4ExistingFile();
static void SupplementalCancelCB();
static void ApplyNextDiskCB();
static void ApplyCB();
static void ApplyAllCB();
static void CancelCB();
static void PopupAddWindow(Boolean);

static string_array_type _font_name; 
static string_array_type _disk_label;
static add_type add_info = { &_font_name, &_disk_label };

static HelpInfo help_insert_disk = { 0, 0, HELP_PATH, TXT_HELP_INSERT_DISK };
static HelpInfo help_insert_supplemental_disk = { 0, 0, HELP_PATH, TXT_HELP_INSERT_SUPPLEMENTAL_DISK };
static HelpInfo help_insert_dos_disk = { 0,0, HELP_PATH, TXT_HELP_INSERT_DOS_DISK };
static HelpInfo help_not_dos_disk = { 0,0, HELP_PATH, TXT_HELP_NOT_DOS_DISK };
static HelpInfo help_insert_atm_disk = {0,0, HELP_PATH, TXT_HELP_INSERT_ATM_DISK };
static HelpInfo help_install = {0,0, HELP_PATH, TXT_HELP_INSTALL };

#define APPLY_BUT 0
#define APPLY_ALL_BUT 1
static MenuItems menu_item[] = {  
{ TRUE, TXT_ADD_APPLY,     ACCEL_ADD_APPLY    , 0, ApplyCB, (char *) &add_info },
{ TRUE, TXT_ADD_APPLY_ALL, ACCEL_ADD_APPLY_ALL, 0, ApplyAllCB, (char *)&add_info},
{ TRUE, TXT_CANCEL,    ACCEL_ADD_CANCEL   , 0, CancelCB, (char *)&add_info },
{ TRUE, TXT_HELP_DDD,      ACCEL_ADD_HELP     , 0, HelpCB, (char *)&help_install },
{ NULL }
	      };

static MenuGizmo menu = {0, "[dm", "dm", menu_item};
static PopupGizmo popup = {0, "dp", TXT_FONT_ADD, (Gizmo)&menu, 0, 0, 0,
		0, 0, 0, 100 };


/*
 * get the ATM label for the specified disk number
 */
static String GetDiskLabel(add_type *info)
{
    int i;
    static char id[MAX_STRING], disk_label[MAX_STRING];
    int disk_num;

    disk_label[0] = 0;
    for(i=0; i<info->disk_label->n_strs; i++) {
	sscanf(info->disk_label->strs[i], "%s %d '%[^']",
	       id, &disk_num, disk_label);
	if (disk_num == info->disk_num)
	    break;
	else
	    disk_label[0] = 0;
    }
    return disk_label;

} /* end of GetDiskLabel */


void AddCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    PopdownPrompt();
    BusyCursor(0);
   PromptUser(TXT_ADD_TAKES_TIME, AddCBContinue, client_data,
               CancelCB, client_data, &help_install,
		TXT_PROMPT_ADD, TXT_HELP_INSTALL);
}


static void 
AddCBContinue(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    PopdownPrompt();
    ScheduleWork (PopupAddWindow, FALSE, 1);

} /* end of AddCB */


static void
CreateAddWindow( info)
    add_type *info;
{
    static String item_fields[] = {XtNlabel};
    Widget scroll_win, upper, list_caption, gauge_caption, rubber;
    int n;
    Arg largs[20];

    /* if popup doesn't exist, then create it */
    if (!info->popup) {
	info->popup = CreateGizmo(base_shell, PopupGizmoClass,
			      &popup, NULL, 0);

	XtVaGetValues( info->popup, XtNupperControlArea, &upper, 0);

	n = 0;
	XtSetArg(largs[n], XtNorientation, OL_HORIZONTAL); n++;
	rubber = XtCreateManagedWidget("rubber",
			rubberTileWidgetClass, upper, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNlabel, GetGizmoText(TXT_ADD_LIST_CAPTION)); n++;
	XtSetArg(largs[n], XtNposition, OL_TOP);    n++;
	XtSetArg(largs[n], XtNalignment, OL_CENTER);    n++;
	list_caption = XtCreateManagedWidget("lc",
			captionWidgetClass, rubber, largs, n);

	/*  Create the controls in the upper control area.  */
	n = 0;
	scroll_win = XtCreateManagedWidget("scroll_win",
		  scrolledWindowWidgetClass, list_caption, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNwidth, 40); n++;
	XtSetArg(largs[n], XtNheight, 40); n++;
	XtSetArg(largs[n], XtNweight, 10); n++;
	XtCreateManagedWidget("stub", stubWidgetClass, rubber, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNlabel, GetGizmoText(TXT_ADD_GAUGE_CAPTION)); n++;
	XtSetArg(largs[n], XtNposition, OL_TOP);    n++;
	XtSetArg(largs[n], XtNalignment, OL_CENTER);    n++;
	gauge_caption = XtCreateManagedWidget("lc",
			captionWidgetClass, rubber, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNminLabel, "0 %");       n++;
	XtSetArg(largs[n], XtNmaxLabel, "100 %");       n++;
	XtSetArg(largs[n], XtNticks, 10);       n++;
	XtSetArg(largs[n], XtNtickUnit, OL_PERCENT);       n++;
	XtSetArg(largs[n], XtNshowValue, FALSE);       n++;
	info->gauge = XtCreateManagedWidget("g", 
				gaugeWidgetClass, gauge_caption, largs, n);

	n = 0;
	XtSetArg(largs[n], XtNviewHeight, NUM_FAMILY_ITEMS);    n++; 

	/* WARNING: always specify XtNitems when creating a flatlist,
	   the widget gets confused and core dumps upon
	   subsequent SetValues */
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
    /* reset gauge */
    XtVaSetValues( info->gauge, XtNsliderValue, 0, 0);
    info->select_cnt = info->font_name->n_strs;
    XtManageChild(menu.child);
    SetPopupMessage( &popup, GetGizmoText(TXT_BLANK));
    MapGizmo(PopupGizmoClass, &popup);
    StandardCursor(info->popup);

} /* end of CreateAddWindow */


/*
 * insert the family names that the user selected into an array
 */
static void
GetSelectedFonts( info)
    add_type *info;
{
    Boolean selected;
    int i,j, cnt;

	/* initialize the count to the total number of fonts in list */
#ifdef DEBUG
	fprintf(stderr,"GetSelectedFonts\n");
#endif
    info->select_cnt = info->font_name->n_strs;
    for(i=info->font_name->n_strs - 1; i>=0; i--) {
	OlVaFlatGetValues(info->font_list, i,
			  XtNset, &selected, 0);
	if (selected) {
			/* find the matching font */
			/* the matching font is the relative index
				of valid fonts indicated by
				selected not being -1 */
		for (j=0, cnt=-1; j < info->font_cnt; j++) {
#ifdef DEBUG
fprintf(stderr,"db[%d].selected=%d\n",j, info->db[j].selected);
#endif
			if (info->db[j].selected != -1){
				cnt++;
			}					
#ifdef DEBUG
fprintf(stderr,"i=%d cnt=%d\n",i, cnt);
#endif
			if (cnt == i)  info->db[j].selected = 1;
		}
	} else
	info->select_cnt--;
    }
#ifdef DEBUG
fprintf(stderr,"info->select_cnt=%d\n",info->select_cnt);
#endif
} /* end of GetSelectedFonts */


static void InsertAddDB(add_type *info, String file_name,
	int type, int disk)

{
    int match = 0;
    int exist = -1;

#ifdef DEBUG
	fprintf(stderr,"InsertAddDB: file_name=%s type=%d disk=%d \n",file_name, type, disk);

	if (info->db) fprintf(stderr,"font_cnt=%d\n", info->font_cnt);
#endif
	/* initialize db */
    if (info->db == NULL) {
	info->db = XtNew(add_db);
	info->font_cnt = 0;
	InitAddDB(info, 0);
    }
    else {
		/* see if filename is already in data base */
        exist = Check4ExistingFile(info, file_name);
	/* if not there then we need to add a new entry */
	if (exist >=0)  match = exist;
	else
	{
	info->db = (add_db *) XtRealloc((char*)info->db,
					sizeof(add_db) * (info->font_cnt+1));

	match = info->font_cnt;
	InitAddDB(info, match);
	}
	
    }
    switch(type) {
	case PFB_TYPE:
    	info->db[match].pfb_disk =  disk;
		/* if not already a pfa fontfile with the same name
			then increment the count of actual files */
	if (info->db[match].pfa_disk == 0) info->fontfiles_found++;
	break;

	case PFA_TYPE:
    	info->db[match].pfa_disk =  disk;
		/* if not already a pfb fontfile with the same name
			then increment the count of actual files */
	if (info->db[match].pfb_disk == 0) info->fontfiles_found++;
	break;

	case AFM_TYPE:
    	info->db[match].afm_disk =  disk;
	break;

	case INF_TYPE:
    	info->db[match].inf_disk =  disk;
	break;


	default:
	break;

	}

    if (exist == -1) {
    	strcpy(info->db[match].file_name, file_name);
	(info->font_cnt)++;
		/* insert fontname in db or a NULL placeholder
			if we do not yet have fontname extracted */
	
	}

} /* end of InsertAddDB */

InitAddDB(add_type *info, int match)
{
	if (match < 0) return;
	strcpy(info->db[match].file_name, NULL);
	info->db[match].pfa_disk = info->db[match].pfb_disk = 0;
	info->db[match].inf_disk = info->db[match].afm_disk = 0;
	info->db[match].fontname_found = -1;
	info->db[match].selected = 0;
	info->db[match].done = 0;
	info->db[match].missing_afm = 0;
}

static int
Check4ExistingFile(add_type *info, String file_name)
{
    int  exist = -1;
    int i;
    for (i=0; i < info->font_cnt; i++) {
	if (strcmp(info->db[i].file_name, file_name) == 0) {
		exist = i;
		break;
		}
	}
return exist;
}	


static Boolean
ReadType1(add_type *info, String dir)
{
    FILE *fp;
    char path[PATH_MAX];
    char buf[PATH_MAX];
    char body[MAX_STRING];
    char suffix[MAX_STRING];
    int subdirs, check_directory = 0;
    int i, cnt,pid, w;

    sprintf( buf, "%s:/%s", info->device, dir);
    if ((fp = ForkWithPipe(&pid, "/usr/bin/dosls", buf, NULL)) == NULL) {
	perror("fontmgr");
	return FALSE;
    }
    while (fgets(buf, sizeof(buf), fp) != NULL) {
	if ((cnt=sscanf(buf, "%[^.]%s", body, suffix) < 2)) {
		/* check for subdirectories if no file suffix */
		sscanf(buf, "%s%s", body, suffix);
		if (cnt == 1) suffix[0]=NULL;
		if (!body) continue;
		if (dir ) 
    		sprintf( path, "%s/%s",  dir, body);
		else
    		sprintf( path, "%s",  body);
		subdirs = Check4Subdirs(info, dir, path, body);
		if (subdirs) continue;
	}
	CheckFileType(info, dir, suffix, body);

    }



    fclose(fp);
    /* if we don't wait for the child process to exit, the child will turn
       into a zombie */
    while ((w = wait(NULL)) != pid && w != -1)
        ;
	/* check for existence of pfa and pfb files */
    for (i=0, cnt=0; i < info->font_cnt; i++) {	
	if ((info->db[i].pfa_disk >0 ) || 
		(info->db[i].pfb_disk > 0))
		cnt++;
	}

    if (cnt) {
        /* insert a stub diskname entry */
        InsertStringDB(info->disk_label, GetGizmoText(INSERT_SUPPLEMENTAL_FORMAT));
        info->adobe_foundry = FALSE;
    }
    else
        return FALSE;

    return TRUE;
} /* end of ReadType1 */


static  int
Check4Subdirs(add_type *info, String dir, String path,  char *body)
{
    FILE *fp;
    char buf[PATH_MAX];
    char prefix[MAX_STRING];
    char suffix[MAX_STRING];
    int subdirs, check_directory = 0;
    int cnt, pid, w;

	/* look into possible  subdirs */
	/* read here for files w/o a suffix - could be a file
		or a directory. If a file

	/* add body of filename to path to search */
#ifdef DEBUG
	fprintf(stderr,"Check4SubDirs path=%s\n", path);
#endif
    sprintf( buf, "%s:%s", info->device, path);
    if ((fp = ForkWithPipe(&pid, "/usr/bin/dosls", buf, NULL)) == NULL) {
        perror("fontmgr");
        return FALSE;
    }
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if ((cnt=sscanf(buf, "%[^.]%s", prefix, suffix) < 2)) {

                /* check for subdirectories if no file suffix */
#ifdef DEBUG
	fprintf(stderr,"Check4Subdirs dir=%s buf=%s prefix=%s suffix=%s\n",dir, buf,prefix, suffix);
#endif
		sscanf(buf, "%s%s", prefix, suffix);
			/* add this filename prefix to the path to check */
			/* e.g. path = a: new path  = a:/z  or
				if path = a:/z then new path = a:/z/z  */
		if (cnt == 1) suffix[0]=0;
#ifdef DEBUG
	fprintf(stderr,"Check4Subdirs NOW prefix=%s\n",prefix);
#endif
		sprintf(path, "%s/%s",  path, prefix);
#ifdef DEBUG
	fprintf(stderr,"Call Check4Subdirs with path=%s\n",path);
#endif
                subdirs = Check4Subdirs(info, path, buf, prefix);
		if (subdirs) continue;
        }
        CheckFileType(info, path, suffix, prefix);

    }

}


static void
CheckFileType(add_type *info, String dir, char *suffix, char *body)
{
    char path_name[MAX_STRING];

#ifdef DEBUG
	fprintf(stderr,"CheckFileType: dir=%s suffix=%s body=%s\n",
		dir, suffix, body);
#endif
	if (strncmp(suffix, ".PFA", 4) == STR_MATCH) {
	    sprintf(path_name, "%s/%s", dir, body);
	    InsertAddDB(info, path_name, PFA_TYPE, info->disk_num);
	}
	else
	if (strncmp(suffix, ".PFB", 4) == STR_MATCH) {
	    sprintf(path_name, "%s/%s", dir, body);
	    InsertAddDB(info, path_name, PFB_TYPE, info->disk_num);
	}
	else
	if (strncmp(suffix, ".INF", 4) == STR_MATCH) {
	    sprintf(path_name, "%s/%s", dir, body);
	    InsertAddDB(info, path_name, INF_TYPE, info->disk_num);
	}
	else
	if (strncmp(suffix, ".AFM", 4) == STR_MATCH) {
	    sprintf(path_name, "%s/%s", dir, body);
	    InsertAddDB(info, path_name, AFM_TYPE, info->disk_num);
	}
}




/*
 * returns FALSE if the specified path didnot contain any font files
 */
static
ReadATM(info)
    add_type *info;
{
    FILE *fp;
    char buf[PATH_MAX];
    char id[MAX_STRING];
    char font_name[MAX_STRING];
    char file_name[MAX_STRING];
    int pid, w, status = TRUE;
    int pfb_disk, inf_disk, ctf_disk, unused, abf_disk, afm_disk;
    
    sprintf( buf, "%s:/install.cfg", info->device);
    if ((fp = ForkWithPipe(&pid, "/usr/bin/doscat", buf, NULL)) == NULL) {
	perror("fontmgr");
	return FALSE;
    }
    while (fgets(buf, sizeof(buf), fp) != NULL) {
	if (sscanf(buf, "%s", id) < 1)
	    continue;
	if (strcmp("FONT", id) == STR_MATCH) {

	     
	    /* got a font entry, parse it */
	    if (sscanf(buf, "%s %s %s %d %d %d %d %d %d",
		       id, font_name, file_name,
		       &pfb_disk, &inf_disk, &ctf_disk,
		       &unused, &abf_disk, &afm_disk) < 9)
		continue;
	    strcat( file_name, "___");
	    if (pfb_disk) {
		info->cfg_found = 1;
		InsertStringDB(info->font_name, font_name);
		InsertAddDB(info, file_name, PFB_TYPE, pfb_disk);
		}
	    if (afm_disk) 
		InsertAddDB(info, file_name, AFM_TYPE, afm_disk);
	    if (inf_disk)  
		InsertAddDB(info, file_name, INF_TYPE, inf_disk);
	}
	else if (strcmp("DISKNAME", id) == STR_MATCH) {

	    /* got a diskname entry, save it */
#ifdef DEBUG
	fprintf(stderr,"DISKNAME=%s\n",buf);
#endif
	    InsertStringDB(info->disk_label, buf);
	}
    } /* end while */
    fclose(fp);
    /* if we don't wait for the child process to exit, the child will turn
       into a zombie */
    while ((w = wait(NULL)) != pid && w != -1)
	;

    /* if there are no PFB files, then try the basic ATM fonts */
    if (info->font_name->n_strs == 0) {
	status = ReadType1( info, "psfonts");
    }

    /* if there are no ATM fonts, try third party Type1 fonts */
    if (status == FALSE)
	status = ReadType1(info, "");

    if (status) {
	/* make sure that the list of fonts excludes those
	with just afm or inf files, and extract fontname
	from appropriate file. The order of extraction of	
	fontname is use:
	1. install.cfg above if it exists.
	2. use  pfa
	3. use afm
	4. use inf
      	5. use pfb 
	6. use filename w/o the suffix
	*/

	if (!info->cfg_found) ExtractFontNames(info);
	}
    return status;
} /* end of ReadATM */


static void
InstallFonts(info)
    add_type *info;
{
    int i, cur_disk_num, pid, w, exit_status;
    char afm_dir[MAX_PATH_STRING];
    char *font_list[1];
    char sys_cmd[MAX_PATH_STRING];
    char buf[MAX_PATH_STRING];
    char font_dir[MAX_PATH_STRING];
    char lowername[MAX_PATH_STRING];
    String file_name;
	/* should check if directory exists */
	/* get owner and group ids */

    afm_missing = 0;
    sprintf(font_dir, "%s/lib/fonts/type1", xwin_home);
    sprintf(afm_dir, "%s/%s", font_dir, AFM_DIR);
    mkdir(font_dir, 0755);
    mkdir(afm_dir, 0755);

    chown(font_dir, 2, 2);
    chown(afm_dir, 2, 2);

    cur_disk_num = info->disk_num;
    for(i=0; i<info->font_cnt; i++) {

		/* if both pfb & pfa exist just load pfb */
	
#ifdef DEBUG
	fprintf(stderr,"InstallFonts: file=%s disk=%d pfb=%d pfa=%d afm=%d\n",
		info->db[i].file_name,
		info->disk_num, 
		info->db[i].pfb_disk, 
		info->db[i].pfa_disk, 
		info->db[i].afm_disk);
#endif
	if (info->db[i].pfa_disk && info->db[i].pfb_disk)
		info->db[i].pfa_disk = 0;

	/* if pfb or pfa exists and there is no afm disk then save
		the afm missing status and increment the count
		of missing afm files */
	/*  Can't use afm_disk because that will get reset 
		before check is done
		to show missing afm files */
        if ((info->db[i].afm_disk == 0)  
		&& ((info->db[i].pfb_disk > 0) || 
	   	(info->db[i].pfa_disk > 0))) {
			afm_missing++;
			info->db[i].missing_afm = 1;
	}

	if ((info->db[i].done == 0) &&
		(info->db[i].selected == 1) &&
		(info->db[i].pfb_disk == info->disk_num)) {
	/*if (info->db[i].pfb_disk == info->disk_num) {*/
	    /* copy PFB files from dos disk to unix disk */
	    sprintf(sys_cmd, "/usr/bin/doscp %s:/%s.pfb %s",
		    info->device, info->db[i].file_name, font_dir);
#ifdef DEBUG
	fprintf(stderr,"cmd=%s\n", sys_cmd);
#endif
	    system(sys_cmd);
	    /* generate ascii version */
	    file_name = strrchr(info->db[i].file_name, '/');
	    if (file_name)
		file_name++;
	    else
		file_name = info->db[i].file_name;
	    if (file_name) {
		strcpy(lowername, file_name);
		LowercaseStr(lowername);
		sprintf(sys_cmd, "/bin/mv %s/%s.pfb %s/%s.pfb", font_dir, lowername, font_dir, file_name);
#ifdef DEBUG
	fprintf(stderr,"cmd=%s\n", sys_cmd);
#endif
	        system(sys_cmd);
		}
	    sprintf(buf, "%s/%s.pfb", font_dir, file_name);
	    chown(buf, 2 ,2);
	    info->db[i].done = 1 ;       /* mark file as done */
	 }

	if ((info->db[i].done == 0) &&
		(info->db[i].selected == 1) &&
		(info->db[i].pfa_disk == info->disk_num)) {
		/*if (info->db[i].pfa_disk == info->disk_num) {*/
	    /* copy PFA files from dos disk to unix disk */
	    sprintf(sys_cmd, "/usr/bin/doscp %s:/%s.pfa %s",
		    info->device, info->db[i].file_name, font_dir);
	    system(sys_cmd);
	    file_name = strrchr(info->db[i].file_name, '/');
	    if (file_name)
		file_name++;
	    else
		file_name = info->db[i].file_name;
	    if (file_name) {
		strcpy(lowername, file_name);
		LowercaseStr(lowername);
		sprintf(sys_cmd, "/bin/mv %s/%s.pfa %s/%s.pfa", font_dir, lowername, font_dir, file_name);
#ifdef DEBUG
	fprintf(stderr,"cmd=%s\n",sys_cmd);
#endif
	       system(sys_cmd);
		}

	    sprintf(buf, "%s/%s.pfb", font_dir, file_name);
	    chown(buf, 2 ,2);
	    info->db[i].done = 1;       /* mark file as done */
	}

	/* copy AFM files from dos disk to unix disk */
	if ((info->db[i].selected == 1) &&
	(info->db[i].afm_disk == info->disk_num)) {
	    sprintf(sys_cmd, "/usr/bin/doscp %s:/%s.afm %s",
		    info->device, info->db[i].file_name, afm_dir);
	    sprintf(buf, "%s/%s.afm", afm_dir, info->db[i].file_name);
	    if (system(sys_cmd) && info->adobe_foundry) {
		break;
	    }
	    file_name = strrchr(info->db[i].file_name, '/');
	    if (file_name)
		file_name++;
	    else
		file_name = info->db[i].file_name;
	    if (file_name) {
		strcpy(lowername, file_name);
		LowercaseStr(lowername);
		sprintf(sys_cmd, "/bin/mv %s/afm/%s.afm %s/afm/%s.afm", font_dir, lowername, font_dir, file_name);
#ifdef DEBUG
	fprintf(stderr,"cmd=%s\n",sys_cmd);
#endif
	       system(sys_cmd);
		}

	    sprintf(buf, "%s/afm/%s.afm", font_dir, file_name);
	    chown(buf, 2 ,2);
	    XtVaSetValues( info->gauge, XtNsliderValue, ++info->slider_val, 0);
	    XFlush(XtDisplay(info->gauge));
	}
    } /* for i */
#ifdef DEBUG
	fprintf(stderr,"info->disk_num=%d\n", info->disk_num);
#endif

    /* make sure we got all the fonts out of the current disk */
    for(i=0; i<info->font_cnt; i++) {

#ifdef DEBUG
	fprintf(stderr,"info->db[%d]: done=%d selected=%d afm_disk=%d\n",i,info->db[i].done, info->db[i].selected, info->db[i].afm_disk);

#endif

	if ((info->db[i].selected == 1) &&
		(info->db[i].done == 1) &&
		((info->db[i].afm_disk == info->disk_num) ||
		 (info->db[i].missing_afm == 1))) {
			info->select_cnt--;
#ifdef DEBUG
			fprintf(stderr,"decremented info->select_cnt now =%d\n",info->select_cnt);
#endif
		} 
	}
	    
    sprintf(buf, "%s%s", font_dir, "/*");
    chown(buf, 2, 2);
    sprintf(buf, "%s%s", afm_dir, "/*");
    chown(buf, 2, 2);
    ++(info->disk_num);
#ifdef DEBUG
	fprintf(stderr, "info->disk_num=%d cur_disk_num=%d\n",info->disk_num,cur_disk_num);
	fprintf(stderr,"info->select_cnt=%d\n", info->select_cnt);
#endif
    /* if wanted disk is same as current disk then we are done */ 
    if ((info->disk_num <= info->disk_label->n_strs) &&
	(info->select_cnt > 0))  {
#ifdef DEBUG
	fprintf(stderr,"info->disk_num=%d info->disk_label->n_strs=%d\n",
	info->disk_num, info->disk_label->n_strs);
#endif
	sprintf(buf, GetGizmoText(INSERT_FORMAT), GetDiskLabel(info));
#ifdef DEBUG
	fprintf(stderr,"call promptuser for supplemental disk\n");
#endif
	PromptUser(buf, ApplyNextDiskCB, info,
               SupplementalCancelCB, info, &help_insert_supplemental_disk, TXT_PROMPT_ADD,
			TXT_HELP_INSERT_SUPPLEMENTAL_DISK);
    }
    else {
	if (afm_missing) CreateNotice(info, afm_missing);
	font_list[0] = font_dir;
	UpdateFonts( font_list, 1, 0);
	SetPopupMessage(&popup, GetGizmoText(TXT_ADD_FINISH));
	InformUser(TXT_ADD_FINISH);
    	StandardCursor(info->popup);
	BringDownPopup((Widget) _OlGetShellOfWidget(info->popup));
    }
} /* end of InstallFonts */


static void
DoApply( info)
    add_type *info;
{
    if (info->select_cnt) {
	SetPopupMessage( &popup, GetGizmoText(TXT_ADD_START));
	XtUnmanageChild(menu.child);
	BusyCursor(info->popup);
	XtVaSetValues( info->gauge, XtNsliderMax, (info->select_cnt*2)+2,
		  XtNsliderValue, info->slider_val, 0);
	XSync (XtDisplay(info->popup), FALSE);
	ScheduleWork(InstallFonts, info, 2);
    }
    else {
	PopupErrorMsg(TXT_NONE_ADD_SEL,
		ErrorCancelCB,
		NULL,
		TXT_PROMPT_ADD,
		&help_install,
		TXT_HELP_INSTALL);

	InformUser(TXT_NONE_ADD_SEL);
	StandardCursor(info->popup);
    }
} /* end of DoApply */


static void
SupplementalCancelCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    add_type *info = (add_type *) client_data;

    PopdownPrompt();
    XSync(XtDisplay(info->popup), FALSE);
    if (info->popup)  
	BringDownPopup((Widget) _OlGetShellOfWidget(info->popup));
    StandardCursor(info->popup);
    callRegisterHelp(app_shellW, ClientName, TXT_HELP_MAIN_MENU);
   

} /* end of SupplementalCancelCB */

static void
ApplyNextDiskCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    add_type *info = (add_type *) client_data;

    
    info->disk_num++;
    PopdownPrompt();
    XSync(XtDisplay(info->popup), FALSE);
    XSync(XtDisplay(info->popup), FALSE);
    XSync(XtDisplay(info->popup), FALSE);
    ScheduleWork(InstallFonts, info, 2);

} /* end of ApplyCB */


void
ApplyCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    add_type *info = (add_type *) client_data;

    GetSelectedFonts(info);
    DoApply(info);
    callRegisterHelp(app_shellW, ClientName, TXT_HELP_MAIN_MENU);

} /* end of ApplyCB */


void
ApplyAllCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    add_type *info = (add_type *) client_data;

    int i;


    info->select_cnt = info->font_name->n_strs;
    for (i=0; i< info->font_cnt; i++) {
		/* for all fonts that have a pfa or pfb select the 
			entry on install all */

	if ((info->db[i].pfa_disk != 0) || (info->db[i].pfb_disk != 0 ))
		info->db[i].selected = 1;

    	}
	DoApply(info);

    callRegisterHelp(app_shellW, ClientName, TXT_HELP_MAIN_MENU);
} /* end of ApplyAllCB */


void
CancelCB(w, client_data, call_data)
     Widget w;
     XtPointer client_data;
     XtPointer call_data;
{
    add_type *info = (add_type *) client_data;

    PopdownPrompt();
    if (info->popup)
	BringDownPopup((Widget) _OlGetShellOfWidget(info->popup));
    StandardCursor(info->popup);
	
} /* end of CancelCB */


void
ErrorCancelCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	PopdownError();
}


static
GetNameOfFont( info)
    add_type *info;
{
    int status, str_len;
    char buf[PATH_MAX];
    extern char * _dtam_fstyp;

    /* we need to resolve the link because dosdir has problems with links */
    if ((str_len = readlink(info->device, buf, PATH_MAX)) != -1) {
	strncpy(info->device, buf, str_len);
	info->device[str_len] = '\0';
    }

    if (ReadATM( info))
        return ATM;
    else

    status = DtamCheckMedia(info->device);
    if ((status == DTAM_FS_TYPE)  && 
	(_dtam_fstyp != NULL) && (strcmp(_dtam_fstyp,"dosfs") == 0)) {
	if (ReadATM( info))
	    return ATM;
	else
	    return NOT_ATM;
    }

    if (status == DTAM_NO_DISK)
	return NOT_INSERTED;
    else if (status == DTAM_BAD_DEVICE)
	return NO_SUCH_DEVICE;
    else
	return NOT_DOS;
} /* end of GetNameOfFont */


static void
PopupAddWindow(Boolean device_specified)
{
    int status, status_B = NOT_INSERTED, i;
    add_type *info = &add_info;

    if (install_allowed == FALSE) {
	StandardCursor(0);
	return;
    }
    /* init */
    DeleteStringsDB( info->font_name);
    DeleteStringsDB( info->disk_label);
    if (info->db)
	XtFree((char*)info->db);
    info->db = NULL;
    info->fontfiles_found = 0;
    info->cfg_found = 0;
    info->font_cnt = 0;
    info->slider_val = 1;
    info->disk_num = 1;
    info->adobe_foundry = TRUE;

    /* try both diskettes if none if specified */
    if (device_specified) {
#ifdef DEBUG
	fprintf(stderr,"device_specified\n");
#endif
	status = GetNameOfFont(info);
#ifdef DEBUG
	fprintf(stderr,"status=%d\n",status);
#endif
    }
    else {
#ifdef DEBUG
	fprintf(stderr,"device not specified\n");
#endif
	strcpy( info->device, "/dev/dsk/f0t");
	if ((status = GetNameOfFont(info)) != ATM) {
#ifdef DEBUG
	fprintf(stderr,"status=%d\n",status);
#endif
	    strcpy( info->device, "/dev/dsk/f1t");
	    status_B = GetNameOfFont(info);
#ifdef DEBUG
	fprintf(stderr,"status_B=%d\n",status_B);
#endif
	}
    }

    if ((status == ATM) || (status_B == ATM)) {
	PopdownPrompt();
	CreateAddWindow(info);
    }
    else {
	if (info->popup) {
	    BringDownPopup((Widget) _OlGetShellOfWidget(info->popup));
    	    callRegisterHelp(app_shellW, ClientName, TXT_HELP_MAIN_MENU);
	    StandardCursor(info->popup);
	}	
	if (status == NOT_DOS) {
#ifdef DEBUG
	fprintf(stderr,"status is not dos\n");
#endif
	    PromptUser(TXT_INSERT_DOS_DISK, AddCB, info,
               CancelCB, info, &help_not_dos_disk,
		TXT_PROMPT_ADD, TXT_HELP_NOT_DOS_DISK);
	}
    	else if (status == NOT_ATM) {
#ifdef DEBUG
	fprintf(stderr,"status is not atm\n");
#endif
	    PromptUser(TXT_INSERT_ATM_DISK, AddCB, info,
               CancelCB, info, &help_insert_atm_disk,
		TXT_PROMPT_ADD, TXT_HELP_INSERT_ATM_DISK);
	}
	else {
#ifdef DEBUG
	fprintf(stderr,"status is not inserted\n");
#endif
	    PromptUser(TXT_INSERT_DISK, AddCB, info,
               CancelCB, info, &help_insert_disk,
		TXT_PROMPT_ADD, TXT_HELP_INSERT_DISK);
	}
    }
    StandardCursor(0);

} /* end of PopupAddWindow */


void
DnDPopupAddWindow( path)
    String path;
{
    strcpy(add_info.device, path);
    BusyCursor(0);
    ScheduleWork(PopupAddWindow, TRUE, 1);

} /* DnDPopupAddWindow */


static void 
ExtractFontNames(add_type *info)
{
 	int j, format, i, k,f;
	char *fontp;
	char filename[MAX_STRING];
	char input[MAX_PATH_STRING];


	if (info->font_cnt != info->fontfiles_found) {
		/* there are some afm or inf files on the disk
			without pfa or pfb files so we need to
		delete them from the list of fonts to show */
		
		for (i=0; i < info->font_cnt; i++) {
			if ((info->db[i].pfa_disk == 0) &&
				(info->db[i].pfb_disk == 0)) {
					InitAddDB(info, i);
					/* set selected to indicate
						invalid selection*/ 
					info->db[i].selected = -1;
					}
		}
	}
		/* reset font_cnt */

	/* make sure that the list of fonts excludes those
	with just afm or inf files, and extract fontname
	from appropriate file. The order of extraction of	
	fontname is use:
	1. install.cfg above if it exists.
	2. use  pfa
	3. use afm
	4. use inf
  	5. use pfb 
	6. use filename w/o the suffix
	*/
		
	for (i=0; i < info->font_cnt; i++) {

		if (info->db[i].fontname_found != -1) continue;
			/* skip fonts without pfa or pfb */
		if (info->db[i].done == 1) continue;
		if ((info->db[i].pfa_disk == 0) &&
			(info->db[i].pfb_disk == 0)) continue;
		if (info->db[i].pfa_disk) {
			sprintf(filename, "%s.PFA", info->db[i].file_name);
			ReadDosFile(info,  filename, i, PFA_TYPE);
			if (info->db[i].fontname_found != -1) continue;
		}
		if (info->db[i].afm_disk) {
			sprintf(filename, "%s.AFM", info->db[i].file_name);
			ReadDosFile(info,  filename, i, AFM_TYPE);
			if (info->db[i].fontname_found != -1) continue;
		}
		if (info->db[i].inf_disk) {
			sprintf(filename, "%s.INF", info->db[i].file_name);
			ReadDosFile(info,  filename, i, INF_TYPE);
			if (info->db[i].fontname_found != -1) continue;
		}
			/* use the filename as the fontname but we
				must extract the name without any
				directories */
		fontp = strrchr(info->db[i].file_name, '/');
		if (!fontp) 
				/* there are no '/' in file_name so
				use as the fontname */
			InsertStringDB(info->font_name, info->db[i].file_name);
			
		else {
			fontp++;
			if (fontp) 
				InsertStringDB(info->font_name, fontp);
			else
				InsertStringDB(info->font_name, info->db[i].file_name);
			}
	} /* end for */

}


static void
ReadDosFile(add_type *info, String filename, int match, int format)
{

    FILE *fp;
    int w, pid;
    char *fontp, *ptr;
    char buf[PATH_MAX];
    char fld1[MAX_STRING];
    char sys_cmd[MAX_PATH_STRING];
    int len=0;
    Boolean found = 0;
    char fontname[MAX_STRING];
    
	/* if pfb we are reading from /tmp not from DOS diskette */
    strcpy(sys_cmd, "/usr/bin/doscat");
    sprintf(buf,"%s:%s", info->device,filename);

    if ((fp = ForkWithPipe(&pid, sys_cmd, buf,  NULL)) == NULL) {
	perror("fontmgr");
	return;
    }
    while (fgets(buf, sizeof(buf), fp) != NULL) {
	ptr = buf;
	if (found) continue;
	if (sscanf(buf, "%s", fld1) < 1) continue;
	if (!fld1) continue;

	switch (format) { 


	case PFA_TYPE:

		/* look for format for pfa or pfb files 
		/FontName /Bodoni-PosterCompressed def
		*/
		if (strcmp(fld1, "/FontName") != STR_MATCH) continue;
			/* get second '/' */
		if (sscanf(buf, "%s%s", fld1, fontname) < 2) continue;
		fontp = strchr(fontname,'/');
		if (fontp) fontp++;
		
		if (!fontp) continue;
	  	found = 1;
		info->db[match].fontname_found = info->font_name->n_strs;
		InsertStringDB(info->font_name, fontp);
		break;
			
	case INF_TYPE:

		/* look for INF file format
		FontName (Bodoni-PosterCompressed)
		*/
		
		if (strcmp(fld1, "FontName") != STR_MATCH) continue;
			/* get second '/' */
		if (fontp = strchr(buf,'(')) fontp++;
		if (!fontp) continue;
		ptr = strchr(fontp, ')');
		if (!ptr) continue;
		len = ptr-fontp;
		strncpy(fontname, fontp, len);
		strcpy(fontname+len, "\0");
	  	found = 1;
		info->db[match].fontname_found = info->font_name->n_strs;
		InsertStringDB(info->font_name, fontname);
		break;
	case AFM_TYPE:
		/* look for AFM file format 
		FontName Bodoni-PosterCompressed
		*/
		if (strcmp(fld1, "FontName") != STR_MATCH) continue;
			/* get second '/' */
		if(sscanf(buf, "%s%s", fld1, fontname) < 2) continue;
		if (!fontname) continue;
	  	found = 1;
		info->db[match].fontname_found = info->font_name->n_strs;
		InsertStringDB(info->font_name, fontname);
		break;

	default:
		break;
		}
	}

	fclose(fp);
    /* if we don't wait for the child process to exit, the child will turn
       into a zombie */
    while ((w = wait(NULL)) != pid && w != -1)
        ;

}

