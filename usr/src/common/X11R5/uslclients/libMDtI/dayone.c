#pragma ident	"@(#)libMDtI:dayone.c	1.21.1.6"

#include <locale.h>
#include <unistd.h>

#include <DesktopP.h>
#include <Dt/DtMsg.h>

#include "DtStubI.h"
#include "dayone.h"

typedef struct _DayOneMap {
	const char * const	id;
	const char * const	name;
} DayOneMap;

static DayOneMap     dayone_strings[] = {
        {  "1", TXT_HARDWARE_SETUP  },
        {  "2", TXT_ADMIN_TOOLS  },
        {  "3", TXT_APP_INSTALLER },
        {  "4", TXT_APPLICATIONS },
        {  "5", TXT_BACKUP_RESTORE },
        {  "6", TXT_CALCULATOR },
        {  "7", TXT_CARTRIDGE_TAPE },
        {  "8", TXT_CLOCK },
        {  "9", TXT_COLOR },
        { "10", TXT_DEBUG },
        { "11", TXT_DESKTOP },
        { "12", TXT_DIALUP_SETUP },
        { "13", TXT_DISK_A },
        { "14", TXT_DISK_B },
        { "15", TXT_DISKS_ETC },
        { "16", TXT_EXTRA_ADMIN },
        { "17", TXT_FILE_SHARING },
        { "18", TXT_FOLDER_MAP },
        { "19", TXT_FONTS },
        { "20", TXT_GAMES },
        { "21", TXT_HELP_DESK },
        { "22", TXT_ICON_EDITOR },
        { "23", TXT_ICON_SETUP },
        { "24", TXT_INTERNET_SETUP },
        { "25", TXT_LOCALE },
        { "26", TXT_MAIL },
        { "27", TXT_MAIL_SETUP },
        { "28", TXT_MAILBOX },
        { "29", TXT_MOUSE },
        { "30", TXT_MSG_MONITOR },
        { "31", TXT_NETWARE },
        { "32", TXT_NETWORKING },
        { "33", TXT_PASSWORD },
        { "34", TXT_PREFERENCES },
        { "35", TXT_PRINTER_SETUP },
        { "36", TXT_PUZZLE },
        { "37", TXT_REMOTE_ACCESS },
        { "38", TXT_SCREENLOCK },
        { "39", TXT_SHUTDOWN },
        { "40", TXT_STARTUP_ITEMS },
        { "41", TXT_SYSTEM_MONITOR },
        { "42", TXT_SYSTEM_STATUS },
        { "43", TXT_SYSTEM_TUNER },
        { "44", TXT_TASK_SCHEDULER },
        { "45", TXT_TERMINAL },
        { "46", TXT_TEXT_EDITOR },
        { "47", TXT_UUCP_INBOX },
        { "48", TXT_USER_SETUP },
        { "49", TXT_WALLPAPER },
        { "50", TXT_WALLPAPER_INSTALLER },
        { "51", TXT_WASTEBASKET },
        { "52", TXT_XTETRIS },
        { "53", TXT_CDROM },
        { "54", TXT_DOS },
        { "55", TXT_ONLINE_DOCS },
        { "56", TXT_REMOTE_APPS },
        { "57", TXT_WIN_SETUP },
        { "58", TXT_APP_SHARING },
        { "59", TXT_INSTALL_SERVER },
        { "60", TXT_MHS_SETUP },
        { "61", TXT_PROCESSOR_SETUP },
        { "62", TXT_DISPLAY_SETUP },
        { "63", TXT_NETWARE_ACCESS },
        { "64", TXT_NETWARE_SETUP },
        { "65", TXT_NETWARE_STATUS },
        { "66", TXT_CDROM1 },
        { "67", TXT_WIN },
        { "68", TXT_WINDOW },
        { "69", TXT_INET_BROWSER },
        { "70", TXT_INSTALL_BROWSER },
        { "71", TXT_NONE },
        { "72", TXT_REMOTE_LOGIN },
        { "73", TXT_GET_INET_BROWSER },
        { "74", TXT_TAPE_2 },
        { "75", TXT_TAPE_3 },
        { "76", TXT_TAPE_4 },
        { "77", TXT_TAPE_5 },
        { "78", TXT_CD_1 },
        { "79", TXT_CD_2 },
        { "80", TXT_WELCOME },
        { "81", TXT_MERGE_SETUP },
        { "82", TXT_DS_INSTALL },
        { "83", TXT_DS_REPAIR },
        { "84", TXT_NETWARE_SETTINGS },
        { "85", TXT_NWS_LICENSING },
        { "86", TXT_NWS_STATUS },
        { "87", TXT_NETWARE_CLIENT_DISKS },
        { "88", TXT_NWS_VOLUME_SETUP }
};

