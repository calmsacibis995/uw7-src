#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/WSMcomm.h	1.2"
#endif

#ifndef _WSMcomm_h
#define _WSMcomm_h

#define QEMPTY			0
#define GOTREQUEST		1

#define WSM_EXECUTE		1
#define WSM_TERMINATE		2
#define WSM_SAVE_YOURSELF	3
#define WSM_EXIT		4
#define WSM_MERGE_RESOURCES	5
#define WSM_DELETE_RESOURCES	6
#define WSM_DELETE_WINDOW	7

#define WSM_SUCCESS 		1
#define WSM_FORK_FAILURE	2
#define WSM_EXEC_FAILURE	3
#define WSM_NONE                42
#define WSM_NORMAL		1001
#define WM_PART_NONE            2001
#define WM_PART_HEADER          2002
#define WM_PART_BACKGROUND      2003

/* Define replacements for OlScreenXXToPixel Stuff */
#define ScreenPointToPixel(x,y,z) _XmConvertUnits(z, x, Xm100TH_POINTS, \
            (y * 100), XmPIXELS)
#define ScreenMMToPixel(x,y,z) _XmConvertUnits(z, x, Xm100TH_MILLIMETERS, \
            (y * 100), XmPIXELS)

/*
 ************************************************************************
 *	Define Constant token for naming the locale definition file.
 ************************************************************************
 */
#define OL_LOCALE_DEF			"ol_locale_def"

/* These are bit masks used in Vendor's XtNwmProtocolMask resource.*/
#if !defined(__STDC__) && !defined(__cplusplus) && !defined(c_plusplus)
#include <limits.h>
#define _CHAR_BIT    CHAR_BIT
#else
#define _CHAR_BIT    8
#endif

#define _HI_BIT() (unsigned long)((unsigned long)1 << (sizeof(long) * _CHAR_BIT - 1))
#define _WM_GROUP(X) (_HI_BIT() >> (X))
#define _WM_GROUPALL()       (_WM_GROUP(0))
#define _WM_BITALL()         (~(_WM_GROUP(0)))
#ifndef WM_DELETE_WINDOW
#define WM_DELETE_WINDOW     (_WM_GROUP(0) | ((unsigned long)1 << 0))
#endif
#ifndef WM_SAVE_YOURSELF
#define WM_SAVE_YOURSELF     (_WM_GROUP(0) | ((unsigned long)1 << 1))
#endif
#ifndef WM_TAKE_FOCUS
#define WM_TAKE_FOCUS        (_WM_GROUP(0) | ((unsigned long)1 << 2))
#endif

#define _WM_TESTBIT(X, B)	(((X & _WM_GROUPALL()) & \
				(B & _WM_GROUPALL())) ? \
				((X & _WM_BITALL()) & \
				(B & _WM_BITALL())) : 0)

/* OlClientsP.h */

#define DEF_STRING(s,d)	        (s == NULL ? d : s)
#define NULL_DEF_STRING(s)      DEF_STRING(s,"")
/* End of OlClientsP.h */

/* OlClients.h */

#ifdef DELIMITER
#undef DELIMITER
#endif

#define DELIMITER       0x1f

/*
 * Following for WMState structure
 */

/*
 * for compatiblity with earlier software
 */

#define XA_WM_DELETE_WINDOW(d)  XInternAtom((d), "WM_DELETE_WINDOW", False)
#define XA_BANG(d)              XInternAtom((d), "BANG", False)
#define XA_WM_SAVE_YOURSELF(d)  XInternAtom((d), "WM_SAVE_YOURSELF", False)
				 
typedef struct
{
	int    serial;
	char * command;
	char * name;
	char * sysname;
	char * nodename;
	char * reserved;
	unsigned short uid;
	unsigned short gid;
} WSM_Request;

typedef struct
{
	int    serial;
	int    detail;
	char * sysname;
	char * nodename;
} WSM_Reply;

#define XA_WSM_QUEUE(d)	XInternAtom(d, "_OL_WSM_QUEUE", False)

#define XA_WSM_REPLY(d)	XInternAtom(d, "_WSM_REPLY", False)
/* End of OlClients.h */

extern void ClearWSMQueue(Display *);
extern int DequeueWSMRequest(
	Display *, Window *, unsigned char *, WSM_Request *
);
extern int SendWSMReply(Display *, Window, unsigned char, WSM_Reply *);

#endif
