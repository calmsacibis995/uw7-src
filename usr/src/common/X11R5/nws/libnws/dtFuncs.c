#ident	"@(#)dtFuncs.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* 
 * dtFuncs.c
 */

#include <stdio.h>
#include <stdlib.h>	/*  for malloc()  */
#include <string.h>	/*  for strchr()  */
#include <unistd.h>	/*  for gettxt()  */

#include <Xm/Xm.h>	
#include "dtFuncs.h"	/* for help structure etc. */


#ifdef __cplusplus
extern "C" XReadPixmapFile(Display *, Drawable, Colormap, char *, 
                          unsigned int *, unsigned *, unsigned int, Pixmap *);
#else
extern XReadPixmapFile(Display *, Drawable, Colormap, char *, 
                          unsigned int *, unsigned * ,unsigned int, Pixmap *);
#endif	/*  __cplusplus */


/* 
 * Default path of pixmap files
 */
#define	PIXMAP_PATH	"/usr/X/lib/pixmaps/"	


/*
 *
 *  FUNCTION:
 *	char	*getStr (char *msgId)
 *
 *  DESCRIPTION:
 *	Get a localized string from a message file.  msgIds contain the filename
 *	(limited to 14 characters), a ':' character, a msgNumber which is an index
 *	of the string in the database, and the string itself. If strchr does not
 *  find an FS_CHAR, we assume the msgId string does not come from a message
 *  file, and we simply return the msgId string.
 *
 *  RETURN:
 *	A pointer to just the string portion of the message file entry.
 */
char *
getStr (char *msgId)
{
    char	*sep;
    char	*str = msgId;

    sep = (char *)strchr (msgId, FS_CHAR);
	if (sep)
	{
    	*sep = '\0';
	
    	str = (char *)gettxt (msgId, sep + 1);
    	*sep = FS_CHAR;
	}
    return (str);
}	/*	End  getStr () */

/*
 *
 *  FUNCTION:
 *	Boolean DtamIsOwner (Display *display)
 *
 *  DESCRIPTION:
 *  Validate owner privileges by using tfadmin
 *
 *  Duplicate of dtamlib/owner.c bu with the leading undescore removed.
 *  I decided it is better to duplicate here rather than link in another
 *  static library for this one routine.
 *
 *  RETURN:
 *	True on succesful exit status of tfadmin
 *	False on unsuccesful exit status of tfadmin
 *
 */
Boolean
DtamIsOwner (Display *display)
{
	String	execName, className;
	char    buf[256];

	XtGetApplicationNameAndClass (display, &execName, &className);

	sprintf(buf, "/sbin/tfadmin -t %s 2>/dev/null", execName);
	return (system (buf) == 0);

}	/*  End  DtamIsOwner () */

/*
 *
 *  FUNCTION:
 *	void	getIconPixmap (Display *display, char *iconName, Pixmap *pixmap)
 *
 *  DESCRIPTION:
 *	Given the display and icon name, create a pixmap using the desktop library 
 *	pixmap read file function.  Put it in the place pointed to by "pixmap".
 *	Failure is not announced.
 *
 *  RETURN:
 *	A pixmap in the "pixmap" location.
 */
void
getIconPixmap (Display *display, char *iconName, Pixmap *pixmap)
{
	int		screen;
	Window		root;
	char		*fullPath;
	unsigned int	pixmapHeight, pixmapWidth;

	screen   = DefaultScreen (display);
	root     = RootWindow (display, screen);
	fullPath = (char *)XtMalloc (strlen (PIXMAP_PATH) + strlen (iconName) + 1);

	*pixmap = 0;
	if (fullPath != NULL)
	{
		strcpy (fullPath, PIXMAP_PATH);
		strcat (fullPath, iconName);

		if (XReadPixmapFile (display ,root, XDefaultColormap (display, screen),
					fullPath, &pixmapWidth, &pixmapHeight,
				XDefaultDepth (display, screen), pixmap))
		{
			*pixmap = 0;
		}
		XtFree (fullPath);
	}
}	/*  End  getIconPixmap () */

/*
 *
 *  FUNCTION:
 *	void setIconPixmap (Widget topLevel, 
 *                      Pixmap *pixmap, unsigned char *iconLabel)
 *  DESCRIPTION:
 *	To set the applications icon pixmap and name.
 *	Uses the desktop library pixmap read file routine to get a pixmap 
 *  for the application. Failure to get the specified pixmap is silently 
 *  ignored.
 *
 *  RETURN:
 */
void
setIconPixmap (Widget topLevel, Pixmap *pixmap, char *iconLabel)
{
	XtVaSetValues (topLevel, XmNiconPixmap, *pixmap,
					XtNiconName, (XtArgVal)iconLabel,
					NULL);
}	/*  End  setIconPixmap () */


/*
 *
 *  FUNCTION:
 *	void	displayHelp (Widget topLevel, HelpText *helpInfo)
 *  DESCRIPTION:
 *	Send a message to dtm to display a help window.  If help is NULL, then
 *	ask dtm to display the help desk.
 *
 *  RETURN:
 *	Nothing.
 */

void
displayHelp (Widget topLevel, HelpText *helpInfo)
{
}	/*  End displayHelp () */

/*
 *
 *
 *  FUNCTION:
 *  void createCursor(Display *display, unsigned int shape)
 *
 *  DESCRIPTION:
 *  Create and return the argumented cursor.
 *
 *  RETURN:
 *  Cursor on success, 0 on failure.
 */
Cursor
createCursor(Display *display, unsigned int shape)
{
	Cursor cursor;

	cursor = XCreateFontCursor (display, shape);
	if (cursor == BadAlloc || cursor == BadFont || cursor == BadValue)
	{
		cursor = 0;
	}
	return (cursor);
}

/*
 *
 *
 *  FUNCTION:
 *  void setWindowCursor(Cursor cursor, Display *dsp, Window win, Boolean flush)
 *
 *  DESCRIPTION:
 *  Define the windows cursor to be the argumented cursor, and make the 
 *  Xserver do it NOW if flush is True.
 *
 *  RETURN:
 *  Nothing.
 *
 */
void
setWindowCursor(Cursor cursor, Display *dsp, Window win, Boolean flush)
{
	XDefineCursor (dsp, win, cursor);
	if (flush)
		XFlush (dsp);
}

/*
 *
 *
 *  FUNCTION:
 *  void unSetWindowCursor(Display *dsp, Window win, Boolean flush)
 *
 *  DESCRIPTION:
 *  Unset the cursor that a window has defined upon it and do it NOW if
 *  flush is True.  This will cause the window to revert back to it's
 *  parents cursor, reverses the effect of a XDefineCursor.
 *
 *  RETURN:
 *  Nothing.
 *
 */
void
unSetWindowCursor(Display *dsp, Window win, Boolean flush)
{
	XUndefineCursor (dsp, win);
	if (flush)
		XFlush (dsp);
}
