#ident  "@(#)userls.c	1.3"
#ident  "$Header$"

/*
* 
* Command: userls
*
* Specified in POSIX Part 3: User and Group Account Management
*
* Options Supported :
*
* -a		Writes all options associated with specified user accounts to stdout
*			in extended option syntax.
*
* -d		Selects user accounts with duplicate userid's.
*
* -m		Displays multiple group membership information contained in the
*			groups	option.
*
* -o		Formats output, one line per account, into colon-delimeted option
*			values. The output should be in the order that the options were 
*			specified.
*
* -l user	Display user names.
*
* -g group	Display groups.
*
* Options Not Supported :
*
*	-D			Lists the system-wide defaults for specified options.
*
*	-p			Selects user accounts with no passwords.
*
*	-R			Selects user accounts that are retired.
*
*	-c cell		Specifies the cell to report on.	
*	
*	-S sysname	Specifies name of the system.
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
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "messages.h"
#include "getxopt.h"

/* Defines */

static int pwd_compar (char *, char *);
static void display_users ();
static void display_groups ();
static void user_sort ();
static void swap ();


/* Externs */

extern	char *strncat(char *s1, const char *s2, size_t n);
extern	char *strcpy(char *s1, const char *s2);
extern	char *strtok(char *s1, const char *s2);

extern char *optarg, 		/* Used by getopt() */
			*errmsgs[];

int errno;
int getopt();
int errmsg();
int getxopt(); 
void bzero();

extern  struct  group   *getgrnam(),
						*getgrent();

extern struct passwd	*getpwent(),
						*getpwuid(),
						*getpwnam();

void user_sort (void* v[], int left, int right, int (*compar)
	(void *, void *));

/* Misc. */

char *msg_label = "UX:userls";
static int oflag=0, dflag=0, aflag=0, mflag=0; 	/* Various option flags */

static struct group *grpstruct = NULL ;	/* Will be filled with entries from 
							 		 * /etc/group.
							 		 */

static struct passwd *pwdstruct = NULL;	/* Will be filled with entries from
									 * /etc/passwd.
									 */

static DpaExtStrToken *exttoken =  NULL ;/* Extended options (-x) structure */

static int end_of_group_file =0;
static int end_of_passwd_file =0;
static int count; /* counter */
static char *xoptattrstring[7];	/* Can only be a maximum of six valid */ 

