/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olmisc:FMcomm.c	1.14"
#endif

/* 
 * FMcomm.c
 *
 */

#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <Xol/OlClientsP.h>
#include <Xol/FMcomm.h>

#ifndef MEMUTIL
extern unsigned short getuid();
extern unsigned short getgid();
extern char *XtMalloc();
extern void XtFree();
#else
#include <X11/IntrinsicP.h>
#endif
extern XDeleteProperty();
static char * _FM_parse();
static char * SaveString();

/* {page break} */
/*
 * ClearFMQueue
 * ------------
 * The ClearFMQueue procedure clears the "_OL_FM_QUEUE" atom stored
 * in the server.  It's called at startup by the File Manager to
 * discard any requests that might have been queued before the
 * File Manager was ready to accept them.
 */

extern void ClearFMQueue(dpy)
Display * dpy;
{

XDeleteProperty(dpy, DefaultRootWindow(dpy), XA_OL_FM_QUEUE(dpy));

} /* end of ClearFMQueue */
/*
 * EnqueueFMRequest
 * ----------------
 * The EnqueueFMRequest function appends a File Manager request to
 * the "_OL_FM_QUEUE" property on the server.  The File Manager will
 * use the information packed in the request to create a new base
 * window.
 */

extern int EnqueueFMRequest(dpy, client, type, request)
Display *     dpy;
Window        client;
unsigned char type;
FM_Request *  request;
{
char * buffer;
int requestlen;
int retval       = 0;

requestlen =  1 /* type          */ +
             10 /* client        */ + 
             10 /* serial        */ + 
             strlen(NULL_DEF_STRING(request-> sysname)) +
             strlen(NULL_DEF_STRING(request-> nodename)) +
             strlen(NULL_DEF_STRING(request-> name)) +
             10 /* uid           */ + 
             10 /* gid           */ + 
             10 /* wingroup      */ + 
             strlen(NULL_DEF_STRING(request-> directory)) +
             strlen(NULL_DEF_STRING(request-> pattern)) +
             strlen(NULL_DEF_STRING(request-> label)) +
             strlen(NULL_DEF_STRING(request-> reserved)) +
             12 /* delimiters    */ + 
              1 /* ending NULL   */;

buffer = (char *) XtMalloc(requestlen);

(void) sprintf(buffer,
   "%c%10d%c%10d%c%s%c%s%c%s%c%10d%c%10d%c%10d%c%s%c%s%c%s%c%s%c",
   type, client,                                           DELIMITER,
   request-> serial,                                       DELIMITER,
   NULL_DEF_STRING(request-> sysname),                     DELIMITER,
   NULL_DEF_STRING(request-> nodename),                    DELIMITER,
   NULL_DEF_STRING(request-> name),                        DELIMITER,
   (int) getuid(),                                         DELIMITER,
   (int) getgid(),                                         DELIMITER,
   request-> wingroup,                                     DELIMITER,
   NULL_DEF_STRING(request-> directory),                   DELIMITER,
   NULL_DEF_STRING(request-> pattern),                     DELIMITER,
   NULL_DEF_STRING(request-> label),                       DELIMITER,
   NULL_DEF_STRING(request-> reserved),                    DELIMITER);

#ifdef  INLINE
XChangeProperty(dpy, DefaultRootWindow(dpy), XA_OL_FM_QUEUE(dpy), XA_STRING, 8, 
		PropModeAppend, (unsigned char *)(buffer), requestlen);
#else
EnqueueCharProperty(
	dpy, DefaultRootWindow(dpy), XA_OL_FM_QUEUE(dpy), buffer, requestlen);
#endif

XtFree(buffer);

return (retval);

} /* end of EnqueueFMRequest */
/*
 * DequeueFMRequest
 * ----------------
 * The DequeueFMRequest function removes a request from the "_OL_FM_QUEUE"
 * property stored on the server.  The FIle Manager calls this function
 * to retrieve requests stored using the EnqueueFMRequest function.  The
 * data stored in the request is used to create a new File Manager base
 * window.
 */

