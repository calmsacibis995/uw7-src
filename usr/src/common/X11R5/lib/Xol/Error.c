#ifndef NOIDENT
#ident	"@(#)olmisc:Error.c	1.61"
#endif

/*
 *************************************************************************
 *
 * Description:
 *		The error diagnostic functions are held in this file.
 *	There are two types of error procedures and two warning procedures.
 *
 *		OlError()		- is a simple fatal error handler
 *		OlWarning()		- is a simple non-fatal error handler
 *		OlVaDisplayErrorMsg()	- Variable argument list fatal
 *					  error handler
 *		OlVaDisplayWarningMsg() - Variable argument list non-fatal
 *					  error handler
 *
 *******************************file*header*******************************
 */

						/* #includes go here	*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#if defined(__STDC__) || defined(c_plusplus) || defined(__cplusplus)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <X11/IntrinsicP.h>

/* Note: OpenLookP.h includes Error.h */

#define IN_ERROR_C
#include <Xol/OpenLookP.h>

#define CString OLconst char * OLconst
#include <Xol/xolerrstrs>
#include <Xol/xolmsgstrs>
#undef CString

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static int 	AddDatabaseRecord OL_ARGS((OLconst char *));
static void 	DeleteDatabaseRecord OL_ARGS((String));
static void	EchoApplicationPrefix OL_ARGS((Display *, OLconst char *, OLconst char *));
static void	ErrorHandler OL_ARGS((OLconst char *));
static int 	FindDatabaseIndexByName OL_ARGS((OLconst char *));
static int 	FindDatabaseIndexByPtr OL_ARGS((XrmDatabase));
static String	GetApplicationMsgPrefix OL_ARGS((Display *, OLconst char *));
static void	InitializeDatabaseRecords OL_NO_ARGS();
static Boolean	IsClient OL_ARGS((OLconst char *));
static String	ResolvedFilename OL_ARGS((Display *, OLconst char *));
static Boolean	StopMessage OL_ARGS((Display *, OLconst char *, OLconst char *));
static void	WarningHandler OL_ARGS((OLconst char *));

static void	VaDisplayErrorMsgHandler OL_ARGS((Display *,
			OLconst char *, OLconst char *, OLconst char *, 
			OLconst char *, va_list));
static void	VaDisplayWarningMsgHandler OL_ARGS((Display *,
			OLconst char *, OLconst char *, OLconst char *, 
			OLconst char *, va_list));

					/* public procedures		*/

OlErrorHandler
	OlSetErrorHandler OL_ARGS((OlErrorHandler));

OlVaDisplayErrorMsgHandler
	OlSetVaDisplayErrorMsghandler OL_ARGS((OlVaDisplayErrorMsgHandler));
OlVaDisplayWarningMsgHandler
	OlSetVaDisplayWarningMsgHandler OL_ARGS((OlVaDisplayWarningMsgHandler));
OlWarningHandler
	OlSetWarningHandler OL_ARGS((OlWarningHandler));


char *		OlGetMessage OL_ARGS((Display *, char *, int, 
				OLconst char *, OLconst char *, OLconst char *,
				OLconst char *, XrmDatabase));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

typedef struct {
	String db_name;
	XrmDatabase db_id;
	String class_prefix;
	char *msg_prefix;
} db_rec ;

static db_rec *db_list;		/* list of database records */
static int nrecs;

static char error_buffer[BUFSIZ];

static XrmDatabase OlToolkitErrDatabase;
static XrmDatabase OlToolkitMsgDatabase;
static Boolean toolkit_e_try;
static Boolean toolkit_m_try;
static String resolved_stopfile;

static char *last_locale, *current_locale;

extern void		exit OL_ARGS((int));

static OlErrorHandler		error_handler	= ErrorHandler;
static OlWarningHandler		warning_handler	= WarningHandler;

static OlVaDisplayErrorMsgHandler	va_display_error_msg_handler =
					VaDisplayErrorMsgHandler;
static OlVaDisplayErrorMsgHandler	va_display_warning_msg_handler =
					VaDisplayWarningMsgHandler;

#if OlNeedFunctionPrototypes
#define VARARGS_START(a,m)	va_start(a, m)
#else
#define VARARGS_START(a,m)	va_start(a)
#endif

			/* define a macro to swap the old handler with the
			 * new one.  Before swapping, set the new one to
			 * the default handler if the new one is NULL.	*/

