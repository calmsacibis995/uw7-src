#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/items.c	1.33"
#endif

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <FList.h>
#include "uucp.h"
#include "error.h"

LoginData *CreateBlankLoginEntry();
void AllocateLoginEntry();
extern void	CreateBlankEntry();
extern void	FreeLoginData();
extern void	FreeHostData();
extern void	FreeNew();
extern void	AddLineToBuffer();
extern void	DeleteLineFromBuffer();
extern void	CheckMode();
extern void	NotifyUser();

static void 	GetDirectory();

static		exists;		/* Non-zero if file exists */
static		readable;	/* Non-zero if file is readable */
static		writeable;	/* Non-zero if file is writable or */
				/* directory is writable and file */
				/* doesn't exist */
LinePtr			GetLine();
#ifdef debug
void			PrintFlatItems();
#endif

static FILE *fd;

/*
 * This routine reads the /etc/uucp/Systems file and populates the info 
 * into the internal data structure that will be used in both the flatlist
 * and the property sheet.
 * The update flag will be used by other routines to make the apply, delete
 *
 * dp -- pointer to the default values (since last apply) of the property
 * sheet.
 */

 
void
GetFlatItems(filename)
char * filename;
{
	static unsigned	stat_flags = 0;	/* Mask returned by CheckMode() */
	int		n = 0;
	HostData	*dp;
	LinePtr		lp;
	char		*begin, *end;
	unsigned	offset, length;
	char		*token;
	char		*f_name;
	char		*f_time;
	char		*f_type;
	char		*f_class;
	char		*f_phone;
	char		*f_expect[F_MAX/2];
	char		*f_respond[F_MAX/2];
	char		buf[BUFSIZ];

	CheckMode(filename, &stat_flags);	/* get the file stat	*/
	exists = BIT_IS_SET(stat_flags, FILE_EXISTS);
	readable = BIT_IS_SET(stat_flags, FILE_READABLE);
	writeable = BIT_IS_SET(stat_flags, FILE_WRITEABLE);
#ifdef debug
	fprintf(stderr, "/etc/uucp/System exists = %d, readable = %d, writeable = %d\n",
			exists, readable, writeable);
#endif
	if (!exists & !writeable) { /* somthing's serious wrong */
#ifdef debug
		fprintf(stderr, GGT(string_createFail), filename);
		exit(1);
#endif
		sprintf(buf, GGT(string_noFile), filename);
		rexit(1, buf, "");
	} else
	if (!readable & exists) {
#ifdef debug
		fprintf(stderr, GGT(string_accessFail), filename);
		exit(2);
#endif
		sprintf(buf, GGT(string_accessFail), filename);
		rexit(2, buf, "");
	} else
	if (exists & readable & !writeable) {
		fprintf(stderr, GGT(string_writeFail), filename);
		/*
		** Update appropriate widgets to reflect the new state
		*/

	}
	if ((fd = fopen(filename, "r")) == (FILE *)NULL) {
		/* Something's wrong here but there are some chances	*/
		/* we might be able to create the file if we are the	*/
		/* prviledged user.  For now just put out message	*/
#ifdef debug
		fprintf(stderr, GGT(string_openFail), filename);
#endif
		;
	} else {
		for (sf->numFlatItems=0; (lp = GetLine (buf, BUFSIZ, fd)) != NULL;
		     sf->numFlatItems++) {
			if ((sf->numFlatItems % INCREMENT) == 0 &&
			   sf->numFlatItems == sf->numAllocated) {
				sf->flatItems = (FlatList *) XtRealloc (
				(char *) sf->flatItems,
				(sf->numFlatItems + INCREMENT) * sizeof(FlatList));
				sf->numAllocated += INCREMENT;
			}
			buf[strlen(buf)-1] = '\0';
			begin = &buf[0];
			if ((token = strtok(buf, " \t\n")) == NULL) {
				sf->numFlatItems--;
				continue;
			} else
				f_name = token;
			if ((token = strtok((char *) NULL, " \t\n")) == NULL) {
				sf->numFlatItems--;
				continue;
			} else
				f_time = token;
			if ((token = strtok((char *) NULL, " \t\n")) == NULL) {
				sf->numFlatItems--;
				continue;
			} else
				f_type = token;
			if ((token = strtok((char *) NULL, " \t\n")) == NULL) {
				sf->numFlatItems--;
				continue;
			} else
				f_class = token;
			if ((token = strtok((char *) NULL, " \t\n")) == NULL) {
				sf->numFlatItems--;
				continue;
			} else
				f_phone = token;
			dp = sf->flatItems[sf->numFlatItems].pField =
				(HostData *)XtMalloc(sizeof(HostData));
			dp->f_name = (XtPointer)strdup(f_name);
			dp->f_time = (XtPointer)strdup(f_time);
			dp->f_type = (XtPointer)strdup(f_type);
			dp->f_class = (XtPointer)strdup(f_class);
			dp->f_phone = (XtPointer)strdup(f_phone);
			dp->f_expectSeq = (XtPointer)NULL;
			dp->changed = False;
			dp->lp = lp;
			for (n =0;
			     (f_expect[n] = strtok(NULL, " \t\n")) != NULL &&
			     n < F_MAX/2;
			     n ++) 
			     if((f_respond[n] = strtok(NULL, " \t\n")) == NULL) { 
				n++;
				break;
			}
			
			dp->loginp = CreateBlankLoginEntry();

		
			if (dp->loginp->numAllocated < n ) {
				AllocateLoginEntry(dp->loginp, n);
			}
			dp->loginp->numExpectSend = n;
			dp->loginp->currExpectSend = -1;
#ifdef DEBUG
		fprintf(stderr,"currExpectSend=%d numExpectSend=%d n=%d\n", 
			dp->loginp->currExpectSend, dp->loginp->numExpectSend, n);
#endif
			for (n=0; n < dp->loginp->numExpectSend; n++) {
				dp->loginp->expectFlatItems[n].pExpectSend->f_prompt = 
					(XtPointer) strdup(f_expect[n]);
				dp->loginp->expectFlatItems[n].pExpectSend->f_response = 
					(XtPointer) strdup(f_respond[n]);
				}


			}
	}

		sf->currentItem = 0;
	if (fd != NULL)
		fclose (fd);
} /* GetFlatItems */

