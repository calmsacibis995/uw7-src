#pragma ident	"@(#)libMDtI:misc.c	1.34"

#include <X11/Intrinsic.h>
#include "DesktopP.h"
#include "DesktopI.h"
#include "DtMsg.h"
#include <cursorfont.h>

	/* We should use POSIX version to keep libMundo alive.
	 * See Dm__UpdateUserFile for more details.
	 */
#if defined(USE_REGCOMP)	/* POSIX version */
#include <regex.h>
#else				/* libgen version */
#include <libgen.h>
#endif				/* defined(USE_REGCOMP) */

#include "DtStubI.h"


#define LIST_INCREMENT 4	/* for DtCallbackList routines */

/* For Dm__UpdateUserFile() */
#define LOOKING_FOR_COMMENTS	0
#define LOOKING_FOR_NONBLANK	1
#define LOOKING_FOR_LANG	2
#define LOOKING_NOT		3
#define MOVE			"/sbin/mv"

/****************************procedure*header*****************************
    Dm_MakePath- make fully qualified path for 'name' by concatenating
	'path' and 'name'.  CAUTION: result is put in (static) Dm__buffer.

	ASSUMES 'path' begin with '/' and if path is NULL or "", 'name' MUST
	be full path.
*/
char *
Dm_MakePath(char * path, char * name, char * buf)
{
    register int len;

    if ((path == NULL) || (path[0] == '\0'))
    {
	len = 0;

    } else
    {
	/* Add '/' separator only if 'path' is not "/" */

	if ( (len = strlen(path)) > 1 )
	    (void)strcpy(buf, path);
	else
	    len = 0;

	buf[len++] = '/';		/* Add '/' separator */
    }

    (void)strcpy(buf + len, name);
    return(buf);
}					/* end of Dm_MakePath */

/*
 * supports simple ${env} expansion. 
 * supports simple %{prop} expansion (property values, if op is !NULL). 
 * - does support recursive expansion via the expand_proc.
 * - does not support conditional parameter expansion.
 * - does not modify the original string.
 * - returns malloced space. Thus the caller should free the returned string
 *   when done with it.
 * - no limit on the length of expanded value.
 */
char *
Dm__expand_sh(str, expand_proc, client_data)
char *str;
char *(*expand_proc)();
XtPointer client_data;
{
	register char *p = str;
	char *start;
	char *name;
	char *end;
	char *value;
	char *ret;
	char *free_this = NULL;
	int size;
	int notfirst = 0;
	int enclosed;

    while (1) {
	while (*p && ((*p != '$') && (!expand_proc || (*p != '%')))) p++;
	if (!*p) {
		/* nothing to expand */
		if (notfirst)
			return(str);
		else
			return(strdup(str));
	}

	start = p;
	if (*++p == '{') {
		/* look for matching '}' */
		name = ++p;
		while (*p && (*p != '}')) p++;
		enclosed = 1;
	}
	else {
		name = p;
		while (*p && (isalpha(*p) || isdigit(*p) || (*p == '_'))) p++;
		enclosed = 0;
	}
	/* now, p points to the end of env name */
	end = p;

	name = Dts__strndup(name, p - name);

	switch (*start) {
	case '$':
		value = getenv(name);
		break;
	case '%':
		value = (*expand_proc)(name, client_data);
		free_this = value;
		break;
	}
	free(name);

	size = strlen(str) + 1 + (value ? strlen(value) : 0);
	if (!(ret = (char *)malloc(size)))
		return(NULL);
	memcpy(ret, str, start - str);
	p = ret + (start - str);
	if (value) {
		strcpy(p, value);
		p += strlen(value);
	}
	if (free_this) {
		free(free_this);
		free_this = NULL;
	}

	if (enclosed)
		++end;
	strcpy(p, end);
	if (notfirst)
		free(str);
	str = ret;
	notfirst = 1;
    } /* while */
}

