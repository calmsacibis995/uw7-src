/*
 * File hw_con.c
 * Information handler for Console
 *
 * @(#) hw_con.c 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <strings.h>

#include "hw_con.h"
#include "hw_util.h"

const char	* const callme_con[] =
{
    "con",
    "console",
    "screen",
    "keyboard",
    "kbd",
    "sound",
    "audio",
    NULL
};

const char	short_help_con[] = "Console configuration";
static const char	grafinfoDir[] = "/usr/lib/grafinfo";

static void displayCard(FILE *out);
static int displayOneCard(FILE *out, const char *mfgr, const char *model,
				const char *code, const char *mode);
static void displayScreen(FILE *out);
static int displayOneScreen(FILE *out, const char *file);
static void displayMouse(FILE *out);
static void displayOneMouse(FILE *out, const char *key);
static const char *getMouseInfo(const char *name);
static char *readCfgLine(FILE *fd);
static void displaySound(FILE *out);

int
have_con(void)
{
    return 1;	/* I HOPE we always have at least one */
}

void
report_con(FILE *out)
{
    report_when(out, "console");
    displayCard(out);
    displayScreen(out);
    displayMouse(out);
    displaySound(out);
}

static
void displayCard(FILE *out)
{
    static const char	grafdevName[] = "grafdev";
    static const char	msg[] = "    Graphics card configuration\n";
    FILE	*grafdevFd;
    char	buf[256];
    int		bufnum = 0;
    int		first = 1;
    char	fileName[256];
    char	lastMfgr[128];
    char	lastModel[128];
    char	lastCode[128];
    char	lastMode[128];
    int		cnt;

    sprintf(fileName, "%s/%s", grafinfoDir, grafdevName);
    if (!(grafdevFd = fopen(fileName, "r")))
    {
	debug_print("cannot open: %s: %s", fileName, strerror(errno));
	return;
    }

    if (verbose)
    {
	fprintf(out, msg);
	fprintf(out, "\tFile:     %s\n\n", fileName);
	first = 0;
    }

    lastMfgr[0] = '\0';
    lastModel[0] = '\0';
    lastCode[0] = '\0';
    lastMode[0] = '\0';
    cnt = 0;
    while (fgets(buf, sizeof(buf), grafdevFd))
    {
	int	n;
	char	*tp;
	char	*dev;
	char	*mfgr;		/* VENDOR */
	char	*model;		/* MODEL */
	char	*code;		/* CLASS */
	char	*mode;		/* MODE */

	n = strlen(buf);
	while ((n > 0) && isspace(buf[n-1]))
	    buf[--n] = '\0';

	for (tp = buf; isspace(*tp); ++tp)
	    ;

	if (!*tp || (*tp == '#'))
	    continue;			/* Ignore blank and comment lines */

	if (strncmp(tp, "/dev/", 5) == 0)
	    tp += 5;

	if (!(dev = strtok(tp, ":")) ||
	    !(mfgr = strtok(NULL, ".")) ||
	    !(model = strtok(NULL, ".")) ||
	    !(code = strtok(NULL, ".")) ||
	    !(mode = strtok(NULL, ".")))
		continue;

	if (first)
	{
	    fprintf(out, msg);
	    first = 0;
	}

	/*
	 * We display on some sobsequent device
	 */

	if (lastMfgr[0])
	{
	    if ((strcmp(lastMfgr, mfgr) == 0) &&
		(strcmp(lastModel, model) == 0) &&
		(strcmp(lastCode, code) == 0) &&
		(strcmp(lastMode, mode) == 0))
	    {
		/*
		 * The normal case is that all of the console
		 * devices use the same card and configuration. 
		 * Fold the line at a point where it makes
		 * two nearly even lines.
		 */

		if (cnt > 50)
		{
		    fprintf(out, ",\n\t%s", dev);
		    cnt = strlen(dev);
		}
		else
		{
		    fprintf(out, ", %s", dev);
		    cnt += strlen(dev) + 2;
		}
	    }
	    else
	    {
		fprintf(out, "\n");
		if (displayOneCard(out, lastMfgr,lastModel,lastCode,lastMode))
		{
		    if (verbose)
			fprintf(out, "\t    File:      %s/%s/%s.xgi\n",
					    grafinfoDir, lastMfgr, lastModel);

		    fprintf(out, "\t    Info:      %s %s %s %s\n",
				    lastMfgr, lastModel, lastCode, lastMode);
		}

		fprintf(out, "\n\t%s", dev);
		cnt = strlen(dev);
		strcpy(lastMfgr, mfgr);
		strcpy(lastModel, model);
		strcpy(lastCode, code);
		strcpy(lastMode, mode);
	    }
	}
	else
	{
	    fprintf(out, "\t%s", dev);
	    cnt = strlen(dev);

	    strcpy(lastMfgr, mfgr);
	    strcpy(lastModel, model);
	    strcpy(lastCode, code);
	    strcpy(lastMode, mode);
	}
    }

    fclose(grafdevFd);

    /*
     * We may have one that was not printed
     */

    if (lastMfgr[0])
    {
	fprintf(out, "\n");

	if (displayOneCard(out,lastMfgr, lastModel, lastCode, lastMode))
	{
	    if (verbose)
		fprintf(out, "\t    File:      %s/%s/%s.xgi\n",
					grafinfoDir, lastMfgr, lastModel);

	    fprintf(out, "\t    Info:      %s %s %s %s\n",
				    lastMfgr, lastModel, lastCode, lastMode);
	}
    }

    if (!first)
	fprintf(out, "\n");
}

