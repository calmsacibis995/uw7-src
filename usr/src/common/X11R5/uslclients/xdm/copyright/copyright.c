#ident	"@(#)xdm:copyright/copyright.c	1.4"

#include <stdio.h>
#include <X11/Xlib.h>

/*
 * use a nice 8 pt font
 */

#define FONT_TO_USE "fixed"

/*
 * treshold of gray scale
 */

#define THRESHOLD   (unsigned long)  32000

/*
 * seconds to sleep before clearing root window and exiting
 */

#define SLEEP_AMOUNT      5  

/*
 * Compile as either a standalone client or 
 * as an extern function to be called from another client.
 */

#define STAND_ALONE

#ifdef STAND_ALONE           /* is a standalone process                    */

#      define Widget       int
#      define XtDisplay(x) XOpenDisplay(NULL)
#      define XtWindow(x)  DefaultRootWindow(dpy)
#      define XtScreen(x)  DefaultScreenOfDisplay(dpy)

#else                        /* if not integrated into another client      */

#endif /* STANDALONE */

extern void DisplayCopyrightNotice(Widget handleRoot);
extern char *getenv();

/* 
 * main
 *
 */

#ifdef STAND_ALONE

main(int argc, char * argv[])
{
   Widget handleRoot;

   if (fork() != 0)
      exit(0);

   DisplayCopyrightNotice(handleRoot);

   exit(0);

} /* end of main */

#else

   /*
    * main is elsewhere
    */

#endif /* end of STAND_ALONE */
/*
 * DisplayCopyrightNotice
 *
 */

extern void
DisplayCopyrightNotice(Widget handleRoot)
{
   int           copyright_sleep_amount = SLEEP_AMOUNT;
   char          copyright_notice[256];
   int           len;

   Display *     dpy                    = XtDisplay(handleRoot);
   Window        window                 = XtWindow(handleRoot);
   Screen *      screen                 = XtScreen(handleRoot);
   GC            gc                     = DefaultGCOfScreen(screen);
   XFontStruct * font; 
   int           direction;
   int           ascent;
   int           descent;
   XCharStruct   overall;
   int           x;
   int           y;
   unsigned      long gray;
   char *	 xdm;

#ifdef STAND_ALONE

   /*
    * fork was done in main()
    */

#else

   /*
    * should fork if we're a routine within another client
    */

   if (fork() != 0)
      return;

#endif /* STAND_ALONE */

   if ( dpy )
      font = XLoadQueryFont(dpy, FONT_TO_USE);
   else
      exit(1);

   XSetFont(dpy, gc, font->fid);

   xdm = getenv("XDM_LOGIN");

   if( xdm && *xdm == 'y' ) 
     XSetForeground(dpy, gc, BlackPixelOfScreen(screen));
   else
     XSetForeground(dpy, gc, WhitePixelOfScreen(screen)); 

   x = 5;
   y = HeightOfScreen(screen);

   /*
    * read message in reverse order
    */

   while (gets(copyright_notice) != NULL) 
   {
      len = strlen(copyright_notice);

      XTextExtents(font, copyright_notice, len, 
          &direction, &ascent, &descent, &overall);

      y = y - (ascent + descent + 1);

      XDrawString(dpy, window, gc, x, y, copyright_notice, len);
   }
   XFlush(dpy);

   sleep(copyright_sleep_amount);

   XClearWindow(dpy, window);
   XFlush(dpy);

   exit(0);

} /* end of DisplayCopyrightNotice */
