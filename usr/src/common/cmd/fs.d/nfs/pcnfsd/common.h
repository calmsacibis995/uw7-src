#ident	"@(#)common.h	1.2"
#ident	"$Header$"

/*
**=====================================================================
** Copyright (c) 1986-1993 by Sun Microsystems, Inc.
**=====================================================================
*/

/*
**=====================================================================
** Any and all changes made herein to the original code obtained from
** Sun Microsystems may not be supported by Sun Microsystems, Inc.
**=====================================================================
*/

/*
**=====================================================================
**             C U S T O M I Z A T I O N   S E C T I O N              *
**=====================================================================
*/

/*
**---------------------------------------------------------------------
** Define the following symbol to enable the use of a 
** shadow password file
**---------------------------------------------------------------------
**/

#define SHADOW_SUPPORT

/*
**---------------------------------------------------------------------
** Define the following symbol to enable the logging 
** of authentication requests to /usr/adm/wtmp
**---------------------------------------------------------------------
**/

#define WTMP

/*
**---------------------------------------------------------------------
** Define the following symbol to use a cache of recently-used
** user names. This has certain uses in university and other settings
** where (1) the pasword file is very large, and (2) a group of users
** frequently logs in together using the same account (for example,
** a class userid).
**---------------------------------------------------------------------
*/

/* #define USER_CACHE */

/*
**---------------------------------------------------------------------
** Define the following symbol to build a version that
** will use the setusershell()/getusershell()/endusershell() calls
** to determine if a password entry contains a legal shell (and therefore
** identifies a user who may log in). The default is to check that
** the last two characters of the shell field are "sh", which should
** cope with "sh", "csh", "ksh", "bash".... See the routine get_password()
** in pcnfsd_misc.c for more details.
*/

/* #define USE_GETUSERSHELL */

/*
**---------------------------------------------------------------------
** Define the following symbol to build a version that
** will consult the NIS (formerly Yellow Pages) "auto.home" map to
** locate the user's home directory (returned by the V2 authentication
** procedure).
**---------------------------------------------------------------------
*/

/* #define USE_YP */

/*
**=====================================================================
**              " O T H E R  S T U F F "   S E C T I O N              *
**=====================================================================
*/

#ifndef assert
#define assert(ex) {if (!(ex)) \
    {char asstmp[256];(void)sprintf(asstmp,"rpc.pcnfsd: Assertion failed: line %d of %s\n", \
    __LINE__, __FILE__); (void)msg_out(asstmp); \
    sleep (10); exit(1);}}
#endif
