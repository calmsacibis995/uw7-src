#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/submit.c	1.6"
#endif

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <lp.h>
#include <msgs.h>

#include "properties.h"
#include "lpsys.h"
#include "error.h"

#define BLOCK_SIZE	4096

typedef struct {
    Printer	*printer;
    Properties	*properties;
    char	*outName;
    int		inFd;
    int		outFd;
    XtInputId	readId;
    XtInputId	errorId;
    char	**fileList;
    char	**rqstList;
    int		indx;
    int		outIndx;
} JobInfo;

static void CopyJob (JobInfo *info);
static void JobDone (JobInfo *info, Boolean ok);
static void CopyBlock (XtPointer client_data, int *inFd, XtInputId *id);
static void InputError (XtPointer client_data, int *inFd, XtInputId *id);
static void ResumeJob (Widget widget, XtPointer client_data,
		       XtPointer call_data);
extern void Die (Widget widget, XtPointer client_data, XtPointer call_data);

/* SubmitJob
 *
 * Send a list of files to the printer.  files is a null-terminated list of
 * file names.  The file list is an element of the properties.  A zero-length
 * file name means stdin.
 */
void
SubmitJob (Properties *properties)
{
    Printer	*printer = properties->printer;
    char	**files = properties->files;
    JobInfo	*info;
    char	*prefix;
    register	cnt;
    static char	*badGetFilesMsg;
    static	first = True;

    if (first)
    {
	first = False;

	badGetFilesMsg = GetStr (TXT_badGetFiles);
    }

    info = (JobInfo *) XtMalloc (sizeof (*info));
    info->indx = info->outIndx = 0;
    info->properties = properties;

    for (cnt=0; files [cnt]; cnt++)
	;	/* Do nothing */

    prefix = LpAllocFiles (cnt + 1);
    if (!prefix)
    {
	ErrorConfirm (TopLevel, badGetFilesMsg, Die, (XtPointer) Job_Error);
	while (--cnt>=0)
	    XtFree (files [cnt]);
	XtFree ((char *) info);
	return;
    }

    info->rqstList = (char **) XtMalloc (cnt * sizeof (char *));
    info->fileList = files;

    /* Leave enough space in outName to tack on the '0' for the request name */
    info->outName = (char *) XtMalloc (strlen(Lp_Temp) + strlen (prefix) + 4);
    sprintf (info->outName, "%s/%s-", Lp_Temp, prefix);

    info->printer = printer;

    CopyJob (info);
}	/* End of SubmitJob () */

/* CopyJob
 *
 * Copy a file to be printed into a temporary file with the lp tree.  The
 * file name may be blank, in which case, stdin will be used.
 */
static void
CopyJob (JobInfo *info)
{
    char	fileName [256];
    char	errbuf [256];
    struct stat	statbuf;
    static char	*badFileMsg;
    static char	*noDirMsg;
    static char	*noOpenMsg;
    static char *noOpenOutputMsg;
    static	first = True;

    if (first)
    {
	first = False;

	badFileMsg = GetStr (TXT_badFile);
	noDirMsg = GetStr (TXT_noDir);
	noOpenMsg = GetStr (TXT_noOpen);
	noOpenOutputMsg = GetStr (TXT_noOpenOutput);
    }

    info->readId = info->errorId = (XtInputId) 0;
    info->inFd = info->outFd = -1;

    /* If the file is stdin, it is already open; otherwise, open the file.
     * Disallow directories.
     */
    if (!*info->fileList [info->indx])
    {
	info->inFd = 0;
	(void) fstat (0, &statbuf);
    }
    else
    {
	if (stat (info->fileList [info->indx], &statbuf))
	{
	    sprintf (errbuf, badFileMsg, info->fileList [info->indx]);
	    ErrorConfirm (TopLevel, errbuf, ResumeJob, (XtPointer) info);
	    return;
	}
    }

    if (statbuf.st_mode & S_IFMT == S_IFDIR)
    {
	sprintf (errbuf, noDirMsg, info->fileList [info->indx]);
	ErrorConfirm (TopLevel, errbuf, ResumeJob, (XtPointer) info);
	return;
    }

    if (*info->fileList [info->indx])
    {
	info->inFd = open (info->fileList [info->indx], O_RDONLY | O_NDELAY);
	if (info->inFd == -1)
	{
	    sprintf (errbuf, noOpenMsg, info->fileList [info->indx]);
	    ErrorConfirm (TopLevel, errbuf, ResumeJob, (XtPointer) info);
	    return;
	}
    }

    sprintf (fileName, "%s%d", info->outName, info->indx+1);
    info->outFd = open (fileName, O_WRONLY);
    if (info->outFd == -1)
    {
	ErrorConfirm (TopLevel, noOpenOutputMsg, Die,
		      (XtPointer) Internal_Error);
	JobDone (info, False);
	return;
    }

    /* Set up the copy operation as an asynchronous event. */
    info->readId = XtAppAddInput (AppContext, info->inFd,
			       (XtPointer) XtInputReadMask, CopyBlock, info);
    info->errorId = XtAppAddInput (AppContext, info->inFd,
			(XtPointer) XtInputExceptMask, InputError, info);
}	/* End of CopyJob () */

