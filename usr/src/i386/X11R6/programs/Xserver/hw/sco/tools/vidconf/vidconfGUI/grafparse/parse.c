/*
 *	@(#) parse.c 12.2 95/07/24 
 *
 *	Copyright (C) The Santa Cruz Operation, 1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *      SCO MODIFICATION HISTORY
 *
 *      S000    Thu Jan 12 11:57:03 PST 1995    davidw@sco.com
 *      - Use case-insensitive sorting.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "graf.h"

/* tcl list merging includes */
#include "basicIncl.h"
#include "tclList.h"

extern char *strdup();
extern char vendor[];
extern char vendorprompt[];
extern char model[];
extern char modelprompt[];
extern char class[];
extern char classprompt[];
extern char mode[];
extern char modeprompt[];
extern char comment[];
extern char vidscript[];
extern char vidsetup[];

extern int CheckMatch();
extern char *MakeListPair();
extern char *MakeResList();
extern char *MakeVideoList();
extern char *MakeMonList();

/* flags */
extern char mflag;                      /* given monitors, return descr */
extern char vflag;                      /* given adapters, return descr */

/* 
 * Since the options do not allow a video and 
 * monitor search at the same time, the struct below
 * holds either the collected video data or monitor 
 * data.  However, if we are looking for monitor data, 
 * vendorprompt and modelprompt are not used.
 */
struct vmdata {
   char vendor[STRLEN];
   char vendorprompt[BUFLEN];
   char model[STRLEN];
   char modelprompt[BUFLEN];
   char prompt[BUFLEN];
} vmData[MAXFILES];

/* Mapping array to quickly sort without having to move data */
int vmMap[MAXFILES];

#define TOLOWER(str) { char *tp; for (tp=str; *tp; ++tp) *tp=tolower(*tp); }


/*************************************************************************
 *
 *  GetXGIheader() 
 *
 *  Description:
 *  -----------
 *	Lex is used to parse the XGI file pointed to by fp. 
 *	Called repeatedly for each Resolution in the grafinfo file.
 *	Searchs for vendor, model, class, mode and corresponding 
 *	strings.  Gets the comment and vidscript/vidsetup strings if any.  
 *
 *  Note:
 *  ----
 *  Much of this code was lifted from vidconf/getlist.c.  However, 
 *  all bugs are mine.
 *
 *  Returns:
 *  --------
 *	TRUE if we found the header information we needed. FALSE otherwise.
 *
 *************************************************************************/
