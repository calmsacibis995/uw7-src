#ident	"@(#)pcintf:bridge/const.h	1.1.1.3"
/* SCCSID(@(#)const.h	6.6	LCC);	/* Modified: 22:29:12 8/21/91 */

/*****************************************************************************

	Copyright (c) 1984, 1988 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/*				Program Constants			*/
#ifndef CONST_H
#define CONST_H

#ifndef NULL
#define NULL         0
#endif /* ~NULL */

#ifndef	NSIG
#include <sys/signal.h>
#endif	/* !NSIG */

#if defined(USE_SIGSET)
#define signal sigset  		/* use reliable signals if available */
#endif /* USE_SIGSET */

#if	0
There's no good reason for this.  If there's really a problem it needs to
be fixed.  [ jbrown 06/19/90 ]
/* The C library versions of mem* don't seem to work with our code. */
/* If we link in memory.o, we get duplicate symbol errors from ld */
/* when using the shared C library.  So, we use this hack to rename */
/* all the mem* calls to Mem*.  The routines in memory.c have been  */
/* renamed for real. */

#define memccpy Memccpy
#define memcpy Memcpy
#define memcmp Memcmp
#define memchr Memchr
#define memset Memset
#endif	/* 0 */

#ifndef	BERKELEY42

#ifndef SIGCLD
#define SIGCLD		SIGCHLD
#endif	/* !SIGCLD */

#ifndef SIGUSR1
#define	SIGUSR1 	16
#endif	/* !SIGUSR1 */

#ifndef SIGUSR2
#define	SIGUSR2 	17
#endif	/* !SIGUSR2 */

#define	SIG_CHILD	SIGCLD
#define SIG_DBG1        SIGUSR1
#define SIG_DBG2        SIGUSR2
#else	/* BERKELEY42 */
#define	SIG_CHILD	SIGCHLD
#define SIG_DBG1        SIGURG
#define SIG_DBG2        SIGSTOP
#endif	/* BERKELEY42 */


#ifndef FALSE
#define FALSE           0
#endif

#ifndef TRUE
#define TRUE            1
#endif

/* Defines used for translation types requested */
/*      NONE		0          No translation tables set */
#define D2D		1	/* DOS-to-DOS translation */
#define D2U		2	/* DOS-to-UNIX translation */
#define U2D		3	/* UNIX-to-DOS translation */
#define U2U		4	/* UNIX-to-UNIX translation */

#define SUCCESS 	0
#define FAILURE 	1
#define CLEAR		0
#define SET		1
#define	INITIALIZED	0
#define	RUNNING		1
#define	SEND_RESPONSE	1
#define	SEND_NO_RESPONSE	0
#define CERTAIN		0
#define	NOT_CERTAIN	4
#define	MAPPED		0
#define DONT_CARE	0
#define	UNMAPPED	1
#define	MODIFY_CONTEXT	1
#define IGNORECASE	2
#define	NO_SWAP		0
#define NEW_CONTEXT	0


/*		    	Configuration Constants				*/



#define CONF		0
#define	BRIDGE		1
#define SHELL		2
#define	DAEMON		3
#define	NETMGR		4
#define	UNSOLICITED	4

#define	PCIDIR		"/usr/pci"
#define	PCIBIN		"/usr/pci/bin"

#if	defined(ETHNETPCI) && ( defined (UDP42) || defined(UDP41C) )
#ifdef	BERKELEY42
#define WTMP_FILE	"/usr/adm/wtmp";
#define UTMP_FILE	"/etc/utmp";
#endif	/* BERKELEY42 */
#endif  /* ETHNETPCI && UDP42 || UDP42C */

#define	PCIPTYS		"/usr/pci/pciptys"

#ifdef	AIX_RT
/*	Full path name of the penable program. */
#define	PENABLE		"/etc/penable"	
#endif	/* AIX_RT */