int
Dm__strnicmp(str1, str2, len)
register const char *str1;
register const char *str2;
register int len;
{
	register int c1;
	register int c2;

	while ((--len >= 0) &&
		((c1 = toupper(*str1)) == (c2 = toupper(*str2++))))
		if (*str1++ == '\0')
			return(0);
	return(len < 0 ? 0 : (c1 - c2));
}

int
Dm__stricmp(str1, str2)
register const char *str1;
register const char *str2;
{
	register int c1;
	register int c2;

	while (*str1 && *str2 &&
	       ((c1 = toupper(*str1)) == (c2 = toupper(*str2++)))) {
		str1++;
		str2++;
	}

	if (c1 != c2)
		return(c1 - c2);
	else
		return((*str1 == '\0') - (*str2 == '\0'));
}


#ifdef USE_REGCMP
char *
CvtToRegularExpression(expression)
char * expression;
{
	char *ret;

	if (expression == NULL || *expression == '\0')
   		ret = strdup(".*$");
	else {
   		register char * i;
   		register char * j = ret = malloc(strlen(expression) * 2 + 3);

   		*j++ = '^';
   		for (i = expression; *i; i++)
      		switch(*i) {
         	case '?':
			 *j++ = '.';
                   	break;
         	case '.':
			*j++ = '\\';
                   	*j++ = '.';
                   	break;
         	case '*':
			*j++ = '.';
                   	*j++ = '*';
                   	break;
         	default:
			*j++ = *i;
                   	break;
         	}
   		*j++ = '$';
   		*j   = '\0';
   	}

	return(ret);

} /* end of CvtToRegularExpression */
#endif

extern int
DmValueToType(value, map)
char  *value;
DmMapping map[];
{
 	int i;

        for(i=0; map[i].value; i++)
           if (!strcmp(value, map[i].value))
		return(map[i].type);

        return (-1);

}

extern char *
DmTypeToValue(type, map)
int type;
DmMapping map[];
{

        int i;

        for(i=0; map[i].value; i++)
           if (type == map[i].type)
/*		return(strdup(map[i].value)); */
		return((char *)map[i].value);

        return(NULL);
}


/****************************procedure*header*****************************
 *	Dm__AddToObjList- add 'new_obj' to container's object list.
 *	If this object is already in the list, ignore it.
 *
 *	INPUTS: container pointer
 *		object to be added
 *		options bitmask:
 *			DM_B_ADD_TO_END - add object to end of container list
 *					  (otherwise add to head)
 *	OUTPUTS: 0 for success, -1 for failure.
 *	GLOBALS:

	FLH MORE: if we ignore an addition, it is likely that we
	have a memory leak.  The caller may have allocated the object,
	and won't know that it was a duplicate.  Reference to the object
	will be lost.  
	NOTE!!: This routine should only be called by
        DmAddObjectToContainer.  DmAddObjectToContainer should be
        used by all other routines.
 *****************************************************************************/

int
Dm__AddToObjList(DmContainerPtr cp, DmObjectPtr new_obj, DtAttrs options)
{
	DmObjectPtr obj;

	if (!(options & DM_B_ALLOW_DUPLICATES)) {
		/* check for duplicate */
		for (obj = cp->op; obj != NULL; obj = obj->next)
		if ((obj == new_obj) || (new_obj->name &&
			(strcmp(obj->name, new_obj->name) == 0)))
		{
#ifdef DEBUG
			fprintf(stderr,"Dm__AddToObjList: duplicate obj; \"
				" check caller for memory leak\n");
#endif
			return(-1);
		}
	}

	if (options & DM_B_ADD_TO_END)
	{
		if (cp->op == NULL)
			cp->op = new_obj;
		else
		{
			DmObjectPtr end = cp->op;

			while(end->next != NULL)
				end = end->next;
			end->next = new_obj;
		}
	} else		/* add to head of list */
	{
		/* add op to cp */
		new_obj->next = cp->op;
		cp->op = new_obj;
	}
	new_obj->container = cp;
	cp->num_objs++;

	return(0);
}					/* end of Dm__AddToObjList */

