#ifndef NOIDENT
#ident	"@(#)dtadmin:dialup/dtcopy.h	1.10"
#endif

/*
 * Desktop remot file copy
 */

#define GGT		GetGizmoText


/* This is the path that will be used for mail command executions */
#define BIN             "PATH=/usr/bin "
#define LASTCHAR(s)     (s+strlen(s)-1)


/* Machine name length */

#define	UNAMESIZE	20

/* Link list of name=value structures */

typedef	struct	stringll {
	char	*name;
	char	*value;
	struct	stringll  *next;
} stringll_t;

#define	NULL_STRING	(stringll_t *)0

