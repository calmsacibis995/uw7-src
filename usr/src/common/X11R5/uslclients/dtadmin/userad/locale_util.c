/*		copyright	"%c%" 	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:userad/locale_util.c	1.1.1.1"
#endif
/*
 *	locale_util.c - LoginMgr locale utility program
 */
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/secsys.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include "LoginMgr.h"

extern void     GetLocaleName(char *);
extern void     GetLocale(char *);
extern void	ErrorNotice();
extern int	exit_code;

extern char	*login, *desc, *home, *group, *remote;
extern char	*shell, *uid, *gname, *gid;
extern char	*userLocale, *userLocaleName;
extern char	*curLocaleName, *curLocale;
extern String	defaultLocale, defaultLocaleName;


/* This routine updates the user's
 * .Xdefaults file with new locale info.
 */
void
ChangeXdefaults()
{

	char       line[BUFSIZ], buf[BUFSIZ];
	char       buf1[BUFSIZ];
	FILE       *fp, *fp1;
	char       *lineptr;
	Boolean    Open1_OK = FALSE;
	Boolean    Open2_OK = FALSE;
	Boolean    found_basic = FALSE;
	Boolean    found_display = FALSE;
	Boolean    found_input = FALSE;
	Boolean    found_numeric = FALSE;
	Boolean    found_time = FALSE;
	Boolean    found_xnl = FALSE;

	/* Open the user's .Xdefaults file and /tmp/.Xdefaults file */
	sprintf(buf, "%s/%s", home, ".Xdefaults");
	if (fp = fopen(buf,"r")) {
		Open1_OK = TRUE;

		if (fp1 = fopen(TMP_XDEFAULTS,"w")) {
			Open2_OK = TRUE;

			/* For each line in the user's .Xdefaults file */
			while (fgets(buf, BUFSIZ, fp)) {
				lineptr = buf;
				if (strncmp (BASICLOCALE, lineptr, 13) == 0) {
					sprintf(buf1, "%s\t%s\n", BASICLOCALE, 
						userLocale);
					fputs(buf1, fp1);
					found_basic = TRUE;
				}
				else if (strncmp (DISPLAYLANG, lineptr, 13) == 0) {
					sprintf(buf1, "%s\t%s\n", DISPLAYLANG, 
						userLocale);
					fputs(buf1, fp1);
					found_display = TRUE;
				}
				else if (strncmp (INPUTLANG, lineptr, 11) == 0) {
					sprintf(buf1, "%s\t%s\n", INPUTLANG, 
						userLocale);
					fputs(buf1, fp1);
					found_input = TRUE;
				}
				else if (strncmp (NUMERIC, lineptr, 9) == 0) {
					sprintf(buf1, "%s\t%s\n", NUMERIC, 
						userLocale);
					fputs(buf1, fp1);
					found_numeric = TRUE;
				}
				else if (strncmp (TIMEFORMAT, lineptr, 12) == 0) {
					sprintf(buf1, "%s\t%s\n", TIMEFORMAT, 
						userLocale);
					fputs(buf1, fp1);
					found_time = TRUE;
				}
				else if (strncmp (XNLLANGUAGE, lineptr, 13) == 0) {
					sprintf(buf1, "%s\t%s\n", XNLLANGUAGE, 
						userLocale);
					fputs(buf1, fp1);
					found_xnl = TRUE;
				}
				else if (strncmp (FONTGROUP, lineptr, 11) == 0) {
					continue;
				}
				else if (strncmp (FONTGROUPDEF, lineptr, 14) == 0) {
					continue;
				}
				else if (strncmp (INPUTMETHOD, lineptr, 13) == 0) {
					continue;
				}
				else if (strncmp (IMSTATUS, lineptr, 10) == 0) {
					continue;
				}
				else
					fputs(lineptr,fp1);

			} /*  While loop */

			/* Check for the critical locale resources */
			if (found_basic == FALSE) {
				sprintf(buf1, "%s\t%s\n", BASICLOCALE, 
					userLocale);
				fputs(buf1, fp1);
			}

			if (found_display == FALSE) {
				sprintf(buf1, "%s\t%s\n", DISPLAYLANG, 
					userLocale);
				fputs(buf1, fp1);
			}

			if (found_input == FALSE) {
				sprintf(buf1, "%s\t%s\n", INPUTLANG, 
					userLocale);
				fputs(buf1, fp1);
			}

			if (found_numeric == FALSE) {
				sprintf(buf1, "%s\t%s\n", NUMERIC, userLocale);
				fputs(buf1, fp1);
			}

			if (found_time == FALSE) {
				sprintf(buf1, "%s\t%s\n", TIMEFORMAT, 
					userLocale);
				fputs(buf1, fp1);
			}

			if (found_xnl == FALSE) {
				sprintf(buf1, "%s\t%s\n", XNLLANGUAGE, 
					userLocale);
				fputs(buf1, fp1);
			}

			fclose(fp1);

		} /* If open fp1 */

		fclose(fp);

	} /* If open fp */

	if (Open1_OK == TRUE && Open2_OK == TRUE) {
		sprintf(buf, "%s %s/.Xdefaults >/dev/null 2>&1", 
		    MOVE_XDEFAULTS, home);
		if ((exit_code = system (buf)) == 0) {
			sprintf(buf,"/usr/bin/chown %s %s/.Xdefaults >/dev/null 2>&1;",
			    login, home);
			sprintf(buf+strlen(buf), 
			    "/usr/bin/chgrp %s %s/.Xdefaults >/dev/null 2>&1",
			    group, home);
			system (buf);
		}
		else {
			sprintf(buf1, GetGizmoText(string_XdefMoveFailed));
			ErrorNotice(buf1, 0);
		}

	}

}