char *
DmResolveSymLink(name, real_path, real_name)
char *name;
char **real_path;
char **real_name;
{
	char *p = strdup(name);

	if (DtsRealPath(p, Dm__buffer) == NULL) {
		free(p);
		*real_name = NULL;
		return(*real_path = NULL);
	}
	else {
		free(p);
		p = strdup(Dm__buffer);
		*real_name = DtsBaseName(p);
		*real_path = DtsDirName(p);
		return(p);
	}

} /* end of DmResolveSymLink */

/****************************procedure*header*****************************
	DtExpandProperty - Expands environment variables and properties defined
	in the property list provided by the caller and returns expanded string.
	The caller should free the returned string when done with it.
 */
char *
DtExpandProperty(char *str, DtPropListPtr plistp)
{
     register char *p = str;
     char *start;
     char *name;
     char *end;
     char *value;
     char *ret;
     char *free_this = NULL;
     int size;
     int notfirst = 0;
     int enclosed;
	char enc_char = '\0';

	while (1) {
		enc_char = '\0';
		while (*p && (*p != '"' || (*p == '"' && *(p+1) != '%') ||
			(*p == '"' && (*(p+1) == '%') && !plistp)) &&
		     (*p != '%' || (*p == '%' && !plistp)) && (*p != '$'))
			p++;
		if (!*p) {
			/* nothing to expand */
			if (notfirst)
				return(str);
			else
				/* return a copy of the original string */
				return(strdup(str));
		}
		if (*p == '"') {
			++p;
			enc_char = '"';
		}
		start = p;
		if (*(p+1) == '{') {
			++p;
			enc_char = '}';
			/* %{name} or ${name} : look for matching '}' */
		}

		if (enc_char != '\0') {
			name = ++p;
			while (*p && (*p != enc_char))
				p++;
			enclosed = 1;
		} else {
			/* $name or %name */
			name = ++p;
			while (*p && (isalpha(*p) || isdigit(*p) || (*p == '_')))
				p++;
			enclosed = 0;
		}
		/* now, p points to the end of env name */
		end = p;
		name = Dts__strndup(name, p - name);
		switch(*start) {
		case '%':
			if (plistp)
				value = DtsGetProperty(plistp, name, NULL);
			if (!value) {
				sprintf(Dm__buffer, "%%%s", name);
				value = Dm__buffer;
			}
			break;
		case '$':
			value = getenv(name);
			break;
		}
		size = strlen(str) + 1 + (value ? strlen(value) : strlen(name));
		if (!(ret = (char *)malloc(size)))
			return(NULL);
		if (enc_char == '"')
			--start;
		memcpy(ret, str, start - str);
		p = ret + (start - str);
		if (value) {
			strcpy(p, value);
			p += strlen(value);
		}
		free(name);

		if (free_this) {
			free(free_this);
			free_this = NULL;
		}
		if (enclosed)
			++end;
		strcpy(p, end);
		if (notfirst)
			free(str);
		str = ret;
		notfirst = 1;
	} /* while */

} /* DtExpandProperty */

