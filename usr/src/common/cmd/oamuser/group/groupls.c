#ident  "@(#)groupls.c	1.4"
#ident  "$Header$"

/*
*
* Command: userls
*
* Specified in POSIX Part 3: User and Group Account Management
*
* Options Supported :
*
* -a        Writes all options associated with specified groups to stdout
*           in extended option syntax.
*
* -d        Selects group accounts with duplicate gid's.
*
* -o        Formats output, one line per account, into colon-delimeted option
*           values. The output should be in the order that the options were
*           specified.
*
* -g group  Display groups.
*
* Options Not Supported :
*
*   -D          Lists the system-wide defaults for specified options.
*
*   -p          Selects group accounts with no passwords.
*
*   -c cell     Specifies the cell to report on.
*
*   -S sysname  Specifies name of the system.
*
*/

/* Headers */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <userdefs.h>
#include <limits.h>
#include <locale.h>
#include <pfmt.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include "gid.h"
#include "messages.h"
#include "getxopt.h"

/* Defines */

static void display_groups ();


/* Externs */

extern char *optarg, 		/* Used by getopt() */
			*errmsgs[];		/* Used by l10n. In messages.c */

extern int getopt(), getxopt(), errmsg();
extern void bzero();

extern	char *strcat(char *s1, const char *s2);
extern	char *strncat(char *s1, const char *s2, size_t n);
extern	int strcmp(const char *s1, const char *s2);
extern	char *strcpy(char *s1, const char *s2);
extern	char *strtok(char *s1, const char *s2);

extern  struct  group   *getgrnam(),
						*getgrent();
extern int errno;

/* Misc. */

char *msg_label = "UX:groupls";

static int oflag, dflag, aflag; 	/* Various option flags */

static struct group *grpstruct = NULL ;	/* Will be filled with entries from 
							 		 * /etc/group.
							 		 */

static char *xoptattrstring[7];    /* Can only be a maximum of six valid */

