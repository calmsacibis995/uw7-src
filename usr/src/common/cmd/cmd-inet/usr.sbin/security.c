/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)security.c	1.2"
#ident "$Header$"

/* security.c: various functions used by the inet services to
** 	deal with both Manditory Access Control and
**	the Least Privilege Mechanism.
** See ./security.c for more info.
**/

#include	<unistd.h>
#include	<mac.h>
#include	"./security.h"

#ifdef	SECURITY_DEBUG

#include	<stdio.h>
#ifndef	NULL
#define	NULL	(0)
#endif

int	security_debug = 0;
FILE	*security_fd = NULL;

#define	SECURITY_DUMP(x)	if (security_debug && (NULL!=security_fd)) \
					fprintf x
#else
#define	SECURITY_DUMP(x)
#endif	SECURITY_DEBUG

/* is_enhancedsecurity()
**
** -returns TRUE if enhanced security Manditory Access Control is installed
** -returns FALSE if enhanced security MAC in not installed
** caches result for future in mac_state_cache.
*/

static int mac_state_cache = -1;

int
is_enhancedsecurity()
{
	register int _restore_errno;

	if (-1 != mac_state_cache) {
		return (mac_state_cache);
	}
	_restore_errno = errno;
	errno = 0;
	(void) devstat((char *)NULL, 0, (struct devstat *)NULL);
	if (!( (errno == ENOSYS) || (errno == ENOPKG))) {
		errno = _restore_errno;
		return (mac_state_cache = TRUE);
	} else {
		errno = _restore_errno;
		return (mac_state_cache = FALSE);
	}
}

#define WORKED		0
#define CANTGETLEVEL	WORKED
#define CANTGETDEVSTAT	WORKED
#define WRONGDEVATTR	2
#define CANTSETLEVEL	3
#define CANTSETDEVSTAT	4

/*
*
* fd_set_to_my_level sets up the security devices attributes for the
* file descriptor of the allocated clone device that corresponds
* to that device ( connection ). fd_set_to_my_level changes the
* (dev_level, the dev_h_level, and the dev_lolevel)
* of the file descriptor (connection) to the process' level.
* The device state is set to DEV_PUBLIC.
* Parameters:
*  connfd	clone device file descriptor
*/

int
fd_set_to_my_level(connfd)
	int	connfd;		/* Device file descripter */
{
	level_t	level;	/* devstat level fields */

	if (lvlproc(MAC_GET, &level) != 0) {
		SECURITY_DUMP((security_fd,"daemon can't get process level\n"));
		return CANTGETLEVEL;
	}

	return set_fd_level(connfd, DEV_PUBLIC, level, level, level);
}


/*
*
* set_fd_level sets up the security devices attributes for the
* file descriptor of the allocated clone device that corresponds
* to that device ( connection ). set_fd_level also changes the level
* of the file descriptor (connection).
* Parameters:
*  connfd	clone device file descriptor
*  State	Device database state: DEV_PUBLIC/DEV_PRIVATE
*  level	MAC level for flvlfile(connfd)
*  Hilevel	MAC Device database field
*  Lolevel	MAC Device database field
*/

