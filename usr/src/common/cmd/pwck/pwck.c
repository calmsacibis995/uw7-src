/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pwck.c	1.4"
#ident  "$Header$"

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/signal.h>
#include	<sys/sysmacros.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<locale.h>
#include	<pfmt.h>
#include	"msg.h"


#define	ERROR7	"Optional shell file not found"

int eflag, code=0;
int badc;
char buf[512];

main(argc,argv)

int argc;
char **argv;

{
	int delim[512];
	char logbuf[80];
	FILE *fptr;
	int error();
	struct	stat obuf;
	uid_t uid;
	gid_t gid;
	int len;
	register int i, j, colons;
	char *pw_file;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore");
	(void) setlabel("UX:pwck");

	if(argc == 1) pw_file="/etc/passwd";
	else pw_file=argv[1];

	if((fptr=fopen(pw_file,"r"))==NULL) {
		pfmt (stderr, MM_ERROR, MSG1, pw_file);
		exit(1);
	}

	while(fgets(buf,512,fptr)!=NULL) {

		colons=0;
		badc=0;
		uid=gid=0;
		eflag=0;

	/*  Check number of fields */

		for(i=0 ; buf[i]!=NULL; i++) {
			if(buf[i]==':') {
				delim[colons]=i;
				++colons;
			}
		delim[6]=i;
		delim[7]=NULL;
		}
		if(colons != 6) {
			pfmt (stderr, MM_ERROR, MSG2);
			continue;
		}

	/*  Check that first character is alpha and rest alphanumeric  */

		if(!(islower(buf[0]))) {
			pfmt (stderr, MM_ERROR, MSG4);
		}
		if(buf[0] == ':') {
			pfmt (stderr, MM_ERROR, MSG4);
		}
		for(i=0; buf[i]!=':'; i++) {
			if(islower(buf[i]));
			else if(isdigit(buf[i]));
			else ++badc;
		}
		if(badc > 0) {
			pfmt (stderr, MM_ERROR, MSG3);
		}

	/*  Check for valid number of characters in logname  */

		if(i <= 0  ||  i > 8) {
			pfmt (stderr, MM_ERROR, MSG5);
		}

	/*  Check that UID is numeric and <= MAXUID  */

		len = (delim[2]-delim[1])-1;
		if ( (len > 5) || (len < 1) ) {
			pfmt (stderr, MM_ERROR, MSG6);
		}
		else {
		    for (i=(delim[1]+1); i < delim[2]; i++) {
			if(!(isdigit(buf[i]))) {
				pfmt (stderr, MM_ERROR, MSG6);
				break;
			}
			uid = uid*10 + (uid_t)((buf[i])-'0');
		    }
		    if(uid > MAXUID  ||  uid < 0) {
			pfmt (stderr, MM_ERROR, MSG6);
		    }
		}

	/*  Check that GID is numeric and <= MAXUID  */

		len = (delim[3]-delim[2])-1;
		if ( (len > 5) || (len < 1) ) {
			pfmt (stderr, MM_ERROR, MSG7);
		}
		else {
		    for(i=(delim[2]+1); i < delim[3]; i++) {
			if(!(isdigit(buf[i]))) {
				pfmt (stderr, MM_ERROR, MSG7);
				break;
			}
			gid = gid*10 + (gid_t)((buf[i])-'0');
		    }
		    if(gid > MAXUID  ||  gid < 0) {
				pfmt (stderr, MM_ERROR, MSG7);
		    }
		}

	/*  Stat initial working directory  */

		for(j=0, i=(delim[4]+1); i<delim[5]; j++, i++) {
			logbuf[j]=buf[i];
		}
		if((stat(logbuf,&obuf)) == -1) {
			pfmt (stderr, MM_ERROR, MSG8);
		}
		if(logbuf[0] == NULL) { /* Currently OS translates */
			pfmt (stderr, MM_ERROR, MSG9);
		}
		for(j=0;j<80;j++) logbuf[j]=NULL;

	/*  Stat of program to use as shell  */

		if((buf[(delim[5]+1)]) != '\n') {
			for(j=0, i=(delim[5]+1); i<delim[6]; j++, i++) {
				logbuf[j]=buf[i];
			}
			if(strcmp(logbuf,"*") == 0)  {  /* subsystem login */
				continue;
			}
			if((stat(logbuf,&obuf)) == -1) {
				pfmt (stderr, MM_ERROR, MSG10);
			}
			for(j=0;j<80;j++) logbuf[j]=NULL;
		}
	}
	fclose(fptr);
	exit(code);
}