main(argc, argv)
int argc;
char *argv[];
{
	void display_groups();
	int opt=0, offset=0, return_code=0, firsttimeflag=1, bytesread=0;
	int end_of_group_file =0, exclusive_option=0, errflag=0;

	int extfd;                  /* File Descriptor for extended options file */
	int count=0;				/* Counter */
	
	gid_t gid;					/* GID */

	char *groupnames = NULL;	/* Group name from the command line */
	char *cellname = NULL;		/* Cell name from the command line */
	char *group = NULL;			/* Group */
	const char *token = NULL;	/* strtok() token */
	char *sysname = NULL;		/* System name from the command line */
	static char *xoptstring = NULL; /* extended options from the command line */
	static char *Xoptstring = NULL; /* extended options file from the command 	
								 	 * line 
									 */
	char *ptr;					/* Pointer for strtol */

	char filebuf[128];
	size_t totalbytesread;

	DpaExtStrToken *exttoken =  NULL ;/* Extended options (-x) structure */



	oflag = dflag = aflag = 0;	/* Zero flags */

	/* Set up the l10n stuff */

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore.abi");
	(void) setlabel(msg_label);

	/* Parse the options */

	while((opt = getopt(argc, argv, "adoDpRg:S:x:X:c:")) != EOF)
		switch(opt) {

			case 'a':
				aflag++;
				break;
			
			case 'd':
				dflag++;
				break;

			case 'o':
				oflag++;
				break;

			case 'g':
				groupnames = optarg;
				break;

			case 'S':
				sysname = optarg;
				break;

			case 'x':
				xoptstring = optarg;
				break;

			case 'X':
				Xoptstring = optarg;
				break;

			case 'c':
				cellname = optarg;
				break;

			case 'D':
			case 'p':
			case 'R':
				errmsg (M_UNSUPPORTED_OPTN);
				exit (EX_SYNTAX);

			case '?':
				errmsg( M_GROUPLS_USAGE );
				exit( EX_SYNTAX );
		}
	
	if ( (exclusive_option > 1) || (dflag && (oflag || aflag)) ) {
		errmsg( M_GROUPLS_USAGE );
		exit( EX_SYNTAX );
	}
	
	if (dflag) {
		
		errmsg( M_DUPGRP );
		exit (EX_FAILURE);
	}

	if (sysname != NULL) {
		/* We do not have sysname at the moment */
		errmsg (M_SYSNAME);
		exit (EX_FAILURE);
	}

	if (cellname != NULL) {
		/* We do not have cells at the moment */
		errmsg (M_CELLNAME);
		exit (EX_FAILURE);
	}

	

	if (Xoptstring != NULL)
	{
		/* We have a file containing extended options. We will open and read th
		 * file into xoptstring, and process it below.
		 */

		extfd = open (Xoptstring, O_RDONLY);
		
		if (extfd == -1)
		{
			errmsg (M_XTENDED_OPT_FILE_DOES_NOT_EXIST,Xoptstring);
			exit (EX_FAILURE);
		}

		bzero (filebuf,128);

        while ( (bytesread = read (extfd, filebuf, 128)) != 0)
        {
			totalbytesread = (strlen(xoptstring) + bytesread);
			xoptstring = (char *) realloc (xoptstring,totalbytesread);
			if (firsttimeflag)
			{
				bzero (xoptstring,totalbytesread);
				firsttimeflag=0;
			}
			xoptstring = strncat (xoptstring,filebuf,bytesread);
			bzero (filebuf,128);
		}
	}

	
	if (xoptstring != NULL)
	{
		/* We have a string of extended options, go and get them */

		while ( (return_code = getxopt(xoptstring, &offset, &exttoken)) == 0)	
		{
			
			xoptattrstring[count] = (char *) malloc (strlen 
											(exttoken->attr_name));

			xoptattrstring[count] = strcpy (xoptattrstring[count], 
												exttoken->attr_name);

			if ((strcmp (xoptattrstring[count],"gr_name") != 0) &&
			(strcmp (xoptattrstring[count], "gr_gid") != 0) &&
			(strcmp (xoptattrstring[count], "gr_mem") != 0))
			{
				errmsg (M_INVALID_XTENDED_OPT,xoptattrstring[count]);
				exit (EX_FAILURE);
			}
			count++;
		}

		if ( return_code != -1 ) /* Error happened */
		{
			if (return_code == -2) errmsg (M_EXTENDED_OPT_ERROR_2);
			if (return_code == -3) errmsg (M_EXTENDED_OPT_ERROR_3);
			if (return_code == -4) errmsg (M_EXTENDED_OPT_ERROR_4);
			if (return_code == -5) errmsg (M_EXTENDED_OPT_ERROR_5);
			if (return_code == -6) errmsg (M_EXTENDED_OPT_ERROR_6);
			if (return_code == -7) errmsg (M_EXTENDED_OPT_ERROR_7);
			
			exit (EX_FAILURE);
		}

		xoptattrstring[count] = (char *) malloc(1);
		xoptattrstring[count] = strcpy (xoptattrstring[count], NULL);
	}

	if (argc == 1)
	{
		/* The POSIX spec says that by default information is provided about 
		 * groups on the specified system or cell. Since we do not support 
		 * either the -S system name OR -c cell options, by default we will
		 * output a list of groups configured on the system.
		 */
		
		
		while (!end_of_group_file) {
			if ((grpstruct = getgrent()) != NULL) {
                (void) fprintf (stdout,"%s\n",grpstruct->gr_name);
            }
            else {
                if (errno == 0)
                    end_of_group_file = 1;
                else {
                    if (errno == EINVAL) errno = 0;
                        else end_of_group_file = 1;
                }
            }
        }

		exit (EX_SUCCESS);
	}


	if ( groupnames != NULL) {
		/* We examine groupnames to get groups, user can specify more than
		 * one group each delimeted by commas. e.g -g root,sys,....
		 */

		grpstruct = NULL;
		token = strtok (groupnames,",");
		group = (char *) malloc (strlen(token));
		group = strcpy (group,token) ;
		grpstruct = getgrnam(group);

		if ( grpstruct != NULL ) {

			/* We got a valid group, print it out */
			display_groups();

		}
		else
		{
			/* getgrnam returned an error cos it couldn't find the group.
			 * However the group could have been specified as a gid rather
			 * than a name, so try this case before returning an error to
			 * the user.
			 */

			grpstruct = NULL;

			gid = (gid_t) strtol(group, &ptr, 10);

			if ( (gid == LONG_MAX) ||  (gid == LONG_MIN) 
						|| (strcmp (group, ptr) == 0) ) {
				errflag++;
				errmsg( M_GRP_INVALID, group );
			}
			else
			{	
				grpstruct = getgrgid (gid); 

				if ( grpstruct == NULL ) {	
					errflag++;
					errmsg (M_GID_INVALID, group);
				}
				else
				{
					display_groups();
				}
			}
		}

		/* Complete the rest of the groups in groupnames in the same way */

		while ( (token = strtok (NULL,",")) != NULL)
		{
			free (group);
			group = (char *) malloc (strlen(token));
			group = strcpy (group,token) ;

			grpstruct = NULL;
			grpstruct = getgrnam (group);

			if ( grpstruct != NULL ) {

				/* We got a valid group, print it out */
				display_groups();
			}
			else
			{

			/* getgrnam returned an error cos it couldn't find the group.
			 * However the group could have been specified as a gid rather
			 * than a name, so try this case before returning an error to
			 * the user.
			 */

				grpstruct  = NULL;
				ptr = NULL;

				gid = (gid_t) strtol(group, &ptr, 10);

				if ( (gid == LONG_MAX) ||  (gid == LONG_MIN)
						|| (strcmp (group, ptr) == 0) ) {
					errflag++;
					errmsg( M_GRP_INVALID, group );
				}
				else
				{
					grpstruct = getgrgid (gid);

					if ( grpstruct == NULL ) 
					{	
						errflag++;
						errmsg (M_GID_INVALID, group );
					}
					else
					{
						display_groups();
					}
				}
			}
		}
	}
	
	if (errflag == 0) exit(EX_SUCCESS);
		else  exit(EX_FAILURE);
}