static int
GetXGIheader(FILE *fp)
{
    extern FILE *yyin;
    extern char yytext[];

    int token;
    static lastToken = 0;	/* Remember if last token was VENDOR */
    int classChanged = FALSE;	/* used to help know when to reset comment[] */
    int modeChanged = FALSE;	/* used to help know when to reset comment[] */

    vendor[0] = vendorprompt[0] = model[0] = modelprompt[0] = CNULL;
    class[0] = classprompt[0] = mode[0] = modeprompt[0] = CNULL;
    comment[0] = vidscript[0] = vidsetup[0] = CNULL;

    yyin = fp;

    /* 
     * To stay in sync...
     * Couldn't find a yyuntoken() to put back the VENDOR token that
     * we stopped last time on.  So, track that we read it and start
     * with it rather then a yylex.
     */
    if (lastToken == VENDOR) { 
	token = lastToken;
	lastToken = 0;
    }
    else
    	token = yylex();

    while ( !feof(fp) )
    {
	switch (token)
	{
	    case VENDOR:
		if ((classChanged == TRUE) || (modeChanged == TRUE)) {
	            /* we're into the next entry - get out of here */
		    lastToken = VENDOR;
		    return (TRUE);
		}

                if ((token = yylex()) != IDENTIFIER)
                    return(FALSE);
                else
                    strcpy(vendor, yytext);

                if ((token = yylex()) == PROMPT)
                {
                    strcpy(vendorprompt, yytext);
                    token = yylex();
                }
                else
                    strcpy(vendorprompt, vendor);
                break;

	    case MODEL:
               if ((token = yylex()) != IDENTIFIER)
                    return(FALSE);
                else
                    strcpy(model, yytext);

                if ((token = yylex()) == PROMPT)
                {
                    strcpy(modelprompt, yytext);
                    token = yylex();
                }
                else
                    strcpy(modelprompt, model);
                break;

	    case CLASS:
		if ((token = yylex()) != IDENTIFIER)
		    return(FALSE);
		else
		    strcpy(class, yytext);

		if ((token = yylex()) == PROMPT)
		{
		    strcpy(classprompt, yytext);
		    token = yylex();
		}
		else
		    strcpy(classprompt, class);
		classChanged = TRUE;
		break;

	    case MODE:
		if ((token = yylex()) != IDENTIFIER)
		    return(FALSE);

		/* do not show DUMMY entries */
		if (strcmp(yytext, "DUMMY") == 0) {
		    classChanged = FALSE;
		    class[0] = CNULL;
		    mode[0] = CNULL;		    
		    break;
		}

		strcpy(mode, yytext);

		if ((token = yylex()) == PROMPT)
		{
		    strcpy(modeprompt, yytext);
		    token = yylex();
		}
		else
		    strcpy(modeprompt, mode);
		modeChanged = TRUE;
		break;

	    case DATA:
		if ((((classChanged == TRUE) && (modeChanged == TRUE)) ||
		    ((classChanged == TRUE) && (mode[0] != CNULL)) ||
		    ((class[0] != CNULL) && (modeChanged == TRUE))) &&
		    (comment[0] != CNULL))
		    return(TRUE);
		else
		    token = yylex();
	        break;
		
	    case VIDSCRIPT:
                /* Read in the EQUALS character */
                if (yylex() != EQUAL) {
                    return(FALSE);
                }

                /* Read in the script file name */
                if (yylex() == PROMPT)
                {
                    strcpy(vidscript, yytext);
                }
		token = yylex ();
                break;

	    case VIDSETUP:
                /* Read in the EQUALS character */
                if (yylex() != EQUAL) {
                    return(FALSE);
                }

                /* Read in the script file name */
                if (yylex() == PROMPT)
                {
                    strcpy(vidsetup, yytext);
                }
		token = yylex ();
                break;

	    /* check for a long comment */
	    case IDENTIFIER:
		if (strcmp(yytext, "COMMENT") == 0) {
		    if (yylex () != EQUAL)
		    	return (FALSE);
		    if (yylex () != PROMPT)
			return (FALSE);
		    strcpy (comment, yytext);
		}
		token = yylex ();
		break;

	    default:
		token = yylex();
	        break;
	}
    }
    if ((classChanged == TRUE) || (modeChanged == TRUE))
	return (TRUE);
    return(FALSE);
}


/*************************************************************************
 *
 *  GetVendorModel() 
 *
 *  Description:
 *  -----------
 *	Shorter version of GetXGIheader that returns vendor, 
 *	vendorprompt, model, and modelprompt strings in list format.
 *
 *  Note:
 *  ----
 *  Portions of this code were lifted from vidconf/getlist.c.  However, 
 *  all bugs are mine.
 *
 *  Returns:
 *  --------
 *	TRUE if everything parses okay.  FALSE otherwise.
 *
 *************************************************************************/
