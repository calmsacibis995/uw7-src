#ident	"@(#)dtadmin:fontmgr/font_prop.c	1.15"
/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_prop.c
 */

#include <stdio.h>
#include <Intrinsic.h>
#include <StringDefs.h>

#include <OpenLook.h>

#include <Gizmos.h>
#include <PopupGizmo.h>
#include <MenuGizmo.h>
#include <NumericGiz.h>
#include <ChoiceGizm.h>
#include <CheckBox.h>
#include <LabelGizmo.h>

#include <fontmgr.h>

#define MAX_CACHE_SIZE 4000     /* unit is in Kbytes */
#define MAX_CACHE_CHARS 4
#define MAX_DEFAULT_PS_CHARS 2
#define MIN_DEFAULT_PS       6
#define MAX_DEFAULT_PS      36

/*
 * external procedures
 */
extern char *getenv();

extern char *xwin_home;

extern void HelpCB();


static Boolean renderer_exist[E_MAX_PROP]={ 
    TRUE };     /* set gereral item to true */

#define CACHE_INDEX  0        /* this is an index into the general_cfg array */
#define MIN_CACHE_INDEX 1
#define DERIVED_PS_INDEX 2
static config_type general_cfg[]= {
     {"cachesize", DEFAULT_CACHE_SIZE,           MIN_MATCH},
     {"mincachesize", "750",        MIN_MATCH},
     {"derived-instance-pointsizes", DEFAULT_POINT_SIZE, MIN_MATCH},
     {0}   /* terminator */
};


static string_array_type file_a;
static prop_type prop_info = { general_cfg };





/* returns TRUE if a keyword, value pair is found */
static Boolean
ParseKeywordValue(
    char* line,
    char* keyword,
    char* value)
{
    int len;
    char *start, *end;

    start = line;
    if (SkipSpace(&start) == FALSE)
	return FALSE;

    /* if comment then return */
    if (start[0]=='#')
	return FALSE;

    /* parse keyword */
    end = start;
    if (FindChar(&end, '=') == FALSE)
	return FALSE;
    len = end - start;
    strncpy(keyword, start, len);
    keyword[len] = 0; /* string terminator */

    /* parse value */
    start = end + 1;
    if (SkipSpace(&start) == FALSE)
	return FALSE;
    end = start;
    if (FindSpace(&end) == FALSE)
	return FALSE;
    len = end - start;
    strncpy(value, start, len);
    value[len] = 0;  /* string terminator */
    return TRUE;

} /* ParseKeywordValue */



static Boolean
InsertConfigDB(prop_type* info,
	       char* line)
{
    char keyword[MAX_STRING], value[MAX_PATH_STRING];
    config_type *cfg;
    int i;

    if (!ParseKeywordValue(line, keyword, value))
	return FALSE;

    /*if (ParseRendererType(info, keyword, value))
	return FALSE;*/

    cfg = info->cur_cfg;
    for(i=0; *cfg[i].keyword; i++) {
	if (strncmp( keyword, cfg[i].keyword, cfg[i].match_len) ==STR_MATCH) {
	    strcpy(cfg[i].value, value);
	    return TRUE;
	}
    }
    return FALSE;

} /* InsertConfigDB */


    
static void
InitCfgReplace(cfg)
    config_type *cfg;
{
    int i;

    for(i=0; *cfg[i].keyword; i++) {
	cfg[i].replaced = FALSE;
    }
}

static void
InitCfg(cfg)
    config_type *cfg;
{
    int i;

    for(i=0; *cfg[i].keyword; i++) {
	strcpy(cfg[i].value, cfg[i].default_value);
    }
    InitCfgReplace(cfg);
} /* end of InitCfg */




static Boolean
ReadCfg( prop_info, clean_up)
    prop_type *prop_info;
    Boolean clean_up;
{
    FILE *file;
    char *dir;
    char line[MAX_PATH_STRING];

    /* open file */
    if ((dir = getenv("XWINFONTCONFIG"))==NULL) {
	sprintf(prop_info->filename, "%s/lib/fonts/type1/config", xwin_home);
    }
    else 
	strcpy(prop_info->filename, dir);
    
    file = fopen(prop_info->filename, "r");
    if (!FileOK(file)) {
	sprintf(line, GetGizmoText(FORMAT_UNABLE_OPEN_CONFIG), prop_info->filename);

	InformUser(line);
	return FALSE;
    }
    
    InitCfg(general_cfg);
    prop_info->cur_cfg = general_cfg;
    while (fgets(line, MAX_PATH_STRING, file) != NULL) {
	InsertStringDB(&file_a, line);
	InsertConfigDB(prop_info, line);
    }

    fclose(file);

    if (clean_up)
	DeleteStringsDB(&file_a);
    return TRUE;

} /* end of ReadCfg */


String
GetDerivedPS() 
{
    if (ReadCfg(&prop_info, TRUE))
	return general_cfg[DERIVED_PS_INDEX].value;
    else
	return NULL;

} /* end of GetDerivedPS */