static void display_groups()
{
	int count=0;

	/* Prints out the group as required, this code is used a lot by main()
	 * and is here to avoid duplication.
	 */

	/* No need to l10n this. The output would be the same in all languages.*/

	if (oflag)
		(void) fprintf (stdout,"%s:", grpstruct->gr_name);
	else
		(void) fprintf (stdout,"gr_name=%s ", grpstruct->gr_name);

	if (xoptattrstring[0] != NULL)
	{
		while ((strcmp (xoptattrstring[count], NULL)) != 0)
		{
			if (( strcmp (xoptattrstring[count],"gr_mem")) == 0)
			{
				if (!oflag) (void) fprintf (stdout," gr_mem={ ");

				while ( *(grpstruct->gr_mem) != NULL)
				{
		 			(void) fprintf (stdout,"%s ",*(grpstruct->gr_mem));
					grpstruct->gr_mem++;
				}

				if (oflag) (void) fprintf (stdout,":");
				else
					(void) fprintf (stdout,"} ");
	
			}

			if (( strcmp (xoptattrstring[count], "gr_gid")) == 0)
			{
				if (oflag)
					(void) fprintf (stdout,"%ld:", grpstruct->gr_gid);
                else
					(void) fprintf (stdout," gr_gid=%ld ", grpstruct->gr_gid);
			}

			count++;
		}
		
		(void) fprintf (stdout,"\n");
	} else (void) fprintf (stdout,"\n");			
				
	if (aflag) {

		/* We will have to print out all groups */

		(void) fprintf (stdout,"gr_gid=%ld ",grpstruct->gr_gid);
		(void) fprintf (stdout,"gr_mem={");
		while ( *(grpstruct->gr_mem) != NULL)
		{
		 	(void) fprintf (stdout,"%s ",*(grpstruct->gr_mem));
			grpstruct->gr_mem++;
		}
		(void) fprintf (stdout,"}");
	}
}
