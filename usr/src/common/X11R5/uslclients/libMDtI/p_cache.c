#ifndef NOIDENT
#ident	"@(#)libMDtI:p_cache.c	1.18"
#endif

#include <stdio.h>
#include <unistd.h>
#include <X11/Intrinsic.h>
#include <xpm.h>
#include "DesktopP.h"
#include "default.icon"
#include "DtStubI.h"

#define PIXMAP_STR	"pixmaps"
#define PIXMASK_STR	"pixmasks"
#define CURSOR_STR	"bitmaps"
#define CURMASK_STR	"bitmasks"

static char dflt_iconpath[] = "/usr/X/lib/locale/%L/%T/%N:/usr/X/lib/%T/%N";
static char *IconPath = dflt_iconpath;

static DmGlyphPtr
Dm__CreateDfltPixmap(screen)
Screen *screen;
{
	DmGlyphPtr gp;

	if (gp = (DmGlyphPtr)DtsGetData(screen, DM_CACHE_PIXMAP, NULL, 0)) {
		gp->count++;
		return(gp);
	}

	if (gp = (DmGlyphPtr)malloc(sizeof(DmGlyphRec))) {
		gp->pix = XCreatePixmapFromBitmapData(DisplayOfScreen(screen),
				RootWindowOfScreen(screen),
				(char *)default_bits,
				default_width, default_height,
				BlackPixelOfScreen(screen),
				WhitePixelOfScreen(screen),
				DefaultDepthOfScreen(screen));
		gp->count = 1;
		gp->path  = NULL;
		gp->depth = DefaultDepthOfScreen(screen);
		gp->width = (Dimension)default_width;
		gp->height= (Dimension)default_height;
		if (Dm__CreateIconMask(screen, gp) == NULL) {
			XFreePixmap(DisplayOfScreen(screen), gp->pix);
			free(gp);
			return(NULL);
		}
		DtsPutData(screen, DM_CACHE_PIXMAP, NULL, 0, (void *)gp);
	}

	return(gp);
}

/*
 * This routine will set the icon path.
 * It can be called more than once. Especially after the ICONPATH property
 * has changed.
 */
void
DmSetIconPath(path)
char *path;
{
	register char *p, *p2;
	int len;
	int dflt_len;

	if (IconPath && (IconPath != dflt_iconpath))
		free(IconPath);

	if (!path || (*path == '\0')) {
		IconPath = dflt_iconpath;
		return;
	}

	p2 = path = Dm__expand_sh(path, NULL, NULL);
	len = strlen(path);
	dflt_len = strlen(dflt_iconpath);
	p = (char *)malloc(len + dflt_len + 2);
	IconPath = p;
	if (*p2 == ':') {
		strcpy(p, dflt_iconpath);
		p += dflt_len;
	}

	while (*p2) {
		if ((*p2 == ':') && (*(p2+1) == ':')) {
			*p++ = *p2++;
			strcpy(p, dflt_iconpath);
			p += dflt_len;
		}

		*p++ = *p2++;
	}

	if (*(p-1) == ':') {
		strcpy(p, dflt_iconpath);
		p += dflt_len;
	}

	*p = '\0';
}

#ifdef CHECK_FILES
static Bool
chkfiles(filename)
String filename;
{
	struct stat status;

	return((access(filename, R_OK|X_OK) == 0) &&
		(stat(filename, &status) == 0) &&
		((status.st_mode & S_IFDIR) == S_IFDIR));
}
#endif

static DmGlyphPtr
Dm__GetPixmap(screen, name, type, masktype)
Screen *screen;
char *name;		/* pixmap filename */
char *type;
char *masktype;
{
	Display *dpy = DisplayOfScreen(screen);
	DmGlyphPtr gp = NULL;
	Pixmap pix;
	int len;
	unsigned int width, height, depth;
	int junk;
	char *fullpath;
	int foundname = 0;

	if (name == NULL)
		return(Dm__CreateDfltPixmap(screen));

	if (*name != '/') {
		/* compose full path */
		fullpath = XtResolvePathname(dpy, type,
				name, NULL, IconPath, NULL, 0,
#ifdef CHECK_FILES
				(XtFilePredicate)chkfiles);
#else
				(XtFilePredicate)NULL);
#endif

		if (!fullpath)
			return(NULL);
		foundname++;
	}
	else
		fullpath = name;

	if (access(fullpath, R_OK))
		goto bye;

	len = strlen(fullpath) + 1;
	if (gp = DtsGetData(screen, DM_CACHE_PIXMAP, (void *)fullpath, len)) {
		if ((*type == 'p') && (gp->depth == 1))
			return(NULL);
		else {
			gp->count++; /* bump usage count */
			goto bye;
		}
	}

	if ((*type == 'b') || DtsXReadPixmapFile(dpy,RootWindowOfScreen(screen),
					      DefaultColormapOfScreen(screen),
					      fullpath, &width, &height,
					      DefaultDepthOfScreen(screen),
					      &pix) != PixmapSuccess) {

		if (XReadBitmapFile(dpy, RootWindowOfScreen(screen),
				fullpath, &width, &height,
				&pix, &junk, &junk) == BitmapSuccess) {
			depth = 1;
		}
		else
			goto bye;
	}
	else
		depth = DefaultDepthOfScreen(screen);

	if ((gp = (DmGlyphPtr)malloc(sizeof(DmGlyphRec))) == NULL) {
		XFreePixmap(dpy, pix);
		goto bye;
	}

	gp->path  = strdup(fullpath);
	gp->count = 1;
	gp->pix   = pix;
	gp->depth = depth;
	gp->width = (Dimension)width;
	gp->height= (Dimension)height;
	if (foundname) {
		free(fullpath);
		fullpath = XtResolvePathname(dpy, masktype, name, NULL,
				IconPath, NULL, 0,
#ifdef CHECK_FILES
				(XtFilePredicate)chkfiles);
#else
				(XtFilePredicate)NULL);
