#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:Gizmos.h	1.20"
#endif

/*
 * Gizmos.h
 *
 */

#ifndef _Gizmos_h
#define _Gizmos_h

#define FS               "\001"

#define XtRExclusive     "Exclusive"
#define XtRNonexclusive  "Nonexclusive"

#define SIZE_ARGS	100

typedef void * Gizmo;

typedef struct _Setting
{
   XtPointer initial_string;
   XtPointer initial_value;
   XtPointer current_value;
   XtPointer previous_value;
   long      flag;
} Setting;

typedef struct _HelpInfo
{
   char * app_title;
   char * title;
   char * filename;
   char * section;
   char * path;
} HelpInfo;    /* FIX: This should eventually be 
	        * removed in favor of OlDtHelpInfo
                */

typedef struct _GizmoClassRec 
{
   char *     name;
   Widget     (*create_function)();
   Gizmo      (*copy_function)();
   void       (*free_function)();
   void       (*map_function)();
   Gizmo      (*get_function)();
   Gizmo      (*get_menu_function)();
   void       (*build_function)();
   void       (*manipulate_function)();
   XtPointer  (*query_function)();
   void       (*manage_function)();
} GizmoClassRec;

typedef GizmoClassRec * GizmoClass;

typedef struct _GizmoRec 
{
   GizmoClass gizmo_class;
   Gizmo      gizmo;
   Arg *      args;
   Cardinal   num_args;
} GizmoRec;

typedef GizmoRec * GizmoArray;

typedef enum 
{
   GetGizmoValue,          /* from widget to current_value            */
   ApplyGizmoValue,        /* from current_value to previous_value    */
   SetGizmoValue,          /* apply + from current_value to Xdefaults */
   ResetGizmoValue,        /* from previous_value to current_value    */
   ReinitializeGizmoValue, /* from initali_value to current_value     */
   RegisterGizmoConverter, /* register converter for the gizmo        */
   GetGizmoSetting,        /* retrieve the gizmo settings pointer     */
   GetGizmoWidget,         /* retrieve the gizmo Widget               */
   GetGizmoGizmo           /* retrieve the named gizmo                */
} ManipulateOption;

typedef enum 
{
   SensitizeGizmo,         /* XtSetSensitive the widget (or parts)    */
   DesensitizeGizmo,       /* XtSetSensitive the widget (or parts)    */
   HideGizmo,              /* XtManageChild the widget (or parts)     */
   UnhideGizmo,            /* XtManageChild the widget (or parts)     */
} ManageOption;

extern Widget    InitializeGizmoClient(char * name, char * class, 
                    char * app_name, GizmoClass gizmoClass, Gizmo gizmo,
                    XrmOptionDescRec * options, Cardinal num_options, 
                    int * argc, char * argv[],
                    XtPointer other,
                    XtPointer base, XtResourceList resources, Cardinal num_resources, 
                    ArgList args, Cardinal num_args, char * resource_name,
                    Boolean (*dropCB)(), XtPointer dropCD);
extern void      InitializeGizmos(char * x_app_name, char * app_name);
extern void      GizmoMainLoop(void (*inputCB)(), XtPointer input_data, 
                               void (*otherCB)(), XtPointer other_data);
extern Gizmo     CopyGizmo(GizmoClass gizmoClass, Gizmo gizmo);
extern void      FreeGizmo(GizmoClass gizmoClass, Gizmo gizmo);
extern Widget    CreateGizmo(Widget parent, GizmoClass gizmoClass,
                             Gizmo gizmo, ArgList arg, int num);
extern void      MapGizmo(GizmoClass gizmoClass, Gizmo gizmo);
extern Gizmo     GetGizmo(GizmoClass gizmoClass, void *shell, int item);
extern void      ManipulateGizmo(GizmoClass gizmoClass, Gizmo gizmo,
                                 ManipulateOption option);
extern XtPointer QueryGizmo
                 (GizmoClass gizmoClass, Gizmo gizmo, int option, char * name);
extern void      ManageGizmo(GizmoClass gizmoclass, Gizmo gizmo, 
                             ManageOption option, char * name);
extern XtPointer QueryGizmoArray
                 (GizmoArray gizmos, int num_gizmos, int option, char * name);
extern Cardinal  AppendArgsToList(ArgList original, Cardinal num_original, 
                                  ArgList additional, Cardinal num_additional);
extern char *    GetGizmoText(char * label);
extern void      CopyGizmoArray(GizmoArray *, int *, GizmoArray, int);
extern void      FreeGizmoArray(GizmoArray, int);
extern void      CreateGizmoArray(Widget, GizmoArray, int);
extern char *    (*GetGizmoAppName)();
extern void      PostGizmoHelp(Widget w, HelpInfo * help);
extern char *    ConcatGizmoString(char ** r, char * s1, char * s2);
extern void      DisallowGizmoPopdown(Widget, XtPointer, XtPointer);
extern void      GizmoRegisterHelp(OlDefine type, Widget w, char * title, 
                                   OlDefine src, HelpInfo * help);

#endif /* _Gizmos_h */
