/*
 *	@(#) main.c 12.3 95/07/25 
 *
 *	Copyright (C) The Santa Cruz Operation, 1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *      SCO MODIFICATION HISTORY
 *
 *      S000    Tue Jul 25 17:18:03 PDT 1995    davidw@sco.com
 *      - fixed warning on getopts.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include "graf.h"

#define TOLOWER(str) { char *tp; for (tp=str; *tp; ++tp) *tp=tolower(*tp); }

extern void ProcessFiles();
extern void ParseDir();
extern int LoadDevices();
extern void exit();

char vendor[STRLEN]; 
char vendorprompt[BUFLEN]; 
char model[STRLEN];
char modelprompt[BUFLEN];
char class[STRLEN];
char classprompt[BUFLEN];
char mode[STRLEN];
char modeprompt[BUFLEN];
char comment[4*BUFLEN];
char vidscript[BUFLEN];
char vidsetup[BUFLEN];

/* flags */
char Mflag;                      /* return all monitor descr */
char Tflag;                      /* return all tty devices */
char Vflag;                      /* return all adapter descr */
char mflag;                      /* given monitors, return descr */
char vflag;                      /* given adapters, return descr */


/*****************************************************************************
 *
 *  CheckTopDirs()
 *
 *  Description:
 *  -----------
 *	Verifies that GRAFINFO and MONINFO directories are installed.
 *
 *  Returns:
 *  -------
 *	Returns NOGRAFINFO/NOMONINFO if either directory is not installed.
 *
 *****************************************************************************/
static int
CheckTopDirs()
{
   DIR *dirp;

   if((dirp = opendir(GRAFINFO)) == NULL)
   {
      fprintf(stderr, "Can't open %s directory.\n", GRAFINFO);
      /* Empty line down stdout needed by tcl */
      fputs(NEWLINE, stdout);
      fflush(stdout);
      return(NOGRAFINFO);
   }
   closedir(dirp);

   if((dirp = opendir(MONINFO)) == NULL)
   {
      fprintf(stderr, "Can't open %s directory.\n", MONINFO);
      /* Empty line down stdout needed by tcl */
      fputs(NEWLINE, stdout);
      fflush(stdout);
      return(NOMONINFO);
   }
   closedir(dirp);

   return(OK);
}


/*****************************************************************************
 *
 *  CheckMatch()
 *
 *  Description:
 *  -----------
 *	Compares a tolower copy of the dirname.filename (eg. ibm.vga)
 *	to a tolower copy of vendor.model (eg. IBM.VGA) to make sure 
 *	they are identical.  If they aren't the same, they aren't used.
 *
 *  Returns:
 *  -------
 *	TRUE if they are the same. FALSE otherwise.
 *
 *****************************************************************************/
int
CheckMatch(char *dirname, char *filename)
{
    char vect[BUFLEN+1];
    char vect_compare[BUFLEN+1];

    sprintf(vect, "%s.%s", dirname, filename);
    sprintf(vect_compare, "%s.%s", vendor, model);

    TOLOWER(vect);
    TOLOWER(vect_compare);

    if (strcmp(vect, vect_compare) == 0)
       return(TRUE);
    else
       return(FALSE);
}


/*****************************************************************************
 *
 *  Usage()
 *
 *  Description:
 *  -----------
 *	Print out a usage message.  Default is -V if no other options
 *	are specified.
 *
 *  Returns:
 *  -------
 *	void
 *
 *****************************************************************************/
static void
Usage(void)
{
   (void) fprintf(stderr, "Usage: %s [-MTVmv]\n", PROGNAME);
   /* Empty line down stdout needed by tcl */
   fputs(NEWLINE, stdout);
   fflush(stdout);
   exit(USAGE);
}



/*****************************************************************************
 *
 *  GetOptions()
 *
 *  Description:
 *  -----------
 *	Gets the options passed in through argv.  Sets global option flags.
 *
 *  Returns:
 *  -------
 *	void
 *
 *****************************************************************************/
static void 
GetOptions(int argc, char **argv)
{
   int c;
   
   if (argc > 2) {
	fprintf(stderr, "Use only a single option per invocation.\n");
	/* Empty line down stdout needed by tcl */
	fputs(NEWLINE, stdout);
	fflush(stdout);
	Usage();
	/* NOTREACHED */
   }

   while ((c = getopt(argc, (char * const *) argv, "MTVmv")) != -1)
      switch(c) {
              case 'M' :
                         Mflag++;
                         break;
              case 'T' :
                         Tflag++;
                         break;
              case 'V' :
                         Vflag++;
                         break;
              case 'm' :
                         mflag++;
                         break;
              case 'v' :
                         vflag++;
                         break;
              case '?' :
                         Usage();
                         /* NOTREACHED */
      }
      /* default w/o options is Vflag */
      if (!Mflag && !mflag && !Tflag && !vflag)
	 Vflag++;
}


/*****************************************************************************
 *
 *  main()
 *
 *  Description:
 *  -----------
 *	Calls CheckTopDirs to verify that GRAFINFO and MONINFO are 
 *	installed.  Calls GetOptions to set the global options flags.
 *	Based on the options, calls the appropriate routines.
 *
 *  Returns:
 *  -------
 *	void
 *
 *****************************************************************************/
void
main(int argc, char **argv)
{
   int status = OK;

   GetOptions(argc, argv);

   if ((status = CheckTopDirs()) != OK)
       exit(status);

   if (mflag || vflag)
       ProcessFiles();
   else if (Mflag) {
	ParseDir(MONINFO, MON);
   }
   else if (Vflag) {
	ParseDir(GRAFINFO, XGI);
   }
   else if (Tflag) {
	if ((status = LoadDevices(DEVICES)) != OK)
	   exit(status);
   }
   exit(status);
}

