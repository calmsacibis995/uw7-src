/* @(#)sanity.c	1.5
 *
 * Copyright (c) 1996 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 * Revision history:
 *
 * 25 June 97     tonylo
 *	Fix made to filecheck(), so that directories are checked properly
 *
 */


/*
 * sanity.c - perform sanity checks on the environment at startup time,
 * and report any errors before we disassociate from the controlling tty,
 * start up our threads, and do other stuff which makes it hard to give
 * feedback to the users.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "slurp.h"
#include "globals.h"
#include "portable.h"
#include "ldaplog.h"

#define FC_DIRBAD	1
#define FC_DIRUNREAD	2
#define FC_DIRUNWRITE	4
#define FC_FILEBAD	8
#define FC_FILEUNREAD	16
#define FC_FILEUNWRITE	32

#define MSG_NOREP1 \
    1,180,"No replicas in configuration file \"%s\"!\n"
#define MSG_NOREPLOGFILE \
    1,181,"No \"replogfile\" directive given\n"
#define MSG_NODIR \
    1,182,"%s: directory does not exist\n"
#define MSG_DIRNOREAD \
    1,183,"%s: directory not readable\n"
#define MSG_FILENOREAD \
    1,184,"%s: file not readable\n"
#define NOREPLOGDIR \
   1,185,"no \"replogfile\" directive given\n"
#define MSG_FILENOWRITE \
    1,186,"%s: file not writeable\n"


/*
 * Forward declarations
 */
#ifdef NEEDPROTOS
static unsigned int filecheck( char * );
#else /* NEEDPROTOS */
static unsigned int filecheck();
#endif /* NEEDPROTOS */



/*
 * Take a look around to catch any fatal errors.  For example, make sure the
 * destination directory for our working files exists, check that all
 * pathnames make sense, and so on.  Returns 0 is everything's ok,
 # -1 if there's something wrong which will keep us from functioning
 * correctly.
 *
 * We do all these checks at startup so we can print a reasonable error
 * message on stderr before we disassociate from the controlling tty.  This
 * keeps some fatal error messages from "disappearing" into syslog.
 */

int
sanity()
{
    int	err = 0;
    int rc;

    /*
     * Are there any replicas listed in the slapd config file?
     */
    if ( sglob->replicas == NULL ) {

	logError(get_ldap_message(MSG_NOREP1,sglob->slapd_configfile));
	err++;
    }

    /*
     * Make sure the directory housing the slapd replogfile exists, and
     * that the slapd replogfile is readable, if it exists.
     */
    if ( sglob->slapd_replogfile[0] == '\0' ) {

	logError(get_ldap_message(MSG_NOREPLOGFILE));
	err++;
    } else {
	rc = filecheck( sglob->slapd_replogfile );
	if ( rc & FC_DIRBAD ) {

	    logError(get_ldap_message(MSG_NODIR,
		sglob->slapd_replogfile));

	    err++;
	} else if ( rc & FC_DIRUNREAD ) {

	    logError(get_ldap_message(MSG_DIRNOREAD,
	        sglob->slapd_replogfile ));

	    err++;
	} else if (!( rc & FC_FILEBAD) && ( rc & FC_FILEUNREAD )) {

	    logError(get_ldap_message(MSG_FILENOREAD,
	        sglob->slapd_replogfile ));
	    err++;
	}
    }

    /*
     * Make sure the directory for the slurpd replogfile is there, and
     * that the slurpd replogfile is readable and writable, if it exists.
     */
    if ( sglob->slurpd_replogfile[0] == '\0' ) {

	logError(get_ldap_message(NOREPLOGDIR));

	err++;
    } else {
	rc = filecheck( sglob->slurpd_replogfile );
	if ( rc & FC_DIRBAD ) {

	    logError(get_ldap_message(MSG_NODIR,
		sglob->slurpd_replogfile ));

	    err++;
	} else if ( rc & FC_DIRUNREAD ) {

	    logError(get_ldap_message(MSG_DIRNOREAD,
		sglob->slurpd_replogfile));

	    err++;
	} else if ( !( rc & FC_FILEBAD ) && ( rc & FC_FILEUNREAD )) {

	     logError(get_ldap_message(MSG_FILENOREAD,
		sglob->slurpd_replogfile ));

	    err++;
	} else if ( !( rc & FC_FILEBAD ) && ( rc & FC_FILEUNWRITE )) {

	    logError(get_ldap_message(MSG_FILENOWRITE,
		sglob->slurpd_replogfile));

	    err++;
	}
    }

    /*
     * Make sure that the directory for the slurpd status file is there, and
     * that the slurpd status file is writable, if it exists.
     */
    rc = filecheck( sglob->slurpd_status_file );
    if ( rc & FC_DIRBAD ) {

	 logError(get_ldap_message(MSG_NODIR,
		sglob->slurpd_status_file ));

	err++;
    } else if ( rc & FC_DIRUNREAD ) {

	logError(get_ldap_message(MSG_DIRNOREAD,
		sglob->slurpd_status_file ));
	err++;
    } else if ( !( rc & FC_FILEBAD ) && ( rc & FC_FILEUNREAD )) {

	logError(get_ldap_message(MSG_FILENOREAD,
		sglob->slurpd_status_file ));

	err++;
    } else if ( !( rc & FC_FILEBAD ) && ( rc & FC_FILEUNWRITE )) {

	logError(get_ldap_message(MSG_FILENOWRITE,
		sglob->slurpd_status_file ));
	err++;
    }
    
    if( err != 0 ) {
	fprintf(stderr, "Number of errors in sanity()=%d\n", err);
    }
    return ( err == 0 ? 0 : -1 );
}



/*
 * Check for the existence of the file and directory leading to the file.
 * Returns a bitmask which is the logical OR of the following flags:
 *
 *  FC_DIRBAD:		directory containing "f" does not exist.
 *  FC_DIRUNREAD:	directory containing "f" exists but is not readable.
 *  FC_DIRUNWRITE:	directory containing "f" exists but is not writable.
 *  FC_FILEBAD:		"f" does not exist.
 *  FC_FILEUNREAD:	"f" exists but is unreadable.
 *  FC_FILEUNWRITE:	"f" exists but is unwritable.
 *
 * The calling routine is responsible for determining which, if any, of
 * the returned flags is a problem for a particular file.
 */
static unsigned int
filecheck(
    char	*f
)
{
    char		dir[ MAXPATHLEN ];
    char		*p;
    unsigned int	ret = 0;

    strcpy( dir, f );
    p = strrchr( dir, '/' );
    if ( p != NULL ) {
	*p = '\0';
    }

    if ( access( dir, F_OK ) < 0 ) {
	ret |= FC_DIRBAD;
    }
    if ( access( dir, R_OK ) < 0 ) {
	ret |= FC_DIRUNREAD;
    }
    if ( access( dir, W_OK ) < 0 ) {
	ret |= FC_DIRUNWRITE;
    }
    if ( access( f, F_OK ) < 0 ) {
	ret |= FC_FILEBAD;
    }
    if ( access( f, R_OK ) < 0 ) {
	ret |= FC_FILEUNREAD;
    }
    if ( access( f, W_OK ) < 0 ) {
	ret |= FC_FILEUNWRITE;
    }

    return ret;
}