#ifdef JANUS
#define LOGDIR          "/usr/tmp"
#define ERR_LOGGER      "exec /usr/lib/merge/mlogger"
#else
#define	LOGDIR		"/usr/spool/pcilog"
#define	ERR_LOGGER	"exec /usr/pci/bin/errlogger"
#endif
#define	CONSVR_LOG	"consvr-log"
#define	MAPSVR_LOG	"mapsvr-log"
#define	LOADPCI_LOG	"loadpci"
#define	DOSSVR_LOG	"dossvr"
#define	DOSOUT_LOG	"dossvr"
#ifdef	RESTRICT_USER
#define	RESTRICTFILE	"/usr/pci/restrict-login"
#endif	/* RESTRICT_USER */

/* This stuff is used for wildcard, etc. matching */
#define	DOT		'.'
#define	STAR		'*'
#define	QUESTION	'?'
#define	WILDPREFIX	"????????"

#ifdef	BERK42FILE
#define	FS_TABLE	"/etc/fstab"
#define	DISKTAB		"/usr/pci/DISKTAB"
#else
#define	MNT_TABLE	"/etc/mtab"
#endif	/* BERK42FILE */

#ifdef	SYS5
#ifndef	LOCUS
#undef	MNT_TABLE
#define	MNT_TABLE	"/etc/mnttab"
#endif	/* !LOCUS */

#ifndef	IX370
#define	TTY_VMIN	1
#define	TTY_VTIME	0
#else	/* IX370 */
#define	TTY_VMIN	64
#define	TTY_VTIME	2
#endif	/* IX370 */
#endif	/* SYS5 */

#ifdef	ETHNETPCI

#define	PCIDOSSVR	"pcidossvr.ip"
#define	PCIOUT		"pcidosout.ip"

#if defined(UDP42) || defined(UDP41C)
#define ANY_PORT	0
#define BCAST43		0x01
#define USESUBNETS	0x02
#define	NETDEV		"/dev/udp"
#endif  /* UDP42 || UDP41C */

#endif	/* ETHNETPCI */

#ifdef	RS232PCI
#define	PCIOUT		"pcidosout.232"
#define	PCIDOSSVR	"pcidossvr.232"
#endif	/* RS232PCI */

/*		Constants Used In Bridge (by pckframe())		*/


#define	NO_DES		0
#define	NO_CNT		0
#define NO_CONST	0
#define	NO_SIZE		0
#define	NO_DATE		0
#define	NO_ATTR		0
#define	NO_MODE		0
#define NO_OFF		0
#define OFF		1
#define	MODE		1
#define	SIZE		1
#define	DATE		1
#define	ATTR		1

#define	DSVR_QSIZE	2

#ifdef RS232PCI
#define MAX_FRAME       256
#endif /* RS232PCI */

#ifdef ETHNETPCI
#define	ETH_IFUDGE	8		/* input fudge */
#define	ETH_OFUDGE	0		/* output fudge */
#define	C_MAX_FRAME	(100+ETH_IFUDGE)
#define	M_MAX_FRAME	(256+ETH_IFUDGE)
#define MAX_FRAME	(1088+ETH_IFUDGE)	/* Maximum input frame size
					*  the +ETH_IFUDGE is because of the 3b5
					*/
#endif /* ETHNETPCI */

#define	MAX_BURST	128		/* Maximum chars to send PC emulator */
#define MAX_READ	256		/* Maximum size of termout frame */
#define	MAX_CWD		128		/* Size of maximum cwd		*/
#define	NDRIVE		2		/* Max number of simultaneous drives */
#define PTYNAMELEN      20              /* Size of entry in pciptys */
#define MAX_SITES	50		/* Maximum site entries in table */
#define MAXFILES        126              /* Size of file context cache */
#define	MAX_FSTR_LEN	1024		/* largest feature string */

