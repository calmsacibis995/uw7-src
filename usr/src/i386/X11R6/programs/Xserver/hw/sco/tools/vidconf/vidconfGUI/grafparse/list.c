/*
 *	@(#) list.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *      SCO MODIFICATION HISTORY
 *
 *
 */

/* tcl list merging includes */
#include <basicIncl.h>
#include <tclList.h>

#include "graf.h"

/*****************************************************************************
 *
 *  MakeListPair()
 *
 *  Description:
 *  -----------
 *	Builds a Tcl list based on the two strings.  The list space 
 *	_must_ be freed later by the calling function.
 *
 *  Parameters:
 *  -----------
 *	str1 - first string to appear in the list
 *	str2 - second string to appear in the list
 *
 *  Returns:
 *  -------
 *	Tcl list containing the pair of strings
 *
 *****************************************************************************/
char *
MakeListPair(char *str1, char *str2) 
{
   char *list;
   char *pair[2];

   pair[0] = str1;
   pair[1] = str2;

   list = TclListMerge(2, (const char **) pair);
   return (list);
}


/*****************************************************************************
 *
 *  MakeMonList()
 *
 *  Description:
 *  -----------
 *	Builds a monitor list from the strings passed in.
 *	The Tcl list space _must_ be freed later by the calling function.
 *
 *  Parameters:
 *  -----------
 *	monvendor - the monitor vendor string
 *	monmodel - the monitor model string
 *	description - the monitor description for monvendor/monmodel
 *
 *  Returns:
 *  -------
 *	Tcl list containing monitor information.
 *
 *****************************************************************************/
char *
MakeMonList(char *monvendor, 
	    char *monmodel, 
	    char *description)
{
   int i;
   char *monList;
   char *monArgv[3];

   monList = monArgv[0] = monArgv[1] = monArgv[2] = NULL;

   monArgv[0] = MakeListPair("MONVENDOR", monvendor);
   monArgv[1] = MakeListPair("MONMODEL", monmodel);
   monArgv[2] = MakeListPair("DESCRIPTION", description);
   monList = TclListMerge(3, (const char **) monArgv);

   for (i=0; i<3; i++)
      free (monArgv[i]);

   return (monList);
}
   

/*****************************************************************************
 *
 *  MakeVideoList()
 *
 *  Description:
 *  -----------
 *	Builds a video adapter list based on the strings passed in. The Tcl 
 *	list space _must_ be freed later by the calling function.
 *
 *  Parameters:
 *  -----------
 *	vendor - the video adapter vendor string
 *	vendorprompt - prompt string corresponding to vendor
 *	model - the particular model of video adapter
 *	modelprompt - the prompt string corresponding to model.
 *
 *  Returns:
 *  -------
 *	Tcl list containing video adapter information.
 *	
 *
 *****************************************************************************/
char *
MakeVideoList(char *vendor,
	      char *vendorprompt,
	      char *model,
	      char *modelprompt)
{
   int i;
   char *videoList;
   char *videoArgs[4];

   videoList = videoArgs[0] = videoArgs[1] = videoArgs[2] = videoArgs[3] = NULL;

   videoArgs[0] = MakeListPair("VENDOR", vendor);
   videoArgs[1] = MakeListPair("VENDORPR", vendorprompt);
   videoArgs[2] = MakeListPair("MODEL", model);
   videoArgs[3] = MakeListPair("MODELPR", modelprompt);
   videoList = TclListMerge(4, (const char **) videoArgs);

   for (i=0; i<4; i++)
      free (videoArgs[i]);

   return (videoList);
}


/*****************************************************************************
 *
 *  MakeResList()
 *
 *  Description:
 *  -----------
 *	Builds a video adapter resolution list based on the strings passed 
 *	in.  The Tcl list space _must_ be freed later by the calling function.
 *
 *  Parameters:
 *  -----------
 *	class - the class of video adapter for a particular vendor/model
 *	classprompt - prompt string corresponding to class
 *	mode - the mode of the video adapter for a particular class
 *	modeprompt - prompt string corresponding to mode
 *	comment - a comment string corresponding to a particular class/mode
 *
 *  Returns:
 *  -------
 *	Tcl list containing video adapter resolution information.
 *
 *****************************************************************************/
/* Called for each resolution in a grafinfo file */
char *
MakeResList(char *class,
	    char *classprompt,
	    char *mode, 
	    char *modeprompt,
	    char *comment)
{
   char *mlres[5];
   char *resList;
   int i;

   mlres[0] = MakeListPair("CLASS", class);
   mlres[1] = MakeListPair("CLASSPR", classprompt);
   mlres[2] = MakeListPair("MODE", mode);
   mlres[3] = MakeListPair("MODEPR", modeprompt);
   mlres[4] = MakeListPair("COMMENT", comment);

   resList = TclListMerge(5, (const char **) mlres);
   for (i=0; i<5; i++)
       free(mlres[i]);

   return(resList);
}