/* CopyBlock
 *
 * Copy a block of data from the input file to the temp file.  Only one block
 * is copied (as opposed to the entire file) to allow other things a chance
 * to run.  client_data is a pointer to a JobInfo structure;
 */
static void
CopyBlock (XtPointer client_data, int *inFd, XtInputId *id)
{
    char	buf [BLOCK_SIZE];
    int		cnt;
    JobInfo	*info = (JobInfo *) client_data;

    cnt = read (*inFd, buf, BLOCK_SIZE);
    if (cnt < 0)
    {
	InputError (client_data, inFd, &info->errorId);
	return;
    }

    if (cnt == 0)
	JobDone (info, True);
    else
	if (cnt != write (info->outFd, buf, cnt))
	    InputError (client_data, inFd, &info->errorId);
}	/* End of CopyBlock () */

/* InputError
 *
 * Something bad happened when reading the input file.  Quit the job.
 * client_data is a pointer to a print job copy information structure.
 */
static void
InputError (XtPointer client_data, int *inFd, XtInputId *id)
{
    char	errbuf [256];
    JobInfo	*info = (JobInfo *) client_data;
    static char	*badCopyMsg;
    static	first = True;

    if (first)
    {
	first = False;

	badCopyMsg = GetStr (TXT_badCopy);
    }

    sprintf (errbuf, badCopyMsg, info->fileList [info->indx]);
    ErrorConfirm (TopLevel, errbuf, ResumeJob, (XtPointer) info);
}	/* End of InputError () */

/* JobDone
 *
 * Clean up after a copy operation.  If the copy succeeded, add the
 * file to the request list.  If there are still more files to process,
 * copy the next file on the list; otherwise, submit the request to lp.
 */