static int
displayOneCard(FILE *out, const char *mfgr, const char *model,
					const char *code, const char *mode)
{
    FILE	*Fd;
    char	buf[256];
    char	fileName[256];
    int		work;
    int		state;

    sprintf(fileName, "%s/%s/%s.xgi", grafinfoDir, mfgr, model);
    if (!(Fd = fopen(fileName, "r")))
    {
	debug_print("cannot open: %s: %s", fileName, strerror(errno));
	return errno;
    }

    /*
     * State:
     *	    0	Looking for VENDOR
     *	    1	Looking for MODEL
     *	    2	Looking for CLASS
     *	    3	Looking for MODE
     *	    4	Looking for DATA
     *	    5	Expecting {
     *	    6	Displaying data
     */

    work = 0;
    state = 0;
    while (fgets(buf, sizeof(buf), Fd))
    {
	static char	VendorBuf[128];
	static char	ModelBuf[128];
	static char	ClassBuf[128];
	static char	ModeBuf[128];
	static struct
	{
	    const char	*key;
	    const char	*value;
	    char	*buf;
	    char	*descr;
	} states[] =
	{
	    { "VENDOR", NULL, VendorBuf, "Vendor" },
	    { "MODEL", NULL, ModelBuf, "Model" },
	    { "CLASS", NULL, ClassBuf, "Class" },
	    { "MODE", NULL, ModeBuf, "Mode" }
	};
	int	n;
	char	*tp;
	char	*ep;
	char	*xp;
	int	len;


	states[0].value = mfgr;
	states[1].value = model;
	states[2].value = code;
	states[3].value = mode;

	n = strlen(buf);
	while ((n > 0) && isspace(buf[n-1]))
	    buf[--n] = '\0';

	for (tp = buf; isspace(*tp); ++tp)
	    ;

	if (!*tp || (*tp == '#'))
	    continue;			/* Ignore blank and comment lines */

	/*
	 * In states 0-3 we attempt to find our driver and take
	 * a copy of the argument.
	 */

	for (n = 0; n < sizeof(states)/sizeof(*states); ++n)
	{
	    len = strlen(states[n].key);
	    if ((strncasecmp(tp, (char *)states[n].key, len) == 0) &&
		isspace(tp[len]))
	    {
		if (state > n)
		    state = n;

		if (state != n)
		    continue;

		for (tp += len; isspace(*tp); ++tp)
		    ;

		for (ep = tp; *ep && !isspace(*ep); ++ep)
		    ;
		if (!*ep)
		    continue;
		*ep = '\0';	/* End of value */

		if (strcasecmp(tp, (char *)states[n].value) != 0)
		    break;

		++state;

		for (tp = ep+1; isspace(*tp); ++tp)
		    ;
		if (*tp == '"')
		{
		    for (xp = ++tp; *xp != '"'; ++xp)
			;
		    if (*xp)
			*xp = '\0';
		}

		strcpy(states[n].buf, tp);
		break;
	    }
	}

	if (n < sizeof(states)/sizeof(*states))
	    continue;

	/*
	 * In state 4 we are looking for DATA
	 */

	if ((strcasecmp(tp, "DATA") == 0) && (state >= 4))
	{
	    state = 5;
	    continue;
	}

	/*
	 * In state 5 we are expecting the start of DATA
	 */

	if (state == 5)
	{
	    if (*tp == '{')
		state = 6;

	    continue;
	}

	/*
	 * State 6 we display the elements of the DATA structure
	 */

	if (state == 6)
	{
	    static const struct
	    {
		const char	*key;
		const char	*descr;
		int		verbose;
	    } params[] =
	    {
		{ "XDRIVER",	"X Driver",	0 },
		{ "VISUAL",	"Visual",	1 },
		{ "DEPTH",	"Depth",	1 },
		{ "PIXWIDTH",	"Width",	1 },
		{ "PIXHEIGHT",	"Height",	1 },
		{ "FREQUENCY",	"Frequency",	1 },
	    };
	    if (*tp == '}')
	    {
		state = 0;
		continue;
	    }

	    for (n = 0; n < sizeof(params)/sizeof(*params); ++n)
	    {
		len = strlen(params[n].key);

		if ((strncasecmp(tp, (char *)params[n].key, len) == 0) &&
		    (isspace(tp[len]) || (tp[len] == '=')))
		{
		    for (ep = &tp[len]; *ep && (*ep != ';'); ++ep)
			;
		    if (*ep)
			*ep = '\0';

		    for (ep = &tp[len]; isspace(*ep); ++ep)
			;

		    if (*ep != '=')
			continue;

		    do
			++ep;
		    while(isspace(*ep));

		    if (*ep == '"')
		    {
			for (xp = ++ep; *xp != '"'; ++xp)
			    ;
			if (xp == ep)
			    continue;

			if (*xp)
			    *xp = '\0';
		    }

		    if (verbose || params[n].verbose)
		    {
			if (!work)
			{
			    int		j;

			    work = 1;

			    if (verbose)
				fprintf(out, "\t    File:      %s\n", fileName);

			    for (j = 0; j < sizeof(states)/sizeof(*states); ++j)
				fprintf(out, "\t    %s:%s%s\n",
					    states[j].descr,
					    spaces(10-strlen(states[j].descr)),
					    states[j].buf);
			}

			fprintf(out, "\t    %s:%s%s\n",
					    params[n].descr,
					    spaces(10-strlen(params[n].descr)),
					    ep);
		    }
		    break;
		}
	    }
	}
    }

    fclose(Fd);
    return work ? 0 : ENOENT;
}

