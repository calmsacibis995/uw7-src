#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:nfs/lookup.h	1.1"
#endif

	/* File lookup.h
	 * Header file to be included by the non-gui part of the library i.e.
	 * lookup.c
	 */

#ifndef _Boolean
#define _Boolean

typedef char            Boolean;

#define True 1
#define False 0

#endif /* _Boolean*/
#define EOS '\0'
#define MEDIUMBUFSIZE     256
#define HOST_ALLOC_SIZE   50

#define ETCHOSTS_PATH 	"/etc/hosts"
#define NSLOOKUP_PATH   "/usr/sbin/nslookup"
#define ETCCONF_PATH    "/etc/resolv.conf"
#define VAR_TMP		"/var/tmp"
#define VAR_TMP_ERR	"/var/tmp/lookup.err"
#define VAR_TMP_DB	"/var/tmp/lookup.db"
#define FS "\001"

typedef enum _status{ INVALID, VALID} status;