/* This routine updates the user's
 * .profile file with the new LANG
 * value.
 */
void
ChangeProfile()
{

	char       line[BUFSIZ], buf[BUFSIZ];
	char       buf1[BUFSIZ], lang_buf[BUFSIZ];
	FILE       *fp, *fp1;
	char       *lineptr;
	Boolean    Open1_OK = FALSE;
	Boolean    Open2_OK = FALSE;
	Boolean    found = FALSE, found_lang = FALSE;
	Boolean    wrote_lang = FALSE;

	sprintf(buf, "%s/%s", home, ".profile");
	/* Open the user's .profile file and /tmp/.profile file */
	if (fp = fopen(buf,"r")) {
		Open1_OK = TRUE;

		/* Do a quick search for LANG in user's .profile file */
		sprintf(lang_buf, "/usr/bin/grep %s %s | \
                            grep -v grep >/dev/null 2>&1", LANG,
		    buf);
		if (system(lang_buf) == 0)
			found_lang = TRUE;

		if (fp1 = fopen(TMP_PROFILE,"w")) {
			Open2_OK = TRUE;

			/* For each line in the user's .profile file */
			while (fgets(buf, BUFSIZ, fp)) {
				lineptr = buf;
				if (found_lang == TRUE) {

					if (strncmp (LANG, lineptr, 5) == 0) {
						sprintf(buf1, "%s%s export LANG\t%s\n", LANG, 
						    userLocale, DONT_EDIT);
						fputs(buf1, fp1);
					}
					else
						fputs(lineptr,fp1);
				}
				else { /* found_lang is false */

					if (*lineptr == '#' || *lineptr == '\n')
						fputs (lineptr, fp1) ;
					else if (wrote_lang == FALSE) {
						sprintf(buf1, "%s%s export LANG\t%s\n", LANG,
						    userLocale, DONT_EDIT);
						fputs(buf1, fp1);
						fputs (lineptr, fp1) ;
						wrote_lang = TRUE;
					}
					else 
						fputs (lineptr, fp1) ;

				} /* if found_lang */

			} /*  While loop */

			fclose(fp1);

		} /* If open fp1 */

		fclose(fp);

	} /* If open fp */

	if (Open1_OK == TRUE && Open2_OK == TRUE) {
		sprintf(buf, "%s %s/.profile >/dev/null 2>&1", 
		    MOVE_PROFILE, home);
		if ((exit_code = system (buf)) == 0) {
			sprintf(buf, "/usr/bin/chown %s %s/.profile >/dev/null 2>&1;",
			    login, home);
			sprintf(buf+strlen(buf), 
			    "/usr/bin/chgrp %s %s/.profile >/dev/null 2>&1",
			    group, home);
			system (buf);
		}
		else {
			sprintf(buf1, GetGizmoText(string_profMoveFailed));
			ErrorNotice(buf1, 0);
		}

	}

}


/* This routine updates the user's
 * .login file with the new LANG
 * value.
 */
