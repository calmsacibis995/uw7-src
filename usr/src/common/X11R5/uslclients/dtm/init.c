/*		copyright	"%c%"	*/

#pragma ident	"@(#)dtm:init.c	1.59"

#include <stdio.h>
#include <libgen.h>
#include <sys/stat.h>
#include <X11/Intrinsic.h>
#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "shortcut.icon"

static const char shortcut_glyph[] = "shortcut.icon";

/* default paths */
static const char dflt_desktopdir[] = "$HOME";
static const char dflt_fclassdb[] = "$HOME/.dtfclass";
static const char dflt_dtfclass[] = "$XWINHOME/desktop/.dtfclass";
static const char dflt_toolbox[] = "%DESKTOPDIR";
static const char dflt_iconpath[] = "";
static const char dflt_wbdir[] = "%DESKTOPDIR/.wastebasket";
static const char dflt_hdpath[] = "$XWINHOME/desktop/Help_Desk";
static const char dflt_templatedir[] = "$XWINHOME/lib/template";

static char *dflt_print = "$XWINHOME/bin/PrtMgr -p %_DEFAULT_PRINTER \"%F\" &";

static int copy_dtfclass(char *new, char *old);
static char *DeEscapeQuotes(char *p);
static void RunStartupItems(void);

static char *
InitDesktopProp(name, dflt, expand)
char *name;
char *dflt; /* default value */
Boolean expand;
{
	char *p;
	char *ret = NULL;
	extern char *getenv();

	if (!(p = getenv(name))) {
		/*
		 * If it is already defined in the prop list, so be it.
		 * Otherwise, use the default.
		 */
		if (p = DtGetProperty(&DESKTOP_PROPS(Desktop), name, NULL))
			return(p);
		else
			p = dflt;
	}

	if (p && *p) {
		if (expand)
			p = Dm__expand_sh(p, DmDTProp, NULL);
		else
			p = strdup(p);
		ret = DtSetProperty(&DESKTOP_PROPS(Desktop), name, p, 0);
		free(p);
	}
	return(ret);
}

static char *
PreProcessRE(re)
char *re;
{
	char *ret;
	register char *p;

	if (p = ret = (char *)malloc(strlen(re) + 2)) {
		strcpy(p, re);

		/* translate all ',' to '\0' */
		for (; *p; p++) {
			if (*p == '\\') {
				/*
				 * make sure we are not
				 * escaping a null char.
				 */
				if (*++p == '\0')
					p--;
			}
			if (*p == ',')
				*p = '\0';
		}
		*++p = '\0'; /* extra null at the end */
	}
	return(ret);
}

void
DmInitFileClass(fnkp)
DmFnameKeyPtr fnkp;
{
	register char *p;

	if (p = DtGetProperty(&(fnkp->fcp->plist), PATTERN, NULL)) {
		p = Dm__expand_sh(p, NULL, NULL);
		if (fnkp->re = PreProcessRE(p))
			fnkp->attrs |= DM_B_REGEXP;
		else
			Dm__VaPrintMsg(TXT_BAD_REGEXP, p);
		free(p);
	}

	if (p = DtGetProperty(&(fnkp->fcp->plist), FILETYPE, NULL)) {
		DmFmodeKeyPtr fmkp = DmStrToFmodeKey(p);

		if (fmkp) {
			fnkp->attrs |= DM_B_FILETYPE;
			fnkp->ftype = fmkp->ftype;
		}
	}

	if (p = DtGetProperty(&(fnkp->fcp->plist), FILEPATH, NULL))
		fnkp->attrs |= DM_B_FILEPATH;

	if (p = DtGetProperty(&(fnkp->fcp->plist), LPATTERN, NULL)) {
		p = Dm__expand_sh(p, NULL, NULL);
		if (fnkp->lre = PreProcessRE(p))
			fnkp->attrs |= DM_B_LREGEXP;
		else
			Dm__VaPrintMsg(TXT_BAD_REGEXP, p);
		free(p);
	}

	if (p = DtGetProperty(&(fnkp->fcp->plist), LFILEPATH, NULL)) {
		fnkp->attrs |= DM_B_LFILEPATH;
		(void)DmResolveLFILEPATH(fnkp, p);
	}

	if (p = DtGetProperty(&(fnkp->fcp->plist), FSTYPE, NULL)) {
		fnkp->fstype = convnmtonum(p);
	}
}

static void
DmGetDesktopProperties(desktop)
DmDesktopPtr desktop;
{
	int lineno;
	int ret;
	char *path;
	FILE *f;
	char name[128];
	char value[256];

	path = DmMakePath(DmGetDTProperty(DESKTOPDIR, NULL), ".dtprops");
	if (f = fopen(path, "r")) {
		lineno = 1;
		while ((ret = fscanf(f,"%[^=]=%[^\n]\n",&name,&value)) == 2) {
			DtSetProperty(&DESKTOP_PROPS(desktop), name,
				DeEscapeQuotes(value), 0);
			lineno++;
		}

		if (ret != EOF) {
			Dm__VaPrintMsg(TXT_SYNTAX, path, lineno);
		}
		(void)fclose(f);
	}
}

