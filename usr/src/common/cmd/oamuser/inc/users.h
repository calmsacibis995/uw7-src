#ident	"@(#)users.h	1.2"
#ident  "$Header$"

#define GROUP		"/etc/group"
#define	SKEL_DIR	"/etc/skel"
#define	CMT_SIZE	(128 + 1)	/* Argument sizes + 1 (for '\0') */
#define	DIR_SIZE	(256 + 1)
#define	SHL_SIZE	(256 + 1)
#define	LOGNAMSIZE	       32	/* Max length of a login name */
#define	ENTRY_LENGTH	      512	/* Max length of a passwd entry */

#define	SHELL			 "/usr/bin/sh"
#define	DATMSK		"DATEMSK=/etc/datemsk"

/* validation returns */
#define NOTUNIQUE	0	/* not unique */
#define RESERVED	1	/* reserved */
#define UNIQUE		2	/* is unique */
#define	TOOBIG		3
#define	INVALID		4

#define	REL_PATH(x)	(x && *x != '/')
#define	INVAL_CHAR(x)	((x) && (strchr((x), (int)':') != NULL))