#ifdef debug
void
PrintFlatItems()
{
	register i;
	HostData *fp;
	char text [BUFSIZ];

	for (i=0; i<sf->numFlatItems; i++) {
		fp = sf->flatItems[i].pField;
		fprintf (stdout,
			"%s %s %s %s %s \n",
			 fp->f_name,
			 fp->f_time,
			 fp->f_type,
			 fp->f_class,
			 fp->f_phone);
	}
} /* PrintFlatItems */
#endif

void
PutFlatItems(filename)
char *filename;
{
	register LinePtr	endp;
	HostData *fp;
	register i, j;
	int len;
	char text [BUFSIZ*4];
	char buf [BUFSIZ];

	buf[0] = NULL;
#ifdef TRACE
	fprintf(stderr,"Put FlatItems filename=%s\n",filename);
#endif

	for (i=0; i<sf->numFlatItems; i++) {
		fp = sf->flatItems[i].pField;
		if (fp->changed == False) continue;
		len =0;
    		buf[0] = 0;
    		for (j=0; j < fp->loginp->numExpectSend; j++) {
			strcpy(buf+len, fp->loginp->expectFlatItems[j].pExpectSend->f_prompt);
			len += strlen(fp->loginp->expectFlatItems[j].pExpectSend->f_prompt);
			buf[len] =  ' ';
			len++;
			if ((strcmp(fp->loginp->expectFlatItems[j].pExpectSend->f_response,"") == 0)) {
			

			strcpy(buf+len, "\"\" ");
			len += 3;
			} else {
			strcpy(buf+len, fp->loginp->expectFlatItems[j].pExpectSend->f_response);
			len += strlen(fp->loginp->expectFlatItems[j].pExpectSend->f_response);
			buf[len] = ' ';
			len++;
			}
    		}
			buf[len++] = '\0';
			fp->f_expectSeq = (XtPointer) strdup(buf);
#ifdef DEBUG
	fprintf(stderr,"buf=%s\n", buf);
	fprintf(stderr, "expectSeq=%s\n",fp->f_expectSeq);
#endif

		sprintf (text,
			"%s %s %s %s %s %s\n",
			fp->f_name,
			fp->f_time,
			fp->f_type,
			fp->f_class,
			fp->f_phone,
			fp->f_expectSeq);
		XtFree (fp->lp->text);
		fp->lp->text = strdup(text);
		fp->changed = False;
	}
#ifdef DEBUG
	fprintf(stderr,"PutFlatItems text=%s\n", text);
#endif
	if ((fd = fopen(filename, "w")) == NULL) {
		sprintf (buf, GGT(string_fopenWrite), filename);
                NotifyUser(sf->toplevel, buf);
		return;
	}
	for (endp = sf->lp; endp; endp = endp->next)
		if (*(endp->text)) fputs(endp->text, fd);

#ifdef DEBUG
fprintf(stderr,"before PUTMSG  %s\n", GGT(string_updated));

#endif
	fclose (fd);
	if (!exists) {
		chmod(filename, (mode_t) 0640);
		chown(filename, UUCPUID, UUCPGID);
		exists = 1;
	}
} /* PutFlatItems */

