/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:dialup/fileinfo.c	1.2"
#endif

/*
 * fileisntall.c
 *
 */

#include <libgen.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h> 

#include <X11/Intrinsic.h> 
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/TextField.h>
#include <Xol/StaticText.h>
#include <Xol/ScrolledWi.h>
#include <Xol/FList.h>
#include <Xol/FButtons.h>

#include "fileinfo.h"
#include "error.h"



static char * listFields[] = { XtNlabel };


static int        qstrcmp(const void * s1, const void * s2);
Filelist * ReadDirectory(FileInfo * fileinfo, char * directory);
static Boolean        RereadDirectory(FileInfo * fileinfo);
void       	ListSelect(Widget w, XtPointer client_data, XtPointer call_data);
void       FixDirectory(FileInfo * fileinfo);
void       FreeLists(FileInfo * fileinfo);
static void       FreeListArray(char ** list, int used);

/*
 * qstrcmp
 *
 */

static int
qstrcmp(const void * s1, const void * s2)
{

   return (strcmp(*((char **)s1), *((char **)s2)));

} /* end of qstrcmp */
/*
 * ReadDirectory
 *
 */

Filelist *
ReadDirectory(FileInfo * fileinfo, char * directory)
{
   Filelist *      list = NULL;
   DIR *           dp;
   struct dirent * dep;
   struct stat     stat_buffer;
   char *          old_directory;
   char            buffer[500];
   Arg		   args[5];
   Boolean         set = False;

#ifdef DEBUG
	fprintf(stderr,"reading directory=%s\n",directory);
#endif
   if ((dp = opendir(directory)) == NULL)
   {
      perror("opendir:");
      return (NULL);
   }
   else
   {
      old_directory = (char *)getcwd(NULL, FILENAME_MAX);
      
      chdir(directory);  /* chdir done to allow stat on relative path name */

      list = (Filelist *)MALLOC(sizeof(Filelist));
      list->dir_size = 10;
      list->dirs = (char **)MALLOC(sizeof(char *) * list->dir_size);
      if (strcmp(fileinfo->directory, "/") != 0)
      {
         list->dir_used = 1;
         list->dirs[0] = STRDUP(GGT(PARENT_DIRECTORY));
      }
      else
      {
         list->dir_used = 0;
      }

      while ((dep = readdir(dp)) != NULL)
      {
         if ((strcmp(dep->d_name, ".") == 0) ||
             (strcmp(dep->d_name, "..") == 0) ||
	     ((set == False) && (dep->d_name[0] == '.')))
            ;  /* skip current and parent directories */
         else
         {
            if (stat(dep->d_name, &stat_buffer) < 0)
            {
               /* skip files that we can't stat (FIX) */
               perror("stat:");
            }
            else
            {
               if ((stat_buffer.st_mode & S_IFMT) == S_IFDIR)
               {
                  if (list->dir_used == list->dir_size)
                  {
                     list->dir_size *= 2;
                     list->dirs = (char **)
                        REALLOC((void *)list->dirs, sizeof(char *) * list->dir_size);
                  }
                  list->dirs[list->dir_used++] = STRDUP(dep->d_name);
               }
            }
         }
      }
      if (list->dir_used > 2)
         qsort((void *)&list->dirs[1], (size_t)list->dir_used - 1, sizeof(char *), qstrcmp);
      if (old_directory)
      {
         chdir(old_directory);
         free(old_directory);
      }

      closedir(dp);

      return (list);
   }

} /* end of ReadDirectory */
/*
 * RereadDirectory
 *
 */

