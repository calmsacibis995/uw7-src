#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/local.c	1.9"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	local.c  
 */

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/MenuGizmo.h>

#include "nfs.h"
#include "text.h"
#include "sharetab.h"
#include "local.h"

extern NFSWindow *MainWindow;

extern dfstabEntryType
Get_dfstab_entry(FILE *fp, dfstab **dfsp, char *entry)
{
    static char     buf[BUFSIZ];
    char           *bp, *token;
    char 	   *errorText;
    Boolean         autoShare;
    struct share   *sharep;

    if (fgets(buf, BUFSIZ, fp) == NULL)
    {
	if (errno && errno != ENOENT)
	{
	    errorText = GetGizmoText(TXT_dfstabReadError);
	    (void)fprintf(stderr, "%s %d\n", errorText, errno);
	}
	*dfsp = (dfstab *)NULL;
	*entry = EOS;
	return NoMore;
    }
    strcpy(entry, buf);

    /* FIX: the code below for parsing DFSTAB entries will only   */
    /* recognize entries in the form that this program writes them */
    /* in , i.e.                                                   */
    /* [#]SHARECMD -o options -d "<text>" <pathname> <resource>    */
    /* if a user edits the file and adds a command with the        */
    /* options in a different order, for ex. we will not recognize */
    /* it here but will probably find it in SHARETAB and get it    */
    /* from there.                                                 */

    /* check for comment character in 1st column */
    bp = buf;
    if (*bp == '#')
    {
	bp++;
	autoShare = False;	/* line is commented out */
    }
    else
	autoShare = True;

    if (strncmp(bp, SHARECMD, SHARECMDLEN) != 0)
	return Mystery;		/* line is not a share command */
    bp += SHARECMDLEN;

    *dfsp = (dfstab *)MALLOC(sizeof(dfstab));
    (*dfsp)-> sharep = sharep =
	(struct share *)CALLOC(1, sizeof(struct share));
    (*dfsp)-> autoShare   = autoShare;

    token = strtok(bp, WHITESPACE);
    if (strcmp(token, "-o") == 0)
    {
	token = strtok(NULL, WHITESPACE);
	sharep-> sh_opts     = STRDUP(token);
	token = strtok(NULL, QUOTEWHITE);
    }
    else
	sharep-> sh_opts = STRDUP("");

    if (strcmp(token, "-d") == 0)
    {
	token = strtok(NULL, QUOTE);
	sharep-> sh_descr = STRDUP(token);

	token = strtok(NULL, WHITESPACE);
	sharep-> sh_path = STRDUP(token);
    }
    else
    {
	sharep-> sh_descr = STRDUP("");
	sharep-> sh_path = STRDUP(token);
    }

    token = strtok(NULL, WHITESPACE);
    sharep-> sh_res = STRDUP(token);
    if (sharep-> sh_res == NULL)
    {
	DEBUG0("incomplete dfstab entry discarded\n");
	free_dfstab(*dfsp);
	return Mystery;
    }
    sharep-> sh_fstype = STRDUP("nfs");
    return NFS;
}


extern void
writedfs(dfstab * dfsp)
{
    extern NFSWindow *nfsw;
    FILE * fp;
    char buf[BUFSIZ], *bp = buf, *dasho;
    struct share  *sharep = dfsp-> sharep;

    if (dfsp-> autoShare == False)
	*bp++ = '#';

    dasho = (*sharep-> sh_opts) ? "-o" : "";

    sprintf(bp, "%s %s %s %s %s\n", SHARECMD, dasho, sharep-> sh_opts,
	    sharep-> sh_path, sharep-> sh_res); 

    if ((fp = fopen(DFSTAB, "a+")) == NULL)
    {
	if (nfsw-> localPropertiesPrompt)
	    SetMessage(nfsw-> localPropertiesPrompt, TXT_dfstabOpenError, Notice);
	else
	    SetMessage(MainWindow, TXT_dfstabOpenError, Notice);
	return;
    }    
    fputs(buf, fp);
    fclose(fp);
    return;
}
