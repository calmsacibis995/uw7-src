/* @(#)pidfile.c	1.2
 *
 * Revision History:
 *
 * 26 June 97		tonylo
 *	Created
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "ldapconfig.h"
#include "slurp.h"

extern char* PIDFILENAME;

/*
 * void
 * createSlurpdPIDfile()
 *
 * globals:
 *      PIDFILENAME
 *
 * Description:
 *      Creates pid directory SLURPD_PIDDIR. Generates PIDFILENAME from the
 *      name of the given configuration file. The algorithm takes each /
 *      character in the configfile path and turns them into underbars e.g.
 *      A daemon with configfile /etc/slapd.conf will produce a pidfile
 *      SLURPD_PIDDIR/_etc_slapd.conf
 *
 */

void
createSlurpdPIDfile(const char* configfile)
{
	size_t 		fnamelen;
	const char		*ptr1;
	char		*ptr2;
	FILE		*fp;

	/* Create pid filename, and file */
	mkdir(SLURPD_PIDDIR,755);
	fnamelen=strlen(SLURPD_PIDDIR)+strlen(configfile)+2;
	PIDFILENAME=(char*) malloc( fnamelen );
	strcpy(PIDFILENAME, SLURPD_PIDDIR );

	ptr1=configfile;
	ptr2=PIDFILENAME;
	ptr2+=strlen(SLURPD_PIDDIR);
	*ptr2++='/';
	while(*ptr1)
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
removeSlurpdPIDfile() 
{
	remove(PIDFILENAME);
	free(PIDFILENAME);
}

void
setSlurpdPIDfile()
{
	FILE* fp;
	if ( (fp = fopen(PIDFILENAME, "w")) !=NULL)
	{
		fprintf( fp, "%d", getpid() );
		fclose(fp);
	}
}

