/* XOL SHARELIB - start */
/* This header file must be included before anything else */

/* This file does not exist in /usr/include/X11/Xol */
/*
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
*/
/* XOL SHARELIB - end */

#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/WSMcomm.c	1.1"
#endif

/*
 * WSMcomm.c
 *
 */

#include <stdio.h>
#include <string.h>
#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "WSMcomm.h"

typedef struct StringBuffer {
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

/*
 * ClearWSMQueue
 *
 */

void
ClearWSMQueue(Display *dpy)
{
	XDeleteProperty(dpy, DefaultRootWindow(dpy), XA_WSM_QUEUE(dpy));
}

/*
 * GetCharProperty (generic routine to get a char type property)
 */

static char *
GetCharProperty(Display * dpy, Window w, Atom property, int * length)
{
	Atom			actual_type;
	int			actual_format;
	unsigned long		num_items;
	unsigned long		bytes_remaining = 1;
	char *                  buffer;
	char *                  Buffer;
	int			Buffersize = 0;
	int			Result;
	int			EndOfBuffer;
	register		i;

	Buffer = (char *) malloc(1);
	Buffer[0] = '\0';
	EndOfBuffer = 0;
	do {
		if (
			(Result = XGetWindowProperty(
				dpy, w, property, 
				(long) ((Buffersize+3) /4),
				(long) ((bytes_remaining+3) / 4), True,
				XA_STRING, &actual_type, &actual_format, 
				&num_items, &bytes_remaining,
				(unsigned char **) &buffer
			)) != Success
		) {
			  if (buffer) {
				free(buffer);
			  }
			  if (Buffer) {
				free(Buffer);
			  }
			  *length = 0;
			  return NULL;
		}
		
		if (buffer) {
			register int	i;

			Buffersize += num_items;
			Buffer = (char *) realloc(Buffer, Buffersize + 1);
			if (Buffer == NULL) {
				free(buffer);
				*length = 0;
				return NULL;
			}
			for (i = 0; i < num_items; i++) {
				Buffer[EndOfBuffer++] = buffer[i];
			}
			Buffer[EndOfBuffer] = '\0';
			free(buffer);
		}
	} while (bytes_remaining > 0);

	*length = Buffersize;
	if (Buffersize == 0 && Buffer != NULL) {
		free (Buffer);
		return NULL;
	}
	else {
		return (Buffer);
	}
}

/*
 * DequeueWSMRequest
 *
 */

int
DequeueWSMRequest(
	Display *dpy, Window * client, unsigned char * type, 
	WSM_Request * request
)
{
	static char *	buffer = NULL;
	static int	bufferlen = 0;
	static char *	start  = NULL;

	if ((buffer != NULL) && ((start - buffer) >= bufferlen)) {
		XtFree(buffer);
		buffer = start = NULL;
		bufferlen = 0;
	}

	if (buffer == NULL) {
		if (
			(buffer = (char *)GetCharProperty(
				dpy, DefaultRootWindow(dpy),
				XA_WSM_QUEUE(dpy), &bufferlen
			)) == NULL
		) {
			return QEMPTY;
		}
		else {
			start  = buffer;
		}
	}

	*type             = start[0];
	*client           = atoi(_WM_parse(&start[1], DELIMITER));
	request->serial   = atoi(_WM_parse(NULL, DELIMITER));
	request->sysname  = _WM_parse(NULL, DELIMITER);
	request->nodename = _WM_parse(NULL, DELIMITER);
	request->name     = _WM_parse(NULL, DELIMITER);
	request->uid      = (unsigned short) atoi(_WM_parse(NULL, DELIMITER));
	request->gid      = (unsigned short) atoi(_WM_parse(NULL, DELIMITER));
	request->command  = _WM_parse(NULL, DELIMITER);
	start             = _WM_parse(NULL, DELIMITER);
	request->reserved = start;
	start            += (strlen(request-> reserved) + 2);

	return GOTREQUEST;

}

/*
 * SendWSMReply
 *
 */

int
SendWSMReply(
	Display *dpy, Window client, unsigned char type, WSM_Reply * reply
)
{
	int	retval = 0;
	int	replylen;
	char *	buffer;

	replylen = (
		1 /* type          */ +
		10 /* serial        */ + 
		strlen(NULL_DEF_STRING(reply->sysname)) +
		strlen(NULL_DEF_STRING(reply->nodename)) +
		10 /* detail        */ + 
		4 /* delimiters    */ + 
		1 /* ending NULL   */
	);

	buffer = (char *) XtMalloc(replylen);

	(void) sprintf(
		buffer,"%c%10d%c%s%c%s%c%10d%c",
		type, reply-> serial, DELIMITER, 
		NULL_DEF_STRING(reply-> sysname), DELIMITER, 
		NULL_DEF_STRING(reply-> nodename), DELIMITER, 
		reply-> detail, DELIMITER
	);

	XChangeProperty(
		dpy, client, XA_WSM_REPLY(dpy), XA_STRING, 8, PropModeReplace,
		(unsigned char *)(buffer), replylen
	);

	return retval;

}

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

static char *
_WM_parse(char * string, int delimiter)
{
	static char *	 x = NULL;
	register char *	 y;

	if (string) {
		x = string;
	}
	string = x;

	if (x == NULL || *x == '\0') {
		return NULL;
	}
	else {
		if ((y = strchr(x, delimiter)) == NULL) {
			x = NULL;
		}
		else {
			*y = '\0';
			x = y + 1;
		}
		return string;
	}

}
