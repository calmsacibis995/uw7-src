/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)security.h	1.2"
#ident "$Header$"

#ifndef	CMD_INET_SECURE
#define	CMD_INET_SECURE

#ifdef	SYSV
/* security.h: various macros used by the inet services to
** 	deal with both Manditory Access Control and
**	the Least Privilege Mechanism.
** See ./security.c for more info.
**/

#include	<errno.h>
#include	<sys/secsys.h>
#include	<priv.h>

#ifndef	TRUE
#define	TRUE	(1)
#endif
#ifndef	FALSE
#define	FALSE	(0)
#endif 

/* CLR_MAXPRIVS_FOR_EXEC is a macro placed directly before
* an inet service potentially exec's a shell stricly for a user
* request.  This prevents a network user from gaining inheritable
* privileges for the particular file the user will request.
*
* If this kernel is running with an ID based privilege
* mechanism, check if  that  ID is equal to ``_loc_privid''.
* If so, set  the ``_loc_clrprivs'' flag to  0 so privileges
* are NOT cleared for a process with the ``privileged''
* ID before ``exec''ing the shell.
*
* If it is not ID based, then clear ALL process privileges
*
* The (_loc_privid == getuid()) code provides COMPATIBILITY with
* UID based privilege mechanism.
*
* IMPORTANT: Since this may clear all of the process privileges,
* if the exec() call following this fails, this process will resume
* without privileges, which may leave you dead in the water.
*
* NOTE: This macro is should not be used when the daemon does intend
* for the exec'd binary to have privileges.  This macro need not be
* always used before an exec when the absolute pathname
* is known not to inherit privileges (/usr/bin/sh, for example).
*/

#define	CLR_MAXPRIVS_FOR_EXEC					\
	{								\
		int	_loc_privid,					\
			_loc_clrprivs = TRUE,				\
			_loc_olderrno;					\
		uid_t	_loc_id_priv,					\
			_loc_uid;					\
									\
		_loc_olderrno = errno;					\
		_loc_uid = geteuid();					\
		if ((_loc_uid == OLD_ROOT_UID)				\
		    || (((_loc_id_priv = secsys(ES_PRVID, 0)) >= 0)	\
			&& (_loc_uid == _loc_id_priv))) {		\
				_loc_clrprivs = FALSE;			\
		}							\
		if (_loc_clrprivs) {					\
			procprivl(CLRPRV, pm_max(P_ALLPRIVS), 0);	\
		}							\
		errno = _loc_olderrno;					\
	}

/* ENABLE_WORK_PRIVS is a macro placed directly before
* code that requires access to network devices or otherwise
* restricted resources.  In particular, this must be used in
* services that require privilege to open reserved ports
* and then must turn off the privilege before performing
* a user request in this same process - or the exec is
* not soon enough in the code path.
* This prevents a network user from exploiting fixed
* privileges for the particular file the user will request.
* Since the privileges are manipulated in the Working set but
* are still in the MAX set, these privileges are not "lost"
* and can be enabled/disabled as particular code segments need.
* Used with DISABLE_WORK_PRIVS.
*/

#define	ENABLE_WORK_PRIVS						\
	{								\
		int	_loc_olderrno;					\
									\
		_loc_olderrno = errno;					\
		procprivl(SETPRV, pm_work(P_ALLPRIVS), 0);		\
		errno = _loc_olderrno;					\
	}

/* DISABLE_WORK_PRIVS is a macro placed in the beginning of 
* programs and directly after code that requires access to
* network devices or otherwise restricted resources.
* In particular, this must be used in
* services that require privilege to open reserved ports
* and then must turn off the privilege before performing
* a user request in this same process - or the exec is
* not soon enough in the code path.
* This prevents a network user from exploiting fixed
* privileges for the particular file the user will request.
* Since the privileges are manipulated in the Working set but
* are still in the MAX set, these privileges are not "lost"
* and can be enabled/disabled as particular code segments need.
* Used with ENABLE_WORK_PRIVS.
*/

#define	DISABLE_WORK_PRIVS						\
	{								\
		int	_loc_olderrno;					\
									\
		_loc_olderrno = errno;					\
		procprivl(CLRPRV, pm_work(P_ALLPRIVS), 0);		\
		errno = _loc_olderrno;					\
	}

/* CLR_WORKPRIVS_NON_ADMIN(ARG):
** If the effective user id is root or secsys(ES_PRVID, 0),
** the daemon must run with privileges in the working set turned on.
** If the effective user id is NEITHER root NOR secsys(ES_PRVID, 0),
** the daemon must run with only enough privileges to acess the network
** file descriptors in the DEV_PRIVATE device state.
*/

	/* This function is defined in ./security.c */
int	CLR_WORKPRIVS_NON_ADMIN();
#define	OLD_ROOT_UID	0

#else	SYSV
#define	CLR_MAXPRIVS_FOR_EXEC
#define	ENABLE_WORK_PRIVS
#define	DISABLE_WORK_PRIVS
#define	CLR_WORKPRIVS_NON_ADMIN(ARG)	ARG
#endif	SYSV

#endif	CMD_INET_SECURE
