#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:ChoiceGizm.h	1.9"
#endif

/*
 * ChoiceGizmo.h
 *
 */

#ifndef _ChoiceGizmo_h
#define _ChoiceGizmo_h

/*
 * ChoiceGizmo
 *
 * The ChoiceGizmo is used to construct a Choice interface
 * element.  Choices can be represented as either Checkbox, Non-exclusive
 * or Exclusive buttons.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <ChoiceGizmo.h>
 * ... 
 */

typedef struct _ChoiceGizmo {
   HelpInfo *      help;            /* help information                 */
   char *          name;            /* name for the Widget              */
   char *          caption;         /* caption label (I18N'd)           */
   Gizmo           menu;            /* MenuGizmo pointer                */
   Setting *       settings;        /* setting structure                */
   void            (*verify)();     /* verify function                  */
   ArgList         args;            /* Arg array for buttons Widget     */
   Cardinal        num_args;        /* number of Args                   */
   Widget          captionWidget;   /* (return) Caption Widget          */
   Widget          buttonsWidget;   /* (return) FButtons Widget         */
   Widget          previewWidget;   /* (return) preview for abbreviated */
} ChoiceGizmo;

extern GizmoClassRec ChoiceGizmoClass[];
extern GizmoClassRec AbbrevChoiceGizmoClass[];

extern Widget GetChoiceButtons();
extern void   SetPreview(Widget w, XtPointer client_data, XtPointer call_data);

#endif /* _ChoiceGizmo_h */