static int
GetVendorModel (FILE *fp)
{
    extern FILE *yyin;
    extern char yytext[];

    int token;

    vendor[0] = vendorprompt[0] = model[0] = modelprompt[0] = CNULL;

    yyin = fp;
    token = yylex();
    /*
     * stop after EOF or
     * after vendorprompt and modelprompt have been read in.
     */
    while (((vendorprompt[0] == CNULL) || (modelprompt[0] == CNULL)) &&
    	 !feof(fp) )
    {
	switch (token)
	{
	    case VENDOR:
		if ((token = yylex()) != IDENTIFIER)
		    return(FALSE);
		else
		    strcpy(vendor, yytext);

		if ((token = yylex()) == PROMPT)
		{
		    strcpy(vendorprompt, yytext);
		    token = yylex();
		}
		else
		    strcpy(vendorprompt, vendor);
		break;

	    case MODEL:
		if ((token = yylex()) != IDENTIFIER)
		    return(FALSE);
		else
		    strcpy(model, yytext);

		if ((token = yylex()) == PROMPT)
		{
		    strcpy(modelprompt, yytext);
		    token = yylex();
		}
		else
		    strcpy(modelprompt, model);
		break;

	    default:
		token = yylex();
	        break;
	}
    }

    if ((vendorprompt[0] == CNULL) || (modelprompt[0] == CNULL))
        return(FALSE);
    else
        return(TRUE);
}


/*****************************************************************************
 *
 *  GetMON()
 *
 *  Description:
 *  -----------
 *	Lex is used to parse the MON file pointed to by fp. 
 *	Searchs for MONVENDOR, MONMODEL and DESCRIPTION.
 *
 *  Note:
 *  ----
 *  Much of this code was lifted from vidconf/getlist.c.  However, 
 *  all bugs are mine.
 *
 *  Returns:
 *  -------
 *	void
 *
 *****************************************************************************/
static int
GetMON(FILE *fp)
{
    extern FILE *yyin;
    extern char yytext[];

    int token;

    vendor[0] = 
    vendorprompt[0] = 
    model[0] = 
    modelprompt[0] = 
    comment[0] = CNULL;

    yyin = fp;
    while (token = yylex())
    {
 	if (token != IDENTIFIER)
		continue;

	if (strcmp(yytext, "DESCRIPTION") == 0)
	{
		if (yylex() != EQUAL) {
			fprintf (stderr, "Missing =\n");
			continue;
		}
		if (yylex() != PROMPT) {
			fprintf (stderr, "Missing string\n");
			continue;
		}
		strcpy (comment, yytext);
	}
	else if (strcmp(yytext, "MON_VENDOR") == 0)
	{
		if (yylex() != EQUAL) {
			fprintf (stderr, "Missing =\n");
			continue;
		}
		if (yylex() != PROMPT) {
			fprintf (stderr, "Missing string\n");
			continue;
		}
		strcpy (vendor, yytext);
	}
	else if (strcmp(yytext, "MON_MODEL") == 0)
	{
		if (yylex() != EQUAL) {
			fprintf (stderr, "Missing =\n");
			continue;
		}
		if (yylex() != PROMPT) {
			fprintf (stderr, "Missing string\n");
			continue;
		}
		strcpy (model, yytext);
	}
    }

    if ( (vendor[0] == CNULL) && 
	 (model[0] == CNULL) &&
	 (comment[0] == CNULL) )
       return(FALSE);
    else
       return(TRUE);
}


/*************************************************************************
 *
 *  LoadDevices()
 *
 *  Description:
 *  -----------
 *  Reads in the TTY devices from the specified directory.  By default
 *  the DEVICES directory is /usr/lib/vidconf/devices.  Every file in 
 *  the DEVICES directory is opened and the first line is read.  This 
 *  line should correspond to the full path name of the device that it 
 *  represents.  A stat(S) is done on the full path name to verify the 
 *  device exists.
 *
 *  Parameters:
 *  ----------
 *  directory - directory from which devices are obtained. Used for 
 *  recursively checking subdirectories.
 *
 *  Notes:
 *  -----
 *  - LoadDevices is recursively called to handle subdirectories 
 *  which is why the directory is passed in.  I have never seen 
 *  /usr/lib/vidconf/devices have sub directories but the code 
 *  allows it.
 *
 *  - Much of this code was lifted from vidconf/grafdev.c.  However, 
 *  all bugs are mine.
 *
 *  Returns:
 *  -------
 *  Returns OK upon successful completion, otherwise NOTOK.
 *
 *************************************************************************/
