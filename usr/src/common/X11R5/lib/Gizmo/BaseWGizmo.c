#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:BaseWGizmo.c	1.43"
#endif

/*
 * BaseWGizmo.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Vendor.h>
#include <X11/RectObj.h>

#include <Xol/OpenLook.h>
#include <Xol/Form.h>
#include <Xol/ScrolledWi.h>
#include <Xol/TextEdit.h>
#include <Xol/StaticText.h>
#include <Xol/RubberTile.h>
#include <Xol/Footer.h>

#include <DtI.h>
#include <xpm.h>

#include "Gizmos.h"
#include "MenuGizmo.h"
#include "BaseWGizmo.h"

static Widget      CreateBaseWindow();
static Gizmo       CopyBaseWindow();
static void        FreeBaseWindow();
static void        RealizeBaseWindow();
static void        ManipulateBaseWindow();
static XtPointer   QueryBaseWindow();

static Gizmo       GetBaseWindowGizmo();
static Gizmo       GetTheMenuGizmo();
static Pixmap      CreateCenteredPixmap(Widget w, 
                                        int icon_width, int icon_height, 
                                        int width, int height, 
                                        Boolean is_color, 
                                        Pixmap pixmap, Pixmap pixmask, 
                                        char * icon_name);

static void        WMCB(Widget w, XtPointer client_data, XtPointer call_data);

GizmoClassRec BaseWindowGizmoClass[] = 
   { 
   "BaseWindowGizmo", 
   CreateBaseWindow, 
   CopyBaseWindow, 
   FreeBaseWindow, 
   RealizeBaseWindow,
   GetBaseWindowGizmo,
   GetTheMenuGizmo,
   NULL,
   ManipulateBaseWindow,
   QueryBaseWindow,
   };


/*
 * CopyBaseWindow
 *
 * The CopyBaseWindow fuction is used to create a copy 
 * of an existing BaseWindowGizmo old.  This function makes
 * it easy to duplicate a static definition of an application
 * base window for use in creating multiple, independent application
 * shells.  To do this the application:~
 * 
 * creates the static definition
 *
 * copies the static definition (creating a dynamically allocated
 * BaseWindowGizmo structure) using the CopyBaseWindow function
 *
 * creates the widget tree using the CreateBaseWindow function
 *
 * realizes the new independent base window using the RealizeBaseWindow
 * procedure.
 * 
 * The application can deallocate the BaseWindowGizmo later, when the
 * user decides to Exit the shell for example, by calling the
 * FreeBaseWindow procedure.
 * 
 * See also:
 *
 * FreeBaseWindow(3), CreateBaseWindow(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 *#include <BaseWGizmo.h>
 * ...
 */

static Gizmo 
CopyBaseWindow(gizmo)
Gizmo gizmo;
{
   BaseWindowGizmo * old = (BaseWindowGizmo *)gizmo;
   BaseWindowGizmo * new = (BaseWindowGizmo *)MALLOC(sizeof(BaseWindowGizmo));

   new->help         = old->help;
   new->name         = old->name;
   new->title        = old->title;
   new->menu         = CopyGizmo(MenuGizmoClass, old->menu);
   CopyGizmoArray(&new->gizmos, &new->num_gizmos, old->gizmos, old->num_gizmos);
   new->icon_name    = old->icon_name;
   new->icon_pixmap  = old->icon_pixmap;
   new->icon_shell   = NULL;
   new->shell        = NULL;
   new->form         = NULL;
   new->message      = NULL;
   new->scroller     = NULL;
   new->error_percent= old->error_percent;
   new->error        = old->error;
   new->status       = old->status;

   return ((Gizmo)new);

} /* end of CopyBaseWindow */
/*
 * FreeBaseWindow
 *
 * The FreeBaseWindow procedure is used to free a previously
 * allocated BaseWindowGizmo old.
 *
 * See also:
 *
 * CopyBaseWindow(3), CreateBaseWindow(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 *#include <BaseWGizmo.h>
 * ...
 */

