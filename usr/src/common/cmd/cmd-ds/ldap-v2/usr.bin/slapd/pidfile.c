/* @(#)pidfile.c	1.4
 *
 * Revision History:
 *
 * 5 June 97		tonylo
 *	Created
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "ldapconfig.h"
#include "slap.h"

extern char* configfile;
extern char* PIDFILENAME;

char repfile[]="/tmp/repfile";

/*
 * void
 * createPIDfile()
 *
 * globals:
 *      configfile
 *      PIDFILENAME
 *
 * Description:
 *      Creates pid directory SLAPD_PIDDIR. Generates PIDFILENAME from the
 *      name of the master configuration file. The algorithm takes each /
 *      character in the configfile path and turns them into underbars e.g.
 *      A daemon with configfile /etc/slapd.conf will produce a pidfile
 *      SLAPD_PIDDIR/_etc_slapd.conf
 *
 */

void
createPIDfile()
{
	size_t 		fnamelen;
	char		*ptr1, *ptr2;
	FILE		*fp;

	/* Create pid filename, and file */
	mkdir(SLAPD_PIDDIR,755);
	fnamelen=strlen(SLAPD_PIDDIR)+strlen(configfile)+2;
	PIDFILENAME=(char*) malloc( fnamelen );
	strcpy(PIDFILENAME, SLAPD_PIDDIR );

	ptr1=configfile;
	ptr2=PIDFILENAME;
	ptr2+=strlen(SLAPD_PIDDIR);
	*ptr2++='/';
	while(*ptr1 != '\0')
	{
		if( *ptr1 == '/' )
		{
			*ptr2='_';
		}	
		else
		{
			*ptr2=*ptr1;
		}
		ptr1++; ptr2++;
	}
	*ptr2='\0';
	if ( (fp = fopen(PIDFILENAME, "w")) !=NULL)
	{
		fprintf(fp, "STARTING");
		fclose(fp);
	}
}

void
removePIDfile() 
{
	remove(PIDFILENAME);
}

void
setPIDfile() 
{
	FILE* fp;

	if ( (fp = fopen(PIDFILENAME, "w")) !=NULL)
	{
		fprintf( fp, "%d", getpid() );
		fclose(fp);
	}
}
