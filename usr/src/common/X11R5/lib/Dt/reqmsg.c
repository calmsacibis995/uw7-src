/*		copyright	"%c%" 	*/

#ifndef NOIDENT
#ident	"@(#)Dt:reqmsg.c	1.14"
#endif

/* 
 * reqmsg.c
 *
 * This file contains routines that are used typically by Desktop clients to
 * send requests and receive replies from the Desktop Manager.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include "DesktopI.h"


static long ticket = 1;

int
DtEnqueueStrRequest(scrn, queue, my_queue, client, str, len)
Screen		*scrn;
Atom		queue;
Atom		my_queue;
Window		client;
char		*str;
int		len;
{
#ifdef OLD_REQ_MECHANISM
	/* send string to window */
	Dt__EnqueueCharProperty(DisplayOfScreen(scrn), RootWindowOfScreen(scrn),
			        queue, str, len);
#else
	XChangeProperty(DisplayOfScreen(scrn), client, my_queue, XA_STRING, 8,
			PropModeReplace, (unsigned char *)str, len);

	XConvertSelection(DisplayOfScreen(scrn), queue,
			  _DT_QUEUE(DisplayOfScreen(scrn)),
			  my_queue, client, CurrentTime);
	XFlush(DisplayOfScreen(scrn));
#endif
	return(ticket - 1);
}

/*
 * DtEnqueueRequest
 * ----------------
 * The DtEnqueueRequest function sends a request to the selection owner
 * queue. The content of the request is put into the my_queue property
 * hanging off of the client window.
 */
int
DtEnqueueRequest(scrn, queue, my_queue, client, request)
Screen		*scrn;
Atom		queue;
Atom		my_queue;
Window		client;
DtRequest 	*request;
{
	char *str;
	int len;
	DtMsgInfo const *mp;
	int mlen;
	struct utsname unames; /* must define here */

	/* fill in struct header */
	if (request->header.nodename == NULL) {
		/* get system name */
		(void)uname(&unames);

		request->header.nodename = unames.nodename;
	}
	request->header.serial = ticket++;
	request->header.client = client;
	request->header.version= DT_VERSION;

	/* check for valid msg function */
	switch(request->header.rqtype >> MSGFUNC_SHIFT) {
	case DTM_MSG:
		mp = Dt__dtm_msgtypes;
		mlen = DT_DTM_NUM_REQUESTS;
		break;
	case WB_MSG:
		mp = Dt__wb_msgtypes;
		mlen = DT_WB_NUM_REQUESTS;
		break;
	case HELP_MSG:
		mp = Dt__help_msgtypes;
		mlen = DT_HELP_NUM_REQUESTS;
		break;
	default:
		/* bad msg type */
		return(-1);
	}

	if ((str = Dt__StructToString(request, &len, mp, mlen)) == NULL)
		return(-1);

	return(DtEnqueueStrRequest(scrn, queue, my_queue, client, str, len));
} /* end of DtEnqueueRequest */

/*
 * DtAcceptReply
 * -------------
 * The DtAcceptReply function is used to retrieve a reply posted on
 * a clients window by the Desktop Manager.
 */
int
DtAcceptReply(scrn, my_queue, client, reply)
Screen		*scrn;
Atom		my_queue;
Window		client;
DtReply 	*reply;
{
	unsigned long len;
	int i;
	char *rpname;
	char *buffer;
	char *p;
	DtMsgInfo const *mp;

	if ((buffer = Dt__GetCharProperty(DisplayOfScreen(scrn), client,
		my_queue, &len)) == NULL)
		return(-1);
	
	/* map reply string to structure */
	if ((len < 4) || (buffer[0] != '@') || (buffer[len-1] != '\0')) {
		free(buffer);
		return(-1); /* BAD format */
	}

	rpname = buffer + 1; /* skip '@' */
	if ((p = strchr(rpname, ':')) == NULL) {
		free(buffer);
		return(-1); /* BAD format */
	}

	*p = '\0'; /* temporary switch */

	/* find request type */
	for (mp=Dt__dtm_replytypes, i=DT_DTM_NUM_REPLIES; i; i--, mp++)
		if (!strcmp(rpname, mp->name))
			break;
	if (i == 0) {
		for (mp=Dt__wb_replytypes, i=DT_WB_NUM_REPLIES; i; i--, mp++)
			if (!strcmp(rpname, mp->name))
				break;
	}

	if (i == 0) {
		for (mp=Dt__help_replytypes,i=DT_HELP_NUM_REPLIES; i; i--,mp++)
			if (!strcmp(rpname, mp->name))
				break;
	}

	if (i == 0)
		return(-1); /* unknown msg name */

	reply->header.rptype = mp->type;
	if (Dt__DecodeFromString((char *)reply, mp, p + 1, NULL) == -1) {
		free(buffer);
		return(-1); /* BAD format */
	}

	free(buffer);
	return (0);
} /* end of DtAcceptReply */