void
DtFreeReply(DtReply *reply)
{
	switch(reply->header.rptype) {
	case DT_QUERY_FILE_CLASS:
		DtsFreePropertyList(&(reply->query_fclass.plist));
		XtFree(reply->query_fclass.class_name);
		break;
	case DT_GET_DESKTOP_PROPERTY:
		XtFree(reply->get_property.value);
		break;
	case DT_GET_FILE_CLASS:
		XtFree(reply->get_fclass.file_name);
		XtFree(reply->get_fclass.class_name);
		DtsFreePropertyList(&(reply->get_fclass.plist));
		break;
	case DT_GET_FILE_NAME:
		XtFree(reply->get_fname.file_name);
		break;
	case DT_OPEN_FOLDER:
	case DT_SYNC_FOLDER:
	case DT_CREATE_FILE_CLASS:
	case DT_DELETE_FILE_CLASS:
	case DT_SET_DESKTOP_PROPERTY:
	case DT_DISPLAY_PROP_SHEET:
	case DT_DISPLAY_BINDER:
	case DT_OPEN_FMAP:
	case DT_SHUTDOWN:
	case DT_DISPLAY_HELP:
	case DT_ADD_TO_HELPDESK:
	case DT_DEL_FROM_HELPDESK:
	case DT_OL_DISPLAY_HELP:
	case DT_MOVE_TO_WB:
	case DT_MOVE_FROM_WB:
	case DT_DISPLAY_WB:
	default:
		break;
	}

} /* end of DtFreeReply */

/*****************************************************************************
 *  	DtAddCallback:  add a callback to a callback list.
 *			A given (callback, client_data) pair can only
 *			be registered once on a callback list.
 *	INPUTS: callback list
 *		callback proc
 *		callback client data
 *	OUTPUTS: none
 *	GLOBALS: none
 *****************************************************************************/
void 
DtAddCallback(DtCallbackList *list, void (*func)(), XtPointer client_data)
{
    int i;
    XtCallbackRec *cb_item;

    /*
     * Check if this func/client_data pair is already registered
     */
    for (i=0, cb_item=list->cbs; i < list->used; i++, cb_item++){
	if (cb_item->callback == func && cb_item->closure == client_data){
	    return;
	}
    }
    if (list->used == list->alloced){
	list->alloced += LIST_INCREMENT;
	list->cbs = (XtCallbackList) REALLOC(list->cbs, list->alloced * sizeof(XtCallbackRec));
    }
    list->cbs[list->used].callback = func;
    list->cbs[list->used].closure = client_data;
    list->used++;
    return;
}	/* end of DtAddCallback */

/*****************************************************************************
 *  	DtCallCallbacks: call all callbacks on a list
 *		callback proc args are:
 *				shell widget of Desktop Folder
 *				registered client data
 *				call_data argument to DtCallCallbacks
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
void 
DtCallCallbacks(DtCallbackList *list, XtPointer call_data)
{
    int i;
    XtCallbackRec *cb_item;

#ifdef DEBUG
    fprintf(stderr,"DtCallCallbacks\n");
#endif

    /* A callback may call DtRemoveCallback to remove itself from
     * this list (e.g., if this is a destroy callback).  If we delete
     * an item other than the last, we fill the whole with the
     * last item.  This means that the last item gets moved to
     * the current position (and will not be considered again
     * if we are processing the items from beginning to end).
     * If we call the callbacks from end to beginning, we will
     * always be removing the last item and will never cause
     * the list to be shuffled.
     */

    for (i=list->used-1; i >= 0; i--){
	cb_item = &list->cbs[i];
	/* MORE: widget is always NULL in a DtCallback proc */
	(cb_item->callback)(NULL, cb_item->closure, call_data);
    }
}	/* end of DtCallCallbacks */

/*****************************************************************************
 *  	DtFreeCallbackList - free a callback list
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *
 *	CAUTION: This just frees the XtCallbackList, not the DtCallbackList
 *****************************************************************************/
void 
DtFreeCallbackList(DtCallbackList *list)
{
    if (list->alloced) {
	FREE(list->cbs);
	list->cbs = NULL;
    }
}	/* end of DtFreeCallbackList */

/*****************************************************************************
 *  	DtRemoveCallback: remove callback from a callback list
 *	INPUTS:  callback list
 *		 callback proc
 *		 client_data
 *	OUTPUTS: none
 *	GLOBALS: none
 *	DESCRIPTION: a (proc/data) pair can only appear on a callback list
 *		     once, so we only need find the first match
 *****************************************************************************/
