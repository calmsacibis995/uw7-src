/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ident	"@(#)olmisc:WSMcomm.c	1.12"

/*
 * WSMcomm.c
 *
 */

#include <stdio.h>
#include <string.h>
#ifndef MEMUTIL
#include <X11/Xlib.h>
#else
#include <Intrinsic.h>
#endif
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <Xol/OlClientsP.h>
#include <Xol/WSMcomm.h>

typedef struct StringBuffer
   {
   char * Buffer;
   int    Size;
   } StringBuffer; 

static StringBuffer ResourceChangeBuffer = { NULL, 0 };
static StringBuffer ResourceDeleteBuffer = { NULL, 0 };

#ifndef MEMUTIL
extern unsigned int getuid();
extern unsigned int getgid();

extern char *XtMalloc();
extern void XtFree();
extern char *XtRealloc();
#endif

extern XDeleteProperty();
static char * _WM_parse();
static char * SaveString();

/* {page break} */
/*
 * ClearWSMQueue
 *
 */

extern void
ClearWSMQueue(dpy)
	Display * dpy;
{

XDeleteProperty(dpy, DefaultRootWindow(dpy), XA_OL_WSM_QUEUE(dpy));

} /* end of ClearWSMQueue */
/*
 * EnqueueWSMRequest
 *
 */

extern int EnqueueWSMRequest(dpy, client, type, request)
Display *     dpy;
Window        client;
unsigned char type;
WSM_Request * request;
{
char * buffer;
int requestlen;
int retval      = 0;

requestlen =  1 /* type          */ +
             10 /* client        */ + 
             10 /* serial        */ + 
             strlen(NULL_DEF_STRING(request-> sysname)) +
             strlen(NULL_DEF_STRING(request-> nodename)) +
             strlen(NULL_DEF_STRING(request-> name)) +
             10 /* uid           */ + 
             10 /* gid           */ + 
             strlen(NULL_DEF_STRING(request-> command)) +
             strlen(NULL_DEF_STRING(request-> reserved)) +
              9 /* delimiters    */ + 
              1 /* ending NULL   */;

buffer = (char *) XtMalloc(requestlen);

(void) sprintf(buffer,"%c%10d%c%10d%c%s%c%s%c%s%c%10d%c%10d%c%s%c%s%c",
                  type,
                  client,                     DELIMITER,
                  request-> serial,           DELIMITER,
  NULL_DEF_STRING(request-> sysname),         DELIMITER,
  NULL_DEF_STRING(request-> nodename),        DELIMITER,
  NULL_DEF_STRING(request-> name),            DELIMITER,
                  (int) getuid(),             DELIMITER,
                  (int) getgid(),             DELIMITER,
  NULL_DEF_STRING(request-> command),         DELIMITER,
  NULL_DEF_STRING(request-> reserved),        DELIMITER);

#ifdef  INLINE
XChangeProperty(dpy, DefaultRootWindow(dpy), XA_OL_WSM_QUEUE(dpy),
		XA_STRING, 8, PropModeAppend, (unsigned char *)(buffer),
		requestlen);
XFlush(dpy);
#else
EnqueueCharProperty(dpy, DefaultRootWindow(dpy),
		    XA_OL_WSM_QUEUE(dpy), buffer, requestlen);
#endif

XtFree(buffer);

return (retval);

} /* end of EnqueueWSMRequest */
/*
 * DequeueWSMRequest
 *
 */

extern int DequeueWSMRequest(dpy, client, type, request)
Display *          dpy;
Window  *          client;        /* Return */
unsigned char *    type;          /* Return */
WSM_Request *      request;       /* Return */
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
		XA_OL_WSM_QUEUE(dpy), &bufferlen)) == NULL)
      return (QEMPTY);
   else
      start  = buffer;

*type               = start[0];
*client             = atoi(_WM_parse(&start[1], DELIMITER));
request-> serial    = atoi(_WM_parse(NULL, DELIMITER));
request-> sysname   = _WM_parse(NULL, DELIMITER);
request-> nodename  = _WM_parse(NULL, DELIMITER);
request-> name      = _WM_parse(NULL, DELIMITER);
request-> uid       = (unsigned short) atoi(_WM_parse(NULL, DELIMITER));
request-> gid       = (unsigned short) atoi(_WM_parse(NULL, DELIMITER));
request-> command   = _WM_parse(NULL, DELIMITER);
start               = _WM_parse(NULL, DELIMITER);
request-> reserved  = start;
start              += (strlen(request-> reserved) + 2);

return (GOTREQUEST);

} /* end of DequeueWSMRequest */
/*
 * SendWSMReply
 *
 */