void
CreateBlankEntry()
{
	HostData *dp;

	if ((sf->numFlatItems % INCREMENT) == 0 &&
	   sf->numFlatItems == sf->numAllocated) {
		sf->flatItems = (FlatList *) XtRealloc ((char *)sf->flatItems,
		(sf->numFlatItems + INCREMENT) * sizeof(FlatList));
		sf->numAllocated += INCREMENT;
	}

#ifdef TRACE
	fprintf(stderr,"CreateBlankEntry\n");
#endif
	if (new) FreeNew();
	new = (FlatList *) XtMalloc (sizeof(FlatList));
	dp = new->pField = (HostData *)XtMalloc(sizeof(HostData));
	if (work) {
		FreeLoginData(work);
	}
	dp->loginp = (LoginData *) CreateBlankLoginEntry();
	work = (LoginData *) CreateBlankLoginEntry();
	UpdateLoginScrollList(work);
	dp->f_name = (XtPointer) strdup (BLANKLINE);
	dp->f_time = (XtPointer) strdup("Any");
	dp->f_type = (XtPointer) strdup("ACU");
	dp->f_class = (XtPointer) strdup("Any");
	dp->f_phone = (XtPointer) strdup(BLANKLINE);
	dp->f_expectSeq = NULL;
	dp->lp = (LinePtr)0;
	dp->changed = False;

} /* CreateBlankEntry */


LoginData *
CreateBlankLoginEntry()
{
	LoginData *loginp;

#ifdef TRACE
	fprintf(stderr,"CreateBlankLoginEntry\n");
#endif
	loginp = (LoginData *) XtMalloc(sizeof (LoginData));
	loginp->numAllocated = 0;
	loginp->currExpectSend = -1;
	loginp->numExpectSend = 0;
	loginp->expectFlatItems = NULL;
	AllocateLoginEntry(loginp, LOGIN_INCREMENT);

	return loginp;
} /* CreateBlankLoginEntry */

void
AllocateLoginEntry(logp, count)
LoginData *logp;
int count;
{

	int i;

#ifdef TRACE
fprintf(stderr, "AllocateLoginEntry with numAllocated=%d numItems=%d count =%d\n", logp->numAllocated, logp->numExpectSend, count);	
#endif

    logp->expectFlatItems = (LoginFlatList *) XtRealloc ((char *)
            logp->expectFlatItems,
       	(logp->numAllocated + count) * sizeof(LoginFlatList));
	for (i= logp->numAllocated; i < (logp->numAllocated + count); i++) {
			logp->expectFlatItems[i].pExpectSend = (loginData *)
				XtMalloc(sizeof (loginData));
			logp->expectFlatItems[i].pExpectSend->f_prompt = NULL;
			logp->expectFlatItems[i].pExpectSend->f_response = NULL;
	}
    logp->numAllocated += count;

}