#define CHK_AND_SWAP(type,new,old,def) \
	type	tmp;\
	if (new == NULL) new = def;\
	tmp	= new;\
	new	= old;\
	old	= tmp;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ****************************private*procedures****************************
 */

/*
 *************************************************************************
 * AddDatabaseRecord- given name of database file, add a database record.
 ****************************procedure*header*****************************
 */

static int
AddDatabaseRecord OLARGLIST((db_name))
	OLGRA (OLconst char *, db_name)
{
	db_list = (db_rec *) XtRealloc((char*)db_list,(nrecs+1)*sizeof(db_rec));

	db_list[nrecs].db_name = XtNewString(db_name);
					/* should not have to do this */
	db_list[nrecs].db_id = (XrmDatabase) NULL;
	db_list[nrecs].class_prefix = (char *) NULL;
	db_list[nrecs].msg_prefix = (char *) NULL;
	nrecs++;

	return nrecs-1; /* index */

} /* END OF AddDatabaseRecord() */

/*
 *************************************************************************
 * DeleteDatabaseRecord- given name of database file (resolved filename), 
 *    delete a database record.
 ****************************procedure*header*****************************
 */

static void
DeleteDatabaseRecord OLARGLIST((db_name))
	OLGRA (String, db_name) /* resolved filename */
{
	int i=0,idx=0;

	if(db_list==NULL)
		return;

	idx = FindDatabaseIndexByName(db_name);
	if(idx == -1)
		return;

	XtFree(db_list[idx].db_name);
	XtFree(db_list[idx].class_prefix);
	XtFree(db_list[idx].msg_prefix);

	if(nrecs-1 == 0) {
		XtFree((char *) db_list);
		db_list = (db_rec *) NULL;
	}
	else {
		for(i=idx; i<nrecs-1; i++)
			db_list[i] = db_list[i+1]; /* shift records in */

		db_list  = (db_rec *) XtRealloc(
					(char*) db_list,nrecs*sizeof(db_rec));
	}
	nrecs--;

} /* END OF DeleteDatabaseRecord() */

/*
 *************************************************************************
 * ErrorHandler - default warning handler
 ****************************procedure*header*****************************
 */
static void
ErrorHandler OLARGLIST((msg))
	OLGRA( OLconst char *,	msg)
{
	char error_text[BUFSIZ];

	(void)	OlGetMessage((	Display *)NULL,
				error_text,
				BUFSIZ,
				OleNfileError,
				OleTmsg1,
				OleCOlToolkitMessage,
				OleMfileError_msg1,
				(XrmDatabase)NULL);

	EchoApplicationPrefix((Display *)NULL,OleCOlToolkitMessage,error_text);
	(void) fprintf(stderr, (OLconst char *)"%s\n", (char *)msg);
	exit(1);
} /* END OF ErrorHandler() */

/*
 *************************************************************************
 * FindDatabaseIndexByName - locate a database record by name of database file.
 ***************************procedure*header*****************************
 */
static int 
FindDatabaseIndexByName OLARGLIST((db_name))
	OLGRA(OLconst char *, db_name)
{
	int idx;

	if(db_name == (char *)NULL)
		return(-1);

	for(idx = 0; idx < nrecs; idx++) 
		if(db_list[idx].db_name &&
		   strcmp(db_list[idx].db_name, db_name) == 0)
			return(idx);

	return(-1) ;

} /* END OF FindDatabaseIndexByName() */

/*
 *************************************************************************
 * FindDatabaseIndexByPtr - locate a database record by XrmDatabase pointer.
 ***************************procedure*header*****************************
 */
static int 
FindDatabaseIndexByPtr OLARGLIST((database))
	OLGRA (XrmDatabase, database)
{
	int idx;

	for(idx = 0; idx<nrecs; idx++) 
		if(db_list[idx].db_id == database)
			return idx;

	return (-1) ;

} /* END OF FindDatabaseIndexByPtr() */

/*
 *************************************************************************
 * GetApplicationMsgPrefix - get a custom application prefix, if it exists.
 * Otherwise, use the toolkit default application prefix.
 ****************************procedure*header*****************************
 */