/*
 * return values:	 0	success, but previous session was not started.
 *			 1	success, previous session was started.
 *			-1	failed.
 */
int
DmOpenDesktop()
{
	char *		p;
	char *		q;
	DmFnameKeyPtr	fnkp;
	int		ret;
	int		tmp_lastsession = 0;
	struct stat	sbuf;
	char		*savep;
	Boolean		new_dtfclass = False;

    /* If $HOME is '/', then DESKTOPDIR will be "//desktop" which will
       confuse things like the Folder menu.
    */
    p = InitDesktopProp(DESKTOPDIR, dflt_desktopdir, True);
    DESKTOP_DIR(Desktop) = ((p[0] == '/') && (p[1] == '/')) ? p + 1 : p;

    /* Load in desktop properties from the previous session first.
     * If an environment variable is set, it has higher precedence than
     * the old desktop property values.  Save properties unexpanded,
     * (to preserve variables in .dtprops file) and expand them on the
     * fly by using DmDTProp to retrieve them.
     */
    DmGetDesktopProperties(Desktop);

    p = InitDesktopProp(ICONPATH, dflt_iconpath, False);
    DmSetIconPath(p);		/* pass it to libDtI */

    p = InitDesktopProp(WBDIR, dflt_wbdir, False);
    p = InitDesktopProp(HDPATH, dflt_hdpath, False);
    p = InitDesktopProp(TEMPLATEDIR, dflt_templatedir, False);
    p = InitDesktopProp(DFLTPRINTCMD, dflt_print, False);

    DmInitDfltFileClasses();

    p = InitDesktopProp(FILEDB_PATH, dflt_fclassdb, False);
    p = DmDTProp(FILEDB_PATH, NULL);

    /* if user is root, remove the extra '/' */
    q = (p[0] == '/' && p[1] == '/') ? p + 1 : p;

    if ((DESKTOP_FNKP(Desktop) = DmReadFileClassDB(q)) == NULL)
    {
		char *s;
		char *t;

		/* set flag to inform user $FILEDB_PATH was replaced */
		new_dtfclass = True;

		/* Check if FILEDB_PATH is dflt_fclassdb.  If it is,
		 * then make a copy of dflt_dtfclass into the user's
		 * home. If not, try using dflt_fclassdb.  If this
		 * fails, resort to copying dflt_dtfclass.
		 */
		s = Dm__expand_sh((char *)dflt_fclassdb, DmDTProp, NULL);
    		t = (s[0] == '/' && s[1] == '/') ? s + 1 : s;

		q = strdup(q);
		t = strdup(t);
		if (strcmp(t, q) == 0) {
			/* copy default version of .dtfclass from
			 * $XWINHOME/desktop
			 */
			ret = copy_dtfclass(t, q);
		} else {
			/* set FILEDB_PATH property */
			DtSetProperty(&DESKTOP_PROPS(Desktop), FILEDB_PATH,
				t, 0);

    			if ((DESKTOP_FNKP(Desktop) =
				DmReadFileClassDB(t)) == NULL)
			{
				/* copy default version of .dtfclass from
				 * $XWINHOME/desktop
				 */
				ret = copy_dtfclass(t, q);
			} else {
				Dm__VaPrintMsg(TXT_DTFCLASS_MISSING, q, t);
				free(q);
				free(t);
				goto there;
			}
		}
		if (ret == -1)
		{
			/* copy of dflt_dtfclass failed - give up! */
			Dm__VaPrintMsg(TXT_FILE_CLASSDB, q);
			free(q);
			free(t);
			return(-1);
		}

		/* copy of dflt_dtfclass succeeded */
		free(q);
		free(t);
    }
there:

    /* Optimization: preprocess all regular expressions and
     * set flags for typing properties.
     */
    for (fnkp = DESKTOP_FNKP(Desktop); fnkp != NULL; fnkp = fnkp->next)
    {
	if (!(fnkp->attrs & DM_B_CLASSFILE))
	    DmInitFileClass(fnkp);
    }

    /* get shortcut glyph */
    DESKTOP_SHORTCUT(Desktop) = DmGetPixmap(DESKTOP_SCREEN(Desktop),
					    (char *)shortcut_glyph);
    if (DESKTOP_SHORTCUT(Desktop) == NULL)
	DESKTOP_SHORTCUT(Desktop) =
	    DmCreateBitmapFromData(DESKTOP_SCREEN(Desktop),
				   "\n/default shortcut icon\n",
				   (unsigned char *)shortcut_bits,
				   shortcut_width, shortcut_height);

	/* cache frequently used strings to avoid multiple calls to gettxt()
	 * for the same string.
	 */
	dtmgr_title     = Dm__gettxt(TXT_DESKTOP_MGR);
	product_title   = Dm__gettxt(TXT_PRODUCT_NAME);
	folder_title    = Dm__gettxt(TXT_FOLDER_TITLE);
	
    /* restart all the things that were in the previous session */
	/* Restore session saved in .Tlastsession if the the session is
	 * being restarted due to a change in locale.
	 */
	p = DmMakePath(DESKTOP_DIR(Desktop), ".Tlastsession");
	if (stat(p, &sbuf) == -1)
		p = DmMakePath(DESKTOP_DIR(Desktop), ".lastsession");
	else {
		/* make a copy of p because Dm__buffer is used in
		 * DmRestartSession
		 */
		savep = strdup(p);
		tmp_lastsession = 1;
	}
	ret = DmRestartSession(p);

	/* Remove temporary lastsession file so that it won't be used the
	 * for the next session.
	 */
	if (tmp_lastsession) {
		unlink(savep);
		free(savep);
	}

	/* Make sure we have opened the desktop dir */
	if (DESKTOP_TOP_FOLDER(Desktop) == NULL)
	    DmOpenFolderWindow(DESKTOP_DIR(Desktop), 0, NULL, False);

	RunStartupItems();

	if (new_dtfclass && (ret == 0 || ret == 1))
		ret = 2;

    return(ret);
}

