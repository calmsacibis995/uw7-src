/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)grpck.c	1.3"
#ident "$Header$"
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <locale.h>
#include <pfmt.h>


static char	*TOOLONG_ID	= ":141",
		*TOOLONG	= "Line too long\n",
		*BADLINE_ID	= ":1102",
		*BADLINE	= "Too many/few fields\n",
		*NONAME_ID	= ":1111",
		*NONAME		= "No group name\n",
		*BADNAME_ID	= ":1112",
		*BADNAME	= "Bad character(s) in group name\n",
		*BADGID_ID	= ":1107",
		*BADGID 	= "Invalid GID\n",
		*NULLNAME_ID	= ":1113",
		*NULLNAME	= "Null login name\n",
		*NOTFOUND_ID	= ":1114",
		*NOTFOUND	= "Login name not found in password file\n",
		*DUPNAME_ID	= ":1115",
		*DUPNAME	= "Duplicate login name entry\n",

		*NGROUPS_ID	= ":1116",
		*NGROUPS	= "Maximum groups exceeded for lognin ame",

		*NOMEM		= ":847:Out of memory\n",
		*USAGE		= ":1117:usage: %s filename\n",
		*OPENFAIL	= ":1118:Cannot open file %s\n";

int eflag, badchar, baddigit,badlognam,colons,len,i;

#define MYBUFSIZE	512	/* max line length including newline and null */
char buf[MYBUFSIZE];
char tmpbuf[MYBUFSIZE];

char *nptr;
char *cptr;
FILE *fptr;
int delim[MYBUFSIZE];
gid_t gid;
int error();

struct group {
	struct group *nxt;
	int cnt;
	gid_t grp;
};

struct node {
	struct node *next;
	int ngroups;
	struct group *groups;
	char user[1];
};

void *
emalloc(size)
{
	void *vp;
	vp = malloc(size);
	if (vp == NULL) {
		pfmt(stderr, MM_ERROR, NOMEM);
		exit(1);
	}
	return vp;
}