#endif
		if (fullpath) {
			if (access(fullpath, R_OK) != 0) {
				foundname = 0;
				free(fullpath);
				fullpath = NULL;
			}
		}
		else
			foundname = 0;
	}

	if (foundname) {
		if (XReadBitmapFile(dpy, RootWindowOfScreen(screen),
				fullpath, &width, &height,
				&(gp->mask), &junk, &junk) == BitmapSuccess) {
			if ((width != gp->width) || (height != gp->height)) {
				fprintf(stderr, "icon mask dimension %s(%dx%d)"
				" does not match icon dimension %s(%dx%d)\n",
				 fullpath, width, height,
				 gp->path, gp->width, gp->height);
				XFreePixmap(dpy, gp->mask);
				foundname = 0;
			}
		}
		else
			foundname = 0;

		free(fullpath);
		fullpath = NULL;
	}

	if (!foundname) {
		if (Dm__CreateIconMask(screen, gp) == NULL) {
			XFreePixmap(dpy, pix);
			free(gp);
			return(NULL);
		}
	}

	DtsPutData(screen, DM_CACHE_PIXMAP, gp->path, len, gp);
bye:
	if (foundname && fullpath)
		free(fullpath);
	return(gp);
}

DmGlyphPtr
DmGetPixmap(screen, name)
Screen *screen;
char *name;		/* pixmap filename */
{
	if (DefaultDepthOfScreen(screen) == 1)
		return(Dm__GetPixmap(screen, name, CURSOR_STR, CURMASK_STR));
	else
		return(Dm__GetPixmap(screen, name, PIXMAP_STR, PIXMASK_STR));
}

DmGlyphPtr
DmGetCursor(screen, name)
Screen *screen;
char *name;		/* pixmap filename */
{
	return(Dm__GetPixmap(screen, name, CURSOR_STR, CURMASK_STR));
}

DmGlyphPtr
DmCreateBitmapFromData(screen, name, data, width, height)
Screen *screen;
char *name;		/* bitmap name */
unsigned char *data;
unsigned int width;
unsigned int height;
{
	Pixmap pix;
	DmGlyphPtr gp;
	int len = strlen(name)+1;

	if (gp = DtsGetData(screen, DM_CACHE_PIXMAP, (void *)name, len)) {
		gp->count++; /* bump usage count */
		return(gp);
	}

	pix = XCreateBitmapFromData(DisplayOfScreen(screen),
			RootWindowOfScreen(screen),
			(char *)data, width, height);

	if ((gp = (DmGlyphPtr)malloc(sizeof(DmGlyphRec))) == NULL) {
		XFreePixmap(DisplayOfScreen(screen), pix);
		return(NULL);
	}

	gp->path  = strdup(name);
	gp->count = 1;
	gp->pix   = pix;
	gp->depth = 1;
	gp->width = (Dimension)width;
	gp->height= (Dimension)height;
	if (Dm__CreateIconMask(screen, gp) == NULL) {
		XFreePixmap(DisplayOfScreen(screen), pix);
		free(gp);
		return(NULL);
	}

	DtsPutData(screen, DM_CACHE_PIXMAP, gp->path, len, gp);
	return(gp);
}

void
DmReleasePixmap(screen, gp)
Screen *screen;
DmGlyphPtr gp;
{
	if (gp && --(gp->count) == 0) {
		/* free the resource as well */
		if (gp->path) {
			DtsDelData(screen, DM_CACHE_PIXMAP, gp->path,
					strlen(gp->path) + 1);
			free(gp->path);
		}
		else
			DtsDelData(screen, DM_CACHE_PIXMAP, NULL, 0);
		XFreePixmap(DisplayOfScreen(screen), gp->pix);
		XFreePixmap(DisplayOfScreen(screen), gp->mask);
	}
}