static int
copy_dtfclass(char *new, char *old)
{
#define NUM_BYTES 1024

	FILE *rfp;
	FILE *wfp;
	size_t n;
	char *s;
	char buf[NUM_BYTES];
		
	sprintf(Dm__buffer, "mv %s %s.bkp", old, old);
	(void)system(Dm__buffer);

	s = Dm__expand_sh((char *)dflt_dtfclass, DmDTProp, NULL);

	if ((rfp = fopen(s, "r")) == NULL || (wfp = fopen(new, "w")) == NULL)
		return(-1);

	while ((n = fread(buf, 1, NUM_BYTES, rfp)) > 0) {
		if (fwrite(buf, 1, n, wfp) != n) {
			(void)fclose(rfp);
			(void)fclose(wfp);
			return(-1);
		}
	}
	if (ferror(rfp)) {
		/* an error was encountered while reading */
		(void)fclose(rfp);
		(void)fclose(wfp);
		return(-1);
	}
	(void)fclose(rfp);
	(void)fclose(wfp);

	/* set FILEDB_PATH property */
	DtSetProperty(&DESKTOP_PROPS(Desktop), FILEDB_PATH, new, 0);

	/* try to read $HOME/.dtfclass */
	if ((DESKTOP_FNKP(Desktop) = DmReadFileClassDB(new)) == NULL)
		return(-1);
	else {
		/* inform user a default version is created */
		Dm__VaPrintMsg(TXT_warningMsg_noDefaults, old);
		return(0);
	}
}

static char *
DeEscapeQuotes(p)
char *p;
{
     int i = 0;
     char buf[1024];
     char *s = p;

     while (*s) {
          if (*s != '\\') {
               buf[i++] = *s;
          }
          s++;
     }
     buf[i] = '\0';
     return(strdup(buf));

} /* end of DeEscapeQuotes */

/****************************procedure*header*********************************
 *  RunStartupItems
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/

static void 
RunStartupItems(void)
{
    DmFolderWindow folder;
    DmObjectPtr obj;
    DmContainerPtr cp;
    char *startup;
    char path[PATH_MAX];


    startup = Dm_DayOneName("Preferences/Startup_Items", DESKTOP_LOGIN(Desktop));
    strcpy(path, DESKTOP_DIR(Desktop));
    strcat(path, "/");
    strcat(path, startup);

    if ((folder = DmOpenFolderWindow(path, 0, NULL, True)) != NULL){
	DmUnmapWindow((DmWinPtr)folder);
	cp = folder->views[0].cp;
	for (obj=cp->op; obj!= NULL; obj=obj->next){
	    if ( !(IS_DOT_DOT(obj->name)) )
		DmOpenObject((DmWinPtr) folder, obj, 0);
	}
	DmCloseFolderWindow(folder);
    }
    XtFree(startup);

#ifdef NOT_USED
    cp = DmOpenDir(path, 
		   DM_B_TIME_STAMP | DM_B_SET_TYPE | DM_B_INIT_FILEINFO);
    for (obj=cp->op; obj!= NULL; obj=obj->next){
	if (!(IS_DOT_DOT(obj->name)) && !(OBJ_IS_DIR(obj))
	    DmOpenObject(DESKTOP_TOP_FOLDER(Desktop), obj, 0);
    }
    DmCloseDir(cp, DM_B_NO_FLUSH);
#endif

} /* end of RunStartupItems */