static void 
FreeBaseWindow(gizmo)
Gizmo gizmo;
{
   BaseWindowGizmo * old = (BaseWindowGizmo *)gizmo;

   FreeGizmo(MenuGizmoClass, old->menu);
   FreeGizmoArray(old->gizmos, old->num_gizmos);
   FREE(old);

} /* end of FreeBaseWindow */
/*
 * CreateBaseWindow
 *
 * The CreateBaseWindow function is used to create an
 * entire, standard base window shell.  This shell is created
 * on the display associated with the parent Widget
 * using the elements of the gizmo BaseWindowGizmo.  args
 * and num, if non-NULL, are used as Args in the creation
 * of the application shell.  The function returns the Widget
 * handle of the application shell that is created.
 *
 * Standard Appearance:
 *
 * The CreateBaseWindow function creates a standard base
 * window composed of an applicationShell Widget shell containing
 * a form Widget form containing a flatButtons Widget (the
 * menu bar), a scrolledWindow * scroller Widget, and a staticText 
 * Widget message.  A typical window appears as:~
 *
 * Example:
 *
 * To use the function one must first allocate a BaseWindowGizmo
 * structure initialized with the title of the application shell
 * and the pointer to the Menu data structure for the menu in
 * the menu bar.  Then the application invokes the CreateBaseWindow
 * function.  So, for example, to create the above appearance:~
 *
 * See also:
 *
 * FreeBaseWindow(3), CreateBaseWindow(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 *#include <BaseWGizmo.h>
 * ...
 * 
 */

static Widget 
CreateBaseWindow
   (Widget parent, BaseWindowGizmo * gizmo, Arg * args, int num)
{
   MenuGizmo *       menu       = (MenuGizmo *)gizmo->menu;
   Widget            menuparent;
   Dimension         footerHeight;
   Arg               arg[100];
   int               i;
   int               error_percent;
   Pixmap            icon_pixmap;
   int               width;
   int               height;

   XtSetArg(arg[0], XtNborderWidth,       0);
   XtSetArg(arg[1], XtNtranslations,      XtParseTranslationTable(""));
   XtSetArg(arg[2], XtNgeometry,          "32x32");
   XtSetArg(arg[3], XtNmappedWhenManaged, False);
   if (gizmo->icon_pixmap && *(gizmo->icon_pixmap))
   {
      gizmo->icon_shell = 
         XtCreateApplicationShell("_X_", vendorShellWidgetClass, arg, 4);

      OlAddCallback(gizmo->icon_shell, XtNwmProtocol, WMCB, (XtPointer)gizmo);

      XtRealizeWidget(gizmo->icon_shell);

      icon_pixmap = PixmapOfFile(gizmo->icon_shell, gizmo->icon_pixmap, 
         gizmo->icon_name, &width, &height);

      XtSetArg(arg[0], XtNbackgroundPixmap,  icon_pixmap);
      XtSetArg(arg[1], XtNwidth,  width);
      XtSetArg(arg[2], XtNheight, height);
      XtSetValues(gizmo->icon_shell, arg, 3);
   }
   else
      gizmo->icon_shell = NULL;

   if (gizmo->shell == NULL)
      gizmo->shell = 
         XtCreateApplicationShell(gizmo->name, topLevelShellWidgetClass, args, num);

   if (gizmo->help)
   {
      GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->shell,
         gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);
      if (gizmo->icon_shell)
         GizmoRegisterHelp(OL_WIDGET_HELP, gizmo->icon_shell,
            gizmo->help->title, OL_DESKTOP_SOURCE, gizmo->help);
   }

   XtSetArg(arg[0], XtNtitle,    
      gizmo->title     ? GetGizmoText(gizmo->title)     : " ");
   XtSetArg(arg[1], XtNiconName,
      gizmo->icon_name ? GetGizmoText(gizmo->icon_name) : (char *)arg[0].value);
   i = 2;
   if (gizmo->icon_shell)
   {
      XtSetArg(arg[i], XtNiconWindow, XtWindow(gizmo->icon_shell));
      i++;
   }
   XtSetValues(gizmo->shell, arg, i);

   XtSetArg(arg[0], XtNorientation,   OL_VERTICAL);
   gizmo->form = 
      XtCreateManagedWidget("_X_", rubberTileWidgetClass, gizmo->shell, arg, 1);

   /*
    * adjust the gravity and padding
    * note: the padding should be specified in points
    * also change the menubar behavior since you can't
    * do set values!!!
    */

   XtSetArg(arg[0], XtNgravity,         NorthWestGravity);
   XtSetArg(arg[1], XtNvPad,            6);
   XtSetArg(arg[2], XtNhPad,            6);
   XtSetArg(arg[3], XtNmenubarBehavior, True);
   menuparent = CreateGizmo(gizmo->form, MenuBarGizmoClass, (Gizmo)menu, arg, 4);
   XtSetArg(arg[0], XtNweight,          0);
   XtSetValues(menuparent, arg, 1);

   if (gizmo->num_gizmos == 0)
   {
      XtSetArg(arg[0], XtNyRefWidget,    menu->child);
      XtSetArg(arg[1], XtNweight, 1);
      gizmo->scroller =
         XtCreateManagedWidget("_X_", scrolledWindowWidgetClass, gizmo->form, arg, 2);
   }
   else
   {
      CreateGizmoArray(gizmo->form, gizmo->gizmos, gizmo->num_gizmos);
   }

   error_percent = gizmo->error_percent == 0 ? 75 : gizmo->error_percent;

   XtSetArg(arg[0], XtNweight,      0);
   XtSetArg(arg[1], XtNleftFoot,    gizmo->error ? GetGizmoText(gizmo->error) : " ");
   XtSetArg(arg[2], XtNrightFoot,   gizmo->status ? GetGizmoText(gizmo->status) : " ");
   XtSetArg(arg[3], XtNleftWeight,  error_percent);
   XtSetArg(arg[4], XtNrightWeight, 100-error_percent);
   gizmo->message = XtCreateManagedWidget (
      "footer", footerWidgetClass, gizmo->form, arg, 5
   );

   return (gizmo->shell);

} /* end of CreateBaseWindow */
/*
 * GetBaseWindowShell
 *
 * The GetBaseWindowShell function is used to retrieve the application
 * shell widget stored in the gizmo BaseWindowGizmo data structure.
 *
 * See also:
 *
 * CopyBaseWindow(3), FreeBaseWindow(3), CreateBaseWindow(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 *#include <BaseWGizmo.h>
 * ...
 */