static String
GetApplicationMsgPrefix OLARGLIST((display, class))
	OLARG( Display *,	display)
	OLGRA( OLconst char *,		class)
{
	OLconst char *msgT, *msgM;
	String application_prefix = NULL;
	Boolean is_client = IsClient(class);
	int idx = FindDatabaseIndexByName(class);

	if(idx!=-1  && db_list[idx].class_prefix && class
		&&!strcmp(db_list[idx].class_prefix,class) 
		&& db_list[idx].msg_prefix!=NULL)

		return db_list[idx].msg_prefix;

					/* first look for a custom prefix */

	if(is_client)
		application_prefix =  (String) OlGetMessage(	display,
								NULL,
								0,	
								OleNmessage,
								OleTprefix,
								class,
								(OLconst char *)"",
								(XrmDatabase)NULL);

	if(!is_client || (is_client && application_prefix &&
		 strlen(application_prefix) == 0)) { 

		if(is_client) {
			msgT = OleTmsg5;
			msgM = OleMfileError_msg5;
		}
		else {
			msgT = OleTmsg4;
			msgM = OleMfileError_msg4;
		}

		application_prefix =  (String)OlGetMessage(	display,
								NULL,	
								0,
								OleNfileError,
								msgT,
								OleCOlToolkitMessage,
								msgM,
								(XrmDatabase)NULL);
	}

	return application_prefix;

} /* END OF GetApplicationMsgPrefix() */ 


/* ************************************************************************* 
 * InitializeDatabaseRecords - when the locale changes, close all the
 * databases and initialize all the internal records.
 ****************************procedure*header*****************************
 */
static void
InitializeDatabaseRecords ()
{
	int i;
	Boolean found1 = FALSE;
	Boolean found2 = FALSE;
	XrmDatabase database;
	Arg arg;
	static char *read_locale;

/*
 * Locale is first ascertained in first OlGetMessage call
 */

	XtSetArg(arg,XtNxnlLanguage, &read_locale);
	OlGetApplicationValues( (Widget)NULL, &arg,1 );

	XtFree(last_locale);
	last_locale = current_locale;
	current_locale = XtNewString(read_locale);

	if(!strcmp(last_locale,current_locale) || nrecs == 0)
		return;

	for(i=0;i<nrecs;i++) {

		XtFree(db_list[i].db_name);
		XtFree(db_list[i].class_prefix);
		XtFree(db_list[i].msg_prefix);
		database = db_list[i].db_id;
		XrmDestroyDatabase(database);
		if(!found1 && database == OlToolkitErrDatabase) {
			OlToolkitErrDatabase = (XrmDatabase)NULL;
			found1 = TRUE;	
		}
		if(!found2 && database == OlToolkitMsgDatabase) {
			OlToolkitMsgDatabase = (XrmDatabase)NULL;
			found2 = TRUE;	
		}
	}
	XtFree((char *)db_list);
	db_list = (db_rec *)NULL;
	nrecs = 0;
	toolkit_e_try = FALSE;
	toolkit_m_try = FALSE;
	XtFree(resolved_stopfile);
	resolved_stopfile = (String)NULL;

} /* END OF InitializeDatabaseRecords() */

/* ************************************************************************* 
 * IsClient - check to see if call is from a client or toolkit and use the
 * information for message formatting.  Note that sometimes we get this 
 * information from the message class and other times from the file name itself.
 ****************************procedure*header*****************************
 */
static Boolean
IsClient OLARGLIST((token))
	OLGRA( OLconst char *,	token)
{
	if( !strcmp(token, OleCOlToolkitWarning)
			 || !strcmp(token,OleCOlToolkitError)
			 || !strcmp(token,OleCOlToolkitMessage)
			 || !strncmp(token,(OLconst char *)"xol_",4)) /* for any new files */
		return FALSE;

	return TRUE;

} /* END OF IsClient() */

/* ************************************************************************* 
 * ResolvedFilename - takes the message file name as input and
 * returns the full pathname of the localized message file.
 ****************************procedure*header*****************************
 */

static String
ResolvedFilename OLARGLIST((display,filename))
	OLARG( Display *,	display)
	OLGRA( OLconst char *,		filename)
{
	String resolved_filename;

	if(IsClient(filename))
		resolved_filename = XtResolvePathname( 	display,
							(OLconst char *)"app-defaults",
							filename,
							(OLconst char *)"",	
							(String)NULL,
							(Substitution)NULL,
							(Cardinal)0,
							(XtFilePredicate) NULL);
	else
		resolved_filename = XtResolvePathname( 	display,
							(OLconst char *)"messages",
							filename,
							(OLconst char *)"",	
							(String)NULL,
							(Substitution)NULL,
							(Cardinal)0,
							(XtFilePredicate) NULL);

	return resolved_filename;

} /* END OF ResolvedFilename() */

