#ident	"@(#)pcintf:bridge/sysconfig.h	1.3"
/*	@(#)sysconfig.h	6.19	modified 14:08:07 12/18/91
 *
 *  This file contains all of the port specific defines.
 */

#if !defined(_SYSCONFIG_H)
#	define	_SYSCONFIG_H

#ifdef AIX_370

#define	POSIX			1
#define ALLOW_ROOT_LOGIN	1
#define BERK42FILE		1
#define BSDGROUPS		1
#define FAST_FIND		1
#define FAST_LSEEK		1
#define IX370			1
#define IXR2			1
#define LOCUS			1
#define PARAM_GETS_TYPES	1
#define RLOCK			1
#define SYS5			1
#define VERSION_MATCHING	1

#ifdef ETHNETPCI
#define BSD43			1
#define UDP42			1
#define USER2000		1
#endif

#endif	/* AIX_370 */


#ifdef AIX_386

#define _h_DOPTIONS		1	/* Kludge to suppress definition
					   of MERGE386 */

#define	POSIX			1
#define ALLOW_ROOT_LOGIN	1
#define BERK42FILE		1
#define BSDGROUPS		1
#define FAST_FIND		1
#define FAST_LSEEK		1
#define LOCUS			1
#define PARAM_GETS_TYPES	1
#define RLOCK			1
#define SYS5			1
#define VERSION_MATCHING	1
#define iAPX386			1

#ifdef ETHNETPCI
#define BSD43			1
#define UDP42			1
#define USER2000		1
#endif

#endif	/* AIX_386 */


#ifdef AIX_RT

#define DONT_START_GETTY	1
#define PARAM_GETS_TYPES	1
#define RLOCK			1
#define SYS5			1
#define VERSION_MATCHING	1
#define iAPX286			1

#ifdef ETHNETPCI
#define UDP42			1
#define USER2000		1
#endif

#endif	/* AIX_RT */


#ifdef CTSPC

#define	SYS5			1
#define USE_SIGSET		1
#define iAPX386			1
#define RLOCK			1
#define	SHADOW_PASSWD		1
#define FAST_FIND		1
#define	FAST_LSEEK		1

#ifdef ETHNETPCI
#define	BSD43			1
#define UDP42			1
#define USER64			1
#define	STREAMNET		1
#endif 

#ifdef RS232PCI
#define RS232_7BIT		1
#endif

#endif /* CTSPC */

#ifdef AIX_RS

#define FAST_FIND		1
#define FAST_LSEEK		1
#define DONT_START_GETTY	1
#define POSIX			1
#define PARAM_GETS_TYPES	1
#define RLOCK			1
#define SYS5			1
#define VERSION_MATCHING	1

#ifdef ETHNETPCI
#define BSD43			1
#define UDP42			1
#define USER2000		1
#endif

#endif	/* AIX_RS */


#if defined(SCO_ODT)
#	define	iAPX386				1
#	define	BSDGROUPS			1
#	define	USE_SETBASESG			1
#	define	SYS5				1
#	define	SecureWare			1
#	define	RLOCK				1
#	define	USER64				1
#	define	FAST_FIND			1
#	define	FAST_LSEEK			1
#	define	USE_SIGSET			1
#	define	REAL_NAME_MAX			512
#	if defined(ETHNETPCI)
#		define	UDP42			1
#		define	TLI			1
#		define	STREAMNET		1
#		define	BROADCAST_OPTION	1
#		define	BELLTLI			1
#	else
#		define	RS232_7BIT		1
#	endif
#endif	/* SCO_ODT */

#if defined(SCO_SVR32)
#	define	iAPX386				1
#	define	SYS5				1
#	define	SecureWare			1
#	define	RLOCK				1
#	define	USER64				1
#	define	FAST_FIND			1
#	define	FAST_LSEEK			1
#	define	USE_SIGSET			1
#	define	REAL_NAME_MAX			512
#	if defined(ETHNETPCI)
#		define	UDP42			1
#		define	TLI			1
#		define	STREAMNET		1
#		define	BROADCAST_OPTION	1
#		define	BELLTLI			1
#	else
#		define	RS232_7BIT		1
#	endif
#endif	/* SCO_SVR32 */

#if defined(ISC386)
#	define	iAPX386				1
#	define	SYS5				1
#	define	RLOCK				1
#	define	FAST_FIND			1
#	define	FAST_LSEEK			1
#	define	USE_SIGSET			1
#	define	REAL_NAME_MAX			512
#	define	POSIX_JC			1
#	define	SHADOW_PASSWD			1
#	define	BSD43				1
#	if defined(ETHNETPCI)
#		define	UDP42			1
#		define	TLI			1
#		define	STREAMNET		1
#		define	BROADCAST_OPTION	1
#	else
#		define	RS232_7BIT		1
#	endif
#endif	/* ISC386 */

#if defined(SVR4)
#	define	BSDGROUPS		1
#	define	SYS5			1
#	define	SYS5_4			1
#	define	POSIX			1
#	define	SHADOW_PASSWD		1
#	define	RLOCK			1
#	define	USER64			1
#	define	FAST_LSEEK		1
#	define	USE_SIGSET		1
#	define	REAL_NAME_MAX		512
#	define	STREAMS_PT		1
#	if defined(ETHNETPCI)
#		define BSD43			1
#		define	UDP42			1
#		define	TLI			1
#		define	STREAMNET		1
#		define	BROADCAST_OPTION	1
#		define	BELLTLI			1
#	else
#		define	RS232_7BIT	1
#	endif
#endif	/* SVR4 */
#endif	/* !_SYSCONFIG_H */