static
void displayScreen(FILE *out)
{
    static const char	grafmonName[] = "grafmon";
    static const char	monInfoDir[] = "moninfo";
    static const char	msg[] = "    Graphics display configuration\n";
    FILE	*grafmonFd;
    char	buf[256];
    int		first = 1;
    char	cfgFileName[256];
    char	monFileName[256];
#ifdef NEED_DEV_FILE
    char	devFileName[256];
#endif

    sprintf(cfgFileName, "%s/%s", grafinfoDir, grafmonName);
    if (!(grafmonFd = fopen(cfgFileName, "r")))
    {
	debug_print("cannot open: %s: %s", cfgFileName, strerror(errno));
	return;
    }

    if (verbose)
    {
	fprintf(out, msg);
	fprintf(out, "\tFile:          %s\n", cfgFileName);
	first = 0;
    }

    while (fgets(buf, sizeof(buf), grafmonFd))
    {
	int	n;
	char	*tp;
	char	*devMfgr;
	char	*devModel;
	char	*scrnMfgr;
	char	*scrnModel;

	n = strlen(buf);
	while ((n > 0) && isspace(buf[n-1]))
	    buf[--n] = '\0';

	for (tp = buf; isspace(*tp); ++tp)
	    ;

	if (!*tp || (*tp == '#'))
	    continue;			/* Ignore blank and comment lines */

	if (verbose)
	    fprintf(out, "\tInfo:          %s\n", tp);

	if (!(devMfgr = strtok(tp, ".")) ||
	    !(devModel = strtok(NULL, ":")) ||
	    !(scrnMfgr = strtok(NULL, ".")) ||
	    !(scrnModel = strtok(NULL, ".")))
		continue;

	sprintf(monFileName, "%s/%s/%s/%s.mon",
				grafinfoDir, monInfoDir, scrnMfgr, scrnModel);
	debug_print("graphmon: %s", monFileName);
#ifdef NEED_DEV_FILE
	sprintf(devFileName, "%s/%s/%s.xgi",
				grafinfoDir, devMfgr, devModel);
	debug_print("graphdev: %s", devFileName);
#endif

	if (first)
	{
	    fprintf(out, msg);
	    first = 0;
	}

	if (displayOneScreen(out, monFileName))
	    fprintf(out, "\t%s %s %s %s\n",
				devMfgr,
				devModel,
				scrnMfgr,
				scrnModel);
    }

    fclose(grafmonFd);

    if (!first)
	fprintf(out, "\n");
}