void
DeleteFlatItems()
{
	HostData *dp;
	int i;

#ifdef TRACE
	fprintf(stderr,"DeleteFlatItems\n");
#endif
	for (i=0; i<sf->numFlatItems; i++) {
		dp = sf->flatItems[i].pField;
		FreeHostData (dp);
	}
#ifdef TRACE
	fprintf(stderr,"END DeleteFlatItems\n");
#endif
} /* DeleteFlatItems */



LinePtr
GetLine(buf, len, fd)
char *buf;
int len;
FILE *fd;
{
	register char *p;
	LinePtr	lp;
	while ((p = fgets(buf, len, fd)) != NULL) {
		lp = (LinePtr) XtMalloc (sizeof(LineRec));
		lp->next = (LinePtr) NULL;
		lp->text = strdup(p);
		AddLineToBuffer((LinePtr) NULL, lp);
		if (buf[0] == ' ' || buf[0] == '\t' ||  buf[0] == '\n'
		    || buf[0] == '\0' || buf[0] == '#')
			continue;
		return(lp);
	}
	return((LinePtr) NULL);
} /* GetLine */

/*
 * add lp to the position right after ap in the buffer if ap is not null
 * otherwise, add it to the end of the buffer
 */
void
AddLineToBuffer(ap, lp)
LinePtr	ap, lp;
{
	if( sf->lp == NULL)
		sf->lp = lp;
	else {
		if (ap == NULL) {
			LinePtr endp;
			for (endp=sf->lp; endp->next; endp=endp->next);
			endp->next = lp;
		} else {
			lp->next = ap->next;
			ap->next = lp;
		}
	}
} /* AddLineToBuffer */

void
DeleteLineFromBuffer(tp)
LinePtr tp;
{
	register LinePtr lp = sf->lp;
	if (lp == NULL) return;
	if (lp == tp) /* lucky, found it */
		sf->lp = tp->next;
	else
		for (; lp->next; lp = lp->next)
			if (lp->next == tp) {
				lp->next =tp->next;
				break;
			}
	if (tp->text != NULL) {
		XtFree(tp->text);
		tp->text = NULL;
	}
	if (tp != NULL) {
		XtFree((char *)tp);
		tp = NULL;
	}
} /* DeleteLineFromBuffer */

void
FreeHostData(dp)
HostData *dp;
{

#ifdef TRACE
	fprintf(stderr,"FreeHostData\n");
#endif
	if (dp->f_name != NULL) {
		 XtFree (dp->f_name);
		dp->f_name = NULL;
	}
	if (dp->f_time != NULL) {
		XtFree (dp->f_time);
		dp->f_time = NULL;
	}

	if (dp->f_type != NULL) {
		 XtFree (dp->f_type);
		dp->f_type = NULL;
	}
	if (dp->f_class != NULL) {
		XtFree (dp->f_class);
		dp->f_class = NULL;
	}
	if (dp->f_phone != NULL)  {
		XtFree (dp->f_phone);
		dp->f_phone = NULL;
	}
	if (dp->f_expectSeq != NULL) {
		XtFree (dp->f_expectSeq);
		dp->f_expectSeq = NULL;
	}
	if (dp->lp != NULL) {
		DeleteLineFromBuffer(dp->lp);
	}
	FreeLoginData(dp->loginp);
	/*XtFree ((XtPointer) dp->loginp);*/
	if (dp != NULL) {
		XtFree ((XtPointer)dp);
		dp = NULL;
	}
#ifdef TRACE
	fprintf(stderr,"End FreeHostData\n");
#endif
} /* Free */

/* free the associated data structure for the new item */