/****************************procedure*header*****************************
 *  This routine reads a desktop user's dayone file and
 *  returns the user's dayone locale.
 *  Caller routine should check for NULL. 
 *  Returns malloc'ed space. The caller routine 
 *  is responsible for freeing the returned string.
 *  Defaults to C locale.
 *
 *  Define it as a static until someone uses it!
 */
static char *
Dm__DayOneLocale(const char * login)
{
    static char *	last_locale = NULL;
    static char *	last_login = NULL;
    static int		longest_login_len = 0;
    static int		path_len;
    int			login_len;
    FILE *		fp;
    extern char *	GetXWINHome();

    /* Check login */
    if (login == NULL || *login == 0)
	return(NULL);

    if (last_login == NULL)			/* first time */
    {
#ifdef TEST_DAYONE
	last_login = strdup("./");
#else
	last_login = strdup(GetXWINHome("desktop/LoginMgr/DayOne/"));
#endif
	path_len = strlen(last_login);

    } else
    {
	/* If login is same as last call, return saved locale */
	if (strcmp(last_login + path_len, login) == 0)
	    return(strdup(last_locale));
    }

    /* Alloc more space for 'login' if needed and copy it to end of path */
    if ((login_len = strlen(login)) > longest_login_len)
    {
	last_login = realloc(last_login, path_len + login_len + 1);
	longest_login_len = login_len;
    }
    strcpy(last_login + path_len, login);

    /* Open and read the user's dayone file */
    if ((fp = fopen(last_login, "r")) != NULL)
    {
	char locale[BUFSIZ];

	if (fscanf(fp, "%s", locale))
	{
	    if (last_locale)
		free(last_locale);
	    last_locale = strdup(locale);
	}
	fclose(fp);
    }
    if (last_locale == NULL)
	last_locale = strdup("C");
    return(strdup(last_locale));
}

/*
 * This routine retrieves the localized text associated
 * with the given label. Note that we have to create the
 * local copy because the msg string is different from others.
 */
static char *
Dm__CallGetTxt(const char * const id, const char * const dft)
{
	static char msgid[7 + 10] = DAYONE_FILENAME;

	strcpy(msgid + 7, id);
	return((char *)gettxt(msgid, dft));
}

/*
 *  This routine maps the dayone strings to the `name' arg
 *  and returns a localized string.
 *  Caller routine should check for NULL.
 *  This routines returns either a malloc'ed space or the original
 *  day1_string.
 *
 *  The caller routine is responsible for checking and freeing accordingly.
 *  gettxt will return the default message if no match.
 */
static char *
Dm__DayOneGettxt(const char * day1_string)
{
	register int	i;
	char *		loc_string = NULL;
	char		numCD[BUFSIZ];
	char		bufCD[BUFSIZ];

		/* Assumed `i' */
#define ID	dayone_strings[i].id
#define DFT	dayone_strings[i].name

	/* to support an unlimited number of CD-ROMs: if we have
	 * a string of the form CD-ROM_# (eg CD-ROM_3) we parse out
	 * the string CD-ROM for translation and then re-append the 
	 * number.
	 */
	if (strncmp(day1_string, "CD-ROM_", 7)==0 && strlen(day1_string) > 7) {
		sscanf(day1_string+7, "%s", numCD);
		/* Note: day1_locale won't be C.  See Dm_DayOneName() */
		sprintf(bufCD, "%s_%s", Dm__CallGetTxt("53", "CD-ROM"), numCD);
		loc_string = strdup(bufCD);
	}
	else for (i = 0; i < XtNumber(dayone_strings); i++)
	{
		if (strcmp(DFT, day1_string) == 0)
		{
				/* Note that day1_locale won't be C.
				 * See Dm_DayOneName()... */
			loc_string = strdup(Dm__CallGetTxt(ID, DFT));
			break;
		}
	}

#undef ID
#undef DFT

/* Warning: don't want this optimization if this routine becomes an extern */

	return(loc_string ? loc_string : (char *)day1_string);
}