static int
displayOneScreen(FILE *out, const char *file)
{
    FILE	*Fd;
    char	buf[256];
    int		work;
    static const struct
    {
	const char	*key;
	const char	*descr;
    } params[] =
    {
	{ "DESCRIPTION", "Description" },
	{ "MON_VENDOR", "Vendor" },
	{ "MON_MODEL", "Model" },
	{ "WIDTH", "Width" },
	{ "HEIGHT", "Height" },
	{ "Type", "Type" }
    };

    if (!(Fd = fopen(file, "r")))
    {
	debug_print("cannot open: %s: %s", file, strerror(errno));
	return errno;
    }

    work = 0;
    while (fgets(buf, sizeof(buf), Fd))
    {
	int	n;
	char	*tp;

	n = strlen(buf);
	while ((n > 0) && isspace(buf[n-1]))
	    buf[--n] = '\0';

	for (tp = buf; isspace(*tp); ++tp)
	    ;

	if (!*tp || (*tp == '#'))
	    continue;			/* Ignore blank and comment lines */

	for (n = 0; n < sizeof(params)/sizeof(*params); ++n)
	{
	    int	len = strlen(params[n].key);

	    if ((strncasecmp(tp, (char *)params[n].key, len) == 0) &&
		(isspace(tp[len]) || (tp[len] == '=')))
	    {
		char	*ep;

		for (ep = &tp[len]; *ep && (*ep != ';'); ++ep)
		    ;
		if (*ep)
		    *ep = '\0';

		for (ep = &tp[len]; isspace(*ep); ++ep)
		    ;

		if (*ep != '=')
		    continue;

		do
		    ++ep;
		while(isspace(*ep));

		if (*ep == '"')
		{
		    char	*xp;

		    for (xp = ++ep; *xp != '"'; ++xp)
			;
		    if (xp == ep)
			continue;

		    if (*xp)
			*xp = '\0';
		}

		 fprintf(out, "\t%s:%s%s\n",
					params[n].descr,
					spaces(14-strlen(params[n].descr)),
					ep);
		 work = 1;
		 break;
	    }
	}
    }

    fclose(Fd);
    return work ? 0 : ENOENT;
}

static const char	ttysName[] =	"/usr/lib/event/ttys";
static const char	devicesName[] =	"/usr/lib/event/devices";

