/*		copyright	"%c%" 	*/

#ident	"@(#)cryptkey.c	1.2"
#ident  "$Header$"

/*	Command for management of user keys in the IAF cr1 scheme	*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stropts.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pfmt.h>
#include <locale.h>

#include "cr1.h"
#include "keymaster.h"

static char *scheme = DEF_SCHEME;
static long mypid;
extern char role = ' ';
static FILE * logfp;

extern int principal_copy();
extern int send_msg();

static int
x_set(char *name)
{
	char *p;

	if ( (p = strrchr(name, '.')) != NULL) {
		p++;
		if (strcmp(p, "enigma") == 0)
			return(X_ENIGMA);
	}
	
	return(X_DES);
}	

main (int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	extern int opterr; 

	int c;			/* return value for getopt() */

	int aflag = 0;		/* ==1 if -a option is specified */
	int cflag = 0;		/* ==1 if -c option is specified */
	int dflag = 0;		/* ==1 if -d option is specified */

	char buf[BUFSIZ];	/* place to build up key db name */

	char *loc_principal = NULL;	/* The name of a local principal */
				/* logname[@system] or [system!]logname */
	char *rem_principal = NULL;	/* The name of a remote principal */
				/* [logname@]system or system[!logname] */

	char *Local,		/* always in the logname@system format */	
	     *Remote;		/* always in the logname@system format */

	Kmessage kmsg;

	char prompt[128];	/* Buffer for building getpass() prompt */

	char *old_key;		/* Alias to old key in kmsg */
	char *new_key;		/* Alias to new key in kmsg */
	char *key_ptr;		/* Pointer to getpass() return value */

	mypid = getpid();

	(void) setlocale(LC_MESSAGES, "");
	(void) setlabel("UX:cr1");
	(void) setcat("uxnsu");

	logfp = stderr;

	/* Parse the command line */

	opterr = 0;		/* Disable "illegal option" message */

	kmsg.type = CHANGE_KEY;

	while ((c = getopt(argc, argv, "acds:")) != -1) {
		switch (c) {
		case 'a':
			aflag = 1;
			kmsg.type = ADD_KEY;
			break;
		case 'c':
			cflag = 1;
			kmsg.type = CHANGE_KEY;
			break;
		case 'd':
			dflag = 1;
			kmsg.type = DELETE_KEY;
			break;
		case 's':
			scheme = optarg;
			break;
		case '?':
			failure(CR_CKUSAGE, "cryptkey");
			break;
		}
	}

	/* can only specify 1 flag */

	if (aflag+cflag+dflag > 1)
		failure(CR_CKUSAGE, "cryptkey");

	/* Grab command line operands */

	if ( (argc - optind) == 2 )
		loc_principal = argv[optind++];

	if ( (argc - optind) == 1 )
		rem_principal = argv[optind++];
	else
		failure(CR_CKUSAGE, "cryptkey");

	/* set up encryption type */

	kmsg.xtype = x_set(scheme);

	/* Establish Local principal */

	Local = kmsg.principal1;

	if ( principal_copy(Local, loc_principal, P_LOCAL) != 0 )
		failure(CR_PRINCIPAL, gettxt(":53", "local"));

	/* Establish Remote principal */
	
	Remote = kmsg.principal2;

	if ( principal_copy(Remote, rem_principal, P_REMOTE) != 0 )
		failure(CR_PRINCIPAL, gettxt(":54", "remote"));

	/* We need the old key unless we're adding or "privileged" */
	/* "privileged" means able to read/write the keys file. */

	old_key = kmsg.key1;
	new_key = kmsg.key2;
	
	(void)sprintf(buf, "%s/%s/%s", DEF_KEYDIR, scheme, DEF_KEYFIL);

	/* we need the old key unless we're adding or authorized */

	if (!aflag && access(buf, R_OK | W_OK | EFF_ONLY_OK)) {
		sprintf(prompt, gettxt(OLD_KEYID, OLD_KEY), scheme);
		if ((key_ptr = getpass(prompt)) == NULL)
			failure(CR_INKEY, NULL);
		strncpy(old_key, key_ptr, KEYLEN);
	}

	/* We need new key unless we're deleting */

	if (!dflag) {

		/* get the new key */

		sprintf(prompt, gettxt(NEW_KEYID, NEW_KEY), scheme);
		if ((key_ptr = getpass(prompt)) == NULL)
			failure(CR_INKEY, NULL);
		strncpy(new_key, key_ptr, KEYLEN);

		/* Verify new key */

		sprintf(prompt, gettxt(VER_KEYID, VER_KEY), scheme);
		if ((key_ptr = getpass(prompt)) == NULL)
			failure(CR_INKEY, NULL);
		if (strncmp(new_key, key_ptr, KEYLEN))
			failure(CR_CONFIRM, NULL);
	}

	/* Send message to keymaster daemon */

	exit(send_msg(scheme, &kmsg, CR_EXIT));

	/* NOTREACHED */

}
