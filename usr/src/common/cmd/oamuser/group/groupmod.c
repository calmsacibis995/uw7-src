#ident  "@(#)groupmod.c	1.5"
#ident  "$Header$"

/*
 * Command:	groupmod
 *
 * Usage:	groupmod [-g gid [-o] ] [-n name] [-K path] group
 *
 * Inheritable Privileges:	P_MACWRITE,P_SETFLEVEL,P_AUDIT,P_DACWRITE
 *				P_MACREAD,P_DACREAD
 *       Fixed Privileges:	None
 *
 * Notes:	modify a group definition on the system.
 *
 *		Arguments are:
 *
 *			gid -	a gid_t less than UID_MAX
 *			name -	a string of printable characters excluding
 *				colon (:) and less than MAXGLEN characters long.
 *			path -  directory containing files `groupmod.pre' and `groupmod.post'
 *			group -	a string of printable characters excluding
 *				colon(:) and less than MAXGLEN characters long.
 *
 *		P_SETFLEVEL is required to reset the level of that file.
 *		P_MACWRITE is required for renaming temporary file to /etc/group.
 */

/* LINTLIBRARY */
#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <grp.h>
#include <userdefs.h>
#include <string.h>
#include <users.h>
#include "messages.h"
#include <audit.h>
#include <priv.h>
#include <pfmt.h>
#include <locale.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <rpc/rpc_com.h>
#include <rpc/rpcb_prot.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netconfig.h>
#include <stddef.h>

static char *domain;
#define       EX_NO_NIS         47
#define       EX_NO_NISMATCH    48
#define       EX_UNK_NIS        49
#define       EX_SCRIPT_ERR	50


extern char	*optarg,
		*argvtostr();

extern int optind;
extern int errno=0;

extern int getopt(), valid_gid(), valid_gname(), mod_group();
extern void exit(), errmsg(), adumprec();
extern long strtol();

extern struct group *nis_getgrnam();

char *msg_label = "UX:groupmod";
static char *cmdline = (char *)0;
#define nisname(n) (*n == '+' || *n == '-')
/*
 * Procedure:     main
 *
 * Restrictions:
 *               printf:   none
 *               getopt:   none
 *               getgrnam: none
 */