/*----------------------------------------------------------------------*/
/* Filename dimensions...						*/
/* MAX_FN_COMP	maximum length of one component of a file name.		*/
/* MAX_FN_TOTAL	maximum length of a file name, including directories	*/
/*									*/
/* The idea is that we look around to find a value from the supplied	*/
/* header files.  If we can't find anything, we guess.			*/
/* REAL_NAME_MAX and REAL_PATH_MAX are for use in systems where		*/
/* NAME_MAX and PATH_MAX are defined, but lie.  For instance, on System	*/
/* V rel 4 "beta", NAME_MAX is the limit for sys5 filesystems, but	*/
/* there can be other filesystem types with larger limits.		*/
/* It's intended that you would set REAL_NAME_MAX and REAL_MAXPATH in	*/
/* sysconfig.h or the makefile.  [ jbrown 07/03/90 ]			*/
/*----------------------------------------------------------------------*/

#ifdef	REAL_NAME_MAX
#	define	MAX_FN_COMP	REAL_NAME_MAX
#else
#	ifdef	NAME_MAX
#		define	MAX_FN_COMP	NAME_MAX
#	else
#		ifdef	MAXNAMELEN
#			define	MAX_FN_COMP	MAXNAMELEN
#		else
#			ifdef	DIRSIZ
#				define	MAX_FN_COMP	DIRSIZ
#			else
#				define	MAX_FN_COMP	255
#			endif
#		endif
#	endif
#endif

#ifdef	REAL_PATH_MAX
#	define	MAX_FN_TOTAL	REAL_PATH_MAX
#else
#	ifdef	PATH_MAX
#		define	MAX_FN_TOTAL	PATH_MAX
#	else
#		ifdef	MAXPATHLEN
#			define	MAX_FN_TOTAL	MAXPATHLEN
#		else
#			define	MAX_FN_TOTAL	1023
#		endif
#	endif
#endif

/*
   MAX_PORTS;		Maximum number of connections
   CT_HASH_SIZE:	Size of hash tables for indexing connection table:
			  Must be a prime number
			  Should be greater than (MAX_PORTS * 1.25)
			  (For ~5 probes per fail; ~2 probes per success)
*/
#if	defined(USER2000)
#define	MAX_PORTS	2000
#define	CT_HASH_SIZE	2503
#endif
#if	defined(USER64)
#define	MAX_PORTS	64		/* Maximum number of connections */
#define	CT_HASH_SIZE	83
#endif
#if	defined(USER32)
#define	MAX_PORTS	32		/* Maximum number of connections */
#define	CT_HASH_SIZE	41
#endif
#if	defined(USER16)
#define	MAX_PORTS	16		/* Maximum number of connections */
#define	CT_HASH_SIZE	21
#endif

#define	MAX_PATTERN	16		/* Maximum length of search pattern */
#define	MAXTBL		20		/* Maximum contexts in search table */
#define DOS_NAME	8		/* Maximum length of MS-DOS filename */
#define OFFSET_LEN	3		/* Length of offset string */
#define CHAR_OFFSET	5		/* Location begining embedded offset */
#define DOS_EXT		3		/* Maximum length of MS-DOS extension */
#define RECORD_SIZE	32		/* Size of directory record	*/
#define	PIPEPREFIXLEN	6
#define FREESPACE_UNITS	4096		/* Units free-space is represented in */
#define	SRC		SZNADDR
#define	DST		SZNADDR
#define	SYNC		'\030'		/* Start of bridge frame character */
#define SYNC_2		'\034'		/* Emualtor flow control character */
#define SYNCNULL	"\34\0"
#define SYNC2		"\34"		/* Special SYNC character */
#define DELAYTIME	5		/* Timeout period on rs232 read */
#define PROBE_INTERVAL	36		/* Period of daemon connection test */
#define RECEIVED	0
#define	MISSING		2
#define PROTECTED	6		/* Number of unSYNCable char's	*/
#define NET		0
#define	STDIN		0		/* Descriptor for standard input */
#define PAGED		-1
#define NO_FDESC	-2
#define	NTRIES		20
#define	TRIES		10		/* Times a frame is retransmitted */
#define	RESET		1		/* Resets internal sequence numbers */
#define PRINT		1
#define	CREATENEW	2	/* Create file only if it doesn't exist */
#define TEMPFILE	3	/* Create temp file mode for create */
#define FCBCREATE	4	/* FCB style create */
#define	DUP_FILE_IN_DIR	2		/* Duplicate filename in directory */