/* ************************************************************************* 
 * StopMessage - check to see if client has a ***_stop
 * file with specific toolkit message(s) to be silenced,
 * indicated by *<name>.<type>:<NULL message>
 ****************************procedure*header*****************************
 */
static Boolean
StopMessage OLARGLIST((display,name,type))
	OLARG( Display *,	display)
	OLARG( OLconst char *,	name)
	OLGRA( OLconst char *,	type)
{
	String app_class;
	static String app_name; 
	String filename;
	static Boolean try_stopfile = False;

	if(try_stopfile && resolved_stopfile==NULL)
		return FALSE;

	if(resolved_stopfile == NULL &&  (display != (Display *)NULL ||
		(display = OlDefaultDisplay) != (Display *)NULL)){
		int n;
		String tmp_str;

		XtGetApplicationNameAndClass(display, &app_name, &app_class);
		n= strlen(app_name);

		filename = XtMalloc((n + 6)*sizeof(char));
		strcpy(filename,app_name);
		strcpy(&filename[n],(OLconst char *)"_stop");

		resolved_stopfile = 
			XtNewString(tmp_str=ResolvedFilename(display,filename));
		XtFree(filename);
		XtFree(tmp_str);
		try_stopfile = TRUE;
		if(resolved_stopfile==NULL)
			return FALSE;
		if(access(resolved_stopfile,R_OK)!=0)
			return FALSE;
	}

	if(resolved_stopfile &&
	   strlen(OlGetMessage(	(Display *)NULL,
				(char *)NULL,
				BUFSIZ,
				name,
				type,
				resolved_stopfile,
				(OLconst char *)"bogus default message",
				(XrmDatabase)NULL)))

		return TRUE;
	else
		return FALSE;


} /* END OF StopMessage() */

/*
 *************************************************************************
 * WarningHandler - default warning handler
 ****************************procedure*header*****************************
 */
static void
WarningHandler OLARGLIST((msg))
	OLGRA( OLconst char *,	msg)
{
	char warning_text[BUFSIZ];

	(void) OlGetMessage((	Display *)NULL,
				warning_text,
				BUFSIZ,
				OleNfileError,
				OleTmsg2,
				OleCOlToolkitMessage,
				OleMfileError_msg2,
				(XrmDatabase)NULL);

	EchoApplicationPrefix((Display *)NULL,OleCOlToolkitMessage,warning_text);
	(void) fprintf(stderr, (OLconst char *)"%s\n", (char *)msg);
} /* END OF WarningHandler() */

/*
 *************************************************************************
 * VaDisplayErrorMsgHandler - default error handler
 ****************************procedure*header*****************************
 */
static void
VaDisplayErrorMsgHandler OLARGLIST((display,name,type,class,message,vargs))
	OLARG( Display *,display)	/* Display pointer or NULL	*/
	OLARG( OLconst char *,	name)	/* message's resource name	*/
	OLARG( OLconst char *,	type)	/* message's resource type	*/
	OLARG( OLconst char *,	class)	/* class of message		*/
	OLARG( OLconst char *,	message)/* the message string		*/
	OLGRA( va_list,	vargs)		/* variable arguments		*/
{
	char buf[BUFSIZ];
	char error_text[BUFSIZ];
	Boolean is_client = IsClient(class);
	XrmDatabase database;

				/* if client silences a message */

	if(StopMessage(display,name,type))
		exit(1);

	if(is_client)
		database = (XrmDatabase)NULL;
	else {
		if(!strcmp(class,OleCOlToolkitWarning) 
			|| !strcmp(class,OleCOlToolkitError))
			database = OlToolkitErrDatabase;
		else if(!strcmp(class,OleCOlToolkitMessage))
			database = OlToolkitMsgDatabase;
	}

	(void) OlGetMessage(	display, buf, BUFSIZ, name, type,
					class, message, database);

	(void) OlGetMessage(	display,
				error_text,
				BUFSIZ,
				OleNfileError,
				OleTmsg1,
				OleCOlToolkitMessage,
				OleMfileError_msg1,
				database);

	EchoApplicationPrefix(display, class, error_text);
	(void) vfprintf(stderr, buf, vargs);
	(void) fputc('\n', stderr);
	exit(1);

} /* END OF VaDisplayErrorMsgHandler() */

/*
 *************************************************************************
 * VaDisplayWarningMsgHandler - default warning handler
 ****************************procedure*header*****************************
 */