static void
displayMouse(FILE *out)
{
    static const char	msg[] = "    Mouse configuration\n";
    FILE	*ttysFd;
    char	buf[256];
    int		first = 1;
    char	*prevKey = NULL;
    int		cnt = 0;

    if (!(ttysFd = fopen(ttysName, "r")))
    {
	debug_print("cannot open: %s: %s", ttysName, strerror(errno));
	return;
    }

    if (verbose)
    {
	fprintf(out, msg);
	fprintf(out, "\tFiles:\t%s\n\t\t%s\n", ttysName, devicesName);
	first = 0;
    }

    while (fgets(buf, sizeof(buf), ttysFd))
    {
	char			*key;
	int			n;

	n = strlen(buf);
	while ((n > 0) && isspace(buf[n-1]))
	    buf[--n] = '\0';

	if (!*buf || (*buf == '#'))
	    continue;			/* Ignore blank and comment lines */

	for (key = buf; *key && !isspace(*key); ++key)
	    ;

	if (!*key)
	    continue;

	*key = '\0';
	if (strncmp(buf, "ttyp", 4) == 0)
	    continue;			/* Ignore pseudo-ttys */

	do
	    ++key;
	while (isspace(*key));

	if (!*key)
	    continue;

	if (first)
	{
	    fprintf(out, msg);
	    first = 0;
	}

	if (prevKey && (strcmp(prevKey, key) == 0))
	{
	    if (cnt > 50)
	    {
		fprintf(out, ",\n\t%s", buf);
		cnt = strlen(buf);
	    }
	    else
	    {
		fprintf(out, ", %s", buf);
		cnt += strlen(buf) + 2;
	    }
	}
	else
	{
	    if (prevKey)
	    {
		displayOneMouse(out, prevKey);
		free(prevKey);
	    }

	    fprintf(out, "\n\t%s", buf);
	    cnt = strlen(buf);
	    prevKey = strdup(key);
	}
    }

    fclose(ttysFd);

    if (prevKey)
    {
	if (first)
	    fprintf(out, msg);

	displayOneMouse(out, prevKey);
	free(prevKey);
    }

    if (!first)
	fprintf(out, "\n");
}

static void
displayOneMouse(FILE *out, const char *key)
{
    const char		*cfg;
    const char		*tp;
    int			n;

    fprintf(out, "\n");

    if (!(cfg = getMouseInfo(key)))
    {
	fprintf(out, "\t    Unknown mouse type: %s\n", key);
	return;
    }

    debug_print("Mouse: %s", cfg);

    /*
     * Display the driver name
     */

    tp = cfg;
    for (n = 0; *tp && (n < 3); ++n)
    {
	while(*tp && !isspace(*tp))
	    ++tp;		/* Find end of field n-1 */
	while(isspace(*tp))
	    ++tp;		/* Find the beginning of field n */
    }

    fprintf(out, "\t    Driver:      ");
    if (*tp)
    {
	const char	*name = tp;

	for (n = 0; *tp && !isspace(*tp); ++tp, ++n)
	    ;
	fprintf(out, "%.*s\n", n, name);
    }
    else
    {
	fprintf(out, "%s\n", key);
	n = strlen(key);
    }

    /*
     * Display the description
     */

    if ((tp = strstr(cfg, "NAME=\"")) != NULL)
    {
	const char	*descr = &tp[6];

	if ((tp = strchr(descr, '\"')) != NULL)
	    fprintf(out, "\t    Description: %.*s\n", tp - descr, descr);
    }

    /*
     * Find the mouse port device
     */

    tp = cfg;
    while(*tp && !isspace(*tp))
	++tp;		/* Find end of first field */
    while(isspace(*tp))
	++tp;		/* Find the beginning of second field */

    if (*tp)
    {
	const char	*port = tp;

	for (n = 0; *tp && !isspace(*tp); ++tp, ++n)
	    ;
	fprintf(out, "\t    Port:        %.*s\n", n, port);
    }
}

static const char *
getMouseInfo(const char *name)
{
    FILE		*devicesFd;
    static const char	*line = NULL;
    int			len;

    if (!(len = strlen(name)))
	return NULL;

    if (line && (strncmp(line, name, len) == 0) && isspace(line[len]))
	return line;

    if (!(devicesFd = fopen(devicesName, "r")))
    {
	debug_print("cannot open: %s: %s", devicesName, strerror(errno));
	return NULL;
    }

    while ((line = readCfgLine(devicesFd)) != NULL)
	if ((strncmp(line, name, len) == 0) && isspace(line[len]))
	{
	    fclose(devicesFd);
	    return line;
	}

    fclose(devicesFd);
    return NULL;
}

/*
 * Read the next logical line from the file.
 */