int
set_fd_level(connfd, State, level, Hilevel, Lolevel)
	int	connfd;		/* Device file descripter */
	level_t	level, Hilevel, Lolevel;	/* devstat level fields */
{
	level_t	cur_connfd_level;
		/* Device Database structure for requested device */
	struct	devstat	devatr;	

	SECURITY_DUMP((security_fd,"level of connection request<%d>:\n",level));

	if (fdevstat(connfd, DEV_GET, &devatr) != 0){
		SECURITY_DUMP((security_fd,"Could not GET devstat\n"));
		return(CANTGETDEVSTAT);	/*First error return*/
	}
	SECURITY_DUMP((security_fd,	\
	    "%s%d,state:%d,mode:%d,hi/lolv:%d/%d,use_cnt:%d\n",\
	    "Device attributes DEV_GET: relflag:", \
	    devatr.dev_relflag,devatr.dev_state, \
	    devatr.dev_mode,devatr.dev_hilevel, \
	    devatr.dev_lolevel,devatr.dev_usecount));
	
	if (DEV_LASTCLOSE != devatr.dev_relflag) {
		SECURITY_DUMP((security_fd,	\
			"daemon: device did not admalloc -s correctly\n"));
		return (WRONGDEVATTR);
	}

	if (flvlfile(connfd, MAC_GET, &cur_connfd_level) != 0){
		SECURITY_DUMP((security_fd,"set level failed for connfd\n"));
		return(CANTGETLEVEL);
	}

	/* Level can be changed if and only if devatr.dev_state==DEV_PRIVATE */
	if (level!=cur_connfd_level) {
		if (devatr.dev_state != DEV_PRIVATE) {
			devatr.dev_state = DEV_PRIVATE;
			SECURITY_DUMP((security_fd,	\
					"daemon Note:%d:fd!DEV_PRIVATE %s%s", \
					connfd, \
					"& fd!=level, making fd ", \
					"DEV_PRIVATE first\n"));
			if (fdevstat(connfd, DEV_SET, &devatr) != 0) {
				SECURITY_DUMP((security_fd,	\
					"daemon Note:%d:fd!DEV_PRIVATE %s%s", \
					connfd, \
					"& fd!=level, couldn't make ", \
					"device DEV_PRIVATE\n"));
				return(CANTSETDEVSTAT);
			}
		}
		if (flvlfile(connfd,MAC_SET,&level)!=0) {
			SECURITY_DUMP((security_fd, \
					"set level failed for connfd\n"));
			return(CANTSETLEVEL);
		}
	}

	if ((devatr.dev_state == State) &&
		(devatr.dev_hilevel == Hilevel) &&
		(devatr.dev_lolevel == Lolevel)) {
			SECURITY_DUMP((security_fd,	\
				"Everything's been set for connfd\n"));
			return(WORKED);
	}

	devatr.dev_state	= State;
	devatr.dev_hilevel	= Hilevel;
	devatr.dev_lolevel	= Lolevel;

#ifdef SECURITY_DEBUG
	if (security_debug){
		level_t dbuglevel;

		if (flvlfile(connfd, MAC_GET, &dbuglevel) != 0){
			SECURITY_DUMP((security_fd,	\
			    "get level failed for connfd<%d>\n",connfd));
		}else{
			SECURITY_DUMP((security_fd,	\
			    "fd level sent back<%d>\n",	dbuglevel));
		}
	
	}
#endif SECURITY_DEBUG

	if (fdevstat(connfd, DEV_SET, &devatr) != 0){
		SECURITY_DUMP((security_fd,"daemon could not DEV_SET\n"));
		return(CANTSETDEVSTAT);
	}

#ifdef SECURITY_DEBUG
	if (security_debug){
		if (fdevstat(connfd, DEV_GET, &devatr) != 0){
			SECURITY_DUMP((security_fd,"Could not get DEV_GET\n"));
		} else {
			SECURITY_DUMP((security_fd,	\
			    "%s%d,state:%d,mode:%d,hi/lolv:%d/%d,use_cnt:%d\n",\
			    "Device attributes DEV_SET: relflag:", \
			    devatr.dev_relflag,devatr.dev_state, \
			    devatr.dev_mode,devatr.dev_hilevel, \
			    devatr.dev_lolevel,devatr.dev_usecount));
		}
	}
#endif SECURITY_DEBUG

	return(WORKED);
}


/* CLR_WORKPRIVS_NON_ADMIN should be used after
* setuid() or seteuid() to re-compute the privileges appropriate
* for that user.
*	CLR_WORKPRIVS_NON_ADMIN(set[ear]*uid((uid_t)uid))
* is the way the daemon source will use it.  Since the parameter is
* evaluated and this function immediately called, there can be no
* pre-svr4ES operations missed with the "wrong" set of privileges.
* Also, the internet services have a small code change.
*
* If the effective user id is "root" or secsys(ES_PRVID, 0),
* the daemon must run with privileges in the working set turned on.
* If the effective user id is NEITHER root NOR secsys(ES_PRVID, 0),
* the daemon must run with only enough privileges to acess the network
* file descriptors in the DEV_PRIVATE state.
*
* IMPORTANT: CLR_WORKPRIVS_NON_ADMIN(ARG) is a MACRO
* defined to ARG in #include <./security.h> when SYSV is undefined.
*/

#ifdef SYSV
#define MIN_PRIV_SET	pm_work(P_DEV)

int
CLR_WORKPRIVS_NON_ADMIN(ARG)
int ARG;
{
	int	_loc_priv_cmd = CLRPRV,
		_loc_olderrno;

	uid_t	_loc_id_priv,
		_loc_uid;
	_loc_olderrno	= errno;
	_loc_uid = geteuid();
	if ((_loc_uid == OLD_ROOT_UID)
	    || (((_loc_id_priv = secsys(ES_PRVID, 0)) >= 0)
		&& (_loc_uid == _loc_id_priv))) {
			_loc_priv_cmd = SETPRV;
	}
	procprivl(_loc_priv_cmd, pm_work(P_ALLPRIVS), 0);
	if (CLRPRV == _loc_priv_cmd) {
		procprivl(SETPRV, MIN_PRIV_SET, 0);
	}
	errno = _loc_olderrno;
	return ARG;
}
#endif SYSV