static void
VaDisplayWarningMsgHandler OLARGLIST((display,name,type,class,message,vargs))
	OLARG( Display *,display)	/* Display pointer or NULL	*/
	OLARG( OLconst char *,	name)	/* message's resource name	*/
	OLARG( OLconst char *,	type)	/* message's resource type	*/
	OLARG( OLconst char *,	class)	/* class of message		*/
	OLARG( OLconst char *,	message)/* the message string		*/
	OLGRA( va_list,	vargs)		/* variable arguments		*/
{
	char buf[BUFSIZ];
	char warning_text[BUFSIZ];
	Boolean is_client = IsClient(class);
	XrmDatabase database;

				/* if client silences a message */

	if(StopMessage(display,name,type))
		return;

	if(is_client)
		database = (XrmDatabase)NULL;
	else {
		if(!strcmp(class,OleCOlToolkitWarning) 
			|| !strcmp(class,OleCOlToolkitError))
			database = OlToolkitErrDatabase;
		else if(!strcmp(class,OleCOlToolkitMessage))
			database = OlToolkitMsgDatabase;
	}


	(void) OlGetMessage(	display, buf, BUFSIZ, name, type,
					class, message, database);
	(void) OlGetMessage(	display,
				warning_text,
				BUFSIZ,
				OleNfileError,
				OleTmsg2,
				OleCOlToolkitMessage,
				OleMfileError_msg2,
				database);

	EchoApplicationPrefix(display, class, warning_text);
	(void) vfprintf(stderr, buf, vargs);
	(void) fputc('\n', stderr);

} /* END OF VaDisplayWarningMsgHandler() */

/*
/*
 *************************************************************************
 * OlGetMessage - if a client message, try to open a locale client 
 * database first, a locale toolkit database second, and if these two
 * fail, return the default error/warning message.  For a client, look 
 * for the message in both databases.  If a toolkit message, look for 
 * a locale toolkit database first and if this fails, return the default 
 * error/warning message.  
 * 
 * Note that if the client passes NULL as the default message, 
 * a generic default message is provided, using the name and type of message.  
 * This means the client is not required to have a set of default messages.
 ****************************procedure*header*****************************
 */