static char *
readCfgLine(FILE *fd)
{
    static char	*buf = NULL;
    static int	bufLen = 0;
    int		len;
    char	*tp;


    if (!fd)
	return NULL;

    len = 0;
    for(;;)
    {
	if ((bufLen - len) < 200)
	{
	    if (!(tp = malloc(bufLen + 1024)))
		return NULL;

	    if (buf)
	    {
		strcpy(tp, buf);
		free(buf);
	    }

	    buf = tp;
	    bufLen += 1024;
	}

	if (!fgets(&buf[len], bufLen-len, fd))
	    break;

	len = strlen(buf);
	while ((len > 0) && ((buf[len-1] == '\n') || (buf[len-1] == '\r')))
	    buf[--len] = '\0';

	if (!len)
	    continue;

	if (buf[len-1] != '\\')
	    break;

	buf[--len] = '\0';
    }

    return buf;
}

static
void displaySound(FILE *out)
{
    static const char	msg[] = "    Sound configuration";
    static const char	SndDev[] = "/dev/sndstat";
    static const char	noneMsg[] = "\t    None\n";
    FILE		*SndFd;
    char		buf[256];
    int			state;
    int			first = 1;


    if (!(SndFd = fopen(SndDev, "r")))
    {
	debug_print("cannot open: %s: %s", SndDev, strerror(errno));
	return;
    }

    if (verbose)
    {
	fprintf(out, "%s\n\tFile:     %s\n", msg, SndDev);
	first = 0;
    }

    /*
     * State:
     *	0	Expecting driver title
     *	1	Skipping blank lines to next section
     *	2	Expecting first data
     *	3	Displaying section data
     */

    state = 0;
    while (fgets(buf, sizeof(buf), SndFd))
    {
	char	*tp;
	char	*xp;
	int	n;


	n = strlen(buf);
	while ((n > 0) && isspace(buf[n-1]))
	    buf[--n] = '\0';

	for (tp = buf; isspace(*tp); ++tp)
	    ;

	if (!*tp)
	{
	    switch (state)
	    {
		case 0:
		case 1:
		    break;

		case 2:
		    fprintf(out, noneMsg);
		    /* Fall thru */
		case 3:
		    state = 1;
	    }

	    continue;
	}

	if (first)
	{
	    fprintf(out, "%s\n\n", msg);
	    first = 0;
	}

	switch (state)
	{
	    case 0:
		/*
		 * Display the driver name, version and date
		 * Move to state 1
		 */

		state = 1;
		if (!(xp = strstr(tp, ":")) != NULL)
		{
		    fprintf(out, "\t%s\n", tp);
		    break;
		}

		*xp = '\0';
		fprintf(out, "\t%s:\n", tp);

		tp = xp+1;
		while (isspace(*tp))
		    ++tp;
		if (!*tp)
		    break;

		if (!(xp = strstr(tp, "(")) != NULL)
		{
		    fprintf(out, "\t    %s\n", tp);
		    break;
		}

		*xp = '\0';
		fprintf(out, "\t    %s\n", tp);

		tp = xp+1;
		while (isspace(*tp))
		    ++tp;

		n = strlen(tp);
		if ((n > 0) && (tp[n-1] == ')'))
		    tp[n-1] = '\0';

		if (!*tp)
		    break;

		fprintf(out, "\t    %s\n", tp);
		break;

	    case 1:
		/*
		 * Display the section head.  If there is
		 * in-line date go to state 3 else 2.
		 */

		fprintf(out, "\n\t%s\n", tp);
		if (tp[strlen(tp)-1] == ':')
		    state = 2;
		else
		    state = 3;
		break;

	    case 2:
	    case 3:
	    {
		u_long	base;
		int	irq;
		int	drq;

		if (*tp == '(')
		{
		    n = strlen(++tp);
		    if ((n > 0) && (tp[n-1] == ')'))
			tp[n-1] = '\0';
		}

		if (!verbose &&
		    *tp &&
		    ((xp = strstr(tp, " at ")) != NULL) &&
		    (sscanf(xp, " at %lx irq %d drq %d",
						&base, &irq, &drq) == 3) &&
		    ((base == 0) && (irq == 0) && (drq == 0)))
			*tp = '\0';

		/*
		 * If we have something iteresting to display,
		 * do so.  We are the solidly in state 3.
		 */

		if (*tp)
		{
		    state = 3;
		    fprintf(out, "\t    %s\n", tp);
		}
	    }
	}
    }

    if (state == 2)
	fprintf(out, noneMsg);

    if (state > 0)
	fprintf(out, "\n");

    fclose(SndFd);
}