int
LoadDevices (char *directory)
{
    FILE		*fp;
    DIR   		*tty_dirp;
    struct stat         stat_buf;
    struct dirent    	*dir_entryp;
    char   		tty_buff[BUFLEN];
    char		device[BUFLEN];
    char		*base, name[BUFLEN];


    if ((tty_dirp = opendir(directory)) == NULL)
    {
        fprintf(stderr, "Can't open %s directory.\n\n", directory);
	/* Empty line down stdout needed by tcl */
        fputs(NEWLINE, stdout);
	fflush(stdout);
	return(NOTOK); 
    } 
    else while ((dir_entryp = readdir(tty_dirp)) != NULL)
    {

	sprintf (tty_buff, "%s/%s", directory, dir_entryp->d_name);

       /*
	* Skip . and ..
	*/
        if (strncmp(dir_entryp->d_name, ".", 1) == 0)
	    continue;

  	else if (stat(tty_buff, &stat_buf) < 0)
	    continue;

	else if ((stat_buf.st_mode & S_IFMT) == S_IFDIR) 
	{
	    /* Its recursive ... */
	    if (LoadDevices(tty_buff) == NOTOK)
		return(NOTOK);
	    else
	        continue;
	}

	name[0] = '\0';
	/* get devices from 'Devices' directory */
        if (((fp = fopen(tty_buff, "r")) != NULL) && 
	    (fscanf(fp, "%s%s\n", device, name) > 0))
	{
	   /*
	    * Verify the device exists.
	    */
	    if (stat(device, &stat_buf) < 0)
		continue;

	    fclose(fp);

            fputs(device, stdout);
            fputs(" ", stdout);
	    if (name[0] != '\0')
               fputs(name, stdout);
	    else {
	       /* name was missing - print device file name without path */
	       if ((base = strrchr (device, '/')) != NULL)
                  fputs(++base, stdout);
	       else
               	  fputs(device, stdout);
	    }
            fputs(NEWLINE, stdout);
        }
    }

    closedir(tty_dirp);
    return(OK);
}


/*****************************************************************************
 *
 *  ProcessFiles()
 *
 *  Description:
 *  -----------
 *	Handles -v and -m options.  Reads directory and filename pairs 
 *	a line at a time from stdin.  Opens the corresponding .xgi or 
 *	.mon file as appropriate.  Collects the header information for 
 *	each file.  Makes sure that the directory and filename pair 
 *	matches the VENDOR (or MON_VENDOR) and MODEL (or MON_MODEL) pulled 
 *	from the file.  Sends this header information to stdout in list
 *	format.  If 'quit' or 'exit' is entered as the first string, exit
 *	otherwise print a message.
 *	
 *	-v:
 *	Used to get VENDOR, MODEL, CLASS and MODE's as well as the 
 *	corresponding prompt and COMMENT strings from XGI files.
 *	
 *	-m: 
 *	Used to get MON_VENDOR and MON_MODEL's as well as the 
 *	DESCRIPTION string from MON files.
 *
 *  Returns:
 *  -------
 *	void
 *
 *****************************************************************************/