/*    			EXEC Argument Offsets				*/

#define	DESCRIPTORS	4	/* Number of file descriptors passed */

#define NARGS           (DESCRIPTORS + 3) /* Number of exec args */


#ifdef	ETHNETPCI
					/* ATT assigned Aplication ID */
#define	PCIAID1		0x76
#define	PCIAID2		0x77
#define	PCIAID		0x7677
#endif	/* ETHNETPCI */


/*			    Bridge States 				*/


#define	READ_LAST	3
#define READ_MTF	4




/*			    Transmission Status Codes 			*/



#define ACK		0
#define NEW		1 * REQUESTS
#define NEW_MTF		2 * REQUESTS
#define EXT_MTF		3 * REQUESTS
#define EXT		4 * REQUESTS
#define	RFNM		5 * REQUESTS

/* The only use for the "REQUESTS" define is in the above five defines.
The number "41" is not significant anymore, except that the dos side
bridge has equates for the above five items, and uses "41" to make them.
The values of the above 6 defines are only significant in that they are
different from each other. There is code in the server that suggests that if
we make "REQUESTS" equal "1" (i.e. remove it), then things will work: e.g. in
p_server.c a status of "1" and "NEW", "2" and "NEW_MTF", and so on, are
treated the same. The dos bridge would need to be changed at the same time,
so this will probably never happen.
*/
#define REQUESTS        41


/*				Request Codes				*/


#define EXIT		1
#define PCI_CREATE	2
#define PCI_DELETE  	3
#define OLD_OPEN 	4
#define NEW_CLOSE	5
#define READ_SEQ	6
#define READ_RAN	7
#define WRITE_SEQ	8
#define	WRITE_RAN	9
#define CHDIR		10
#define MKDIR		11
#define RMDIR		12
#define CHMOD		13
#define RENAME		14		/* Old style rename	*/
#define SEARCH		15
#define NEXT_SEARCH	16
#define TIME_DATE	17
#define NEW_OPEN	18
#define FS_STATUS	19
#define SET_STATUS	20
#define	FILE_SIZE	21
#define L_SEEK		22
#define NEXT_FIND	23
#define RS232_ALIVE	24		/* Host to PC when 232 service ready */
#define GETWD		25
#define PROBE		26
#define EST_BRIDGE	27
#define	EST_SHELL	28
#define TERM_SHELL	29
#define SEND_MAP	30
#define CONNECT		31
#define	DISCONNECT	32
#define OLD_CLOSE	33
#define LOCK_MODE	34
#define SECURITY_CHECK  35
#define PC_CRASH        36
#define	CONSVR_HERE	37		/* Connection server notification */
#define	CONSVR_BYE	38		/* Connection server shutting down */
#define GET_INIT_DATA   39              /* Get "PC Server" initialization */
#define DEVICE_INFO_C   40              /* DOS ioctl call (44H) */
#define LOCK		41		/* File lock and unlock request */
#define	GET_EXT_ERROR	42		/* Get extra error information */
#define CONSOLE_READ    43              /* DOS functions 1,6,7,8,a,b */
#define	LOG_CTRL	44		/* Control remote logging functions */
#define	LOG_MSG		45		/* Log a message from workstation */
#define	UEXEC		46		/* Exec a program on the server */
#define	UWAIT		47		/* Wait for UEXED'd process to exit */
#define	UKILL		48		/* Send a signal to a UEXEC'd process */
#define S_LOGIN		49		/* Ask consvr to start up a login */
#define K_LOGIN		50		/* Ask consvr to kill the em login */
#define MAPNAME		51		/* Map file name */
#define SET_SDEBUG	52		/* turn on logging from dos bridge */
#define IPC		53		/* Sys V IPC Functions	*/
#define X_MSGGET   	54		/* Sys V: make queue    */
#define X_MSGSND   	55		/* Sys V: send to msg queue */
#define X_MSGRCV   	56		/* Sys V: rec'v msg from queue */
#define X_MSGCTL   	57		/* Sys V: msg queue control */
#define X_SEMGET   	58		/* Sys V: make semaphore */
#define X_SEMOP		59		/* Sys V: operate on semaphore */
#define X_SEMCTL   	60		/* Sys V: semaphore control */
#define	RENAME_NEW	61		/* New style rename	*/
#define	FIND_FIRST	62		/* New style command	*/
#define	GET_SITE_ATTR	63		/* get server attributes - consvr */
#define	DOS_NLS_INFO	64		/* get dos country and code page */


