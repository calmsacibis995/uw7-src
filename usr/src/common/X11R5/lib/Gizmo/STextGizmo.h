#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:STextGizmo.h	1.7"
#endif

/*
 * STextGizmo.h
 *
 */

#ifndef _STextGizmo_h
#define _STextGizmo_h

/*
 * StaticTextGizmo
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <STextGizmo.h>
 * ... 
 */

typedef struct _StaticTextGizmo
{
   HelpInfo *  help;         /* help information              */
   char *      name;         /* name of the widget            */
   char *      text;         /* text string                   */
   OlDefine    gravity;      /* text gravity                  */
   char *      font;         /* text font                     */
   Widget      widget;       /* static text widget (returned) */
} StaticTextGizmo;

extern GizmoClassRec StaticTextGizmoClass[];

#endif /* _STextGizmo_h */
