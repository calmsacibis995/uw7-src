#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:BaseWGizmo.h	1.11"
#endif

/*
 * BaseWGizmo.h
 *
 */

#ifndef _BaseWGizmo_h
#define _BaseWGizmo_h

/*
 * BaseWindowGizmo
 *
 * The BaseWindowGizmo is used to construct a Base Window interface
 * element consisting of a toplevel application shell containing a resizable
 * form managing a menu bar, scrolled window, and form.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <BaseWGizmo.h>
 * ... 
 */

typedef struct _BaseWindowGizmo {
   HelpInfo *   help;        /* help information                           */
   char *       name;        /* name of the shell                          */
   char *       title;       /* title of the widget                        */
   Gizmo        menu;        /* pointer to the menubar Menu Gizmo          */
   GizmoArray   gizmos;      /* list of Gizmos to insert                   */
   int          num_gizmos;  /* number of Gizmos in list                   */
   char *       icon_name;   /* name to use when iconic                    */
   char *       icon_pixmap; /* name of file containing the pixmap         */
   char *       error;       /* error message displayed in left footer     */
   char *       status;      /* status message displayed in right footer   */
   int          error_percent;/* percent of error message                  */
   Widget       icon_shell;  /* (return) icon shell Widget                 */
   Widget       shell;       /* (return) shell Widget                      */
   Widget       form;        /* (return) form Widget                       */
   Widget       message;     /* (return) footer message Widget             */
   Widget       scroller;    /* (return) scrolled window Widget            */
} BaseWindowGizmo;

extern GizmoClassRec   BaseWindowGizmoClass[];

extern Widget          GetBaseWindowShell();
extern Widget          GetBaseWindowScroller();

extern void            SetBaseWindowTitle();
extern void            SetBaseWindowMessage();
extern void            SetBaseWindowStatus();
extern Pixmap	       PixmapOfFile(Widget, char *, char *, int *, int *);

#endif /* _BaseWGizmo_h */