char *
OlGetMessage OLARGLIST((display, buf, buf_size, name, type, class, default_msg, database))
	OLARG( Display *,	display)
	OLARG( char *,		buf)
	OLARG( int,		buf_size)
	OLARG( OLconst char *,	name)
	OLARG( OLconst char *,	type)
	OLARG( OLconst char *,	class)
	OLARG( OLconst char *,	default_msg)
	OLGRA( XrmDatabase,	database)
{
XrmDatabase database_found = (XrmDatabase) NULL;
Boolean is_client = IsClient(class);
Boolean client_database = FALSE;
String type_str;
XrmValue result;
char temp[BUFSIZ];
static Boolean done = FALSE;

XtAppContext ac = (XtAppContext) (display != (Display *)NULL ||
		   (display = OlDefaultDisplay) != (Display *)NULL ?
			XtDisplayToApplicationContext(display) : NULL);

/*
 * InitializeDatabaseRecords() reinitializes things whenever
 * there is a change of locale.
 */

if(done == FALSE) {
	char *read_locale = NULL;
	Arg arg;
						/* get the current locale */
	XtSetArg(arg,XtNxnlLanguage, &read_locale);
	OlGetApplicationValues( (Widget)NULL, &arg,1 );
	last_locale = XtNewString(read_locale);
	current_locale = XtNewString(read_locale);

						/* prepare for locale change */
	OlRegisterDynamicCallback( (OlDynamicCallbackProc) InitializeDatabaseRecords, (XtPointer)NULL);
	done = TRUE;
}

result.addr = (caddr_t) NULL;

if (ac != (XtAppContext)NULL) {

   if(database != (XrmDatabase)NULL) {

      database_found = database;
      if(is_client)
         client_database = TRUE;

      else {
         if( !strcmp(class,OleCOlToolkitWarning) 
			|| !strcmp(class,OleCOlToolkitError))

            OlToolkitErrDatabase = database;

         else if(!strcmp(class,OleCOlToolkitMessage))
            OlToolkitMsgDatabase = database;
      }
   } 

   else {	/* no database passed */

      if(is_client) {
		int idx = FindDatabaseIndexByName(class);

		if(idx != -1) {
        		if ( (database_found = db_list[idx].db_id) 
				!= (XrmDatabase) NULL)
            		client_database = TRUE;
		}
        	else {
        		database_found = OlOpenDatabase(display,class);
            		if(database_found != (XrmDatabase) NULL)
               			client_database = TRUE;
        	}

      }
			/* for toolkit */
      else {

         if(!strcmp(class,OleCOlToolkitWarning)
			 || !strcmp(class,OleCOlToolkitError)) {

            if(OlToolkitErrDatabase!=(XrmDatabase)NULL)
               database_found = OlToolkitErrDatabase;
            else if(OlToolkitErrDatabase==(XrmDatabase)NULL 
						&& !toolkit_e_try) {
               database_found = OlToolkitErrDatabase = OlOpenDatabase(display,(OLconst char *)"xol_errs");
               toolkit_e_try = TRUE;
            }
         }
	 else if(!strcmp(class,OleCOlToolkitMessage)) {

            if(OlToolkitMsgDatabase!=(XrmDatabase)NULL)
            		database_found = OlToolkitMsgDatabase;
            else if(OlToolkitMsgDatabase==(XrmDatabase)NULL 
						&& !toolkit_m_try) {
               database_found = OlToolkitMsgDatabase = OlOpenDatabase(display,(OLconst char *)"xol_msgs");
               toolkit_m_try = TRUE;
            }
         }
      }
   } /* no database passed */

/*
 * last part is to try to get message, opening toolkit database if necessary
 */

   (void) sprintf(temp, (OLconst char *)"%s.%s", name, type);
   /* database_found is the database returned by the lookup above -
    * it COULD be equal to NULL, since we now store NULL
    * databases for efficiency - in which case, the Xrm call
    * below doesn't get executed.
    */
   if(database_found)
      (void) XrmGetResource(	database_found,
				temp,
				class,
				&type_str,
				&result);

/*
 * Do not open the toolkit database looking for an 
 * (optional) customized client prefix.
 */

   if(!strcmp(name,(OLconst char *)"message") && !strcmp(type,(OLconst char *)"prefix"))
	goto message_processing ;

    if(( is_client && client_database && result.addr==(caddr_t)NULL) 
		|| (is_client && !client_database)) { 

        if(OlToolkitErrDatabase!=(XrmDatabase)NULL)
           database_found = OlToolkitErrDatabase;

        else if(OlToolkitErrDatabase==(XrmDatabase)NULL && !toolkit_e_try) {
           database_found = OlToolkitErrDatabase = OlOpenDatabase(display,(OLconst char *)"xol_errs");
           toolkit_e_try = TRUE;
        }
        if(database_found) 
          (void) XrmGetResource(	database_found,
					temp,
					class,
					&type_str,
					&result);
    }
} /* ac != NULL */

message_processing: {}

/* if there is no ac or a message was not found - return a default message */

if (ac == (XtAppContext)NULL
		|| (ac!=(XtAppContext)NULL && !database_found)
		|| (ac!=(XtAppContext)NULL && result.addr==NULL)) {
	char *return_defmsg;
	int buffer_size;

	if(default_msg != (char *) NULL) {
		buffer_size = strlen(default_msg) + 1;
		return_defmsg = (char *)default_msg;;
	}
	else {
		(void) sprintf(error_buffer,(OLconst char *)"%s %s",name,type);
		buffer_size = strlen(name) + strlen(type) +4;
		error_buffer[buffer_size-1] = '\0';
		return_defmsg = error_buffer;
	}

	if(buf!=NULL) {
		if(strlen(return_defmsg)!=0)
   			(void) strncpy(buf, return_defmsg, buffer_size);
   		buf[buf_size-1] = '\0';
		return buf;
	}
	else 
		return return_defmsg;
}

/* 
 * if here, then there is an ac and a message - copy the message if 
 * a buffer is provided and return the appropriate string pointer 
 */ 

if (buf!=(char *)NULL) {
	(void) strncpy(buf,result.addr,buf_size);
   	buf[buf_size-1] = '\0';
	return buf;
}
else
	return result.addr;

} /* END OF OlGetMessage() */

/*
 *************************************************************************
 * OlOpenDatabase - given a specific Xrm database file, return XrmDatabase.
 ****************************procedure*header*****************************
 */