void
ChangeLogin()
{

	char       line[BUFSIZ], buf[BUFSIZ];
	char       buf1[BUFSIZ], lang_buf[BUFSIZ];
	FILE       *fp, *fp1;
	char       *lineptr;
	Boolean    Open1_OK = FALSE;
	Boolean    Open2_OK = FALSE;
	Boolean    found = FALSE, found_lang = FALSE;
	Boolean    wrote_lang = FALSE;

	sprintf(buf, "%s/%s", home, ".login");
	/* Open the user's .login file and /tmp/.login file */
	if (fp = fopen(buf,"r")) {
		Open1_OK = TRUE;

		/* Do a quick search for LANG in user's .login file */
		sprintf(lang_buf, "/usr/bin/grep \"setenv LANG\" %s | \
                               grep -v grep > /dev/null 2>&1",
		    buf);
		if (system(lang_buf) == 0)
			found_lang = TRUE;

		if (fp1 = fopen(TMP_LOGIN,"w")) {
			Open2_OK = TRUE;

			/* For each line in the user's .login file */
			while (fgets(buf, BUFSIZ, fp)) {
				lineptr = buf;
				if (found_lang == TRUE) {

					if (strncmp (LANG2, lineptr, 11) == 0) {
						sprintf(buf1, "%s %s\t%s\n", 
						LANG2, userLocale, DONT_EDIT);
						fputs(buf1, fp1);
					}
					else
						fputs(lineptr,fp1);
				}
				else { /* found_lang is false */

					if (*lineptr == '#' || *lineptr == '\n')
						fputs (lineptr, fp1) ;
					else if (wrote_lang == FALSE) {
						sprintf(buf1, "%s %s\t%s\n", 
						LANG2, userLocale, DONT_EDIT);
						fputs(buf1, fp1);
						fputs (lineptr, fp1) ;
						wrote_lang = TRUE;
					}
					else 
						fputs (lineptr, fp1) ;

				} /* if found_lang */

			} /*  While loop */

			fclose(fp1);

		} /* If open fp1 */

		fclose(fp);

	} /* If open fp */

	if (Open1_OK == TRUE && Open2_OK == TRUE) {
		sprintf(buf, "%s %s/.login >/dev/null 2>&1", 
		    MOVE_LOGIN, home);
		if ((exit_code = system (buf)) == 0) {
			sprintf(buf, "/usr/bin/chown %s %s/.login >/dev/null 2>&1;",
			    login, home);
			sprintf(buf+strlen(buf), 
			    "/usr/bin/chgrp %s %s/.login >/dev/null 2>&1",
			    group, home);
			system (buf);
		}
		else {
			sprintf(buf1, GetGizmoText(string_loginMoveFailed));
			ErrorNotice(buf1, 0);
		}

	}

}

void
CheckXdefaults(char *userhome)
{

	static char       line[BUFSIZ], buf[BUFSIZ];
	FILE       *fp;
	char       *lineptr, locale[BUFSIZ];
	int        i;
	Boolean    found = FALSE;

	sprintf(buf, "%s/%s", userhome, ".Xdefaults");
	if (fp = fopen(buf,"r"))
	{
		while (fgets(line, BUFSIZ, fp))
		{
			lineptr = line;
			if (strncmp (XNLLANGUAGE, lineptr, 13) == 0)
			{
				while (*lineptr++ != ':');
				while (*lineptr == '\t' || *lineptr == ' ')lineptr++;
				i = 0;
				while (*lineptr != '\n')
					locale[i++]=*lineptr++;
				locale[i++] = '\0';
				GetLocaleName(locale);
				found = TRUE;
				break;
			} /* If XNLLANGUAGE is found */

		}/* While fgets */

		fclose(fp);

	} /* If open */

	if (found == FALSE)
	{
		userLocaleName = defaultLocaleName;
		userLocale = defaultLocale;
	}

	curLocaleName = userLocaleName;
	curLocale = userLocale;
}


void
CheckProfile(char *userhome)
{

	char       line[BUFSIZ], buf[BUFSIZ];
	FILE       *fp;
	char       *lineptr, locale[BUFSIZ];
	int        i;
	Boolean    found = FALSE;

	sprintf(buf, "%s/%s", userhome, ".profile");
	if (fp = fopen(buf,"r")) {
		while (fgets(line, BUFSIZ, fp)) {
			lineptr = line;
			if (strncmp (LANG, lineptr, 5) == 0) {
				while (*lineptr != '=' && *lineptr != '\n')
					lineptr++;
				if (*lineptr == '=') {
					lineptr++;
					while (*lineptr == ' ')lineptr++;
					i = 0;
					while (*lineptr != ' ' && *lineptr != '\n')
						locale[i++]=*lineptr++;
					locale[i++] = '\0';
					GetLocaleName(locale);
					found = TRUE;
					break;
				}
			} /* If LANG is found */

		}/* While fgets */

		fclose(fp);

	} /* If open */

	if (found == FALSE) {
		userLocaleName = defaultLocaleName;
		userLocale = defaultLocale;
	}

	curLocaleName = userLocaleName;
	curLocale = userLocale;
}
