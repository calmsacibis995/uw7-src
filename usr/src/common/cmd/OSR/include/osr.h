#ident	"@(#)OSRcmds:include/osr.h	1.1"
/* Below are a set of defines, used for things commonly defined in OSR,
 * which are not available in UW
 */

/* 1 Beyond the maximum possible GID
 *	defined in OSR /usr/include/sys/param.h to be "((gid_t) 65535)"
 *	use the value beyond GID_NOBODY defined in /usr/include/sys/param.h
 */
#define NULLGROUP	GID_NOBODY+1

/* The maximum size of a path
 *	defined in OSR in local places, equivalent to MAXPATHLEN in UW
 *	/usr/include/sys/param.h
 */
#define	PATHSIZE	MAXPATHLEN

/* The maximum size of a long file name
 *	defined in OSR as 255 in /usr/include/sys/param.h, equivalent to
 *	1 less than MAXNAMELEN defined in /usr/include/sys/param.h
 */
#define _SCO_NAMELEN	MAXNAMELEN-1

/* Used as a reserved process ID number
 *	defined in OSR as MAXPID+1 in /usr/include/sys/param.h, equivalent
 *	1 more than MAXPID defined in /usr/include/sys/param.h
 */
#define	RESPID		MAXPID+1

/* Macro to convert a character digit to its equivalent int
 *	defined in OSR as the below in /usr/include/ctype.h, there is no
 *	equivalent in UW
 */
#define toint(c)	((int)((c)-'0'))

/* Macro to convert a integer digit 0, 1, 2, 3, 4, 5, 6, 7, 8, or 9, to
 * its equivalent char digit.
 *	defined in OSR as the below in /usr/include/ctype.h, there is no
 *	equivalent in UW
 */
#define todigit(n)	((char)((n)+'0'))

/* Old procedure to use either stat or lstat, depending on the OS being used.
 * We're doing SVR4 only, so always use lstat
 */
#define statlstat lstat

/* A define originally used in OSR for a fcntl, which when successful, put
 * the device into block mode, where the offset was then counts of 512
 * byte blocks, instead of bytes
 */
#define F_GETBMODE	30
#define F_SETBMODE	31