static void
JobDone (JobInfo *info, Boolean ok)
{
    char		*noticeMsg;
    char		buf [256];
    static char		*rqIdMsg;
    static char		*prtDeletedMsg;
    static char		*badCharSetMsg;
    static char		*badPitchMsg;
    static char		*badPageSizeMsg;
    static char		*bannerRqdMsg;
    static char		*deniedMsg;
    static char		*badFormMsg;
    static char		*formDeniedMsg;
    static char		*unmountedMsg;
    static char		*rejectMsg;
    static char		*noFilterMsg;
    static char		*badJobMsg;
    static char		*noJobMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;

	rqIdMsg = GetStr (TXT_rqId);
	prtDeletedMsg = GetStr (TXT_prtDeleted);
	badCharSetMsg = GetStr (TXT_badCharSet);
	badPitchMsg = GetStr (TXT_badPitch);
	badPageSizeMsg = GetStr (TXT_badPageSize);
	bannerRqdMsg = GetStr (TXT_bannerRqd);
	deniedMsg = GetStr (TXT_denied);
	badFormMsg = GetStr (TXT_badForm);
	formDeniedMsg = GetStr (TXT_formDenied);
	unmountedMsg = GetStr (TXT_unmounted);
	rejectMsg = GetStr (TXT_reject);
	noFilterMsg = GetStr (TXT_noFilter);
	badJobMsg = GetStr (TXT_badJob);
	noJobMsg = GetStr (TXT_noJob);
    }

    if (info->readId)
	XtRemoveInput (info->readId);
    if (info->errorId)
	XtRemoveInput (info->errorId);

    /* Close the src and dst files, if they were successfully opened.  Be
     * sure to leave stdin open.
     */
    if (info->inFd > 0)
	close (info->inFd);
    if (info->outFd > 0)
	close (info->outFd);

    /* Add to request list */
    if (ok)
    {
	char	*name;

	name = info->rqstList [info->outIndx++] =
	    XtMalloc (strlen (info->outName) + 10);
	sprintf (name, "%s%d", info->outName, info->indx + 1);
    }

    /* Process the next file on the list */
    if (info->fileList [++info->indx])
	CopyJob (info);
    else
    {
	long		prtChk;
	int		status;
	int		rc;
	register	i;

	/* No more files to process.  Ship it off to lp. */
	info->rqstList [info->outIndx] = (char *) 0;
	info->properties->request->file_list = info->rqstList;

	rc = Job_Error;
	if (info->outIndx != 0)
	{
	    strcat (info->outName, "0");
	    /* outName was formed using Lp_Temp, but putrequest demands that
	     * absolute path names start with Lp_Tmp.  Thanks a lot.  So use
	     * a relative name that is just the last component.
	     */
	    (void) putrequest (strrchr (info->outName, '/') + 1,
			       info->properties->request);

	    status = LpRequest (info->outName, &info->properties->id, &prtChk);

	    /* If the request failed for a reason that the user can correct,
	     * set the return code to Job_Canceled.  In these cases, it would
	     * be nice if we could let the user correct the problem.
	     */
	    switch (status) {
	    case MOK:
		noticeMsg = buf;
		sprintf (buf, rqIdMsg, info->outIndx, info->properties->id);
		rc = Job_Submitted;
		break;
	    case MNODEST:
		noticeMsg = prtDeletedMsg;
		rc = No_Printer;
		break;
	    case MDENYDEST:
		rc = Job_Canceled;
		if (prtChk & PCK_CHARSET)
		    noticeMsg = badCharSetMsg;
		else if (prtChk & (PCK_CPI | PCK_LPI))
		    noticeMsg = badPitchMsg;
		else if (prtChk & (PCK_WIDTH | PCK_LENGTH))
		    noticeMsg = badPageSizeMsg;
		else if (prtChk & PCK_BANNER)
		    noticeMsg = bannerRqdMsg;
		else
		{
		    noticeMsg = deniedMsg;
		    rc = Job_Error;
		}
		break;
	    case MNOMEDIA:
		noticeMsg = badFormMsg;
		break;
	    case MDENYMEDIA:
		noticeMsg = formDeniedMsg;
		break;
	    case MNOMOUNT:
		noticeMsg = unmountedMsg;
		break;
	    case MERRDEST:
		noticeMsg = rejectMsg;
		break;
	    case MNOFILTER:
		noticeMsg = noFilterMsg;
		rc = Job_Canceled;
		break;
	    default:
		noticeMsg = badJobMsg;
		break;
	    }
	}
	else
	    noticeMsg = noJobMsg;

	/* If rc is Job_Canceled, there is an error that the user can
	 * probably correct on the property sheet.  And if this were
	 * truly the best of all possible worlds, lp would let us go
	 * back and easily correct this.  But we can't.  The temporary
	 * lp files are deleted by lp as soon as it detects an error.
	 * We could recopy all the files, except that we can't replace
	 * the files that were read in from stdin.  We could make
	 * a copy of the files somewhere other than in the lp tree,
	 * but this requires additional thought to avoid security problems.
	 * So, this problem will have to keep until a later release; for
	 * now, just quit.
	 */
	if (ok == True || info->outIndx != 0)
		ErrorConfirm (TopLevel, noticeMsg, Die, (XtPointer) rc);
	else
		exit((int)rc);

	for (i=0; i<info->outIndx; i++)
	    XtFree (info->rqstList [i]);

	for (i=0; i<info->indx; i++)
	    XtFree (info->fileList [i]);

	XtFree ((char *) info->fileList);
	XtFree ((char *) info->rqstList);
	XtFree (info->outName);
	XtFree ((char *) info);
    }
}	/* End of JobDone () */

/* ResumeJob
 *
 * Called when the error notice is popped down.  Resume processing jobs on
 * the file list.  client_data is a pointer to a job info structure.
 */
static void
ResumeJob (Widget widget, XtPointer client_data, XtPointer call_data)
{
    JobDone ((JobInfo *) client_data, False);
}	/* End of ResumeJob () */