main(argc, argv)
int argc;
char *argv[];
{
	void display_users();
	void display_groups();
	int opt=0, offset=0, return_code=0, firsttimeflag=1;
	ssize_t bytesread=0;
	int exclusive_option=0;
	int errflag = 0;
	gid_t gid;					/* GID */
	uid_t uid;					/* UID */
	int extfd;					/* File Descriptor for extended options file */

	char *groupnames = NULL;	/* Group name from the command line */
	char *group = NULL;			/* Group */
	char *user = NULL;			/* user */
	const char *token = NULL;	/* strtok() token */
	char *sysname = NULL;		/* System name from the command line */
	char *usernames = NULL;		/* User name from the command line */
	char *cellname = NULL;		/* Cell name from the command line */
	char *ptr;					/* Pointer */
	char filebuf[128];			/* Buffer for extended options file */ 
	size_t totalbytesread =0;	/* Total bytes read drom extended options file*/
	static char *xoptstring = NULL;	/* extended options from the command line */
	static char *Xoptstring = NULL;	/* extended options file from the command 
									 * line 
									 */

	/* Set up the l10n stuff */

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore.abi");
	(void) setlabel(msg_label);

	/* Parse the options */

	while((opt = getopt(argc, argv, "adomDpRg:l:S:x:X:c:")) != EOF)
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

			case 'm':
				mflag++;
				break;

			case 'g':
				groupnames = optarg;
				exclusive_option++;
				break;

			case 'l':
				usernames = optarg;
				exclusive_option++;
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
				errmsg( M_USERLS_USAGE ); 
				exit( EX_SYNTAX );
		}
	
	if ( (exclusive_option > 1) || (dflag && (oflag || aflag)) ) {
		errmsg( M_USERLS_USAGE ); 
		exit( EX_SYNTAX );
	}
	
	if (dflag) {
		
		errmsg( M_DUPUID );
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
		/* We have a file containing extended options. We will open and read the
		 * file into xoptstring, and process it below.
		 */

		extfd = open (Xoptstring, O_RDONLY);

		if (extfd == -1)
		{
			errmsg (M_XTENDED_OPT_FILE_DOES_NOT_EXIST,Xoptstring);
			exit (EX_FAILURE);
		}

		bzero (filebuf,128);

		while ( (bytesread = read (extfd, filebuf, (ssize_t)128)) != 0)
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

			if ((strcmp (xoptattrstring[count],"groups") != 0) &&
			(strcmp (xoptattrstring[count], "pw_name") != 0) &&
			(strcmp (xoptattrstring[count], "pw_uid") != 0) &&
			(strcmp (xoptattrstring[count], "pw_gid") != 0) &&
			(strcmp (xoptattrstring[count], "pw_dir") != 0) &&
			(strcmp (xoptattrstring[count], "pw_shell") != 0) &&
			(strcmp (xoptattrstring[count], "comment") != 0)) 
			{
				errmsg (M_INVALID_XTENDED_OPT,xoptattrstring[count]);
				exit (1);
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

	if (argc == 1) {

		/* No users were specified so do the default action. According to the
		 * POSIX spec, that is a list of users (1 per line) sorted by userid.
		 * We will use (3C) qsort to do the sort.
		 */

		/* Firstly count the number of users to be sorted so that we can 
		 * allocate large enough arrays later ( in any case qsort also needs to
		 * know how many elements it's sorting.
		 */

		count = 0;

		while (!end_of_passwd_file) {
			if ((pwdstruct = getpwent()) != NULL) {
				count++;
			}
        	else {
				if (errno == 0)
					end_of_passwd_file = 1;
				else {
					if (errno == EINVAL) errno = 0;
             		 		else end_of_passwd_file = 1;
           		}
        	}
    	}

		{
			size_t buflen=0;
			char *passwd_entries[1024];
			char pwdbuf[255];
			char *token;
			int x=0;
			count = 0;

			setpwent(); /* Rewind /etc/passwd so we can go through again */
			end_of_passwd_file = 0;

			while (!end_of_passwd_file) {
				if ((pwdstruct = getpwent()) != NULL) {
					if ((sprintf (pwdbuf,"%ld:%s:",pwdstruct->pw_uid,
						 pwdstruct->pw_name)) < 0)
					{
						errmsg (M_INTERNAL_ERROR);
					}
					if (buflen < strlen(pwdbuf)) buflen = strlen(pwdbuf);
					ptr = (char *) malloc (strlen(pwdbuf));
					ptr = strcpy (ptr, pwdbuf);
					passwd_entries[count] = ptr;
					count++;
				}
        		else {
					if (errno == 0)
						end_of_passwd_file = 1;
					else {
						if (errno == EINVAL) errno = 0;
               		 		else end_of_passwd_file = 1;
            		}
        		}
    		}

			/* We now have an array passwd_entries full of /etc/passwd data.
			 * Sort the data with user_sort() below. user_sort() requires a
			 * defined comparison algorithm, see pwd_compar() below.
			 */

			user_sort ( (void **) passwd_entries, 0, count -1, 
					(int (*) (void *, void *)) pwd_compar);
			
			for (x=0; x < count -1 ; x++)
			{
				token = strtok(passwd_entries[x],":");
				token = strtok(NULL,":");
				(void) fprintf (stdout,"%s\n",token);

			}
		}
	}
	
	if ( groupnames != NULL) {
		/* We examine groupnames to get groups, user can specify more than
		 * one group each delimeted by commas. e.g -g root,sys,....
		 */

		grpstruct = NULL;
		token = strtok (groupnames,",");

		group = (char *)malloc (strlen(token));
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
				errmsg( M_GRP_INVALID, group ); 
				errflag++;
			}
			else
			{	
				setgrent ();	/* Rewind the /etc/group file for more 
								 * searches */

				grpstruct = getgrgid (gid); 

				if ( grpstruct == NULL ) {	
					errmsg (M_GID_INVALID, group);
					errflag++;
				}
				else
				{
					display_groups();
				}
			}
		}

		/* Complete the rest of the groups in groupnames in the same way */

		while ( (token =  strtok (NULL,",")) != NULL)
		{
			setgrent (); /*Rewind the /etc/group file for more searches */

			free(group);

			group = (char *)malloc (strlen(token));
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

				grpstruct = NULL;

				gid = (gid_t) strtol(group, &ptr, 10);

				if ( (gid == LONG_MAX) ||  (gid == LONG_MIN)
					|| (strcmp (group, ptr) == 0) ) {
					errmsg( M_GRP_INVALID, group ); 
					errflag++;
				}
				else
				{
					setgrent ();	/* Rewind the /etc/group file for more 
									 * searches */

					grpstruct = getgrgid (gid);

					if ( grpstruct == NULL ) 
					{	
						errmsg (M_GID_INVALID, group );
						errflag++;
					}
					else
					{
						display_groups();
					}
				}
			}

			setgrent (); /*Rewind the /etc/group file for more searches */
		}
	}

	if ( usernames != NULL) {
		/* We examine usernames to get users, user can specify more than
		 * one user each delimited by commas. e.g -l root,sys,....
		 */

		pwdstruct = NULL;
		token = strtok (usernames,",");
		user =  (char *) malloc (strlen(token));
		user = strcpy (user,token) ;
		pwdstruct = getpwnam(user);

		if ( pwdstruct != NULL ) {

			/* We got a valid user, print it out */
			display_users();
		}
		else
		{
			/* getpwnam returned an error cos it couldn't find the group.
			 * However the group could have been specified as a gid rather
			 * than a name, so try this case before returning an error to
			 * the user.
			 */

			pwdstruct = NULL;

			uid = (uid_t) strtol(user, &ptr, 10);

			if ( (uid == LONG_MAX) ||  (uid == LONG_MIN)
				|| (strcmp (user, ptr) == 0) ) {
				errmsg( M_USER_INVALID, user ); 
				errflag++;
			}
			else
			{	
				pwdstruct = getpwuid (uid); 

				if ( pwdstruct == NULL ) {	
					errmsg (M_USER_INVALID, user);
					errflag++;
				}
				else
				{
					display_users();
				}
			}
		}

		/* Complete the rest of the users in usernames in the same way */

		while ( (token =  strtok (NULL,",")) != NULL)
		{
			free (user);
			user =  (char *) malloc (strlen(token));
			user = strcpy (user,token) ;
			pwdstruct = NULL;

			pwdstruct = getpwnam (user);

			if ( pwdstruct != NULL ) {

				/* We got a valid group, print it out */
				display_users();
			}
			else
			{

			/* getpwnam returned an error cos it couldn't find the user.
			 * However the user could have been specified as a uid rather
			 * than a name, so try this case before returning an error to
			 * the user.
			 */

				pwdstruct = NULL;
				ptr = NULL;


				uid = (uid_t) strtol(user, &ptr, 10);

				if ( (uid == LONG_MAX) ||  (uid == LONG_MIN)
					|| (strcmp (user, ptr) == 0) ) {
					errmsg( M_USER_INVALID, user );
					errflag++;
				}
				else
				{
					pwdstruct = getpwuid (uid);

					if ( pwdstruct == NULL ) 
					{	
						errmsg (M_USER_INVALID, user );
						errflag++;
					}
					else
					{
						display_users();
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
	/* No need to l10n this. The output would be the same in all languages.*/

	end_of_passwd_file = 0;

	while (!end_of_passwd_file) {
		if ((pwdstruct = getpwent()) != NULL) {

			if (pwdstruct->pw_gid == grpstruct->gr_gid)
			{
				(void) fprintf (stdout,"%s ",pwdstruct->pw_name);
			}
		}
       	else {
			if (errno == 0)
				end_of_passwd_file = 1;
			else {
				if (errno == EINVAL) errno = 0;
           		 		else end_of_passwd_file = 1;
       		}
       	}
    }

	(void) fprintf (stdout,"\n");

	setpwent(); /* Rewind the /etc/passwd file for next search if any*/
	
}

static void display_users()
{
	int count = 0;

	/* Prints out the user info as required, this code is used a lot by main()
	 * and is here to avoid duplication.
	 */

	/* No need to l10n this. The output would be the same in all languages.*/

	if (oflag)
	(void) fprintf (stdout,"%s:",pwdstruct->pw_name);/* Print the users name */
	else
	(void) fprintf (stdout,"pw_name=%s ",pwdstruct->pw_name);/* Print the users name */

	if (xoptattrstring[0] != NULL)
	{
		while ((strcmp (xoptattrstring[count], NULL)) != 0)	
		{
			if (( strcmp (xoptattrstring[count],"groups")) == 0)
			{
				if (!oflag) (void) fprintf (stdout," groups={ ");

				end_of_group_file = 0;
			
				while (!end_of_group_file) {
					if ((grpstruct = getgrent()) != NULL) {
					
						while ( *(grpstruct->gr_mem) != NULL)
						{
							if (( strcmp (pwdstruct->pw_name, 
									*(grpstruct->gr_mem)) == 0))
							{	
								(void) fprintf (stdout,"%s ",
											grpstruct->gr_name);
							}
	
							grpstruct->gr_mem++;
						}
					} 
					else { 
						if (errno == 0) end_of_group_file = 1;
						else {
							if (errno == EINVAL) errno = 0;
   	            				else end_of_group_file = 1;
   	        			}
					}
   		     	}

				if (oflag) (void) fprintf (stdout,":");
				else
					(void) fprintf (stdout,"}");
	

				setgrent();	/* Rewind the group file so we can search again */
			}

			if (strcmp (xoptattrstring[count], "pw_name") == 0)
			{
				if (oflag)
					(void) fprintf (stdout,"%s:",pwdstruct->pw_name);
				else
					(void) fprintf (stdout," pw_name=%s",pwdstruct->pw_name);
			}
			if (strcmp (xoptattrstring[count], "pw_uid") == 0)
			{
				if (oflag)
					(void) fprintf (stdout,"%ld:",pwdstruct->pw_uid);
				else
					(void) fprintf (stdout," pw_uid=%ld",pwdstruct->pw_uid);
			}
			if (strcmp (xoptattrstring[count], "pw_gid") == 0)
			{
				if (oflag)
					(void) fprintf (stdout,"%ld:",pwdstruct->pw_gid);
				else
					(void) fprintf (stdout," pw_gid=%ld",pwdstruct->pw_gid);
			}
			if (strcmp (xoptattrstring[count], "pw_dir") == 0)
			{
				if (oflag)
					(void) fprintf (stdout,"%s:",pwdstruct->pw_dir);
				else
					(void) fprintf (stdout," pw_dir=%s",pwdstruct->pw_dir);
			}
			if (strcmp (xoptattrstring[count], "pw_shell") == 0)
			{
				if (oflag)
					(void) fprintf (stdout, "%s:",pwdstruct->pw_shell);
				else
					(void) fprintf (stdout," pw_shell=%s",pwdstruct->pw_shell);
			}
			if (strcmp (xoptattrstring[count], "comment") == 0)
			{
				if (oflag)
					(void) fprintf (stdout,"%s:",pwdstruct->pw_comment);
				else
					(void) fprintf (stdout," pw_comment='%s'",pwdstruct->pw_comment);
			}
			count++;
		}
		(void) fprintf (stdout,"\n");
	}

	if (aflag)
	{
		/* Extended options style ouput.*/
		/* User specified all options, so all options are printed */

		(void) fprintf (stdout,"pw_uid=%ld pw_gid=%ld comment='%s' pw_dir=%s pw_shell=%s", 
			pwdstruct->pw_uid, pwdstruct->pw_gid, pwdstruct->pw_comment,\
			pwdstruct->pw_dir, pwdstruct->pw_shell);

		(void) fprintf (stdout," groups={");

		end_of_group_file = 0;
		
		while (!end_of_group_file) {
			if ((grpstruct = getgrent()) != NULL) {
			
				while ( *(grpstruct->gr_mem) != NULL)
				{
					if (( strcmp (pwdstruct->pw_name, 
							*(grpstruct->gr_mem)) == 0))
					{	
	 					(void) fprintf (stdout,"%s ",grpstruct->gr_name);
					}
					grpstruct->gr_mem++;
				}

			} else { 
				if (errno == 0) end_of_group_file = 1;
				else {
					if (errno == EINVAL) errno = 0;
           				else end_of_group_file = 1;
       			}
			}
        }

		(void) fprintf (stdout,"}\n");

		setgrent();	/* Rewind the group file so we can search again */
	}

	if (mflag)
	{
		end_of_group_file = 0;

		(void) fprintf (stdout,":");
		
		while (!end_of_group_file) {
			if ((grpstruct = getgrent()) != NULL) {
			
				while ( *(grpstruct->gr_mem) != NULL)
				{
					if (( strcmp (pwdstruct->pw_name, 
							*(grpstruct->gr_mem)) == 0))
					{	
	 					(void) fprintf (stdout, "%s ",grpstruct->gr_name);
					}
					grpstruct->gr_mem++;
				}

			} else { 
				if (errno == 0) end_of_group_file = 1;
				else {
					if (errno == EINVAL) errno = 0;
           				else end_of_group_file = 1;
       			}
			}
		}
		(void) fprintf (stdout,":\n");
	}
	
}

static int pwd_compar (char *s1, char *s2)
{
	char *s1work, *s2work;		/* Working copies of s1 and s2 */
	char *s1uid, *s2uid;		/* The part of s1 and s2 we are interested in */
	char *ptr;					/* For strtol */
	long s1uidnum, s2uidnum; 	/* The uid portion of s1 and s2 in numeric form
								 * we will base our comparison on these 
								 * variables.
								 */

	s1work = (char *) malloc (strlen(s1));
	s2work = (char *) malloc (strlen(s2));

	s1work = strcpy (s1work,s1);
	s2work = strcpy (s2work,s2);

	s1uid = strtok (s1work,":");
	s2uid = strtok (s2work,":");
	
	s1uidnum = strtol(s1uid, &ptr, 10);
	s2uidnum = strtol(s2uid, &ptr, 10);

	free (s1work);
	free (s2work);

	if (s1uidnum < s2uidnum)
		return -1;
	else if (s1uidnum > s2uidnum)
		return 1;
	else
		return 0;
}

static void user_sort ( void *v[], int left, int right, int (*comp) (void *,void *))
{
	int i, last;
	void swap( void *v[], int, int);
	if (left >= right) return;
	swap (v, left, (left + right)/2);
	last = left;
	for (i=left+1; i <= right; i++)
		if ((*comp)(v[i], v[left]) < 0)
			swap (v,++last,i);
	swap (v,left,last);
	user_sort (v,left,last-1,comp);
	user_sort (v,last+1,right,comp);
}

static void swap (void *v[], int i, int j)
{
	void *temp;
	
	temp = v[i];
	v[i] = v[j];
	v[j] = temp;
}