XrmDatabase
OlOpenDatabase OLARGLIST((display, class))
	OLARG( Display *,	display)
	OLGRA( OLconst char *,		class)
{
	Boolean is_client = IsClient(class);
	String resolved_filename;
	XrmDatabase database = (XrmDatabase)NULL;
	int idx;


	XtAppContext ac = (XtAppContext) (display != (Display *)NULL ||
			   (display = OlDefaultDisplay) != (Display *)NULL ?
				XtDisplayToApplicationContext(display) : NULL);

	if(ac == (XtAppContext) NULL )
		return (XrmDatabase) NULL;

	idx = FindDatabaseIndexByName(class);

	if(idx != -1) { /* previous (un)successful open tries */
		if(db_list[idx].db_id==(XrmDatabase)NULL)
			return (XrmDatabase) NULL;
		else
			return db_list[idx].db_id;
	}
	/* Can't find database in current list; start
	 * from scratch
	 */
	resolved_filename = ResolvedFilename(display,class);
	if(resolved_filename==(String)NULL) {
		/* WARNING HERE */
		/* Can't find database file, make dumb db record,
	 	 * set db_id to NULL and return.  AddDBRec()
		 * NULLs all fields.
	 	 */
		idx = AddDatabaseRecord(class);
		return (XrmDatabase)NULL;
	}

	database = XrmGetFileDatabase(resolved_filename);
	XtFree(resolved_filename);
	idx = AddDatabaseRecord(class);
	db_list[idx].db_id = database;
	db_list[idx].class_prefix = XtNewString(class);
	db_list[idx].msg_prefix = 
		XtNewString(GetApplicationMsgPrefix(display,class));

	if(database!=(XrmDatabase)NULL && !is_client) {
		if(!strcmp(class,OleCOlToolkitError) 
			|| !strcmp(class,OleCOlToolkitWarning))
				OlToolkitErrDatabase = database;
		else if(!strcmp(class,OleCOlToolkitMessage))
				OlToolkitMsgDatabase = database;
	}

	return database;

} /* END OF OlOpenDatabase() */
  
/*
 *************************************************************************
 * OlCloseDatabase - given a specific Xrm database, destroy it.
 ****************************procedure*header*****************************
 */
void
OlCloseDatabase OLARGLIST(( database))
	OLGRA( XrmDatabase,	database)
{
	int idx;

	if(database==(XrmDatabase)NULL)
		return;

	if(database != (XrmDatabase)NULL)
		(void) XrmDestroyDatabase(database);

	idx = FindDatabaseIndexByPtr(database);
	if(idx != -1)
		(void) DeleteDatabaseRecord (db_list[idx].db_name);

	if(OlToolkitErrDatabase == database)
		OlToolkitErrDatabase = (XrmDatabase)NULL;
	else if(OlToolkitMsgDatabase == database)
		OlToolkitMsgDatabase = (XrmDatabase)NULL;
}

/*
 *************************************************************************
 * EchoApplicationPrefix - get the application prefix string and 
 * echo this to stderr, using the application name and message type 
 * as identifiers in this prefix.
 ****************************procedure*header*****************************
 */
static void
EchoApplicationPrefix OLARGLIST((display, class, msg_type))
	OLARG( Display *,	display)
	OLARG( OLconst char *,		class)
	OLGRA( OLconst char *, msg_type)		 /* error or warning text */
{
	String	name;
	char unknown_application[BUFSIZ];
	String application_prefix;

				/* first get an application name */

	if (display != (Display *)NULL ||
	    (display = OlDefaultDisplay) != (Display *)NULL)
	{
		String	aclass;		/* ignored */	

		XtGetApplicationNameAndClass(display, &name, &aclass);
	}
	else
	{
		String	dft_msg;

			/* if display is NULL then it is because the
			 * error/warning is happened in toolkit
			 * initialization so we want to give a nicer
			 * information then "??Unknown"...
			 */
		dft_msg = display == (Display *)NULL ?
					XrmQuarkToString(_OlApplicationName) :
					(char *)OleMfileError_msg3;

		(void) OlGetMessage(	display,
					unknown_application,
					BUFSIZ,
					OleNfileError,
					OleTmsg3,
					OleCOlToolkitMessage,
					dft_msg,
					(XrmDatabase)NULL);

		name = (String) unknown_application;
	}

					/* then get a message prefix */

	application_prefix = GetApplicationMsgPrefix(display,class);

	(void) fprintf(stderr, application_prefix, (char *)name, msg_type);

} /* END OF EchoApplicationPrefix() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlVaDisplayErrorMsg - this procedure handles errors of a fatal type.
 ****************************procedure*header*****************************
 */