void
FreeLoginEntries(logp)
LoginData *logp;
{
    int i;


#ifdef TRACE
	fprintf(stderr,"FreeLoginEntries  logp->numExpectSend=%d\n", logp->numExpectSend);
#endif
    for (i=0; i < logp->numExpectSend; i++) {
	if (logp->expectFlatItems[i].pExpectSend->f_prompt) {
        	XtFree(logp->expectFlatItems[i].pExpectSend->f_prompt);
        	logp->expectFlatItems[i].pExpectSend->f_prompt=NULL;
	}
	if (logp->expectFlatItems[i].pExpectSend->f_response) {
        	XtFree(logp->expectFlatItems[i].pExpectSend->f_response);
        	logp->expectFlatItems[i].pExpectSend->f_response=NULL;
	}
    }
    logp->numExpectSend = 0;
    logp->currExpectSend = -1;
#ifdef TRACE
	fprintf(stderr, "END FreeLoginEntries\n");
#endif
}

void
FreeLoginData(logp)
LoginData *logp;
{
	int i;

#ifdef TRACE
	fprintf(stderr,"FreeLoginData  logp->numExpectSend=%d\n", logp->numExpectSend);
#endif


	FreeLoginEntries(logp);
	for (i=0; i < logp->numAllocated; i++) {
		if (logp->expectFlatItems[i].pExpectSend) {
			logp->expectFlatItems[i].pExpectSend = NULL;
		}
	}
	logp->numAllocated = 0;
	if (logp->expectFlatItems) {
		XtFree ((XtPointer) logp->expectFlatItems);
		logp->expectFlatItems = NULL;
	}
	if (logp) {
		XtFree ((XtPointer) logp);
		logp = NULL;
	}

#ifdef TRACE
	fprintf(stderr,"END FreeLoginData\n");
#endif
}

void
FreeNew()
{
#ifdef TRACE
	fprintf(stderr,"FreeNew\n");
#endif
	FreeHostData(new->pField);
	XtFree((XtPointer)new);
	new = (FlatList *) NULL;
} /* FreeNew */



/*
** uses access() to set bits in `stat_flags' corresponding to
** FILE_READABLE and FILE_WRITEABLE.  FILE_EXISTS is set by this function.
** if the file doesn't exist and the file's directory is accessible,
** access() is called on the directory.
*/
void
CheckMode(pathname, stat_flags)
char		*pathname;
unsigned	*stat_flags;
{
	char		dirname[128];
	unsigned short	useruid;
	unsigned short	usergid;
	struct stat	statbuf;

	errno = 0;
	/* for tfadmin */
	if (access(pathname, W_OK) == 0) {
		SET_BIT(*stat_flags, FILE_WRITEABLE | FILE_EXISTS);
	}
	if (access(pathname, R_OK) == 0) {
		SET_BIT(*stat_flags, FILE_READABLE | FILE_EXISTS);
		return;
	} else {
		switch (errno) {
			case ENOENT:	/* File named by `name' doesn't */
					/* exist; check the directory */
				UNSET_BIT(*stat_flags, FILE_EXISTS);
				GetDirectory(pathname, dirname);
				if (access(dirname, W_OK) == -1)
				/* something's wrong here */
					*stat_flags = 0;
				else
					SET_BIT(*stat_flags, FILE_WRITEABLE);
				break;
			case EACCES:
			case EMULTIHOP:
			case ENOLINK:
			case ENOTDIR:
				*stat_flags = 0;
				break;

			default:
				exit(1);
				break;
		}
	}
} /* CheckMode */

/*
** Returns the result of stripping the last component in the pathname in
** dirname.
*/
static void
GetDirectory(pathname, dirname)
char	*pathname;
char	*dirname;
{
	char	*cp;
  
	/*
	** find the last occurance of '/' in pathname
	*/
	cp = strrchr(pathname, '/');

	if (cp) {
		cp++;
		while (pathname != cp)
			*dirname++ = *pathname++;
	} else
		*dirname++ = '.';

	*dirname = '\0';

} /* GetDiretory */