void 
DtRemoveCallback(DtCallbackList *list, void (*func)(), XtPointer client_data)
{
    int i;
    XtCallbackRec *cb_item;

#ifdef DEBUG
    fprintf(stderr, "DtRemoveCallback\n");
#endif

    for (i=0, cb_item = list->cbs; i < list->used; i++, cb_item++) {
	if (cb_item->callback == func && cb_item->closure == client_data) {

	    list->used--;
	    if (list->used > 0){
		/*
		 * replace deleted item with last item
		 */
		cb_item->callback = list->cbs[list->used].callback;
		cb_item->closure = list->cbs[list->used].closure;
#ifdef NOT_USED
		/* Don't realloc list.  We may be in the middle
		   of DtCallcallbacks.
		 */
		if ((list->alloced - list->used) >= LIST_INCREMENT) {
		    list->alloced -= LIST_INCREMENT;
		    list->cbs = (XtCallbackList) REALLOC((void *) list->cbs, 
				 list->alloced * sizeof(XtCallbackRec));
		}
#endif
	    }
	    else if (list->alloced) {
		list->alloced = 0;
		FREE(list->cbs);
		list->cbs = NULL;
	    }
	}
    }

}	/* end of DtRemoveCallback */


/*
 *************************************************************************
 * DtGetShellOfWidget - this routine starts at the given widget looking to
 * see if the widget is a shell widget.  If it is not, it searches up the
 * widget tree until it finds a shell or until a NULL widget is
 * encountered.  The procedure returns either the located shell widget or
 * NULL.
 ****************************procedure*header*****************************
 */
Widget
DtGetShellOfWidget (Widget w)
{
	while(w != (Widget) NULL && !XtIsShell(w))
		w = XtParent(w);
	return(w);
} /* END OF DtGetShellOfWidget() */

/*****************************************************************************
 *  	DtGetBusyCursor: get the busy cursor
 *	MORE: this will not work across multiple screens
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
Cursor 
DtGetBusyCursor(Widget w)
{
    static Cursor BusyCursor;

    if (BusyCursor == NULL)
	BusyCursor = XCreateFontCursor(XtDisplayOfObject(w), XC_watch);
    return (BusyCursor);
} 	/* end of DtGetBusyCursor */
/*****************************************************************************
 *  	DtGetStandardCursor: get the standard cursor
 *	MORE: this will not work across multiple screens
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
Cursor 
DtGetStandardCursor(Widget w)
{
    static Cursor StandardCursor;

    if (StandardCursor == NULL)
	StandardCursor = XCreateFontCursor(XtDisplay(w), XC_left_ptr);
    return (StandardCursor);
} 	/* end of DtGetStandardCursor */

/*
 * Update the LANG environment variable in the file specified by filename
 */
void
Dm__UpdateUserFile(
	char *home, char *locale, char *filename,
	char *regexp1, char *regexp2, char *regexp3
)
{
	/* Have necessary #define(s) for these 2 types so that people
	 * can switch them easily:
	 *
	 *	1. #define USE_REGCOMP in c code or -DUSE_REGCOMP
	 *	   in Imakefile/Makefile, will enable POSIX version.
	 *	2. #undef USE_REGCOMP in c code or -UUSE_REGCOMP
	 *	   in Imakefile/Makefile, will enable libgen version.
	 *
	 * `1' is the default for sbird, and is enabled via Imakefile.
	 */
#if defined(USE_REGCOMP)	/* POSIX version */

#define SPRINTF(BUF,FMT,P1,P2,P3)       sprintf(BUF,FMT,P1,P2,P3)
#define REGEX_T                         regex_t
#define COMPILE(PREG,BUF,R,P1,P2,P3)    (regcomp(PREG,BUF,\
						REG_NOSUB|REG_EXTENDED) == 0)