void
OlVaDisplayErrorMsg OLARGLIST((display,name,type,class,message,OLVARGLIST))
	OLARG( Display *,display)	/* Display pointer or NULL	*/
	OLARG( OLconst char *,	name)		/* message's resource name	*/
	OLARG( OLconst char *,	type)		/* message's resource type	*/
	OLARG( OLconst char *,	class)		/* class of message		*/
	OLARG( OLconst char *,	message)	/* the message string		*/
	OLVARGS				/* variable arguments		*/
{
	va_list	args;			/* the variable arguments	*/

	VARARGS_START(args, message);
	(*va_display_error_msg_handler)(display, name, type, 
						class, message, args);
	va_end(args);
} /* END OF OlVaDisplayErrorMsg() */

/*
 *************************************************************************
 * OlVaDisplayWarningMsg - this procedure handles errors of a non-fatal type.
 ****************************procedure*header*****************************
 */
void
OlVaDisplayWarningMsg OLARGLIST((display,name,type,class,message,OLVARGLIST))
	OLARG( Display *,display)	/* Display pointer or NULL	*/
	OLARG( OLconst char *,	name)		/* message's resource name	*/
	OLARG( OLconst char *,	type)		/* message's resource type	*/
	OLARG( OLconst char *,	class)		/* class of message		*/
	OLARG( OLconst char *,	message)	/* the message string		*/
	OLVARGS				/* variable arguments		*/
{
	va_list		args;		/* the variable arguments	*/

	VARARGS_START(args, message);
	(*va_display_warning_msg_handler)(display, name, type, 
							class, message, args);
	va_end(args);
} /* END OF OlVaDisplayWarningMsg() */

/*
 *************************************************************************
 * OlError - this procedure writes the application supplied message to
 * stderr and then exits with a status of 1 if the application did not
 * specify an overriding warning handler.
 ****************************procedure*header*****************************
 */
void
OlError OLARGLIST((s))
	OLGRA( OLconst char *,	s)
{
	(*error_handler)(s);
} /* END OF OlError() */

/*
 *************************************************************************
 * OlSetVaDisplayErrorMsgHandler - sets the procedure to be called when a
 * fatal error occurs.
 ****************************procedure*header*****************************
 */
OlVaDisplayErrorMsgHandler
OlSetVaDisplayErrorMsgHandler OLARGLIST((handler))
	OLGRA( OlVaDisplayErrorMsgHandler, handler) /* handler or NULL	*/
{
	CHK_AND_SWAP(OlVaDisplayErrorMsgHandler, handler,
		va_display_error_msg_handler, VaDisplayErrorMsgHandler);
	return(handler);
} /* END OF OlSetVaDisplayErrorMsgHandler() */

/*
 *************************************************************************
 * OlSetVaDisplayWarningMsgHandler - sets the procedure to be called when
 * a non-fatal error occurs.
 ****************************procedure*header*****************************
 */
OlVaDisplayWarningMsgHandler
OlSetVaDisplayWarningMsgHandler OLARGLIST((handler))
	OLGRA( OlVaDisplayWarningMsgHandler, handler) /* handler or NULL*/
{
	CHK_AND_SWAP(OlVaDisplayWarningMsgHandler, handler,
		va_display_warning_msg_handler, VaDisplayWarningMsgHandler);
	return(handler);
} /* END OF OlSetVaDisplayWarningMsgHandler() */

/*
 *************************************************************************
 * OlSetErrorHandler - registers/unregisters the fatal error handler
 ****************************procedure*header*****************************
 */
OlErrorHandler
OlSetErrorHandler OLARGLIST((handler))
	OLGRA( OlErrorHandler, handler)
{
	CHK_AND_SWAP(OlErrorHandler, handler, error_handler, ErrorHandler);
	return(handler);
} /* END OF OlSetErrorHandler() */

/*
 *************************************************************************
 * OlSetWarningHandler - registers/unregisters the non-fatal error handler
 ****************************procedure*header*****************************
 */
OlWarningHandler
OlSetWarningHandler OLARGLIST((handler))
	OLGRA( OlWarningHandler, handler)
{
	CHK_AND_SWAP(OlWarningHandler, handler, warning_handler,
				WarningHandler);
	return(handler);
} /* END OF OlSetWarningHandler() */

/*
 *************************************************************************
 * OlWarning - this procedure writes the application supplied message to
 * stderr and then returns if the application did not specify an
 * overriding warning handler.
 ****************************procedure*header*****************************
 */
void
OlWarning OLARGLIST((s))
	OLGRA( OLconst char *,	s)
{
	(*warning_handler)(s);
} /* END OF OlWarning() */

