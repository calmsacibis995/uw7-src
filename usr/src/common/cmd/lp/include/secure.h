/*		copyright	"%c%" 	*/


#ident	"@(#)secure.h	1.2"
#ident	"$Header$"

#ifndef	_LP_SECURE_H
#define _LP_SECURE_H

#include <sys/types.h>
#include <mac.h>
#include "msgs.h"

/**
 ** The disk copy of the secure request files:
 **/

/*
 * There are 9 fields in the secure request file.
 */
# define SC_MAX  	10

# define SC_REQID	0	/* Original request id */
# define SC_UID		1	/* Originator's user ID */
# define SC_USER	2	/* Originator's real login name */
# define SC_GID		3	/* Originator's group ID */
# define SC_SIZE	4	/* Total size of the request data */
# define SC_DATE	5	/* Date submitted (in seconds) */
# define SC_SYSTEM	6	/* Originating system */
# define SC_LID		7	/* Originators's MAC-level */
# define SC_STATUS	8	/* Status of the request: accepted or not.  */
# define SC_REM_REQID	9	/* Request id of job on remote host */

#define	SC_STATUS_UNACCEPTED	0x0
#define	SC_STATUS_ACCEPTED	0x1
/**
 ** The internal copy of a request as seen by the rest of the world:
 **/

typedef	struct
{
	uint	status;
	uid_t	uid;
	gid_t	gid;
	level_t	lid;
	off_t	size;
	time_t	date;
	char	*system;
	char	*user;
	char	*req_id;
	char	*rem_reqid;

}  SECURE;

/**
 ** Various routines.
 **/

#ifdef	__STDC__

SECURE *	getsecure (char *);
int		putsecure (char *, SECURE *);
int		rmsecure (char *);
void		freesecure (SECURE *);

#else

SECURE *	getsecure ();
int		putsecure ();
int		rmsecure ();
void		freesecure ();

#endif	/*  __STDC__  */

#endif	/*  _LP_SECURE_H  */