void
ProcessFiles()
{
   FILE *fp;
   char line[BUFLEN];
   char filename[STRLEN];
   char dirpart[BUFLEN];
   char filepart[STRLEN];
   char *monList, *resList, *videoList;
#define VSIZE 7
   char *videoArgs[VSIZE], *resArgv[MAXRESLINES];
   int i, cnt;

   dirpart[0] = CNULL; filepart[0] = CNULL; line[0] = CNULL;
   while ( (fgets(line, BUFLEN, stdin)) != NULL )
   {
      if (sscanf(line, "%s%s", dirpart, filepart) != 2) {
	 if ( !strcmp(dirpart, "quit") || !strcmp(dirpart, "exit") ) {
	    exit (OK);
	 }
         fprintf(stderr, "Expected \"directory filename\" pair.\n");
	 /* Empty line down stdout needed by tcl */
         fputs(NEWLINE, stdout);
	 fflush(stdout);
         continue;
      }

      if (vflag) 
	 sprintf(filename, "%s/%s/%s%s", GRAFINFO, dirpart, filepart, XGI);
      else if (mflag)
	 sprintf(filename, "%s/%s/%s%s", MONINFO, dirpart, filepart, MON);

      if ((fp = fopen(filename, "r")) == FNULL)
      {
	 fprintf(stderr, "Can't open %s.\n", filename);
	 /* Empty line down stdout needed by tcl */
         fputs(NEWLINE, stdout);
	 fflush(stdout);
	 continue;
      }

      /* not reached if file fails to open above */
      if (vflag) 
      {
	 for (i=0; i<VSIZE-1; i++) videoArgs[i] = NULL;
	 cnt = 0;

	 while ( !feof(fp) )
         {
	    /* Process each entry in the grafinfo file, one at a time */
	    if ( GetXGIheader(fp) && CheckMatch(dirpart, filepart) )
	    {
		/* grafinfo doesn't like caps */
		TOLOWER(class);
		TOLOWER(mode);

		/* only need to set up once. vendor/model don't change */
		if (videoArgs[0] == NULL)
		{
		   videoArgs[0] = MakeListPair("VENDOR", dirpart);
		   videoArgs[1] = MakeListPair("VENDORPR", vendorprompt);
		   videoArgs[2] = MakeListPair("MODEL", filepart);
		   videoArgs[3] = MakeListPair("MODELPR", modelprompt);
		}

		if (vidscript[0] != CNULL) 
		{
		   /* save only the first vidscript found in the file */
		   if (videoArgs[4] == NULL) 
		   {
		      videoArgs[4] = MakeListPair("VIDSCRIPT", vidscript);
		   }
		}

		if (vidsetup[0] != CNULL) 
		{
		   /* save only the first vidsetup found in the file */
		   if (videoArgs[5] == NULL) 
		   {
		      videoArgs[5] = MakeListPair("VIDSETUP", vidsetup);
		   }
		}

		/* get each of the resolutions out of .xgi file */
		resArgv[cnt] = MakeResList(class, classprompt,
					   mode, modeprompt, comment);
		/* bail before we overflow resArgv */
		if (cnt >= MAXRESLINES-1) {
		   break;
		}
		cnt++;
	     }
 	 }
	 if (cnt) /* build lists only if we got resolutions or a vidscript */
	 {
	    /* EOF for .xgi file - no more resolutions - Make the list */
	
	    /* combine all resolutions */
	    resList = TclListMerge(cnt, (const char **) resArgv);
	    /* if vidscript wasn't found above, create {VIDSCRIPT {}} */
	    if (videoArgs[4] == NULL) 
		videoArgs[4] = MakeListPair("VIDSCRIPT", vidscript);
	    /* if vidsetup wasn't found above, create {VIDSETUP {}} */
	    if (videoArgs[5] == NULL) 
		videoArgs[5] = MakeListPair("VIDSETUP", vidsetup);
	    videoArgs[6] = MakeListPair("RESOLUTIONS", resList);
	    videoList = TclListMerge(VSIZE, (const char **) videoArgs);
	    fputs(videoList, stdout);
            fputs(NEWLINE, stdout); /* Mark end of list with blank line */
	    fflush(stdout);
   
	    free(resList); free(videoList);
	    for (i=0; i<(cnt); i++) free(resArgv[i]);
	    for (i=0; i<VSIZE-1; i++) free(videoArgs[i]);

	    /* let em know we overflowed resArgv */
	    if (cnt >= MAXRESLINES-1) {
	       fprintf(stderr, 
	          "\nWarning: Maximum number of resolutions (%d) read from the file\n%s.\n",
		  cnt+1, filename);
	    }
	 }
      }
      else /* if (mflag) */
      {
	 monList = NULL;
	 if ( GetMON(fp) && CheckMatch(dirpart, filepart) )
	 {
	        monList = MakeMonList(dirpart, filepart, comment);
		fputs(monList, stdout);
      		fputs(NEWLINE, stdout); /* Mark end of list with blank line */
		fflush(stdout);

		free(monList);
	 }
      }
      fputs(NEWLINE, stdout); /* Mark end of list with blank line */
      fflush(stdout);
      fclose(fp);
      dirpart[0] = CNULL; filepart[0] = CNULL; line[0] = CNULL;
   }
}