extern int SendWSMReply(dpy, client, type, reply)
Display *     dpy;
Window        client;
unsigned char type;
WSM_Reply *   reply;
{
int retval = 0;
int replylen;
char * buffer;

replylen   =  1 /* type          */ +
             10 /* serial        */ + 
             strlen(NULL_DEF_STRING(reply-> sysname)) +
             strlen(NULL_DEF_STRING(reply-> nodename)) +
             10 /* detail        */ + 
              4 /* delimiters    */ + 
              1 /* ending NULL   */;

buffer = (char *) XtMalloc(replylen);

(void) sprintf(buffer,"%c%10d%c%s%c%s%c%10d%c",
   type, reply-> serial,                                    DELIMITER, 
         NULL_DEF_STRING(reply-> sysname),                  DELIMITER, 
         NULL_DEF_STRING(reply-> nodename),                 DELIMITER, 
         reply-> detail,                                    DELIMITER);

XChangeProperty(dpy, client, XA_OL_WSM_REPLY(dpy), XA_STRING, 8, 
		PropModeReplace,
		(unsigned char *)(buffer),
		replylen);

return (retval);
	
} /* end of SendWSMReply */
/*
 * AcceptWSMReply
 *
 */

extern int AcceptWSMReply(dpy, client, type, reply)
Display *       dpy;
Window          client;
unsigned char * type;    /* Return */
WSM_Reply *     reply;   /* Return */
{
int len = 0;
char * buffer;

if (( buffer = GetCharProperty(
		dpy, client, XA_OL_WSM_REPLY(dpy), &len) ) != NULL)
   {
   *type            = buffer[0];
   reply-> serial   = atoi(_WM_parse(&buffer[1], DELIMITER));
   reply-> sysname  = SaveString(_WM_parse(NULL, DELIMITER));
   reply-> nodename = SaveString(_WM_parse(NULL, DELIMITER));
   reply-> detail   = atoi(_WM_parse(NULL, DELIMITER));
   XtFree(buffer);
   }

return (len);

} /* end of AcceptWSMReply */
extern void InitializeResourceBuffer()
{

if (ResourceChangeBuffer.Size == 0)
   {
   ResourceChangeBuffer.Buffer = (char *) XtMalloc(1);
   ResourceChangeBuffer.Size   = 1;
   ResourceChangeBuffer.Buffer[0] = '\0';
   }

if (ResourceDeleteBuffer.Size == 0)
   {
   ResourceDeleteBuffer.Buffer = (char *) XtMalloc(1);
   ResourceDeleteBuffer.Size   = 1;
   ResourceDeleteBuffer.Buffer[0] = '\0';
   }

} /* end of InitializeResourceBuffer */
extern int AppendToResourceBuffer(application, name, value)
char * application;
char * name;
char * value;
{
register int AdditionalLength;
int status;

if (application               == NULL || 
    name                      == NULL ||
    ResourceChangeBuffer.Size == 0    || 
    ResourceDeleteBuffer.Size == 0)
   status = 0;
else
   {
   status = 1;
   if (value == NULL)
      {
      AdditionalLength = strlen(application) + strlen(name) + 2;
      ResourceDeleteBuffer.Buffer = 
         (char *)XtRealloc(ResourceDeleteBuffer.Buffer, 
         ResourceDeleteBuffer.Size + AdditionalLength);
      (void) strcat(ResourceDeleteBuffer.Buffer, application);
      (void) strcat(ResourceDeleteBuffer.Buffer, name);
      (void) strcat(ResourceDeleteBuffer.Buffer, ":\n");
      ResourceDeleteBuffer.Size = ResourceDeleteBuffer.Size + AdditionalLength;
      }
   else
      {
      AdditionalLength = strlen(application) + strlen(name) + strlen(value) + 2;
      ResourceChangeBuffer.Buffer = 
         (char *)XtRealloc(ResourceChangeBuffer.Buffer, 
         ResourceChangeBuffer.Size + AdditionalLength);
      (void) strcat(ResourceChangeBuffer.Buffer, application);
      (void) strcat(ResourceChangeBuffer.Buffer, name);
      (void) strcat(ResourceChangeBuffer.Buffer, ":");
      (void) strcat(ResourceChangeBuffer.Buffer, value);
      (void) strcat(ResourceChangeBuffer.Buffer, "\n");
      ResourceChangeBuffer.Size = ResourceChangeBuffer.Size + AdditionalLength;
      }
   }
   
return (status);

} /* end of AppendToResourceBuffer */
extern void SendResourceBuffer(dpy, client, serial, name)
Display * dpy;
Window    client;
int       serial;
char *    name;
{
WSM_Request request;

request.serial   = serial;
request.name     = name;
request.sysname  = NULL;
request.nodename = NULL;
request.reserved = NULL;

if (ResourceChangeBuffer.Size > 1)
   {
   request.command = ResourceChangeBuffer.Buffer;
   EnqueueWSMRequest(dpy, client, WSM_MERGE_RESOURCES, &request);
   XtFree(ResourceChangeBuffer.Buffer);
   ResourceChangeBuffer.Size = 0;
   }
if (ResourceDeleteBuffer.Size > 1)
   {
   request.command = ResourceDeleteBuffer.Buffer;
   EnqueueWSMRequest(dpy, client, WSM_DELETE_RESOURCES, &request);
   XtFree(ResourceDeleteBuffer.Buffer);
   ResourceDeleteBuffer.Size = 0;
   }

} /* end of SendResourceBuffer */

/*
 * _WM_parse
 *
 * This function is used to _WM_parse a given string using a given delimiter.
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

static char * _WM_parse(string, delimiter)
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

} /* end of _WM_parse */

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