extern Widget
GetBaseWindowShell(BaseWindowGizmo * gizmo)
{

   return (gizmo->shell);

} /* end of GetBaseWindowShell */
/*
 * GetTheMenuGizmo
 *
 */

static Gizmo  
GetTheMenuGizmo(BaseWindowGizmo * gizmo)
{

   return (gizmo->menu);

} /* end of GetTheMenuGizmo */
/*
 * GetBaseWindowGizmo
 *
 */

static Gizmo  
GetBaseWindowGizmo(BaseWindowGizmo * gizmo, int item)
{

   return (gizmo->gizmos[item].gizmo);

} /* end of GetBaseWindowGizmo */

/*
 * GetBaseWindowScroller
 *
 * The GetBaseWindowScroller function is used to retrieve the
 * scrolled window widget stored in the gizmo BaseWindowGizmo data 
 * structure.  This Widget is used as the parent of the application
 * specific pane content.
 *
 * See also:
 *
 * CopyBaseWindow(3), FreeBaseWindow(3), CreateBaseWindow(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 *#include <BaseWGizmo.h>
 * ...
 */

extern Widget
GetBaseWindowScroller(BaseWindowGizmo * gizmo)
{

   return (gizmo->scroller);

} /* end of GetBaseWindowScroller */
/*
 * RealizeBaseWindow
 *
 * The RealizeBaseWindow procedure is used to map (realize)
 * the application shell in the gizmo BaseWindowGizmo.
 *
 * See also:
 *
 * CopyBaseWindow(3), FreeBaseWindow(3), CreateBaseWindow(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 *#include <BaseWGizmo.h>
 * ...
 */

static void
RealizeBaseWindow(BaseWindowGizmo * gizmo)
{

   if (!XtIsRealized(gizmo->shell))
      XtRealizeWidget(gizmo->shell);
   XtMapWidget(gizmo->shell);

} /* end of RealizeBaseWindow */
/*
 * SetBaseWindowError
 *
 * The SetBaseWindowError procedure is set the message
 * appearing in the left footer message are of the gizmo BaseWindowGizmo 
 * to the message string.
 *
 * See also:
 *
 * CopyBaseWindow(3), FreeBaseWindow(3), CreateBaseWindow(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 *#include <BaseWGizmo.h>
 * ...
 */

extern void 
SetBaseWindowMessage(BaseWindowGizmo * gizmo, char * message)
{
   Arg arg[10];

   XtSetArg(arg[0], XtNleftFoot,     message ? GetGizmoText(message) : " ");
   XtSetValues(gizmo->message, arg, 1);

} /* end of SetBaseWindowError */
/*
 * SetBaseWindowStatus
 *
 * The SetBaseWindowStatus procedure is set the message
 * appearing in the right footer message are of the gizmo BaseWindowGizmo 
 * to the message string.
 *
 * See also:
 *
 * CopyBaseWindow(3), FreeBaseWindow(3), CreateBaseWindow(3)
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <MenuGizmo.h>
 *#include <BaseWGizmo.h>
 * ...
 */