main (argc,argv)
int argc;
char *argv[];
{
	struct passwd *pwp;
	struct node *root = NULL;
	struct node *t;
	struct group *gp;
	int ngroups_max;
	int listlen;
	int i;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore");
	(void) setlabel("UX:grpck");

	ngroups_max = sysconf(_SC_NGROUPS_MAX);

	if ( argc == 1)
		argv[1] = "/etc/group";
	else if ( argc != 2 ) {
		pfmt(stderr, MM_ERROR, USAGE, *argv);
		exit(1);
	}

	if ( ( fptr = fopen (argv[1],"r" ) ) == NULL ) {
		pfmt(stderr, MM_ERROR, OPENFAIL, argv[1]);
		exit(1);
	}

	while ((pwp = getpwent()) != NULL) {
		t = (struct node *)emalloc(sizeof(*t) + strlen(pwp->pw_name)+1);
		t->next = root;
		root = t;
		strcpy(t->user, pwp->pw_name);
		t->ngroups = 1;
		if (!ngroups_max)
			t->groups = NULL;
		else {
			t->groups = (struct group*)
			  emalloc(sizeof(struct group));
			t->groups->grp = pwp->pw_gid;
			t->groups->cnt = 1;
			t->groups->nxt = NULL;
		}
	}

	while(fgets(buf,MYBUFSIZE,fptr) != NULL ) {
		if ( buf[0] == '\n' )    /* blank lines are ignored */
			continue;

		i = strlen(buf);
		if ( (i == (MYBUFSIZE-1)) && (buf[i-1] != '\n') ) {  
			/* line too long */
			buf[i-1] = '\n';	/* add newline for printing */
			error(TOOLONG_ID,TOOLONG);
			while(fgets(tmpbuf,MYBUFSIZE,fptr) != NULL )  {
				i = strlen(tmpbuf);
				if ( (i == (MYBUFSIZE-1)) 
				  && (tmpbuf[i-1] != '\n') )
					/* another long line */
					continue;
				else
					break;
			}
			/* done reading continuation line(s) */

			strcpy(tmpbuf, buf);
		} else {
			/* change newline to comma for strchr */
			strcpy(tmpbuf, buf);
			tmpbuf[i-1] = ',';	
		}

		colons=0;
		eflag=0;
		badchar=0;
		baddigit=0;
		badlognam=0;
		gid=(gid_t)0;

		/*	Check number of fields	*/

		for (i=0 ; buf[i]!=NULL ; i++)
		{
			if (buf[i]==':')
			{
				delim[colons]=i;
				++colons;
			}
		}
		if (colons != 3 )
		{
			error(BADLINE_ID,BADLINE);
			continue;
		}

		/* check to see that group name is at least 1 character	*/
		/* and that all characters are printable.		*/

		if ( buf[0] == ':' )
			error(NONAME_ID,NONAME);
		else
		{
			for ( i=0; buf[i] != ':'; i++ )
			{
				if ( ! ( isprint(buf[i])))
					badchar++;
			}
			if ( badchar > 0 )
				error(BADNAME_ID,BADNAME);
		}

		/*	check that GID is numeric and <= 65535	*/

		len = ( delim[2] - delim[1] ) - 1;

		if ( len > 5 || len == 0 )
			error(BADGID_ID,BADGID);
		else
		{
			for ( i=(delim[1]+1); i < delim[2]; i++ )
			{
				if ( ! (isdigit(buf[i])))
					baddigit++;
				else if ( baddigit == 0 )
					gid=gid * 10 + (gid_t)(buf[i] - '0');
				/* converts ascii GID to decimal */
			}
			if ( baddigit > 0 )
				error(BADGID_ID,BADGID);
			else if ( gid > (gid_t)65535 || gid < (gid_t)0 )
				error(BADGID_ID,BADGID);
		}

		/*  check that logname appears in the passwd file  */

		nptr = &tmpbuf[delim[2]];
		nptr++;

		listlen = strlen(nptr) - 1;

		while ( ( cptr = strchr(nptr,',') ) != NULL )
		{
			*cptr=NULL;
			if ( *nptr == NULL )
			{
				if (listlen)
					error(NULLNAME_ID,NULLNAME);
				nptr++;
				continue;
			}

			for (t = root; ; t = t->next) {
				if (t == NULL) {
					badlognam++;
					error(NOTFOUND_ID,NOTFOUND);
					goto getnext;
				}
				if (strcmp(t->user, nptr) == 0)
					break;
			}
			
			if (!ngroups_max)
				goto getnext;

			t->ngroups++;

			/*
			 * check for duplicate logname in group
			 */

			for (gp = t->groups; gp != NULL; gp = gp->nxt) {
				if (gid == gp->grp) {
					if (gp->cnt++ == 1) {
						badlognam++;
						error(DUPNAME_ID,DUPNAME);
					}
					goto getnext;
				}
			}

			gp = (struct group*)emalloc(sizeof(struct group));
			gp->grp = gid;
			gp->cnt = 1;
			gp->nxt = t->groups;
			t->groups = gp;
getnext:
			nptr = ++cptr;
		}
	}

	if (ngroups_max) {
		for (t = root; t != NULL; t = t->next) {
			if (t->ngroups > ngroups_max)
				fprintf(stderr,"\n\n%s %s (%d)\n",
				 gettxt(NGROUPS_ID,NGROUPS),t->user,t->ngroups);
		}
	}
}

/*	Error printing routine	*/

error(id,msg)

char *id,*msg;
{
	if ( eflag==0 )
	{
		fprintf(stderr,"\n\n%s",buf);
		eflag=1;
	}

	if ( badchar != 0 )
	{
		fprintf(stderr,"\t%d %s",badchar,gettxt(id,msg));
		badchar=0;
		return;
	}
	else if ( baddigit != 0 )
	{
		fprintf(stderr,"\t%s",gettxt(id,msg));
		baddigit=0;
		return;
	}
	else if ( badlognam != 0 )
	{
		fprintf(stderr,"\t%s - %s",nptr,gettxt(id,msg));
		badlognam=0;
		return;
	}
	else
	{
		fprintf(stderr,"\t%s",gettxt(id,msg));
		return;
	}
}