#define EXEC_OK(PREG,REG,BUF)		(regexec(PREG,BUF,0,NULL,0) == 0)
#define FREE_IT(PREG,REG)		regfree(PREG)

#else /* libgen version */

#define SPRINTF(BUF,FMT,P1,P2,P3)
#define REGEX_T                         int
#define COMPILE(PREG,BUF,R,P1,P2,P3)    ((R = regcmp(P1,P2,P3,NULL)) != NULL)
#define EXEC_OK(PREG,REG,BUF)		(regex(REG, BUF) != NULL)
#define FREE_IT(PREG,REG)		free(REG)

#endif /* defined(USE_REGCOMP) */

	char *	outfile;
	char	infile[BUFSIZ];
	char	lang[BUFSIZ];
	char	buf[BUFSIZ];
	char *	reg;
	FILE *	in;
	FILE *	out;
	int	lookfor;

		/* define stuff for regex/regcmp */
	REGEX_T	preg;
	Boolean	valid_pattern;


		/* Assume these 3 can fit into buf[BUFSIZE] */
	SPRINTF(buf, "%s%s%s", regexp1, regexp2, regexp3);
	valid_pattern = COMPILE(&preg, buf, reg, regexp1, regexp2, regexp3);

	sprintf (lang, "%s%s%s\n", regexp1, locale, regexp3);

	sprintf (infile, "%s/%s", home, filename);
	if ((in = fopen(infile, "r")) == NULL) {
	    return;
	}
	outfile = tmpnam(0);
	if ((out = fopen(outfile, "w+")) == NULL) {
	    fclose(in);
	    return;
	}

	/* Now, loop thru the input file and write all lines not */
	/* matching the regular expression to the output file. */
	lookfor = LOOKING_FOR_COMMENTS;
	while (fgets(buf, BUFSIZ, in) != NULL) {
	    switch (lookfor) {
		case LOOKING_FOR_COMMENTS: {
		    if (buf[0] == '#') {
			break;
		    }
		    lookfor = LOOKING_FOR_NONBLANK;
		    /* Fall thru */
		}
		case LOOKING_FOR_NONBLANK: {
		    if (buf[0] == '\n') {
			break;
		    }
		    lookfor = LOOKING_FOR_LANG;
		    /* Fall thru */
		}
		case LOOKING_FOR_LANG: {
		    if (valid_pattern && EXEC_OK(&preg, reg, buf)) {
			strcpy(buf, lang);
		        lookfor = LOOKING_NOT;
		    }
		    break;
		}
	    }
	    fputs(buf, out);
	}

	if (lookfor != LOOKING_NOT) {
	    /* Opps! didn't find "LANG=" go back thru src file and */
	    /* put it into the temp file. */
	    rewind(in);
	    /* The output file doesn't need to be zero'ed because the */
	    /* addtion of the LANG= line will increase its size. */
	    rewind(out);

	    lookfor = LOOKING_FOR_COMMENTS;
	    while (fgets(buf, BUFSIZ, in) != NULL) {
		switch (lookfor) {
		    case LOOKING_FOR_COMMENTS: {
			if (buf[0] == '#') {
			    break;
			}
			lookfor = LOOKING_FOR_NONBLANK;
			/* Fall thru */
		    }
		    case LOOKING_FOR_NONBLANK: {
			if (buf[0] != '\n') {
			    lookfor = LOOKING_NOT;
			    fputs(lang, out);
			}
			break;
		    }
		}
		fputs(buf, out);
	    }
	    if (lookfor != LOOKING_NOT) {
		/* Maybe all the file has is a comment header. */
		/* In that case output the LANG= line. */
		fputs(lang, out);
	    }
	}
	fclose(out);
	fclose(in);

	sprintf(buf, "%s %s %s > /dev/null 2>&1", MOVE, outfile, infile);
	system(buf);

	if (valid_pattern)
		FREE_IT(&preg, reg);
}