extern void 
SetBaseWindowStatus(BaseWindowGizmo * gizmo, char * message)
{
   Arg arg[10];

   XtSetArg(arg[0], XtNrightFoot,     message ? GetGizmoText(message) : " ");
   XtSetValues(gizmo->message, arg, 1);

} /* end of SetBaseWindowStatus */
/*
 * SetBaseWindowTitle
 *
 */

extern void
SetBaseWindowTitle(BaseWindowGizmo * gizmo, char * title)
{
   Arg arg[10];

   XtSetArg(arg[0], XtNtitle, title ? GetGizmoText(title) : " ");
   XtSetArg(arg[1], XtNiconName, 
      gizmo->icon_name ? GetGizmoText(gizmo->icon_name) : (char *)arg[0].value);
   XtSetValues(gizmo->shell, arg, 2);

   gizmo->title = title;

} /* end of SetBaseWindowTitle */
/*
 * ManipulateBaseWindow
 *
 */

static void
ManipulateBaseWindow(BaseWindowGizmo * gizmo, ManipulateOption option)
{
   GizmoArray gp = gizmo->gizmos;
   int        i;

   for (i = 0; i < gizmo->num_gizmos; i++) 
   {
      ManipulateGizmo(gp[i].gizmo_class, gp[i].gizmo, option);
   }

} /* end of ManipulateBaseWindow */
/*
 * QueryBaseWindow
 *
 */

static XtPointer
QueryBaseWindow(BaseWindowGizmo * gizmo, int option, char * name)
{

   if (!name || !gizmo->name || strcmp(name, gizmo->name) == 0)
   {
      switch(option)
      {
         case GetGizmoSetting:
            return (XtPointer)(NULL);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->shell);
            break;
         case GetGizmoGizmo:
            return (XtPointer)(gizmo);
            break;
         default:
            return (NULL);
            break;
      }
   }
   else
   {
      XtPointer value = QueryGizmo(MenuGizmoClass, gizmo->menu, option, name);
      if (value)
         return (value);
      else
      {
         return(QueryGizmoArray(gizmo->gizmos, gizmo->num_gizmos, option, name));
      }
   }

} /* end of QueryBaseWindow */
/*
 * PixmapOfFile
 *
 */

extern Pixmap
PixmapOfFile(Widget w, char * fname, char * icon_name, int * iwid, int * iht)
{
   Screen *     screen = XtScreen(w);
   DmGlyphRec * gp;

   typedef struct _PixmapCache
      {
      struct _PixmapCache *     next;
      char *                    fname;
      Pixmap                    pixmap;
      DmGlyphRec *              gp;
      } PixmapCache;

   typedef struct _PixmapSizeCache
      {
      struct _PixmapSizeCache * next;
      Screen *                  screen;
      Dimension                 width;
      Dimension                 height;
      PixmapCache *             pixmaps;
      } PixmapSizeCache;

   static PixmapSizeCache * pixmap_size_cache = NULL;
   PixmapCache *            current_pixmap;
   PixmapSizeCache *        p;

   for (p = pixmap_size_cache; p != NULL; p = p->next)
      if (p->screen == screen)
         break;
   if (p == NULL)
   {
      XIconSize * icon_size_hint;
      int         count;

      p = (PixmapSizeCache *)MALLOC(sizeof(PixmapSizeCache));
      p->next        = pixmap_size_cache;
      p->screen      = screen;
      if (XGetIconSizes(DisplayOfScreen(screen), RootWindowOfScreen(screen), 
          &icon_size_hint, &count) && count > 0)
      {
         p->width       = icon_size_hint->max_width;
         p->height      = icon_size_hint->max_height;
         FREE(icon_size_hint);
      }
      else
      {
         p->width       = 70;
         p->height      = 70;
      }
      current_pixmap = 
      p->pixmaps     = (PixmapCache *)MALLOC(sizeof(PixmapCache));
      current_pixmap->next   = NULL;
      current_pixmap->fname  = STRDUP(fname);
      current_pixmap->pixmap = (Pixmap)NULL;
      current_pixmap->gp     = (DmGlyphRec *)NULL;
      /*
       * find out how big to make pixmaps for this screen
       */
   }
   else
   {
      for (current_pixmap = p->pixmaps; 
           current_pixmap != NULL; 
           current_pixmap = current_pixmap->next)
         if (strcmp(current_pixmap->fname, fname) == 0)
            break;
      if (current_pixmap == NULL)
      {
         current_pixmap = (PixmapCache *)MALLOC(sizeof(PixmapCache));
         current_pixmap->next   = p->pixmaps;
         current_pixmap->fname  = STRDUP(fname);
         current_pixmap->pixmap = (Pixmap)NULL;
         current_pixmap->gp     = (DmGlyphRec *)NULL;
         p->pixmaps = current_pixmap;
      }
      else
      {
         *iwid = p->width;
         *iht  = p->height;
         return(current_pixmap->pixmap);
      }
   }

   gp = current_pixmap->gp = DmGetPixmap(screen, fname);
   if (!gp)
      gp = current_pixmap->gp = DmGetPixmap(screen, NULL);

   current_pixmap->pixmap = CreateCenteredPixmap(w, p->width, p->height, 
      gp-> width, gp->height, gp->depth > 1, gp->pix, gp->mask, icon_name);

   *iwid = p->width;
   *iht  = p->height;

   return(current_pixmap->pixmap);

} /* PixmapOfFile */
/*
 * CreateCenteredPixmap
 *
 */

