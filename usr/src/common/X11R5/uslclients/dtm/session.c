#pragma ident	"@(#)dtm:session.c	1.24"

#include <errno.h>
#include <limits.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/*
 * Session file format:
 *
 * *F=geoemtry,state,path		folder window
 * *A=geometry,state,command		application window
 * *W=geometry,state,command		wastebasket window
 */

/*
 * This routine will read the specified session file and reopen
 * all the folder windows, and restart all the
 * applications that were there before.
 *
 * returns:	0	if error or no session info file
 *		1	if the previous session was started successfully
 */
int
DmRestartSession(path)
char *path;
{
	FILE *f;
	char type, state;
	int ret;
	int line = 1;
	char geometry[64];
	char str[PATH_MAX];
	char *geom_ptr;
	Boolean iconic;
	static const char * const format="*%c=%s ,%c,%[^\n]\n";
	

	if ((f = fopen(path, "r")) == NULL) {
		if (errno != ENOENT)
			Dm__VaPrintMsg(TXT_SESSION_OPEN, path);
		return(0);
	}

	while ((ret = fscanf(f,format,&type,geometry,&state,str)) == 4) {
		iconic = (state == 'i') ? True : False;

		/*
		 * Because of the fix format for each line, one cannot skip
		 * fields. So, geometry of "*" means don't care.
		 */
		if (!strcmp(geometry, "*"))
			geom_ptr = NULL;
		else
			geom_ptr = geometry;

		switch(type) {
		case 'F':
			DmOpenFolderWindow(str, 0, geom_ptr, iconic);
			break;
		case 'W':
			DmInitWasteBasket(geom_ptr, iconic,
					  (state == 'w') ? False : True);
			break;
		case 'H':
			DmInitHelpDesk(geom_ptr, iconic,
					  (state == 'w') ? False : True);
			break;
		case 'A':
			/* application, not implemented yet */
			break;
		default:
			Dm__VaPrintMsg(TXT_SESSION_SYNTAX, line);
		}
		line++;
	}
	(void)fclose(f);

	if (ret != EOF)
		Dm__VaPrintMsg(TXT_SESSION_SYNTAX, line);
	return(1);
}

static void
Dm__PrintLine(f, type, dwp)
FILE *f;
char type;
DmWinPtr dwp;
{
	if (dwp) {
		Position x, y;
		XWindowAttributes attrs;
		int win_state;
		Display *dpy = XtDisplay(dwp->shell);
		Window win = XtWindow(dwp->shell);
		Window root, parent;
		Window *children;
		unsigned int num;
		int dx, dy;
		int w = 0, h;
	
		/*
		 * To check if a window is in NormalState, you must check
		 * both win_state and map_state. In the case if a window
		 * was realized but never mapped, GetWMState() returns
		 * NormalState even though the window is not mapped.
		 */
		XtTranslateCoords(dwp->shell, 0, 0, &x, &y);
		win_state = GetWMState(dpy, win);
		dx = dy = 0;

		/* don't save info if withdrawn */
		if (win_state == WithdrawnState)
			return;

		/*
		 * Loop through all the parents until root window to find
		 * the "thickness" of the window decoration.
		 */
		do {
			if (!XGetWindowAttributes(dpy, win, &attrs))
				return;
			if (!XQueryTree(dpy, win, &root, &parent, &children,
				 &num))
				return;

			if (!w) {
				w = attrs.width;
				h = attrs.height;
			}

			free(children);
			if (parent != root) {
				dx += attrs.x;
				dy += attrs.y;
				win = parent;
			}
			else
				break;
		} while (1);

		x -= dx;
		y -= dy;
		if (x < 0) x = 0;
		if (y < 0) y = 0;

		/*
		 * don't bother saving a window that have 0x0 geometry.
		 */
		if (attrs.width && attrs.height) {
			fprintf(f, "*%c=%dx%d%+d%+d ,%c,%s\n",
				type, w, h, x, y,
				(win_state == IconicState) ? 'i' :
				(((win_state == NormalState) &&
				  (attrs.map_state != IsUnmapped))? 'n' : 'w'),
				dwp->views[0].cp->path);
		}
	}
}

static int
CatchBadWindow(dpy, event)
Display *dpy;
XErrorEvent *event;
{
	/* ignore error */
	return(0);
}

void
DmSaveSession(path)
char *path;
{
	int	win_state;
	DmWinPtr dwp;
	FILE *f;

	if ((f = fopen(path, "w")) == NULL) {
		Dm__VaPrintMsg(TXT_SESSION_SAVE);
		return;
	}

	/* Ignore bad window errors for XGetWindowAttributes(). */
	XSync(XtDisplay(DESKTOP_SHELL(Desktop)), False);
	XSetErrorHandler(CatchBadWindow);

	/* folder windows */
	for (dwp=(DmWinPtr)DESKTOP_FOLDERS(Desktop); dwp; dwp=dwp->next)
		Dm__PrintLine(f, 'F', dwp);

	/* waste basket */
	Dm__PrintLine(f, 'W', (DmWinPtr)DESKTOP_WB_WIN(Desktop));

	/* help desk */
	Dm__PrintLine(f, 'H', (DmWinPtr)DESKTOP_HELP_DESK(Desktop));

	XSync(XtDisplay(DESKTOP_SHELL(Desktop)), False);
	XSetErrorHandler(NULL);
	fclose(f);
}

