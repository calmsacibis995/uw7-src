/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Gizmo:FileGizmo.h	1.5"
#endif

/*
 * FileGizmo.h
 *
 */

#ifndef _FileGizmo_h
#define _FileGizmo_h

/*
 * FileGizmo
 *
 * The FileGizmo is used to construct a File Shell interface
 * element consisting of a text and controls in the the upper area
 * and a menubar (constructed from a MenuGizmo) placed at the bottom
 * of the shell.  The map_function of this Gizmo uses XtPopup
 * with GrabExclusive.
 *
 * Synopsis:
 *#include <Gizmos.h>
 *#include <FileGizmo.h>
 * ... 
 */

typedef struct _Filelist
{
   int dir_size;
   int dir_used;
   int match_size;
   int match_used;
   int nomatch_size;
   int nomatch_used;
   char ** dirs;
   char ** matchs;
   char ** nomatchs;
} Filelist;

#define FOLDERS_ONLY        1
#define FOLDERS_AND_FILES   2
#define FOLDERS_AND_FOLDERS 3

typedef struct _FileGizmo 
{
   HelpInfo *  help;             /* help information                       */
   char *      name;             /* name of the shell                      */
   char *      title;            /* title of the window                    */
   MenuGizmo * menu;             /* Pointer to menu info                   */
   char *      message;          /* message for stext (Notice case)        */
   char *      path;             /* current path      (used as a filter)   */
   char *      directory;        /* current directory                      */
   int         dialog_type;      /* FOLDERS_ONLY or FOLDERS_AND_FILES      */
   GizmoArray  gizmos;           /* the gizmo list                         */
   int         num_gizmos;       /* number of gizmos                       */
   char *      operlabel;	 /* Label for Copy/Move/Link etc 	   */
   ArgList     args;             /* args applied to the shell              */
   Cardinal    num_args;         /* number of args                         */
   Filelist *  list;             /* contents of the current directory      */
   Widget      controlWidget;    /* control area       (returned)          */
   Widget      textFieldWidget;  /* text field         (returned)          */
   Widget      textFieldCaption; /* text field         (returned)          */
   Widget      staticTextWidget; /* static text        (returned)          */
   Widget      operstaticTextWidget; /* static text        (returned)      */
   Widget      staticTextCaption;/* static text        (returned)          */
   Widget      checkboxWidget;/* static text        (returned)          */
   Widget      subdirListWidget; /* subdirs list       (returned)          */
   Widget      fnameListWidget;  /* filename list      (returned)          */
   Widget      messageWidget;    /* static text widget (returned)          */
   Widget      shell;            /* File shell         (returned)          */
} FileGizmo;

extern GizmoClassRec FileGizmoClass[];

extern Widget GetFileGizmoShell(FileGizmo * gizmo);
extern void   SetFileGizmoMessage(FileGizmo * gizmo, char * message);

extern int    ExpandFileGizmoFilename(FileGizmo * gizmo, int * flag);
extern char * GetFilePath(FileGizmo * gizmo);
extern void   SetFileCriteria(FileGizmo * gizmo, char * directory, char * path);
extern void   SetFileGizmoPathLabel(FileGizmo * gizmo, char * path);
extern void   SetFileGizmoNameLabel(FileGizmo * gizmo, char * name);
extern void   SetFileGizmoSrcList(FileGizmo * gizmo, char * name);
extern void   SelectFileGizmoTextFieldString(Widget textField);

#endif /* _FileGizmo_h */