static Boolean 
RereadDirectory(FileInfo * fileinfo)
{
   Arg    arg[10];
   Filelist *tmplist;
      
	/* Store the new list in a temporary so that we can check whether
	 * a NULL list is returned as in the case where the selected
	 * directory cannot be opened.  This ensures that the current
	 * list is not freed when the new list is NULL.
	 */
   tmplist = ReadDirectory(fileinfo, fileinfo->directory);

   if (tmplist)
   {
	FreeLists(fileinfo);
	fileinfo->list = tmplist;

	/* reset original value of name if list was selected */
      XtVaSetValues(fileinfo->textFieldWidget, XtNstring,
		fileinfo->name, 0);

      XtSetArg(arg[0], XtNstring, fileinfo->directory);
      XtSetValues(fileinfo->curFolderWidget, arg, 1);
      XtSetValues(fileinfo->curPathWidget, arg, 1);

      XtSetArg(arg[0], XtNitems,    fileinfo->list->dirs);
      XtSetArg(arg[1], XtNnumItems, fileinfo->list->dir_used);
      XtSetValues(fileinfo->subdirListWidget, arg, 2);
      return True;


   }

   return False;

} /* end of RereadDirectory */

/*
 * ListSelect
 *
 */

void
ListSelect(Widget w, XtPointer client_data, XtPointer call_data)
{
   FileInfo *      fileinfo    = (FileInfo *)client_data;
   OlFlatCallData * p        = (OlFlatCallData *)call_data;
   int              item     = p->item_index;
   Arg              arg[10];
   char             copy[PATH_MAX];


   if (fileinfo->subdirListWidget == w)
   {
      strcpy(copy, fileinfo->directory);
      if (item == 0) /* parent */
      {
         char * x = strrchr(fileinfo->directory, '/');
         if (x == fileinfo->directory)
         {
            *++x = '\0';
         }
         else
            *x = '\0';
      }
      else
      {
	/* add subdirectory name to the directory name to get the full path */
         fileinfo->directory = 
            (char *)REALLOC(fileinfo->directory, strlen(fileinfo->directory) +
                            strlen(fileinfo->list->dirs[item]) + 2);
         (void)strcat(fileinfo->directory, "/");
         (void)strcat(fileinfo->directory, fileinfo->list->dirs[item]);
      }
      if (RereadDirectory(fileinfo) == False)
      {
         FREE(fileinfo->directory);
         fileinfo->directory = strdup(copy);
      }
   }
} /* end of ListSelect */

/*
 * FixDirectory
 *
 */

void
FixDirectory(FileInfo * fileinfo)
{
   char * current_dir = (char *)getcwd(NULL, FILENAME_MAX);
   char * directory;

   if (fileinfo->directory == NULL || (strcmp(fileinfo->directory, ".") == 0))
   {
      if (fileinfo->directory)
         FREE(fileinfo->directory);
      fileinfo->directory = current_dir;
   }
   else
      if (*fileinfo->directory != '/')
      {
         directory = (char *)
            MALLOC(strlen(current_dir) + strlen(fileinfo->directory) + 2);
         (void)strcpy(directory, current_dir);
         (void)strcat(directory, "/");
         (void)strcat(directory, fileinfo->directory);
         free(current_dir);
         FREE(fileinfo->directory);
         fileinfo->directory = directory;
      }

} /* end of FixDirectory */
/*
 * FreeLists
 *
 */

void FreeLists(FileInfo * fileinfo)
{

   FreeListArray(fileinfo->list->dirs, fileinfo->list->dir_used);
   FREE((void *)fileinfo->list);

} /* end of FreeLists */
/*
 * FreeListArray
 *
 */

static void
FreeListArray(char ** list, int used)
{
   int i;

   for (i = 0; i < used; i++)
      FREE(list[i]);
   FREE((void *)list);

} /* end of FreeListArray */

/*
 * SetFileCriteria
 *
 */

extern void
SetFileCriteria(FileInfo * fileinfo, char * directory)
{
   Arg arg[10];
 
   if (directory)
   {
     if (fileinfo->directory)
         FREE(fileinfo->directory);
      fileinfo->directory = STRDUP(directory);
      FixDirectory(fileinfo);
     (void)RereadDirectory(fileinfo);
   }

   XtSetArg(arg[0], XtNselectStart,    0);
   XtSetArg(arg[1], XtNselectEnd,      0);
   XtSetArg(arg[2], XtNcursorPosition, 0);
   XtSetValues(fileinfo->textFieldWidget, arg, 3);


} /* end of SetFileCriteria */