extern int DequeueFMRequest(dpy, client, type, request)
Display *       dpy;
Window  *       client;        /* Return */
unsigned char * type;          /* Return */
FM_Request *    request;       /* Return */
{
static char * buffer = NULL;
static int    bufferlen = 0;
static char * start  = NULL;

if ((buffer != NULL) && ((start - buffer) >= bufferlen))
   {
   XtFree(buffer);
   buffer = start = NULL;
   bufferlen = 0;
   }

if (buffer == NULL)
   if ( (buffer = GetCharProperty(
			dpy, DefaultRootWindow(dpy),
			XA_OL_FM_QUEUE(dpy), &bufferlen)) == NULL)
      return (QEMPTY);
   else
      start  = buffer;

*type               = start[0];
*client             = atoi(_FM_parse(&start[1], DELIMITER));
request-> serial    = atoi(_FM_parse(NULL, DELIMITER));
request-> sysname   = _FM_parse(NULL, DELIMITER);
request-> nodename  = _FM_parse(NULL, DELIMITER);
request-> name      = _FM_parse(NULL, DELIMITER);
request-> uid       = (unsigned short) atoi(_FM_parse(NULL, DELIMITER));
request-> gid       = (unsigned short) atoi(_FM_parse(NULL, DELIMITER));
request-> wingroup  = atoi(_FM_parse(NULL, DELIMITER));
request-> directory = _FM_parse(NULL, DELIMITER);
request-> pattern   = _FM_parse(NULL, DELIMITER);
request-> label     = _FM_parse(NULL, DELIMITER);
start               = _FM_parse(NULL, DELIMITER);
request-> reserved  = start;
start              += (strlen(request-> reserved) + 2);

return (GOTREQUEST);

} /* end of DequeueFMRequest */
/*
 * SendFMReply
 * -----------
 * The SendFMReply function packages and sends a reply to a client
 * from the File Manager.
 */

extern int SendFMReply(dpy, client, type, reply)
Display *     dpy;
Window        client;
unsigned char type;
FM_Reply *    reply;
{
int retval  = 0;
int replylen;
char * buffer;

replylen   =  1 /* type          */ +
             10 /* serial        */ + 
              strlen(NULL_DEF_STRING(reply-> sysname)) + 
              strlen(NULL_DEF_STRING(reply-> nodename)) + 
              strlen(NULL_DEF_STRING(reply-> path)) + 
              4 /* delimiters    */ + 
              1 /* ending NULL   */;

buffer = (char *) XtMalloc(replylen);

(void) sprintf(buffer,"%c%10d%c%s%c%s%c%s%c",
         type, 
         reply-> serial,                            DELIMITER,
         NULL_DEF_STRING(reply-> sysname),          DELIMITER,
         NULL_DEF_STRING(reply-> nodename),         DELIMITER,
         NULL_DEF_STRING(reply-> path),             DELIMITER);

XChangeProperty(dpy, client, XA_OL_FM_REPLY(dpy), XA_STRING, 8, 
		PropModeReplace,
		(unsigned char *)(buffer),
		replylen);

return (retval);
	
} /* end of SendFMReply */
/*
 * AcceptFMReply
 * -------------
 * The AcceptFMReply function is used to retrieve a reply posted on
 * a clients window by the File Manager.
 */

extern int AcceptFMReply(dpy, client, type, reply)
Display *  dpy;
Window     client;
int *      type;       
FM_Reply * reply;
{
int len = 0;
char * buffer;

if (( buffer = GetCharProperty(
			dpy, client, XA_OL_FM_REPLY(dpy), &len) ) != NULL)
   {
   *type            = buffer[0];
   reply-> serial   = atoi(_FM_parse(&buffer[1], DELIMITER));
   reply-> sysname  = SaveString(_FM_parse(NULL, DELIMITER));
   reply-> nodename = SaveString(_FM_parse(NULL, DELIMITER));
   reply-> path     = SaveString(_FM_parse(NULL, DELIMITER));
   XtFree(buffer);
   }

return (len);

} /* end of AcceptFMReply */

/*
 * _FM_parse
 *
 * This function is used to _FM_parse a given string using a given delimiter.
 * It can be used in lieu of strtok(3) when it is desireable to "find"
 * null string tokens.	That is if the delimiter is ':' and the string
 * "Test::string" is parsed, then this routine would return the tokens
 * "Test", NULL, and "string" whereas strtok returns "Test" and "string".
 *
 * Input:     char * string
 *	      int    delimiter
 *
 * Output:    Pointer to the next token.
 *
 * Note: This routine does no analysis to determine when it has exhausted
 *	 the original string.
 */

static char * _FM_parse(string, delimiter)
char * string;
int    delimiter;
{
static	 char * x = NULL;
register char * y;

if (string) x = string;
string = x;

if (x == NULL || *x == '\0')
   return NULL;
else
   {
   if ((y = strchr(x, delimiter)) == NULL)
      x = NULL;
   else
      {
      *y = '\0';
      x = y + 1;
      }
   return string;
   }

} /* end of _FM_parse */

/*
 * SaveString
 *
 * This function is a convenience routine used to allocate storage for
 * a string and save a copy in the new space.
 *
 * Input:     char * string
 *
 * Output:    char * newstring
 *
 */

static char * SaveString(string)
char * string;
{
char * new;

if (string == NULL)
   new = SaveString("");
else
   {
   new = (char *) XtMalloc((unsigned) (strlen(string) + 1));
   (void) strcpy(new, string);
   }

return (new);

} /* end of SaveString */