static Pixmap
CreateCenteredPixmap(Widget w, int icon_width, int icon_height, int width, int height, Boolean is_color, Pixmap pixmap, Pixmap pixmask, char * icon_name)
{
   Screen *  screen     = XtScreen(w);
   Display * dpy        = XtDisplay(w);
   Window    win        = XtWindow(w);
   int       depth      = DefaultDepthOfScreen(screen);
   Pixmap    new_pixmap = pixmap;
   Arg       arg[5];
   GC        gc;
   XGCValues values;
   int       x          = (icon_width - width) / 2;
   int       y          = (icon_height - height) / 2;
   Pixel     background;
   Pixel     foreground;
   XFontStruct * fs;

   XtSetArg(arg[0], XtNbackground, &background);
   XtSetArg(arg[1], XtNforeground, &foreground);
   XtGetValues(w, arg, 2);

   fs = (XFontStruct *)_OlGetDefaultFont(w, OlDefaultFont);
   icon_name = GetGizmoText(icon_name);

   values.foreground = background;
   values.font       = fs->fid;
   values.clip_x_origin = 0;
   values.clip_y_origin = 0;
   values.clip_mask = NULL;
   gc = XtGetGC(w, GCForeground|GCFont|GCClipMask|GCClipXOrigin|GCClipYOrigin, &values);
   new_pixmap = XCreatePixmap(dpy, win, icon_width, icon_height, depth);

   XFillRectangle(dpy, new_pixmap, gc, 0, 0, icon_width, icon_height);

#ifdef NEED_TO_DRAW_THE_NAME
   /*
    * FIX Draw the icon name (if the window manager doesn't do this)
    *
    */
   {
   int text_len   = strlen(icon_name);
   int text_width = XTextWidth(fs, icon_name, text_len);
   int text_x     = (icon_width - text_width) / 2;
   int text_y     = icon_height - fs->descent;
   XSetForeground(dpy, gc, foreground);
   XDrawString(dpy, new_pixmap, gc, text_x, text_y, icon_name, text_len);
   }
   /*
    *
    */
#endif /* NEED_TO_DRAW_THE_NAME */

   XSetClipMask(dpy, gc, pixmask);
   XSetClipOrigin(dpy, gc, x, y);

   if (is_color)
   {
      XCopyArea(dpy, pixmap, new_pixmap, gc, 0, 0, width, height, x, y);
   }
   else
   {
      XSetForeground(dpy, gc, foreground);
      XCopyPlane(dpy, pixmap, new_pixmap, gc, 0, 0, width, height, x, y, 1L);
   }

   XSetClipMask(dpy, gc, NULL);
   XSetClipOrigin(dpy, gc, 0, 0);
   return (new_pixmap);

} /* end of CreateCenteredPixmap */
/*
 * WMCB
 *
 */

static void
WMCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   BaseWindowGizmo *    gizmo = (BaseWindowGizmo *)client_data;
   OlWMProtocolVerify * cd    = (OlWMProtocolVerify *)call_data;

   if (cd->msgtype == OL_WM_DELETE_WINDOW)
      if (OlHasCallbacks(gizmo->shell, XtNwmProtocol) == XtCallbackHasSome)
         OlCallCallbacks(gizmo->shell, XtNwmProtocol, call_data);
      else
         MapGizmo(BaseWindowGizmoClass, gizmo);
   else
      ;
#ifdef DEBUG
      (void)fprintf(stderr,"wm protocol - %d \n", cd->msgtype);
#endif

} /* end of WMCB */