main(argc, argv)
int argc;
char *argv[];
{
	int ch;				/* return from getopt */
	gid_t gid;			/* group id */
	int oflag = 0;	/* flags */
	int valret;			/* return from valid_gid() */
	char *ptr;
	char *gidstr = NULL;		/* gid from command line */
	char *newname = NULL;		/* new group name with -n option */
	char *customdir = NULL;		/* directory name with -K option */
	char *grpname;			/* group name from command line */
	char end_of_file=0;
        FILE    *fp;
        struct group *gstruct;


	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore.abi");
	(void) setlabel(msg_label);

	/* save command line arguments */
	if (( cmdline = (char *)argvtostr(argv)) == NULL) {
				errmsg (M_FAILED_ARGVTOSTR);
		adumprec(ADT_MOD_GRP,1,strlen(argv[0]),argv[0]);
                exit(1);
        }

	oflag = 0;	/* flags */

	while((ch = getopt(argc, argv, "g:on:K:")) != EOF)  {
		switch(ch) {
			case 'g':
				gidstr = optarg;
				break;
			case 'o':
				oflag++;
				break;
			case 'n':
				newname = optarg;
				break;
			case 'K':
				customdir = optarg;
				break;
			case '?':
				errmsg( M_GROUPMOD_USAGE );
				adumprec(ADT_MOD_GRP,EX_SYNTAX,strlen(cmdline),cmdline);
				exit( EX_SYNTAX );
		}
	}

	if( (oflag && !gidstr) || optind != argc - 1 ) {
		errmsg( M_GROUPMOD_USAGE );
		adumprec(ADT_MOD_GRP,EX_SYNTAX,strlen(cmdline),cmdline);
		exit( EX_SYNTAX );
	}
	grpname = argv[optind];
	if(nisname(grpname) && nisname(newname)){
		errmsg( M_GROUPMOD_USAGE );
		adumprec(ADT_MOD_GRP,EX_SYNTAX,strlen(cmdline),cmdline);
		exit( EX_SYNTAX );
	}
	if(nisname(grpname)&& (oflag || gidstr)){
		errmsg( M_GROUPMOD_USAGE );
		adumprec(ADT_MOD_GRP,EX_SYNTAX,strlen(cmdline),cmdline);
		exit( EX_SYNTAX );
	}

	if( nis_getgrnam(grpname) == (struct group *)NULL) {
		errmsg( M_NO_GROUP, grpname );
		adumprec(ADT_MOD_GRP,EX_NAME_NOT_EXIST,strlen(cmdline),cmdline);
		exit( EX_NAME_NOT_EXIST );
	}

	if (nisname(newname)) {
		int niserr=0;
		if((niserr = nis_getgrp(grpname)) != 0) {
			switch (niserr) {
				case YPERR_YPBIND:
					errmsg (M_NIS_UNAVAILABLE);
					adumprec(ADT_ADD_GRP,EX_NO_NIS,strlen(cmdline),cmdline);
					exit( EX_NO_NIS );
					break;
				case YPERR_KEY:
					errmsg (M_NIS_GROUPMAP, grpname);
					adumprec(ADT_ADD_GRP,EX_NO_NISMATCH,strlen(cmdline),cmdline);
					exit( EX_NO_NISMATCH );
					break;
				default:
					errmsg (M_NIS_UNKNOWN);
					adumprec(ADT_ADD_GRP,EX_UNK_NIS,strlen(cmdline),cmdline);
					exit( EX_UNK_NIS );
						break;
				}
		}
	}
	if( gidstr && !nisname(grpname)) {
		/* convert gidstr to integer */

		gid = (gid_t) strtol(gidstr, &ptr, 10);

		if( *ptr ) {
			errmsg( M_GID_INVALID, gidstr );
			adumprec(ADT_MOD_GRP,EX_BADARG,strlen(cmdline),cmdline);
			exit( EX_BADARG );
		}

		switch( valid_gid( gid, NULL ) ) {
		case RESERVED:
			errmsg( M_RESERVED, gid );
			adumprec(ADT_MOD_GRP,EX_BADARG,strlen(cmdline),cmdline);
			exit(  EX_BADARG  );
			/*NOTREACHED*/

		case NOTUNIQUE:
			if( !oflag ) {
				errmsg( M_GRP_USED, gidstr );
				adumprec(ADT_MOD_GRP,EX_ID_EXISTS,strlen(cmdline),cmdline);
				exit( EX_ID_EXISTS );
			}
			break;

		case INVALID:
			errmsg( M_GID_INVALID, gidstr );
			adumprec(ADT_MOD_GRP,EX_BADARG,strlen(cmdline),cmdline);
			exit( EX_BADARG );
			/*NOTREACHED*/

		case TOOBIG:
			errmsg( M_TOOBIG, gid );
			adumprec(ADT_MOD_GRP,EX_BADARG,strlen(cmdline),cmdline);
			exit( EX_BADARG );
			/*NOTREACHED*/

		}

	} else gid = -1;

	if( newname ) {
		switch( valid_gname( newname, NULL ) ) {
		case INVALID:
			errmsg( M_GRP_INVALID, newname );
			adumprec(ADT_MOD_GRP,EX_BADARG,strlen(cmdline),cmdline);
			exit( EX_BADARG );
		case NOTUNIQUE:
			if(!nisname(newname)) {
				errmsg( M_GRP_USED, newname );
				adumprec(ADT_MOD_GRP,EX_NAME_EXISTS,strlen(cmdline),cmdline);
				exit( EX_NAME_EXISTS );
			}
		}
	}

	if (customdir && (execScript(customdir,"groupmod.pre") != 0))
		exit(EX_SCRIPT_ERR);
	valret = mod_group(grpname, gid, newname);
	adumprec(ADT_MOD_GRP,valret,strlen(cmdline),cmdline);
	if (valret != EX_SUCCESS)
		errmsg( M_GROUP_UPDATE );
	else if (customdir)
		execScript(customdir,"groupmod.post");

	exit(valret);
	/*NOTREACHED*/
}
/*
 * Checks to see if NIS is up and running
 */
nis_check()
{
	if (domain == NULL) {
		if (yp_get_default_domain(&domain)){
			return(-1);
		}
	}
	return(0);
}
nis_getgrp(grp)
char *grp;
{
	register char *bp;
	char *val = NULL;
	int vallen, err;
	char *key = grp;

	if (nis_check() < 0)
		return(-1);

	if (err = yp_match(domain, "group.byname", key, strlen(key),
				&val, &vallen)) {
				return(err);
	}
}