/*		Transaction Response Codes 	(i.e.- out.hdr.res)	*/


#define INVALID_FUNCTION 	(unsigned char)1
#define FILE_NOT_FOUND		(unsigned char)2
#define	PATH_NOT_FOUND		(unsigned char)3
#define	TOO_MANY_FILES		(unsigned char)4
#define	ACCESS_DENIED		(unsigned char)5
#define	FILDES_INVALID		(unsigned char)6
#define	MEMORY_BLOCK_DESTROYED	(unsigned char)7
#define	INSUFFICIENT_MEMORY	(unsigned char)8
#define	MEM_BLOCK_INVALID	(unsigned char)9
#define	ENVIRONMENT_INVALID	(unsigned char)10
#define	FORMAT_INVALID		(unsigned char)11
#define	ACCESS_CODE_INVALID	(unsigned char)12
#define	DATA_INVALID		(unsigned char)13
#define	DRIVE_INVALID		(unsigned char)15
#define ATTEMPT_TO_REMOVE_DIR	(unsigned char)16
#define	NOT_SAME_DEVICE		(unsigned char)17
#define	NO_MORE_FILES		(unsigned char)18

/* The following codes do not directly match DOS return codes */
#define	LOGIN_FAILED		(unsigned char)19
#define	INTERNAL_ERRORS		(unsigned char)20
#define	SYSTEM_GOING_DOWN	(unsigned char)21
#define	NO_SESSION		(unsigned char)22
#define	NO_PORTS		(unsigned char)23
#define	OTHER			(unsigned char)24
#define	DUPLICATE_CONNECTION	(unsigned char)25
#define	INV_SHELL		(unsigned char)26

#define SHARE_VIOLATION		(unsigned char)32

/* The following codes DO directly match DOS return codes (!!) */
#define LOCK_VIOLATION		(unsigned char)33
#define FILE_EXISTS		(unsigned char)80

#define RESPONSES		26		/* number of Response Codes */


/*		Valid MS-DOS File Attributes	(i.e.- in.hdr.attr)	 */


#define	READ_ONLY	0x01			/* Read only set */
#define HIDDEN		0x02			/* Hidden file	*/
#define SYSTEM		0x04			/* System file	*/
#define	VOLUME_LABEL	0x08			/* Volume label	*/
#define SUB_DIRECTORY	0x10			/* Sub Directory */
#define ARCHIVE         0x20                    /* Archive flag  */




/*
 *	Valid Event/Actions  - (action/event = in.hdr.stat * in.hdr.req;)	 *				Action/Event index is the product of the
 *				transmission status and the request
 */	


/*				Utility Macro's				*/

extern char *myname;


/* Constants for open mode lock checks */
#define MHASHLI		23	/* Size of hash table - make it prime */
#define MLOCKI		50	/* Number of inodes simultaneously open */
#define MLOCKF		100	/* Number of simultaneous opens */

/* Constants for disappearing! temp files */
#define NSLOT		20	/* Number of displaced tempfiles */
#define NPRINT		5	/* Number of print streams */

/* Unix protection mode bits */
#define	O_WRITE		0200	/* Owner write permission bit */
#define	G_WRITE		020	/* Group write permission */
#define	W_WRITE		02	/* World write */
#define	ALL_WRITE	0222	/* All write bits */
#define	WRITE_ACCESS	02  	/* All write bits */

extern int
	errno;

#endif /* !CONST_H */