/*
 *  This routine validates and parses the path argument,
 *  calls the Dm__DayOneLocale and Dm__DayOneGettxt routines
 *  and returns a localized string.
 *  Caller routine should check for NULL.
 *  This routines could return NULL, or a malloc'ed space (via strdup). 
 *  The caller routine should check accordingly.
 */
char *
Dm_DayOneName(const char * path, const char * login)
{

	char *	day1_locale;
	char *	loc_string;
	char *	day1_string;
	char *	delim;
	char *	save_str;
	char *	cur_locale;
	char	buf[BUFSIZ];

		/* Return NULL if there is no path or no login or
		 * can't get user's dayone locale... */
	if (path == NULL  || *path == 0  ||
	    login == NULL || *login == 0 ||
	    (day1_locale = Dm__DayOneLocale(login)) == NULL)
		return(NULL);

		/* Check if day1_locale is "C" */
	if (strcmp(day1_locale, "C") == 0)
	{
		free(day1_locale);
		return(strdup(path));
	}

#define CATE	LC_MESSAGES	/* vs LC_ALL originally */

	cur_locale = strdup(setlocale(CATE, NULL)); /* Save current locale */
	if (setlocale(CATE, day1_locale) == NULL)   /* Set to day 1 locale */
	{					    /* If failed then...   */
		free(day1_locale);
		(void)setlocale(CATE, cur_locale);  /* Back to cur locale  */
		return(strdup(path));
	}
	free(day1_locale);

#define SLASH		"/"
		/* Check for intermediate directories */
	save_str = day1_string = strdup(path);
	buf[0] = 0;
	while ((delim = strchr(day1_string, *SLASH)))
	{
		*delim = 0;

			/* *day1_string can be 0 if `path' contains "//". */
		if (*day1_string)
		{
			loc_string = Dm__DayOneGettxt(day1_string);
			strcat(buf, loc_string);
			if (loc_string != day1_string)
				free(loc_string);
		}
		strcat(buf, SLASH);

		day1_string = delim + 1;
	}

#undef SLASH

	if (*day1_string)
	{
		loc_string = Dm__DayOneGettxt(day1_string);
		strcat(buf, loc_string);
		if (loc_string != day1_string)
			free(loc_string);
	}

	(void)setlocale(CATE, cur_locale); /* Back to the cur_locale */

	free(cur_locale);
	free(save_str);

		/* packager will have to call free(), dtm code
		 * will have to be examined for memory problem... */
	return(strdup(buf));

#undef CATE
}

#ifdef TEST_DAYONE
#include <stdio.h>
#include "dayone.h"

#define LOGIN	"foo"
#define MY_FREE(B)	if (B) free(B)

char *
Dt__strndup(str, len)
char *str;
int len;
{
        char *p;

        if (p = malloc(len + 1)) {
                memcpy(p, str, len);
                p[len] = '\0';
	}

	return(p);
}

main()
{
	char	path[2048];
	char *	tmp;

	printf("1: %s vs %s\n\n",
		TXT_ADMIN_TOOLS, tmp=Dm_DayOneName(TXT_ADMIN_TOOLS, LOGIN));
	MY_FREE(tmp);

	sprintf(path, "%s/%s", TXT_ADMIN_TOOLS, TXT_APP_INSTALLER);
	printf("2: %s vs %s\n\n", path, tmp=Dm_DayOneName(path, LOGIN));
	MY_FREE(tmp);

	sprintf(path, "/%s/%s", TXT_ADMIN_TOOLS, TXT_APP_INSTALLER);
	printf("3: %s vs %s\n\n", path, tmp=Dm_DayOneName(path, LOGIN));
	MY_FREE(tmp);

	sprintf(path, "///%s/%s", TXT_ADMIN_TOOLS, TXT_APP_INSTALLER);
	printf("4: %s vs %s\n\n", path, tmp=Dm_DayOneName(path, LOGIN));
	MY_FREE(tmp);

	sprintf(path, "///%s/%s/", TXT_ADMIN_TOOLS, TXT_APP_INSTALLER);
	printf("5: %s vs %s\n\n", path, tmp=Dm_DayOneName(path, LOGIN));
	MY_FREE(tmp);
}
#endif /* TEST_DAYONE */
