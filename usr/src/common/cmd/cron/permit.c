/*		copyright	"%c%" 	*/

#ident	"@(#)cron:common/cmd/cron/permit.c	1.14"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <priv.h>
#include <ctype.h>
#include <pwd.h>
#include "cron.h"

struct stat globstat;
#define	exists(file)	(stat(file,&globstat) == 0)

int per_errno;	/* status info from getuser */

/*
 * Procedure:     getuser
 *
 * Restrictions:
 *                getpwuid: P_MACREAD
*/


char *getuser(uid, sh)
uid_t uid;
char **sh;
{
	struct passwd *nptr;
	char *s;

	procprivl(CLRPRV, pm_work(P_MACREAD), (priv_t)0); 
	if ((nptr=getpwuid(uid)) == NULL) {
		per_errno=1;
		procprivl(SETPRV, pm_work(P_MACREAD), (priv_t)0); 
		return(NULL); 
	}
	procprivl(SETPRV, pm_work(P_MACREAD), (priv_t)0); 

	s = select_shell(nptr->pw_shell);
	if (sh != NULL) 
		*sh = s;
	if ((strcmp(nptr->pw_shell,s)!=0) && (strcmp(nptr->pw_shell,"")!=0)) {
		per_errno=2;
		/* return NULL if you want crontab and at to abort
		   when the users login shell is not /usr/bin/sh otherwise
		   return pw_name
		*/
		return(nptr->pw_name);
	}
	return(nptr->pw_name);
}


/**********************/
allowed(user,allow,deny)
/**********************/
char *user,*allow,*deny;
{
	if ( exists(allow) ) {
		if ( within(user,allow) ) return(1);
		else return(0); }
	else if ( exists(deny) ) {
		if ( within(user,deny) ) return(0);
		else return(1); }
	else return(0);
}

/*
 * Procedure:     within
 *
 * Restrictions:
                 fopen: P_MACREAD
                 fgets: None
                 fclose: None
 * Notes : Check to see if user is "within" the at.deny or at.allow files
*/

/************************/
within(username,filename)
/************************/
char *username,*filename;
{
	char line[UNAMESIZE];
	FILE *cap;
	int i;

	procprivl(CLRPRV, pm_work(P_MACREAD), (priv_t)0); 
	if((cap = fopen(filename,"r")) == NULL)
	{
		procprivl(SETPRV, pm_work(P_MACREAD), (priv_t)0); 
		return(0);
	}
	procprivl(SETPRV, pm_work(P_MACREAD), (priv_t)0); 
	while ( fgets(line,UNAMESIZE,cap) != NULL ) {
		for ( i=0 ; line[i] != '\0' ; i++ ) {
			if ( isspace(line[i]) ) {
				line[i] = '\0';
				break; }
		}
		if ( strcmp(line,username)==0 ) {
			fclose(cap);
			return(1); }
	}
	fclose(cap);
	return(0);
}