/*****************************************************************************
 *
 *  SortData()
 *
 *  Description:
 *  -----------
 * 	Sorts the data held in the prompt field by moving indexes 
 *	within a Map array.  The prompt field holds either the 
 *	vendorprompt and modelprompt (combined) from within an XGI file
 *	or it holds the description from within a MON file.  
 *
 *	To print the data in sorted order, simply walk through the Map 
 *	array and use the ordered indexes to access the Data.
 * 
 *	This routine is similar to how vectors were sorted in the 
 *	original vidconf program in the getlist.c:dblvecsort() routine.
 *
 *  Returns:
 *  -------
 *	void
 *
 *****************************************************************************/
void
SortData(Data, Map, nelems)
struct vmdata * Data;
int * Map;
int nelems;
{
   int count;
   int i, j;
   int temp;
   char prompt1[BUFLEN], prompt2[BUFLEN];

   for (count = nelems / 2; count > 0; count /= 2)
   {
      for (i = count; i < nelems; i++)
      {
          for (j = i - count; j >= 0; j -= count)
          {
	      strcpy(prompt1, Data[Map[j]].prompt);		/* S000 vvv*/
	      strcpy(prompt2, Data[Map[j + count]].prompt);
	      TOLOWER(prompt1);
	      TOLOWER(prompt2);
              if (strcmp(prompt1, prompt2) <= 0)		/* S000 ^^^*/
                  break;

              temp = Map[j];
              Map[j] = Map[j + count];
              Map[j + count] = temp;
            }
        }
    }
}


/*****************************************************************************
 *
 *  PrintList()
 *
 *  Description:
 *  -----------
 * 	Walk through the Map array to retrieve each sorted index.
 * 	Obtain the data by using this sorted index to index into
 *	the data array.  Make and then print the list.  
 *
 *  Returns:
 *  -------
 *	void
 *
 *****************************************************************************/
void
PrintList(nelems, suffix) 
int nelems;
char * suffix;
{
   int i, index;
   char * List;

   for(i = 0; i < nelems; i++)
   {
      List = NULL;
      index = vmMap[i];	/* Use sorted map index to print in sorted order */

      if (strcmp(XGI, suffix) == 0)
      {
	 List = MakeVideoList(vmData[index].vendor, 
			vmData[index].vendorprompt, 
			vmData[index].model, 
			vmData[index].modelprompt);
      }
      else if (strcmp(MON, suffix) == 0)
      {
         List = MakeMonList(vmData[index].vendor, vmData[index].model,
			vmData[index].prompt);
      }
      fputs(List, stdout);
      /* mark end of list with blank line */
      fputs(NEWLINE, stdout);

      free(List);
   }
   /* flush it all out */
   fflush(stdout);
}


/*****************************************************************************
 *
 *  ParseDir()
 *
 *  Description:
 *  -----------
 *	Parses GRAFINFO heirarchy searching for XGI suffix files.
 * 	For each of these files found, GetVendorModel is called to
 *	obtain the vendor, vendorprompt, model and modelprompt.  
 *	The strings are sent to stdout in list format.
 *
 * 	If the MONINFO heirarchy is parsed, MON files are searched for.
 *
 *	Handles both -V and -M flags.
 *
 *  Returns:
 *  -------
 *	void
 *
 *****************************************************************************/
void
ParseDir(char *topdir, char *suffix)
{
    FILE *fp;
    struct stat sb;
    struct dirent *dp;
    struct dirent *sdp;
    DIR * dirp;
    DIR * subdirp;
    char * file_nosuffix;
    char filename[STRLEN];
    char dirname[BUFLEN];
    int vmcnt = 0;

    if ((dirp = opendir(topdir)) == NULL)
    {
	/* this should never fail cause CheckTopDirs was okay */
	fprintf(stderr, "Can't open %s directory.\n", topdir);
	exit(NOTOK);
    }

    /* for each entry in the $topdir directory */
    while ((dp = readdir(dirp)) != NULL)
    {
	/* ignore . and .. */
	if (dp->d_name[0] == '.')
	    continue;

        /* if the entry is a directory */
	sprintf(dirname, "%s/%s", topdir, dp->d_name);
	stat(dirname, &sb);
	if ((sb.st_mode & S_IFMT) ==  S_IFDIR)
	{
	    if ((subdirp = opendir(dirname)) == NULL)
		continue;

	    /* for each subdirectory entry */
	    while ((sdp = readdir(subdirp)) != NULL)
	    {
		/* ignore . and .. */
		if (sdp->d_name[0] == '.')
		    continue;

		/* if the entry is the same as suffix */
		if (strncmp(sdp->d_name+(strlen(sdp->d_name)-4),suffix, 4) == 0)
		{

		    sprintf(filename, "%s/%s",dirname, sdp->d_name);
		    if ((fp = fopen(filename, "r")) == FNULL) 
		    {
			fprintf(stderr, 
				"Can't open %s.\n", filename);
			continue;
		    }

		    /* checking for either XGI or MON ... which is it? */
		    if (strcmp(XGI, suffix) == 0)
		    {
                       if (GetVendorModel(fp) != FALSE) 
		       {
    			  /*
     			   * Check that vendor and model specified in 
			   * file agrees with name of file, otherwise 
			   * go to the next file.
     			   */
		          /* file_nosuffix == model */
		          file_nosuffix = strdup(sdp->d_name);
		          file_nosuffix[strlen(file_nosuffix)-4] = '\0';
		       	  /* dp->d_name == vendor */

      			  /* compare/save using original dir and file name */
			  if (CheckMatch(dp->d_name, file_nosuffix) == TRUE) 
			  {
			      vmMap[vmcnt] = vmcnt;
			      strcpy(vmData[vmcnt].vendor, dp->d_name);
			      strcpy(vmData[vmcnt].vendorprompt, 
					vendorprompt);
			      strcpy(vmData[vmcnt].model, file_nosuffix);
			      strcpy(vmData[vmcnt].modelprompt, modelprompt);
			      strcpy(vmData[vmcnt].prompt, vendorprompt);
			      strcat(vmData[vmcnt].prompt, " ");
			      strcat(vmData[vmcnt].prompt, modelprompt);
			      ++vmcnt;
			  }
		       }
		    }
		    else if (strcmp(MON, suffix) == 0)
		    {
                       if (GetMON(fp) != FALSE) 
		       {
    			  /*
     			   * Check that vendor and model specified in 
			   * file agrees with name of file, otherwise 
			   * go to the next file.
     			   */
		          /* file_nosuffix == model */
		          file_nosuffix = strdup(sdp->d_name);
		          file_nosuffix[strlen(file_nosuffix)-4] = '\0';
		       	  /* dp->d_name == vendor */
      			  /* compare/save using original dir and file name */
			  if (CheckMatch(dp->d_name, file_nosuffix) == TRUE)
			  {
			      vmMap[vmcnt] = vmcnt;
			      strcpy(vmData[vmcnt].vendor, dp->d_name);
			      strcpy(vmData[vmcnt].model, file_nosuffix);
			      /* You might wonder how a string stored in 
			       * comment could fit into prompt.  Well, in 
			       * the case of MON files, comment is actually
			       * only holding the description, it will never 
			       * be larger then BUFLEN.  Conserving space.
			       * Hope you don't hate me :)
			       */
			      strcpy(vmData[vmcnt].prompt, comment);
			      ++vmcnt;
			  }
		       }
		    }
		    fclose(fp);
		}
	    }
	    closedir(subdirp);
	}
    }
    closedir(dirp);

    /* Sorts either video or monitor data */
    SortData(vmData, vmMap, vmcnt);

    PrintList(vmcnt, suffix);
}


